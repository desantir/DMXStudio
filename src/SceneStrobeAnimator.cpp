/* 
Copyright (C) 2011-2017 Robert DeSantis
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

#include "SceneStrobeAnimator.h"
#include "SceneStrobeAnimatorTask.h"

const char* SceneStrobeAnimator::className = "SceneStrobeAnimator";
const char* SceneStrobeAnimator::animationName = "Strobe";

// ----------------------------------------------------------------------------
//
SceneStrobeAnimator::SceneStrobeAnimator( UID animation_uid, bool shared, UID reference_fixture, 
										AnimationSignal signal,
										StrobeType strobe_type,
										UINT strobe_percent,
										StrobeTime strobe_time,
										RGBWA strobe_color,	
										RGBWA strobe_neg_color ) :
	AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
	m_strobe_neg_color( strobe_neg_color ),
	m_strobe_time( strobe_time ),
	m_strobe_color( strobe_color ),
	m_strobe_type( strobe_type ),
	m_strobe_percent( strobe_percent )
{
}

// ----------------------------------------------------------------------------
//
SceneStrobeAnimator::~SceneStrobeAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneStrobeAnimator::clone( ) {
	return new SceneStrobeAnimator( 0L, m_shared, m_reference_fixture, m_signal, m_strobe_type, m_strobe_percent,
								    m_strobe_time, m_strobe_color, m_strobe_neg_color );
}

// ----------------------------------------------------------------------------
//
CString SceneStrobeAnimator::getSynopsis(void) {
    CString synopsis;

	if ( m_strobe_type == STROBE_FIXTURE ) {
		synopsis.AppendFormat( "Fixture Strobe( %u percent )", m_strobe_percent );
	}
	else {
		synopsis.AppendFormat( "Strobe( color=%s -color=%s +ms=%u +fade=%u -ms=%u -fade=%u flash=%u )\n%s",
			m_strobe_color.getColorName(),
			m_strobe_neg_color.getColorName(),
			m_strobe_time.getOnMS(), m_strobe_time.getFadeInMS(), m_strobe_time.getOffMS(), m_strobe_time.getFadeOutMS(), m_strobe_time.getFlashes(),
			AnimationDefinition::getSynopsis() );
	}

    return synopsis;
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneStrobeAnimator::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneStrobeAnimatorTask( engine, m_uid, actors, owner_uid );
}

