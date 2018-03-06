/* 
Copyright (C) 2014-2016 Robert DeSantis
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

#include "SceneChannelFilter.h"
#include "SceneChannelFilterTask.h"

const char* SceneChannelFilter::className = "SceneChannelFilter";
const char* SceneChannelFilter::animationName = "Channel Filter";

// ----------------------------------------------------------------------------
//
SceneChannelFilter::SceneChannelFilter( UID animation_uid, bool shared, UID reference_fixture, 
                                        AnimationSignal signal,
                                        ChannelFilter filter,
                                        ChannelList channels,
                                        BYTE step,
                                        BYTE amplitude,
                                        int offset ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_filter(filter),
    m_channels(channels),
    m_step(step),
    m_amplitude(amplitude),
    m_offset(offset)
{
    if ( m_step < 1 )
        m_step = 1;
}

// ----------------------------------------------------------------------------
//
SceneChannelFilter::~SceneChannelFilter(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneChannelFilter::clone( ) {
	return new SceneChannelFilter( 0L, m_shared, m_reference_fixture, m_signal, m_filter, m_channels, m_step, m_amplitude, m_offset );
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneChannelFilter::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneChannelFilterTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString SceneChannelFilter::getSynopsis(void) {
    CString synopsis;
    CString channels;

    for ( channel_address channel : m_channels ) {
        if ( channels.GetLength() > 0 )
            channels.Append( "," );
        channels.AppendFormat( "%d", channel+1 );
    }

    LPCSTR plural = m_channels.size() > 1 ? "s" : "";
    
    switch ( m_filter ) {
        case CF_SINE_WAVE:
            synopsis.Format( "Sine Wave( channel%s=%s step=%d amplitude=%d offset=%d )", plural, channels, m_step, m_amplitude, m_offset );
            break;

        case CF_STEP_WAVE:
            synopsis.Format( "Step Wave( channel%s=%s step=%d )", plural, channels, m_step, m_amplitude );
            break;

        case CF_RAMP_UP:
            synopsis.Format( "Ramp Up( channel%s=%s step=%d maximum=%d)", plural, channels, m_step, m_amplitude );
            break;

        case CF_RAMP_DOWN:
            synopsis.Format( "Ramp Down( channel%s=%s step=%d minimum=%d )", plural, channels, m_step, m_amplitude );
            break;

        case CF_RANDOM:
            synopsis.Format( "Random( channel%s=%s amplitude=%d )", plural, channels, m_amplitude );
            break;
    }

    synopsis.AppendFormat( "\n%s", AnimationDefinition::getSynopsis() );

    return synopsis;
}


