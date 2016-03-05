/* 
Copyright (C) 2011-14 Robert DeSantis
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

typedef ULONG FixtureNumber;					// Fixture numbers will be user friendly numbers 1-n

class Fixture : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    universe_t			m_universe;				// Ignored for now - only a single DMX universe
    channel_t			m_address;				// Base address
    FUID				m_fuid;                 // UID of the fixture type (not this fixture instance)
    CString             m_full_name;            // Full name of fixture (Manufacture Model Location)

    FixtureDefinition*	m_fixture_definition;	// We only need to get this once

    ChannelList	        m_channel_map;			// Channel mappings logical -> physical packet address

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

    void setFixtureNumber( FixtureNumber number )  {
        setNumber( number);
    }
    inline FixtureNumber getFixtureNumber( ) const {
        return getNumber();
    }

    void setUniverseId( universe_t universe ) {
        m_universe = universe;
    }
    inline universe_t getUniverseId( ) const {
        return m_universe;
    }

    void setAddress( channel_t channel ) {
        m_address = channel;
    }
    inline channel_t getAddress( ) const {
        return m_address;
    }

    bool setPhysicalChannels( ChannelList& channel_map );

    // Accessors to handle channel re-mapping
    inline channel_t getChannelAddress( channel_t channel ) {				// Return 1-based actual channel address
        return m_address+mapChannel(channel);
    }

    inline Channel* getChannel( channel_t channel ) {
        return getFixtureDefinition()->getChannel( mapChannel( channel ) );
    }

    inline channel_t mapChannel( channel_t channel ) const {
        return ( m_channel_map.size() > 0 ) ? m_channel_map[channel] : channel;
    }

    // Helper function to avoid consumers needing two pointers
    inline LPCSTR getManufacturer( ) { return getFixtureDefinition()->getManufacturer(); }
    inline LPCSTR getModel( ) { return getFixtureDefinition()->getModel(); }
    inline FixtureType getType() { return getFixtureDefinition()->getType(); }
    inline LPCSTR getTypeName() { return getFixtureDefinition()->getTypeName(); }
    inline bool canPan() { return getFixtureDefinition()->canPan(); }
    inline bool canTilt() { return getFixtureDefinition()->canTilt(); }
    inline bool canMove() { return canPan() || canTilt(); }
    inline bool canWhiteout() { return getFixtureDefinition()->canWhiteout(); }
    inline size_t getNumChannels() { return getFixtureDefinition()->getNumChannels(); }
    inline size_t getNumPixels() { return getFixtureDefinition()->getNumPixels(); }
    inline PixelArray* getPixels() { return getFixtureDefinition()->getPixels(); }
    inline size_t getNumHeads( ) { return getFixtureDefinition()->getNumHeads(); }
    inline bool getHead( UINT head_number, Head& head) { return getFixtureDefinition()->getHead(head_number, head); }

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

    inline FUID getFUID( ) const {
        return m_fuid;
    }

protected:
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

