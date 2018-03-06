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

#include "ChannelAnimatorTask.h"
#include "SceneMovementAnimator.h"
#include "AnimationEngine.h"

class SceneMovementAnimatorTask : public ChannelAnimatorTask
{
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

    void generateProgram( AnimationDefinition* definition );

public:
    SceneMovementAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~SceneMovementAnimatorTask(void);

private:
    void genRandomMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genRotateMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genNodMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genFanMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genXcrossMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genMoonflowerMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genCoordinatesMovement( MovementAnimation& movement, ParticipantArray& participants );
    void genSineMovement( MovementAnimation& movement, ParticipantArray& participants );

    ChannelValueArray anglesToValues( Channel* channel, AngleList& tilt, UINT min_angle, UINT max_angle );

    void populateChannelAnimations( ParticipantArray& participants, size_t& particpant_index, 
        AngleList& tilt, AngleList& pan, ChannelValueArray& dimmer,
        ChannelValueArray& speed, size_t group_size, bool run_once );

    template <class T>
    void swap( T& s1, T& s2 ) {
        T tmp = s1;
        s1 = s2;
        s2 = tmp;
    }
};