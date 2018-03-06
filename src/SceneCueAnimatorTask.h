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

#include "SceneCueAnimator.h"
#include "AnimationTask.h"

typedef std::vector< PaletteEntryMap > CueGroups;

class SceneCueAnimatorTask : public AnimationTask
{
    // Saved configuration
    bool                m_tracking;
    CueArray            m_cues;
    bool                m_fan_cues;

    // State 
    size_t              m_cue_index;
    CueGroups           m_cue_groups;               // Stores cue groups and individual actor channel values (for non-tracking and wrap)
    PaletteEntryMap     m_current_cue_channels;     // Tracks resolved values in current cue (used to reset channel value)

public:
    SceneCueAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~SceneCueAnimatorTask(void);

    void setupAnimation( AnimationDefinition* definition, DWORD time_ms );
    bool sliceAnimation( DWORD time_ms );
    bool restartAnimation( DWORD time_ms );
    void stopAnimation();

private:
    void applyCue( UIDArray cues, PaletteEntryMap& group, bool reset_values );
};
