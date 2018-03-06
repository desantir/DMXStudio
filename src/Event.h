/* 
Copyright (C) 2016 Robert DeSantis
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

enum EventSource {
    ES_STUDIO=1,
    ES_VENUE=2,
    ES_SCENE=3,
    ES_CHASE=4,
    ES_FIXTURE=5,
    ES_FIXTURE_GROUP=6,
    ES_CHANNEL=7,
    ES_WHITEOUT=8,
    ES_BLACKOUT=9,
    ES_MUTE_BLACKOUT=10,
    ES_TRACK=11,
    ES_PLAYLIST=12,
    ES_VOLUME=13,
    ES_VOLUME_MUTE=14,
    ES_MUSIC_MATCH=15,
    ES_ANIMATION_SPEED=16,
    ES_MUSIC_PLAYER=17,
    ES_MASTER_DIMMER=18,
    ES_WHITEOUT_STROBE=19,
    ES_WHITEOUT_COLOR=20,
    ES_TRACK_QUEUES=21,
    ES_ANIMATION=22,
    ES_PALETTE=23,
    ES_WHITEOUT_EFFECT=24,
	ES_VIDEO_PALETTE=25,
    ES_FIXTURE_STATUS=26
};

enum EventAction {
    EA_START=1,
    EA_STOP=2,
    EA_DELETED=3,
    EA_PAUSE=4,
    EA_RESUME=5,
    EA_NEW=6,
    EA_CHANGED=7,
    EA_ERROR=8,
    EA_TIME=9,
    EA_MESSAGE=10
};

struct Event
{
    EventSource         m_source;
    DWORD               m_uid;
    EventAction         m_action;
    CString             m_text;
    DWORD               m_val1;
    DWORD               m_val2;
    DWORD               m_val3;
    DWORD               m_val4;

    Event() {}

    Event( EventSource source, DWORD uid, EventAction action, LPCSTR text=NULL, DWORD val1=0L, DWORD val2=0L, DWORD val3=0L, DWORD val4=0L ) :
        m_source( source ),
        m_uid( uid ),
        m_action( action ),
        m_text( text == NULL ? "" : text ),
        m_val1( val1 ),
        m_val2( val2 ),
        m_val3( val3 ),
        m_val4( val4 ) {}

    ~Event() {}
};

typedef std::queue<Event> EventQueue;

