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
#include "ScenePulse.h"
#include "AnimationTask.h"

struct PulseFixture
{
    Fixture*         		m_fixture;
    RGBWAArray              m_colors;               // Initial values for all pixels

    PixelArray*      	    m_pixels;               // Channel numbers for all colors
    DWORD                   m_pulse_end_ms;         // Fixture pulse end ms
	bool					m_pulsed;				// Track fixtures that just pulsed

    PulseFixture( Fixture* pf, PixelArray* pixels, RGBWAArray& color ) :
        m_fixture( pf ),
        m_colors( color ),
        m_pixels( pixels ),
        m_pulse_end_ms( 0 ),
		m_pulsed( false )
    {}
};

typedef std::vector< PulseFixture > PulseFixtureArray;

class ScenePulseTask : public AnimationTask
{
    PulseFixtureArray           m_fixtures;
    unsigned                    m_fixture_index;

    RGBWAArray 					m_pulse_colors; 
    UINT                        m_color_index;

    unsigned					m_pulse_ms;
    bool                        m_select_random;
    unsigned                    m_pulse_fixture_count; 
    PulseEffect                 m_pulse_effect;

public:
    ScenePulseTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~ScenePulseTask(void);

    void setupAnimation( AnimationDefinition* definition, DWORD time_ms );
    bool sliceAnimation( DWORD time_ms );
    bool restartAnimation( DWORD time_ms );
    void stopAnimation();

private:
    void loadColorChannels( Fixture* pf, Pixel& pixel, RGBWA& rgbwa );
};

