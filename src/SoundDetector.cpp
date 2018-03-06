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

#include "SoundDetector.h"

// NOTE: The sound capture device level can affect this maximum
unsigned SoundDetector::maxAmplitude = 1000;

// ----------------------------------------------------------------------------
//
SoundDetector::SoundDetector( DWORD mute_ms ) :
    m_audio_stream( NULL ),
    m_mute_ms( mute_ms ),
    m_amplitude( 0 ),
    m_last_sound_ms( 0 ),
    m_ampbuf_index( 0 ),
    m_ampbuf_sum( 0 ),
    m_ampbuf_count( 0 ),
    m_amplitude_buffer( NULL ),
    m_amplitude_beat( 0 ),
    m_avg_amplitude( 0 ),
    m_beat_index( 0 )
{
}

// ----------------------------------------------------------------------------
//
SoundDetector::~SoundDetector(void)
{
    detach();
}

// ----------------------------------------------------------------------------
//
void SoundDetector::attach( AudioInputStream* audio ) {
    detach();

    m_audio_stream = audio;

    if ( audio ) {
        m_amplitude_buffer_size = audio->getSamplesPerSecond() / audio->getSampleSize();    // Approx 1 second of samples

        if ( m_amplitude_buffer_size > 0 ) {
            m_amplitude_buffer = new unsigned[m_amplitude_buffer_size];
            memset( m_amplitude_buffer, 0 , m_amplitude_buffer_size * sizeof(unsigned) );
        }

        m_audio_stream->addAudioProcessor( this, ProcessorFormat( 2, false) );
    }
}

// ----------------------------------------------------------------------------
//
void SoundDetector::detach( ) {
    if ( m_audio_stream ) {
        m_audio_stream->removeAudioProcessor( this );

        if ( m_amplitude_buffer ) 
            delete m_amplitude_buffer;

        m_audio_stream = NULL;
        m_amplitude_buffer = NULL;
    }
}

// ----------------------------------------------------------------------------
//
HRESULT SoundDetector::ProcessAmplitudes( WORD channels, size_t sample_size, float* sample_data[] ) {

    // Determine peak amplitude for both channels (values approach 1.0 where .999 is loudest and .000 is no sound)
    float peak_amplitude = 0;
	float sample;

    for ( size_t i=0; i < sample_size; i++ ) {
        sample = sample_data[LEFT_CHANNEL][i] < 0.0 ? -sample_data[LEFT_CHANNEL][i] : sample_data[LEFT_CHANNEL][i];
        if ( sample > peak_amplitude )
            peak_amplitude = sample;

        if ( channels > 1 ) {
            sample = sample_data[RIGHT_CHANNEL][i] < 0.0 ? -sample_data[RIGHT_CHANNEL][i] : sample_data[RIGHT_CHANNEL][i];
            if ( sample > peak_amplitude )
                peak_amplitude = sample;
        }
    }

    DWORD time_ms = GetTickCount();

    CSingleLock( &m_access_lock, true );

    // Convert to 0 - 1000
    m_amplitude = std::min<unsigned>( (unsigned)(peak_amplitude * 1000.0), SoundDetector::maxAmplitude );

    if ( m_amplitude > 0 )
        m_last_sound_ms = time_ms;
    else
        m_last_quiet_ms = time_ms;

    // Compute moving average 
    if ( m_ampbuf_count < m_amplitude_buffer_size )
        m_ampbuf_count++;
    else
        m_ampbuf_sum -= m_amplitude_buffer[m_ampbuf_index];

    m_ampbuf_sum += m_amplitude;
    m_amplitude_buffer[m_ampbuf_index] = m_amplitude;

    if ( ++m_ampbuf_index >= m_amplitude_buffer_size )
        m_ampbuf_index = 0;

    m_avg_amplitude = m_ampbuf_count == 0 ? 0 : m_ampbuf_sum/m_ampbuf_count;

    // Primitive peak (beat) detection

    if ( m_amplitude > 0 && m_avg_amplitude > 0 ) {
        unsigned beat_intensity = (m_amplitude*100) / m_avg_amplitude;

        if ( beat_intensity < 110 ) {
            m_amplitude_beat = 0;
            m_beat_index = 0;
        }
        else if ( m_beat_index + 200 < time_ms ) {
           m_amplitude_beat = beat_intensity - 100;
           m_beat_index = time_ms;
        }
    }
    else {
        m_amplitude_beat = 0;
        m_beat_index = 0;
    }

    return 0;
}

// ----------------------------------------------------------------------------
//
void SoundDetector::getSoundData( SoundLevel& level ) {
    CSingleLock( &m_access_lock, true );

    level.amplitude = m_amplitude;
    level.avg_amplitude = m_avg_amplitude;
    level.amplitude_beat = m_amplitude_beat;
    level.beat_index = m_beat_index;
}