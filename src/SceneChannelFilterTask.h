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

#pragma once

#include "ChannelAnimatorTask.h"

class SceneChannelFilterTask : public ChannelAnimatorTask
{
    void generateProgram( AnimationDefinition* definition );

public:
    SceneChannelFilterTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~SceneChannelFilterTask(void);

private:
    ChannelValueArray generateSineWave( int start_value, double start_angle, int amplitude, int step );
    ChannelValueArray generateStepWave( int start_value, int step );
    ChannelValueArray generateRandom( int start, int end );
    ChannelValueArray generateRampUp( int start_value, int amplitude, int maximum );
    ChannelValueArray generateRampDown( int start_value, int step, int minimum );

};



