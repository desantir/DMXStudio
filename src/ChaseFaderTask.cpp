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

#include "ChaseFaderTask.h"

// ----------------------------------------------------------------------------
//
ChaseFaderTask:: ChaseFaderTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    AnimationTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
ChaseFaderTask::~ChaseFaderTask(void)
{
}

// ----------------------------------------------------------------------------
//
void ChaseFaderTask::setupAnimation( AnimationDefinition* definition, DWORD time_ms )
{
    ChaseFader* config = dynamic_cast<ChaseFader *>( definition );

    for ( SceneActor* new_actor : config->getTargetActors() ) {
        // Actor may not currently exist, if not, then no action
        if ( !isActorActive( new_actor->getActorUID() ) )
            continue;

        for ( Fixture *pf : resolveActorFixtures( new_actor ) ) {
            for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                BYTE current_value = getCurrentChannelValue( pf->getMultiPacketAddress( channel ) );
                BYTE target_value = new_actor->getFinalChannelValue( pf->getUID(), channel );

                int delta = (int)target_value - (int)current_value;
                if ( delta == 0 )
                    continue;

                int step = delta < 0 ? -1 : 1;
                long delta_ms = ((long)config->getFadeTime()) / (delta < 0 ? -delta : delta); // Compute MS between +/-1 steps
                long next_ms = time_ms + delta_ms;

                // printf( "Fixture %d ch %d from %d to %d delta %dms step %d next %ldms\n", pf->getFixtureNumber(), 
                //         channel, current_value, target_value, delta_ms, step, next_ms );

                m_fades.emplace_back( pf, channel, current_value, step, delta_ms, next_ms );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
bool ChaseFaderTask::sliceAnimation( DWORD time_ms )
{
    bool changed = false;

    for ( FadeDelta& fade : m_fades ) {
        while ( fade.m_next_ms < time_ms ) {        // While loop to catch up
            if ( fade.m_step < 0 ) {
                if ( fade.m_current_value > 0 )
					loadChannel( fade.m_pf, fade.m_channel, --fade.m_current_value );
            }
            else {
                if ( fade.m_current_value < 255 )
					loadChannel( fade.m_pf, fade.m_channel, ++fade.m_current_value );
            } 

            fade.m_next_ms += fade.m_delta_ms;
            changed = true;
        }
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
bool ChaseFaderTask::restartAnimation( DWORD time_ms ) {
    return false;
}

// ----------------------------------------------------------------------------
//
void ChaseFaderTask::stopAnimation()
{
}