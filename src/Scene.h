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
#include "AnimationDefinition.h"
#include "AnimationReference.h"

typedef ULONG SceneNumber;				// Scene numbers will be user friendly numbers 1-n

// Controls how scenes are loaded over existing scenes
enum SceneLoadMethod {
    SLM_LOAD = 1,                       // Load the scene and remove any existing actors (default method)
    SLM_ADD = 2,                        // Add this scene's actors and animations
    SLM_MINUS = 3                       // Remove this scene's actor and animation
};

class Scene : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    CCriticalSection        m_scene_mutex;					// Protect scene objects

    ActorMap			    m_actors;						// "Actors" in this scene
    AnimationReferenceArray m_animations;
    BPMRating               m_bpm_rating;                   // BPM rating for this scene   
    
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
    void setActors( ActorList& actors );
    bool removeActor( UID uid );
    SceneActor* getActor( UID uid );
    ActorPtrArray getActors( void );
    UIDArray getActorUIDs( void );

    inline bool hasActor( UID uid ) {
        return getActor( uid ) != NULL;
    }

    inline size_t getNumActors( void ) const {
        return m_actors.size();
    }

    inline void removeAllActors() {
        m_actors.clear();
    }

    inline SceneNumber getSceneNumber( void ) const {
        return getNumber();
    }
    inline void setSceneNumber( SceneNumber scene_number ) {
        setNumber( scene_number );
    }

    inline bool hasAnimations() const {
        return m_animations.size() > 0;
    }

    inline size_t getNumAnimations() const {
        return m_animations.size();
    }

    inline BPMRating getBPMRating() const {
        return m_bpm_rating;
    }
    inline void setBPMRating( BPMRating rating ) {
        m_bpm_rating = rating;
    }

    AnimationReference* getAnimationByUID( UID animation_uid );
    bool removeAnimationByUID( UID animation_uid );
        
    bool clearAnimations( );
    void addAnimation( AnimationReference& animation );
    void replaceAnimation( size_t animation_index, AnimationReference& animation );

    AnimationReference* getAnimation( size_t animation_index );

    AnimationReferenceArray& animations() {
        return m_animations;
    }
};

typedef std::map<UID,Scene> SceneMap;
typedef std::vector<Scene *> ScenePtrArray;
