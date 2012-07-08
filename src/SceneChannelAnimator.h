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


#pragma once

#include "AbstractAnimation.h"
#include "Scene.h"
#include "AnimationTask.h"

typedef enum {
	CAM_LIST = 1,					// Cycle list of values
	CAM_RANGE = 2,					// Scale range of values
	CAM_SCALE = 3					// Scale scene value
} ChannelAnimationStyle;

typedef std::vector<BYTE> ChannelValueArray;

class ChannelAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

	UID						m_actor;
	channel_t				m_channel;
	ChannelAnimationStyle	m_animation_style;
	ChannelValueArray		m_value_list;

public:
	ChannelAnimation(void) {}
	ChannelAnimation( UID actor, channel_t channel, 
					  ChannelAnimationStyle animation_style, 
					  ChannelValueArray& value_list );

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

	ChannelValueArray& valueList() { 
		return m_value_list;
	}

	UID getActor() const {
		return m_actor;
	}
	void setActor( UID actor ) {
		m_actor = actor;
	}

	channel_t getChannel() const {
		return m_channel;
	}
	void setChannel( channel_t channel ) {
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
    
	virtual CString getSynopsis(void);
};

typedef std::vector< ChannelAnimation > ChannelAnimationArray;

class SceneChannelAnimator : public AbstractAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

	struct ChannelState
	{
		Fixture*				m_pf;
		channel_t				m_channel;
		BYTE					m_initial_value;
		unsigned				m_index;
		ChannelAnimationStyle	m_animation_style;
		ChannelValueArray*		m_value_list;

		ChannelState( Fixture* pf, channel_t channel,
					  BYTE initial_value, 
					  ChannelAnimationStyle animation_style,
					  ChannelValueArray* value_list ) :
			m_pf( pf ),
			m_channel( channel ),
			m_initial_value( initial_value ),
			m_animation_style( animation_style ),
			m_value_list( value_list ),
			m_index(0)
		{}
	};

	typedef std::vector< ChannelState > ChannelStateArray;

	// State information
	ChannelStateArray			m_channel_state;
	bool						m_decay_channel[512];
	AnimationSignalProcessor*	m_signal_processor;

protected:
	// Configuration
	ChannelAnimationArray		m_channel_animations;
    bool                        m_run_once;             // Runs though list values once (no repeating)

private:
	SceneChannelAnimator& operator=(SceneChannelAnimator& rhs);
	SceneChannelAnimator( SceneChannelAnimator& other );

public:
	static const char* className;

	SceneChannelAnimator(void) :
		m_signal_processor( NULL )
	{}

	SceneChannelAnimator( UID animation_uid, 
						  AnimationSignal signal );

	SceneChannelAnimator( UID animation_uid, 
						  AnimationSignal signal, 
						  ChannelAnimationArray& channel_animations );

	virtual ~SceneChannelAnimator(void);
	
	AbstractAnimation* clone();
	CString getSynopsis(void);

	void removeActor( UID actor );

	const char* getName() { return "Channel Animator"; }
	const char* getClassName() { return SceneChannelAnimator::className; }

   void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

	void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );
	bool sliceAnimation( DWORD time_ms, BYTE* dmx_packet );
	void stopAnimation( void );

	void setChannelAnimations( ChannelAnimationArray animations ) {
		m_channel_animations = animations;
	}
	ChannelAnimationArray& channelAnimations( ) {
		return m_channel_animations;
	}
};
