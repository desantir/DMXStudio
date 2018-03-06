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
        m_bpm_rating( other.m_bpm_rating ),
        m_animations( other.m_animations )
{
}

// ----------------------------------------------------------------------------
//
Scene& Scene::operator=( Scene& rhs ) {
    if ( this == &rhs )
        return *this;

    DObject::operator=( rhs ); 
    m_actors = rhs.m_actors;
    m_bpm_rating = rhs.m_bpm_rating;
    m_animations = rhs.m_animations;

    return *this;
}

// ----------------------------------------------------------------------------
//
Scene::~Scene(void)
{
}

// ----------------------------------------------------------------------------
//
void Scene::addActor( SceneActor& actor ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    m_actors[ actor.getActorUID() ] = actor;
}

// ----------------------------------------------------------------------------
//
void Scene::setActors( ActorList& actors ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    UIDArray old_actors = getActorUIDs();

    m_actors.clear();

    for ( SceneActor& actor : actors )
        m_actors[ actor.getActorUID() ] = actor;

    // Remove all actors that have been removed from the scene (do after animation update in case they are affected)
    for ( UID old_actor_uid : old_actors )
        if ( m_actors.find( old_actor_uid ) == m_actors.end() )
            removeActor( old_actor_uid );
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
    for ( AnimationReference& animation : m_animations )
        animation.removeActor( uid );

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
//
void Scene::addAnimation( AnimationReference& animation ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    m_animations.push_back( animation );
}

// ----------------------------------------------------------------------------
//
bool Scene::clearAnimations( ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    bool changed = m_animations.size() > 0;

    m_animations.clear();

    return changed;
}

// ----------------------------------------------------------------------------
// 
AnimationReference* Scene::getAnimation( size_t animation_index) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    STUDIO_ASSERT( animation_index < m_animations.size(), "Requested invalid animation" );
    return &m_animations[ animation_index ];
}

// ----------------------------------------------------------------------------
// 
AnimationReference* Scene::getAnimationByUID( UID animation_uid ) {
    for ( AnimationReference& ref : m_animations )
        if ( ref.getUID() == animation_uid )
            return &ref;
    return NULL;
}

// ----------------------------------------------------------------------------
//
bool Scene::removeAnimationByUID( UID animation_uid ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    for ( AnimationReferenceArray::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
        if ( (*it).getUID() == animation_uid ) {
            m_animations.erase( it );
            return true;
        }

    return false;
}

// ----------------------------------------------------------------------------
//
void Scene::replaceAnimation( size_t animation_index, AnimationReference& animation ) {
    CSingleLock lock( &m_scene_mutex, TRUE );

    STUDIO_ASSERT( animation_index < m_animations.size(), "Animation delete index out of range" );

    m_animations[animation_index] = animation;
}
