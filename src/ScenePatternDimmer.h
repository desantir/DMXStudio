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

#include "SceneChannelAnimator.h"

typedef enum {
    DP_SEQUENCE = 1,
    DP_CYLON = 2,
    DP_PAIRS = 3,
    DP_TOCENTER = 4,
    DP_ALTERNATE = 5,
    DP_ALL = 6
} DimmerPattern;

class ScenePatternDimmer : 	public SceneChannelAnimator
{
    friend class VenueWriter;
    friend class VenueReader;

    DimmerPattern			m_dimmer_pattern;

public:
    static const char* className;

    ScenePatternDimmer( UID animation_uid, 
                        AnimationSignal signal,
                        UIDArray actors,
                        DimmerPattern pattern );

    ScenePatternDimmer(void) {}
    virtual ~ScenePatternDimmer(void);

    AbstractAnimation* clone();
    CString getSynopsis(void);

    const char* getName() { return "Scene Pattern Sequencer"; }
    const char* getClassName() { return ScenePatternDimmer::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    DimmerPattern getDimmerPattern() const {
        return m_dimmer_pattern;
    }
    void setDimmerPattern( DimmerPattern dimmer_pattern ) {
        m_dimmer_pattern = dimmer_pattern;
    }

    void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );
};

