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

#include "stdafx.h"
#include "AudioInputStream.h"

class SampleEntry {
    AudioChannel			m_channel;
    unsigned				m_bin;
    float					m_frequency;
    int						m_db;
    unsigned				m_power;

    // Buffer and values to compute power moving average
    unsigned				m_power_buffer[ 16 ];
    unsigned				m_power_index;
    unsigned				m_power_sum;
    unsigned				m_power_count;

public:
    SampleEntry( AudioChannel channel, unsigned bin );
    ~SampleEntry() {}

    inline float getFrequency() const {
        return m_frequency;
    }

    inline AudioChannel getChannel() const {
        return m_channel;
    }

    inline unsigned getBin() const {
        return m_bin;
    }

    inline unsigned getPower() const {
        return m_power;
    }

    inline unsigned getDB() const {
        return m_db;
    }

    inline unsigned getAvgPower() const {
        return m_power_sum / m_power_count;
    }

    void setSample( float frequency, unsigned power, int db );
};

typedef std::vector<SampleEntry> SampleSet;

class SoundSampler : public IAudioProcessor
{
    AudioInputStream*		m_audio_stream;
    unsigned				m_channels;
    CMutex              	m_mutex;
    SampleSet				m_samples;
    ULONG					m_sample_number;
    CEvent*					m_listener;

    SoundSampler(SoundSampler& other) {}
    SoundSampler& operator=(SoundSampler& rhs) { return *this; }

public:
    SoundSampler( unsigned channels, CEvent* listener=NULL );
    ~SoundSampler(void);

    void attach( AudioInputStream* stream );
    void detach();

    inline bool isAttached() const {
        return m_audio_stream != NULL;
    }

    HRESULT ProcessFFT( WORD channels, FFT_Result* fft_result[] );  
    HRESULT ProcessAmplitudes( WORD channels, size_t sample_size, float* sample_data[] ) { return 0; }

    void Unlink( AudioInputStream* audio ) {
        detach();
    }

    bool setFrequencySamples( unsigned channels, const unsigned* frequencies );

    SampleSet getSampleSet( ULONG &sample_number );
    int getLevel( unsigned freq_low, unsigned freq_high );
    int getDB( unsigned freq_low, unsigned freq_high );

private:
    void processChannelFFT( AudioChannel channel, FFT_Result* fft_data );
};




