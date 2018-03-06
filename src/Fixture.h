/* 
Copyright (C) 2011-17 Robert DeSantis
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

#include "stdafx.h"
#include "FixtureDefinition.h"
#include "DObject.h"
#include "IVisitor.h"

typedef ULONG FixtureNumber;					// Fixture numbers will be user friendly numbers 1-n

struct GridPosition {
	long			m_x;						// X position
	long			m_y;						// Y position

	GridPosition() :
		m_x(0), m_y(0)
	{}

	GridPosition(UINT32 x, UINT32 y) :
		m_x(x), m_y(y)
	{}
};

class Fixture : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    universe_t			m_universe;				// Ignored for now - only a single DMX universe
	channel_address		m_address;				// Base address
    FUID				m_fuid;                 // UID of the fixture type (not this fixture instance)
    CString             m_full_name;            // Full name of fixture (Manufacture Model Location)
    bool                m_allow_master_dimming; // Dim this fixture with master dimmer (fixture must be capable)
    bool                m_allow_whiteout;       // Whiteout this fixture (fixture must be capable)
	GridPosition		m_grid_position;		// Postion in UI grid
	
	channel_address		m_realBaseAddress;		// Computed base address for channels based on universe

    FixtureDefinition*	m_fixture_definition;	// We only need to get this once

public:
    Fixture() :
        m_universe(0),
        m_address(0),
        m_fuid(0),
        m_fixture_definition(NULL),
        m_allow_master_dimming( true ),
        m_allow_whiteout( true ),
		m_realBaseAddress( 0 ),
        DObject() 
    {}

    Fixture( UID uid, FixtureNumber fixture_number, universe_t universe, 
					 channel_address base_address, FUID fuid, 
                     const char *name = NULL, const char *description = NULL,
                     bool allow_master_dimming = true, bool allow_whiteout = true );
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
		computeChannelBaseRealAddress();
    }
    inline universe_t getUniverseId( ) const {
        return m_universe;
    }

    void setAddress( channel_address channel ) {
        m_address = channel;
		computeChannelBaseRealAddress();
    }
    inline channel_address getAddress( ) const {
        return m_address;
    }

	// Return the multi-universe packet offset
	inline channel_address getMultiPacketAddress( channel_address channel ) const {
		return m_realBaseAddress + channel - 1;
	}

    void setAllowWhiteout( bool allow_whiteout ) {
        m_allow_whiteout = allow_whiteout;
    }
    inline bool getAllowWhiteout( ) const {
        return m_allow_whiteout;
    }

    void setAllowMasterDimming( bool allow_dimming ) {
        m_allow_master_dimming = allow_dimming;
    }
    inline bool getAllowMasterDimming( ) const {
        return m_allow_master_dimming;
    }

	inline GridPosition getGridPosition( ) const {
		return m_grid_position;
	}
	inline void setGridPosition(GridPosition& pos ) {
		m_grid_position = pos;
	}

    // Accessors to handle channel re-mapping
    inline channel_address getChannelAddress( channel_address channel ) const {	// Return 1-based actual channel address
        return m_address+channel;
    }

    inline Channel* getChannel( size_t channel ) {
        return getFixtureDefinition()->getChannel( channel );
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
    inline bool canDimmerStrobe() { return getFixtureDefinition()->canDimmerStrobe(); }
    inline bool hasDimmer() { return getFixtureDefinition()->hasDimmer(); }
    inline size_t getNumChannels() { return getFixtureDefinition()->getNumChannels(); }
    inline size_t getNumPixels() { return getFixtureDefinition()->getNumPixels(); }
    inline PixelArray* getPixels() { return getFixtureDefinition()->getPixels(); }
    inline size_t getNumHeads( ) { return getFixtureDefinition()->getNumHeads(); }
    inline bool getHead( UINT head_number, Head& head) { return getFixtureDefinition()->getHead(head_number, head); }

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
    inline void setFUID( FUID fuid ) {
        m_fuid = fuid;
    }

    FixtureDefinition * getFixtureDefinition( ) {
        if ( !m_fixture_definition ) {
            m_fixture_definition = FixtureDefinition::lookupFixture( m_fuid );
            STUDIO_ASSERT( m_fixture_definition != NULL, "Missing fixture definition %lu for id=%lu name=%s", m_fuid, getUID(), getName() );
        }
        return m_fixture_definition;
    }

private:
	inline void computeChannelBaseRealAddress( ) {
		m_realBaseAddress = ((getUniverseId()-1) * DMX_PACKET_SIZE) + m_address;
	}
};

typedef std::map<UID, Fixture> FixtureMap;
typedef std::vector<Fixture*> FixturePtrArray;

