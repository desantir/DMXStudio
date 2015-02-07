/* 
Copyright (C) 2014 Robert DeSantis
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

#include <cmath>

const char* SceneChannelFilter::className = "SceneChannelFilter";
const char* SceneChannelFilter::animationName = "Channel Filter";

// ----------------------------------------------------------------------------
//
SceneChannelFilter::SceneChannelFilter( UID animation_uid, 
                                        AnimationSignal signal,
                                        UIDArray actors,
                                        ChannelFilter filter,
                                        channel_t channel,
                                        BYTE step,
                                        BYTE amplitude,
                                        int offset ) :
    SceneChannelAnimator( animation_uid, signal ),
    m_filter(filter),
    m_channel(channel),
    m_step(step),
    m_amplitude(amplitude),
    m_offset(offset)
{
    m_actors = actors;

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
AbstractAnimation* SceneChannelFilter::clone() {
    return new SceneChannelFilter( m_uid, m_signal, m_actors, m_filter, m_channel, m_step, m_amplitude, m_offset );
}

// ----------------------------------------------------------------------------
//
CString SceneChannelFilter::getSynopsis(void) {
    CString synopsis;

    switch ( m_filter ) {
        case CF_SINE_WAVE:
            synopsis.Format( "Sine Wave( channel=%d step=%d amplitude=%d offset=%d )", m_channel+1, m_step, m_amplitude, m_offset );
            break;

        case CF_STEP_WAVE:
            synopsis.Format( "Step Wave( channel=%d step=%d )", m_channel+1, m_step, m_amplitude );
            break;

        case CF_RAMP_UP:
            synopsis.Format( "Ramp Up( channel=%d step=%d )", m_channel+1, m_step );
            break;

        case CF_RAMP_DOWN:
            synopsis.Format( "Ramp Down( channel=%d step=%d )", m_channel+1, m_step );
            break;

        case CF_RANDOM:
            synopsis.Format( "Random( channel=%d amplitude=%d )", m_channel+1, m_amplitude );
            break;
    }

    synopsis.AppendFormat( "\n%s", AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void SceneChannelFilter::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_channel_animations.clear();

    double start_angle = 0.0;

    // Determine which channels will be participating
    for ( UID actor_uid : populateActors() ) {
        Fixture* pf = m_animation_task->getActorRepresentative( actor_uid );
        if ( !pf || pf->getNumChannels() <= m_channel )
            continue;

        // Each fixture may have a different start value so do independantly (and for random)
        ChannelValueArray value_array;

        // Get unmodified initial value
        SceneActor* actor = m_animation_task->getScene()->getActor( actor_uid );
        BYTE start_value =  actor->getChannelValue( m_channel );

        switch ( m_filter ) {
            case CF_SINE_WAVE:
                value_array = generateSineWave( start_value, start_angle, m_amplitude, m_step );
                start_angle += m_offset;
                break;

            case CF_STEP_WAVE:
                value_array = generateStepWave( start_value, m_step );
                break;

            case CF_RAMP_UP:
                value_array = generateRampUp( start_value, m_step );
                break;

            case CF_RAMP_DOWN:
                value_array = generateRampDown( start_value, m_step );
                break;

            case CF_RANDOM:
                value_array = generateRandom( start_value, m_amplitude );
                break;
        }

        m_channel_animations.push_back( 
            ChannelAnimation( actor_uid, m_channel, CAM_LIST, value_array ) );
    }

    return SceneChannelAnimator::initAnimation( task, time_ms, dmx_packet );
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilter::generateSineWave( int start_value, double start_angle, int amplitude, int step )
{
    ChannelValueArray value_array;
    
    int previous_value = -1;

    for ( double angle=start_angle; angle < start_angle+360.0; angle += step ) {
        double radians = angle * M_PI / 180.0;

        int new_value = static_cast<int>(start_value + (sin(radians) * amplitude) );
        if ( new_value > 255 )
            new_value = 255;
        else if ( new_value < 0 )
            new_value = 0;

        if ( new_value != previous_value ) {
            value_array.push_back( new_value );
            previous_value = new_value;
        }
    }
    
    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilter::generateStepWave( int start_value, int step )
{
    ChannelValueArray value_array;

    value_array.push_back( std::min<int>( 255, start_value+step ) );
    value_array.push_back( std::max<int>( 0, start_value-step ) );
    
    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilter::generateRampUp( int start_value, int step )
{
    ChannelValueArray value_array;

    for ( ; start_value <= 255; start_value += step )
        value_array.push_back( std::min<int>( 255, start_value ) );

    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilter::generateRampDown( int start_value, int step )
{
    ChannelValueArray value_array;

    for ( ; start_value >= 0; start_value -= step )
        value_array.push_back( std::max<int>( 0, start_value ) );

    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilter::generateRandom( int start_value, int amplitude )
{
    ChannelValueArray value_array;

    UINT range = (255 - start_value) % (amplitude+1);

    for ( int samples=0; samples < 500; samples++ )
        value_array.push_back( start_value + (rand() % (range+1)) );

    return value_array;
}

