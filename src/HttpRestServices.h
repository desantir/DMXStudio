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

#include "DMXHttpServer.h"
#include "DObject.h"
#include "MusicPlayer.h"
#include "SimpleJsonParser.h"
#include "SimpleJsonBuilder.h"

#define DMX_URL_REST                            DMX_URL_ROOT "rest/"

#define DMX_URL_EVENTS                          DMX_URL_REST "events/"

#define DMX_URL_QUERY_SCENE                     DMX_URL_REST "query/scene/"
#define DMX_URL_QUERY_CHASE                     DMX_URL_REST "query/chase/"
#define DMX_URL_QUERY_FIXTURE                   DMX_URL_REST "query/fixture/"

#define DMX_URL_QUERY_SCENES                    DMX_URL_REST "query/scenes/"
#define DMX_URL_QUERY_CHASES                    DMX_URL_REST "query/chases/"
#define DMX_URL_QUERY_FIXTURES                  DMX_URL_REST "query/fixtures/"
#define DMX_URL_QUERY_VENUE_STATUS              DMX_URL_REST "query/venue/status/"
#define DMX_URL_QUERY_VENUE_DESCRIBE            DMX_URL_REST "query/venue/describe/"
#define DMX_URL_QUERY_FIXTURE_DEFINITIONS       DMX_URL_REST "query/fixture/definitions/"
#define DMX_URL_QUERY_VENUE_LAYOUT              DMX_URL_REST "query/venue/layout/"

#define DMX_URL_CONTROL_VENUE_BLACKOUT          DMX_URL_REST "control/venue/blackout/"
#define DMX_URL_CONTROL_VENUE_WHITEOUT          DMX_URL_REST "control/venue/whiteout/"
#define DMX_URL_CONTROL_VENUE_MASTERDIMMER      DMX_URL_REST "control/venue/masterdimmer/" 
#define DMX_URL_CONTROL_VENUE_STROBE            DMX_URL_REST "control/venue/strobe/" 
#define DMX_URL_CONTROL_VENUE_WHITEOUT_COLOR    DMX_URL_REST "control/venue/whiteout/color/" 
#define DMX_URL_CONTROL_SCENE_SHOW              DMX_URL_REST "control/scene/show/" 
#define DMX_URL_CONTROL_SCENE_STAGE             DMX_URL_REST "control/scene/stage/" 
#define DMX_URL_CONTROL_CHASE_SHOW              DMX_URL_REST "control/chase/show/" 
#define DMX_URL_CONTROL_VENUE_MUSIC_MATCH       DMX_URL_REST "control/venue/music_match/"
#define DMX_URL_CONTROL_ANIMATION_SPEED         DMX_URL_REST "control/venue/animation_speed/"

#define DMX_URL_CONTROL_FIXTURE_CHANNELS        DMX_URL_REST "control/fixture/channels/"
#define DMX_URL_CONTROL_FIXTURE                 DMX_URL_REST "control/fixture/"
#define DMX_URL_CONTROL_FIXTUREGROUP            DMX_URL_REST "control/fixturegroup/"
#define DMX_URL_CONTROL_FIXTURE_CAPTURE         DMX_URL_REST "control/select/"
#define DMX_URL_CONTROL_FIXTURE_RELEASE         DMX_URL_REST "control/fixture/release/"
#define DMX_URL_CONTROL_FIXTURE_CHANNEL         DMX_URL_REST "control/channel/"

#define DMX_URL_CONTROL_VENUE_VOLUME_MASTER     DMX_URL_REST "control/venue/volume/master/"
#define DMX_URL_CONTROL_VENUE_VOLUME_MUTE       DMX_URL_REST "control/venue/volume/mute/"

#define DMX_URL_CONTROL_SOUNDSAMPLER_START      DMX_URL_REST "control/soundsampler/start/"
#define DMX_URL_CONTROL_SOUNDSAMPLER_STOP       DMX_URL_REST "control/soundsampler/stop/"
#define DMX_URL_QUERY_SOUNDSAMPLER              DMX_URL_REST "query/soundsampler/"

#define DMX_URL_CONTROL_BEATSAMPLER_START       DMX_URL_REST "control/beatsampler/start/"
#define DMX_URL_CONTROL_BEATSAMPLER_STOP        DMX_URL_REST "control/beatsampler/stop/"
#define DMX_URL_QUERY_BEATSAMPLER               DMX_URL_REST "query/beatsampler/"

#define DMX_URL_DELETE_SCENE                    DMX_URL_REST "delete/scene/"
#define DMX_URL_DELETE_CHASE                    DMX_URL_REST "delete/chase/"
#define DMX_URL_DELETE_FIXTURE                  DMX_URL_REST "delete/fixture/"
#define DMX_URL_DELETE_FIXTUREGROUP             DMX_URL_REST "delete/fixturegroup/"

#define DMX_URL_EDIT_SCENE_COPY_FIXTURES        DMX_URL_REST "edit/scene/copy_fixtures/"
#define DMX_URL_EDIT_VENUE_UPDATE               DMX_URL_REST "edit/venue/update/"
#define DMX_URL_EDIT_SCENE_CREATE               DMX_URL_REST "edit/scene/create/"
#define DMX_URL_EDIT_SCENE_UPDATE               DMX_URL_REST "edit/scene/update/"
#define DMX_URL_EDIT_SCENE_COPY                 DMX_URL_REST "edit/scene/copy/"
#define DMX_URL_EDIT_CHASE_CREATE               DMX_URL_REST "edit/chase/create/"
#define DMX_URL_EDIT_CHASE_UPDATE               DMX_URL_REST "edit/chase/update/"
#define DMX_URL_EDIT_CHASE_COPY                 DMX_URL_REST "edit/chase/copy/"
#define DMX_URL_EDIT_FIXTURE_CREATE             DMX_URL_REST "edit/fixture/create/"
#define DMX_URL_EDIT_FIXTURE_UPDATE             DMX_URL_REST "edit/fixture/update/"
#define DMX_URL_EDIT_VENUE_LAYOUT_SAVE          DMX_URL_REST "edit/venue/layout/save/"

#define DMX_URL_EDIT_VENUE_SAVE                 DMX_URL_REST "edit/venue/save/"
#define DMX_URL_EDIT_VENUE_LOAD                 DMX_URL_REST "edit/venue/load/"
#define DMX_URL_EDIT_VENUE_NEW                  DMX_URL_REST "edit/venue/new/"

#define DMX_URL_EDIT_FIXTUREGROUP_CREATE        DMX_URL_REST "edit/fixturegroup/create/"
#define DMX_URL_EDIT_FIXTUREGROUP_UPDATE        DMX_URL_REST "edit/fixturegroup/update/"

#define DMX_URL_VENUE_UPLOAD                    DMX_URL_REST "venue/upload/"

#define DMX_URL_CONTROL_MUSIC_TRACK_BACK        DMX_URL_REST "control/music/track/back/"
#define DMX_URL_CONTROL_MUSIC_TRACK_FORWARD     DMX_URL_REST "control/music/track/forward/"
#define DMX_URL_CONTROL_MUSIC_TRACK_STOP        DMX_URL_REST "control/music/track/stop/"
#define DMX_URL_CONTROL_MUSIC_TRACK_PAUSE       DMX_URL_REST "control/music/track/pause/"
#define DMX_URL_CONTROL_MUSIC_TRACK_PLAY        DMX_URL_REST "control/music/track/play/"
#define DMX_URL_QUERY_MUSIC_PLAYLISTS           DMX_URL_REST "query/music/playlists/"
#define DMX_URL_QUERY_MUSIC_PLAYLIST_TRACKS     DMX_URL_REST "query/music/playlist/tracks/"
#define DMX_URL_QUERY_MUSIC_QUEUED              DMX_URL_REST "query/music/queued/"
#define DMX_URL_QUERY_MUSIC_PLAYED              DMX_URL_REST "query/music/played/"
#define DMX_URL_CONTROL_MUSIC_QUEUE_TRACK       DMX_URL_REST "control/music/queue/track/"
#define DMX_URL_CONTROL_MUSIC_PLAY_TRACK        DMX_URL_REST "control/music/play/track/"
#define DMX_URL_CONTROL_MUSIC_PLAY_PLAYLIST     DMX_URL_REST "control/music/play/playlist/"
#define DMX_URL_CONTROL_MUSIC_PLAYER_LOGIN      DMX_URL_REST "control/music/player/login/"
#define DMX_URL_QUERY_MUSIC_MATCHER             DMX_URL_REST "query/music/matcher/"
#define DMX_URL_QUERY_MUSIC_MATCHER_SEARCH      DMX_URL_REST "query/music/matcher/search/"
#define DMX_URL_QUERY_MUSIC_TRACK_ANALYZE       DMX_URL_REST "query/music/track/analyze/"
#define DMX_URL_EDIT_MUSIC_MATCHER_LOAD         DMX_URL_REST "edit/music/matcher/load/"
#define DMX_URL_EDIT_MUSIC_MATCHER_UPDATE       DMX_URL_REST "edit/music/matcher/update/"

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
        COPY = 3
    };

    typedef bool (HttpRestServices::*RestGetHandlerFunc)( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    typedef bool (HttpRestServices::*RestPostHandlerFunc)( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

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
    HttpRestServices(void);
    ~HttpRestServices(void);

    LPCSTR getPrefix() {
        return DMX_URL_REST;
    }

    UINT getPort() {
        return studio.getHttpPort();
    }

    DWORD processGetRequest( HttpWorkerThread* worker );
    DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size );
    bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content ) { return false; }

protected:
    bool fetch_events( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool query_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool query_venue_status( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool control_venue_blackout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_venue_whiteout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_venue_masterdimmer( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_venue_music_match( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_scene_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_scene_stage( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_chase_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_venue_strobe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_venue_whiteout_color( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_fixture_release( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_fixture_channel( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_master_volume( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_mute_volume( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_fixture_capture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool control_music_track_back( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_track_forward( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_track_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_track_pause( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_track_play( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_playlists( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_queued( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_played( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_playlist_tracks( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_play_track( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_queue_track( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_music_play_playlist( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_track_analysis( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool control_animation_speed( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool query_scenes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_chases( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_venue_describe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_fixture_definitions( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_venue_layout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool control_soundsampler_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool control_soundsampler_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_soundsampler( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_matcher( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_music_matcher_search( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool control_beatsampler_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool control_beatsampler_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool query_beatsampler( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    bool delete_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool delete_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool delete_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );
    bool delete_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data );

    // POST handlers

    bool control_fixture_channels( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool control_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool control_fixture_group( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool edit_scene_copy_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool edit_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    bool edit_scene_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_scene( venue, session, response, data, NEW );  }
    bool edit_scene_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_scene( venue, session, response, data, UPDATE );  }
    bool edit_scene_copy( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_scene( venue, session, response, data, COPY );  }

    bool edit_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    bool edit_fixturegroup_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixturegroup( venue, session, response, data, NEW );  }
    bool edit_fixturegroup_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixturegroup( venue, session, response, data, UPDATE );  }

    bool edit_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    bool edit_chase_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_chase( venue, session, response, data, NEW );  }
    bool edit_chase_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_chase( venue, session, response, data, UPDATE );  }
    bool edit_chase_copy( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_chase( venue, session, response, data, COPY );  }

    bool edit_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode );
    bool edit_fixture_create( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixture( venue, session, response, data, NEW );  }
    bool edit_fixture_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) { return edit_fixture( venue, session, response, data, UPDATE );  }

    bool edit_venue_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_new( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_venue_layout_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool edit_music_matcher_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool edit_music_matcher_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool player_login( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool venue_upload( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

private:
    Venue* getStudio( ServiceFlags flags );
    void musicPlayerToJson( JsonBuilder& json );
};

extern bool CompareObjectNumber( DObject* o1,  DObject* o2 );

