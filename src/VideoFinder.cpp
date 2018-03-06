/* 
Copyright (C) 2017 Robert DeSantis
hopluvr at gmail dot com

This file is part of DMX Studio.

DMX Studio is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

DMX Studio is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with DMX Studio; see the file _COPYING.txt.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.
*/

#include "DMXStudio.h"
#include "VideoFinder.h"
#include "SimpleJsonParser.h"
#include "PaletteMaker.h"
#include "SimpleJsonBuilder.h"
#include "SimpleJsonParser.h"
#include "HttpUtils.h"

#include <Winhttp.h>
#include <Shlwapi.h>

VideoMap VideoFinder::videoCache;
VideoSearchMap VideoFinder::videoSearchCache;

// ----------------------------------------------------------------------------
//
VideoFinder::VideoFinder() :
	Threadable( "VideoFinder.PaletteProduction" )
{
    m_paletteContainer.Format( "%s\\DMXStudio\\VideoPaletteCache", studio.getUserDocumentDirectory() );
    CreateDirectory( m_paletteContainer, NULL );
}

// ----------------------------------------------------------------------------
//
VideoFinder::~VideoFinder()
{
}

// ----------------------------------------------------------------------------
//
Video* VideoFinder::getCachedVideo( LPCSTR video_id ) {
    CSingleLock lock( &m_video_cache_lock, TRUE );

    VideoMap::iterator it = videoCache.find( video_id );
    if ( it != videoCache.end() )
        return &it->second;

    return NULL;
}

// ----------------------------------------------------------------------------
//
VideoPtrArray VideoFinder::find( TrackInfo* track_info, int count ) {
	CSingleLock lock( &m_video_cache_lock, TRUE );

	CString search_cache_key;
    search_cache_key.Format( "%s-%d", track_info->track_link, count );

    VideoSearchMap::iterator it = videoSearchCache.find( search_cache_key );
	if ( it != videoSearchCache.end( ) )
		return it->second;

    CString search;

	search.Format( "%s %s music video", track_info->track_name, track_info->artist_name );

    m_video_cache_lock.Unlock();

    VideoPtrArray videos = find( (LPCSTR)search, count );

    m_video_cache_lock.Lock();

    // Add the video list to our cache
    videoSearchCache[ search_cache_key ] = videos;

    return videos;
}

// ----------------------------------------------------------------------------
//
VideoPtrArray VideoFinder::find( LPCSTR search, int video_count ) {
    CString key( search );

    // HACK - deal with UTF-8 single quote
    key.Replace( "\xe2\x80\x99", "'" );

    CString url;

    video_count = std::min<int>( video_count, 50 );

    url.Format( "/youtube/v3/search?part=snippet&q=%s&maxResults=%u&key=%s&orderby=rating", encodeString(key), video_count, studio.getGoogleAPIKey() );

    return loadVideos( url );
}

// ----------------------------------------------------------------------------
//
VideoPtrArray VideoFinder::find( std::vector<CString> video_ids ) {
    CSingleLock lock( &m_video_cache_lock, TRUE );

    CString id_list;
    
    // Get a list of IDs not in the cache
    for ( CString& id : video_ids ) {
        VideoMap::iterator it = videoCache.find( id );
        if ( it == videoCache.end() ) {
            if ( !id_list.IsEmpty() )
                id_list.Append( "," );
            id_list.Append( id );
        }
    }

    // Fetch videos not in the cache
    if ( !id_list.IsEmpty() ) {
        CString url;
		url.Format( "/youtube/v3/videos?part=snippet&id=%s&key=%s", (LPCSTR)id_list, studio.getGoogleAPIKey() );
		loadVideos( url );
    }

    // Return list of found videos
    VideoPtrArray results;
    for ( CString& id : video_ids ) {
        VideoMap::iterator it = videoCache.find( id );
        if ( it != videoCache.end() )
            results.push_back( &it->second );
    }

    return results;
}

// ----------------------------------------------------------------------------
//
VideoPtrArray VideoFinder::loadVideos( LPCSTR search_url ) {
    BYTE *buffer = NULL;
    ULONG buffer_size = 0L;

    SimpleJsonParser parser;

    try {
		DWORD status = httpGet( L"www.googleapis.com", search_url, NULL, &buffer, &buffer_size);
		if ( status != 200 )
			throw StudioException( "HTTP error %lu reading video metadata", status );

        buffer = (BYTE *)realloc( buffer, buffer_size+1 );
        buffer[buffer_size] = '\0';

        parser.parse( (LPCSTR)buffer );

        free( buffer );
    }
    catch ( std::exception& e ) {
        if ( buffer != NULL )
            free( buffer );

        throw StudioException( "Error obtaining search video data (%s): %s", search_url, e.what() );
    }

    VideoPtrArray videos;
    CString new_video_ids;                         // Track newly discovered (non-cached videos for additional processing)
    
    if ( !parser.has_key( "items" ) )            // Search returned no results
        return videos;

    CSingleLock lock( &m_video_cache_lock, TRUE );
    CString kind, video_id;

    try {
        for ( JsonNode* item : parser.getObjects( "items" ) ) {
            kind = item->get<CString>( "kind" );
            if ( kind == "youtube#searchResult" ) {
                JsonNode* id_node = item->getObject( "id" );
                kind = id_node->get<CString>( "kind" );
                if ( kind != "youtube#video" )
                    continue;       
                video_id = id_node->get<CString>( "videoId" );
            }
            else if ( kind == "youtube#video" ) {
                video_id = item->get<CString>( "id" );            
            }
            else
                continue;

            // At this point we should have a video
            JsonNode* snippet_node = item->getObject( "snippet" );

            VideoMap::iterator it = videoCache.find( video_id );
            if ( it != videoCache.end() ) {
                videos.push_back( &it->second );
                continue;
            }

            // New video - add it to the cache
            Video video;
            video.m_video_id = video_id;
            video.m_title = snippet_node->get<CString>( "title" );
            video.m_description = snippet_node->get<CString>( "description" );

            JsonNode* thumbnails_node = snippet_node->getObject( "thumbnails" );
                
            for ( CString& key : thumbnails_node->keys() ) {
                JsonNode* key_node = thumbnails_node->getObject( (LPCSTR)key );

                CString thumbnail_name = key_node->getTagName();
                CString url = key_node->get<CString>( "url" );
                UINT height = key_node->get<UINT>( "width" );
                UINT width = key_node->get<UINT>( "height" );

                video.m_thumbnails[ thumbnail_name ] = VideoThumbnail( thumbnail_name, url, width, height );
            }

            bool schedule_palette = !getPalette( video );

            Video& new_video = videoCache[video_id] = video;
            videos.push_back( &new_video );

            if ( schedule_palette ) {
                // Only use medium thumbnail in the UI at the moment
                VideoThumbnailMap::iterator it = video.m_thumbnails.find( "medium" );
                if ( it != video.m_thumbnails.end() ) // Schedule the work to produce a palette for the video
                    schedulePaletteProduction( video_id, it->second.m_url, studio.getVideosPaletteSize(), false );
            }

            // Track video ideas for video metadata request
            if ( !new_video_ids.IsEmpty() )
                new_video_ids.AppendChar( ',' );
            new_video_ids.Append( video_id );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "Error parsing video data (%s): %s", search_url, e.what() );
    }

    // If we have new videos, fetch additional information not in search
    if ( new_video_ids.IsEmpty() )
        return videos;

    try {
        CString url;

        url.Format( "/youtube/v3/videos?part=contentDetails&key=%s&id=%s", studio.getGoogleAPIKey(), (LPCSTR)new_video_ids );

        buffer = NULL;

        httpGet( L"www.googleapis.com", (LPCSTR)url, NULL, &buffer, &buffer_size);

        buffer = (BYTE *)realloc( buffer, buffer_size+1 );
        buffer[buffer_size] = '\0';

        parser.parse( (LPCSTR)buffer );

        free( buffer );

        for ( JsonNode* item : parser.getObjects( "items" ) ) {
            kind = item->get<CString>( "kind" );
            if ( kind != "youtube#video" )
                continue;

            VideoMap::iterator it = videoCache.find( item->get<CString>( "id" ) );
            if ( it != videoCache.end() ) {
                JsonNode* contentDetails_node = item->getObject( "contentDetails" );
                it->second.m_duration = contentDetails_node->get<CString>( "duration" );
            }
        }
    }
    catch ( std::exception& e ) {
        if ( buffer != NULL )
            free( buffer );

        studio.log( StudioException( "Error obtaining auxillary search video data (%s): %s", search_url, e.what() ) );
    }


    return videos;
}

// ----------------------------------------------------------------------------
//
bool VideoFinder::getPalette( Video& video ) {
	CString palette_filename;
	palette_filename.Format( "%s\\%s.palette.json", (LPCSTR)m_paletteContainer, video.m_video_id );

	if ( GetFileAttributes( palette_filename ) != INVALID_FILE_ATTRIBUTES ) {
		SimpleJsonParser parser;
		try {
            FILE* fp = _fsopen( palette_filename, "rt", _SH_DENYWR );
            if ( fp == NULL ) 
                throw StudioException( "Error opening %s (%d)", palette_filename, _errno() );

			parser.parse( fp );

            video.m_default_palette = parser.getArrayAsVector<RGBWA>( "default_palette" );
            video.m_default_weights = parser.getArrayAsVector<double>( "default_weights" );

            for ( JsonNode* node : parser.getObjects( "video_palettes" ) ) {
                DWORD time_ms = node->get<DWORD>( "time_ms" );
                RGBWAArray palette = node->getArrayAsVector<RGBWA>( "palette" );
                ColorWeights weights = node->getArrayAsVector<double>( "weights" );
                video.m_video_palettes.emplace_back( time_ms, palette, weights );
            }

            fclose( fp );

			return true;
		}
		catch ( std::exception& e ) {
			studio.log( e );
			// DeleteFile( palette_filename );
		}
	}

	return false;
}

// ----------------------------------------------------------------------------
//
bool VideoFinder::schedulePaletteProduction( LPCSTR video_id, LPCSTR url, UINT palette_size, bool true_black ) {
	CSingleLock lock( &m_palette_lock, TRUE );

	for ( PaletteProduction& prod : m_produce )
		if ( prod.m_video_id == video_id )
			return false;

	m_produce.emplace_back( video_id, url, palette_size, true_black );

	if ( !isRunning( ) )
		this->startThread( );

	m_wake.SetEvent( );

    return true;
}

// ----------------------------------------------------------------------------
//
bool VideoFinder::scheduleVideoPaletteProduction( LPCSTR video_id, UINT palette_size, bool true_black ) {
    return schedulePaletteProduction( video_id, "", palette_size, true_black );
}

// ----------------------------------------------------------------------------
//
UINT VideoFinder::run(void) {
	CSingleLock lock( &m_palette_lock, TRUE );

	while (isRunning()) {
		lock.Unlock( );

		if (::WaitForSingleObject(m_wake.m_hObject, INFINITE) == WAIT_OBJECT_0) {
			lock.Lock( );

			while ( m_produce.size( ) > 0 ) {
				// Get next work item
				PaletteProduction produce = m_produce.back();
				m_produce.pop_back();

				lock.Unlock( );

                try {
                    if ( produce.isVideoPalette() )
                        produceVideoPalette( produce ); 
                    else
                        produceThumbnailPalette( produce );
                }
                catch ( std::exception& e ) {
                    if ( produce.isVideoPalette() )
                        DMXStudio::fireEvent( ES_VIDEO_PALETTE, 0L, EA_ERROR, produce.m_video_id, 1 );

                    studio.log( e );
                }

				lock.Lock( );
			}
		}
	}

	return 0;
}

// ----------------------------------------------------------------------------
//
void VideoFinder::produceThumbnailPalette( PaletteProduction& produce ) {
    Video* video = getCachedVideo( produce.m_video_id );
    if ( video == NULL )
        throw StudioException( "Video missing from cache [ID %s]", video->m_video_id );

    BYTE *buffer = NULL;
    ULONG buffer_size = 0L;

    RGBWAArray default_palette;
    ColorWeights default_weights;

    try {
        DWORD status = httpGet( (LPCSTR)produce.m_image_url, &buffer, &buffer_size );
        if ( status != 200 )
            throw StudioException( "HTTP error %lu reading video thumbnail %s", status, (LPCSTR)produce.m_image_url );

        generatePalette( buffer, buffer_size, produce.m_palette_size, default_palette, default_weights );

        if ( produce.m_true_black )
            convertToTrueBlack( default_palette );

        free( buffer );
    }
    catch ( std::exception& e ) {
        if ( buffer != NULL )
            free( buffer );
        throw e;
    }

    // Save palette
    CString palette_filename;
    palette_filename.Format( "%s\\%s.palette.json", (LPCSTR)m_paletteContainer, produce.m_video_id );

    if ( PathFileExists( palette_filename ) )
        DeleteFile( palette_filename );

    JsonFileWriter fp( palette_filename );
    JsonBuilder builder( fp, true );

    builder.startObject( );
    builder.add( "video_id", produce.m_video_id );
    builder.add( "video_title", video->m_title );
    builder.add( "video_duration", video->m_duration );

    builder.addArray<RGBWAArray>( "default_palette", default_palette );
    builder.addArray<ColorWeights>( "default_weights", default_weights );
    builder.startArray( "video_palettes" );
    builder.endArray( "video_palettes" );
    builder.endObject( );
    
    fp.close();

    // Update video in cache
    video->m_default_palette = default_palette;
    video->m_default_weights = default_weights;

    studio.log( "Generated palette for %s", produce.m_image_url );

    DMXStudio::fireEvent( ES_VIDEO_PALETTE, 0L, EA_NEW, produce.m_video_id, 0 );
}

// ----------------------------------------------------------------------------
//
void VideoFinder::produceVideoPalette( PaletteProduction& produce ) {
    Video* video = getCachedVideo( produce.m_video_id );
    if ( video == NULL )
        throw StudioException( "Video missing from cache [ID %s]", video->m_video_id );

    studio.log_status( "Generating video palette for '%s' [ID %s]", video->m_title, video->m_video_id );

    StoryboardArray storyboards = readStoryboards( video->m_video_id );
    Storyboard* sb = NULL;

    // Find the L2 10x10 storyboard
    for ( Storyboard& s : storyboards )
        if ( s.m_url_pattern.Find( "_L2/" ) != -1 ) {
            sb = &s;
            break;
        }
            
    if ( sb == NULL )                       // Didn't find the one we want
        throw StudioException( "Video palette build for '%s' failed (Unable to find L2 storyboard) [ID %s]", video->m_title, video->m_video_id  );

    UINT image_counter = 0;                 // Track the number of images we have analyzed
    JpegImage image;
    UINT row=sb->m_page_rows, col=sb->m_page_cols;
    UINT page = 0;
    BYTE *buffer = NULL;
    ULONG buffer_size = 0L;
    VideoPaletteArray video_palettes;

    ColorWeights weights;
    RGBWAArray palette;

    while ( image_counter < sb->m_image_count ) {
        if ( col >= sb->m_page_cols ) {
            col = 0;
            row++;
        }

        if ( row >= sb->m_page_rows ) {
            try {
                CString uri = sb->makePageURI( page );

                DWORD status = httpGet( (LPCSTR)uri, &buffer, &buffer_size );
                if ( status != 200 )
                    throw StudioException( "HTTP error %lu reading video palette storyboard %s", status, (LPCSTR)uri );

                image.readJPEG( NULL, buffer, buffer_size );

                free( buffer );

                row = col = 0;
                page++;
            }
            catch ( std::exception& e ) {
                if ( buffer != NULL )
                    free( buffer );
                throw e;
            }
        }

        unsigned x = col * sb->m_image_width;
        unsigned y = row * sb->m_image_height;

        generatePalette( image, x, y, sb->m_image_width, sb->m_image_height, produce.m_palette_size, palette, weights );

        if ( produce.m_true_black )
            convertToTrueBlack( palette );

        video_palettes.emplace_back( image_counter * sb->m_duration, palette, weights );

        col++;
        image_counter++;
    }

    // Update video in cache
    video->m_video_palettes = video_palettes;

    // Update the palette file
    CString palette_filename;
    palette_filename.Format( "%s\\%s.palette.json", (LPCSTR)m_paletteContainer, video->m_video_id );

    if ( PathFileExists( palette_filename ) )
        DeleteFile( palette_filename );

    JsonFileWriter fp( palette_filename );
    JsonBuilder builder( fp, true );

    builder.startObject( );
    builder.add( "video_id", video->m_video_id );
    builder.add( "video_title", video->m_title );
    builder.add( "video_duration", video->m_duration );

    builder.addArray<RGBWAArray>( "default_palette", video->m_default_palette );
    builder.addArray<ColorWeights>( "default_weights", video->m_default_weights );

    builder.startArray( "video_palettes" );

    for ( VideoPalette& p : video_palettes ) {
        builder.startObject();
        builder.add( "time_ms", p.m_time_ms );
        builder.addArray<RGBWAArray>( "palette", p.m_palette );
        builder.addArray<ColorWeights>( "weights", p.m_weights );
        builder.endObject();
    }

    builder.endArray( "video_palettes" );
    builder.endObject( );

    fp.close();

    studio.log( "Generated video palette for video ID %s", (LPCSTR)produce.m_video_id );

    DMXStudio::fireEvent( ES_VIDEO_PALETTE, 0L, EA_NEW, produce.m_video_id, 1 );
}

// ----------------------------------------------------------------------------
//
StoryboardArray VideoFinder::readStoryboards( LPCSTR video_id ) {
    CString uri;
    uri.Format( "https://www.youtube.com/get_video_info?video_id=%s&asv=3&el=detailpage&hl=en_US", video_id );

    BYTE *buffer = NULL;
    ULONG buffer_size = 0L;

    StoryboardArray storyboards;
    CString results;

    try {
        DWORD status = httpGet( (LPCSTR)uri, &buffer, &buffer_size );
        if ( status != 200 )
            throw StudioException( "HTTP error %lu reading video '%s' information (URI %s)", status, video_id, (LPCSTR)uri );

        memcpy( results.GetBufferSetLength( buffer_size ), buffer, buffer_size );
        results.ReleaseBufferSetLength( buffer_size );
        free( buffer );
    }
    catch ( std::exception& e ) {
        if ( buffer != NULL )
            free( buffer );
        throw e;
    }

    PropertyMap video_info;
    int index = 0;

    while ( true ) {
        CString nvp = results.Tokenize( "&", index );
        if ( index == -1 )
            break;

        size_t equal_pos = nvp.Find( '=' );
        if ( equal_pos == -1 )
            continue;

        CString name = nvp.Mid( 0, equal_pos );

        if ( name == "storyboard_spec" || name == "status" || name == "reason" || name == "errorcode" ) {
            video_info[name] = decodeString( nvp.Mid( equal_pos + 1 ) );
        }
    }

    if ( video_info["status"] == "fail" )
        throw StudioException( "Video palette build for '%s' failed (%s)", video_id, video_info["reason"] );

    PropertyMap::iterator it = video_info.find( "storyboard_spec" );
    if ( it == video_info.end() || it->second.IsEmpty() )
        throw StudioException( "Video palette build for '%s' failed (Storyboard not available)", video_id );

    /*
    storyboard_spec=https://i9.ytimg.com/sb/_VyaeyXD374/storyboard3_L$L/$N.jpg|
    48#27#100#10#10#0#default#rs$AOn4CLAQFJqHJyj-5KogK2INS6LQUOrkAg|
    79#45#113#10#10#2000#M$M#rs$AOn4CLCNV7lqAOUyqZG86ve5ao8WKfxNGw|
    159#90#113#5#5#2000#M$M#rs$AOn4CLB493CB_pyImXWFgYBlTTQx2KJRZw
    */

    index = 0;

    UINT level = 0;
    char int_buffer[48];
    CString& storyboard_spec = it->second;

    CString url = storyboard_spec.Tokenize( "|", index );
    if ( index == -1 )
        throw StudioException( "Unable to find storyboard spec for video ID %s (separator not found)", video_id );

    while ( true ) {
        CString storyboard_data = storyboard_spec.Tokenize( "|", index );
        if ( index == -1 )
            break;

        int data_index = 0;
        CString image_width = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString image_height = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString image_count = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString image_rows = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString image_columns = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString duration = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString name_pattern = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;
        CString sigh = storyboard_data.Tokenize( "#", data_index );
        if ( index == -1 )
            break;

        _itoa_s( level, int_buffer, 10 );

        CString board_url( url );
        board_url.Replace( "$L", int_buffer );
        board_url.Replace( "$N", (LPCSTR)name_pattern );

        storyboards.emplace_back( (LPCSTR)board_url, 
                                    atoi( image_width ),
                                    atoi( image_height ),
                                    atoi( image_count ),
                                    atoi( image_rows ),
                                    atoi( image_columns ),
                                    sigh,
                                    atoi( duration ) );
        level++;
    }

    return storyboards;
}

// ----------------------------------------------------------------------------
//
void VideoFinder::convertToTrueBlack( RGBWAArray& palette ) {
    static BYTE black_threshold = 16;

    for ( RGBWA& color : palette ) {
        if ( color.red() < black_threshold && color.green() < black_threshold && color.blue() < black_threshold )
            color = RGBWA::BLACK;
    }
}


// ----------------------------------------------------------------------------
//
void testVideoPalette() {
    BYTE *buffer = NULL;
    ULONG buffer_size = 0L;

    CStringW serverW( L"i.ytimg.com" );
    RGBWAArray palette;
    ColorWeights weights;

    try {
        httpGet( serverW, "/vi/1aMLXLBMczE/mqdefault.jpg", NULL, &buffer, &buffer_size );

        generatePalette( buffer, buffer_size, studio.getVideosPaletteSize( ), palette, weights );

        for ( RGBWA& rgb : palette )
            printf( "%s\n", (LPCSTR)rgb.asString() );

        free( buffer );
    }
    catch ( ... ) {
        if ( buffer != NULL )
            free( buffer );
    }
}

/*
https: //www.googleapis.com/youtube/v3/search?part=snippet&q=ry+x+only+video&maxResults=10&key=AIzaSyAyffXNnEzwUcOdwzTmaBHeEQ2t2L1AF_0&orderby=rating&category=??

https: //www.youtube.com/watch?v=_601kPxo1lQ

{
  "kind": "youtube#searchListResponse",
  "etag": "\"gMxXHe-zinKdE9lTnzKu8vjcmDI/5nQ_3miuU86AcG3cJoLcK0jK_r4\"",
  "nextPageToken": "CAoQAA",
  "regionCode": "US",
  "pageInfo": {
    "totalResults": 217815,
    "resultsPerPage": 10
  },
  "items": [
    {
      "kind": "youtube#searchResult",
      "etag": "\"gMxXHe-zinKdE9lTnzKu8vjcmDI/W0DYbfJ1LXTKwULTEuNYXdp29LQ\"",
      "id": {
        "kind": "youtube#video",
        "videoId": "bbokXheXhxY"
      },
      "snippet": {
        "publishedAt": "2016-03-24T23:59:14.000Z",
        "channelId": "UCGUUYg1OxfDSpfZK5K073ZA",
        "title": "RY X - Only (Official Video)",
        "description": "Directed by RY X and Dugan O'Neal The album \"Dawn\" is out now on iTunes: http://found.ee/RyX_DawniT // Spotify: http://found.ee/RYX_DawnSpfy // Amazon: ...",
        "thumbnails": {
          "default": {
            "url": "https://i.ytimg.com/vi/bbokXheXhxY/default.jpg",
            "width": 120,
            "height": 90
          },
          "medium": {
            "url": "https://i.ytimg.com/vi/bbokXheXhxY/mqdefault.jpg",
            "width": 320,
            "height": 180
          },
          "high": {
            "url": "https://i.ytimg.com/vi/bbokXheXhxY/hqdefault.jpg",
            "width": 480,
            "height": 360
          }
        },
        "channelTitle": "RY X",
        "liveBroadcastContent": "none"
      }
    },
  ]
}

*/