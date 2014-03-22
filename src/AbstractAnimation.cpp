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

#include "AbstractAnimation.h"
#include "Scene.h"
#include "SceneSequence.h"
#include "SceneSoundLevel.h"
#include "SceneChannelAnimator.h"
#include "ScenePatternDimmer.h"
#include "SceneColorFader.h"
#include "SceneMovementAnimator.h"
#include "SceneStrobeAnimator.h"

// ----------------------------------------------------------------------------
//
CString AbstractAnimation::getSynopsis(void) {
    CString synopsis;

    synopsis.Format( "Fixtures( " );
    for ( UIDArray::iterator it=m_actors.begin(); it != m_actors.end(); ++it ) {
        Fixture *pf = studio.getVenue()->getFixture( (*it) );
        if ( pf != NULL )
            synopsis.AppendFormat( "F%lu ", pf->getNumber() );
        else {
            FixtureGroup* group = studio.getVenue()->getFixtureGroup( (*it) );
            if ( group != NULL )
                synopsis.AppendFormat( "G%lu ", group->getNumber() );
        }
    }
    synopsis += ")";

    return synopsis;
}

// ----------------------------------------------------------------------------
//
UIDArray AbstractAnimation::populateActors( ) {
    UIDArray resolved_actors;

    if ( m_actors.size() > 0 )              // Resolve preset actor list
        resolved_actors = m_actors;
    else                               
        resolved_actors = m_animation_task->getScene()->getActorUIDs();

    return resolved_actors;
}

// ----------------------------------------------------------------------------
//
void AbstractAnimation::removeActor( UID actor ) {
    for ( UIDArray::iterator it=m_actors.begin(); it != m_actors.end(); ) {
        if ( (*it) == actor )
            it = m_actors.erase( it );
        else
            it++;
    }
}
