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

#include "Threadable.h"
#include "chase.h"
#include "BeatDetector.h"

class Venue;

enum ChaseState {
    CHASE_IDLE = 1,
    CHASE_INIT = 2,
    CHASE_LOAD = 3,
    CHASE_DELAY = 4,
    CHASE_FADE = 5
};

class ChaseEngine : public Threadable
{
    Venue*				m_venue;
    CCriticalSection    m_chase_mutex;		    // Protect chase objects

    // Running chase state
    Chase*				m_chase;
    volatile unsigned	m_step_number;			// psuedo-atomic
    ChaseState			m_chase_state;
    DWORD				m_next_state_ms;
    DWORD               m_fade_ms;
    unsigned			m_next_step;
    std::atomic_int     m_advance_steps;        // Manual chase step advance

    UINT run(void);

    void fadeToNextScene( ULONG fade_time );
    ChaseStep* advanceScene( void );
    void prepareForNext();

public:
    ChaseEngine( Venue* venue );
    ~ChaseEngine(void);

    bool start();
    bool stop();

    bool startChase( Chase* chase );
    bool stopChase();
    void chaseStep( int steps );

    bool isChaseRunning() const {
        return m_chase != NULL && m_chase_state != CHASE_IDLE;
    }

    bool isFading() const {
        return m_chase_state == CHASE_FADE;
    }

    Chase* getChase() const {
        return m_chase;
    }

    unsigned getCurrentStep() const {
        return m_step_number;
    }
};

