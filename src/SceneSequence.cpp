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

#include "SceneSequence.h"
#include "SceneSequenceTask.h"

const char* SceneSequence::className = "FixtureSequencer";
const char* SceneSequence::animationName = "Fixture sequencer (deprecated)";

// ----------------------------------------------------------------------------
//
SceneSequence::SceneSequence( UID animation_uid, bool shared, UID reference_fixture, AnimationSignal signal ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal )
{
}

// ----------------------------------------------------------------------------
//
SceneSequence::~SceneSequence(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneSequence::clone( ) {
	return new SceneSequence( 0L, m_shared, m_reference_fixture, m_signal );
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneSequence::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneSequenceTask( engine, m_uid, actors, owner_uid );
}
