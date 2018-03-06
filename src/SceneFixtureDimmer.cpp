/* 
Copyright (C) 2017 Robert DeSantis
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

#include "SceneFixtureDimmer.h"
#include "SceneFixtureDimmerTask.h"

const char* SceneFixtureDimmer::className = "SceneFixtureDimmer";
const char* SceneFixtureDimmer::animationName = "Fixture dimmer";

// ----------------------------------------------------------------------------
//
SceneFixtureDimmer::SceneFixtureDimmer( UID animation_uid, bool shared, UID reference_fixture, 
                                        AnimationSignal signal,
                                        DimmerMode mode, UINT min_percent, UINT max_percent ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_dimmer_mode( mode ),
    m_min_percent( min_percent ),
    m_max_percent( max_percent )
{
}

// ----------------------------------------------------------------------------
//
SceneFixtureDimmer::~SceneFixtureDimmer(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneFixtureDimmer::clone( ) {
	return new SceneFixtureDimmer( 0L, m_shared, m_reference_fixture, m_signal, m_dimmer_mode, m_min_percent, m_max_percent );
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneFixtureDimmer::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneFixtureDimmerTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString SceneFixtureDimmer::getSynopsis(void) {
    CString synopsis;
    const char *mode_name;

    switch ( m_dimmer_mode ) {
        case DM_RAMP_UP:        mode_name = "Ramp Up";		    break;
        case DM_RAMP_DOWN:      mode_name = "Ramp Down";	    break;
        case DM_BREATH:         mode_name = "Breath";		    break;
        case DM_RANDOM:         mode_name = "Random";		    break;
    }

    synopsis.Format( "Dimmer( %s %u%% - %u%%)\n%s", mode_name, m_min_percent, m_max_percent,
        AnimationDefinition::getSynopsis() );

    return synopsis;
}

