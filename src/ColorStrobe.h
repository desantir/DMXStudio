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

struct StrobeTime {
    UINT m_on_ms;
    UINT m_off_ms;

    StrobeTime( UINT on_ms=0, UINT off_ms=0 ) :
        m_on_ms( on_ms ),
        m_off_ms( off_ms )
    {}
};

struct StrobeState {
    bool    m_on;
    UINT    m_time_ms;

    StrobeState( bool on, UINT time_ms ) :
        m_on( on ),
        m_time_ms( time_ms )
    {}
};

class ColorStrobe
{
    RGBWA			m_strobe_positive;
    RGBWA			m_strobe_negative;

    StrobeTime      m_timing;

    DWORD			m_strobe_next_ms;
    bool			m_strobe_on;

    UINT            m_flashes;

    UINT            m_index;
    std::vector<StrobeState> m_states;

public:
    ColorStrobe( void ) {}
    ~ColorStrobe(void) {}

    inline bool isOn() const { return m_strobe_on; }

    inline RGBWA rgbwa() { return m_strobe_on ? m_strobe_positive : m_strobe_negative; }

    inline void setPositive( const RGBWA& rgbwa ) {
        m_strobe_positive = rgbwa;
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

    void start( DWORD time_ms, UINT light_ms, UINT dark_ms, UINT flashes );
    void start( DWORD time_ms, const StrobeTime& timing, UINT flashes );
    bool strobe( DWORD time_ms );
};