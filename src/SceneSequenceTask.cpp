/* 
Copyright (C) 2011-2016 Robert DeSantis
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

#include "SceneSequenceTask.h"

// ----------------------------------------------------------------------------
//
SceneSequenceTask::SceneSequenceTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    AnimationTask( engine, animation_uid, actors, owner_uid )
{}

// ----------------------------------------------------------------------------
//
SceneSequenceTask::~SceneSequenceTask(void)
{
}

// ----------------------------------------------------------------------------
//
void SceneSequenceTask::setupAnimation( AnimationDefinition* definition, DWORD time_ms )
{
    SceneSequence* config = dynamic_cast<SceneSequence *>( definition );

    m_current_actor = -1;

//    selectActor( m_current_actor );
}

// ----------------------------------------------------------------------------
//
bool SceneSequenceTask::sliceAnimation( DWORD time_ms )
{
    SignalState state = tick( time_ms );

	// If first time through, turn off all actors
	if ( m_current_actor == -1 ) {
		for ( unsigned i=0; i < m_actors.size(); i++ )
			unselectActor( i );
		m_current_actor = 0;
	}

    if ( state == SIGNAL_LEVEL_OFF ) {
        unselectActor( m_current_actor );
        return true;
    }
    else if ( state == SIGNAL_NEW_LEVEL ) {
        unselectActor( m_current_actor );

        if ( ++m_current_actor == m_actors.size() )
            m_current_actor = 0;

        selectActor( m_current_actor );

		return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
void SceneSequenceTask::unselectActor( unsigned actor_num ) {
    if ( actor_num >= m_actors.size() )
        return;

	SceneActor* actor = &m_actors[actor_num];

    for ( Fixture *pf : resolveActorFixtures( actor ) ) {
        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            if ( pf->getChannel( channel )->canBlackout() )
                loadChannel( pf, channel, 0 );
        }
    }
}

// ----------------------------------------------------------------------------
//
void SceneSequenceTask::selectActor( unsigned actor_num )
{
    if ( actor_num >= m_actors.size() )
        return;

    SceneActor* actor = &m_actors[actor_num];

    for ( Fixture *pf : resolveActorFixtures( actor ) ) {
        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            if ( pf->getChannel( channel )->canBlackout() ) {
                BYTE value = actor->getFinalChannelValue( pf->getUID(), channel );
                loadChannel( pf, channel, value );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
void SceneSequenceTask::stopAnimation()
{
}

// ----------------------------------------------------------------------------
//
bool SceneSequenceTask::restartAnimation( DWORD time_ms )
{
    return false;
}