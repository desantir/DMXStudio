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

#include "AnimationTask.h"

// ----------------------------------------------------------------------------
//
AnimationTask::AnimationTask( Venue* venue ) :
    Threadable( "AnimationTask" ),
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
AnimationTask::~AnimationTask(void)
{
    stop();
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::handleEvent( const Event& event ) 
{
    switch ( event.m_source ) {
        case ES_WHITEOUT:
        case ES_WHITEOUT_COLOR:
        case ES_WHITEOUT_STROBE:
        case ES_MASTER_DIMMER:
            if ( event.m_action == EA_CHANGED )
                m_load_channels = true;
            break;
     }

     return false;
}

// ----------------------------------------------------------------------------
//
UINT AnimationTask::run(void) {
    DMXStudio::log_status( "Scene animator running" );

    m_load_channels = true;

    CSingleLock lock( &m_animation_mutex, FALSE );

    while ( isRunning() ) {
        try { 
            lock.Lock();

            DWORD time_ms = GetCurrentTime();
            bool changed = m_load_channels;

            // ANIMATION TIME SLICE

            for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
                changed |= (*it)->sliceAnimation( time_ms, m_dmx_packet );

            // WHITEOUT STROBE

            if ( m_venue->getWhiteout() == WHITEOUT_STROBE_SLOW ||
                 m_venue->getWhiteout() == WHITEOUT_STROBE_FAST ||
                 m_venue->getWhiteout() == WHITEOUT_STROBE_MANUAL )
                changed |= m_venue->m_whiteout_strobe.strobe( time_ms );

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

            m_load_channels = false;

            lock.Unlock();

            if ( changed )
                m_venue->writePacket( m_dmx_packet );

            Sleep(1);
        }
        catch ( std::exception& ex ) {
            if ( lock.IsLocked() )
                lock.Unlock();

            DMXStudio::log( ex );

            clearAnimations();
        }
    }

    DMXStudio::log_status( "Scene animator stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::reloadActors() {
    ActorPtrArray actors;
    for ( ActorMap::iterator it=m_actors.begin(); it != m_actors.end(); it++ )
        actors.push_back( &it->second );

    m_venue->loadSceneChannels( m_dmx_packet, actors );
}

// ----------------------------------------------------------------------------
//
void AnimationTask::fadeToNextScene( ULONG fade_time, ActorPtrArray& actors ) {
    ChaseFader* fader = new ChaseFader( m_chase_fade_uid, fade_time, actors );

    m_animations.push_back( fader );

    fader->initAnimation( this, GetCurrentTime(), m_dmx_packet );
}

// ----------------------------------------------------------------------------
//
void AnimationTask::playScene( Scene* scene, SceneLoadMethod method )
{
    CSingleLock lock( &m_animation_mutex, TRUE );

    switch ( method ) {
        case SLM_LOAD:
            clearAnimations();

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

    // Store actors 
    ActorPtrArray actors = scene->getActors();
    for ( auto it = actors.begin(); it != actors.end(); it++ ) {
        m_actors[ (*it)->getActorUID() ] = *(*it);
    }

    // Latch in scene actors
    m_venue->loadSceneChannels( m_dmx_packet, scene->getActors() );

    // Initialize animations
    DWORD time_ms = GetCurrentTime();

    // Create animation clones and initialize
    AnimationPtrArray& animationPtrs = scene->animations();
    for ( size_t i=0; i < animationPtrs.size(); i++ ) {
        AbstractAnimation* anim = getAnimation( animationPtrs[i]->getUID() );

        if ( anim == NULL ) {
            anim = animationPtrs[i]->clone();
            m_animations.push_back( anim );
        }

        anim->initAnimation( this, time_ms, m_dmx_packet );

    }

    m_load_channels = true;             // Make sure we load channels at least once
}

// ----------------------------------------------------------------------------
//
void AnimationTask::stageActors( ActorPtrArray& actors ) {
    CSingleLock lock( &m_animation_mutex, TRUE );
    
    // Stage all future actors that require movement or gobo changes to avoid
    // latency in future scene setup.

    for ( SceneActor* future_actor : actors ) {
        // If the actor is currently in play, then it can not be staged
        if ( m_actors.find( future_actor->getActorUID() ) != m_actors.end() )
            continue;

        for ( Fixture* pf : resolveActorFixtures( future_actor ) ) {
            // It is possible for individual actors in a group to be in play
            if ( future_actor->isGroup() && m_actors.find( pf->getUID() ) != m_actors.end() )
                continue;
            
            for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                Channel* cp = pf->getChannel(channel);

                if ( cp->getType() == ChannelType::CHNLT_PAN ||
                     cp->getType() == ChannelType::CHNLT_PAN_FINE ||
                     cp->getType() == ChannelType::CHNLT_TILT ||
                     cp->getType() == ChannelType::CHNLT_TILT_FINE ||
                     cp->getType() == ChannelType::CHNLT_GOBO ) {

                     m_venue->loadChannel( m_dmx_packet, pf, channel, future_actor->getChannelValue( channel ) );
                     m_load_channels = true;
                }
            }
        }                
    }
}

// ----------------------------------------------------------------------------
// Remove all actors and animations associated with the given scene

bool AnimationTask::removeScene( Scene* scene )
{
    bool changed = false;

    // Remove actors 
    ActorPtrArray actors = scene->getActors();
    for ( auto it = actors.begin(); it != actors.end(); it++ ) {
        auto actors_it = m_actors.find( (*it)->getActorUID() );
        if ( actors_it != m_actors.end() ) {
            // Reset DMX values to "home" or zero
            for ( Fixture* pf : m_venue->resolveActorFixtures( &actors_it->second ) ) {
                for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                    BYTE home_value = pf->getChannel( channel )->getHomeValue();
                    m_venue->loadChannel( m_dmx_packet, pf, channel, home_value );
                }               
            }

            m_actors.erase( actors_it );
            changed = true;
        }
    }

    // Remove animations
    for ( AbstractAnimation* amin : scene->animations() )
        changed |= removeAnimation( amin->getUID() );

    return changed;
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::removeAnimation( UID amin_uid ) {
    bool changed = true;

    for ( auto it=m_animations.begin(); it != m_animations.end(); it++ ) {
        if ( (*it)->getUID() == amin_uid ) {
            AbstractAnimation* anim = (*it);

            m_animations.erase( it );
            anim->stopAnimation( );
            delete anim;

            changed = true;
            break;
        }
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::start()
{
    DMXStudio::getEventBus()->addListenerFirst( this );

    return startThread();
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::stop()
{
    DMXStudio::getEventBus()->removeListener( this );

    clearAnimations();

    if ( !stopThread() )
        return false;

    return true;
}

// ----------------------------------------------------------------------------
// Clear all animations, but do not change the current DMX values

void AnimationTask::clearAnimations()
{
    CSingleLock lock( &m_animation_mutex, TRUE );

    // Clear out all animations
    for ( size_t i=0; i < m_animations.size(); i++ ) {
        m_animations[i]->stopAnimation( );
        delete m_animations[i];
    }

    m_animations.clear();

    m_actors.clear();
}

// ----------------------------------------------------------------------------
// 
void AnimationTask::removeChaseFadeAnimation()
{
    removeAnimation( m_chase_fade_uid );
}

// ----------------------------------------------------------------------------
//
DWORD AnimationTask::getAnimationSampleRate()
{
    CSingleLock lock( &m_animation_mutex, TRUE );

    if ( m_animations.size() > 0 )
        return m_animations[0]->signal().getSampleRateMS();

    return 0L;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::setAnimationSampleSampleRate( DWORD sample_rate_ms )
{
    CSingleLock lock( &m_animation_mutex, TRUE );

    for ( size_t i=0; i < m_animations.size(); i++ )
        m_animations[i]->signal().setSampleRateMS( sample_rate_ms );

    DMXStudio::fireEvent( ES_ANIMATION_SPEED, 0L, EA_CHANGED, NULL, sample_rate_ms );
}
