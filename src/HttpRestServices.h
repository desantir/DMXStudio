/* 
Copyright (C) 2012 Robert DeSantis
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
#include "SoundSampler.h"
#include "BeatDetector.h"
#include "SimpleJsonParser.h"
#include "SimpleJsonBuilder.h"

#define DMX_URL_REST                            DMX_URL_ROOT "rest/"

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
#define DMX_URL_CONTROL_MUSIC_PLAY_TRACK        DMX_URL_REST "control/music/play/track/"
#define DMX_URL_CONTROL_MUSIC_PLAY_PLAYLIST     DMX_URL_REST "control/music/play/playlist/"
#define DMX_URL_CONTROL_MUSIC_PLAYER_LOGIN      DMX_URL_REST "control/music/player/login/"
#define DMX_URL_QUERY_MUSIC_MATCHER             DMX_URL_REST "query/music/matcher/"
#define DMX_URL_EDIT_MUSIC_MATCHER              DMX_URL_REST "edit/music/matcher/"

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

class HttpRestServices : public IRequestHandler
{
    typedef bool (HttpRestServices::*RestGetHandlerFunc)( CString& response, LPCSTR data );
    typedef std::map<CString, RestGetHandlerFunc> RestGetHandlerMap;

    typedef bool (HttpRestServices::*RestPostHandlerFunc)( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    typedef std::map<CString, RestPostHandlerFunc> RestPostHandlerMap;

    enum EditMode {
        NEW = 1,
        UPDATE = 2,
        COPY = 3
    };

    RestGetHandlerMap       m_rest_get_handlers;
    RestPostHandlerMap      m_rest_post_handlers;

public:
    HttpRestServices(void);
    ~HttpRestServices(void);

    // TODO these should be made client specific ( and shouldn't be here )
    SoundSampler            m_sound_sampler;
    BeatDetector            m_beat_sampler;
    BeatBinArray            m_beats;


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
    bool query_venue_status( CString& response, LPCSTR data );

    bool control_venue_blackout( CString& response, LPCSTR data );
    bool control_venue_whiteout( CString& response, LPCSTR data );
    bool control_venue_masterdimmer( CString& response, LPCSTR data );
    bool control_venue_music_match( CString& response, LPCSTR data );
    bool control_scene_show( CString& response, LPCSTR data );
    bool control_chase_show( CString& response, LPCSTR data );
    bool control_venue_strobe( CString& response, LPCSTR data );
    bool control_venue_whiteout_color( CString& response, LPCSTR data );
    bool control_fixture_release( CString& response, LPCSTR data );
    bool control_fixture_channel( CString& response, LPCSTR data );
    bool control_master_volume( CString& response, LPCSTR data );
    bool control_mute_volume( CString& response, LPCSTR data );
    bool control_fixture_capture( CString& response, LPCSTR data );

    bool control_music_track_back( CString& response, LPCSTR data );
    bool control_music_track_forward( CString& response, LPCSTR data );
    bool control_music_track_stop( CString& response, LPCSTR data );
    bool control_music_track_pause( CString& response, LPCSTR data );
    bool control_music_track_play( CString& response, LPCSTR data );
    bool query_music_playlists( CString& response, LPCSTR data );
    bool query_music_queued( CString& response, LPCSTR data );
    bool query_music_played( CString& response, LPCSTR data );
    bool query_music_playlist_tracks( CString& response, LPCSTR data );
    bool control_music_play_track( CString& response, LPCSTR data );
    bool control_music_play_playlist( CString& response, LPCSTR data );
    bool control_animation_speed( CString& response, LPCSTR data );

    bool query_scenes( CString& response, LPCSTR data );
    bool query_fixtures( CString& response, LPCSTR data );
    bool query_chases( CString& response, LPCSTR data );
    bool query_venue_describe( CString& response, LPCSTR data );
    bool query_fixture_definitions( CString& response, LPCSTR data );
    bool query_venue_layout( CString& response, LPCSTR data );

    bool control_soundsampler_start( CString& response, LPCSTR data );
    bool control_soundsampler_stop( CString& response, LPCSTR data );
    bool query_soundsampler( CString& response, LPCSTR data );
    bool query_music_matcher( CString& response, LPCSTR data );

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

    bool edit_music_matcher( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
    bool player_login( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );

    bool venue_upload( CString& response, LPCSTR data, DWORD size, LPCSTR content_type );
};

extern bool CompareObjectNumber( DObject* o1,  DObject* o2 );

