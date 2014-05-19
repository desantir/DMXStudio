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
    FIXT_PIXEL = 12,
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

class Pixel {
    channel_t   m_red;
    channel_t   m_green;
    channel_t   m_blue;
    channel_t   m_white;
    channel_t   m_amber;

public:
     Pixel() : 
        m_red( INVALID_CHANNEL ), 
        m_green( INVALID_CHANNEL ), 
        m_blue( INVALID_CHANNEL ), 
        m_white( INVALID_CHANNEL ),
        m_amber( INVALID_CHANNEL )
    {}

    inline channel_t red( ) const { return m_red; };
    inline channel_t green( ) const { return m_green; };
    inline channel_t blue( ) const { return m_blue; };
    inline channel_t white( ) const { return m_white; };
    inline channel_t amber( ) const { return m_amber; };

    inline void red( channel_t red ) { m_red = red; };
    inline void green( channel_t green ) { m_green = green; };
    inline void blue( channel_t blue ) { m_blue = blue; };
    inline void white( channel_t white ) { m_white = white; };
    inline void amber( channel_t amber ) { m_amber = amber; };

    inline bool hasRed( ) const { return m_red != INVALID_CHANNEL; };
    inline bool hasGreen( ) const { return m_green != INVALID_CHANNEL; };
    inline bool hasBlue( ) const { return m_blue != INVALID_CHANNEL; };
    inline bool hasWhite( ) const { return m_white != INVALID_CHANNEL; };
    inline bool hasAmber( ) const { return m_amber != INVALID_CHANNEL; };

    inline void reset() {
        m_red=m_green = m_blue = m_white = m_amber = INVALID_CHANNEL;
    }
};

typedef std::vector<Pixel> PixelArray;

struct Head {
    UINT        m_head_number;        // Head number
    channel_t   m_pan;                // Pan channel
    channel_t   m_tilt;               // Tilt channel
    channel_t   m_speed;              // Movement speed channel
    channel_t   m_dimmer;             // Dimmer channel ( may be for entire fixture)

    Head() :
        m_head_number( 0 ),
        m_pan( INVALID_CHANNEL ),
        m_tilt( INVALID_CHANNEL ),
        m_speed( INVALID_CHANNEL ),
        m_dimmer( INVALID_CHANNEL )
    {}

    inline bool isValid() const {
        m_pan != INVALID_CHANNEL || m_tilt != INVALID_CHANNEL;
    }
};

typedef std::map<UINT,Head> HeadMap;

class FixtureDefinition
{
    friend class DefinitionReader;
    friend class DefinitionWriter;

    CString         m_source_filename;      // Track source file in case we need it
    CString         m_author;
    CString         m_version;

    FUID			m_fuid;
    CString	        m_manufacturer;
    CString	        m_model;
    FixtureType		m_type;
    bool			m_can_tilt;
    bool			m_can_pan;
    bool            m_can_whiteout;

    ChannelArray	m_channels;				// Ordered list of channels

    PixelArray      m_pixels;               // Pixels for all devices (non-pixel devices will have maximim of one)
    HeadMap         m_heads;                // Movement heads for this fixture

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

    inline FUID getFUID( ) const {
        return m_fuid;
    }

    inline const char * getManufacturer( ) const {
        return m_manufacturer;
    }

    inline const char * getModel( ) const {
        return m_model;
    }

    inline FixtureType getType() const {
        return m_type;
    }

    inline LPCSTR getTypeName() const {
        return convertFixtureTypeToText( m_type );
    }

    inline bool canPan() const {
        return m_can_pan;
    }

    inline bool canTilt() const {
        return m_can_tilt;
    }

    inline bool canWhiteout() const { 
        return m_can_whiteout;
    }

    inline size_t getNumChannels() const {
        return m_channels.size();
    }

    Channel* getChannel( size_t offset ) {
        if ( offset >= m_channels.size() )
            return NULL;
        return &m_channels[offset];
    }

    inline size_t getNumPixels( ) const {
        return m_pixels.size();
    }

    PixelArray* getPixels() {
        return &m_pixels;
    }

    inline size_t getNumHeads( ) const {
        return m_heads.size();
    }

    bool getHead( UINT head_number, Head& head) {
        HeadMap::iterator it = m_heads.find( head_number );
        if ( it == m_heads.end() )
            return false;
        head = (*it).second;
        return true;
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
    bool findPixel( size_t pixel_index, Pixel &pixel );
    Head findHead( UINT head_number );

    static FixtureType convertTextToFixtureType( LPCSTR text_type );
    static LPCSTR convertFixtureTypeToText( FixtureType type );
    static void addFixtureDefinition( FixtureDefinition* fd );
};

typedef std::vector<FixtureDefinition *> FixtureDefinitionPtrArray;
