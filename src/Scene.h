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

#include "IVisitor.h"
#include "SceneActor.h"
#include "AbstractAnimation.h"

typedef ULONG SceneNumber;								// Scene numbers will be user friendly numbers 1-n

class Scene : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    SceneNumber			m_scene_number;
    ActorMap			m_actors;						// "Actors" in this scene

    AnimationPtrArray	m_animations;

    void copy_animations( Scene& rhs );

public:
    Scene() {}
    Scene( Scene& other );
    Scene( UID uid, SceneNumber scene_number, const char * name = NULL, const char *description = NULL );
    ~Scene(void);

    Scene& operator=( Scene& rhs );

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    void addActor( SceneActor& actor );
    bool removeActor( UID uid );
    SceneActor* getActor( UID uid );
    ActorPtrArray getActors( void );
    UIDArray getActorUIDs( void );

    size_t getNumActors( void ) const {
        return m_actors.size();
    }

    void removeAllActors() {
        m_actors.clear();
    }

    SceneNumber getSceneNumber( void ) const {
        return m_scene_number;
    }
    void setSceneNumber( SceneNumber scene_number ) {
        m_scene_number = scene_number;
    }

    bool hasAnimations() const {
        return m_animations.size() > 0;
    }

    size_t getNumAnimations() const {
        return m_animations.size();
    }

    AbstractAnimation* getAnimation( size_t animation_num );
    void addAnimation( AbstractAnimation* animation );
    void removeAnimation( UID animation_uid );
    void insertAnimation( unsigned animation_num, AbstractAnimation* animation );
    void clearAnimations( );

    AbstractAnimation* getAnimation( UID animation_uid ) {
        for ( AnimationPtrArray::iterator it=m_animations.begin(); it != m_animations.end(); it++ )
            if ( (*it)->getUID() == animation_uid )
                return (*it);
        return NULL;
    }

    AnimationPtrArray& animations() {
        return m_animations;
    }
};

typedef std::map<UID,Scene> SceneMap;
typedef std::vector<Scene *> ScenePtrArray;