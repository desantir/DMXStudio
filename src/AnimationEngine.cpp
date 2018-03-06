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

#include "AnimationEngine.h"
#include "AnimationTask.h"

// ----------------------------------------------------------------------------
//
AnimationEngine::AnimationEngine( Venue* venue ) :
    Threadable( "AnimationEngine" ),
    m_venue( venue ),
    m_load_channels( false ),
    m_chase_fade_uid( m_venue->allocUID() )
{
    // Initialize packet - we may start in blackout mode
    memset( m_dmx_packet, 0, sizeof(m_dmx_packet) );
    m_venue->setHomePositions( m_dmx_packet );
}

// ----------------------------------------------------------------------------
//
AnimationEngine::~AnimationEngine(void)
{
    stop();
}

// ----------------------------------------------------------------------------
//
bool AnimationEngine::start()
{
    CSingleLock animation_lock( &m_animation_mutex, TRUE );

    DMXStudio::getEventBus()->addListenerFirst( this );

    return startThread();
}

// ----------------------------------------------------------------------------
//
bool AnimationEngine::stop()
{
    CSingleLock animation_lock( &m_animation_mutex, TRUE );

    DMXStudio::getEventBus()->removeListener( this );

    clearAllAnimations();

    while ( m_command_queue.size() > 0 )
        m_command_queue.pop();

    m_animation_mutex.Unlock();

    return stopThread();
}

// ----------------------------------------------------------------------------
//
bool AnimationEngine::handleEvent( const Event& event ) 
{
    switch ( event.m_source ) {
        case ES_WHITEOUT:
        case ES_WHITEOUT_COLOR:
        case ES_WHITEOUT_STROBE:
        case ES_MASTER_DIMMER:
            if ( event.m_action == EA_CHANGED )
                m_load_channels = true;
            break;

        case ES_ANIMATION_SPEED:
            restartAnimations();
            break;
     }

     return false;
}

// ----------------------------------------------------------------------------
//
UINT AnimationEngine::run(void) {
    DMXStudio::log_status( "Animation engine running" );

    m_load_channels = true;

    CSingleLock lock( &m_animation_mutex, FALSE );

    m_whiteout_color_change_ms = 0;

	channel_value transient_packet[ MULTI_UNIV_PACKET_SIZE ];

    while ( isRunning() ) {
        try { 
            lock.Lock();

            processCommands();

            DWORD time_ms = GetCurrentTime();
            bool changed = m_load_channels;
			bool hasTransients = false;

            // ANIMATION TIME SLICE

			for ( AnimationTask* task : m_animation_tasks ) {
				if ( m_venue->isDefaultScene() || !task->isTransient() )
	                changed |= task->slice( time_ms, m_dmx_packet );
				else
					hasTransients = true;
			}

            // WHITEOUT STROBE

            if ( m_venue->getWhiteout() == WHITEOUT_STROBE_SLOW ||
                 m_venue->getWhiteout() == WHITEOUT_STROBE_FAST ||
                 m_venue->getWhiteout() == WHITEOUT_STROBE_MANUAL ||
                 m_venue->getWhiteout() == WHITEOUT_PULSE ) {
                changed |= m_venue->m_whiteout_strobe.check_strobe( time_ms );
            }
            else if ( m_venue->getWhiteout() == WHITEOUT_ON && time_ms >= m_whiteout_color_change_ms ) {
                m_venue->advanceWhiteoutColor();
                m_whiteout_color_change_ms = time_ms + m_venue->m_whiteout_strobe_ms;
                changed = true;
            }

            // MUTE BLACKOUT

            if ( m_venue->getAutoBlackoutMS() != 0 && m_venue->isMute() && !m_venue->isMuteBlackout() ) {
                m_venue->setMuteBlackout( true );
                changed = true;
            }
            else if ( !m_venue->isMute() && m_venue->isMuteBlackout() ) {
                m_venue->setMuteBlackout( false );
                changed = true;
            }
            else if ( m_venue->isMuteBlackout() )   // In blackout - don't need to push changes unless forced
                changed = m_load_channels;

			// APPLY TRANSIENT ANIMATIONS

			if ( hasTransients ) {
				memcpy( transient_packet, m_dmx_packet, MULTI_UNIV_PACKET_SIZE );

				if ( m_load_channels ) // Need to do this now when transients
					m_venue->applyDefaultActors( transient_packet, &m_transient_channels );

				// Restore transient fixure channel values as they may have been clobbered by scene actors
				for ( ChannelData& cd : m_transient_channels )
					transient_packet[ cd.m_address ] = cd.m_value;

				for ( AnimationTask* task : m_animation_tasks ) {
					if ( task->isTransient() )
						changed |= task->slice( time_ms, transient_packet );
				}

				// Save channel values for transient fixures
				for ( ChannelData& cd : m_transient_channels )
					cd.m_value = transient_packet[ cd.m_address ];
			}

			m_load_channels = false;

            lock.Unlock();

			if ( changed ) {
				if ( !hasTransients )
					m_venue->writePacket( m_dmx_packet, true );
				else
					m_venue->writePacket( transient_packet, false );
			}

            Sleep(5);
        }
        catch ( std::exception& ex ) {
            if ( lock.IsLocked() )
                lock.Unlock();

            DMXStudio::log( ex );

            clearAllAnimations();
        }
    }

    DMXStudio::log_status( "Animation engine stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::queue( EngineAction action, UID target_uid, DWORD data ) {
    CSingleLock command_lock( &m_command_mutex, TRUE );

    // printf( "Action: %u UID: %lu Size: %u\n", action, target_uid, m_command_queue.size() );

    m_command_queue.emplace( action, target_uid, data );
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::processCommands( ) {
    CSingleLock venue_lock( m_venue->getVenueLock(), TRUE );
    CSingleLock command_lock( &m_command_mutex, TRUE );

    while ( m_command_queue.size() > 0 ) {
        EngineCommand cmd = m_command_queue.front();

        m_command_queue.pop();

        switch ( cmd.m_action ) {
            case UpdateAnimation:
                updateAnimation( cmd.m_target_uid );
                break;

            case FadeToScene:
                fadeToNextScene( cmd.m_target_uid, cmd.m_data );
                break;

            case StageActors:
                stageActors( cmd.m_target_uid );
                break;

            case PlayScene:
                playScene( cmd.m_target_uid, (SceneLoadMethod)cmd.m_data );
                break;

            case ClearAllAnimations:
				clearAllAnimations();
                break;

			case ClearAllTransientAnimations:
				clearAnimations( true );
				updateChannels(); 
				break;

            case RemoveAnimation:
                removeAnimation( cmd.m_target_uid, (UID)cmd.m_data, false );
                break;

			case RemoveTransientAnimation:
				removeAnimation( cmd.m_target_uid, (UID)cmd.m_data, true );
				break;

            case RestartAllAnimations:
                restartAnimations();
                break;

			case PlayTransientAnimation: {
				AnimationDefinition* animation = m_venue->getAnimation( cmd.m_target_uid );
				STUDIO_ASSERT( animation, "Unable to find animation UID %lu", cmd.m_target_uid );
				playTransientAnimation( animation, (UID)cmd.m_data );
				break;
			}

			case UpdateChannelValue: {  // Data contains the packed real packet address and value
				m_dmx_packet[(channel_address)(cmd.m_data >> 8)] = (channel_value)(cmd.m_data & 0xFFFF);
				updateChannels(); 
				break;
			}
        }
    }
}

// ----------------------------------------------------------------------------
//
TaskPtrArray::iterator AnimationEngine::getTask( UID anim_uid, UID owner_uid, bool transient ) {
    for ( TaskPtrArray::iterator it=m_animation_tasks.begin(); it != m_animation_tasks.end(); ++it )
        if ( (*it)->getUID() == anim_uid && ( owner_uid==NOUID || owner_uid == (*it)->getOwnerUID()) && (*it)->isTransient() == transient )
            return it;
    return m_animation_tasks.end();
}

// ----------------------------------------------------------------------------
//
AnimationTask* AnimationEngine::createTask( AnimationDefinition* animation, ActorList& actors, UID owner_uid, bool transient ) {
	AnimationTask* task = animation->createTask( this, actors, owner_uid );
	task->setTransient( transient );
	return task;
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::fadeToNextScene( UID scene_uid, ULONG fade_time ) {
    Scene* scene = m_venue->getScene( scene_uid );
    if ( scene == NULL )
        return;

    ChaseFader fader( m_chase_fade_uid, fade_time, scene->getActors() );

    // Make sure we don't already have one - we never should
    if ( removeChaseFadeAnimation() )
        DMXStudio::log_status( "A chase fader was still active" );       

    UIDArray actor_uids;
    for ( auto actor : fader.getTargetActors() )
        actor_uids.push_back( actor->getActorUID() );

	ActorList actor_list;
	for ( SceneActor* actor : scene->getActors() )
		actor_list.emplace_back( *actor );

    AnimationTask* task = createTask( &fader, actor_list, NOUID, false );

    m_animation_tasks.push_back( task );

    task->init( &fader, GetCurrentTime(), m_dmx_packet );
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::playScene( UID scene_uid, SceneLoadMethod method ) {
    Scene* scene = m_venue->getScene( scene_uid );
    if ( scene == NULL )
        return;

    switch ( method ) {
        case SLM_LOAD:
            m_active_actors.clear();

			// If default scene, then clear all transients animations since we optimize for this state
			if ( m_venue->getDefaultScene()->getUID() == scene_uid )
				clearAllAnimations();
			else
				clearAnimations( false );

            // Stage new scene animations
            memset( m_dmx_packet, 0, sizeof(m_dmx_packet) );

            m_venue->setHomePositions( m_dmx_packet );
            break;
        
        case SLM_ADD:
            removeChaseFadeAnimation();
            break;
        
        case SLM_MINUS:
            removeChaseFadeAnimation();
            m_load_channels = removeScene( scene );
            return;
    }

    // FUTURE - Maintain consistency when the same animation and actors span scenes
    //          Would need to retain the task, actors, actor channels, etc.
    //          Challenges: not reseting animation channels, scene actor channel differences

    // Latch in scene actors
    m_venue->loadSceneChannels( m_dmx_packet, scene->getActors() );

    // Keep track of active actors
    for ( auto actor : scene->getActors() )
        m_active_actors.insert( actor->getActorUID() );

    // Initialize animations
    DWORD time_ms = GetCurrentTime();

    // Create animation clones and initialize
    for ( AnimationReference& animation_ref : scene->animations() ) {
        AnimationDefinition* animation = m_venue->getAnimation( animation_ref.getUID() );
        STUDIO_ASSERT( animation, "Unable to find animation UID %lu in scene %s", animation_ref.getUID(), scene->getName() );

        TaskPtrArray::iterator it = getTask( animation_ref.getUID(), scene->getUID() );

        if ( it == m_animation_tasks.end() ) {
			ActorList actor_list;
			for ( UID actor_uid : animation_ref.getActors() )
				actor_list.emplace_back( *scene->getActor( actor_uid ) );

            AnimationTask* task = createTask( animation, actor_list, scene->getUID(), false );

            m_animation_tasks.push_back( task );

            task->init( animation, time_ms, m_dmx_packet );
        }
    }

    updateChannels();                    // Make sure we load channels at least once
}

// ----------------------------------------------------------------------------
//
bool AnimationEngine::updateAnimation( UID animation_uid ) {
    AnimationDefinition* animation = m_venue->getAnimation( animation_uid );
    if ( animation == NULL )
        return false;

    bool found = false;

    // Since we are updating an animation definition, there may be multiple instances of this animation
    for ( TaskPtrArray::iterator it=m_animation_tasks.begin(); it != m_animation_tasks.end(); it++ )
        if ( (*it)->getUID() == animation->getUID() ) {

            AnimationTask* task = createTask( animation, (*it)->getActors(), (*it)->getOwnerUID(), (*it)->isTransient() );

            // Erase the current task as this may be a different animator with same UID
            (*it)->stop( );
            delete (*it);
            
            it = m_animation_tasks.insert( m_animation_tasks.erase( it ), task );

			if ( !task->isTransient() ) {
				restoreActors( task->getActors() );

				task->init( animation, GetCurrentTime(), m_dmx_packet );
			}
			else {
				initTransientAnimation( task, animation );
			}

            found = true;
        }

    if ( found )
        updateChannels();    

    return found;
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::restartAnimations( ) {
    DWORD time_ms = GetCurrentTime();
    bool changed = false;

    for ( AnimationTask* task : m_animation_tasks )
        changed |= task->restart( time_ms, m_dmx_packet );

    if ( changed )
        updateChannels();
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::playTransientAnimation( AnimationDefinition* animation, UID owner_uid  ) {
    CSingleLock lock( &m_animation_mutex, TRUE );

	// If the animation is already in our list, remove the old one first
	removeAnimation( animation->getUID(), owner_uid, true );

	// Store default scene actors 
	ActorList actor_list;
	for ( SceneActor* actor : m_venue->getDefaultScene()->getActors() )
		actor_list.emplace_back( *actor );

    AnimationTask* task = createTask( animation, actor_list, owner_uid, true );
    m_animation_tasks.push_back( task );

	initTransientAnimation( task, animation );
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::initTransientAnimation( AnimationTask* task, AnimationDefinition* animation ) {
	channel_value transient_packet[ MULTI_UNIV_PACKET_SIZE ];

	m_venue->applyDefaultActors( transient_packet, &m_transient_channels );

	task->init( animation, GetCurrentTime(), transient_packet );

	// Save channel values for transient fixures
	for ( ChannelData& cd : m_transient_channels )
		cd.m_value = transient_packet[ cd.m_address ];
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::clearAnimations( bool transients ) {
	CSingleLock lock( &m_animation_mutex, TRUE );

	for ( TaskPtrArray::iterator it=m_animation_tasks.begin(); it != m_animation_tasks.end(); ) {
		if ( (*it)->isTransient( ) == transients ) {
			(*it)->stop( );
			delete (*it);

			it = m_animation_tasks.erase( it );
		}
		else
			++it;
	}
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::stageActors( UID scene_uid ) {
    Scene* scene = m_venue->getScene( scene_uid );
    if ( scene == NULL )
        return;
    
    // Stage all future actors that require movement or gobo changes to avoid
    // latency in future scene setup.

    for ( SceneActor* future_actor : scene->getActors() ) {
        // If the actor is currently in play, then it can not be staged
        if ( isActorBeingAnimated( future_actor->getActorUID() ) )
            continue;

        for ( Fixture* pf : resolveActorFixtures( future_actor ) ) {
            // It is possible for individual actors in a group to be in play
            if ( future_actor->isGroup() && isActorBeingAnimated( pf->getUID() ) )
                continue;
            
            for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                Channel* cp = pf->getChannel(channel);

                if ( cp->getType() == ChannelType::CHNLT_PAN ||
                     cp->getType() == ChannelType::CHNLT_PAN_FINE ||
                     cp->getType() == ChannelType::CHNLT_TILT ||
                     cp->getType() == ChannelType::CHNLT_TILT_FINE ||
                     cp->getType() == ChannelType::CHNLT_GOBO ) {

                     m_venue->loadChannel( m_dmx_packet, pf, channel, future_actor->getFinalChannelValue( pf->getUID(), channel ) );
                     updateChannels(); 
                }
            }
        }                
    }
}

// ----------------------------------------------------------------------------
// Remove all actors and animations associated with the given scene

bool AnimationEngine::removeScene( Scene* scene ) {
    bool changed = false;

	// Remove animations
	for ( AnimationReference& animation_ref : scene->animations() )
		changed |= removeAnimation( animation_ref.getUID(), scene->getUID(), false );

    // Restore actors to init state
	if ( changed ) {
        for ( auto actor : scene->getActors() ) {
			if ( !isActorActive( actor->getActorUID() ) ) {
				resetActor( actor );
                m_active_actors.erase( actor->getActorUID() );
            }
        }
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::resetActor( SceneActor* actor ) {
	// Reset DMX values to "home" or zero
	for ( Fixture* pf : m_venue->resolveActorFixtures( actor ) ) {
		for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
			channel_value home_value = pf->getChannel( channel )->getHomeValue();
			m_venue->loadChannel( m_dmx_packet, pf, channel, home_value );
		}               
	}
}

// ----------------------------------------------------------------------------
//
void AnimationEngine::restoreActors( ActorList& actor_list ) {
    ActorPtrArray actors;

    for ( SceneActor& actor : actor_list )
		actors.push_back( &actor );

    m_venue->loadSceneChannels( m_dmx_packet, actors );
}

// ----------------------------------------------------------------------------
//
bool AnimationEngine::removeAnimation( UID amin_uid, UID owner_uid, bool transient ) {
	CSingleLock lock( &m_animation_mutex, TRUE );

    TaskPtrArray::iterator it = getTask( amin_uid, owner_uid, transient );
    if ( it == m_animation_tasks.end() )
        return false;

    AnimationTask* task = (*it);
    m_animation_tasks.erase( it );
    
    task->stop( );

	if ( !transient )
		restoreActors( task->getActors() );

	delete task;

    updateChannels();
           
    return true;
}

// ----------------------------------------------------------------------------
// Clear all animations, but do not change the current DMX values

void AnimationEngine::clearAllAnimations() {
	CSingleLock lock( &m_animation_mutex, TRUE );

    // Clear out all animations
    for ( size_t i=0; i < m_animation_tasks.size(); i++ ) {
        m_animation_tasks[i]->stop( );
        delete m_animation_tasks[i];
    }

    m_animation_tasks.clear();
}

// ----------------------------------------------------------------------------
// 
bool AnimationEngine::removeChaseFadeAnimation()
{
    return removeAnimation( m_chase_fade_uid, NOUID, false );
}

// ----------------------------------------------------------------------------
// 
AnimationLevelMap AnimationEngine::loadAnimationLevelData( DWORD after_ms )
{
    AnimationLevelMap results;

    CSingleLock lock( &m_animation_mutex, TRUE );

    for ( AnimationTask* task : m_animation_tasks )
        if ( task->getSignalProcessor() != NULL )
            results[task->getUID() ] = task->getSignalProcessor()->fetchLevelData( after_ms );

    return results;
}

// ----------------------------------------------------------------------------
//
bool AnimationEngine::isActorBeingAnimated( UID actor_uid  ) {
	for ( AnimationTask* task : m_animation_tasks )
        if ( task->getUID() != m_chase_fade_uid && task->getActor( actor_uid ) != NULL )
			return true;
	return false;
}

// ----------------------------------------------------------------------------
// Returns TRUE if the indicator actor is currently active.  The actor may
// or may not be part of an animation and transient actors are ignored.
bool AnimationEngine::isActorActive( UID actor_uid ) const {
    return m_active_actors.find( actor_uid ) != m_active_actors.end();
}
