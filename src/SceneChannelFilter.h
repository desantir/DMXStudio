/* 
Copyright (C) 2014 Robert DeSantis
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

#include "IVisitor.h"
#include "SceneChannelAnimator.h"

enum ChannelFilter {
    CF_SINE_WAVE = 1,					// Sine wave (amplitude, angle step)
    CF_RAMP_UP = 2,					    // Ramp up (step)
    CF_RAMP_DOWN = 3,					// Ramp down (step)
    CF_STEP_WAVE = 4,				    // Step wave (step)
    CF_RANDOM = 5                       // Random value (amplitude)
} ;

class SceneChannelFilter : public SceneChannelAnimator
{
    friend class VenueWriter;
    friend class VenueReader;

    // Configuration
    ChannelFilter       m_filter;
    ChannelList         m_channels;
    BYTE                m_step;
    BYTE                m_amplitude;
    int                 m_offset;

public:
    static const char* className;
    static const char* animationName;

    SceneChannelFilter() :
        m_filter(CF_SINE_WAVE),
        m_channels(0),
        m_step(1),
        m_amplitude(5),
        m_offset(0)
    {}

    SceneChannelFilter( UID animation_uid, 
                        AnimationSignal signal,
                        UIDArray actors,
                        ChannelFilter filter,
                        ChannelList channels,
                        BYTE step,
                        BYTE amplitude,
                        int offset
                       );

    ~SceneChannelFilter(void);

    AbstractAnimation* clone();

    const char* getName() { return SceneChannelFilter::animationName; }
    const char* getClassName() { return SceneChannelFilter::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );

    virtual CString getSynopsis(void);

    inline ChannelFilter getFilter() const {
        return m_filter;
    }
    inline void setFilter( ChannelFilter filter ) {
        m_filter = filter;
    }

    inline ChannelList getChannels() const {
        return m_channels;
    }
    inline void setChannels( ChannelList channels ) {
        m_channels = channels;
    }

    inline BYTE getStep() const {
        return m_step;
    }
    inline void setStep( BYTE step ) {
        m_step = step;
    }

    inline BYTE getAmplitude() const {
        return m_amplitude;
    }
    inline void setAmplitude( BYTE amplitude ) {
        m_amplitude = amplitude;
    }

    inline int getOffset() const {
        return m_offset;
    }
    inline void setOffset( int offset ) {
        m_offset = offset;
    }

private:
    ChannelValueArray generateSineWave( int start_value, double start_angle, int amplitude, int step );
    ChannelValueArray generateStepWave( int start_value, int step );
    ChannelValueArray generateRandom( int start, int end );
    ChannelValueArray generateRampUp( int start_value, int amplitude, int maximum );
    ChannelValueArray generateRampDown( int start_value, int step, int minimum );

};



