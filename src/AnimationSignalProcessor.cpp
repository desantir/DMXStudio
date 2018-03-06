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

#include "AnimationSignalProcessor.h"
#include "AnimationEngine.h"
#include "AnimationTask.h"

#define REPORT_MS       25

// ----------------------------------------------------------------------------
//
AnimationSignalProcessor::AnimationSignalProcessor( 
    AnimationSignal& signal_definition, 
    AnimationEngine* engine ) :
    m_signal_def( signal_definition ),
    m_engine( engine ),
    m_detector( NULL ),
    m_sampler( NULL ),
    m_next_sample_ms( 0 ),
    m_next_off_ms( 0 ),
    m_level( 0 ),
    m_wave_value( 0 ),
    m_last_beat_index( 0 ),
    m_ring_head( 0 ),
    m_ring_count( 0 ),
    m_level_expire_ms( 0 ),
    m_next_report_ms( 0 )
{
    m_amplitude = m_signal_def.getLevelCeil()-m_signal_def.getLevelFloor();

    // Precompute wave increments
    switch ( m_signal_def.getSource() ) {
        case SOURCE_SAWTOOTH: {          // Sawtooth of 'n' trigger periods
            double increment = (double)m_amplitude / (double)m_signal_def.getLevelPeriods();
            double value = m_signal_def.getLevelFloor();

            for ( UINT i=0; i < m_signal_def.getLevelHold(); i++ )
                m_values.push_back( (UINT)value );

            for ( UINT i=0; i < m_signal_def.getLevelPeriods(); i++ ) {
                if ( i < m_signal_def.getLevelPeriods() )
                    value += increment;

                m_values.push_back( (UINT)(value + 0.5) );
            }

            for ( UINT i=0; i < m_signal_def.getLevelHold(); i++ )
                m_values.push_back( m_signal_def.getLevelCeil() );

            break;
        }

        case SOURCE_TRIANGLEWAVE: {      // Triangle wave of 'n' trigger periods
            double increment = ((double)m_amplitude / (double)m_signal_def.getLevelPeriods() );
            double value = m_signal_def.getLevelFloor();

            for ( UINT i=0; i < m_signal_def.getLevelPeriods() * 2.0; i++ ) {
                if ( i == m_signal_def.getLevelPeriods() ) {
                    for ( UINT j=0; j < m_signal_def.getLevelHold(); j++ )
                        m_values.push_back( m_signal_def.getLevelCeil() );

                    continue;
                }
                
                if ( i < m_signal_def.getLevelPeriods() )
                    value += increment;
                else
                    value -= increment;

                m_values.push_back( (UINT)(value + 0.5) );
            }

            for ( UINT i=0; i < m_signal_def.getLevelHold(); i++ )
                m_values.push_back( m_signal_def.getLevelFloor() );

            break;
        }

        case SOURCE_SQUAREWAVE: {         // Squarewave of 'n' trigger periods
            for ( UINT j=0; j < m_signal_def.getLevelPeriods()+m_signal_def.getLevelHold(); j++ )
                m_values.push_back( m_signal_def.getLevelFloor() );

            for ( UINT j=0; j < m_signal_def.getLevelPeriods()+m_signal_def.getLevelHold(); j++ )
                m_values.push_back( m_signal_def.getLevelCeil() );

            break;
        }

        case SOURCE_STEPWAVE: {             // Stepwave of 'n' steps
            UINT value = 0;
            UINT periods = m_signal_def.getLevelPeriods();
            UINT duration = (m_signal_def.getLevelHold() > 0) ? m_signal_def.getLevelHold() : 1;
            UINT rise = m_amplitude / periods;

            for ( UINT i=0; i < periods+1; i++ ) {
                for ( UINT j=0; j < duration; j++ )
                    m_values.push_back( m_signal_def.getLevelFloor() + value );

                value += rise;
            }

            break;
        }

        case SOURCE_STEPWAVE_FULL: {            // Stepwave of 'n' steps (full up and down)
            UINT value = 0;
            UINT periods = m_signal_def.getLevelPeriods()*2;
            UINT duration = (m_signal_def.getLevelHold() > 0) ? m_signal_def.getLevelHold() : 1;
            UINT rise = m_amplitude / (m_signal_def.getLevelPeriods());

            for ( UINT i=0; i < periods; i++ ) {
                for ( UINT j=0; j < duration; j++ )
                    m_values.push_back( m_signal_def.getLevelFloor() + value );

                if ( i >= m_signal_def.getLevelPeriods() )
                    value -= rise;
                else
                    value += rise;
            }

            break;
        }

        case SOURCE_SINEWAVE: {
            double step = 360.0/m_signal_def.getLevelPeriods();

            double half_amplitude = (double)m_amplitude / 2;

            for ( double angle=0; angle < 360.0-step; angle += step ) {
                double radians = angle * M_PI / 180.0;

                // Compute wave around amplitude midpoint based at floor
                m_values.push_back( std::max<UINT>( static_cast<UINT>(m_signal_def.getLevelFloor() + half_amplitude + (sin(radians) * half_amplitude) ), 0 ) );
            }

            for ( UINT i=0; i < m_signal_def.getLevelHold(); i++ ) {
                m_values.push_back( static_cast<UINT>( m_signal_def.getLevelFloor() + half_amplitude ) );
            }

            break;
        }
    }

    // Start event generators
    if ( m_signal_def.getTrigger() == TRIGGER_FREQ_BEAT ) {
        m_detector = new BeatDetector( 64 );
        m_detector->attach( engine->getAudio() );
        m_detector->addFrequencyEvent( &this->m_trigger, m_signal_def.getFreqLowHz(), m_signal_def.getFreqHighHz() );
    }
    else if ( m_signal_def.getSource() == SOURCE_FREQ ) {
        m_sampler = new SoundSampler( 2 );
        m_sampler->attach( engine->getAudio() );
    }
}

// ----------------------------------------------------------------------------
//
AnimationSignalProcessor::~AnimationSignalProcessor( )
{
    if ( m_detector ) {
        m_detector->removeFrequencyEvent( &this->m_trigger );
        delete m_detector;
        m_detector = NULL;
    }

    if ( m_sampler ) {
        m_sampler->detach();
        delete m_sampler;
        m_sampler = NULL;
    }
}

// ----------------------------------------------------------------------------
//
SignalState AnimationSignalProcessor::tick( DWORD time_ms )
{
    bool update_sound_level = true;
    bool trigger = false;
    bool force_off = false;

    // See if we are waiting on an off signal
    if ( m_next_off_ms > 0 ) {
        if ( time_ms < m_next_off_ms ) {
            if ( time_ms > m_next_report_ms ) {
                record_level( time_ms, m_level );
                m_next_report_ms = time_ms + REPORT_MS;
            }        

            return SIGNAL_NO_CHANGE;
        }

        force_off = true;
    }

    // See if we hit a trigger
    switch( m_signal_def.getTrigger() ) {
        case TRIGGER_TIMER:
        case TRIGGER_RANDOM_TIMER:
            trigger = ( time_ms >= m_next_sample_ms );
            break;
            
        case TRIGGER_AMPLITUDE_BEAT: {
            m_engine->getSoundLevels( m_sound_level );
            update_sound_level = false;

            trigger = ( m_last_beat_index != m_sound_level.beat_index &&
                        m_sound_level.amplitude_beat > m_signal_def.getSensitivity() );

            m_last_beat_index = m_sound_level.beat_index;
            break;
        }

        case TRIGGER_FREQ_BEAT:
            trigger = ( ::WaitForSingleObject( m_trigger.m_hObject, 1 ) == WAIT_OBJECT_0 );
            break;
    }

    if ( !trigger ) {
        if ( force_off ) {
            m_next_off_ms = 0;
            m_level = level_adjust( 0 );

            record_level( time_ms, m_level );

            return SIGNAL_LEVEL_OFF;
        }

        if ( time_ms > m_next_report_ms ) {
            record_level( time_ms, m_level );
            m_next_report_ms = time_ms + REPORT_MS;
        }
        
        return SIGNAL_NO_CHANGE;
    }

    // Got a trigger, figure out the level
    switch ( m_signal_def.getSource() ) {
        case SOURCE_HIGH:                // Always returns high
            m_level = level_adjust( MAXIMUM_LEVEL );
            break;

        case SOURCE_LOW:                 // Always returns low
            m_level = level_adjust( 0 );
            break;

        case SOURCE_AMPLITUDE: {         // Current amplitude scaled 0-MAXIMUM_LEVEL
            if ( update_sound_level )
                m_engine->getSoundLevels( m_sound_level );

            m_level = peak_hold( time_ms, level_adjust( m_sound_level.amplitude * MAXIMUM_LEVEL / SoundDetector::maxAmplitude ) );
            break;
        }

        case SOURCE_AVG_AMPLITUDE: {     // Current average amplitude scaled 0-MAXIMUM_LEVEL
            if ( update_sound_level )
                m_engine->getSoundLevels( m_sound_level );

            m_level = peak_hold( time_ms, level_adjust( m_sound_level.avg_amplitude * MAXIMUM_LEVEL / SoundDetector::maxAmplitude ) );
            break;
        }

        case SOURCE_BEAT_INTENSITY: {   // Amplitude beat intensity
            if ( update_sound_level )
                m_engine->getSoundLevels( m_sound_level );

            m_level = peak_hold( time_ms, level_adjust( m_sound_level.amplitude_beat * MAXIMUM_LEVEL / SoundDetector::maxAmplitude ) );
            break;
        }

        case SOURCE_FREQ: {              // Frequency range max scaled 0-MAXIMUM_LEVEL
            int dB = std::max<int>( m_sampler->getDB( m_signal_def.getFreqLowHz(), m_signal_def.getFreqHighHz() ), -100 );
            dB = 100 + dB;          // flip as it is it a value from -100 to 0

            m_level = peak_hold( time_ms, level_adjust( dB * MAXIMUM_LEVEL / 100 ) );          
            break;
        }

        case SOURCE_VOLUME:              // Current volume level
            m_level = level_adjust( (m_engine->getVolume() * MAXIMUM_LEVEL) / MAXIMUM_VOLUME );
            break;

        case SOURCE_SOUND:               // Current mute detection state
            m_level = level_adjust( m_engine->isMute() ? 0 : MAXIMUM_LEVEL );
            break;

        case SOURCE_SQUAREWAVE:          // Squarewave of 'n' trigger periods
        case SOURCE_TRIANGLEWAVE:        // Triangle wave of 'n' trigger periods
        case SOURCE_SAWTOOTH:            // Sawtooth of 'n' trigger periods
        case SOURCE_SINEWAVE:            // Sinewave of 'n' trigger periods
        case SOURCE_STEPWAVE_FULL:       // Stepwave of 'n' steps (full)
        case SOURCE_STEPWAVE: {          // Stepwave of 'n' steps (half)
            if ( m_values.size() == 0 ) {
                m_level = 0;
                break;
            }

            if ( m_wave_value >= m_values.size() )
                m_wave_value = 0;

            m_level = level_adjust( m_values[m_wave_value++] );
            break;
        }

        case SOURCE_RANDOM:              // Random level 0-100 of 'n' trigger periods
            if ( m_wave_value++ == 0 )
                m_level = level_adjust( m_signal_def.getLevelFloor() + (rand() % (m_signal_def.getLevelCeil()-m_signal_def.getLevelFloor()+1)) );

            if ( m_wave_value >= m_signal_def.getLevelPeriods() )
                m_wave_value = 0;

            break;
    }
    
    DWORD off_timer_ms = m_signal_def.getOffMS();

    // Setup next timer interval
    switch( m_signal_def.getTrigger() ) {
        case TRIGGER_TIMER:
            m_next_sample_ms = time_ms + speed_adjust( m_signal_def.getTimerMS() );
            break;

        case TRIGGER_RANDOM_TIMER:
            m_next_sample_ms = time_ms + speed_adjust( (rand() % (m_signal_def.getTimerMS()+1)) );
            break;

        case TRIGGER_AMPLITUDE_BEAT:
        case TRIGGER_FREQ_BEAT:
            if ( off_timer_ms == 0 && m_signal_def.getSource() == SOURCE_HIGH )
                off_timer_ms = 25;
            break;
    }

    // Setup for off signal
    if ( off_timer_ms > 0 )
        m_next_off_ms = time_ms + speed_adjust( off_timer_ms );
    
    m_next_report_ms = time_ms + REPORT_MS;

    record_level( time_ms, m_level );

    return SIGNAL_NEW_LEVEL;
}

// ----------------------------------------------------------------------------
//
UINT AnimationSignalProcessor::level_adjust( unsigned level ) {
    if ( m_signal_def.getLevelScale() != 1 )
        level *= m_signal_def.getLevelScale();

    // Make sure the level is in range
    if ( level > m_signal_def.getLevelCeil() )
        level =  m_signal_def.getLevelCeil();
    else if ( level < m_signal_def.getLevelFloor() )
        level = m_signal_def.getLevelFloor();

    if ( m_signal_def.isLevelInvert() )
        level = ( m_amplitude - (level - m_signal_def.getLevelFloor()) )  + m_signal_def.getLevelFloor();

    return level;
}

// ----------------------------------------------------------------------------
//
UINT AnimationSignalProcessor::peak_hold( DWORD time_ms, UINT level ) {
    DWORD levelHoldMS = m_signal_def.getLevelHold(); 

    if ( levelHoldMS == 0 )
        return level;

    if ( m_level_expire_ms < time_ms ) {
        m_level_expire_ms = time_ms + speed_adjust( levelHoldMS * m_signal_def.getTimerMS() );
        return level;
    }

    return  m_level;
}

// ----------------------------------------------------------------------------
//
DWORD AnimationSignalProcessor::speed_adjust( DWORD speed_ms ) {

    if ( m_signal_def.getSpeedAdjust() == SPEED_FAST ) {
        speed_ms = (speed_ms * (MAXIMUM_LEVEL-m_level)) / MAXIMUM_LEVEL;
    }
    else if ( m_signal_def.getSpeedAdjust() == SPEED_SLOW ) {
        speed_ms = (speed_ms * (MAXIMUM_LEVEL+m_level)) / MAXIMUM_LEVEL;
    }

    return std::max<DWORD>( 25, m_engine->scaleAnimationSpeed( speed_ms ) );
}

// ----------------------------------------------------------------------------
//
void AnimationSignalProcessor::record_level( DWORD time_ms, UINT level ) {
    m_level_ring_buffer[m_ring_head].time_ms = time_ms;
    m_level_ring_buffer[m_ring_head].level = level;

    m_ring_head += 1;
    
    if ( m_ring_head >= LEVEL_HISTORY_SIZE )
        m_ring_head = 0;

    if ( m_ring_count < LEVEL_HISTORY_SIZE )
        m_ring_count++;
}

// ----------------------------------------------------------------------------
// NOTE: We call this while we have the animation lock, so the buffer should not 
// by updated during this call.
LevelRecordArray AnimationSignalProcessor::fetchLevelData( DWORD after_ms ) {

    LevelRecordArray result;
    result.reserve( LEVEL_HISTORY_SIZE / 2 );

    size_t count = m_ring_count;
    size_t index = count < LEVEL_HISTORY_SIZE ? 0 : m_ring_head;

    while ( count-- > 0 ) {
        if ( index >= LEVEL_HISTORY_SIZE )
            index = 0;

        if ( m_level_ring_buffer[index].time_ms > after_ms )
            result.push_back( m_level_ring_buffer[index] );

        index++;
    }

    return result;
}
