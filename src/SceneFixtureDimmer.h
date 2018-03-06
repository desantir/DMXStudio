/* 
Copyright (C) 2017 Robert DeSantis
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

enum DimmerMode {
    DM_RAMP_UP = 1,
    DM_RAMP_DOWN = 2,
    DM_BREATH = 3,
    DM_RANDOM = 4
} ;

class SceneFixtureDimmer : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

    DimmerMode  m_dimmer_mode;
    UINT        m_min_percent;
    UINT        m_max_percent;

public:
    static const char* className;
    static const char* animationName;

    SceneFixtureDimmer( UID animation_uid, bool shared, UID reference_fixture, 
                        AnimationSignal signal,
                        DimmerMode mode, UINT min_percent, UINT max_percent );

    SceneFixtureDimmer(void) {}
    virtual ~SceneFixtureDimmer(void);

    CString getSynopsis(void);

    const char* getPrettyName() { return SceneFixtureDimmer::animationName; }
    const char* getClassName() { return SceneFixtureDimmer::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    DimmerMode getDimmerMode() const {
        return m_dimmer_mode;
    }
    void setDimmerMode( DimmerMode dimmer_mode ) {
        m_dimmer_mode = dimmer_mode;
    }

    UINT getMinPercent() const {
        return m_min_percent;
    }
    void setMinPercent( UINT min_percent ) {
        m_min_percent = min_percent;
    }

    UINT getMaxPercent() const {
        return m_max_percent;
    }
    void setMaxPercent( UINT max_percent ) {
        m_max_percent = max_percent;
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();
};

