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
#include "FixtureGroup.h"

// ----------------------------------------------------------------------------
//
FixtureGroup::FixtureGroup( UID uid, const char * name, const char *description ) :
    DObject( uid, name, description )
{
}

// ----------------------------------------------------------------------------
//
FixtureGroup::~FixtureGroup(void)
{
}

// ----------------------------------------------------------------------------
//
bool FixtureGroup::removeFixture( UID pfuid ) {
    UIDSet::iterator it = m_fixtures.find( pfuid );
    if ( it != m_fixtures.end() ) {
        m_fixtures.erase( it );
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
//
bool FixtureGroup::containsFixture( UID pfuid ) {
    UIDSet::iterator it = m_fixtures.find( pfuid );
    return ( it != m_fixtures.end() );
}
