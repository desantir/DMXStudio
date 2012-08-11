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

#include "DMXStudio.h"
#include "ffft/FFTReal.h"
#include "Threadable.h"

/*
    After calling the FFT, the output arrays contain the Real and Imaginary parts of the transform/analysis, but are only meaningful up to N/2 (from N/2 to N, the results are essentially a mirror/dupicate of those from 0 to N/2).
    The Imaginary parts correspond to the amplitudes of the original waveform analysed at 90-degrees phase offset to the Real values.

    To get the overall (phase-independent) amplitude of any given frequency-bin, you take the sqrt(Real*Real+Imag*Imag)

    And for bin n, where 0<=n<N/2
    the nominal/centre frequency for bin n is samplerate(kHz)*n/N

    To convert amplitude to dBFS
        dBFS = 20*log10(amplitude/32767)
    where 32767 is your maximum amplitude / 0dBFS point.

    To save yourself computing a sqrt which is slow, use power = Real*Real+Imag*Imag , then

    dBFS = 10*log10( power/(32767*32767))  (bsd *2?)
*/

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

typedef enum {
    LEFT_CHANNEL = 0,
    RIGHT_CHANNEL = 1
} AudioChannel;

struct AudioCaptureDevice {
    CStringW	m_id;
    CString		m_friendly_name;
    bool		m_isDefault;

    AudioCaptureDevice( LPWSTR id, LPSTR friendly_name, bool isDefault ) :
        m_id( id ),
        m_friendly_name( friendly_name ),
        m_isDefault( isDefault )
    {}
};

typedef std::vector<AudioCaptureDevice> AudioCaptureDeviceArray;

class FFT_Result {
    friend class AudioInputStream;

    unsigned		m_samples;
    unsigned		m_sample_rate;
    float*			m_dft;
    float*			m_dfti;

public:
    FFT_Result( unsigned sample_size, unsigned sample_rate ) :
        m_samples( sample_size ),
        m_sample_rate( sample_rate )
    { 
        m_dft = new float[ sample_size ];
        m_dfti = &m_dft[ sample_size/2 ];
    }

    ~FFT_Result(void) {
        delete m_dft;
    }

    void clear() {
        for ( unsigned i=0; i < m_samples; i++ )
            m_dft[i] = 0.0;
    }

    inline unsigned getSampleSize() const {
        return m_samples/2;
    }

    inline unsigned getFrequencyBin( float frequency ) {
        return (unsigned)(frequency * m_samples) / m_sample_rate;
    }

    inline float getFrequency( int bin ) {
        return ((float)(bin) * (float)m_sample_rate) / (float)m_samples;
    }

    // Each complex number (real,imaginary) represents the magnitude and phase of a frequency

    inline float getReal( unsigned bin ) const {
        if ( bin >= m_samples/2 )
            return 0.0F;

        return m_dft[ bin ];
    }

    inline float getImaginary( unsigned bin ) const {
        if ( bin >= m_samples/2 )
            return 0.0F;

        return m_dfti[ bin ];
    }

    inline float getPower( unsigned bin ) const {
        if ( bin >= m_samples/2 )
            return 0.0F;
        return m_dft[ bin ] * m_dft[bin] + m_dfti[bin] * m_dfti[bin];
    }

    // Frequency amplitude
    inline float getAmplitude( unsigned bin ) const {
        return sqrt( getPower( bin ) );		// Modulus of complex number = sqrt( r^2 * i^2 )
    }

    inline int getDB( unsigned bin ) const {
        return (int)(10.0F * log10( getPower(bin) / (32767.0F * 4.0f) ));	// * The 32767.0F * 4.0f is an estimate (seems about right)
    }

protected:
    void process( ffft::FFTReal <float>& fft, float* sampleSet ) {
        fft.do_fft( m_dft, sampleSet );			// FFT
    }
};

class IAudioProcessor {

public:
    virtual HRESULT ProcessFFT( WORD channels, FFT_Result* fft_result[] ) = 0;
    virtual HRESULT ProcessAmplitudes( WORD channels, size_t sample_size, float* sample_data[] ) = 0;
};

struct ProcessorFormat
{
    WORD		m_channels;
    bool		m_fft;

    ProcessorFormat( ) {}
    ProcessorFormat( WORD channels, bool fft ) :
        m_channels( channels ),
        m_fft( fft )
    {}
};

typedef std::map< IAudioProcessor*, ProcessorFormat > AudioProcessorMap;

class AudioInputStream : public Threadable
{
    CStringW					m_endpoint_id;				// Device endpoint ID
    unsigned					m_sample_size;
    WAVEFORMATEXTENSIBLE		m_format;
    ffft::FFTReal <float>		m_fft_object;
    float                       m_scale_factor;
    float                       m_audio_sample_min;

    UINT32						m_bufferFrameCount;
    IMMDeviceEnumerator *		m_pEnumerator;
    IMMDevice *					m_pDevice;
    IAudioClient *				m_pAudioClient;
    IAudioCaptureClient *		m_pCaptureClient;
    WAVEFORMATEX *				m_pwfx;

    // Audio sample storage 
    float*						m_sample_left;
    float*						m_sample_right;

    unsigned					m_data_index;

    CCriticalSection			m_sink_mutex;
    AudioProcessorMap			m_sinks;

    FFT_Result *				m_fft_channels[2];

    UINT run(void);

public:
    AudioInputStream( LPCWSTR endpoint_id, unsigned sample_size=1024, float scale_factor=0.0, float audio_sample_min=0.0009 );
    virtual ~AudioInputStream(void);

    WAVEFORMATEXTENSIBLE getFormat( ) const {
        return m_format;
    }

    unsigned getSampleSize() const {
        return m_sample_size;
    }

    unsigned getSamplesPerSecond() const {
        return m_format.Format.nSamplesPerSec;
    }

    virtual HRESULT openAudioStream();
    virtual HRESULT closeAudioStream();

    bool addAudioProcessor( IAudioProcessor* sink,  ProcessorFormat& format );
    bool removeAudioProcessor( IAudioProcessor* sink );

    static void collectAudioCaptureDevices();
    static AudioCaptureDeviceArray audioCaptureDevices;
    static AudioInputStream* createAudioStream( LPCSTR capture_device,  unsigned sample_size=1024, 
                                           float scale_factor=0.0, float audio_sample_min=0.0009 );
    static void releaseAudioStream( AudioInputStream* audio_stream );

private:
    HRESULT releaseResources();

    void SetFormat(WAVEFORMATEX * pwfx );
    HRESULT CopyData( BYTE *pData, UINT32 numFramesAvailable );

    void applyWindowFunction( unsigned m_sample_size, float* sampleSet );
    void scaleSample( unsigned m_sample_size, float* sampleSet );

    HRESULT recordAudioStream();

    LPCSTR getEndpointDeviceName( LPCWSTR endpoint_id );
};

// If audio support is not wanted this audio stream does nothing

class NullAudioStream : public AudioInputStream
{
public:
    NullAudioStream() : AudioInputStream( NULL ) {}

    HRESULT openAudioStream() { return 0; }
    HRESULT closeAudioStream() { return 0; }
};

