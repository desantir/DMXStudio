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

#include "ChaseFader.h"
#include "AnimationTask.h"

// ----------------------------------------------------------------------------
//
ChaseFader::ChaseFader( UID animation_uid, ULONG fade_time, ActorPtrArray& target_actors ) :
    AbstractAnimation( animation_uid ),
    m_fade_time( fade_time ),
    m_target_actors( target_actors )
{
}

// ----------------------------------------------------------------------------
//
ChaseFader::~ChaseFader(void)
{
}

// ----------------------------------------------------------------------------
//
void ChaseFader::stopAnimation( )
{
}

// ----------------------------------------------------------------------------
//
void ChaseFader::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;

    for ( SceneActor* new_actor : m_target_actors ) {
        SceneActor* current_actor = m_animation_task->getActor( new_actor->getActorUID() );
        if ( current_actor == NULL )
            continue;

        for ( Fixture *pf : m_animation_task->resolveActorFixtures( current_actor ) ) {
            for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                channel_t dmx_address = 
                    ((pf->getUniverseId()-1) * DMX_PACKET_SIZE) + pf->getChannelAddress( channel ) - 1;

                BYTE current_value = current_actor->getChannelValue( channel );
                BYTE target_value = new_actor->getChannelValue( channel );

                int delta = (int)target_value - (int)current_value;
                if ( delta == 0 )
                    continue;

                int step = delta < 0 ? -1 : 1;
                long delta_ms = ((long)m_fade_time) / (delta < 0 ? -delta : delta); // Compute MS between +/-1 steps
                long next_ms = time_ms + delta_ms;

                // printf( "Fixture %d ch %d from %d to %d delta %dms step %d next %ldms\n", pf->getFixtureNumber(), 
                //         channel, current_value, target_value, delta_ms, step, next_ms );

                m_fades.push_back( FadeDelta( dmx_address, step, delta_ms, next_ms ) );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
bool ChaseFader::sliceAnimation( DWORD time_ms, BYTE* dmx_packet )
{
    bool changed = false;

    for ( FadeDelta& fade : m_fades ) {
        while ( fade.m_next_ms < time_ms ) {        // While loop to catch up
            if ( fade.m_step < 0 ) {
                if ( dmx_packet[ fade.m_dmx_address ] > 0 )
                    dmx_packet[ fade.m_dmx_address ] -= 1;
            }
            else {
                if ( dmx_packet[ fade.m_dmx_address ] < 255 )
                    dmx_packet[ fade.m_dmx_address] += 1;
            } 

            fade.m_next_ms += fade.m_delta_ms;
            changed = true;
        }
    }

    return changed;
}
