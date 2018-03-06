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

class SceneSequence : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

    SceneSequence& operator=(SceneSequence& rhs) { return *this; }

public:
    static const char* className;
    static const char* animationName;

    SceneSequence( void ) {};
    SceneSequence( UID animation_uid, bool shared, UID reference_fixture, AnimationSignal signal );
    virtual ~SceneSequence(void);

    const char* getPrettyName() { return SceneSequence::animationName; }
    const char* getClassName() { return SceneSequence::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();
};
