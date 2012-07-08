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
#include "AudioInputStream.h"
#include "FreqBin.h"

struct BeatEvent {
	CEvent*			m_beatEvent;			// Beat event trigger
	DWORD			m_mask[2];

	BeatEvent( CEvent* eventHandler, DWORD mask[] ) :
				m_beatEvent( eventHandler ) {

		m_mask[0] = mask[0];
		m_mask[1] = mask[1];
	}
};

typedef std::vector< BeatEvent > BeatEventArray;

class BeatDetector : public IAudioProcessor
{
	AudioInputStream* m_audio_stream;

	unsigned		m_samples_per_second;	// Samples per second
	unsigned		m_sample_size;			// Raw sample size N (pre FFT, post FFT = N/2)
	unsigned		m_sensitivity_ms;		// MS between beat detection (dampens the risidual effect of beats)
	unsigned		m_frequency_bins;		// Number of frequency bins (1 provides sound energy)

	// Simple sound energy detection 
	FreqBinArray	m_bins;
	BeatEventArray	m_event_handlers;

	BeatDetector(BeatDetector& other) {}
	BeatDetector& operator=(BeatDetector& rhs) { return *this; }

public:
	BeatDetector( unsigned frequency_bins=1, unsigned sensitivity=30 );
	~BeatDetector(void);

	void attach( AudioInputStream* stream );
	void detach();

	void addFrequencyEvent( CEvent* beatEvent, unsigned freq_low, unsigned freq_high );
	void removeFrequencyEvent( CEvent* eventHandler );

	void addFrequencyEvent( CEvent* beatEvent ) {
		addFrequencyEvent( beatEvent, 0, 22000 );
	}

	void AudioProcess (float input);

	HRESULT ProcessFFT( WORD channels, FFT_Result* fft_result[] ); 
	HRESULT ProcessAmplitudes( WORD channels, size_t sample_size, float* sample_data[] );

private:
	double* m_B;
	double* m_Es;

	void output( const char *fmt, ... ) {
		if ( studio.isDebug() ) {
			va_list args;
			va_start( args, fmt );
			vprintf( fmt, args );
			va_end( args );
		}
	}
};

