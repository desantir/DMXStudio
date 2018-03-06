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

#include "AnimationDefinition.h"
#include "AnimationSignalProcessor.h"
#include "ColorFader.h"
#include "ColorStrobe.h"

enum FaderEffect {
    FADER_EFFECT_CHANGE = 1,				// Simple color changer (single)
	FADER_EFFECT_CHANGE_MULTI = 2,			// Simple color changer (distributes palette across multiple fixtures)
    FADER_EFFECT_STROBE = 3,				// Strobing color
    FADER_EFFECT_BLEND = 4,					// Blending colors together (single)
	FADER_EFFECT_BLEND_MULTI = 5,			// Blending colors together (distributes palette across multiple fixtures)
    FADER_EFFECT_ALL = 6                    // Use all effects

} ;

class SceneColorFader : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
    // Configuration
    RGBWA   					m_strobe_neg_color;
	StrobeTime					m_strobe_time;
    RGBWAArray                  m_custom_colors;
    FaderEffect                 m_fader_effect;

public:
    static const char* className;
    static const char* animationName;

    SceneColorFader( UID animation_uid, 
                     bool shared,
					 UID reference_fixture,
                     AnimationSignal signal, 
					 StrobeTime strobeTime,
                     RGBWA strobe_neg_color,
                     RGBWAArray custom_colors,
                     FaderEffect fader_effect );

    SceneColorFader( void ) {}

    virtual ~SceneColorFader(void);

    CString getSynopsis(void);

    const char* getPrettyName() { return SceneColorFader::animationName; }
    const char* getClassName() { return SceneColorFader::className; }

    RGBWA getStrobeNegColor() const {
        return m_strobe_neg_color;
    }
    void setStrobeNegColor( RGBWA strobe_neg_color ) {
        m_strobe_neg_color = strobe_neg_color;
    }

	StrobeTime getStrobeTime() const {
		return m_strobe_time;
	}
	void setStrobeTime(StrobeTime& strobe_time) {
		m_strobe_time = strobe_time;
	}

    RGBWAArray& getCustomColors( ) {
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

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();
};

