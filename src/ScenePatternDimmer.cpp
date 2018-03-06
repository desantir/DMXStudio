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

#include "ScenePatternDimmer.h"
#include "ScenePatternDimmerTask.h"

const char* ScenePatternDimmer::className = "ScenePatternDimmer";
const char* ScenePatternDimmer::animationName = "Pattern sequencer";

// ----------------------------------------------------------------------------
//
ScenePatternDimmer::ScenePatternDimmer( UID animation_uid, bool shared, UID reference_fixture, 
                                        AnimationSignal signal,
                                        DimmerPattern pattern ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_dimmer_pattern( pattern )
{
}

// ----------------------------------------------------------------------------
//
ScenePatternDimmer::~ScenePatternDimmer(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* ScenePatternDimmer::clone( ) {
	return new ScenePatternDimmer( 0L, m_shared, m_reference_fixture, m_signal, m_dimmer_pattern );
}

// ----------------------------------------------------------------------------
//
AnimationTask* ScenePatternDimmer::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new ScenePatternDimmerTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString ScenePatternDimmer::getSynopsis(void) {
    CString synopsis;
    const char *pattern_name;

    switch ( m_dimmer_pattern ) {
        case DP_SEQUENCE:		pattern_name = "Sequence";		    break;
        case DP_CYLON:			pattern_name = "Cylon";			    break;
        case DP_PAIRS:			pattern_name = "Pairs";			    break;
        case DP_TOCENTER:		pattern_name = "Center";		    break;
        case DP_ALTERNATE:		pattern_name = "Alternate";		    break;
        case DP_ALL:			pattern_name = "All";			    break;
        case DP_RANDOM:			pattern_name = "Random";		    break;
        case DP_RAMP_UP:		pattern_name = "Ramp Up";		    break;
        case DP_RAMP_UP_DOWN:	pattern_name = "Ramp Up/Down";	    break;
        case DP_RANDOM_TO_ALL:	pattern_name = "Random Ramp Up";    break;
    }

    synopsis.Format( "Pattern( %s )\n%s", pattern_name,
        AnimationDefinition::getSynopsis() );

    return synopsis;
}

