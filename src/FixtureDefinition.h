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

#include "DMXStudio.h"
#include "channel.h"
#include "IDefinitionVisitor.h"

typedef enum fixture_type {
    FIXT_UNKNOWN = 0,
    FIXT_PAR = 1,
    FIXT_SPOT = 2,
    FIXT_STROBE = 3,
    FIXT_LASER = 4,
    FIXT_FOG = 5,
    FIXT_EFFECT = 6,			// EFX
    FIXT_WASH = 7,
    FIXT_DIMMER = 8,
    FIXT_DOTS = 9,
    FIXT_H2O = 10,
    FIXT_SCANNER = 11,
    FIXT_NUM_TYPES
} FixtureType;

typedef UID FUID;						// Fixture unique ID

typedef std::vector<Channel> ChannelArray;

class FixtureDefinition;

typedef std::map<FUID,FixtureDefinition> FixtureDefinitionMap;

// Fixture "database" organized as hierarchy of Manufacturer -> Model -> Personality (# channels) -> FUID
typedef bool(*CaseInsensitiveCStringComparitor)(LPCSTR, LPCSTR);

typedef std::map<UINT,FUID> FixturePersonalityToFUID;
typedef std::map<CString,FixturePersonalityToFUID, CaseInsensitiveCStringComparitor> FixtureModelToPersonalityMap;
typedef std::map<CString, FixtureModelToPersonalityMap, CaseInsensitiveCStringComparitor> FixtureDefinitionHierarchy;

class FixtureDefinition
{
    friend class DefinitionReader;
    friend class DefinitionWriter;

    CString         m_source_filename;      // Track source file in case we need it

    FUID			m_fuid;
    CString	        m_manufacturer;
    CString	        m_model;
    FixtureType		m_type;
    bool			m_can_tilt;
    bool			m_can_pan;
    bool            m_can_whiteout;

    ChannelArray	m_channels;				// Ordered list of channels

public:
    static void readFixtureDefinitions();
    static FixtureDefinition* lookupFixture( FUID fuid );
    static FixtureDefinitionMap FixtureDefinitions;
    static FixtureDefinitionHierarchy FixtureDefinition::FixtureDefinitionHierarchy;

    static LPCSTRArray getUniqueManufacturers();
    static LPCSTRArray getUniqueModels( LPCSTR manufacturer );
    static FixturePersonalityToFUID getModelPersonalities( LPCSTR manufacturer, LPCSTR model );

    FixtureDefinition( FUID fuid=0, const char *manufacturer="", const char *model="", 
                       FixtureType type=FIXT_UNKNOWN );

    virtual ~FixtureDefinition(void);

    void accept( IDefinitionVisitor* visitor) {
        visitor->visit(this);
    }

    void setSourceFile( LPCSTR source_filename ) {
        m_source_filename = source_filename;
    }

    FUID getFUID( ) const {
        return m_fuid;
    }

    const char * getManufacturer( ) const {
        return m_manufacturer;
    }

    const char * getModel( ) const {
        return m_model;
    }

    FixtureType getType() const {
        return m_type;
    }

    LPCSTR getTypeName() const {
        return convertFixtureTypeToText( m_type );
    }

    bool canPan() const {
        return m_can_pan;
    }

    bool canTilt() const {
        return m_can_tilt;
    }

    bool canWhiteout() const { 
        return m_can_whiteout;
    }

    size_t getNumChannels() const {
        return m_channels.size();
    }

    Channel* getChannel( size_t offset ) {
        if ( offset >= m_channels.size() )
            return NULL;
        return &m_channels[offset];
    }

    void print() {
        printf( "FUID %ld: %s %s type=%d tilt=%d pan=%d\n", 
            m_fuid, (const char*)m_manufacturer, (const char*)m_model, m_type, m_can_tilt, m_can_pan );
        
        ChannelArray::iterator it;

        for ( it=m_channels.begin(); it != m_channels.end(); ++it ) {
            Channel& ch = *it;
            printf ( "Channel %d: %s type=%d\n", ch.getOffset(), ch.getName(), ch.getType() );
        }
    }

private:
    void chooseCapabilities( void );
    FUID generateFUID( void );

    static FixtureType convertTextToFixtureType( LPCSTR text_type );
    static LPCSTR convertFixtureTypeToText( FixtureType type );
    static void addFixtureDefinition( FixtureDefinition* fd );
};

