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
    m_fading( false ),
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

    DMXStudio::log_status( "Chase %d running", m_chase->getChaseNumber() );

    return true;
}

// ----------------------------------------------------------------------------
//
bool ChaseTask::stopChase() {
    CSingleLock lock( &m_chase_mutex, TRUE );

    if ( m_chase ) {
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

                    m_fade_ms = m_chase->getFadeMS();

                    if ( m_next_state_ms - m_fade_ms > 255  )
                        m_next_state_ms -= m_fade_ms;
                    else
                        m_fade_ms = 0L;

                    m_next_state_ms += GetTickCount();

                    m_chase_state = CHASE_DELAY;
                    break;
                }

                case CHASE_DELAY: {
                    if ( GetTickCount() < m_next_state_ms ) {
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
                    m_fading = true;
                    computeChannelFade( m_fade_ms );
                    m_chase_state = CHASE_FADE;
                }

                case CHASE_FADE: {
                    DWORD current_time = GetTickCount();

                    advanceChannelFade( current_time );

                    if ( current_time >= m_next_state_ms ) {
                        m_fading = false;
                        m_chase_state = CHASE_LOAD;
                    }

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
            return -1;
        }
    }

    DMXStudio::log_status( "Chase task stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
ChaseStep* ChaseTask::advanceScene()
{
    m_step_number = m_next_step;
    ChaseStep* step = m_chase->getStep( m_step_number );

    if ( ++m_next_step >= m_chase->getNumSteps() )
        m_next_step = 0;

    m_venue->selectScene( step->getSceneUID() );

    return step;
}

// ----------------------------------------------------------------------------
//
void ChaseTask::advanceChannelFade( DWORD current_time ) {
    for ( channel_t channel=0; channel < MULTI_UNIV_PACKET_SIZE; channel++ ) {
        if ( m_channel_delta_ms[ channel ] != 0 ) {
            while ( current_time > m_channel_next_ms[ channel ] ) {
                if ( m_channel_delta_ms[ channel ] < 0 ) {
                    if ( m_dmx_fade[ channel ] > 0 )
                        m_dmx_fade[ channel ] -= 1;
                    m_channel_next_ms[ channel ] += -m_channel_delta_ms[ channel ];
                }
                else {
                    if ( m_dmx_fade[ channel ] < 255 )
                        m_dmx_fade[ channel ] += 1;
                    m_channel_next_ms[ channel ] += m_channel_delta_ms[ channel ];
                                        
                }
            }
        }
    }

    m_venue->writePacket( m_dmx_fade );
}

// ----------------------------------------------------------------------------
//
void ChaseTask::computeChannelFade( ULONG fade_time )
{
    // Stop any animations, fade starts at current scene state
    m_venue->clearAnimations();

    // Get the target channel values
    Scene* next_scene = m_venue->getScene( m_chase->getStep( m_next_step )->getSceneUID() );
    BYTE dmx_fade_targets[MULTI_UNIV_PACKET_SIZE];
    memset( dmx_fade_targets, 0, MULTI_UNIV_PACKET_SIZE );
    m_venue->setHomePositions( dmx_fade_targets );

    m_venue->loadSceneChannels( dmx_fade_targets, next_scene );
                
    // Get the current channel values
    m_venue->readPacket( m_dmx_fade );

    memset( m_channel_delta_ms, 0, sizeof(m_channel_delta_ms) );

    DWORD current_time = GetTickCount();

    for ( channel_t channel=0; channel < MULTI_UNIV_PACKET_SIZE; channel++ ) {
        int delta = dmx_fade_targets[channel] - m_dmx_fade[channel];
        if ( delta != 0 ) {
            m_channel_delta_ms[ channel ] = ((long)fade_time)/delta;
            m_channel_next_ms[ channel ] = current_time + abs(m_channel_delta_ms[ channel ]);
        }
    }

    m_next_state_ms = current_time + fade_time;
}