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

#pragma once

#include "Venue.h"
#include "DMXHttpServer.h"
#include "DObject.h"
#include "MusicPlayer.h"
#include "SimpleJsonParser.h"
#include "SimpleJsonBuilder.h"
#include "RestServiceException.h"

#define DMX_URL_REST        DMX_URL_ROOT "rest/"

class HttpRestServices : public IRequestHandler
{
    enum ServiceFlags {
        VENUE_NOT_REQUIRED = 0x00,
        VENUE_REQUIRED = 0x01,
        RUNNING_REQUIRED = 0x02,
        RUNNING_VENUE = 0x03,
        PLAYER_REQUIRED = 0x04,
        PLAYER_AND_VENUE_REQUIRED = 0x07
    };

    enum EditMode {
        NEW = 1,
        UPDATE = 2,
        COPY = 3,
        TEST = 4
    };

    typedef void (HttpRestServices::*RestGetHandlerFunc)( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    typedef void (HttpRestServices::*RestPostHandlerFunc)( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    template <class T>
    struct RestService 
    {
        ServiceFlags    m_flags;
        T               m_handler;

        RestService()
        {}

        RestService( T handler, ServiceFlags flags = RUNNING_VENUE ) :
            m_flags( flags ),
            m_handler( handler )
        {}
    };

    typedef std::map<CString, RestService<RestPostHandlerFunc>> RestPostHandlerMap;
    typedef std::map<CString, RestService<RestGetHandlerFunc>> RestGetHandlerMap;

    RestGetHandlerMap       m_rest_get_handlers;
    RestPostHandlerMap      m_rest_post_handlers;

public:
    HttpRestServices( UINT port );
    ~HttpRestServices(void);

    LPCSTR getPrefix() {
        return DMX_URL_REST;
    }

    DWORD processGetRequest( HttpWorkerThread* worker );
    DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size );
    bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content ) { return false; }

protected:
    void fetch_events( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void query_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_animation( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void query_venue_status( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void control_venue_blackout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_whiteout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_animation_speed( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_masterdimmer( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_music_match( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_scene_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_scene_stage( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_chase_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_chase_step( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_strobe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_whiteout_color( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_whiteout_effect( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_master_volume( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_mute_volume( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_animation_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_animation_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void control_music_track_back( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_track_forward( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_track_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_track_pause( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_track_play( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_playlists( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_queued( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_played( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_playlist_tracks( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_play_track( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_queue_track( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_music_play_playlist( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
	void control_music_queue_playlist( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_track_analysis( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
	void query_video_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void query_scenes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_chases( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_venue_describe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_fixture_definitions( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_venue_layout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_animations( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_palettes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void control_soundsampler_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_soundsampler_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_soundsampler( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_matcher( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_music_matcher_search( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_amplitude( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_beatsampler_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void control_beatsampler_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_beatsampler( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void query_animation_levels( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void delete_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void delete_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void delete_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void delete_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void delete_animation( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void delete_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    void control_quickscene_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    
    void query_player_web_login( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    // POST handlers

    void control_fixture_channel( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void control_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void control_fixture_palettes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
	void control_fixture_position( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

	void query_video_search( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
	void query_music_videos( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void query_video_ids( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void edit_scene_copy_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void edit_venue_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void edit_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    void edit_scene_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_scene( venue, session, response, data, NEW );  }
    void edit_scene_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_scene( venue, session, response, data, UPDATE );  }
    void edit_scene_copy( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_scene( venue, session, response, data, COPY );  }
	
	void venue_create_quickscene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void edit_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    void edit_palette_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_palette( venue, session, response, data, NEW );  }
    void edit_palette_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_palette( venue, session, response, data, UPDATE );  }
    void edit_palette_copy( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_palette( venue, session, response, data, COPY );  }

    void edit_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    void edit_fixturegroup_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_fixturegroup( venue, session, response, data, NEW );  }
    void edit_fixturegroup_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_fixturegroup( venue, session, response, data, UPDATE );  }

    void edit_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    void edit_chase_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_chase( venue, session, response, data, NEW );  }
    void edit_chase_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_chase( venue, session, response, data, UPDATE );  }
    void edit_chase_copy( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_chase( venue, session, response, data, COPY );  }

    void edit_animation( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    void edit_animation_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_animation( venue, session, response, data, NEW );  }
    void edit_animation_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_animation( venue, session, response, data, UPDATE );  }
    void edit_animation_copy( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_animation( venue, session, response, data, COPY );  }
   
    void control_animation_test_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_animation( venue, session, response, data, TEST );  }
    void control_animation_test_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    void control_venue_whiteout_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void edit_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    void edit_fixture_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_fixture( venue, session, response, data, NEW );  }
    void edit_fixture_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { edit_fixture( venue, session, response, data, UPDATE );  }

    void edit_venue_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void edit_venue_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void edit_venue_new( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void edit_venue_layout_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void edit_music_matcher_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void edit_music_matcher_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void create_video_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void create_image_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    void generate_image_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    void player_login( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    
    void venue_upload( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

private:
    Venue* getStudio( ServiceFlags flags, LPCSTR handler_name );
    void musicPlayerToJson( JsonBuilder& json );

    void addService( LPCSTR service_name, RestGetHandlerFunc handler, ServiceFlags flags = RUNNING_VENUE ) {
        CString service_path ( DMX_URL_REST );
        service_path.Append( service_name );

        m_rest_get_handlers[ service_path ] = RestService<RestGetHandlerFunc>( handler, flags );
    }

    void addService( LPCSTR service_name, RestPostHandlerFunc handler, ServiceFlags flags = RUNNING_VENUE ) {
        CString service_path ( DMX_URL_REST );
        service_path.Append( service_name );

        m_rest_post_handlers[ service_path ] = RestService<RestPostHandlerFunc>( handler, flags );
    }  
};

extern bool CompareObjectNumber( DObject* o1,  DObject* o2 );
extern bool CompareObjectNumber2( DObject& o1,  DObject& o2 );
