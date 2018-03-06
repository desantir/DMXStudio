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

#define FADE_COLORS		0x01
#define FADE_DIMMERS	0x02
#define FADE_ALL		(FADE_COLORS | FADE_DIMMERS) 

class SceneSoundLevel : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

    WORD				m_fade_what;						// What to fade

    SceneSoundLevel& operator=(SceneSoundLevel& rhs) { return *this; }

public:
    static const char* className;
    static const char* animationName;

    SceneSoundLevel( void ) {};
    SceneSoundLevel( UID animation_uid, bool shared, UID reference_fixture, AnimationSignal signal, WORD fade_what );
    virtual ~SceneSoundLevel(void);

    CString getSynopsis(void);

    const char* getPrettyName() { return SceneSoundLevel::animationName; }
    const char* getClassName() { return SceneSoundLevel::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    WORD getFadeWhat() const {
        return m_fade_what;
    }
    void setFadeWhat( WORD fade_what ) {
        m_fade_what = fade_what;
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();
};