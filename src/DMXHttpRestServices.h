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

class DMXHttpRestServices : public IRequestHandler
{
public:
    DMXHttpRestServices(void) {}

    virtual ~DMXHttpRestServices(void) {}

protected:
    bool query_venue_status( CString& response, LPCSTR data );

    bool control_venue_blackout( CString& response, LPCSTR data );
    bool control_venue_whiteout( CString& response, LPCSTR data );
    bool control_venue_masterdimmer( CString& response, LPCSTR data );
    bool control_venue_music_match( CString& response, LPCSTR data );
    bool control_scene_show( CString& response, LPCSTR data );
    bool control_chase_show( CString& response, LPCSTR data );
    bool control_venue_strobe( CString& response, LPCSTR data );
    bool control_fixture_release( CString& response, LPCSTR data );
    bool control_fixture_channel( CString& response, LPCSTR data );
    bool control_master_volume( CString& response, LPCSTR data );
    bool control_mute_volume( CString& response, LPCSTR data );

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
};

