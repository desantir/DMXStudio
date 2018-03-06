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

#include "DMXStudio.h"

#define FLASH_DELAY_MS  50         // Pause between flashes

void ColorStrobe::start(DWORD time_ms, const StrobeTime& timing, bool auto_fade ) {
    m_timing = timing;
    m_auto_fade = auto_fade;

    // Make sure fades aren't longer than what they are fading to
    m_timing.setFadeInMS( std::min<UINT>( m_timing.getFadeInMS(), m_timing.getOnMS()) );
    m_timing.setFadeOutMS( std::min<UINT>( m_timing.getFadeOutMS(), m_timing.getOffMS()) );

    // Fade time eats on/off time
    m_timing.setOnMS( m_timing.getOnMS() - m_timing.getFadeInMS() );
    m_timing.setOffMS(m_timing.getOffMS() - m_timing.getFadeOutMS() );

    m_states.clear();
    
    m_states.emplace_back( StrobeState::SE_START, 0 );
    m_states.emplace_back( StrobeState::SE_OFF, m_timing.getOffMS() );

    if ( m_timing.getFadeInMS() > 0 ) {
        m_states.emplace_back( StrobeState::SE_FADE_IN_START, 1 );
        m_states.emplace_back( StrobeState::SE_FADE_IN, m_timing.getFadeInMS());
    }

    int flash_ms = (m_timing.getOnMS() - ((m_timing.getFlashes() - 1)*FLASH_DELAY_MS)) / m_timing.getFlashes();

    if ( flash_ms >= FLASH_DELAY_MS ) {
        for ( UINT f = 0; f < m_timing.getFlashes(); f++) {
            if ( f > 0 )
                m_states.emplace_back( StrobeState::SE_OFF, FLASH_DELAY_MS );

            m_states.emplace_back( StrobeState::SE_ON, flash_ms );
        }
    }
    else
        m_states.emplace_back( StrobeState::SE_ON, m_timing.getOnMS() );

    if ( m_timing.getFadeOutMS() > 0 ) {
        m_states.emplace_back( StrobeState::SE_FADE_OUT_START, 1 );
        m_states.emplace_back( StrobeState::SE_FADE_OUT, m_timing.getFadeOutMS());
    }

    m_index = 0;
    m_current_state = StrobeState::SE_OFF;
    m_strobe_next_ms = time_ms;
}

StrobeState ColorStrobe::strobe( DWORD time_ms ) {
    if ( time_ms <= m_strobe_next_ms ) {
        // If fading in/out, continuously return the event - else none
        if ( m_current_state == SE_FADE_IN || m_current_state == SE_FADE_OUT ) {
            if ( m_auto_fade && !m_fader.fade( time_ms ) )
                return SE_NONE;

            return m_current_state;
        }
        return SE_NONE;
    }

    m_current_state = m_states[m_index].m_event;

    if ( m_current_state == StrobeState::SE_START ) {
        if ( m_positive_colors.size() > 0 ) {
            m_strobe_color = m_positive_colors[ m_positive_index ];
            m_positive_index = (m_positive_index+1) % m_positive_colors.size();
        }

        m_current_state = m_states[++m_index].m_event;
    }

    m_strobe_next_ms = time_ms + m_states[m_index].m_time_ms;
    m_index = (m_index == m_states.size() - 1) ? 0 : m_index + 1;

    if ( m_auto_fade ) {
        switch ( m_current_state ) {
            case StrobeState::SE_FADE_IN_START:
                m_fader.setCurrent( getNegative() );
                m_fader.start( time_ms, m_timing.getFadeInMS(), m_strobe_color );
                break;

            case StrobeState::SE_FADE_OUT_START:
                m_fader.setCurrent( m_strobe_color );
                m_fader.start( time_ms, m_timing.getFadeOutMS(), getNegative() );
                break;
        }
    }

    return m_current_state;
}

// For simple strobe, return current color
RGBWA ColorStrobe::rgbwa() {
    switch ( m_current_state ) {
        case StrobeState::SE_FADE_IN_START:
        case StrobeState::SE_FADE_IN:
        case StrobeState::SE_FADE_OUT_START:
        case StrobeState::SE_FADE_OUT:
            if ( m_auto_fade )
                return m_fader.rgbwa();

        case StrobeState::SE_ON:
            return m_strobe_color;

        case StrobeState::SE_OFF:
        default:
            return m_strobe_negative;
    }
}