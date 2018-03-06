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
#include "AnimationTask.h"

struct FadeDelta
{
    Fixture*		m_pf;			            // Fixture
	channel_address	m_channel;					// Channel #
	channel_value	m_current_value;			// Channel value (changes on each step)
    int				m_step;                     // Step (1 or -1)
    ULONG			m_delta_ms;                 // MS between increments
    ULONG			m_next_ms;                  // MS of next change

    FadeDelta( Fixture*	pf, channel_address channel, channel_value current_value, int step, LONG delta_ms, ULONG next_ms ) :
		m_pf( pf ),
		m_channel( channel ),
		m_current_value( current_value ),
        m_step( step ),
        m_delta_ms( delta_ms ),
        m_next_ms( next_ms )
    {}

    ~FadeDelta() {}
};

typedef std::vector<FadeDelta> FadeDeltaList;

class ChaseFaderTask : public AnimationTask
{
    FadeDeltaList   m_fades;

public:
    ChaseFaderTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~ChaseFaderTask(void);

    void setupAnimation( AnimationDefinition* definition, DWORD time_ms );
    bool sliceAnimation( DWORD time_ms );
    bool restartAnimation( DWORD time_ms );
    void stopAnimation();
};

