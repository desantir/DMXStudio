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

#include "SceneCueAnimator.h"
#include "AnimationEngine.h"
#include "SceneCueAnimatorTask.h"

const char* SceneCueAnimator::className = "SceneCueAnimator";
const char* SceneCueAnimator::animationName = "Cue Animator";

// ----------------------------------------------------------------------------
//
SceneCueAnimator::SceneCueAnimator( UID animation_uid, bool shared, UID reference_fixture, AnimationSignal signal, 
                                    bool tracking, UINT group_size, CueArray cues ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_tracking( tracking ), 
    m_group_size( group_size ),
    m_cues( cues )
{
}

// ----------------------------------------------------------------------------
//
SceneCueAnimator::~SceneCueAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneCueAnimator::clone( ) {
	return new SceneCueAnimator( 0L, m_shared, m_reference_fixture, m_signal, m_tracking, m_group_size, m_cues );
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneCueAnimator::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneCueAnimatorTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString SceneCueAnimator::getSynopsis(void) {
    CString synopsis;

    synopsis.Format( "Control( tracking=%s group=%u )\n", m_tracking ? "yes" : "no", m_group_size );

    size_t cue_number = 1;
    for ( UIDArray& palette_refs : m_cues ) {
        synopsis.AppendFormat( "Cue %d( ", cue_number++ );

        bool first = true;

        for ( UID palette_id : palette_refs ) {
            Palette palette;

            if ( !studio.getVenue()->getPalette( palette_id, palette ) )
                continue;

            if ( !first )
                synopsis.Append( ", " );
            else
                first = false;

            synopsis.AppendFormat( "%s", palette.getName() );
        }

        synopsis += " ) \n";
    }

    synopsis.Append( AnimationDefinition::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//

bool SceneCueAnimator::removePaletteReference( UID palette_id ) {
    bool updated = false;

    for ( UIDArray& cue : m_cues ) {
        for ( UIDArray::iterator it=cue.begin(); it != cue.end(); ++it )
            if ( (*it) == palette_id ) {
                cue.erase( it );
                updated = true;
                break;
             }
    }

    return updated;
}
