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

#include "ScenePatternDimmerTask.h"

// ----------------------------------------------------------------------------
//
ScenePatternDimmerTask::ScenePatternDimmerTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    ChannelAnimatorTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
ScenePatternDimmerTask::~ScenePatternDimmerTask()
{
}

// ----------------------------------------------------------------------------
//
void ScenePatternDimmerTask::generateProgram( AnimationDefinition* definition ) 
{
    ScenePatternDimmer* config = dynamic_cast< ScenePatternDimmer *>( definition );

    struct DimmerValue {
        BYTE m_on;
        BYTE m_off;

        DimmerValue( Channel* cp ) :
            m_on( cp->getDimmerHighestIntensity() ),
            m_off( cp->getDimmerLowestIntensity() )
        {}

        BYTE getValue( bool on ) const {
            return on ? m_on : m_off;
        }
    };

    ChannelProgramArray program;
    std::vector<DimmerValue> dimmer_values_array;

    // Determine which channels will be participating
	for ( SceneActor& actor : getActors() ) {
        Fixture* pf = getActorRepresentative( actor.getActorUID() );
        if ( !pf )
            continue;

        // Determine which channels will be participating
        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            if ( cp->isDimmer() ) {
                program.emplace_back( actor.getActorUID(), channel, CAM_LIST );
                dimmer_values_array.emplace_back( cp );
            }
        }
    }

    int num_channels = program.size();

    switch ( config->getDimmerPattern() ) {
        case DP_SEQUENCE: {
            int target = 0; // X - - - -> - X - - -> - - X - -> - - - X
            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index ) );

                target++;
            }
            break;
        }

        case DP_CYLON: { // X - - -> - X - -> - - X -> - X - -> - - X
            int target = 0;

            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels*2-2; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index || num_channels*2-2-index == target ) );
                target++;
            }
            break;
        }

        case DP_PAIRS: { // X X - - -> - - X X 
            int target = 0;

            for ( int index=0; index < (num_channels+1)/2; index++ ) {
                for ( int i=0; i < num_channels; i++ ) {
                    ChannelProgram& chan_anim = program[i];
                    DimmerValue& dimmer_values = dimmer_values_array[i];
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == i || target+1 == i ) );
                }
                target += 2;
            }
            break;
        }

        case DP_TOCENTER: { // X - - X -> - X X -
            int target = 0;
            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index || num_channels-index-1 == target ) );
                target += 1;
            }
            break;
        }

        case DP_ALTERNATE: { // X - X - -> - X - X 
            int target = 1;
            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( index ^ target ? true : false ) );
                target ^= 1;
            }
            break;
        }

        case DP_ALL: {          // X X X X -> - - - - 
            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < 2; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( index ? true : false ) );
            }
            break;
        }

        case DP_RANDOM: {       // * * * * -> * * * * -> ...
            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int i=0; i < 50; i++ )   // Generate 50 states per dimmer
                    chan_anim.valueList().push_back( dimmer_values.getValue( (rand() % 2) ? true : false ) );
            }
            break;
        }

        case DP_RAMP_UP:        // - - - - -> X - - - -> X X - - -> X X X - -> X X X X
        case DP_RAMP_UP_DOWN: { // - - - - -> X - - - -> X X - - -> X X X - -> X X X X -> - X X X -> - - X X -> - - - X
            for ( int i=0; i < num_channels; i++ ) {
                ChannelProgram& chan_anim = program[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=-1; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( index >= i ) );

                if ( config->getDimmerPattern() == DP_RAMP_UP_DOWN ) {
                    for ( int index=num_channels-1; index >= 0; index-- )
                        chan_anim.valueList().push_back( dimmer_values.getValue( index >= i ) );
                }
            }
            break;
        }

        case DP_RANDOM_TO_ALL: {    // * * * * -> * * * * -> * * * * -> X X X X
            std::vector<size_t> sequence;
            for ( int i=0; i < num_channels; i++ )
                sequence.push_back( i );

            for ( int i=0; i < 50; i++ ) {   // Generated states
                std::vector<size_t> pool = sequence;
                std::set<size_t> on;

                for ( int j=0; j < num_channels; j++ ) {
                    size_t index = rand() % pool.size();
                    on.insert( pool.at( index ) );
                    pool.erase( pool.begin() + index );

                    for ( int i=0; i < num_channels; i++ ) {
                        ChannelProgram& chan_anim = program[i];
                        DimmerValue& dimmer_values = dimmer_values_array[i];

                        chan_anim.valueList().push_back( dimmer_values.getValue( on.find(i) != on.end() ) );
                    }
                }
            }
            break;
        }
    }

    add( program );
}

