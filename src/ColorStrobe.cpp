/*
Copyright (C) 2011-14 Robert DeSantis
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

#define FLASH_DELAY_MS  100         // Pause between flashes

void ColorStrobe::start(DWORD time_ms, UINT light_ms, UINT dark_ms, UINT flashes) {
    StrobeTime timing(light_ms, dark_ms);

    start(time_ms, timing, flashes);
}

void ColorStrobe::start(DWORD time_ms, const StrobeTime& timing, UINT flashes) {
    m_timing = timing;
    m_flashes = flashes;
    m_states.clear();

    int flash_ms = (m_timing.m_on_ms - ((m_flashes - 1)*FLASH_DELAY_MS)) / m_flashes;

    if ( flash_ms >= FLASH_DELAY_MS ) {
        for (int f = 0; f < m_flashes; f++) {
            if ( f > 0 )
                m_states.push_back(StrobeState(false, FLASH_DELAY_MS));
           
            m_states.push_back(StrobeState(true, flash_ms));
        }
    }
    else
        m_states.push_back(StrobeState(true, m_timing.m_on_ms));

    m_states.push_back(StrobeState(false, m_timing.m_off_ms));

    m_index = 0;
    m_strobe_on = false;
    m_strobe_next_ms = time_ms;
}

// Returns true if strobe state changed
bool ColorStrobe::strobe(DWORD time_ms) {
    if (time_ms <= m_strobe_next_ms)
        return false;

    m_strobe_on = m_states[m_index].m_on;
    m_strobe_next_ms = time_ms + m_states[m_index].m_time_ms;

    m_index = (m_index == m_states.size() - 1) ? 0 : m_index + 1;

    return true;
}