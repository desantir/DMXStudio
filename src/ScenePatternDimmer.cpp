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


#include "ScenePatternDimmer.h"

const char* ScenePatternDimmer::className = "ScenePatternDimmer";

// ----------------------------------------------------------------------------
//
ScenePatternDimmer::ScenePatternDimmer( UID animation_uid, 
                                        AnimationSignal signal,
                                        UIDArray actors,
                                        DimmerPattern pattern ) :
    SceneChannelAnimator( animation_uid, signal ),
    m_dimmer_pattern( pattern )
{
    m_actors = actors;
}

// ----------------------------------------------------------------------------
//
ScenePatternDimmer::~ScenePatternDimmer(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* ScenePatternDimmer::clone() {
    return new ScenePatternDimmer( m_uid, m_signal, m_actors, m_dimmer_pattern );
}

// ----------------------------------------------------------------------------
//
CString ScenePatternDimmer::getSynopsis(void) {
    CString synopsis;
    const char *pattern_name;

    switch ( m_dimmer_pattern ) {
        case DP_SEQUENCE:		pattern_name = "Sequence";		break;
        case DP_CYLON:			pattern_name = "Cylon";			break;
        case DP_PAIRS:			pattern_name = "Pairs";			break;
        case DP_TOCENTER:		pattern_name = "Center";		break;
        case DP_ALTERNATE:		pattern_name = "Alternate";		break;
        case DP_ALL:			pattern_name = "All";			break;
    }

    synopsis.Format( "Pattern( %s )\n%s", pattern_name,
        AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void ScenePatternDimmer::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_channel_animations.clear();

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
       
    ChannelValueArray value_array;
    std::vector<DimmerValue> dimmer_values_array;

    // Determine which channels will be participating
    for ( UIDArray::iterator it=m_actors.begin(); it != m_actors.end(); ++it ) {
        Fixture* pf = m_animation_task->getFixture( (*it) );
        STUDIO_ASSERT( pf != NULL, "Missing fixture UID=%lu", (*it) );

        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            if ( cp->isDimmer() ) {
                m_channel_animations.push_back( 
                    ChannelAnimation( pf->getUID(), channel, CAM_LIST, value_array ) );
                dimmer_values_array.push_back( DimmerValue( cp ) );
            }
        }
    }

    int num_channels = m_channel_animations.size();

    switch ( m_dimmer_pattern ) {
        case DP_SEQUENCE: {
            int target = 0; // X - - - -> - X - - -> - - X - -> - - - X
            for ( size_t i=0; i < m_channel_animations.size(); i++ ) {
                ChannelAnimation& chan_anim = m_channel_animations[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index ) );

                target++;
            }
            break;
        }

        case DP_CYLON: { // X - - -> - X - -> - - X -> - X - -> - - X
            int target = 0;

            for ( size_t i=0; i < m_channel_animations.size(); i++ ) {
                ChannelAnimation& chan_anim = m_channel_animations[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels*2-2; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index || num_channels*2-2-index == target ) );
                target++;
            }
            break;
        }

        case DP_PAIRS: { // X X - - -> - - X X 
            int target = 0;
            for ( size_t i=0; i < m_channel_animations.size(); i++ ) {
                ChannelAnimation& chan_anim = m_channel_animations[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index || target+1 == index ) );
                target += 2;
            }
            break;
        }

        case DP_TOCENTER: { // X - - X -> - X X -
            int target = 0;
            for ( size_t i=0; i < m_channel_animations.size(); i++ ) {
                ChannelAnimation& chan_anim = m_channel_animations[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( target == index || num_channels-index-1 == target ) );
                target += 1;
            }
            break;
        }

        case DP_ALTERNATE: { // X - X - -> - X - X 
            int target = 1;
            for ( size_t i=0; i < m_channel_animations.size(); i++ ) {
                ChannelAnimation& chan_anim = m_channel_animations[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < num_channels; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( index ^ target ? true : false ) );
                target ^= 1;
            }
            break;
        }

        case DP_ALL: { // X X X X -> - - - - 
            for ( size_t i=0; i < m_channel_animations.size(); i++ ) {
                ChannelAnimation& chan_anim = m_channel_animations[i];
                DimmerValue& dimmer_values = dimmer_values_array[i];

                for ( int index=0; index < 2; index++ )
                    chan_anim.valueList().push_back( dimmer_values.getValue( index ? true : false ) );
            }
            break;
        }
    }

    return SceneChannelAnimator::initAnimation( task, time_ms, dmx_packet );
}

