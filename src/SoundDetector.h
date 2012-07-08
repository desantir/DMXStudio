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

class SoundDetector : public IAudioProcessor
{
	AudioInputStream* m_audio_stream;
	DWORD			m_mute_ms;				// Length in MS of no amplitude before silence (zero = OFF)

	unsigned		m_amplitude;			// Current sound amplitude
	DWORD			m_last_sound_ms;		// Last time sounds was detected
	DWORD			m_last_quiet_ms;		// Last time silence was detected

	// Buffer and values to compute moving average
	unsigned		m_amplitude_buffer_size;
	unsigned*		m_amplitude_buffer;
	unsigned		m_ampbuf_index;
	unsigned		m_ampbuf_sum;
	unsigned		m_ampbuf_count;

	SoundDetector(SoundDetector& other) {}
	SoundDetector& operator=(SoundDetector& rhs) { return *this; }

public:
	static unsigned	maxAmplitude;

	SoundDetector( DWORD mute_ms=500 );
	~SoundDetector(void);

	void attach( AudioInputStream* stream );
	void detach();

	HRESULT ProcessFFT( WORD channels, FFT_Result* fft_result[] ) { return 0; } 
	HRESULT ProcessAmplitudes( WORD channels, size_t sample_size, float* sample_data[] );

	unsigned getAmplitude( ) const {
		return m_amplitude;
	}

	unsigned getAvgAmplitude( ) const {
		return m_ampbuf_sum/m_ampbuf_count;
	}

	bool isMute() const {
		if ( m_mute_ms == 0 )				// Zero MS delay means mute detection is OFF
			return false;

		DWORD time = GetTickCount();
		return (time-m_last_sound_ms) > m_mute_ms;
	}

	void setMuteMS( DWORD mute_ms ) {
		m_mute_ms = mute_ms;
	}
	DWORD getMuteMS( ) const {
		return m_mute_ms;
	}
};
