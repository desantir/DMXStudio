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

#include "stdafx.h"
#include "RGBWA.h"
#include "SceneActor.h"
#include "AnimationEngine.h"
#include "AnimationDefinition.h"
#include "AnimationSignalProcessor.h"

struct ChannelUpdate
{
	Fixture*			m_pf;			            // Fixture
	channel_address		m_channel;					// Channel #
	channel_value		m_value;					// Channel value

	ChannelUpdate( Fixture*	pf, channel_address channel, channel_value current_value ) :
		m_pf( pf ),
		m_channel( channel ),
		m_value( current_value )
	{}

	~ChannelUpdate() {}
};

typedef std::vector<ChannelUpdate> ChannelUpdateList;

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

class AnimationTask
{
    friend class AnimationEngine;

    AnimationTask& operator=( AnimationTask& rhs ) {}
    AnimationTask( const AnimationTask& other ) {}

    AnimationSignalProcessor*	m_signal_processor;
    AnimationEngine*	        m_engine;

	ChannelUpdateList			m_channel_updates;

protected:
    UID                         m_animation_uid;
    UID                         m_owner_uid;
	bool						m_transient;
	ActorList					m_actors;

public:
    AnimationTask() {}

    AnimationTask( AnimationEngine*	engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID ) :
        m_engine( engine ),
        m_actors( actors ),
        m_animation_uid( animation_uid ),
        m_owner_uid( owner_uid),
        m_signal_processor( NULL ),
		m_transient( false )
    {
    }

    virtual ~AnimationTask( void ) {
        if ( m_signal_processor != NULL ) {
            delete m_signal_processor;
            m_signal_processor = NULL;
        }
    }

protected:
    virtual void setupAnimation( AnimationDefinition* animation, DWORD time_ms ) = 0;
    virtual bool sliceAnimation( DWORD time_ms ) = 0;
    virtual bool restartAnimation( DWORD time_ms ) = 0; 
    virtual void stopAnimation() = 0; 

    inline SignalState tick( DWORD time_ms ) {
        return m_signal_processor->tick( time_ms );
    }

    inline UINT getLevel() const {
        return m_signal_processor->getLevel();
    }

    inline DWORD getNextSampleMS() const {
        return m_signal_processor->getNextSampleMS();
    }

    inline AnimationSignal& getSignal() {
        return m_signal_processor->signal();
    }

    inline UID getUID() const {
        return m_animation_uid;
    }

    inline UID getOwnerUID() const {
        return m_owner_uid;
    }

    inline AnimationEngine* getEngine() const {
        return m_engine;
    }

    inline DWORD scaleAnimationSpeed( DWORD time_ms ) {
        return m_engine->scaleAnimationSpeed( time_ms );
    }

    inline AnimationSignalProcessor* getSignalProcessor() {
        return m_signal_processor;
    }

    inline ActorList& getActors( ) {
        return m_actors;
    }

	inline SceneActor* getActor( UID actor_uid ) const {
		for ( const SceneActor& actor : m_actors )
			if ( actor.getActorUID() == actor_uid )
				return const_cast<SceneActor *>( &actor );
		return NULL;
	}

	inline Fixture* getActorRepresentative( UID actor_uid ) {
		SceneActor* actor = getActor( actor_uid );
		STUDIO_ASSERT( actor, "Actor %lu missing from scene with UID %d", actor_uid, m_engine->getCurrentSceneUID() );

		if ( actor->isGroup() )
			return m_engine->getGroupRepresentative( actor->getActorUID() );

		return m_engine->getFixture( actor->getActorUID() );
	}

    inline Fixture* getFixture( UID pfuid ) const {
        return m_engine->getFixture( pfuid );
    }

    inline FixtureGroup* getFixtureGroup( UID uid ) const {
        return m_engine->getFixtureGroup( uid );
    }

    inline Palette* getPalette( UID palette_id ) const {
        return m_engine->getPalette( palette_id );
    }

    inline void loadChannel( Fixture* pf, channel_address channel, channel_value value ) {
		m_channel_updates.emplace_back( pf, channel, value );
    }

    inline FixturePtrArray resolveActorFixtures( SceneActor* actor ) {
        return m_engine->resolveActorFixtures( actor );
    }

    inline AudioInputStream* getAudio( ) {
        return m_engine->getAudio();
    }

    bool isMute() const {
        return m_engine->isMute();
    }

    inline void getSoundLevels( SoundLevel& level ) {
        m_engine->getSoundLevels( level );
    }

    inline UINT getVolume() {
        return m_engine->getVolume();
    }

	inline bool isTransient( ) const {
		return m_transient;
	}

    inline channel_value getCurrentChannelValue( channel_address channel ) const {
        return m_engine->getCurrentChannelValue( channel );
    }

    inline bool isActorActive( UID actor_uid ) const {
        return m_engine->isActorActive( actor_uid );
    }

    void setupFixtureFaders( FaderFixtureArray& faders );
    void loadFaderColorChannels( FaderFixture& sfixture, RGBWA& rgbwa );

private:
    void init( AnimationDefinition* animation, DWORD time_ms, channel_value* dmx_packet );
	bool slice( DWORD time_ms, channel_value* dmx_packet );
	bool restart( DWORD time_ms, channel_value* dmx_packet );

    virtual void stop( void ) {
        stopAnimation();

        if ( m_signal_processor ) {
            delete m_signal_processor;
            m_signal_processor = NULL;
        }
    }

    virtual bool restart( DWORD time_ms ) { 
        if ( m_signal_processor != NULL ) 
            m_signal_processor->restart( time_ms );

        return restartAnimation( time_ms );
    }

	inline void setTransient( bool transient ) {
		m_transient = transient;
	}

	void applyChannelUpdates( channel_value* dmx_packet );
};


