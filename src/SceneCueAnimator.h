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
#include "AnimationSignalProcessor.h"

typedef std::vector<UIDArray> CueArray;

class SceneCueAnimator : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
    // Configuration
    bool            m_tracking;             // Cues are cumulative
    UINT            m_group_size;           // Cues are distributed to groups of fixture when > 0 (C1->G1, C2->G2, etc.)
    CueArray        m_cues;                 // C1 .. Cn (lists of palette IDs)

public:
    static const char* className;
    static const char* animationName;

    SceneCueAnimator( UID animation_uid, bool shared, UID reference_fixture, AnimationSignal signal,
                      bool tracking, UINT m_group_size, CueArray cues );

    SceneCueAnimator( void ) {}

    virtual ~SceneCueAnimator(void);

    CString getSynopsis(void);

    const char* getPrettyName() { return SceneCueAnimator::animationName; }
    const char* getClassName() { return SceneCueAnimator::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    inline bool isTracking() const {
        return m_tracking;
    }
    inline void setTracking( bool tracking ) {
        m_tracking = tracking;
    }

    inline UINT getGroupSize() const {
        return m_group_size;
    }
    inline void setGroupSize( UINT group_size ) {
        m_group_size = group_size;
    }

    inline CueArray& getCues() {
        return m_cues;
    }
    inline void setCues( CueArray cues ) {
        m_cues = cues;
    }

    bool removePaletteReference( UID palette_id );

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();
};

