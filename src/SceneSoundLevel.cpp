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


#include "SceneSoundLevel.h"

const char* SceneSoundLevel::className = "SoundLevel";

// ----------------------------------------------------------------------------
//
SceneSoundLevel::SceneSoundLevel( UID animation_uid,
								  AnimationSignal signal,
								  UIDArray actors,
								  WORD fade_what ) :
	SceneChannelAnimator( animation_uid, signal ),
	m_fade_what( fade_what )
{
	m_actors = actors;
}

// ----------------------------------------------------------------------------
//
SceneSoundLevel::~SceneSoundLevel(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneSoundLevel::clone() {
	return new SceneSoundLevel( m_uid, m_signal, m_actors, m_fade_what );
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

	synopsis.Format( "Fade ( %s) %s", fade, AbstractAnimation::getSynopsis() );

	return synopsis;
}

// ----------------------------------------------------------------------------
//
void SceneSoundLevel::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
	m_animation_task = task;
	m_channel_animations.clear();

	UIDArray actors = populateActors( m_animation_task->getScene() );
	ChannelValueArray value_array;

	// Determine which channel will be participating
	for ( UIDArray::iterator it=actors.begin(); it != actors.end(); it++ ) {
		Fixture* pf = m_animation_task->getFixture( (*it) );
		STUDIO_ASSERT( pf != NULL, "Missing fixture UID=%lu", (*it) );

		for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
			Channel* cp = pf->getChannel( channel );

			if ( ((m_fade_what & FADE_COLORS) && cp->isColor()) ) {
				m_channel_animations.push_back( 
					ChannelAnimation(  pf->getUID(), channel, CAM_SCALE, value_array ) );
			}
		}
	}

	return SceneChannelAnimator::initAnimation( task, time_ms, dmx_packet );
}
