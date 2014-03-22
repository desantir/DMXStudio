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

class ColorStrobe
{
    RGBWA			m_current_color;			// Current RGBWA
    RGBWA			m_target_color;			    // RGBWA targets
    RGBWA			m_strobe_negative;

    StrobeTime      m_timing;

    DWORD			m_strobe_next_ms;
    bool			m_strobe_on;

public:
    ColorStrobe( void ) {}
    ~ColorStrobe(void) {}

    inline bool isOn() const { return m_strobe_on; }

    inline RGBWA rgbwa() { return m_current_color; }

    inline void setColor( const RGBWA& rgbwa ) {
        m_target_color = rgbwa;
    }

    inline void setNegative( const RGBWA& rgbwa ) {
        m_strobe_negative = rgbwa;
    }

    void start( DWORD time_ms, UINT light_ms, UINT dark_ms ) {
        m_timing.m_on_ms = light_ms;
        m_timing.m_off_ms = dark_ms;
        m_strobe_on = true;
        m_strobe_next_ms = time_ms;		
    }

    void start( DWORD time_ms, const StrobeTime& timing ) {
        m_timing = timing;
        m_strobe_on = true;
        m_strobe_next_ms = time_ms;		
    }

    // Returns true if strobe state changed
    bool strobe( DWORD time_ms ) {
        if ( time_ms <= m_strobe_next_ms )
            return false;

        m_current_color = ( m_strobe_on ) ? m_target_color : m_strobe_negative;

        m_strobe_on = !m_strobe_on;

        if ( m_strobe_on )
            m_strobe_next_ms = time_ms + m_timing.m_on_ms;
        else
            m_strobe_next_ms = time_ms + m_timing.m_off_ms;

        return true;
    }
};