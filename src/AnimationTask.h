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
#include "Threadable.h"
#include "Scene.h"
#include "Venue.h"

class AnimationTask : public Threadable
{
    CCriticalSection    m_animation_mutex;						// Protect animation objects

    Venue*				m_venue;
    Scene*				m_scene;

    BYTE				m_dmx_packet[DMX_PACKET_SIZE];

    AnimationPtrArray	m_animations;
    bool                m_load_channels;

    AnimationTask(AnimationTask& other) {}
    AnimationTask& operator=(AnimationTask& rhs) { return *this; }

public:
    AnimationTask( Venue* venue );
    ~AnimationTask(void);

    void stageScene( Scene* scene );
    void clearAnimations();

    bool start();
    bool stop();

    inline Scene* getScene() const {
        return m_scene;
    }

    inline Fixture* getFixture( UID pfuid ) const {
        return m_venue->getFixture( pfuid );
    }

    inline void loadChannel( BYTE *dmx_packet, Fixture* pf, channel_t channel, BYTE value ) {
        m_venue->loadChannel( dmx_packet, pf, channel, value );
    }

    inline AudioInputStream* getAudio( ) {
        return m_venue->getAudio();
    }

    bool isMute() const {
        return m_venue->isMute();
    }

    unsigned getSoundLevel( ) const {
        return m_venue->getSoundLevel();
    }

    unsigned getAvgAmplitude( ) const {
        return m_venue->getAvgAmplitude();
    }

    DWORD getAnimationSampleRate();
    void setAnimationSampleSampleRate( DWORD sample_rate_ms );

protected:
    UINT run(void);
};

