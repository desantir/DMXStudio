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

#include "MusicWatcher.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
MusicWatcher::MusicWatcher( MusicPlayer* player, Venue* venue ) :
    Threadable( "MusicWatcher" ),
    m_player( player ),
    m_venue( venue )
{
}

// ----------------------------------------------------------------------------
//
MusicWatcher::~MusicWatcher(void)
{
    stop();
}

// ----------------------------------------------------------------------------
//
UINT MusicWatcher::run(void) {
    DMXStudio::log_status( "Music match running" );

    DWORD current_track_id = 0;
    bool current_track_paused = false;

    while ( isRunning() ) {
        try {
            DWORD track_id;
            bool track_paused;

            if ( !studio.getMusicPlayer()->isLoggedIn() ) {
                Sleep( 1000 );
                continue;
            }

            if ( !m_player->waitOnTrackEvent( 1000, &track_id, &track_paused ) )
                continue;

            if ( current_track_id == track_id && current_track_paused == track_paused )
                continue;       // We already dealt with this state (i.e. we missed a transition)

            current_track_id = track_id;
            current_track_paused = track_paused;

            if ( !m_venue->isMusicSceneSelectEnabled() )
                continue;

            CString track_name;

            if ( current_track_id == 0 || current_track_paused ) {  // No sound - select silence
                track_name = SILENCE_TRACK_NAME;
            }
            else {                                                  // Use the full track name as the key
                track_name = m_player->getTrackFullName( current_track_id );
            }

            // Select object type and UID that we will be displaying
            MusicSelectorType type = MST_SCENE;
            UID type_uid = m_venue->getDefaultScene()->getUID();

            m_venue->mapMusicToScene( track_name, type, type_uid );

            if ( type == MST_RANDOM_SCENE ) {
                Scene* scene = m_venue->getSceneByNumber( (rand() % m_venue->getNumScenes())+1 );
                if ( scene ) {
                    type_uid = scene->getUID();
                    type = MST_SCENE;
                }
            }
            else if ( type == MST_RANDOM_CHASE ) {
                Chase* chase = m_venue->getChaseByNumber( (rand() % m_venue->getNumChases())+1 );
                if ( chase ) {
                    type_uid = chase->getUID();
                    type = MST_CHASE;
                }
            }

            // Stop any running chases and load the new object
            m_venue->stopChase();

            if ( type == MST_SCENE ) {
                Scene* scene = m_venue->getScene( type_uid );
                STUDIO_ASSERT( scene, "Music match scene %lu does not exist for track '%s'", type_uid, track_name );
                DMXStudio::log_status( "Music match selected scene '%s' for track '%s'", scene->getName(), track_name );
                m_venue->selectScene( type_uid );
            }
            else if ( type == MST_CHASE ) {
                Chase* chase = m_venue->getChase( type_uid );
                STUDIO_ASSERT( chase, "Music match chase %lu does not exist for track '%s'", type_uid, track_name );
                DMXStudio::log_status( "Music match selected chase '%s' for track '%s'", chase->getName(), track_name );
                m_venue->startChase( type_uid );
            }
        }
        catch ( std::exception& ex ) {
            DMXStudio::log( ex );
            return -1;
        }
    }

    DMXStudio::log_status( "Music match stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
bool MusicWatcher::start()
{
    return startThread();
}

// ----------------------------------------------------------------------------
//
bool MusicWatcher::stop()
{
    if ( !stopThread() )
        return false;

    return true;
}


