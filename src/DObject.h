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

/**
    Base class for user's managed venue objects.  Provides UID, name, number, and description.
*/

#include "stdafx.h"
#include "Act.h"

#define MAX_OBJECT_NUMBER	1000000

class DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    inline void setCreated() {
        m_created = std::chrono::duration_cast<std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch() ).count();
    }

protected:
    UID				m_uid;                          // The UID is guarenteed to be unique _across_ all objects
    ULONG			m_number;						// User selected friendly number
    CString		    m_name;
    CString		    m_description;
    UINT64          m_created;                      // Created tick count
    Acts            m_acts;                         // List of acts this object belongs to

    virtual inline void setNumber( ULONG number ) {
        m_number = number;
    }

public:
    DObject(void) :
        m_uid(NOUID),
        m_number(0)
    {
        setCreated();
    }

    DObject( UID uid, ULONG number, const char* name, const char *description ) :
        m_uid(uid),
        m_number(number)
    {
        setCreated();

        if ( name )
            m_name = name;
        if ( description )
            m_description = description;
    }

    virtual ~DObject(void) 
    {}

    DObject( DObject& other ) {
        m_uid = other.m_uid;
        m_number = other.m_number;
        m_name = other.m_name;
        m_created = other.m_created;
        m_description = other.m_description;
        m_acts = other.m_acts;
    }

    virtual inline UID getUID( ) const {
        return m_uid;
    }
    virtual void setUID( UID uid ) {
        m_uid = uid;
    }

    virtual inline ULONG getNumber( void ) const {
        return m_number;
    }

    virtual const char *getName() const {
        return m_name;
    }
    virtual void setName( const char *name ) {
        m_name = name;
    }

    virtual const char *getDescription( void ) const {
        return m_description;
    }
    virtual void setDescription( const char * description ) {
        m_description = description;
    }

    virtual UINT64 getCreated() const {
        return m_created;
    }

    inline Acts getActs() const {
        return m_acts;
    }
    inline void setActs( Acts& acts ) {
        m_acts = acts;
    }
};

typedef std::vector<DObject> DObjectArray;