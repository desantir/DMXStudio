/* 
Copyright (C) 2016 Robert DeSantis
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
#include "AnimationDefinition.h"
#include "AnimationSignalProcessor.h"

enum PulseEffect {
    PULSE_EFFECT_STROBE = 1,
    PULSE_EFFECT_BREATH = 2
} ;

class ScenePulse : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
    // Configuration
    RGBWA   					m_pulse_color;                      // Color used on pulse
    unsigned					m_pulse_ms;
    bool                        m_select_random;                    // Pulse random fixture or in order
    unsigned                    m_pulse_fixture_count;              // Number of fixtures / pulse
    PulseEffect                 m_pulse_effect;

public:
    static const char* className;
    static const char* animationName;

    ScenePulse( UID animation_uid, bool shared, UID reference_fixture, 
        AnimationSignal signal,
        RGBWA pulse_color,
        unsigned pulse_ms,
        unsigned pulse_fixture_count,
        bool select_random,
        PulseEffect pulse_effect );

    ScenePulse( void ) {}

    virtual ~ScenePulse(void);

    CString getSynopsis(void);

    const char* getPrettyName() { return ScenePulse::animationName; }
    const char* getClassName() { return ScenePulse::className; }

    inline PulseEffect getPulseEffect() const {
        return m_pulse_effect;
    }
    inline void setPulseEffect( PulseEffect effect ) {
        m_pulse_effect = effect;
    }

    inline RGBWA getPulseColor() const {
        return m_pulse_color;
    }
    inline void setPulseColor( RGBWA& pulse_color ) {
        m_pulse_color = pulse_color;
    }

    inline unsigned getPulseMS() const {
        return m_pulse_ms;
    }
    inline void setPulseMS( unsigned pulse_ms ) {
        m_pulse_ms = pulse_ms;
    }

    inline unsigned getPulseFixtureCount() const {
        return m_pulse_fixture_count;
    }
    inline void setPulseFixtureCount( unsigned pulse_fixture_coun ) {
        m_pulse_fixture_count = pulse_fixture_coun;
    }

    inline bool isSelectRandom() const {
        return m_select_random;
    }
    inline void setSelectRandom( bool select_random ) {
        m_select_random = select_random; 
    }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();
};

