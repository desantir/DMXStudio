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
#include "FixtureDefinition.h"
#include "DefinitionReader.h"
#include "DefinitionWriter.h"

bool caseInsensitiveCompare( LPCSTR left, LPCSTR right ) {
    return _stricmp( left, right ) < 0;
}

// Base fixture definitions
FixtureDefinitionMap FixtureDefinition::FixtureDefinitions;
FixtureDefinitionHierarchy FixtureDefinition::FixtureDefinitionHierarchy( caseInsensitiveCompare );

typedef std::map<FixtureType,CString> FixtureTypeToNameMap;

static FixtureTypeToNameMap fixtureTypeToNameMap;

static int populateFixtureTypes() {
    fixtureTypeToNameMap[FIXT_UNKNOWN] = "Unknown";
    fixtureTypeToNameMap[FIXT_PAR] = "Par";
    fixtureTypeToNameMap[FIXT_SPOT] = "Spot";
    fixtureTypeToNameMap[FIXT_STROBE] = "Strobe";
    fixtureTypeToNameMap[FIXT_LASER] = "Laser";
    fixtureTypeToNameMap[FIXT_FOG] = "Fog";
    fixtureTypeToNameMap[FIXT_EFFECT] = "Effect";		
    fixtureTypeToNameMap[FIXT_WASH] = "Wash";
    fixtureTypeToNameMap[FIXT_DIMMER] = "Dimmer";
    fixtureTypeToNameMap[FIXT_DOTS] = "Dots";
    fixtureTypeToNameMap[FIXT_H2O] = "H2O";
    fixtureTypeToNameMap[FIXT_SCANNER] = "Scanner";
    fixtureTypeToNameMap[FIXT_PIXEL] = "Pixel";

    return 0;
}

static int static_kludge = populateFixtureTypes();

// ----------------------------------------------------------------------------
//
FixtureDefinition::FixtureDefinition( FUID fuid, const char *manufacturer, const char *model, FixtureType type ) :
    m_fuid( fuid ),
    m_manufacturer( manufacturer ),
    m_model( model ),
    m_type( type ),
    m_can_tilt( false ),
    m_can_pan( false ),
    m_can_whiteout( false )
{
}

// ----------------------------------------------------------------------------
//
FixtureDefinition::~FixtureDefinition(void)
{
}

// ----------------------------------------------------------------------------
//
void FixtureDefinition::chooseCapabilities()
{
    // Set fixture capabilities (we do this once since definitions are immutable)
    bool red=false, blue=false, green=false, white=false, dimmer=false;

    for ( ChannelArray::iterator it=m_channels.begin(); it != m_channels.end(); ++it ) {
        Channel& channel = *it;

        if ( channel.isDimmer() )
            dimmer = true;

        switch ( channel.getType() ) {
            case CHNLT_TILT:    m_can_tilt = true;     break;
            case CHNLT_PAN:	    m_can_pan = true;      break;
            case CHNLT_RED:     red = true;            break;
            case CHNLT_BLUE:    blue = true;           break;
            case CHNLT_GREEN:   green = true;          break;
            case CHNLT_WHITE:   white = true;          break;
        }
    }

    // Load pixels for pixel fixtures - insures consecutive pixels 1-n
    if ( getType() == FIXT_PIXEL ) {
        for ( size_t index=1; true; index++ ) {
            Pixel pixel;
            if ( findPixel( index, pixel ) )
                m_pixels.push_back( pixel );
            else
                break;
        }
    }
    else {
        // All other fixtures, get the main RGB[WA] channels as the "pixel". Currently ignoring
        // situations such as non-pixel fixtures with multiple colors or multiple single colors

        Pixel pixel;
        if ( findPixel( 0, pixel ) )
            m_pixels.push_back( pixel );
    }

    // Set whiteout support
    switch ( getType() ) {
        case FIXT_DIMMER:
        case FIXT_STROBE:
            m_can_whiteout = (dimmer == true);
            break;

        case FIXT_SCANNER:
        case FIXT_PIXEL:
        case FIXT_PAR:
        case FIXT_SPOT:
        case FIXT_WASH:
            m_can_whiteout = (white || (red && blue && green));
            break;
    }
}

// ----------------------------------------------------------------------------
//
bool FixtureDefinition::findPixel( size_t pixel_index, Pixel &pixel ) {
    bool found = false;

    pixel.reset();

    for ( Channel& channel : m_channels ) {
        if ( channel.getPixelIndex() == pixel_index ) {
            switch ( channel.getType() ) {
                case CHNLT_AMBER:   pixel.amber( channel.getOffset() );         found = true; break;
                case CHNLT_RED:     pixel.red( channel.getOffset() );           found = true; break;
                case CHNLT_BLUE:    pixel.blue( channel.getOffset() );          found = true; break;
                case CHNLT_GREEN:   pixel.green( channel.getOffset() );         found = true; break;
                case CHNLT_WHITE:   pixel.white( channel.getOffset() );         found = true; break;
            }
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
//
FixtureDefinition* FixtureDefinition::lookupFixture( FUID fuid ) {

    FixtureDefinitionMap::iterator it = FixtureDefinitions.find( fuid );
    if ( it == FixtureDefinitions.end() )
        return NULL;

    return &it->second;				// Not sure about this - I hope the iterator is not a copy ...
}

// ----------------------------------------------------------------------------
//
FixtureType FixtureDefinition::convertTextToFixtureType( LPCSTR text_type )
{
    // Try to convert text representation
    for ( FixtureTypeToNameMap::iterator it=fixtureTypeToNameMap.begin();
          it != fixtureTypeToNameMap.end();
          it++ )
    {
        if ( _stricmp( text_type, it->second ) == 0 )
            return it->first;
    }

    // Maybe a numeric representation
    FixtureType type = FIXT_UNKNOWN;
    if ( sscanf_s( text_type, "%d", &type ) != 1 )
        type = FIXT_UNKNOWN;

    STUDIO_ASSERT( type >= FIXT_UNKNOWN && type < FIXT_NUM_TYPES, "Invalid fixture type %d", type );

    return type;
}

// ----------------------------------------------------------------------------
//
LPCSTR FixtureDefinition::convertFixtureTypeToText( FixtureType type )
{
    FixtureTypeToNameMap::iterator it=fixtureTypeToNameMap.find( type );
    STUDIO_ASSERT( it != fixtureTypeToNameMap.end(), "Unknown fixture type %d", type );
    return it->second;
}

// ----------------------------------------------------------------------------
//
void FixtureDefinition::addFixtureDefinition( FixtureDefinition *fd )
{
    FixtureDefinitionMap::iterator it_fd = FixtureDefinition::FixtureDefinitions.find( fd->getFUID() );
    STUDIO_ASSERT( it_fd == FixtureDefinition::FixtureDefinitions.end(),
        "%s %s personality %d already defined as %s %s personality %d (FUID %lu)", 
                fd->getManufacturer(), fd->getModel(), fd->getNumChannels(), 
                it_fd->second.getManufacturer(), it_fd->second.getModel(), 
                it_fd->second.getNumChannels(), fd->getFUID() );

    FixtureDefinition::FixtureDefinitions[ fd->getFUID() ] = *fd;
    
    // Manufacturer
        // Model
            // Personality (channels)
                // FUID

    // Find or add the manufacturer
    FixtureDefinitionHierarchy::iterator it = FixtureDefinition::FixtureDefinitionHierarchy.find( fd->getManufacturer() );
    if ( it == FixtureDefinition::FixtureDefinitionHierarchy.end() ) {          // New manufacturer
        FixtureDefinition::FixtureDefinitionHierarchy[ fd->getManufacturer() ] = FixtureModelToPersonalityMap( caseInsensitiveCompare );
        it = FixtureDefinition::FixtureDefinitionHierarchy.find( fd->getManufacturer() );
    }

    FixtureModelToPersonalityMap& m2p = it->second;

    // Find or add the fixture model
    FixtureModelToPersonalityMap::iterator it2 = m2p.find( fd->getModel() );
    if ( it2 == m2p.end() ) {                                                   // New model
        m2p[ fd->getModel() ] = FixturePersonalityToFUID();
        it2 = m2p.find( fd->getModel() );
    }

    FixturePersonalityToFUID& p2f = it2->second;

    // Add new personality
    FixturePersonalityToFUID::iterator it3 = p2f.find( fd->getNumChannels() );
    STUDIO_ASSERT( it3 == p2f.end(), "%s %s personality %d already defined", 
                    fd->getManufacturer(), fd->getModel(), fd->getNumChannels() );

    p2f[ fd->getNumChannels() ] = fd->getFUID();
}

// ----------------------------------------------------------------------------
//
LPCSTRArray FixtureDefinition::getUniqueManufacturers()
{
    FixtureDefinitionHierarchy::iterator it = FixtureDefinition::FixtureDefinitionHierarchy.begin();

    LPCSTRArray manufactures;
    for ( ; it != FixtureDefinition::FixtureDefinitionHierarchy.end(); ++it )
        manufactures.push_back( it->first );
    return manufactures;
}

// ----------------------------------------------------------------------------
//
LPCSTRArray FixtureDefinition::getUniqueModels( LPCSTR manufacturer )
{
    FixtureDefinitionHierarchy::iterator it = FixtureDefinition::FixtureDefinitionHierarchy.find( manufacturer );
    if ( it == FixtureDefinition::FixtureDefinitionHierarchy.end() )
        return LPCSTRArray();

    FixtureModelToPersonalityMap& m2p = it->second;
    
    LPCSTRArray models;
    for ( FixtureModelToPersonalityMap::iterator it2=m2p.begin(); it2 != m2p.end(); it2++ )
        models.push_back( it2->first );
    return models;
}

// ----------------------------------------------------------------------------
//
FixturePersonalityToFUID FixtureDefinition::getModelPersonalities( LPCSTR manufacturer, LPCSTR model )
{
    FixtureDefinitionHierarchy::iterator it = FixtureDefinition::FixtureDefinitionHierarchy.find( manufacturer );
    if ( it == FixtureDefinition::FixtureDefinitionHierarchy.end() )
        return FixturePersonalityToFUID();

    FixtureModelToPersonalityMap& m2p = it->second;

    FixtureModelToPersonalityMap::iterator it2 = m2p.find( model );
    if ( it2 == m2p.end() )
        return FixturePersonalityToFUID();

    return it2->second;
}

// ----------------------------------------------------------------------------
//
void FixtureDefinition::readFixtureDefinitions()
{
    DefinitionReader reader;
    reader.readFixtureDefinitions();
}

// ----------------------------------------------------------------------------
// Generate fixture's FUID by hashing name, manufacturer and channels
// Uses FNV hash from http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
//
FUID FixtureDefinition::generateFUID( void ) {
    CString key;
    
    // Key is type-manufacturer-fixture name-channels
    key.Format( "%d-%s-%s-%d", m_type, m_manufacturer, m_model, m_channels.size() );

    DWORD hash = 2166136261;                // offset_basis

    for ( LPCSTR p=key; *p; p++ ) {
        hash ^= *p;
        hash *= 16777619;                   // FNV_prime
    }

    DMXStudio::log( "Fixture key '%s' assigned hash %lu", key, hash );

    return (FUID)hash;
}

