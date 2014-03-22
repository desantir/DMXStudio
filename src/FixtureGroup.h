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

/**
    Group of similar fixture types.
*/

#include "IVisitor.h"
#include "Fixture.h"

typedef ULONG GroupNumber;

class FixtureGroup : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    UIDSet	    m_fixtures;

    size_t      m_channels;

    // m_channel_values always contains unmapped fixture channel values. For
    // example, if a fixture maps channel 3 to channel 0, the value for physical
    // channel 3 will be stored in m_channel_values[0].  The value will be 
    // remapped from channel 0 to 3 when added to the DMX packet.

    BYTE		m_channel_values[DMX_PACKET_SIZE];			// Not worth a container for this

public:
    FixtureGroup(void) :
      DObject(),
      m_channels( 0 ) 
    {
        reset_channel_values();
    }
      
    FixtureGroup( UID uid, ULONG group_number, const char * name, const char *description );
    ~FixtureGroup(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    GroupNumber getGroupNumber( ) const {
        return getNumber();
    }
    void setGroupNumber( GroupNumber group_number ) {
        setNumber( group_number );
    }

    void addFixture( UID pfuid ) {
        m_fixtures.insert( pfuid );
    }

    void setFixtures( UIDSet fixtures ) {
        m_fixtures = fixtures;
    }

    bool removeFixture( UID pfuid );
    bool containsFixture( UID pfuid );

    UIDSet getFixtures( ) const {
        return UIDSet( m_fixtures );
    }

    void reset_channel_values( void ) {
        memset( m_channel_values, 0, sizeof(m_channel_values) );
    }

    inline size_t getNumChannelValues() const {
        return m_channels;
    }

    inline BYTE getChannelValue( channel_t channel ) const {
        STUDIO_ASSERT( channel < m_channels, "Channel out of range" );
        return m_channel_values[ channel ];
    }

    inline void setChannelValues( size_t channels, BYTE* channel_values ) {
        m_channels = channels;
        memcpy( m_channel_values, channel_values, channels );
    }

    inline size_t getChannelValues( BYTE * destination ) const {
        memcpy( destination, m_channel_values, m_channels );
        return m_channels;
    }
};

typedef std::map< UID, FixtureGroup > FixtureGroupMap;
typedef std::vector< FixtureGroup * > FixtureGroupPtrArray;