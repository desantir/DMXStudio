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

#pragma once

#include "AnimationTask.h"
#include "Scene.h"

enum ChannelAnimationStyle {
    CAM_LIST = 1,					// Cycle list of values
    CAM_RANGE = 2,					// Scale range of values
    CAM_SCALE = 3,					// Scale scene value
    CAM_LEVEL = 4,                  // Apply level directly to value

    CAM_LIST_ONCE = 999             // One shot through list of values (last value persists - INTERNAL)
} ;

typedef std::vector<channel_value> ChannelValueArray;

class ChannelProgram
{
    UID                     m_actor_uid;
    channel_address			m_channel;
    ChannelAnimationStyle	m_animation_style;
    ChannelValueArray		m_value_list;

public:
    ChannelProgram( ) {}

    ChannelProgram( UID actor_uid, channel_address channel,  ChannelAnimationStyle animation_style, ChannelValueArray& value_list ) :
        m_actor_uid( actor_uid ),
        m_channel( channel ),
        m_animation_style( animation_style ),
        m_value_list( value_list )
    {}

    ChannelProgram( UID actor_uid, channel_address channel, ChannelAnimationStyle animation_style ) :
        m_actor_uid( actor_uid ),
        m_channel( channel ),
        m_animation_style( animation_style )
    {}

    UID getActorUID() const {
        return m_actor_uid;
    }

    ChannelValueArray& valueList() { 
        return m_value_list;
    }

	channel_address getChannel() const {
        return m_channel;
    }
    void setChannel( channel_address channel ) {
        m_channel = channel;
    }

    ChannelAnimationStyle getAnimationStyle() const {
        return m_animation_style;
    }
    void setAnimationStyle( ChannelAnimationStyle style ) {
        m_animation_style = style;
    }

    ChannelValueArray getChannelValues() const {
        return m_value_list;
    }
    void setChannelValues( ChannelValueArray value_list ) {
        m_value_list = value_list;
    }

    inline void addChannelValue( channel_value value ) {
        m_value_list.push_back( value );
    }
};

typedef std::vector< ChannelProgram > ChannelProgramArray;

class ChannelAnimatorTask : public AnimationTask
{
    struct ChannelState
    {
        Fixture*				m_pf;
		channel_address			m_channel;
		channel_value			m_initial_value;
        unsigned				m_index;
        ChannelAnimationStyle	m_animation_style;
        ChannelValueArray       m_value_list;

        ChannelState( Fixture* pf, channel_address channel,
			channel_value initial_value, 
            ChannelAnimationStyle animation_style,
            ChannelValueArray& value_list ) :
            m_pf( pf ),
            m_channel( channel ),
            m_initial_value( initial_value ),
            m_animation_style( animation_style ),
            m_value_list( value_list ),
            m_index(0)
        {}

        ChannelState( Fixture* pf, channel_address channel,
            channel_value initial_value, 
            ChannelAnimationStyle animation_style ) :
            m_pf( pf ),
            m_channel( channel ),
            m_initial_value( initial_value ),
            m_animation_style( animation_style ),
            m_index(0)
        {}
    };

    typedef std::vector< ChannelState > ChannelStateArray;

    // State information
    ChannelStateArray			m_channel_state;

private:
    ChannelAnimatorTask& operator=(ChannelAnimatorTask& rhs);
    ChannelAnimatorTask( ChannelAnimatorTask& other );

public:
    ChannelAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID ) :
        AnimationTask( engine, animation_uid, actors, owner_uid )
    {
        m_channel_state.reserve( 256 );
    }

    virtual ~ChannelAnimatorTask(void);

    void setupAnimation( AnimationDefinition* animation, DWORD time_ms );
    bool sliceAnimation( DWORD time_ms );
    bool restartAnimation( DWORD time_ms );
    void stopAnimation();

protected:

    virtual void generateProgram( AnimationDefinition* definition ) = 0;

    void add( UID actor_uid, channel_address channel, ChannelAnimationStyle animation_style, ChannelValueArray& value_list  );

    inline void add( ChannelProgramArray& program ) {
        for ( ChannelProgram& step :program )
            add( step.getActorUID(), step.getChannel(), step.getAnimationStyle(), step.getChannelValues() );
    }
};

