/* 
Copyright (C) 2016 Robert DeSantis
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

#include "Fixture.h"
#include "AnimationDefinition.h"
#include "ColorFader.h"
#include "SceneActor.h"

class ChaseFader : public AnimationDefinition
{
    ULONG           m_fade_time;
    ActorPtrArray   m_target_actors;

public:
    ChaseFader( UID animation_uid, ULONG fade_time, ActorPtrArray& target_actors );

    virtual ~ChaseFader(void);

    CString getSynopsis(void) { CString dummy; return dummy; }
    const char* getPrettyName() { return "Chase Fader"; }
    const char* getClassName() { return NULL; }
    void accept( IVisitor* visitor) { }

    inline ULONG getFadeTime() const {
        return m_fade_time;
    }

    inline ActorPtrArray& getTargetActors() {
        return m_target_actors;
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actor_list, UID owner_uid );

	AnimationDefinition* clone( );
};

