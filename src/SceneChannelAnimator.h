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

#include "AnimationDefinition.h"
#include "SceneChannelAnimatorTask.h"
#include "Scene.h"

class ChannelAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

	channel_address			m_channel;
    ChannelAnimationStyle	m_animation_style;
    ChannelValueArray		m_value_list;

public:
    ChannelAnimation(void) {}
    ChannelAnimation( channel_address channel, 
        ChannelAnimationStyle animation_style, 
        ChannelValueArray& value_list );
    ChannelAnimation( channel_address channel, 
        ChannelAnimationStyle animation_style );

    void accept( IVisitor* visitor) {
        visitor->visit(this);
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

    virtual CString getSynopsis(void);
};

typedef std::vector< ChannelAnimation > ChannelAnimationArray;

class SceneChannelAnimator : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
    // Configuration
    ChannelAnimationArray		m_channel_animations;

public:
    static const char* className;
    static const char* animationName;

    SceneChannelAnimator() {}

    SceneChannelAnimator( UID animation_uid, bool shared, UID reference_fixture, 
                          AnimationSignal signal );

    SceneChannelAnimator( UID animation_uid, bool shared, UID reference_fixture, 
                          AnimationSignal signal, 
                          ChannelAnimationArray& channel_animations );

    virtual ~SceneChannelAnimator(void);
    
    CString getSynopsis(void);

    const char* getPrettyName() { return SceneChannelAnimator::animationName; }
    const char* getClassName() { return SceneChannelAnimator::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    void setChannelAnimations( ChannelAnimationArray animations ) {
        m_channel_animations = animations;
    }
    ChannelAnimationArray& channelAnimations( ) {
        return m_channel_animations;
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
        return new SceneChannelAnimatorTask( engine, m_uid, actors, owner_uid );
    }

	AnimationDefinition* clone();
};

