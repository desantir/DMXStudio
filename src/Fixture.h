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
    Class represents a single fixture in a universe.
*/

#include "DMXStudio.h"
#include "FixtureDefinition.h"
#include "DObject.h"
#include "IVisitor.h"

typedef std::vector<channel_t> PhysicalChannels;

typedef ULONG FixtureNumber;					// Fixture numbers will be user friendly numbers 1-n

class Fixture : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    universe_t			m_universe;				// Ignored for now - only a single DMX universe
    channel_t			m_address;				// Base address
    FUID				m_fuid;
    CString             m_full_name;            // Full name of fixture (Manufacture Model Location)

    FixtureDefinition*	m_fixture_definition;	// We only need to get this once

    PhysicalChannels	m_channel_map;			// Channel mappings logical -> physical packet address

public:
    Fixture() :
        m_universe(0),
        m_address(0),
        m_fuid(0),
        m_fixture_definition(NULL),
        DObject() 
    {}

    Fixture( UID uid, FixtureNumber fixture_number, universe_t universe, 
                     channel_t base_address, FUID fuid, 
                     const char *name = NULL, const char *description = NULL );
    ~Fixture(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    void setUID( UID uid ) {
        m_uid = uid;
    }
    inline UID getUID() const {
        return m_uid;
    }

    FixtureNumber getFixtureNumber( ) const {
        return getNumber();
    }

    universe_t getUniverse( ) const {
        return m_universe;;
    }

    void setAddress( channel_t channel ) {
        m_address = channel;
    }
    inline channel_t getAddress( ) const {
        return m_address;
    }

    bool setPhysicalChannels( PhysicalChannels& channel_map );

    // Accessors to handle channel re-mapping
    inline channel_t getChannelAddress( channel_t channel ) {				// Return 1-based actual channel address
        channel = ( m_channel_map.size() > 0 ) ? m_channel_map[channel] : channel;
        return m_address+channel;
    }

    inline Channel* getChannel( channel_t channel ) {
        channel = ( m_channel_map.size() > 0 ) ? m_channel_map[channel] : channel;
        return getFixtureDefinition()->getChannel( channel );
    }

    // Helper function to avoid consumers needing two pointers
    inline const char * getManufacturer( ) { return getFixtureDefinition()->getManufacturer(); }
    inline const char * getModel( ) { return getFixtureDefinition()->getModel(); }
    inline FixtureType getType() { return getFixtureDefinition()->getType(); }
    inline bool canPan() { return getFixtureDefinition()->canPan(); }
    inline bool canTilt() { return getFixtureDefinition()->canTilt(); }
    inline bool canMove() { return canPan() || canTilt(); }
    inline bool canWhiteout() { return getFixtureDefinition()->canWhiteout(); }
    inline size_t getNumChannels() { return getFixtureDefinition()->getNumChannels(); }

    inline channel_t getChannelByType( ChannelType channel_type ) {
        for ( channel_t channel=0; channel < getNumChannels(); channel++ ) {
            if ( getChannel( channel )->getType() == channel_type )
                return channel;
        }
        return INVALID_CHANNEL;
    }

    void print() {
        printf( "PID %ld: universe=%ld address=%ld fuid=%d location_name=%s description=%s\n", 
            m_uid, m_universe, m_address, m_fuid, (LPCTSTR)m_name, (LPCTSTR)m_description );
    }

    LPCSTR getFullName() {
        if ( m_full_name.GetLength() == 0 )
            m_full_name.Format( "%s %s (%s)", getManufacturer(), getModel(), getName() );
        return m_full_name;
    }

    void setName( const char *name ) {
        DObject::setName( name );
        m_full_name.Empty();
    }

protected:
    FUID getFUID( ) const {
        return m_fuid;
    }

    FixtureDefinition * getFixtureDefinition( ) {
        if ( !m_fixture_definition ) {
            m_fixture_definition = FixtureDefinition::lookupFixture( m_fuid );
            STUDIO_ASSERT( m_fixture_definition != NULL, "Missing fixture definition for %lu", m_fuid);
        }
        return m_fixture_definition;
    }
};

typedef std::map<UID, Fixture> FixtureMap;
typedef std::vector<Fixture*> FixturePtrArray;

