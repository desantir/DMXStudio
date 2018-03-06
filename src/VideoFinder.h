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
#pragma once

#include "stdafx.h"

struct VideoThumbnail {
    CString     m_name;
    CString     m_url;
    UINT        m_width;
    UINT        m_height;

    VideoThumbnail( LPCSTR name, LPCSTR url, UINT width, UINT height ) :
        m_name( name ),
        m_url( url ),
        m_width( width ),
        m_height( height )
    {}

    VideoThumbnail() {}
};

typedef std::map<CString, VideoThumbnail> VideoThumbnailMap;

struct VideoPalette {
    DWORD           m_time_ms;
    RGBWAArray      m_palette;
    ColorWeights    m_weights;

    VideoPalette( DWORD time_ms, RGBWAArray& palette, ColorWeights& weights ) :
        m_time_ms( time_ms ),
        m_palette( palette ),
        m_weights( weights )
    {}

    VideoPalette()
    {}
};

typedef std::vector<VideoPalette> VideoPaletteArray;

struct Video {
    CString             m_video_id;
    CString             m_title;
    CString             m_description;
    VideoThumbnailMap   m_thumbnails;
    RGBWAArray          m_default_palette;
    ColorWeights        m_default_weights;
    VideoPaletteArray   m_video_palettes;
    CString             m_duration;                         // ISO 8601 time duration e.g. PT#M#S

    Video()
    {}

    Video( LPCSTR video_id, LPCSTR title, LPCSTR description, VideoThumbnailMap& thumbnails, RGBWAArray default_palette, 
        VideoPaletteArray& video_palettes ) :
        m_video_id( video_id ),
        m_title( title ),
        m_description( description ),
        m_thumbnails( thumbnails ),
        m_default_palette( default_palette ),
        m_video_palettes( video_palettes )
    {}
};

struct PaletteProduction {
	CString             m_video_id;
	CString             m_image_url;
    UINT                m_palette_size;
    bool                m_true_black;

	PaletteProduction() 
	{}

	PaletteProduction( LPCSTR video_id, PCSTR image_url, UINT palette_size, bool true_black=false) :
		m_video_id( video_id ),
		m_image_url( image_url ),
        m_palette_size( palette_size ),
        m_true_black( true_black )
	{}

    inline bool isVideoPalette() const {
        return m_image_url.IsEmpty();
    }
};

typedef std::vector<Video *> VideoPtrArray;
typedef std::map<CString, Video> VideoMap;
typedef std::vector<PaletteProduction> PaletteProductionList;
typedef std::map<CString,VideoPtrArray> VideoSearchMap;

struct Storyboard {
    CString             m_url_pattern;          // Like "https://i9.ytimg.com/sb/_VyaeyXD374/storyboard3_L$L/$N.jpg" ($L and $N will be resolved)
    UINT                m_image_width;          // Height and width of each snapshot
    UINT                m_image_height;
    UINT                m_image_count;          // Total number of snapshots across all pages
    UINT                m_page_rows;            // Snapshow rows and columns per page
    UINT                m_page_cols;
    CString             m_sigh;
    ULONG               m_duration;             // Duration of each clip in MS

    Storyboard( LPCSTR url_pattern, UINT image_width, UINT image_height, UINT image_count, UINT rows, UINT columns, LPCSTR sigh, ULONG duration ) :
        m_url_pattern( url_pattern ),
        m_image_width( image_width ),
        m_image_height( image_height ),
        m_image_count( image_count ),
        m_page_rows( rows ),
        m_page_cols( columns ),
        m_sigh( sigh ),
        m_duration( duration )
    {}

    CString makePageURI( UINT page ) const {
        char int_buffer[48];
        CString uri( m_url_pattern );
        _itoa_s( page, int_buffer, 10 );
        uri.Replace( "$M", int_buffer );
        uri.AppendFormat( "?sigh=%s", (LPCSTR)m_sigh );
        return uri;
    }
};

typedef std::vector<Storyboard> StoryboardArray;

class VideoFinder : public Threadable
{
    CString					m_paletteContainer;
	CCriticalSection		m_palette_lock;					// Palette lock (controls palette production)
	CEvent					m_wake;							// Wakes up pallete production
    PaletteProductionList	m_produce;						// Palettes to produce
	CCriticalSection		m_video_cache_lock;				// Control access to track video cache

	UINT run(void);

public:
    VideoFinder();
    ~VideoFinder();

    Video* getCachedVideo( LPCSTR video_id );
    VideoPtrArray find( TrackInfo* track_info, int count );
    VideoPtrArray find( LPCSTR search, int count );
    VideoPtrArray find( std::vector<CString> video_ids );
    bool scheduleVideoPaletteProduction( LPCSTR video_id, UINT palette_size, bool true_black );
    bool schedulePaletteProduction( LPCSTR video_id, LPCSTR url, UINT palette_size, bool true_black=false );

private:
    static VideoMap videoCache;
    static VideoSearchMap videoSearchCache;

    void produceThumbnailPalette( PaletteProduction& produce );
    void produceVideoPalette( PaletteProduction& produce );
    bool getPalette( Video& video );
    void convertToTrueBlack( RGBWAArray& palette );

    StoryboardArray readStoryboards( LPCSTR video_id );

    VideoPtrArray loadVideos( LPCSTR search_url );
};

