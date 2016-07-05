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

#include "AnimationTask.h"

// ----------------------------------------------------------------------------
//
AnimationTask::AnimationTask( Venue* venue ) :
    Threadable( "AnimationTask" ),
    m_venue( venue ),
    m_active( true ),
    m_load_channels( false )
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
UINT AnimationTask::run(void) {
    DMXStudio::log_status( "Scene animator running" );

    m_load_channels = true;

    CSingleLock lock( &m_animation_mutex, FALSE );

    // TODO - these should all be even driven
    BYTE currentDimmer = m_venue->getMasterDimmer();
    WhiteoutMode currentWhiteoutMode = WHITEOUT_OFF;
    UINT curentWhiteoutStrobeMS = m_venue->getWhiteoutStrobeMS();
    RGBWA currntWhiteoutColor = m_venue->getWhiteoutColor();

    while ( isRunning() ) {
        try { 
            lock.Lock();

            DWORD time_ms = GetCurrentTime();
            bool changed = m_load_channels;

            // MASTER DIMMER

            if ( currentDimmer != m_venue->getMasterDimmer() ) {
                currentDimmer = m_venue->getMasterDimmer();
                // Adjust dimmer channels only to not interrrupt animation
                changed |= m_active && adjust_dimmer_channels();
            }

            // ANIMATION TIME SLICE

            for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
                changed |= (*it)->sliceAnimation( time_ms, m_dmx_packet );

            // WHITEOUT

            if ( currentWhiteoutMode != m_venue->getWhiteout() ||
                 curentWhiteoutStrobeMS != m_venue->getWhiteoutStrobeMS() ||
                 currntWhiteoutColor != m_venue->getWhiteoutColor() ) {
                currentWhiteoutMode = m_venue->getWhiteout();
                curentWhiteoutStrobeMS = m_venue->getWhiteoutStrobeMS();
                currntWhiteoutColor = m_venue->getWhiteoutColor();
                changed = true;
            }

            if ( currentWhiteoutMode == WHITEOUT_STROBE_SLOW ||
                currentWhiteoutMode == WHITEOUT_STROBE_FAST ||
                currentWhiteoutMode == WHITEOUT_STROBE_MANUAL )
                changed |= m_venue->m_whiteout_strobe.strobe( time_ms );

            // BLACKOUT
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
bool AnimationTask::adjust_dimmer_channels() 
{
    bool changed = false;

    for ( auto it : m_actors ) {
        SceneActor& actor = it.second;

        for ( Fixture* pf : resolveActorFixtures( &actor ) ) {
            for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                if ( pf->getChannel( channel )->isDimmer() ) {
                    BYTE value = actor.getChannelValue( channel );
                    loadChannel( m_dmx_packet, pf, channel, value );
                    changed = true;
                }
            }                
        }
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::stageScene( Scene* scene, SceneLoadMethod method )
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
            break;
        
        case SLM_MINUS:
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

    m_active = true;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::stageActor( SceneActor* actor ) {
    CSingleLock lock( &m_animation_mutex, TRUE );
    
    for ( Fixture* pf : resolveActorFixtures( actor ) ) {
        if ( !pf->canMove() || m_actors.find( pf->getUID() ) != m_actors.end() )
            continue;
            
        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel(channel);

            if ( cp->getType() == ChannelType::CHNLT_PAN ||
                 cp->getType() == ChannelType::CHNLT_PAN_FINE ||
                 cp->getType() == ChannelType::CHNLT_TILT ||
                 cp->getType() == ChannelType::CHNLT_TILT_FINE ||
                 cp->getType() == ChannelType::CHNLT_GOBO ) {

                 m_venue->loadChannel( m_dmx_packet, pf, channel, actor->getChannelValue( channel ) );
                 m_load_channels = true;
            }
        }                
    }
}

// ----------------------------------------------------------------------------
//
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
    AnimationPtrArray& animationPtrs = scene->animations();
    for ( size_t i=0; i < animationPtrs.size(); i++ ) {
        for ( auto it=m_animations.begin(); it != m_animations.end(); it++ ) {
            if ( (*it)->getUID() == animationPtrs[i]->getUID() ) {
                AbstractAnimation* anim = (*it);

                m_animations.erase( it );
                anim->stopAnimation( );
                delete anim;

                changed = true;
                break;
            }
        }
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::start()
{
    return startThread();
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::stop()
{
    clearAnimations();

    if ( !stopThread() )
        return false;

    return true;
}

// ----------------------------------------------------------------------------
//
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

    m_active = false;
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
        return m_animations[0]->signal().setSampleRateMS( sample_rate_ms );
}
