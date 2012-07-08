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

#include "IVisitor.h"
#include "DMXStudio.h"
#include "BeatDetector.h"
#include "SoundSampler.h"
#include "SoundDetector.h"

typedef enum {
    APPLY_CHANNEL = 1,		        // Input level affects channel values
    APPLY_SPEED = 2,	            // Input level affects sample speed
    APPLY_SPEED_AND_CHANNEL = 3,   	// Input level affects all
    APPLY_INVERSE_SPEED = 4         // Input level inverse sample speed
} ApplySignal;

class AnimationTask;

typedef enum {
	CAI_TIMER_ONLY = 1,
	CAI_SOUND_LEVEL = 2,
	CAI_AVG_SOUND_LEVEL = 3,
	CAI_FREQ_BEAT = 4,
	CAI_FREQ_LEVEL = 5,
	CAI_RANDOM = 6
} AnimationSignalInput;

class AnimationSignal
{
    friend class VenueWriter;
    friend class VenueReader;

	DWORD					m_sample_rate_ms;				// Sample rate
	AnimationSignalInput	m_input_type;
	unsigned				m_input_low;
	unsigned				m_input_high;
	DWORD					m_sample_decay_ms;				// Length of decay for previous values
	unsigned				m_scale_factor;					// Level scaling curve
	unsigned				m_max_threshold;				// Lower level of maximum signal (n >= full_threshold = 100)
	ApplySignal				m_apply_to;						// Apply signal to value and/or time

public:
	AnimationSignal( DWORD sample_rate_ms = 200, 
					 AnimationSignalInput input_type = CAI_TIMER_ONLY, 
					 DWORD sample_decay = 0,
					 unsigned input_low = 0,
					 unsigned input_high = 0,
					 unsigned scale_factor = 1,
					 unsigned max_threshold = 100,
					 ApplySignal apply_to = APPLY_CHANNEL );

	~AnimationSignal(void);

	static const unsigned MAXIMUM_LEVEL;

	inline DWORD getSampleDecayMS() const {
		return m_sample_decay_ms;
	}
	void setSampleDecayMS( DWORD decay_ms ) {
		m_sample_decay_ms = decay_ms;
	}

	inline DWORD getSampleRateMS() const {
		return m_sample_rate_ms;
	}
	void setSampleRateMS( DWORD sample_rate_ms ) {
		m_sample_rate_ms = sample_rate_ms;
	}

	inline AnimationSignalInput getInputType() const {
		return m_input_type;
	}
	void setInputType( AnimationSignalInput type ) {
		m_input_type = type;
	}

	inline unsigned getInputLow() const {
		return m_input_low;
	}
	void setInputLow( unsigned low ) {
		m_input_low = low;
	}

	inline unsigned getInputHigh() const {
		return m_input_high;
	}
	void setInputHigh( unsigned high ) {
		m_input_high = high;
	}

	inline unsigned getScaleFactor( ) const {
		return m_scale_factor;
	}
	inline void setScaleFactor( unsigned scale_factor ) {
		m_scale_factor = scale_factor;
	}

	inline unsigned getMaxThreshold( ) const {
		return m_max_threshold;
	}
	inline void setMaxThreshold( unsigned max_threshold ) {
		m_max_threshold = max_threshold;
	}

	inline ApplySignal getApplyTo() const {
		return m_apply_to;
	}
	inline void setApplyTo( ApplySignal apply_to ) {
		m_apply_to = apply_to;
	}

	bool isApplyToSpeed( ) const {
		return m_apply_to == APPLY_SPEED;
	}

	bool isApplyToChannel( ) const {
		return m_apply_to == APPLY_CHANNEL;
	}

    bool isApplyInverseSpeed( ) const {
		return m_apply_to == APPLY_INVERSE_SPEED;
	}

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    CString AnimationSignal::getSynopsis() const;
};

class AnimationSignalProcessor
{	
	// State information

	AnimationSignal&		m_signal_def;

	DWORD					m_next_sample_ms;				// Time of next sample
	CEvent					m_trigger;						// Event trigger
	BeatDetector*			m_detector;
	SoundSampler*			m_sampler;
	bool					m_beat;							// In beat
	DWORD					m_decay_ms;						// Decay time
	AnimationTask*			m_animation_task;
	double					m_scale_max;					// Precalculated max for scaling

	unsigned				m_level;						// Current signal level
	bool					m_decay_latch;					// Is decay

	AnimationSignalProcessor( AnimationSignalProcessor& other );
	AnimationSignalProcessor& operator=( AnimationSignalProcessor& rhs );

public:
	AnimationSignalProcessor( AnimationSignal& signal_definition, AnimationTask* task );
	~AnimationSignalProcessor();

	bool isDecay( ) {
		bool decay = m_decay_latch;
		m_decay_latch = false;
		return decay;
	}

	bool isBeat() const { // CAUTION: Non-beat input is "always" beating
		return m_signal_def.getInputType() != CAI_FREQ_BEAT || m_beat;
	}

	DWORD getNextSampleMS() const {
		return m_next_sample_ms;
	}

	unsigned getLevel();
	bool tick( DWORD time_ms );

private:
	inline unsigned level_adjust( unsigned level ) {
		if ( m_signal_def.getScaleFactor() == 1 )
			return level;

		double result = pow( (double)level, m_signal_def.getScaleFactor() );
		level =  (unsigned)(result / m_scale_max * 100.0);

		if ( level >= m_signal_def.getMaxThreshold() )
			return AnimationSignal::MAXIMUM_LEVEL;

		return level;
	}
};
