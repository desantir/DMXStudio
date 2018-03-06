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

#include "AnimationSignal.h"

static LPCSTR triggerNames[] = { "None", "Timer", "Frequency Beat", "Amplitude Beat", "Random Timer" };
static LPCSTR sourceNames[] = { "None", "Always High", "Amplitude", "Avg. Amplitude", "Amplitude Beat Intensity", "Frequency", "Volume", "Squarewave", "Sawtooth", "Sinewave", "Triangle", "Random", "Always Low" };

// ----------------------------------------------------------------------------
//
AnimationSignal::AnimationSignal( SignalTrigger trigger, 
                                  SignalSource source,
                                  DWORD timer_ms,
                                  DWORD off_ms,
                                  UINT level_floor,
                                  UINT level_ceil,
                                  bool level_invert,
                                  UINT level_scale,
                                  UINT level_periods,
                                  UINT freq_low_hz,
                                  UINT freq_high_hz,
                                  SpeedAdjust speed_adjust,
                                  UINT level_hold,
                                  UINT sensitivity ) :
        m_trigger( trigger ),
        m_source( source ),
        m_timer_ms( timer_ms ),
        m_off_ms( off_ms ),
        m_level_floor( level_floor ),
        m_level_ceil( level_ceil ),
        m_level_invert( level_invert ),
        m_level_scale( level_scale ),
        m_level_periods( level_periods ),
        m_freq_low_hz( freq_low_hz ),
        m_freq_high_hz( freq_high_hz ),
        m_speed_adjust( speed_adjust ),
        m_level_hold( level_hold ),
        m_sensitivity( sensitivity )
{
}

// ----------------------------------------------------------------------------
//
AnimationSignal::~AnimationSignal(void)
{
}

// ----------------------------------------------------------------------------
//
CString AnimationSignal::getSynopsis() const {
    CString synopsis( triggerNames[m_trigger ] );
    CString speed_adjust;

    switch ( m_speed_adjust ) {
        case SPEED_NORMAL:
            break;

        case SPEED_SLOW:
            speed_adjust.AppendFormat( " level reduces" );
            break;

        case SPEED_FAST:
            speed_adjust.AppendFormat( " level increases" );
            break;
    }

    switch ( m_trigger ) {
        case TRIGGER_TIMER:
            synopsis.AppendFormat( "=%dms%s", m_timer_ms, speed_adjust );
            break;

        case TRIGGER_RANDOM_TIMER:
            synopsis.AppendFormat( "=0-%dms%s", m_timer_ms, speed_adjust );
            break;
        
        case TRIGGER_FREQ_BEAT:
            synopsis.AppendFormat( "=%d-%d Hz", m_freq_low_hz, m_freq_high_hz );
            break;

        case TRIGGER_AMPLITUDE_BEAT:
            synopsis.AppendFormat( " sensitivity=%u", m_sensitivity );
            break;
    }

    if ( m_off_ms > 0 ) 
        synopsis.AppendFormat( " off=%dms", m_off_ms );

    synopsis.AppendFormat( " source=%s", sourceNames[m_source] );

    switch ( m_source ) {
        case SOURCE_AMPLITUDE:
        case SOURCE_AVG_AMPLITUDE:
        case SOURCE_FREQ:
        case SOURCE_BEAT_INTENSITY:
            synopsis.AppendFormat( " hold periods=%d", m_level_hold );
            break;

        case SOURCE_HIGH:
        case SOURCE_LOW:
        case SOURCE_VOLUME:
        case SOURCE_SOUND:
            break;

        case SOURCE_SQUAREWAVE:
        case SOURCE_SAWTOOTH:
        case SOURCE_SINEWAVE:
        case SOURCE_TRIANGLEWAVE:
        case SOURCE_RANDOM:
            synopsis.AppendFormat( " periods=%d hold periods=%d", m_level_periods, m_level_hold );
            break;
    }
    
    if ( !m_level_invert )
        synopsis.AppendFormat( " level=%d-%d scale=%d", m_level_floor, m_level_ceil, m_level_scale );
    else
        synopsis.AppendFormat( " level=%d-%d scale=%d", m_level_ceil, m_level_floor, m_level_scale );

    return synopsis;
}
