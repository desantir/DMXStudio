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
#include "IVisitor.h"
#include "BeatDetector.h"
#include "SoundSampler.h"
#include "SoundDetector.h"

enum SpeedAdjust {
    SPEED_NORMAL = 1,		        // Level does not affects timer MS
    SPEED_FAST = 2, 	            // Level reduces timer MS for faster animations
    SPEED_SLOW = 3             	    // Level increase timer MS for slower animations
} ;

class AnimationEngine;

#define MAXIMUM_LEVEL               255
#define PERCENT_OF_LEVEL(x)         ((x * MAXIMUM_LEVEL) / 100)
#define SCALE_BY_LEVEL(val, lvl)    ((unsigned)val * lvl / MAXIMUM_LEVEL)

enum SignalTrigger {
    TRIGGER_TIMER = 1,
    TRIGGER_FREQ_BEAT = 2,
    TRIGGER_AMPLITUDE_BEAT = 3,
    TRIGGER_RANDOM_TIMER = 4
};

enum SignalSource {
    SOURCE_HIGH = 1,                // Always returns high
    SOURCE_AMPLITUDE = 2,           // Current amplitude scaled 0-MAXIMUM_LEVEL
    SOURCE_AVG_AMPLITUDE = 3,       // Current average amplitude scaled 0-MAXIMUM_LEVEL
    SOURCE_BEAT_INTENSITY = 4,      // Amplituce beat intensity
    SOURCE_FREQ = 5,                // Frequency range max scaled 0-MAXIMUM_LEVEL
    SOURCE_VOLUME = 6,              // Current volume
    SOURCE_LOW = 7,                 // Always low
    SOURCE_SQUAREWAVE = 8,          // Squarewave of 'n' trigger periods
    SOURCE_SAWTOOTH = 9,            // Sawtooth of 'n' trigger periods
    SOURCE_SINEWAVE = 10,           // Sinewave
    SOURCE_TRIANGLEWAVE = 11,       // Triangle wave of 'n' trigger periods
    SOURCE_STEPWAVE = 12,           // Stepwave (half)
    SOURCE_STEPWAVE_FULL = 13,      // Stepwave (full)
    SOURCE_RANDOM = 14,             // Random level 0-MAXIMUM_LEVEL of 'n' trigger periods,
    SOURCE_SOUND = 15               // Current mute detection state
};

class AnimationSignal
{
    friend class VenueWriter;
    friend class VenueReader;

    SignalTrigger           m_trigger;                      // Triggers the start of a signal
    SignalSource            m_source;                       // Source of the signal level

    DWORD                   m_timer_ms;                     // Timer trigger rate
    DWORD                   m_off_ms;                       // Options off signal after trigger
    
    UINT                    m_level_floor;                  // Minimum level
    UINT                    m_level_ceil;                   // Maximum level
    bool                    m_level_invert;                 // Invert level result
    UINT                    m_level_scale;					// Level scaling (simple level * factor)
    UINT                    m_level_periods;                // Number of trigger periods for some level sources
    UINT                    m_level_hold;                   // Hold periods for some level sources 
    UINT                    m_freq_low_hz;                  // Frequency low (Hz)
    UINT                    m_freq_high_hz;                 // Frequency high (Hz)
    UINT                    m_sensitivity;                  // Amplitute beat sensitivity

    SpeedAdjust				m_speed_adjust;					// Apply level to timer speed

public:
    AnimationSignal( SignalTrigger trigger = TRIGGER_TIMER,
                     SignalSource source = SOURCE_HIGH,
                     DWORD timer_ms = 500,
                     DWORD  off_ms = 0,
                     UINT level_floor = 0,
                     UINT level_ceil = MAXIMUM_LEVEL,
                     bool level_invert = false,
                     UINT level_scale = 1,
                     UINT level_periods = 10,
                     UINT freq_low_hz = 0,
                     UINT freq_high_hz = 22000,
                     SpeedAdjust speed_modify = SPEED_NORMAL,
                     UINT level_hold = 0,
                     UINT sensitivity = 30 );

    ~AnimationSignal(void);

    inline SignalTrigger getTrigger() const {
        return m_trigger;
    }
    void setTrigger( SignalTrigger trigger ) {
        m_trigger = trigger;
    }

    inline SignalSource getSource() const {
        return m_source;
    }
    void setSource( SignalSource source ) {
        m_source = source;
    }

    inline DWORD getTimerMS() const {
        return m_timer_ms;
    }
    void setTimerMS( DWORD timer_ms ) {
        m_timer_ms = timer_ms;
    }

    inline DWORD getOffMS() const {
        return m_off_ms;
    }
    void setOffMS( DWORD off_ms ) {
        m_off_ms = off_ms;
    }

    inline UINT getLevelFloor() const {
        return m_level_floor;
    }
    void setLevelFloor( UINT floor ) {
        m_level_floor = floor;
    }

    inline UINT getLevelCeil() const {
        return m_level_ceil;
    }
    void setLevelCeil( UINT ceil ) {
        m_level_ceil = ceil;
    }

    inline bool isLevelInvert() const {
        return m_level_invert;
    }
    void setLevelInvert( bool invert ) {
        m_level_invert = invert;
    }

    inline UINT getLevelScale() const {
        return m_level_scale;
    }
    void setLevelScale( UINT level_scale ) {
        m_level_scale = level_scale;
    }

    inline UINT getLevelPeriods() const {
        return m_level_periods;
    }
    void setLevelPeriods( UINT periods ) {
        m_level_periods = periods;
    }

    inline UINT getFreqLowHz() const {
        return m_freq_low_hz;
    }
    void setFreqLowHz( UINT freq_low_hz ) {
        m_freq_low_hz = freq_low_hz;
    }

    inline UINT getFreqHighHz() const {
        return m_freq_high_hz;
    }
    void setFreqHighHz( UINT freq_high_hz ) {
        m_freq_high_hz = freq_high_hz;
    }

    inline UINT getLevelHold() const {
        return m_level_hold;
    }
    void setLevelHold( UINT level_hold ) {
        m_level_hold = level_hold;
    }

    inline SpeedAdjust getSpeedAdjust() const {
        return m_speed_adjust;
    }
    inline void setSpeedAdjust( SpeedAdjust speed_adjust ) {
        m_speed_adjust = speed_adjust;
    }

    inline UINT getSensitivity() const {
        return m_sensitivity;
    }
    inline void setSensitivity( UINT sensitivity ) {
        m_sensitivity = sensitivity;
    }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    CString AnimationSignal::getSynopsis() const;
};
