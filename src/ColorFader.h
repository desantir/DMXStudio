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
	BYTE		m_current_color[COLOR_CHANNELS];			// Current RGBWA
	BYTE		m_target_color[COLOR_CHANNELS];				// Blend RGBWA targets
	int			m_blend_delta[COLOR_CHANNELS];				// Channel value blending deltas
	DWORD		m_blend_next_ms[COLOR_CHANNELS];

public:
	ColorFader( void ) {
		for ( int i=0; i < COLOR_CHANNELS; i++ )
			m_current_color[i] = m_target_color[i] = 0;
	}

	~ColorFader(void) {}

	inline void setTargets( const BYTE rgbw[COLOR_CHANNELS] ) {
		for ( int i=0; i < COLOR_CHANNELS; i++ )
			m_target_color[i] = rgbw[i];
	}

	inline void setCurrent( const BYTE rgbw[COLOR_CHANNELS] ) {
		for ( int i=0; i < COLOR_CHANNELS; i++ )
			m_current_color[i] = rgbw[i];
	}

	inline BYTE red() const { return m_current_color[0]; }
	inline BYTE green() const { return m_current_color[1]; }
	inline BYTE blue() const { return m_current_color[2]; }
	inline BYTE white() const { return m_current_color[3]; }
	inline BYTE amber() const { return m_current_color[4]; }

	inline BYTE* rgbwa() { return m_current_color; }

	void start( DWORD time_ms, int fade_time ) {
		for ( int i=0; i < COLOR_CHANNELS; i++ ) {
			int delta = m_target_color[i]-m_current_color[i];
			if ( delta != 0 ) {
				m_blend_delta[i] = fade_time / delta;
				m_blend_next_ms[i] = time_ms;
			}
			else {
				m_blend_delta[i] = 0;
			}
		}
	}

	bool fade( DWORD time_ms ) {
		bool changed = false;

		for ( int i=0; i < COLOR_CHANNELS; i++ ) {
			if ( m_target_color[i] == m_current_color[i] )
				continue;

			while ( m_blend_next_ms[i] < time_ms && m_blend_delta[i] != 0 ) {
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

		return changed;
	}
};

