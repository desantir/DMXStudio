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

#include "SceneSequence.h"

const char* SceneSequence::className = "FixtureSequencer";
const char* SceneSequence::animationName = "Fixture sequencer";

// ----------------------------------------------------------------------------
//
SceneSequence::SceneSequence( UID animation_uid, AnimationSignal signal, UIDArray actors ) :
    AbstractAnimation( animation_uid, signal )
{
    m_actors = actors;
}

// ----------------------------------------------------------------------------
//
SceneSequence::~SceneSequence(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneSequence::clone() {
    return new SceneSequence( m_uid, m_signal, m_actors );
}

// ----------------------------------------------------------------------------
//
void SceneSequence::stopAnimation( )
{
}

// ----------------------------------------------------------------------------
//
void SceneSequence::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_running_actors = populateActors();

    for ( unsigned i=0; i < m_running_actors.size(); i++ )
        unselectActor( i, dmx_packet );

    m_current_actor = 0;

    selectActor( m_current_actor, dmx_packet );

    m_next_actor_ms = time_ms + m_signal.getSampleRateMS();
}

// ----------------------------------------------------------------------------
//
bool SceneSequence::sliceAnimation( DWORD time_ms, BYTE* dmx_packet )
{
    if ( time_ms < m_next_actor_ms )
        return false;

    unselectActor( m_current_actor, dmx_packet );

    if ( ++m_current_actor == m_running_actors.size() )
        m_current_actor = 0;

    selectActor( m_current_actor, dmx_packet );

    m_next_actor_ms = time_ms + m_signal.getSampleRateMS();

    return true;
}

// ----------------------------------------------------------------------------
//
void SceneSequence::unselectActor( unsigned actor_num, BYTE* dmx_packet ) {
    SceneActor* actor = m_animation_task->getScene()->getActor( m_running_actors[actor_num] );
    STUDIO_ASSERT( actor != NULL, "Missing scene actor for fixture %lu", m_running_actors[actor_num] );

    for ( Fixture *pf : m_animation_task->resolveActorFixtures( actor ) ) {
        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            if ( pf->getChannel( channel )->canBlackout() )
                m_animation_task->loadChannel( dmx_packet, pf, channel, 0 );
        }
    }
}

// ----------------------------------------------------------------------------
//
void SceneSequence::selectActor( unsigned actor_num, BYTE* dmx_packet )
{
    SceneActor* actor = m_animation_task->getScene()->getActor( m_running_actors[actor_num] );
    STUDIO_ASSERT( actor != NULL, "Missing scene actor for fixture %lu", m_running_actors[actor_num] );

    for ( Fixture *pf : m_animation_task->resolveActorFixtures( actor ) ) {
        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            if ( pf->getChannel( channel )->canBlackout() ) {
                BYTE value = actor->getChannelValue( channel );
                m_animation_task->loadChannel( dmx_packet, pf, channel, value );
            }
        }
    }
}
