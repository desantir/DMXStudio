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


#include "DMXStudio.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
ChaseTask::ChaseTask( Venue* venue, Chase* chase, ChaseRunMode run_mode ) :
    Threadable( "ChaseTask" ),
    m_venue( venue ),
    m_chase( chase ),
    m_run_mode( run_mode ),
    m_trigger( FALSE, FALSE ),
    m_tap_auto( false ),
    m_fading( false ),
    m_beat_detector( NULL )
{
    switch ( run_mode ) {
        case CHASE_MANUAL:
        case CHASE_RECORD:
            m_chase_state = CHASE_TAP_INIT;
            break;
            
        case CHASE_AUTO:
            m_chase_state = CHASE_AUTO_INIT;
            break;
    }
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
    if ( !stopThread() )
        return false;

    if ( m_beat_detector != NULL ) {
        m_beat_detector->detach();
        delete m_beat_detector;
        m_beat_detector = NULL;
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool ChaseTask::followBeat( unsigned start_freq, unsigned end_freq ) {
    if ( m_beat_detector != NULL ) {
        m_beat_detector->detach();
        delete m_beat_detector;
    }

    m_beat_detector = new BeatDetector( 64 );
    m_beat_detector->attach( m_venue->getAudio() );
    m_beat_detector->addFrequencyEvent( getTrigger(), start_freq, end_freq );
    return true;
}

// ----------------------------------------------------------------------------
//
UINT ChaseTask::run(void) {
    bool tap_capture = false;
    unsigned tap_interval = 0;

    DMXStudio::log_status( "Chase %d running", m_chase->getChaseNumber() );

    try {
        while ( isRunning() ) {
            if ( m_venue->getWhiteout() != WHITEOUT_OFF ) {
                Sleep( 10 );
                continue;
            }

            switch ( m_chase_state ) {
                case CHASE_AUTO_INIT:
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
                    m_next_state_ms += GetTickCount();

                    m_chase_state = CHASE_DELAY;
                    break;
                }

                case CHASE_DELAY: {
                    if ( GetTickCount() < m_next_state_ms ) {
                        Sleep( 10 );
                        break;
                    }

                    // If no fade, just load the next chase scene
                    ULONG fade_time = m_chase->getFadeMS();

                    if ( fade_time < 255 ) {					// No sense fading if < 255ms (i.e. byte value)
                        m_chase_state = CHASE_LOAD;
                    }
                    else {
                        m_fading = true;
                        computeChannelFade( fade_time );
                        m_chase_state = CHASE_FADE;
                    }
                    break;
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

                case CHASE_TAP_INIT: {
                    m_step_number = m_next_step = 0;
                    m_chase_state = CHASE_TAP_ADVANCE;
                    tap_capture = false;
                    break;
                }

                case CHASE_TAP_WAIT:
                    if ( GetTickCount() < m_next_state_ms )
                        Sleep( 1 );
                    else
                        m_chase_state = CHASE_TAP_ADVANCE;
                    break;

                case CHASE_TAP_ADVANCE: {
                    advanceScene();

                    if ( !m_tap_auto || m_tap_intervals.size() == 0 ) {
                        m_chase_state = CHASE_TAP_RECORD;
                        m_next_state_ms = GetTickCount();
                    }
                    else {
                        m_chase_state = CHASE_TAP_WAIT;
                        m_next_state_ms = GetTickCount()+m_tap_intervals[tap_interval];
                        tap_interval = (tap_interval+1) % m_tap_intervals.size();
                    }
                    break;
                }

                case CHASE_TAP_RECORD: {
                    if ( ::WaitForSingleObject( m_trigger.m_hObject, 100 ) == WAIT_OBJECT_0 ) {
                        if ( CHASE_RECORD == m_run_mode ) {
                            if ( !tap_capture ) {					// First tap starts capture
                                tap_capture = true;
                                m_next_state_ms = GetTickCount();
                            }
                            else if ( !m_tap_auto || m_tap_intervals.size() == 0) {
                                DWORD now = GetTickCount();
                                m_tap_intervals.push_back( now-m_next_state_ms );
                                m_next_state_ms = now;
                            }
                            else {
                                m_step_number = m_next_step = tap_interval = 0;
                            }
                        }

                        m_chase_state = CHASE_TAP_ADVANCE;
                    }
                    break;
                }
            }
        }
    }
    catch ( StudioException& ex ) {
        DMXStudio::log( ex );
        return -1;
    }
    catch ( std::exception& ex ) {
        DMXStudio::log( ex );
        return -1;
    }

    DMXStudio::log_status( "Chase %d stopped", m_chase->getChaseNumber() );

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
    for ( channel_t channel=0; channel < DMX_PACKET_SIZE; channel++ ) {
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