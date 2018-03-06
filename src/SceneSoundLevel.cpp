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

#include "SceneSoundLevel.h"
#include "SceneSoundLevelTask.h"

const char* SceneSoundLevel::className = "SoundLevel";
const char* SceneSoundLevel::animationName = "Channel Fader";

// ----------------------------------------------------------------------------
//
SceneSoundLevel::SceneSoundLevel( UID animation_uid, bool shared, UID reference_fixture,
                                  AnimationSignal signal,
                                  WORD fade_what ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_fade_what( fade_what )
{
}

// ----------------------------------------------------------------------------
//
SceneSoundLevel::~SceneSoundLevel(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneSoundLevel::clone( ) {
	return new SceneSoundLevel( 0L, m_shared, m_reference_fixture, m_signal, m_fade_what );
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneSoundLevel::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneSoundLevelTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString SceneSoundLevel::getSynopsis(void) {
    CString synopsis;
    CString fade;

    if ( m_fade_what & FADE_COLORS )
        fade += "colors ";
    if ( m_fade_what & FADE_DIMMERS )
        fade += "dimmers ";

    synopsis.Format( "Fade ( %s) %s", fade, AnimationDefinition::getSynopsis() );

    return synopsis;
}

