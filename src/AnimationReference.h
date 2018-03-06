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
#include "IVisitor.h"
#include "DObject.h"

class AnimationReference
{
    friend class VenueWriter;
    friend class VenueReader;

    UID         m_animation_uid;
    UIDArray	m_actors;

public:
    AnimationReference() {}
    ~AnimationReference() {}

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    AnimationReference( UID animation_uid, UIDArray actor_uids ) :
        m_animation_uid( animation_uid ),
        m_actors( actor_uids )
    {}

    inline UID getUID() const {
        return m_animation_uid;
    } 
    inline void setUID( UID animation_uid ) {
        m_animation_uid = animation_uid;
    }

    inline UIDArray getActors( ) const {
        return UIDArray( m_actors );
    }
    inline void setActors( UIDArray& actors ) {
        m_actors.assign( actors.begin(), actors.end() );        
    }

    inline void addActor( UID actor_uid ) {
        if ( !hasActor( actor_uid ) )
            m_actors.push_back( actor_uid );      
    }

    inline void removeActor( UID actor_uid ) {
        for ( UIDArray::iterator it=m_actors.begin(); it != m_actors.end(); )
            if ( *it == actor_uid )
                it = m_actors.erase( it );
            else
                it++;
    }

    inline bool hasActor( UID actor_uid ) {
        return std::find<UIDArray::iterator>( m_actors.begin(), m_actors.end(), actor_uid ) != m_actors.end();
    }
};

typedef std::vector<AnimationReference> AnimationReferenceArray;
