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

unsigned SoundDetector::maxAmplitude = 999;

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
    m_amplitude_buffer( NULL )
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
        m_amplitude_buffer_size = audio->getSamplesPerSecond() / audio->getSampleSize();

        if ( m_amplitude_buffer_size > 0 ) {
            m_amplitude_buffer = new unsigned[m_amplitude_buffer_size];
            memset( m_amplitude_buffer, 0 , sizeof(m_amplitude_buffer) );
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
    for ( size_t i=0; i < sample_size; i++ ) {
        float sample = abs( sample_data[LEFT_CHANNEL][i] );
        if ( sample > peak_amplitude )
            peak_amplitude = sample;

        if ( channels > 1 ) {
            float sample = abs( sample_data[RIGHT_CHANNEL][i] );
            if ( sample > peak_amplitude )
                peak_amplitude = sample;
        }
    }

    // Convert to 0 - 999
    m_amplitude = (unsigned)(peak_amplitude * 1000.0);

    DWORD time = GetTickCount();
    if ( m_amplitude > 0 )
        m_last_sound_ms = time;
    else
        m_last_quiet_ms = time;

    // Compute moving average 
    if ( m_ampbuf_count < m_amplitude_buffer_size )
        m_ampbuf_count++;
    else
        m_ampbuf_sum -= m_amplitude_buffer[m_ampbuf_index];

    m_ampbuf_sum += m_amplitude;
    m_amplitude_buffer[m_ampbuf_index] = m_amplitude;

    if ( ++m_ampbuf_index >= m_amplitude_buffer_size )
        m_ampbuf_index = 0;

    return 0;
}
