/* 
Copyright (C) 2011,2012 Robert DeSantis
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

#include "DMXHttpServer.h"
#include "DObject.h"
#include "DMXHttpRestServices.h"
#include "MusicPlayer.h"
#include "SoundSampler.h"
#include "BeatDetector.h"
#include "SimpleJsonParser.h"

#define DMX_URL_ROOT_FULL                   DMX_URL_ROOT "full/"
#define DMX_URL_FULL_HOME                   DMX_URL_ROOT_FULL "full.htm"

class BeatBin
{
    unsigned    m_start_freq;
    unsigned    m_end_freq;
    CEvent      m_beat;

    BeatBin& operator=( BeatBin& rhs );

public:
    BeatBin( unsigned start_freq, unsigned end_freq ) :
        m_start_freq( start_freq ),
        m_end_freq( end_freq )
    { }

    BeatBin( BeatBin& other ) {
        m_start_freq = other.m_start_freq;
        m_end_freq = other.m_end_freq;
    }

    inline unsigned getStartFreq( ) const {
        return m_start_freq;
    }

    inline unsigned getEndFreq( ) const {
        return m_end_freq;
    }

    inline CEvent* getEvent() {
        return &m_beat;
    }

    bool inline isBeat( ) {
        return ( ::WaitForSingleObject( m_beat.m_hObject, 0 ) == WAIT_OBJECT_0 );
    }
};

typedef std::vector<BeatBin> BeatBinArray;

class DMXHttpFull : public DMXHttpRestServices
{
    typedef bool (DMXHttpFull::*RestGetHandlerFunc)( CString& response, LPCSTR data );
    typedef std::map<CString, RestGetHandlerFunc> RestGetHandlerMap;

    typedef bool (DMXHttpFull::*RestPostHandlerFunc)( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    typedef std::map<CString, RestPostHandlerFunc> RestPostHandlerMap;

    enum EditMode {
        NEW = 1,
        UPDATE = 2,
        COPY = 3
    };

    RestGetHandlerMap       m_rest_get_handlers;
    RestPostHandlerMap      m_rest_post_handlers;

    // TODO these should be made client specific ( and shouldn't be here )
    SoundSampler            m_sound_sampler;
    BeatDetector            m_beat_sampler;
    BeatBinArray            m_beats;

public:
    DMXHttpFull(void);
    ~DMXHttpFull(void);

    LPCSTR getPrefix() {
        return DMX_URL_ROOT_FULL;
    }

    UINT getPort() {
        return studio.getHttpPort();
    }

    DWORD processGetRequest( HttpWorkerThread* worker );
    DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size );
    bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content ) { return false; }

private:
    bool query_scenes( CString& response, LPCSTR data );
    bool query_fixtures( CString& response, LPCSTR data );
    bool query_chases( CString& response, LPCSTR data );
    bool query_venue_describe( CString& response, LPCSTR data );
    bool query_fixture_definitions( CString& response, LPCSTR data );
    bool query_venue_layout( CString& response, LPCSTR data );

    bool control_soundsampler_start( CString& response, LPCSTR data );
    bool control_soundsampler_stop( CString& response, LPCSTR data );
    bool query_soundsampler( CString& response, LPCSTR data );

    bool control_beatsampler_start( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool control_beatsampler_stop( CString& response, LPCSTR data );
    bool query_beatsampler( CString& response, LPCSTR data );

    bool delete_scene( CString& response, LPCSTR data );
    bool delete_chase( CString& response, LPCSTR data );
    bool delete_fixture( CString& response, LPCSTR data );
    bool delete_fixturegroup( CString& response, LPCSTR data );

    // POST handlers

    bool control_fixture_channels( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool control_fixture( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool control_fixture_group( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool edit_scene_copy_fixtures( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool edit_scene( CString& response, LPCSTR data, EditMode mode );
    bool edit_scene_create( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_scene( response, data, NEW );  }
    bool edit_scene_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_scene( response, data, UPDATE );  }
    bool edit_scene_copy( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_scene( response, data, COPY );  }

    bool edit_fixturegroup( CString& response, LPCSTR data, EditMode mode );
    bool edit_fixturegroup_create( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixturegroup( response, data, NEW );  }
    bool edit_fixturegroup_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixturegroup( response, data, UPDATE );  }

    bool edit_chase( CString& response, LPCSTR data, EditMode mode );
    bool edit_chase_create( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_chase( response, data, NEW );  }
    bool edit_chase_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_chase( response, data, UPDATE );  }
    bool edit_chase_copy( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_chase( response, data, COPY );  }

    bool edit_fixture( CString& response, LPCSTR data, EditMode mode );
    bool edit_fixture_create( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixture( response, data, NEW );  }
    bool edit_fixture_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixture( response, data, UPDATE );  }

    bool edit_venue_save( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_load( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_new( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_layout_save( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool venue_upload( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
};

extern bool CompareObjectNumber( DObject* o1,  DObject* o2 );

// ----------------------------------------------------------------------------
//
template <class T>
CString makeUIDArray( T& array )
{
    CString response( "[" );

    for ( T::iterator it=array.begin(); it != array.end(); ++it ) {
        if ( it != array.begin() )
            response.Append( "," );
        response.AppendFormat( "%lu", (ULONG)(*it) );
    }

    response.Append( "]" );

    return response;
}

