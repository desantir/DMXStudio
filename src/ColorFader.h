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

class ColorFader
{
    bool        m_tick_mode;                                // Time is sequenial ticks (i.e. 1,2,3,4,...)
    RGBWA		m_current_color;			                // Current RGBWA
    RGBWA		m_target_color;				                // Blend RGBWA targets
    int			m_blend_delta[COLOR_CHANNELS];				// Channel value blending deltas
    DWORD		m_blend_next_ms[COLOR_CHANNELS];
    DWORD       m_current_time;                             // Current fader time

public:
    ColorFader( bool tick_mode=false ) :
        m_tick_mode( tick_mode )
    {}

    ~ColorFader(void) {}

    inline void setCurrent( const RGBWA& rgbw ) {
        m_current_color = rgbw;
    }

    inline RGBWA rgbwa() { return m_current_color; }

    void start( DWORD current_time, int fade_time, const RGBWA& rgbw ) {
        m_target_color = rgbw;
        m_current_time = current_time;

        for ( int i=0; i < COLOR_CHANNELS; i++ ) {
            int delta = (int)m_target_color[i]-(int)m_current_color[i];
            if ( delta != 0 ) {
                if ( m_tick_mode ) {
                    m_blend_delta[i] = (fade_time != 0 ) ? delta/fade_time : 255;
                    m_blend_next_ms[i] = 0;
                }
                else {
                    m_blend_delta[i] = fade_time/delta;
                    m_blend_next_ms[i] = current_time;
                } 
            }
            else {
                m_blend_delta[i] = m_blend_next_ms[i] = 0;
            }
        }
    }

    // Fade time is divided into a specific number of increments
    inline bool tick( ) {
        bool changed = false;

        for ( int i=0; i < COLOR_CHANNELS; i++ ) {
            if ( m_target_color[i] == m_current_color[i] || m_blend_delta[i] == 0 )
                continue;

            if ( m_blend_delta[i] > 0 && m_current_color[i] < m_target_color[i] ) {
                m_current_color[i] = 
                    std::min<int>( m_current_color[i] + m_blend_delta[i], m_target_color[i] );
                changed = true;
            }
            else if ( m_blend_delta[i] < 0 && m_current_color[i] > m_target_color[i] ) {
                m_current_color[i] = 
                    std::max<int>( (int)m_current_color[i] + m_blend_delta[i], m_target_color[i] );
                changed = true;
            }
        }

        m_current_time++;

        return changed;
    }

    // Fade time is based on the clock
    bool fade( DWORD time_ms ) {
        bool changed = false;

        for ( int i=0; i < COLOR_CHANNELS; i++ ) {
            if ( m_target_color[i] == m_current_color[i] || m_blend_delta[i] == 0 )
                continue;

            while ( m_blend_next_ms[i] < time_ms ) {
                if ( m_blend_delta[i] > 0 ) {
                    if ( m_current_color[i] < 255 ) {
                        m_current_color[i]++;
                        changed = true;
                    }
                    m_blend_next_ms[i] += m_blend_delta[i];
                }
                else {
                    if ( m_current_color[i] > 0 ) {
                        m_current_color[i]--;
                        changed = true;
                    }
                    m_blend_next_ms[i] += -m_blend_delta[i];
                }
            }
        }

        m_current_time = time_ms;

        return changed;
    }
};

