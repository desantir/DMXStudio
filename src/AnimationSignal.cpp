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


#include "AnimationSignal.h"
#include "AnimationTask.h"

const unsigned AnimationSignal::MAXIMUM_LEVEL = 100;

// ----------------------------------------------------------------------------
//
AnimationSignal::AnimationSignal( DWORD sample_rate_ms, 
                                  AnimationSignalInput input_type, 
                                  DWORD sample_decay,
                                  unsigned input_low,
                                  unsigned input_high,
                                  unsigned scale_factor,
                                  unsigned max_threshold,
                                  ApplySignal apply_to ) :
        m_sample_rate_ms( sample_rate_ms ),
        m_sample_decay_ms( sample_decay ),
        m_input_type( input_type ),
        m_input_low( input_low ),
        m_input_high( input_high ),
        m_scale_factor( scale_factor ),
        m_max_threshold( max_threshold ),
        m_apply_to( apply_to )
{
}

// ----------------------------------------------------------------------------
//
AnimationSignal::~AnimationSignal(void)
{
}

// ----------------------------------------------------------------------------
//
CString AnimationSignal::getSynopsis() const {
    CString synopsis;
    CString input;

    switch ( m_input_type ) {
        case CAI_TIMER_ONLY:
            input = "Timer";
            break;

        case CAI_SOUND_LEVEL:
            input = "Sound level";
            break;

        case CAI_AVG_SOUND_LEVEL:
            input = "Avg sound level";
            break;

        case CAI_FREQ_BEAT:
            input = "Frequency beat %uHz-%uHz";
            break;

        case CAI_FREQ_LEVEL:
            input = "Frequency level %uHz-%uHz";
            break;

        case CAI_RANDOM:
            input = "Random level %u-%u";
            break;
    }

    synopsis.Format( input, m_input_low, m_input_high );
    synopsis.AppendFormat( " rate=%lums", m_sample_rate_ms );

    if ( m_input_type != CAI_TIMER_ONLY ) {
        synopsis.AppendFormat( " curve=%u decay=%lums threshold=%u",
            m_scale_factor, m_sample_decay_ms, m_max_threshold );

        synopsis.Append( " apply=" );

        if ( m_apply_to == 0 )
            synopsis.Append( "none" );
        else {
            if ( isApplyToChannel() )
                synopsis.Append( "values " );
            if ( isApplyToSpeed() )
                synopsis.Append( "speed " );
            if ( isApplyInverseSpeed() )
                synopsis.Append( "-speed " );
        }
    }

    return synopsis;
}

// ----------------------------------------------------------------------------
//
AnimationSignalProcessor::AnimationSignalProcessor( 
            AnimationSignal& signal_definition, 
            AnimationTask* task ) :
    m_signal_def( signal_definition ),
    m_animation_task( task ),
    m_detector( NULL ),
    m_sampler( NULL )
{
    m_next_sample_ms = m_decay_ms = 0;
    m_beat = false;
    m_decay_latch = false;
    m_scale_max = pow( (double)AnimationSignal::MAXIMUM_LEVEL, (double)m_signal_def.getScaleFactor() );

    // Start event generators
    if ( m_signal_def.getInputType() == CAI_FREQ_BEAT ) {
        m_detector = new BeatDetector( 64 );
        m_detector->attach( task->getAudio() );
        m_detector->addFrequencyEvent( &this->m_trigger, m_signal_def.getInputLow(), m_signal_def.getInputHigh() );
    }
    else if ( m_signal_def.getInputType() == CAI_FREQ_LEVEL ) {
        m_sampler = new SoundSampler( 2 );
        m_sampler->attach( task->getAudio() );
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
bool AnimationSignalProcessor::tick( DWORD time_ms )
{
    bool tick = true;

    if ( m_signal_def.getInputType() == CAI_FREQ_BEAT ) {
        bool beat = ( ::WaitForSingleObject( m_trigger.m_hObject, 1 ) == WAIT_OBJECT_0 );

        // Not in a beat and don't have to decay existing beat, just leave
        if ( !beat && (!m_beat || time_ms < m_next_sample_ms) )
            tick = false;
        else
            m_beat = beat;
    }
    else if ( time_ms < m_next_sample_ms ) {
        tick = false;
    }

    if ( !tick ) {
        if ( m_decay_ms && time_ms > m_decay_ms ) {		// Handle value decay
            m_decay_latch = true;
            m_decay_ms = 0;
        }
        return false;
    }

    m_level = 0;

    switch ( m_signal_def.getInputType() ) {
        case CAI_TIMER_ONLY:
            m_level = AnimationSignal::MAXIMUM_LEVEL;
            break;

        case CAI_FREQ_BEAT:
            m_level = (m_beat) ? AnimationSignal::MAXIMUM_LEVEL : 0;
            break;
    
        case CAI_FREQ_LEVEL:
            // The amplitude will be between 0 (no sound) and 150+ (loudness)
            m_level = level_adjust( m_sampler->getLevel( m_signal_def.getInputLow(), m_signal_def.getInputHigh() ) * 100 / 150 );
            break;

        case CAI_SOUND_LEVEL:
            m_level = level_adjust( m_animation_task->getSoundLevel() * 100 / SoundDetector::maxAmplitude );
            break;

        case CAI_AVG_SOUND_LEVEL:
            m_level = level_adjust( m_animation_task->getAvgAmplitude() * 100 / SoundDetector::maxAmplitude );
            break;
    }

    // Determine next time slice

    DWORD sample_rate_ms = m_signal_def.getSampleRateMS();

    switch ( m_signal_def.getInputType() ) {
        case CAI_RANDOM:
            m_level = level_adjust( rand() % (sample_rate_ms+1) );

        case CAI_FREQ_LEVEL:
        case CAI_SOUND_LEVEL:
        case CAI_AVG_SOUND_LEVEL:
            if ( m_signal_def.isApplyToSpeed() ) {
                m_next_sample_ms = time_ms + std::max<int>( 10, (sample_rate_ms * (100-m_level)) / 100 );
                break;
            }
            else if ( m_signal_def.isApplyInverseSpeed() ) {
                m_next_sample_ms = time_ms + std::max<int>( 10, (sample_rate_ms * m_level) / 100 );
                break;
            }

        case CAI_TIMER_ONLY:
        case CAI_FREQ_BEAT:
            m_next_sample_ms = time_ms + sample_rate_ms;
            break;
    }

    if ( m_signal_def.getSampleDecayMS() )
        m_decay_ms = time_ms + m_signal_def.getSampleDecayMS();

    return tick;
}

// ----------------------------------------------------------------------------
//
unsigned AnimationSignalProcessor::getLevel()
{
    if ( m_signal_def.getInputType() == CAI_RANDOM )		// Random values 0 to 100
        m_level = m_signal_def.getInputLow() + 
            (rand() % (m_signal_def.getInputHigh()-m_signal_def.getInputLow()+1));	

    return m_level;
}