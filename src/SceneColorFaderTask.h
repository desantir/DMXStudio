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

#include "SceneColorFader.h"
#include "AnimationTask.h"

class SceneColorFaderTask : public AnimationTask
{
protected:
    FaderFixtureArray		    m_fixtures;
    FaderEffect					m_current_effect;
    UINT       					m_color_index;
    ColorStrobe					m_strobe;
    UINT                        m_effect_periods;
    RGBWAArray                  m_colors;
    bool                        m_restart_strobe;
    FaderEffect                 m_fader_effect;
	StrobeTime					m_strobe_time;

public:
    SceneColorFaderTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~SceneColorFaderTask(void);

    void setupAnimation( AnimationDefinition* definition, DWORD time_ms );
    bool sliceAnimation( DWORD time_ms );
    bool restartAnimation( DWORD time_ms );
    void stopAnimation();

private:
	RGBWA nextColor() {
		UINT index = m_color_index;
		m_color_index = (m_color_index + 1) % m_colors.size();
		return m_colors[index];
	}
};

