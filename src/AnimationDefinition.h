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
#include "DObject.h"
#include "RGBWA.h"
#include "AnimationSignal.h"
#include "IVisitor.h"
#include "SceneActor.h"

class AnimationTask;

typedef ULONG AnimationNumber;

class AnimationDefinition : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
    bool                m_shared;                           // Shared animation
    UID                 m_reference_fixture;
    AnimationSignal		m_signal;

public:
    AnimationDefinition( ) :
        DObject(),
        m_reference_fixture(0)
    {}

    AnimationDefinition( UID animation_uid ) :
        DObject( animation_uid, 0, NULL, NULL ),
        m_shared( false ),
        m_reference_fixture(0)
    {}

    AnimationDefinition( UID animation_uid, bool shared, UID reference_fixture, AnimationSignal signal ) :
        DObject( animation_uid, 0, NULL, NULL ),
        m_shared( shared ),
        m_reference_fixture(reference_fixture),
        m_signal( signal )
    {}

    virtual ~AnimationDefinition( void ) {}

    virtual AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) = 0;

	virtual AnimationDefinition* clone() = 0;

    void setAnimationNumber( AnimationNumber number ) {
        setNumber( number );
    }

    bool isShared( ) const {
        return m_shared;
    }
    void setShared( bool shared ) {
        m_shared = shared;
    }

    AnimationSignal& signal( ) { return m_signal; }	

    UID getReferenceFixtureId() const {
        return m_reference_fixture;
    }
    void setReferenceFixtureId( UID fixture_id ) {
        m_reference_fixture = fixture_id;
    }

    virtual CString getSynopsis(void) {
        return "";
    }

    virtual bool removePaletteReference( UID palette_id ) {
        return false;
    }

    virtual void accept( IVisitor* visitor) = 0;
    virtual const char* getPrettyName(void) = 0;
    virtual const char* getClassName(void) = 0;
};

typedef std::vector<AnimationDefinition*> AnimationPtrArray;
typedef std::map<UID, AnimationDefinition*> AnimationPtrMap;
typedef std::map<UID, std::unique_ptr<AnimationDefinition>> AnimationMap;