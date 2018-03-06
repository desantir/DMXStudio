/* 
    Copyright (C) 2012-2016 Robert DeSantis
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

#include "HttpRestServices.h"
#include "SimpleJsonBuilder.h"
#include "Venue.h"
#include "PaletteMaker.h"
#include "MimeDecoder.h"

bool addTrackInfo( JsonBuilder& json, LPCSTR object_name, LPCSTR track_link, UINT track_number, TrackInfo* track_info );
void json_track_list( PlayerItems& tracks, CString& response );
void json_video_list( JsonBuilder& json, VideoPtrArray& videos );

// ----------------------------------------------------------------------------
//
void HttpRestServices::player_login( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    CString username;
    CString password;

    try {
        parser.parse( data );

        username = parser.get<CString>( "username" );
        password = parser.get<CString>( "password" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    JsonBuilder json( response );
    json.startObject();

    if ( studio.getMusicPlayer()->signon( username, password ) ) {
        json.add( "logged_in", true );
    }
    else {
        CString login_error = studio.getMusicPlayer()->getLastPlayerError();

        if ( login_error.IsEmpty() )
            login_error = "Login failed";

        json.add( "logged_in", false );
        json.add( "login_error", login_error );
    }

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_player_web_login( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    JsonBuilder json( response );
    json.startObject();
    json.add( "player_name", studio.getMusicPlayer()->getPlayerName() );
    json.add( "authorization_request_url", studio.getMusicPlayer()->getPlayerAuthorizationURL() );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_master_volume( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UINT master_volume;
        
    if ( sscanf_s( data, "%u", &master_volume ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setMasterVolume( master_volume );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_mute_volume( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UINT mute_volume;
        
    if ( sscanf_s( data, "%u", &mute_volume ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setMasterVolumeMute( mute_volume ? true : false );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_queue_track( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    char track_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", track_link, MAX_LINK_SIZE ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    studio.getMusicPlayer()->queueTrack( track_link );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_play_track( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    char        track_link[MAX_LINK_SIZE];
    DWORD       seek_ms = 0L;

    CString scan_data = data;
    scan_data.Replace( "/", " " );

    if ( sscanf_s( scan_data, "%s %lu", track_link, MAX_LINK_SIZE, &seek_ms ) < 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !studio.getMusicPlayer()->playTrack( track_link, seek_ms ) )
        throw RestServiceException( "Unable to play track" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_play_playlist( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    char        playlist_link[MAX_LINK_SIZE];

    CString scan_data = data;
    scan_data.Replace( "/", " " );

	if ( sscanf_s( scan_data, "%s", playlist_link, MAX_LINK_SIZE ) != 1 )
		throw RestServiceException( "Invalid service arguments" );

    if ( !studio.getMusicPlayer()->playAllTracks( playlist_link, false ) )
        throw RestServiceException( "Unable to play playlist" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_queue_playlist( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
	char        playlist_link[MAX_LINK_SIZE];

	CString scan_data = data;
	scan_data.Replace( "/", " " );

	if ( sscanf_s( scan_data, "%s", playlist_link, MAX_LINK_SIZE ) != 1 )
		throw RestServiceException( "Invalid service arguments" );

	if ( !studio.getMusicPlayer( )->playAllTracks( playlist_link, true ) )
		throw RestServiceException( "Unable to play playlist" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_track_back( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    studio.getMusicPlayer()->backTrack( );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_track_forward( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    studio.getMusicPlayer()->forwardTrack( );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_track_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    studio.getMusicPlayer()->stopTrack( );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_track_pause( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    studio.getMusicPlayer()->pauseTrack( true );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_music_track_play( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    studio.getMusicPlayer()->pauseTrack( false );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_track_analysis( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    char track_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", track_link, MAX_LINK_SIZE ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    AnalyzeInfo* info;

    if ( !studio.getMusicPlayer()->getTrackAnalysis( track_link, &info ) )
        throw RestServiceException( "Unable to fetch track analysis" );

    JsonBuilder json( response );

    json.startObject();
    json.add( "link", info->link );

    std::vector<uint16_t> amplitude_data( std::begin( info->data), &info->data[info->data_count] );

    json.startObject( "amplitude" );
    json.add( "duration_ms", info->duration_ms );
    json.add( "data_count", info->data_count );
    json.addArray< std::vector<uint16_t>>( "data", amplitude_data );
    json.endObject( "amplitude" );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_playlists( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    JsonBuilder json( response );
    json.startObject();

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );

	PlaylistInfo playlist_info;

    json.startArray( "playlists" );
    for ( PlayerItems::iterator it=playlists.begin(); it != playlists.end(); it++ ) {
		if ( studio.getMusicPlayer()->getPlaylistInfo((*it), &playlist_info) ) {
			json.startObject();
			json.add( "link", (*it) );
			json.add( "name", playlist_info.playlist_name );
			json.endObject();
		}
	}
    json.endArray( "playlists" );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_queued( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    PlayerItems tracks;
    studio.getMusicPlayer()->getQueuedTracks(tracks);

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_played( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    PlayerItems tracks;
    studio.getMusicPlayer()->getPlayedTracks(tracks);

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_playlist_tracks( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    char playlist_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", playlist_link, MAX_LINK_SIZE ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    PlayerItems tracks;
    studio.getMusicPlayer()->getTracks( playlist_link, tracks );

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_video_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
	char video_id[MAX_LINK_SIZE];

	if ( sscanf_s( data, "%s", video_id, MAX_LINK_SIZE ) != 1 )
		throw RestServiceException( "Invalid service arguments" );

    Video* video = studio.getVideoFinder( )->getCachedVideo( video_id );
	if ( video == NULL )
		throw RestServiceException( "Unable access palette for video %s", video_id );

	JsonBuilder json( response );

    json.startObject( );

    json.addArray<RGBWAArray>( "default_palette", video->m_default_palette );

    json.startArray( "video_palette" );
    for ( VideoPalette& p : video->m_video_palettes ) {
        json.startObject();
        json.add( "time_ms", p.m_time_ms );
        json.addArray<RGBWAArray>( "palette", p.m_palette );
        json.addArray<ColorWeights>( "weights", p.m_weights );
        json.endObject();
    }
    json.endArray( "video_palette" );

	json.endObject( );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_videos( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
	SimpleJsonParser parser;
	CString track_link;
	int count;

	try {
		parser.parse( data );
		track_link = parser.get<CString>( "track_link" );
		count = ( parser.has_key( "count" ) ) ? parser.get<int>( "count" ) : studio.getVideosPerTrack( );
	}
	catch ( std::exception& e ) {
		throw RestServiceException( "JSON parser error (%s) data (%s)", e.what( ), data );
	}

	JsonBuilder json( response );
	TrackInfo track_info;

	json.startObject( );

	if ( !addTrackInfo( json, "summary", track_link, 1, &track_info ) )
		throw RestServiceException( "Unable to collect track information for '%s'", track_link );

	json_video_list( json, studio.getVideoFinder( )->find( &track_info, count ) );

	json.endObject( );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_video_search( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
	SimpleJsonParser parser;
	CString search;
	int count = 25;

	try {
		parser.parse(data);
		search = parser.get<CString>("search");
		count = parser.get<int>( "count" );
	}
	catch (std::exception& e) {
		throw RestServiceException("JSON parser error (%s) data (%s)", e.what(), data);
	}

	VideoPtrArray videos = studio.getVideoFinder()->find( search, count );

    JsonBuilder json( response );
    json.startObject();
    json_video_list( json, videos );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_video_ids( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    std::vector<CString> video_ids;

    try {
        parser.parse(data);
        video_ids = parser.getArrayAsVector<CString>("video_ids");
    }
    catch (std::exception& e) {
        throw RestServiceException("JSON parser error (%s) data (%s)", e.what(), data);
    }

	VideoPtrArray videos = studio.getVideoFinder()->find( video_ids );

    JsonBuilder json( response );

    json.startObject();
    json_video_list( json, videos );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::create_video_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    SimpleJsonParser parser;
    CString video_id;
    bool true_black;
    unsigned palette_size;

    try {
        parser.parse(data);
        video_id = parser.get<CString>( "video_id" );
        palette_size = parser.get<unsigned>( "palette_size", 8 );
        true_black = parser.get<bool>( "true_black", false );
    }
    catch (std::exception& e) {
        throw RestServiceException("JSON parser error (%s) data (%s)", e.what(), data);
    }

    bool scheduled = studio.getVideoFinder( )->scheduleVideoPaletteProduction( video_id, palette_size, true_black );

    JsonBuilder json( response );

    json.startObject();
    json.add( "success", scheduled );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::create_image_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    SimpleJsonParser parser;
    CString video_id;
    bool true_black;
    unsigned palette_size;

    try {
        parser.parse(data);
        video_id = parser.get<CString>( "video_id" );
        palette_size = parser.get<unsigned>( "palette_size", 8 );
        true_black = parser.get<bool>( "true_black", false );
    }
    catch (std::exception& e) {
        throw RestServiceException("JSON parser error (%s) data (%s)", e.what(), data);
    }

    bool scheduled = false;

    Video* video = studio.getVideoFinder( )->getCachedVideo( video_id );
    if ( video != NULL ) {
        scheduled = studio.getVideoFinder( )->
            schedulePaletteProduction( video_id, video->m_thumbnails["medium"].m_url, studio.getVideosPaletteSize(), false );
    }

    JsonBuilder json( response );

    json.startObject();
    json.add( "success", scheduled );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::generate_image_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    MimeDecoder message( (LPBYTE)data, size );
    
    CString field_name, type;
    int num_colors = 16;
    JpegImage image;

    while ( message.nextPart( field_name, type ) ) {
        if ( field_name == "cpd_colors" ) {
            CString color_str;
            if ( !message.getNextLine( color_str ) ) 
                throw RestServiceException( "Invalid multipart form - field data missing" );

            num_colors = atoi( (LPCSTR)color_str );
        }
        else if ( field_name == "jpg_file" ) {
            if ( type != "image/jpeg" ) {
                JsonBuilder json( response );
                json.startObject();
                json.add( "success", false ); 
                json.add( "error", "Not a JPEG image" );
                json.endObject();
                return;
            }

            DWORD pos = message.findString( message.getMarker() );
            if ( pos == -1 ) 
                throw RestServiceException( "Invalid multipart form - cannot find end marker" );

            image.readJPEG( NULL, message.getBuffer(), pos );

            message.advance( pos );
        }
    }

    RGBWAArray palette;
    ColorWeights weights;

    bool success = generatePalette( image, num_colors, palette, weights );

    JsonBuilder json( response );

    json.startObject();
    json.add( "success", true );
    json.addArray<RGBWAArray>( "palette", palette );
    json.addArray<ColorWeights>( "weights", weights );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_matcher( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    MusicSceneSelectMap& mm = venue->music_scene_select_map();
    JsonBuilder json( response );

    json.startArray( );

    json.startObject();
    if ( mm.find( SILENCE_TRACK_LINK ) != mm.end() ) {
        MusicSceneSelector& silence = mm[SILENCE_TRACK_LINK];
        json.add( "track", silence.m_track_full_name );
        json.add( "id", silence.m_selection_uid );
        json.add( "type", silence.m_selection_type );
        json.add( "link", SILENCE_TRACK_LINK );
        json.add( "special", true );
    }
    else {
        json.add( "track", SILENCE_TRACK_NAME );
        json.add( "id", venue->getDefaultScene()->getUID() );
        json.add( "type", MST_SCENE );
        json.add( "link", SILENCE_TRACK_LINK );
        json.add( "special", true );
    }
    json.endObject();

    json.startObject();
    if ( mm.find( UNMAPPED_TRACK_LINK ) != mm.end() ) {
        MusicSceneSelector& unmapped = mm[UNMAPPED_TRACK_LINK];
        json.add( "track", unmapped.m_track_full_name );
        json.add( "id", unmapped.m_selection_uid );
        json.add( "type", unmapped.m_selection_type );
        json.add( "link", UNMAPPED_TRACK_LINK );
        json.add( "special", true );
    }
    else {
        json.add( "track", UNMAPPED_TRACK_NAME );
        json.add( "id", 0 );
        json.add( "type", MST_RANDOM_SCENE );
        json.add( "link", UNMAPPED_TRACK_LINK );
        json.add( "special", true );
    }
    json.endObject();

    for ( MusicSceneSelectMap::iterator it=mm.begin(); it != mm.end(); ++it ) {
        MusicSceneSelector& mss = it->second;

        if ( mss.m_track_link == SILENCE_TRACK_LINK || 
             mss.m_track_link == UNMAPPED_TRACK_LINK )
            continue;

        json.startObject();
        json.add( "track", mss.m_track_full_name );
        json.add( "link", mss.m_track_link );
        json.add( "id", mss.m_selection_uid );
        json.add( "type", mss.m_selection_type );
        json.endObject();
    }
    json.endArray( );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_music_matcher_search( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    char playlist_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", playlist_link, MAX_LINK_SIZE ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    MusicSceneSelectMap& mm = venue->music_scene_select_map();
    JsonBuilder json( response );

    json.startArray( );

    MusicSceneSelector selector;

    if ( venue->findMusicMapping( playlist_link, selector ) ) {
        json.startObject();

        json.add( "track", selector.m_track_full_name );
        json.add( "id", selector.m_selection_uid );
        json.add( "type", selector.m_selection_type );
        json.add( "link", selector.m_track_link );
        json.add( "special", selector.isSpecialTrack() );
  
        json.endObject();
    }

    json.endArray( );
}

// ----------------------------------------------------------------------------
//
static void music_matcher_load( Venue* venue, LPCSTR data, boolean clearFirst ) {
    SimpleJsonParser parser;

    std::vector<MusicSceneSelector> selections;

    try {
        parser.parse( data );

        JsonNodePtrArray selection_parsers = parser.getObjects();

        for ( JsonNode* selection : selection_parsers ) {
            UID selection_id = selection->get<UID>( "id" );
            CString selection_name = selection->get<CString>( "track" );
            CString selection_link = selection->get<CString>( "link" );
            MusicSelectorType selection_type = (MusicSelectorType)selection->get<int>( "type" );

            selections.emplace_back( selection_name, selection_link, selection_type, selection_id );
        }
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Reload the selection map
    if ( clearFirst )
        venue->clearMusicMappings();

    venue->addMusicMappings( selections );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_music_matcher_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    music_matcher_load( venue, data, true );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_music_matcher_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    music_matcher_load( venue, data, false );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::musicPlayerToJson( JsonBuilder& json )
{
    // If we have a music player, return player status
    if ( studio.hasMusicPlayer() ) {
        json.startObject( "music_player" );
        json.add( "player_name", studio.getMusicPlayer()->getPlayerName() );
        json.add( "username", studio.getMusicPlayer()->getUsername() );

        if ( studio.getMusicPlayer()->isLoggedIn() ) {
            PlayingInfo playing_info;

            bool success = studio.getMusicPlayer()->getPlayingTrack( &playing_info );

            json.add( "logged_in", true );
            json.add( "queued", playing_info.queued_tracks );
            json.add( "played", playing_info.previous_tracks );

            if ( success ) {
                json.startObject( "playing" );
                json.add( "link", playing_info.track_link );
                json.add( "name", studio.getMusicPlayer()->getTrackFullName( playing_info.track_link ) );
                json.add( "length", playing_info.track_length );
                json.add( "remaining", playing_info.time_remaining );
                json.add( "paused", studio.getMusicPlayer()->isTrackPaused() );

                AudioInfo audio_info;
                if ( studio.getMusicPlayer()->getTrackAudioInfo( playing_info.track_link, &audio_info, 0L ) == OK ) {
                    json.add( "bpm", audio_info.tempo );
                    json.add( "key", audio_info.key );
                }

                json.endObject( "playing" );
            }
            else
                json.addNull( "playing" );
        }
        else {
            json.add( "logged_in", false );
        }

        CString last_error = studio.getMusicPlayer()->getLastPlayerError( );
        if ( !last_error.IsEmpty() )
            json.add( "player_error", last_error );

        json.endObject( "music_player" );
    }
    else
        json.addNull( "music_player" );
}

// ----------------------------------------------------------------------------
//
void json_track_list( PlayerItems& tracks, CString& response )
{
    UINT track_number = 1;
    TrackInfo track_info;

    JsonBuilder json( response );
    json.startObject();

    json.startArray( "tracks" );
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ )
        addTrackInfo( json, NULL, (*it), track_number, &track_info );
    json.endArray( "tracks" );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
bool addTrackInfo( JsonBuilder& json, LPCSTR object_name, LPCSTR track_link, UINT track_number, TrackInfo* track_info ) {
    AudioInfo audioInfo;

    if ( !studio.getMusicPlayer()->getTrackInfo( track_link, track_info ) )
        return false;

    json.startObject( object_name );
    json.add( "link", track_link );
    json.add( "number", track_number );
    json.add ("full_name", studio.getMusicPlayer()->getTrackFullName( track_link ) );    // Legacy for music match
    json.add( "track_name", track_info->track_name );
    json.add( "artist_name", track_info->artist_name );
    json.add( "album_name", track_info->album_name );
    json.add( "duration", track_info->track_duration_ms ); 

    // Collect any audio info available in the cache and queue up the rest
    if ( studio.getMusicPlayer()->getTrackAudioInfo( track_link, &audioInfo, 0L ) == OK ) {
        json.startObject( "audio_info" );
        json.add( "key", audioInfo.key );
        json.add( "mode", audioInfo.mode );
        json.add( "bpm", audioInfo.tempo );
        json.endObject( "audio_info" );
    }

	for ( size_t index = 0; index < 3; index++ ) {
		if ( track_info->image[index].m_width > 0 && track_info->image[index].m_width <= 120 ) {
			json.add( "href_sm_image", track_info->image[index].m_href );
		}
		else if ( track_info->image[index].m_width > 120 && track_info->image[index].m_width <= 300 ) {
			json.add( "href_lg_image", track_info->image[index].m_href );
		}
	}

    json.endObject( object_name );

    return true;
}

void json_video_list( JsonBuilder& json, VideoPtrArray& videos )
{
    json.startArray( "videos" );

    try {
        for ( Video* video : videos ) {
            json.startObject();
            json.add( "video_id", video->m_video_id );
            json.add( "title", video->m_title );
            json.add( "description", video->m_description );
            json.add( "duration", video->m_duration );

            json.startObject( "thumbnails" );
            for ( VideoThumbnailMap::value_type& t : video->m_thumbnails ) {
                json.startObject( t.first );
                json.add( "url", t.second.m_url );
                json.add( "height", t.second.m_height );
                json.add( "width", t.second.m_width );
                json.endObject( t.first );
            }
            json.endObject( "thumbnails" );

            json.addArray<RGBWAArray>( "default_palette", video->m_default_palette );
            json.addArray<ColorWeights>( "default_weights", video->m_default_weights );

            json.startArray( "video_palette" );
            for ( VideoPalette& p : video->m_video_palettes ) {
                json.startObject();
                json.add( "time_ms", p.m_time_ms );
                json.addArray<RGBWAArray>( "palette", p.m_palette );
                json.addArray<ColorWeights>( "weights", p.m_weights );
                json.endObject();
            }
            json.endArray( "video_palette" );

            json.endObject();
        }
    }
    catch ( std::exception& e ) {
        throw RestServiceException( e.what() );
    }

    json.endArray( "videos" );
}
