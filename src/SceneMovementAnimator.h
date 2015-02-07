/* 
Copyright (C) 2011-14 Robert DeSantis
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

#include "IVisitor.h"
#include "SceneChannelAnimator.h"
#include "MovementAnimation.h"

class SceneMovementAnimator : public SceneChannelAnimator
{
    friend class VenueWriter;
    friend class VenueReader;

    typedef std::vector<UINT> AngleList;

    struct Participant {
        UID         m_actor_uid;
        Head        m_head;

        Participant( UID actor_uid, Head& head ) :
            m_actor_uid( actor_uid ),
            m_head( head )
        {}
    };

    typedef std::vector<Participant> ParticipantArray;

    MovementAnimation		m_movement;			// Movement animation description

public:
    static const char* className;
    static const char* animationName;

    SceneMovementAnimator() {}

    SceneMovementAnimator( UID animation_uid, 
                        AnimationSignal signal,
                        UIDArray actors,
                        MovementAnimation movement );
    ~SceneMovementAnimator(void);

    MovementAnimation& movement( ) {
        return m_movement;
    }

    AbstractAnimation* clone();

    const char* getName() { return SceneMovementAnimator::animationName; }
    const char* getClassName() { return SceneMovementAnimator::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );

    virtual CString getSynopsis(void);

private:
    void genRandomMovement( AnimationTask* task, ParticipantArray& participants );
    void genRotateMovement( AnimationTask* task, ParticipantArray& participants );
    void genNodMovement( AnimationTask* task, ParticipantArray& participants );
    void genFanMovement( AnimationTask* task, ParticipantArray& participants );
    void genXcrossMovement( AnimationTask* task, ParticipantArray& participants );
    void genMoonflowerMovement( AnimationTask* task, ParticipantArray& participants );
    void genCoordinatesMovement( AnimationTask* task, ParticipantArray& participants );
    void genSineMovement( AnimationTask* task, ParticipantArray& participants );

    ChannelValueArray anglesToValues( Channel* channel, AngleList& tilt );

    void populateChannelAnimations( AnimationTask* task, ParticipantArray& participants, size_t& particpant_index, 
                                    AngleList& tilt, AngleList& pan, ChannelValueArray& dimmer,
                                    ChannelValueArray& speed, size_t group_size );

    template <class T>
    void swap( T& s1, T& s2 ) {
        T tmp = s1;
        s1 = s2;
        s2 = tmp;
    }
};