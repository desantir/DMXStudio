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

typedef enum chase_state {
    CHASE_IDLE = 1,
    CHASE_INIT = 2,
    CHASE_LOAD = 3,
    CHASE_DELAY = 4,
    CHASE_FADE = 5
} ChaseState;

class ChaseTask : public Threadable
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
    bool				m_fading;

    BYTE				m_dmx_fade[ MULTI_UNIV_PACKET_SIZE ];					// DMX fade packet current values
    long				m_channel_delta_ms[ MULTI_UNIV_PACKET_SIZE ];			// DMX fade packet ms deltas
    DWORD				m_channel_next_ms[ MULTI_UNIV_PACKET_SIZE ];			// DMX fade packet ms next

    UINT run(void);

    void computeChannelFade( ULONG fade_time );
    void advanceChannelFade( DWORD current_time );
    ChaseStep* advanceScene( void );

public:
    ChaseTask( Venue* venue );
    ~ChaseTask(void);

    bool start();
    bool stop();

    bool startChase( Chase* chase );
    bool stopChase();

    bool isChaseRunning() const {
        return m_chase != NULL && m_chase_state != CHASE_IDLE;
    }

    bool isFading() const {
        return m_fading;
    }

    Chase* getChase() const {
        return m_chase;
    }

    unsigned getCurrentStep() const {
        return m_step_number;
    }
};

