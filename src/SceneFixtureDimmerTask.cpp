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

#include "SceneFixtureDimmerTask.h"

// ----------------------------------------------------------------------------
//
SceneFixtureDimmerTask::SceneFixtureDimmerTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    ChannelAnimatorTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
SceneFixtureDimmerTask::~SceneFixtureDimmerTask()
{
}

// ----------------------------------------------------------------------------
//
void SceneFixtureDimmerTask::generateProgram( AnimationDefinition* definition ) 
{
    SceneFixtureDimmer* config = dynamic_cast< SceneFixtureDimmer *>( definition );

    // Determine which channels will be participating
	for ( SceneActor& actor : getActors() ) {
        Fixture* pf = getActorRepresentative( actor.getActorUID() );
        if ( !pf )
            continue;

        // Determine which channels will be participating
        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            if ( cp->isDimmer() ) {
                ChannelValueArray values;
                generateDimmerProgram( config, cp, values );
                add( actor.getActorUID(), channel, CAM_LIST, values );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
 void SceneFixtureDimmerTask::generateDimmerProgram( SceneFixtureDimmer* config, Channel* cp, ChannelValueArray& values ) {
     int high = cp->getDimmerHighestIntensity();
     int low = cp->getDimmerLowestIntensity();
     bool reverse = high < low;

     int range = ( high > low ) ? high-low : low-high;
     int minimum = (range * config->getMinPercent()) / 100;
     int maximum = (range * config->getMaxPercent()) / 100;

    switch ( config->getDimmerMode() ) {
        case DM_BREATH:
        case DM_RAMP_UP:
        case DM_RAMP_DOWN: {
            // Generate curve of values 
            ChannelValueArray steps;

            double accumulator = static_cast<double>( minimum );
            for ( double step=1.0; accumulator <= maximum; step += 1.0 ) {
                steps.push_back( static_cast<BYTE>(accumulator ) );
                accumulator += pow( step, range > 200 ? 1.2 : 1.1 );
            }
        
            if ( config->getDimmerMode() != DM_RAMP_DOWN ) {
                for ( size_t index=0; index < steps.size(); index++ ) {
                    if ( reverse )
                        values.push_back( std::max<int>( high, low - steps[index] ) );
                    else
                        values.push_back( std::min<int>( high, low + steps[index] ) );
                }
            }

            if ( config->getDimmerMode() != DM_RAMP_UP ) {
                for ( size_t index=steps.size(); index-- > 0; ) {
                    if ( reverse )
                        values.push_back( std::max<int>( high, low - steps[index] ) );
                    else
                        values.push_back( std::min<int>( high, low + steps[index] ) );
                }
            }

            break;
        }

        case DM_RANDOM: {
            int num_steps = std::max<int>( 1, (config->getMaxPercent()-config->getMinPercent()) / 5 );

            for ( int i=0; i < num_steps; i++ ) {
                int step = rand() % (range+1);
                values.push_back( step + ((reverse) ? high : low ) );
            }
            break;
        }
    }
 }

