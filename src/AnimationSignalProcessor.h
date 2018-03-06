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

#pragma once

#include "stdafx.h"
#include "AnimationSignal.h"
#include "BeatDetector.h"
#include "SoundSampler.h"
#include "SoundDetector.h"
#include "LevelRecord.h"

#define LEVEL_HISTORY_SIZE 1000

class AnimationEngine;

enum SignalState {
    SIGNAL_NO_CHANGE,       // No change to level
    SIGNAL_NEW_LEVEL,       // New level triggered
    SIGNAL_LEVEL_OFF        // Signal off signaled
};

class AnimationSignalProcessor
{
    // State information
    AnimationSignal 		m_signal_def;

    DWORD					m_next_sample_ms;				// Time of next sample
    DWORD					m_next_off_ms;				    // Time of next off signal
    DWORD                   m_level_expire_ms;              // Time level hold expires
    DWORD                   m_next_report_ms;               // Time to add a report for accurate charting

    CEvent					m_trigger;						// Event trigger
    BeatDetector*			m_detector;
    SoundSampler*			m_sampler;
    AnimationEngine*		m_engine;

    UINT    				m_level;						// Current signal level
    UINT                    m_wave_value;                   // Wave form current value
    std::vector<UINT>       m_values;                       // Pre-computed wave values
    UINT                    m_amplitude;                    // Current signal amplitude

    SoundLevel              m_sound_level;                  // Current tick's sound level
    DWORD                   m_last_beat_index;              // Amplitude beat index

    LevelRecord             m_level_ring_buffer[LEVEL_HISTORY_SIZE];
    size_t                  m_ring_head;
    size_t                  m_ring_count;

    AnimationSignalProcessor( AnimationSignalProcessor& other );
    AnimationSignalProcessor& operator=( AnimationSignalProcessor& rhs );

public:
    AnimationSignalProcessor( AnimationSignal& signal_definition, AnimationEngine* engine );
    ~AnimationSignalProcessor();

    DWORD getNextSampleMS() const {
        return m_next_sample_ms;
    }

    inline AnimationSignal& signal() {
        return m_signal_def;
    }

    inline UINT getLevel() const {
        return m_level;
    }

    SignalState tick( DWORD time_ms );

    void restart( DWORD time_ms ) { 
        m_next_sample_ms = time_ms;
        if ( m_next_off_ms > 0 )
            m_next_off_ms = time_ms;
    }

    LevelRecordArray fetchLevelData( DWORD after_ms );

private:
    UINT level_adjust( unsigned level );
    DWORD speed_adjust( DWORD speed );
    void record_level( DWORD time_ms, UINT level );
    UINT peak_hold( DWORD time_ms, UINT level ); 
};
