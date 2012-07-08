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
#include "MusicPlayer.h"

#define DMX_URL_ROOT                        "/dmxstudio/"
#define DMX_URL_ROOT2                       "/dmxstudio"

// CAUTION: THESE PREFIXES SHOULD ALL BE UNIQUE

#define DMX_URL_CONTROL_SCENE_SHOW          "/dmxstudio/control/scene/show/"
#define DMX_URL_CONTROL_CHASE_SHOW          "/dmxstudio/control/chase/show/"
#define DMX_URL_CONTROL_VENUE_BLACKOUT      "/dmxstudio/control/venue/blackout/"
#define DMX_URL_CONTROL_VENUE_DIMMER        "/dmxstudio/control/venue/dimmer/"
#define DMX_URL_CONTROL_VENUE_STROBE        "/dmxstudio/control/venue/strobe/"
#define DMX_URL_CONTROL_VENUE_WHITEOUT      "/dmxstudio/control/venue/whiteout/"
#define DMX_URL_CONTROL_FIXTURE_CAPTURE     "/dmxstudio/control/fixture/select/"
#define DMX_URL_CONTROL_FIXTURE_RELEASE     "/dmxstudio/control/fixture/release/"
#define DMX_URL_CONTROL_FIXTURE_CHANNEL     "/dmxstudio/control/fixture/channel/"
#define DMX_URL_CONTROL_ANIMATION_SPEED     "/dmxstudio/control/venue/animation_speed/"
#define DMX_URL_QUERY_VENUE                 "/dmxstudio/query/venue/"
#define DMX_URL_CONTROL_MASTER_VOLUME       "/dmxstudio/control/venue/master_volume/"
#define DMX_URL_QUERY_SOUND                 "/dmxstudio/query/sound/"
#define DMX_URL_CONTROL_MUTE_VOLUME         "/dmxstudio/control/venue/mute_volume/"
#define DMX_URL_CONTROL_MUSIC_TRACK_BACK    "/dmxstudio/control/music/track/back/"
#define DMX_URL_CONTROL_MUSIC_TRACK_FORWARD "/dmxstudio/control/music/track/forward/"
#define DMX_URL_CONTROL_MUSIC_TRACK_STOP    "/dmxstudio/control/music/track/stop/"
#define DMX_URL_CONTROL_MUSIC_TRACK_PAUSE   "/dmxstudio/control/music/track/pause/"
#define DMX_URL_CONTROL_MUSIC_TRACK_PLAY    "/dmxstudio/control/music/track/play/"
#define DMX_URL_QUERY_MUSIC_PLAYLISTS       "/dmxstudio/query/music/playlists/"
#define DMX_URL_QUERY_MUSIC_PLAYLIST_TRACKS "/dmxstudio/query/music/playlist/tracks/"
#define DMX_URL_CONTROL_MUSIC_PLAY_TRACK    "/dmxstudio/control/music/play/track/"
#define DMX_URL_CONTROL_MUSIC_PLAY_PLAYLIST "/dmxstudio/control/music/play/playlist/"

class DMXHttpMobile : public IRequestHandler
{
    typedef bool (DMXHttpMobile::*RestHandlerFunc)( CString& response, LPCSTR path_fragment );
    typedef std::map<CString, RestHandlerFunc> RestHandlerMap;

    RestHandlerMap      m_rest_handlers;

public:
    DMXHttpMobile(void);
    ~DMXHttpMobile(void);

    LPCSTR getPrefix() {
        return DMX_URL_ROOT;
    }

    UINT getPort() {
        return studio.getHttpPort();
    }

    DWORD processGetRequest( HttpWorkerThread* worker );
    DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size );
    bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content );

private:
    CString getFixtureDivContent();

    bool control_scene_show( CString& response, LPCSTR path_fragment );
    bool control_chase_show( CString& response, LPCSTR path_fragment );
    bool control_venue_blackout( CString& response, LPCSTR path_fragment );
    bool control_venue_dimmer( CString& response, LPCSTR path_fragment );
    bool control_venue_whiteout( CString& response, LPCSTR path_fragment );
    bool control_fixture_capture( CString& response, LPCSTR path_fragment );
    bool control_fixture_release( CString& response, LPCSTR path_fragment );
    bool control_fixture_channel( CString& response, LPCSTR path_fragment );
    bool control_venue_strobe( CString& response, LPCSTR path_fragment );
    bool control_animation_speed( CString& response, LPCSTR path_fragment );
    bool query_venue( CString& response, LPCSTR path_fragment );
    bool control_master_volume( CString& response, LPCSTR path_fragment );
    bool query_sound( CString& response, LPCSTR path_fragment );
    bool control_mute_volume( CString& response, LPCSTR path_fragment );
    bool control_music_track_back( CString& response, LPCSTR path_fragment );
    bool control_music_track_forward( CString& response, LPCSTR path_fragment );
    bool control_music_track_stop( CString& response, LPCSTR path_fragment );
    bool control_music_track_pause( CString& response, LPCSTR path_fragment );
    bool control_music_track_play( CString& response, LPCSTR path_fragment );
    bool query_music_playlists( CString& response, LPCSTR path_fragment );
    bool query_music_playlist_tracks( CString& response, LPCSTR path_fragment );
    bool control_music_play_track( CString& response, LPCSTR path_fragment );
    bool control_music_play_playlist( CString& response, LPCSTR path_fragment );
};

