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

    CString current_track_link;
    bool current_track_paused = false;
    bool init = true;
    MusicSelectorType last_type = MST_SCENE;
    UID last_type_uid = m_venue->getDefaultScene()->getUID();

    while ( isRunning() ) {
        try {
            bool track_changed = true;

            if ( !studio.getMusicPlayer()->isLoggedIn() ||
                 !m_venue->isMusicSceneSelectEnabled() ) {
                Sleep( 1000 );
                continue;
            }

            if ( !init ) {
                CString track_link;
                bool track_paused;

                if ( !m_player->waitOnTrackEvent( 1000, track_link, &track_paused ) )
                    continue;

                bool isCurrentTrack = current_track_link.Compare( track_link ) == 0;

                if ( isCurrentTrack && current_track_paused == track_paused )
                    continue;       // We already dealt with this state (i.e. we missed a transition)

                track_changed = !isCurrentTrack;    // For dealing with pause transitions

                current_track_link = track_link;
                current_track_paused = track_paused;
            }
            else {
                init = false;
            }

            CString track_name, artist_name, track_link, track_full_name;
            MusicSelectorType type = MST_SCENE;
            UID type_uid = m_venue->getDefaultScene()->getUID();

            if ( current_track_link.IsEmpty() || current_track_paused ) {  // No sound - select silence
                track_name = SILENCE_TRACK_NAME;
                track_link = SILENCE_TRACK_LINK;
                track_full_name = track_name;
            }
            else {                                                  // Use the full track name as the key
                m_player->getTrackInfo( current_track_link, &track_name, &artist_name, NULL, NULL, NULL );
                track_full_name.Format( "track '%s by %s'", track_name, artist_name );
                track_link = current_track_link;
            }

            CString method_of_selection;

            if ( current_track_paused || track_changed ) {
                // Select object type and UID that we will be displaying
                mapMusicToScene( track_link, type, type_uid, method_of_selection );
            }
            else {
                type = last_type;
                type_uid = last_type_uid;
                method_of_selection = "re-";
            }

            if ( type == MST_SCENE ) {
                Scene* scene = m_venue->getScene( type_uid );
                STUDIO_ASSERT( scene, "Music match scene %lu does not exist for %s", type_uid, track_full_name );

                DMXStudio::log_status( "Music match %sselected scene '%s' rated '%s' for %s", 
                    method_of_selection, scene->getName(), BPMRatings[scene->getBPMRating()].name, track_full_name );
                m_venue->selectScene( type_uid );
            }
            else if ( type == MST_CHASE ) {
                Chase* chase = m_venue->getChase( type_uid );
                STUDIO_ASSERT( chase, "Music match chase %lu does not exist for %s", type_uid, track_full_name );

                DMXStudio::log_status( "Music match %sselected chase '%s' for %s", 
                    method_of_selection, chase->getName(), track_full_name );
                m_venue->startChase( type_uid );
            }

            // Remember last selection in case of a pause
            if ( !current_track_paused ) {
                last_type = type;
                last_type_uid = type_uid;
            }
        }
        catch ( std::exception& ex ) {
            DMXStudio::log( ex );
            return -1;
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
//
void MusicWatcher::mapMusicToScene( LPCSTR track_link, MusicSelectorType& type, UID& type_uid, CString& method_of_selection )
{
    // Select object type and UID that we will be displaying
    m_venue->mapMusicToScene( track_link, type, type_uid );

    if ( type == MST_SCENE_BY_BPM) {
        UINT bpm = 0;
        type_uid = findSceneByBPM( track_link, bpm );
        if ( type_uid != 0 ) {
            type = MST_SCENE;
            method_of_selection.Format( "%d BPM, ", bpm );
        }
        else    // Fall back to random scene if no BPM info
            type = MST_RANDOM_SCENE;
    }

    if ( type == MST_RANDOM_SCENE ) {
        type_uid = m_venue->getRandomScene();
        if ( type_uid ) {
            type = MST_SCENE;
            method_of_selection = "randomly ";
        }
    }
    else if ( type == MST_RANDOM_CHASE ) {
        type_uid = m_venue->getRandomChase();
        if ( type_uid ) {
            type = MST_CHASE;
            method_of_selection = "randomly ";
        }
    }
}

// ----------------------------------------------------------------------------
//
UID MusicWatcher::findSceneByBPM( LPCSTR track_link, UINT& bpm )
{
    // TODO - Cache scene BPM map?

    AudioInfo audio_info;
    AudioStatus status = studio.getMusicPlayer()->getTrackAudioInfo( track_link, &audio_info, 0L );
    if ( status != OK )
        return 0;

    bpm = (UINT)(audio_info.tempo+.5);

    // Determine track's BMP rating
    BPMRating track_bpm_rating = computeBPMRating( bpm );
    if ( track_bpm_rating == BPM_NO_RATING )
        return 0;

    SceneRatingsMap ratingsMap;

    m_venue->populateSceneRatingsMap( ratingsMap );

    UINT up, down;

    up = track_bpm_rating;
    down = up-1;

    while ( down > BPM_NO_RATING || up < BPM_END ) {
        if ( up < BPM_END ) {
            SceneRatingsMap::iterator it=ratingsMap.find( (BPMRating)up );
            if ( it != ratingsMap.end() ) {
                UIDArray& choices = it->second;
                return choices.at( (rand() % choices.size()) );
            }

            up++;
        }

        if ( down > BPM_NO_RATING ) {
            SceneRatingsMap::iterator it=ratingsMap.find( (BPMRating)down );
            if ( it != ratingsMap.end() ) {
                UIDArray& choices = it->second;
                return choices.at( (rand() % choices.size()) );
            }

            down--;
        }
    }

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


