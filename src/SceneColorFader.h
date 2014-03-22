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
    FADER_EFFECT_CHANGE = 1,				// Simple color changer
    FADER_EFFECT_STROBE = 2,				// Strobing color
    FADER_EFFECT_BLEND = 3,					// Blending colors together
    FADER_EFFECT_ALL = 4                    // Use all effects
} FaderEffect;

struct FaderFixture
{
    Fixture*				m_pf;
    PixelArray*      	    m_pixels;               // Channel numbers for all colors
    RGBWA                   m_color;                // Initial values for all colors
    ColorFader				m_fader;

    FaderFixture( Fixture* pf, PixelArray* pixels, RGBWA color ) :
        m_pf( pf ),
        m_color( color ),
        m_pixels( pixels )
    {}
};

typedef std::vector< FaderFixture > FaderFixtureArray;

class SceneColorFader : public AbstractAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
    AnimationSignalProcessor*	m_signal_processor;
    FaderFixtureArray		    m_fixtures;
    FaderEffect					m_current_effect;
    UINT       					m_color_index;
    ColorStrobe					m_strobe;
    UINT                        m_effect_periods;
    RGBWAArray                  m_colors;
    bool                        m_start_strobe;

    // Configuration
    RGBWA   					m_strobe_neg_color;
    unsigned					m_strobe_neg_ms;
    unsigned					m_strobe_pos_ms;
    RGBWAArray                  m_custom_colors;
    FaderEffect                 m_fader_effect;

public:
    static const char* className;
    static const char* animationName;

    SceneColorFader( UID animation_uid, 
                        AnimationSignal signal,
                        UIDArray actors,
                        RGBWA strobe_neg_color,
                        unsigned strobe_pos_ms,
                        unsigned strobe_neg_ms,
                        RGBWAArray custom_colors,
                        FaderEffect fader_effect );

    SceneColorFader(void) :
        m_signal_processor( NULL )
    {}

    virtual ~SceneColorFader(void);

    AbstractAnimation* clone();
    CString getSynopsis(void);

    const char* getName() { return SceneColorFader::animationName; }
    const char* getClassName() { return SceneColorFader::className; }

    RGBWA getStrobeNegColor() const {
        return m_strobe_neg_color;
    }
    void setStrobeNegColor( RGBWA strobe_neg_color ) {
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

    RGBWAArray getCustomColors( ) const {
        return m_custom_colors;
    }
    void setCustomColors( RGBWAArray colors ) {
        m_custom_colors = colors;
    }

    FaderEffect getFaderEffect() const {
        return m_fader_effect;
    }
    void setFaderEffect( FaderEffect fader_mode ) {
        m_fader_effect = fader_mode;
    }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    virtual void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );
    virtual bool sliceAnimation( DWORD time_ms, BYTE* dmx_packet );
    virtual void stopAnimation( void );

protected:
    void loadColorChannels( BYTE* dmx_packet, FaderFixture& sfixture, RGBWA& rgbw );
};

