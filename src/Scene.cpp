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


#include "DMXStudio.h"
#include "Scene.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
Scene::Scene( UID uid, SceneNumber scene_number, const char * name, const char *description ) : 
    DObject( uid, scene_number, name, description ),
    m_bpm_rating( BPM_NO_RATING )
{
}

// ----------------------------------------------------------------------------
//
Scene::Scene( Scene& other ) :
        DObject( other ),
        m_actors( other.m_actors ),
        m_bpm_rating( other.m_bpm_rating )
{
    copy_animations( other );
}

// ----------------------------------------------------------------------------
//
Scene& Scene::operator=( Scene& rhs ) {
    DObject::operator=( rhs ); 
    m_actors = rhs.m_actors;
    m_acts = rhs.m_acts;
    m_bpm_rating = rhs.m_bpm_rating;
    copy_animations(rhs );
    return *this;
}

// ----------------------------------------------------------------------------
//
void Scene::copy_animations( Scene& rhs ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    for ( AnimationPtrArray::iterator it=rhs.m_animations.begin();
          it != rhs.m_animations.end(); ++it ) {
        m_animations.push_back( (*it)->clone() );
    }
}

// ----------------------------------------------------------------------------
//
Scene::~Scene(void)
{
    clearAnimations();
}

// ----------------------------------------------------------------------------
//
void Scene::addActor( SceneActor& actor ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    m_actors[ actor.getActorUID() ] = actor;
}

// ----------------------------------------------------------------------------
//
bool Scene::removeActor( UID uid ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    ActorMap::iterator it = m_actors.find( uid );
    if ( it == m_actors.end() )
        return false;

    m_actors.erase( it );

    // Remove actor from all animations
    for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
        (*it)->removeActor( uid );

    return true;
}

// ----------------------------------------------------------------------------
//
SceneActor* Scene::getActor( UID uid ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    ActorMap::iterator it = m_actors.find( uid );
    if ( it != m_actors.end() )
        return &it->second;
    return NULL;
}

// ----------------------------------------------------------------------------
//
ActorPtrArray Scene::getActors( ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    ActorPtrArray list;
    ActorMap::iterator it;

    for ( it=m_actors.begin(); it != m_actors.end(); ++it )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
UIDArray Scene::getActorUIDs( ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    UIDArray list;
    ActorMap::iterator it;

    for ( it=m_actors.begin(); it != m_actors.end(); ++it )
        list.push_back( it->first );

    return list;
}

// ----------------------------------------------------------------------------
// Transfers animation life-cycle responsibilty to the scene
void Scene::addAnimation( AbstractAnimation* animation ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    if ( animation->getUID() == NOUID )
        animation->setUID( studio.getVenue()->allocUID() );

    m_animations.push_back( animation );
}

// ----------------------------------------------------------------------------
// Animation destructor will be called
void Scene::clearAnimations( ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
        delete (*it);
    m_animations.clear();
}

// ----------------------------------------------------------------------------
// 
AbstractAnimation* Scene::getAnimation( size_t animation_num ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    STUDIO_ASSERT( animation_num < m_animations.size(), "Requested invalid animation" );
    return m_animations[ animation_num ];
}

// ----------------------------------------------------------------------------
// Animation destructor will be called
void Scene::removeAnimation( UID animation_uid ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
        if ( (*it)->getUID() == animation_uid ) {
            AbstractAnimation* animation = (*it);
            m_animations.erase( it );
            delete animation;
            break;
        }
}

// ----------------------------------------------------------------------------
// Transfers animation life-cycle responsibilty to the scene
void Scene::insertAnimation( unsigned animation_num, AbstractAnimation* animation ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    STUDIO_ASSERT( animation_num <= m_animations.size(), "Animation insert step out of range" );
    m_animations.insert( m_animations.begin()+animation_num, animation );
}
