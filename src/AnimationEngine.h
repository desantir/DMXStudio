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

#include "stdafx.h"
#include "Threadable.h"
#include "Scene.h"
#include "Venue.h"
#include "ChaseFader.h"
#include "LevelRecord.h"

enum EngineAction {
    UpdateAnimation = 1,
    FadeToScene = 2,
    StageActors = 3,
    PlayScene = 4,
    ClearAllAnimations = 5,
    RemoveAnimation = 6,
    RestartAllAnimations = 7,
	RemoveTransientAnimation = 8,
	ClearAllTransientAnimations = 9,
	PlayTransientAnimation = 10,
	UpdateChannelValue = 11
};

struct EngineCommand {
    EngineAction    m_action;
    UID             m_target_uid;
    DWORD           m_data;

    EngineCommand( EngineAction action, UID target_uid, DWORD data ) :
        m_action( action ),
        m_target_uid( target_uid ),
        m_data( data )
    {}
};

typedef std::queue<EngineCommand> EngineCommandQueue;

class AnimationTask;

typedef std::vector<AnimationTask*> TaskPtrArray;

class AnimationEngine : public Threadable, public EventBusListener
{
    friend class AnimationTask;
    friend class AnimationSignalProcessor;

    CCriticalSection    m_animation_mutex;						// Protect animation objects
    CCriticalSection    m_command_mutex;						// Protect animation command queue

    EngineCommandQueue  m_command_queue;

    Venue*				m_venue;

    channel_value		m_dmx_packet[MULTI_UNIV_PACKET_SIZE];

    TaskPtrArray	    m_animation_tasks;
    bool                m_load_channels;
    UID                 m_chase_fade_uid;
    DWORD               m_whiteout_color_change_ms;

    UIDSet              m_active_actors;                       // Current set of active scene-base actors (non-transient)

	//  Needed to manage transient animations
	ChannelDataList		m_transient_channels;

    AnimationEngine(AnimationEngine& other);
    AnimationEngine& operator=(AnimationEngine& rhs) { return *this; }

    bool handleEvent( const Event& event );

public:
    AnimationEngine( Venue* venue );
    ~AnimationEngine(void);

    bool start();
    bool stop();

    inline void updateChannels() {
        m_load_channels = true;
    }

    inline void updateWhiteout( ) {
        m_whiteout_color_change_ms = 0;
    }

    void queue( EngineAction action, UID target_uid=0L, DWORD data=0L );

    AnimationLevelMap loadAnimationLevelData( DWORD after_ms );
	
	void playTransientAnimation( AnimationDefinition* animation, UID owner_uid );

protected:
    UINT run(void);

    inline Fixture* getFixture( UID pfuid ) const {
        return m_venue->getFixture( pfuid );
    }

    inline FixtureGroup* getFixtureGroup( UID uid ) const {
        return m_venue->getFixtureGroup( uid );
    }

	inline Fixture* getGroupRepresentative( UID actor_uid ) {
		return m_venue->getGroupRepresentative( actor_uid );
	}

    inline Palette* getPalette( UID palette_id ) const {
        return m_venue->getPalette( palette_id );
    }

    inline FixturePtrArray resolveActorFixtures( SceneActor* actor ) {
        return m_venue->resolveActorFixtures( actor );
    }

	inline UID getCurrentSceneUID() const {
		return m_venue->getCurrentSceneUID();
	}

    inline AudioInputStream* getAudio( ) {
        return m_venue->getAudio();
    }

    inline bool isMute() const {
        return m_venue->isMute();
    }

    inline void getSoundLevels( SoundLevel& level ) {
        m_venue->getSoundLevels( level );
    }

    inline UINT getVolume() {
        return m_venue->getMasterVolume();
    }

    inline DWORD scaleAnimationSpeed( DWORD time_ms ) {
        UINT animation_speed_percentage = m_venue->getAnimationSpeed();
        if ( animation_speed_percentage != 100 )
            time_ms = (time_ms * animation_speed_percentage) / 100;
        return time_ms;
    }

    inline channel_value getCurrentChannelValue( channel_address channel ) const {
        return m_dmx_packet[ channel ];
    }

    bool isActorActive( UID actor_uid ) const;

private:
    TaskPtrArray::iterator getTask( UID anim_uid, UID owner_uid=NOUID, bool transient=false );
    bool removeScene( Scene* scene );
    bool removeChaseFadeAnimation();
    void restoreActors( ActorList& actor_list );
    AnimationTask* createTask( AnimationDefinition* animation, ActorList& actors, UID owner_uid, bool transient );
    void processCommands();
	void resetActor( SceneActor* scene );
	void initTransientAnimation( AnimationTask* task, AnimationDefinition* animation );

    bool updateAnimation( UID animation_uid );
    void fadeToNextScene( UID scene_uid, ULONG fade_time );
    void stageActors( UID scene_uid );
    void playScene( UID scene_uid, SceneLoadMethod method );
    void clearAllAnimations();
    bool removeAnimation( UID animation_uid, UID owner_uid, bool transient );
    void restartAnimations( void );
	bool isActorBeingAnimated( UID actor_uid  );
	void clearAnimations( bool transients );
};

