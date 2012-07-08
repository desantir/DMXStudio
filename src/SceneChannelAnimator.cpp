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


#include "SceneChannelAnimator.h"

/*
Base class for general channel animation effects but can also be used directly.

INPUTS            AFFECTS							CHANNEL					MODIFIERS
------            -------                           -------                 ---------
TIME			  ON/OFF							1-N PER FIXTURE			ATTACK
LEVEL			  SET VALUE AS %					1-N FIXTURES			RELEASE
BEAT (FREQ)		  ROTATE RANGE OF CHANNEL VALUES							SAMPLING
AVG LEVEL		  SEQUENCE CHANNELS											SCALING (log ) 
FREQ LEVEL		  CONTROL MULTIPLE CHANNELS
RANDOMNESS		  CHOOSE FROM PRESET SETS OF CHANNELS/VALUES

INPUT SIGNAL -> CONVERT SIGNAL -> EVENT STREAM -> CHANNEL MODIFICATION --
	^       				                    						|
	|						                    						|
	---------------------------------------------------------------------

- get signal
	- sampling time or event
- shape signal ( beat 1/0 (level = high), time tick 1 (level = high), audio level 1 (level = var), random (level=rnd)
- apply signal to fixture/channel pairs
	- channel on/off (values 0 & 255 same as below)
	- channel rotate set values (0-255)
	- channel value is a percent of level (with range of possible values e.g. 100-110 % input percent 0-100%)

		Time ------>
		tick	tick	tick	EVENT	tick	tick	

F1C1	V1a		V2a		...			(beat is 100%, no beat-0%)
F1C2	V1b		V2b		...			(V# modified by input level, with range limits)
F2C2	V1c		V2c		...

Examples:
	ON/OFF
	Light banks bank A on / bank B off / bank A off / bank B on / etc
	Range 
*/

const char* SceneChannelAnimator::className = "SceneChannelAnimator";

// ----------------------------------------------------------------------------
//
SceneChannelAnimator::SceneChannelAnimator( UID animation_uid, 
											AnimationSignal signal,
											ChannelAnimationArray& channel_animations ) :
	AbstractAnimation( animation_uid, signal ),
	m_channel_animations( channel_animations ),
	m_signal_processor( NULL ),
    m_run_once( false )
{
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimator::SceneChannelAnimator( UID animation_uid, 
											AnimationSignal signal ) :
	AbstractAnimation( animation_uid, signal ),
	m_signal_processor( NULL ),
    m_run_once( false )
{
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimator::~SceneChannelAnimator(void)
{
	stopAnimation();
}

// ----------------------------------------------------------------------------
//
CString SceneChannelAnimator::getSynopsis(void) {
	CString synopsis;

	synopsis.Format( "Channels( " );

	for ( ChannelAnimationArray::iterator it=m_channel_animations.begin(); 
			it != m_channel_animations.end(); it++ )
		synopsis.AppendFormat( "\n%s", (*it).getSynopsis() );

	synopsis += ")";

	return synopsis;
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneChannelAnimator::clone() {
	return new SceneChannelAnimator( m_uid, m_signal, m_channel_animations );
}

// ----------------------------------------------------------------------------
//
void SceneChannelAnimator::removeActor( UID actor ) {
	for ( ChannelAnimationArray::iterator it=m_channel_animations.begin(); it != m_channel_animations.end(); ) {
		if ( (*it).getActor() == actor )
			it = m_channel_animations.erase( it );
		else
			it++;
	}
}

// ----------------------------------------------------------------------------
//
void SceneChannelAnimator::stopAnimation( )
{
	if ( m_signal_processor ) {
		delete m_signal_processor;
		m_signal_processor = NULL;
	}
}

// ----------------------------------------------------------------------------
//
void SceneChannelAnimator::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
	memset( m_decay_channel, 0, sizeof(m_decay_channel) );

	m_animation_task = task;
	m_channel_state.clear();				// This may be restarted

	// Setup channel states
	for ( ChannelAnimationArray::iterator it=m_channel_animations.begin();
		  it != m_channel_animations.end(); it++ ) {

		Fixture* pf = m_animation_task->getFixture( (*it).getActor() );
		STUDIO_ASSERT( pf != NULL, "Invalid actor UID in animation" );
		SceneActor* actor = m_animation_task->getScene()->getActor( pf->getUID() );
		STUDIO_ASSERT( pf != NULL, "Invalid scene actor UID in animation" );
		channel_t channel = (*it).getChannel();

		// Setup state so that the slice animation does not need to gather additional info
		m_channel_state.push_back( ChannelState( pf,
												 (*it).getChannel(), 
												 actor->getChannelValue( channel ),
												 (*it).getAnimationStyle(),
												 &(*it).valueList() ) );
	}

	m_signal_processor = new AnimationSignalProcessor( m_signal, task );
}

// ----------------------------------------------------------------------------
//
bool SceneChannelAnimator::sliceAnimation( DWORD time_ms, BYTE* dmx_packet )
{
	bool tick = m_signal_processor->tick( time_ms );

	if ( !tick ) {
		if ( m_signal_processor->isDecay() ) {		// Handle value decay
			bool changed = false;
			for ( channel_t channel=0; channel < 512; channel++ ) {
				if ( m_decay_channel[ channel ] ) {
					dmx_packet[ channel ] = 0;
					changed = true;
				}
			}
			return changed;
		}

		return false;
	}

	bool changed = false;

	memset( m_decay_channel, 0, sizeof(m_decay_channel) );

	for ( ChannelStateArray::iterator it=m_channel_state.begin();
		  it != m_channel_state.end(); it++ ) {

		ChannelState& state = (*it);
		BYTE value = 0;

		unsigned level = m_signal_processor->getLevel();

		if ( !m_signal_processor->isBeat() && state.m_animation_style != CAM_SCALE )
			continue;

		switch ( state.m_animation_style ) {
			case CAM_RANGE: {
				STUDIO_ASSERT( state.m_value_list->size() == 2, "Invalid size for range channel animation (UID=%lu CH=%d)", 
							   state.m_pf->getUID(), state.m_channel );
				unsigned offset = (unsigned)(state.m_value_list->at(1) - state.m_value_list->at(0) + 1) * level / 100;
				value = (BYTE)(state.m_value_list->at(0)+offset);
				break;
			}

			case CAM_LIST:
				STUDIO_ASSERT( state.m_value_list->size() > 0, "Invalid size for list channel animation (UID=%lu CH=%d)", 
							   state.m_pf->getUID(), state.m_channel );
				value = state.m_value_list->at( state.m_index++ );
				if ( state.m_index == state.m_value_list->size() )
					state.m_index = (m_run_once) ? state.m_value_list->size()-1 : 0;

				if ( m_signal.isApplyToChannel() )
					value = (BYTE)((unsigned)value * level / 100);

				break;

			case CAM_SCALE:
				if ( m_signal.isApplyToChannel() )
					value = (BYTE)((unsigned)state.m_initial_value * level / 100);
				else
					value = state.m_initial_value;
				break;
		}

		if ( value == 0 && m_signal.getSampleDecayMS() > 0 )
			m_decay_channel[ state.m_pf->getChannelAddress( state.m_channel ) - 1 ] = true;
		else
			m_animation_task->loadChannel( dmx_packet, state.m_pf, state.m_channel, value );
		changed = true;
	}

	return changed;
}

// ----------------------------------------------------------------------------
//
ChannelAnimation::ChannelAnimation( UID actor, channel_t channel, 
					ChannelAnimationStyle animation_style,
					ChannelValueArray& value_list ) :
	m_actor( actor ),
	m_channel( channel ),
	m_animation_style( animation_style ),
	m_value_list( value_list )
{
}

// ----------------------------------------------------------------------------
//
CString ChannelAnimation::getSynopsis(void) {
	CString synopsis;
	CString style;

	switch ( m_animation_style ) {
		case CAM_LIST:	style = "list="; break;
		case CAM_RANGE:	style = "range="; break;
		case CAM_SCALE:	style = "scale"; break;
	}

	synopsis.Format( "id=%lu channel=%d %s", m_actor, m_channel+1, style );

	if ( m_value_list.size() > 0 ) {
		for ( ChannelValueArray::iterator it=m_value_list.begin(); 
				it != m_value_list.end(); it++ ) {
			if ( it != m_value_list.begin() )
				synopsis.Append( "," );
			synopsis.AppendFormat( "%u", (unsigned)(*it) );
		}
		synopsis.Append( " " );
	}

	return synopsis;
}