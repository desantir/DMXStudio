/* 
Copyright (C) 2016 Robert DeSantis
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

#include "DObject.h"
#include "Channel.h"
#include "Fixture.h"

typedef ULONG PaletteNumber;

enum EntryAddressing {              // Palette values addressed by channel number or channel type
    BY_CHANNEL = 1,
    BY_TYPE
};

#define SYSTEM_PALETTE_GLOBAL_COLORS        10000
#define SYSTEM_PALETTE_PREDEFINED_COLORS    10001
#define SYSTEM_PALETTE_RAINBOW_COLORS       10002
#define SYSTEM_PALETTE_VIDEO_COLORS         10003
#define SYSTEM_PALETTE_USER_1               10004
#define SYSTEM_PALETTE_USER_2               10005
#define SYSTEM_PALETTE_USER_3               10006
#define SYSTEM_PALETTE_USER_4               10007

typedef std::unordered_map< unsigned, BYTE > PaletteValues;

class PaletteEntry {
    friend class VenueWriter;
    friend class VenueReader;

    UID                 m_target_uid;
    EntryAddressing     m_addressing;
    PaletteValues       m_values;

public:
    PaletteEntry( UID target_uid=0L, EntryAddressing addressing=BY_CHANNEL ) :
        m_target_uid( target_uid ),
        m_addressing( addressing )
    {}

    ~PaletteEntry() {}

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    inline bool hasValues() const {
        return m_values.size() > 0;
    }

    inline EntryAddressing getAddressing() const {
        return m_addressing;
    }

    inline void setAddressing( EntryAddressing addressing ) {
        m_addressing = addressing;
    }

    inline PaletteValues& getValues() {
        return m_values;
    }

    inline BYTE getValue( unsigned channel ) {
        PaletteValues::iterator it = m_values.find( channel );
        return it == m_values.end() ? 0 : it->second;
    }

    inline UID getTargetUID() const {
        return m_target_uid;
    }

    inline void setTargetUID( UID target_uid ) {
        m_target_uid = target_uid;
    }

    inline void addValue( unsigned channel, BYTE value ) {
        m_values[channel] = value;
    }

    inline void clear() {
        m_values.clear();
    }
};

typedef std::map< UID, PaletteEntry > PaletteEntryMap;

enum PaletteType {                    // Metadata describing entry type (optional for future use)
    PT_UNSPECIFIED = 0,
    PT_LOCATION = 1,
    PT_COLOR = 2,
    PT_DIMMER = 3,
    PT_STROBE = 4,
    PT_UNUSED = 5,                   // UNUSED!!!!
    PT_GOBO = 6,
	PT_FIXTURE_PRESET = 7,
	PT_COLOR_PALETTE = 8             // Palette of color values
};

struct WeightedColor {
    RGBWA   m_color;
    double  m_weight;

    WeightedColor( RGBWA& color, double weight ) :
        m_color( color ),
        m_weight( weight )
    {}

    inline RGBWA& getColor() {
        return m_color;
    }

    inline double getWeight() {
        return m_weight;
    }
};

typedef std::vector<WeightedColor> WeightedColorArray;

class Palette : public DObject {
    friend class VenueWriter;
    friend class VenueReader;

    PaletteType          m_type;
    RGBWAArray           m_palette_colors;	
    ColorWeights         m_palette_weights;

    PaletteEntryMap      m_fixture_map;
    PaletteEntryMap      m_fixture_definition_map;
    PaletteEntry         m_global;

    WeightedColorArray   m_weighted_colors;

public:
    Palette() :
        m_type( PT_UNSPECIFIED )
    {}

    Palette( UID uid, ULONG number, LPCSTR name, RGBWAArray& colors ) :
        DObject( uid, number, name, "" ),
        m_type( PT_COLOR_PALETTE ),
        m_palette_colors( colors )
    {}

    Palette( UID uid, ULONG number, LPCSTR name, RGBWAArray& colors, ColorWeights& palette_weights ) :
        DObject( uid, number, name, "" ),
        m_type( PT_COLOR_PALETTE ),
        m_palette_colors( colors ),
        m_palette_weights( palette_weights )
    {
    }
    
    ~Palette() {}

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    inline PaletteType getType() const {
        return m_type;
    }
    inline void setType( PaletteType type ) {
        m_type = type;
    }

    inline RGBWAArray& getPaletteColors( ) {
        return m_palette_colors; 
    }
    inline void setPaletteColors(RGBWAArray& palette_colors) {
        m_palette_colors = palette_colors;
        m_weighted_colors.clear();
    }

    inline ColorWeights& getPaletteWeights( ) {
        return m_palette_weights; 
    }
    inline void setPaletteWeights(ColorWeights& palette_weights) {
        m_palette_weights = palette_weights;
        m_weighted_colors.clear();
    }

    WeightedColorArray& getWeightedColors();

    PaletteNumber getPaletteNumber( ) const {
        return getNumber();
    }
    void setPaletteNumber( PaletteNumber palette_number ) {
        setNumber( palette_number );
    }

    inline PaletteEntryMap& getFixtureMap() {
        return m_fixture_map;
    }
    inline void setFixtureMap( PaletteEntryMap& map ) {
        m_fixture_map = map;
    }

    inline PaletteEntryMap& getFixtureDefinitionMap() {
        return m_fixture_definition_map;
    }
    inline void setFixtureDefinitionMap( PaletteEntryMap& map ) {
        m_fixture_definition_map = map;
    }

    inline PaletteEntry* getFixtureEntry( UID fixture_id ) {
        PaletteEntryMap::iterator it = m_fixture_map.find( fixture_id );     
        return ( it == m_fixture_map.end() ) ? NULL : &it->second;
    }

    inline PaletteEntry* getFixtureDefinitionEntry( UID definition_id ) {
        PaletteEntryMap::iterator it = m_fixture_definition_map.find( definition_id );     
        return ( it == m_fixture_definition_map.end() ) ? NULL : &it->second;
    }

    inline PaletteEntry* getGlobalEntry() {
        return &m_global;
    }

    inline void setGlobalEntry( PaletteEntry& entry ) {
        m_global = entry;
    }

    PaletteEntry* getEntry( Fixture* pf );

    bool removeFixture( UID fixture_id ) {
        return m_fixture_map.erase( fixture_id ) > 0;
    }

    bool removeFixtureDefinition( UID fixture_definition_id ) {
        return m_fixture_definition_map.erase( fixture_definition_id ) > 0;
    }
};

typedef std::map< UID, Palette > PaletteMap;
typedef std::vector<Palette*> PalettePtrArray;
