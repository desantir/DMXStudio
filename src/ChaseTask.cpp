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

#include "DMXStudio.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
ChaseTask::ChaseTask( Venue* venue ) :
    Threadable( "ChaseTask" ),
    m_chase(NULL),
    m_venue( venue ),
    m_chase_state(CHASE_IDLE)
{
}

// ----------------------------------------------------------------------------
//
ChaseTask::~ChaseTask(void)
{
    stop();
}

// ----------------------------------------------------------------------------
//
bool ChaseTask::start( ) {
    return startThread();
}

// ----------------------------------------------------------------------------
//
bool ChaseTask::stop() {
    stopChase();

    return stopThread();
}

// ----------------------------------------------------------------------------
//
bool ChaseTask::startChase( Chase* next_chase ) {
    CSingleLock lock( &m_chase_mutex, TRUE );

    stopChase();

    m_chase = next_chase;
    m_chase_state = CHASE_INIT;

    DMXStudio::fireEvent( ES_CHASE, m_chase->getUID(), EA_START );

    DMXStudio::log_status( "Chase %d running", m_chase->getChaseNumber() );

    return true;
}

// ----------------------------------------------------------------------------
//
bool ChaseTask::stopChase() {
    CSingleLock lock( &m_chase_mutex, TRUE );

    if ( m_chase ) {
        DMXStudio::fireEvent( ES_CHASE, m_chase->getUID(), EA_STOP );

        DMXStudio::log_status( "Chase %d stopped", m_chase->getChaseNumber() );

        m_chase = NULL;
        m_chase_state = CHASE_IDLE;
    }

    return true;
}

// ----------------------------------------------------------------------------
//
UINT ChaseTask::run(void) {
    DMXStudio::log_status( "Chase task running" );

    CSingleLock lock( &m_chase_mutex, FALSE );

    while ( isRunning() ) {
        try {
            lock.Lock();

            switch ( m_chase_state ) {
                case CHASE_IDLE:
                    break;

                case CHASE_INIT:
                    m_step_number = m_next_step = 0;
                    m_chase_state = CHASE_LOAD;
                    m_next_state_ms = 0UL;
                    break;

                case CHASE_LOAD: {
                    ChaseStep* step = advanceScene();

                    // Compute length of running scene
                    m_next_state_ms = step->getDelayMS();
                    if ( m_next_state_ms == 0 )
                        m_next_state_ms = m_chase->getDelayMS(); 

                    // No fading if next step is just add/remove actors
                    m_fade_ms = step->getMethod() == SLM_LOAD ? m_chase->getFadeMS() : 0L;

                    if ( m_next_state_ms - m_fade_ms > 255  )
                        m_next_state_ms -= m_fade_ms;
                    else
                        m_fade_ms = 0L;

                    m_next_state_ms += GetTickCount();
                    m_next_prep = true;

                    m_chase_state = CHASE_DELAY;
                    break;
                }

                case CHASE_DELAY: {
                    if ( GetTickCount() < m_next_state_ms ) {
                        if ( m_next_prep ) {
                            m_next_prep = false;
                            prepareForNext();
                        }

                        break;
                    }

                    if ( !m_chase->isRepeat() && m_next_step == 0 && m_step_number > 0 ) {
                        m_venue->selectScene( m_venue->getDefaultScene()->getUID() );
                        stopChase();
                        break;
                    }

                    // Just load the next chase scene
                    if ( m_fade_ms == 0 ) {
                        m_chase_state = CHASE_LOAD;
                        break;
                    }

                    // Setup and fall through
                    fadeToNextScene( m_fade_ms );

                    m_next_state_ms = GetTickCount() + m_fade_ms;
                    m_chase_state = CHASE_FADE;
                }

                case CHASE_FADE: {
                    if ( GetTickCount() >= m_next_state_ms )
                        m_chase_state = CHASE_LOAD;
                    break;
                }
            }

            lock.Unlock();

            Sleep(1);
        }
        catch ( std::exception& ex ) {
            if ( lock.IsLocked() )
                lock.Unlock();

            DMXStudio::log( ex );

            m_chase_state = CHASE_IDLE;
        }
    }

    DMXStudio::log_status( "Chase task stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
void ChaseTask::prepareForNext() {
    ChaseStep* step_next = m_chase->getStep( m_next_step );
    if ( step_next->getMethod() == SLM_MINUS )
        return;

    Scene* futureScene = m_venue->getScene( step_next->getSceneUID() );
    m_venue->stageActors( futureScene->getActors() );
}

// ----------------------------------------------------------------------------
//
ChaseStep* ChaseTask::advanceScene()
{
    m_step_number = m_next_step;
    ChaseStep* step = m_chase->getStep( m_step_number );

    if ( ++m_next_step >= m_chase->getNumSteps() )
        m_next_step = 0;

    m_venue->playScene( step->getSceneUID(), step->getMethod() );

    return step;
}

// ----------------------------------------------------------------------------
//
void ChaseTask::fadeToNextScene( ULONG fade_time )
{
    ChaseStep* next_step = m_chase->getStep( m_next_step );

    if ( fade_time == 0L || next_step->getMethod() == SLM_MINUS )
        return;

    Scene* next_scene = m_venue->getScene( next_step->getSceneUID() );
    
    STUDIO_ASSERT( next_scene != NULL, "Chase next step scene was not found" );

    m_venue->fadeToNextScene( fade_time, next_scene->getActors() );
}