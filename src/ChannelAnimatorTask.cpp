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

#include "ChannelAnimatorTask.h"

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

// ----------------------------------------------------------------------------
//
ChannelAnimatorTask::~ChannelAnimatorTask(void)
{
}

// ----------------------------------------------------------------------------
//
void ChannelAnimatorTask::add( UID actor_uid, channel_address channel, ChannelAnimationStyle animation_style, ChannelValueArray& value_list  )
{
    // Setup channel states
    SceneActor* actor = getActor( actor_uid );
	if ( actor == NULL )
		return;

    // Setup state so that the slice animation does not need to gather additional info
    for ( Fixture *pf : resolveActorFixtures( actor ) ) {
        if ( channel < pf->getNumChannels() && channel != INVALID_CHANNEL )
            m_channel_state.emplace_back( pf,
                channel, 
                actor->getFinalChannelValue( pf->getUID(), channel ),
                animation_style,
                value_list );
    }
}

// ----------------------------------------------------------------------------
//
void ChannelAnimatorTask::setupAnimation( AnimationDefinition* animation, DWORD time_ms )
{
    generateProgram( animation );

    if ( studio.isDebug() ) {
        CString debug_info;

        for ( ChannelState cs : m_channel_state ) {
            debug_info.Format( "%s channel %d style %d values ", cs.m_pf->getFullName(), cs.m_channel, cs.m_animation_style );
            
            for ( channel_value value : cs.m_value_list )
                debug_info.AppendFormat( "%d ", value );

            studio.log( debug_info );
        }
    }
}

// ----------------------------------------------------------------------------
//
bool ChannelAnimatorTask::sliceAnimation( DWORD time_ms )
{
    if ( m_channel_state.size() == 0 )
        return false;

    SignalState signal_state = tick( time_ms );

    if ( signal_state != SIGNAL_NEW_LEVEL && signal_state != SIGNAL_LEVEL_OFF )
        return false;

    bool changed = false;

    for ( ChannelStateArray::iterator it=m_channel_state.begin(); it != m_channel_state.end(); ++it ) {
        ChannelState& state = (*it);

        BYTE value = 0;

        unsigned level = getLevel();

        switch ( state.m_animation_style ) {
            case CAM_RANGE: {
                STUDIO_ASSERT( state.m_value_list.size() == 2, "Invalid size for range channel animation (UID=%lu CH=%d)", 
                    state.m_pf->getUID(), state.m_channel );

                unsigned offset = SCALE_BY_LEVEL( (state.m_value_list.at(1) - state.m_value_list.at(0) + 1), level );
                value = (BYTE)(state.m_value_list.at(0)+offset);
                break;
            }

            case CAM_LIST:
            case CAM_LIST_ONCE:
                STUDIO_ASSERT( state.m_value_list.size() > 0, "Invalid size for list channel animation (UID=%lu CH=%d)", 
                    state.m_pf->getUID(), state.m_channel );

                value = state.m_value_list.at( state.m_index++ );
                
                if ( state.m_index == state.m_value_list.size() )
                    state.m_index = (state.m_animation_style == CAM_LIST_ONCE) ? state.m_value_list.size()-1 : 0;
                    
                break;

            case CAM_SCALE:
                value = SCALE_BY_LEVEL( state.m_initial_value, level );
                break;

            case CAM_LEVEL:
                value = level;
                break;
        }

        loadChannel( state.m_pf, state.m_channel, value );

        changed = true;
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
void ChannelAnimatorTask::stopAnimation()
{
}

// ----------------------------------------------------------------------------
//
bool ChannelAnimatorTask::restartAnimation( DWORD time_ms )
{
    return false;
}
