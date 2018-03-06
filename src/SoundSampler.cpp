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

#include "SoundSampler.h"

static const unsigned DEFAULT_FREQUENCIES[] = { 32, 64, 128, 300, 500, 800, 1000, 2000, 4000, 5000, 8000, 10000, 16000, 20000, 0 };

// ----------------------------------------------------------------------------
//
SoundSampler::SoundSampler( unsigned channels,  CEvent* listener ) :
    m_audio_stream( NULL ),
    m_channels( channels ),
    m_listener( listener )
{
}

// ----------------------------------------------------------------------------
//
SoundSampler::~SoundSampler(void)
{
    detach();
}

// ----------------------------------------------------------------------------
//
void SoundSampler::attach( AudioInputStream* audio ) {
    CSingleLock lock( &m_mutex, TRUE );

    detach();

    m_audio_stream = audio;

    if ( audio ) {
        m_audio_stream->addAudioProcessor( this, ProcessorFormat( m_channels, true ) );

        setFrequencySamples( m_channels, DEFAULT_FREQUENCIES );
    }
}

// ----------------------------------------------------------------------------
//
void SoundSampler::detach( ) {
    CSingleLock lock( &m_mutex, TRUE );

    if ( m_audio_stream ) {
        m_audio_stream->removeAudioProcessor( this );
        m_audio_stream = NULL;
    }
}

// ----------------------------------------------------------------------------
//
bool SoundSampler::setFrequencySamples( unsigned channels, const unsigned* frequencies ) {
    CSingleLock lock( &m_mutex, TRUE );

    m_channels = channels;
    m_samples.clear();
    m_sample_number = 0;

    // Setup bins
    unsigned sample_size = m_audio_stream->getSampleSize();
    unsigned samples_per_second = m_audio_stream->getSamplesPerSecond();

    for ( unsigned channel=0; channel < channels; channel++ ) {
        for ( unsigned f=0; frequencies[f] != 0; f++ ) {
            unsigned bin = (unsigned)((frequencies[f] * sample_size) / samples_per_second);
            m_samples.emplace_back( (AudioChannel)channel, bin );
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
SampleSet SoundSampler::getSampleSet( ULONG &sample_number )
{
    CSingleLock lock( &m_mutex, TRUE );

    sample_number = m_sample_number;

    return m_samples;
}

// ----------------------------------------------------------------------------
// Returns frequency range amplitude average
int SoundSampler::getLevel( unsigned freq_low, unsigned freq_high )
{
    unsigned sample_size = m_audio_stream->getSampleSize();
    unsigned samples_per_second = m_audio_stream->getSamplesPerSecond();

    unsigned bin_start = ((freq_low * sample_size) / samples_per_second);
    unsigned bin_end = ((freq_high * sample_size) / samples_per_second);

    CSingleLock lock( &m_mutex, TRUE );

    ULONG total = 0;
    int count = 0;

    for ( SampleSet::iterator it=m_samples.begin(); it != m_samples.end(); ++it ) {
        SampleEntry& entry = (*it);
        if ( entry.getBin() >= bin_start && entry.getBin() <= bin_end ) {
            unsigned power = entry.getPower();			// entry.getPower();
            if ( power > 0 ) {
                total += power; 
                count++;
            }
        }
    }

    return count > 0 ? (int)sqrt(total/count) : 0;
}


// ----------------------------------------------------------------------------
// Returns frequency range dB average
int SoundSampler::getDB( unsigned freq_low, unsigned freq_high )
{
    unsigned sample_size = m_audio_stream->getSampleSize();
    unsigned samples_per_second = m_audio_stream->getSamplesPerSecond();

    unsigned bin_start = ((freq_low * sample_size) / samples_per_second);
    unsigned bin_end = ((freq_high * sample_size) / samples_per_second);

    CSingleLock lock( &m_mutex, TRUE );

    LONG total = 0;
    int count = 0;

    for ( SampleSet::iterator it=m_samples.begin(); it != m_samples.end(); ++it ) {
        SampleEntry& entry = (*it);
        if ( entry.getBin() >= bin_start && entry.getBin() <= bin_end ) {
            unsigned db = entry.getDB();	
            if ( db > 0 ) {
                total += db; 
                count++;
            }
        }
    }

    return count > 0 ? total/count : -100;
}

// ----------------------------------------------------------------------------
//
HRESULT SoundSampler::ProcessFFT( WORD channels, FFT_Result* fft_result[] )
{
    CSingleLock lock( &m_mutex );

    if ( lock.Lock( 0 ) ) {			// Don't wait on the lock - but if we get it process data
        processChannelFFT( LEFT_CHANNEL, fft_result[LEFT_CHANNEL] );

        if ( m_channels > 1 && channels > 1 )
            processChannelFFT( RIGHT_CHANNEL, fft_result[RIGHT_CHANNEL] );

        m_sample_number++;

        if ( m_listener )			// Notify listener (if we have one)
            m_listener->SetEvent();
    }

    return 0;
};

// ----------------------------------------------------------------------------
//
void SoundSampler::processChannelFFT( AudioChannel channel, FFT_Result* fft_data )
{
    // Determine frequency and dB
    for ( SampleSet::iterator it=m_samples.begin(); it != m_samples.end(); ++it ) {
        SampleEntry& entry = (*it);

        if ( entry.getChannel() == channel ) {
            unsigned bin = entry.getBin();

            entry.setSample( fft_data->getFrequency( bin ),
                             (unsigned)fft_data->getPower( bin ),
                             fft_data->getDB( bin ) );

            //double phase = 180.0F * atan2( sinBin[bin], cosBin[bin]) / M_PI - 90.0F;
        }
    }
}

// ----------------------------------------------------------------------------
//
SampleEntry::SampleEntry( AudioChannel channel, unsigned bin ) :
        m_channel(channel),
        m_bin(bin),
        m_frequency(0),
        m_db(0),
        m_power(0),
        m_power_index(0),
        m_power_sum(0),
        m_power_count(0)
{
}

// ----------------------------------------------------------------------------
//
void SampleEntry::setSample( float frequency, unsigned power, int db )
{
    m_frequency = frequency;
    m_power = power;

    if ( db < -110 )
        m_db = -110;
    else if ( db > 0 )
        m_db = 0;
    else
        m_db = db;

    // Compute moving average 
    static const unsigned MAX_SAMPLE = sizeof(m_power_buffer) / sizeof(unsigned);

    if ( m_power_count < MAX_SAMPLE )
        m_power_count++;
    else
        m_power_sum -= m_power_buffer[m_power_index];

    m_power_sum += m_power;
    m_power_buffer[m_power_index] = m_power;

    if ( ++m_power_index >= MAX_SAMPLE )
        m_power_index = 0;
}