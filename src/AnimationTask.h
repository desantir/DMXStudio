/* 
Copyright (C) 2011-2016 Robert DeSantis
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
    bool				m_active;

    BYTE				m_dmx_packet[MULTI_UNIV_PACKET_SIZE];

    ActorMap            m_actors;                               // Actors currently in play
    AnimationPtrArray	m_animations;
    bool                m_load_channels;

    AnimationTask(AnimationTask& other) {}
    AnimationTask& operator=(AnimationTask& rhs) { return *this; }

public:
    AnimationTask( Venue* venue );
    ~AnimationTask(void);

    void stageScene( Scene* scene, SceneLoadMethod method );
    void clearAnimations();
    void stageActor( SceneActor* actor );

    bool start();
    bool stop();

    inline SceneActor* getActor( UID actor_uid ) const {
        auto it = m_actors.find( actor_uid );
        if ( it == m_actors.end() )
            return NULL;

        return const_cast<SceneActor *>( &it->second );
    }

    inline AbstractAnimation* getAnimation( UID anim_uid ) const {
        for ( auto it=m_animations.begin(); it != m_animations.end(); it++ )
            if ( (*it)->getUID() == anim_uid )
                return (*it);
        return NULL;
    }

    inline Fixture* getFixture( UID pfuid ) const {
        return m_venue->getFixture( pfuid );
    }

    inline FixtureGroup* getFixtureGroup( UID uid ) const {
        return m_venue->getFixtureGroup( uid );
    }

    inline void loadChannel( BYTE *dmx_packet, Fixture* pf, channel_t channel, BYTE value ) {
        m_venue->loadChannel( dmx_packet, pf, channel, value );
    }
    
    inline FixturePtrArray convertActorToFixtures( UID actor_uid ) {
        SceneActor* actor = getActor( actor_uid );
        STUDIO_ASSERT( actor, "Actor %lu missing from scene with UID %d", actor_uid, m_venue->getCurrentSceneUID() );
        return resolveActorFixtures( actor );
    }

    inline FixturePtrArray resolveActorFixtures( SceneActor* actor ) {
        return m_venue->resolveActorFixtures( actor );
    }

    inline Fixture* getActorRepresentative( UID actor_uid ) {
        SceneActor* actor = getActor( actor_uid );
        STUDIO_ASSERT( actor, "Actor %lu missing from scene with UID %d", actor_uid, m_venue->getCurrentSceneUID() );

        if ( actor->isGroup() )
            return m_venue->getGroupRepresentative( actor->getActorUID() );

        return m_venue->getFixture( actor->getActorUID() );
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

    inline void updateChannels() {
        m_load_channels = true;
    }

protected:
    UINT run(void);

private:
    bool adjust_dimmer_channels();
    bool removeScene( Scene* scene );
    void reloadActors();
};

