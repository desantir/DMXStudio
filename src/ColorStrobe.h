/* 
Copyright (C) 2011-16 Robert DeSantis
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

#include "RGBWA.h"
#include "ColorFader.h"

enum StrobeState {
    SE_NONE,
    SE_START,                   // Internal restart sequence - not returned
    SE_OFF,
    SE_FADE_IN_START,
    SE_FADE_IN,
    SE_ON,
    SE_FADE_OUT_START,
    SE_FADE_OUT
};

class ColorStrobe
{
    struct StrobeStep {
        StrobeState     m_event;
        UINT            m_time_ms;

        StrobeStep( StrobeState event, UINT time_ms ) :
            m_event( event ),
            m_time_ms( time_ms )
        {}
    };

    RGBWA			m_strobe_positive;
    RGBWA			m_strobe_negative;

    StrobeTime      m_timing;

    DWORD			m_strobe_next_ms;
    StrobeState 	m_current_state;
    UINT            m_index;
    std::vector<StrobeStep> m_states;
    ColorFader      m_fader;
    bool            m_auto_fade;
    RGBWA           m_strobe_color;
    RGBWAArray      m_positive_colors;
    size_t          m_positive_index;

public:
    ColorStrobe( void ) :
        m_strobe_next_ms( 0 ),
        m_current_state( SE_OFF ),
        m_index( 0 ),
        m_positive_index( 0 )
    {}

    ~ColorStrobe(void) {}

    inline bool isOn() const { 
        return m_current_state == StrobeState::SE_ON ||
            m_current_state == StrobeState::SE_FADE_IN_START ||
            m_current_state == StrobeState::SE_FADE_IN ||
            m_current_state == StrobeState::SE_FADE_OUT_START ||
            m_current_state == StrobeState::SE_FADE_OUT;
    }

    RGBWA rgbwa();
    
    // For simple strobe, return strobe state change
    inline bool check_strobe( DWORD time_ms ) {
        return strobe( time_ms ) != SE_NONE;
    }
    
    inline void setPositive( const RGBWA& rgbwa ) {
        m_strobe_positive = rgbwa;
        m_positive_index = 0;

        RGBWA::resolveColor( rgbwa, m_positive_colors );
    }
    inline RGBWA getPositive() const {
        return m_strobe_positive;
    }

    inline void setNegative( const RGBWA& rgbwa ) {
        m_strobe_negative = rgbwa;
    }
    inline RGBWA getNegative( ) const {
        return m_strobe_negative;
    }

    void start( DWORD time_ms, const StrobeTime& timing, bool auto_fade=false );
    StrobeState strobe( DWORD time_ms );
};