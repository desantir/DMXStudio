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

#include "DMXStudio.h"
#include "Threadable.h"
#include "MusicPlayer.h"
#include "IVisitor.h"

#define SILENCE_TRACK_NAME      "Silence"
#define UNMAPPED_TRACK_NAME     "Unmapped Tracks"

class Venue;

enum MusicSelectorType {
    MST_SCENE = 1,
    MST_CHASE = 2,
    MST_RANDOM_SCENE = 3,
    MST_RANDOM_CHASE = 4
};

struct MusicSceneSelector
{
    CString               m_track_full_name;
    UID                   m_selection_uid;          // UID of chase or scene
    MusicSelectorType     m_selection_type;         // UID object type

    MusicSceneSelector( ) {}

    MusicSceneSelector( LPCSTR track_full_name, MusicSelectorType type, UID type_uid ) :
        m_track_full_name( track_full_name ),
        m_selection_uid( type_uid ),
        m_selection_type( type )
    {}

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }
};

typedef std::map<CString, MusicSceneSelector> MusicSceneSelectMap;

class MusicWatcher : public Threadable
{
    Venue*                  m_venue;
    MusicPlayer*            m_player;

    MusicWatcher(MusicWatcher& other) {}
    MusicWatcher& operator=(MusicWatcher& rhs) { return *this; }

public:
    MusicWatcher( MusicPlayer* player, Venue* venue );
    ~MusicWatcher(void);

    bool start();
    bool stop();

protected:
    UINT run(void);
};

