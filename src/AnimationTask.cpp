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
    m_scene( NULL ),
    m_load_channels( false )
{
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

    while ( isRunning() ) {
        try { 
            lock.Lock();

            DWORD time_ms = GetCurrentTime();
            bool changed = m_load_channels;
            m_load_channels = false;

            for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
                changed |= (*it)->sliceAnimation( time_ms, m_dmx_packet );

            if ( m_venue->getWhiteout() == WHITEOUT_STROBE_SLOW ||
                 m_venue->getWhiteout() == WHITEOUT_STROBE_FAST ||
                 m_venue->getWhiteout() == WHITEOUT_STROBE_MANUAL )
                changed |= m_venue->m_whiteout_strobe.strobe( time_ms );

            if ( m_venue->isLightBlackout() )   // Already in blackout - don't need to push changes
                changed = false;

            // Check auto blackout situation
            if ( m_venue->getAutoBlackout() != 0 && m_venue->isMute() && !m_venue->isLightBlackout() ) {
                m_venue->setLightBlackout( true );
                if ( m_scene )
                    m_venue->loadSceneChannels( m_dmx_packet, m_scene );
                changed = true;
            }
            else if ( !m_venue->isMute() && m_venue->isLightBlackout() ) {
                m_venue->setLightBlackout( false );
                if ( m_scene )
                    m_venue->loadSceneChannels( m_dmx_packet, m_scene );
                // We should apply animation slice values here ...
                changed = true;
            }

            lock.Unlock();

            if ( changed )
                m_venue->writePacket( m_dmx_packet );

            Sleep(1);
        }
        catch ( std::exception& ex ) {
            DMXStudio::log( ex );

            if ( lock.IsLocked() )
                lock.Unlock();

            clearAnimations();
        }
    }

    DMXStudio::log_status( "Scene animator stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::stageScene( Scene* scene )
{
    CSingleLock lock( &m_animation_mutex, TRUE );

    clearAnimations();

    // Stage new scene animations
    m_scene = scene;

    memset( m_dmx_packet, 0, sizeof(m_dmx_packet) );

    m_venue->setHomePositions( m_dmx_packet );

    // Latch in scene actors
    m_venue->loadSceneChannels( m_dmx_packet, m_scene );

    // Initialize animations
    DWORD time_ms = GetCurrentTime();

    // Create animation clones and initialize
    AnimationPtrArray& animationPtrs = m_scene->animations();
    for ( size_t i=0; i < animationPtrs.size(); i++ ) {
        AbstractAnimation* anim = animationPtrs[i]->clone();
        anim->initAnimation( this, time_ms, m_dmx_packet );
        m_animations.push_back( anim );
    }

    m_load_channels = true;             // Make sure we load channels at least once
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

    m_scene = NULL;
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
