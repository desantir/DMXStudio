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

#include "Fixture.h"
#include "AbstractAnimation.h"
#include "ColorFader.h"
#include "ColorStrobe.h"

typedef enum {
	COLOR_EFFECT_CHANGE = 0,				// Simple color changer
	COLOR_EFFECT_STROBE = 1,				// Strobing color
	COLOR_EFFECT_BLEND = 2,					// Blending colors together
	COLOR_EFFECT_COUNT						// Must be last
} ColorEffect;

typedef std::map<UINT, CString> ColorNames;

struct SwitcherFixture
{
	Fixture*				m_pf;
	channel_t				m_channels[COLOR_CHANNELS];             // Channel numbers for all colors
    BYTE                    m_channel_values[COLOR_CHANNELS];       // Actors initial values for all colors
	ColorFader				m_fader;

	SwitcherFixture( Fixture* pf, channel_t channels[], BYTE channel_values[] ) :
		m_pf( pf )
    {
        memcpy( m_channels, channels, sizeof(m_channels) );
		memcpy( m_channel_values, channel_values, sizeof(m_channel_values) );
	}
};

typedef std::vector< SwitcherFixture > SwitcherFixtureArray;
typedef std::vector<UINT> ColorProgression;

class SceneColorSwitcher : public AbstractAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
	AnimationSignalProcessor*	m_signal_processor;
	SwitcherFixtureArray		m_fixtures;
	ColorEffect					m_color_effect;
	UINT       					m_color_index;
	ColorStrobe					m_strobe;
    UINT                        m_custom_index;
    UINT                        m_strobe_periods;

	// Configuration
	unsigned					m_strobe_neg_color;
	unsigned					m_strobe_neg_ms;
	unsigned					m_strobe_pos_ms;
    ColorProgression            m_custom_colors;

public:
	static const char* className;

    static ColorNames color_names;

	SceneColorSwitcher( UID animation_uid, 
						AnimationSignal signal,
						UIDArray actors,
						unsigned strobe_neg_color,
						unsigned strobe_pos_ms,
						unsigned strobe_neg_ms,
                        ColorProgression custom_colors );

	SceneColorSwitcher(void) :
		m_signal_processor( NULL )
	{}

	virtual ~SceneColorSwitcher(void);

	AbstractAnimation* clone();
	CString getSynopsis(void);

	const char* getName() { return "Scene Color Switcher"; }
	const char* getClassName() { return SceneColorSwitcher::className; }

	unsigned getStrobeNegColor() const {
		return m_strobe_neg_color;
	}
	void setStrobeNegColor( unsigned strobe_neg_color ) {
		m_strobe_neg_color = strobe_neg_color;
	}

	unsigned getStrobeNegMS() const {
		return m_strobe_neg_ms;
	}
	void setStrobeNegMS( unsigned strobe_neg_ms ) {
		m_strobe_neg_ms = strobe_neg_ms;
	}

	unsigned getStrobePosMS() const {
		return m_strobe_pos_ms;
	}
	void setStrobePosMS( unsigned strobe_pos_ms ) {
		m_strobe_pos_ms = strobe_pos_ms;
	}

    ColorProgression getCustomColors( ) const {
        return m_custom_colors;
    }
    void setCustomColors( ColorProgression colors ) {
        m_custom_colors = colors;
    }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

	virtual void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );
	virtual bool sliceAnimation( DWORD time_ms, BYTE* dmx_packet );
	virtual void stopAnimation( void );

protected:
	void loadColorChannels( BYTE* dmx_packet, SwitcherFixture& sfixture, const BYTE *rgbw );
};

