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

#include <cmath>

// ----------------------------------------------------------------------------
//
SceneChannelFilterTask::SceneChannelFilterTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    ChannelAnimatorTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
SceneChannelFilterTask::~SceneChannelFilterTask(void)
{
}

// ----------------------------------------------------------------------------
//
void SceneChannelFilterTask::generateProgram( AnimationDefinition* definition )
{
    double start_angle = 0.0;
    
    SceneChannelFilter* config = dynamic_cast<SceneChannelFilter *>( definition );

    // Determine which channels will be participating
	for ( SceneActor& actor : getActors() ) {
        Fixture* pf = getActorRepresentative( actor.getActorUID() );
        if ( !pf )
            continue;

        for ( channel_address channel : config->getChannels() ) {
            if ( pf->getNumChannels() <= channel )
                continue;

            // Each fixture may have a different start value so do independantly (and for random)
            ChannelValueArray value_array;

            // Get unmodified initial value
            BYTE start_value =  actor.getFinalChannelValue( pf->getUID(), channel );

            switch ( config->getFilter() ) {
                case CF_SINE_WAVE:
                    start_angle += config->getOffset();    // We want this first to be able to offset multiple filters
                    value_array = generateSineWave( start_value, start_angle, config->getAmplitude(), config->getStep() );
                    break;

                case CF_STEP_WAVE:
                    value_array = generateStepWave( start_value, config->getStep() );
                    break;

                case CF_RAMP_UP:
                    value_array = generateRampUp( start_value, config->getStep(), config->getAmplitude() );
                    break;

                case CF_RAMP_DOWN:
                    value_array = generateRampDown( start_value, config->getStep(), config->getAmplitude() );
                    break;

                case CF_RANDOM:
                    value_array = generateRandom( start_value, config->getAmplitude() );
                    break;
            }

            add( actor.getActorUID(), channel, CAM_LIST, value_array );
        }
    }
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilterTask::generateSineWave( int start_value, double start_angle, int amplitude, int step )
{
    ChannelValueArray value_array;

    int previous_value = -1;

    for ( double angle=start_angle; angle < start_angle+360.0; angle += step ) {
        double radians = angle * M_PI / 180.0;

        int new_value = static_cast<int>(start_value + (sin(radians) * amplitude) );

        if ( new_value != previous_value ) {
            previous_value = new_value;

            if ( new_value > 255 )
                new_value = 255;
            else if ( new_value < 0 )
                new_value = 0;

            value_array.push_back( new_value );
        }
    }

    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilterTask::generateStepWave( int start_value, int step )
{
    ChannelValueArray value_array;

    value_array.push_back( std::min<int>( 255, start_value+step ) );
    value_array.push_back( std::max<int>( 0, start_value-step ) );

    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilterTask::generateRampUp( int start_value, int step, int maximum )
{
    ChannelValueArray value_array;

    if ( maximum == 0 )
        maximum = 255;

    for ( ; start_value <= maximum; start_value += step )
        value_array.push_back( std::min<int>( maximum, start_value ) );

    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilterTask::generateRampDown( int start_value, int step, int minimum )
{
    ChannelValueArray value_array;

    for ( ; start_value >= minimum; start_value -= step )
        value_array.push_back( std::max<int>( minimum, start_value ) );

    return value_array;
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneChannelFilterTask::generateRandom( int start_value, int amplitude )
{
    ChannelValueArray value_array;

    UINT range = (255 - start_value) % (amplitude+1);

    for ( int samples=0; samples < 500; samples++ )
        value_array.push_back( start_value + (rand() % (range+1)) );

    return value_array;
}

