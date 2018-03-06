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

#include "HttpRestServices.h"
#include "Venue.h"

void paletteToJson( Venue* venue, JsonBuilder& json, Palette* palette );
void paletteEntryToJson( JsonBuilder& json, LPCSTR object_name, PaletteEntry* entry );
PaletteEntry parsePaletteEntry( JsonNode* entry_node );

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;
    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    Palette palette;
    if ( !venue->getPalette( uid, palette ) )
        throw RestServiceException( "Invalid palette UID" );

    JsonBuilder json( response );
    json.startArray();
    paletteToJson( venue, json, &palette );
    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_palettes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    DObjectArray palettes = venue->getPaletteSummary();
    std::sort( palettes.begin(), palettes.end(), CompareObjectNumber2 );

    JsonBuilder json( response );
    json.startArray();
    for ( DObject& summary : palettes ) {
        Palette palette;
        if ( venue->getPalette( summary.getUID(), palette ) )
            paletteToJson( venue, json, &palette );
    }
    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::delete_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID palette_id;

    if ( sscanf_s( data, "%lu", &palette_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !venue->deletePalette( palette_id ) )
        throw RestServiceException( "Invalid palette UID" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode )
{
    SimpleJsonParser parser;
    UID palette_id;
    CString name, description;
    PaletteNumber number;
    PaletteType type;
    PaletteEntryMap fixture_map;
    PaletteEntryMap fixture_definition_map;
    PaletteEntry global;
    RGBWAArray palette_colors;

    try {
        parser.parse( data );

        palette_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        type = (PaletteType)parser.get<UINT>( "type" );
        number = parser.get<ULONG>( "number" );
        global = parsePaletteEntry( parser.getObject( "default_palette" ) );
        palette_colors = parser.getArrayAsVector<RGBWA>( "palette_colors" );

        for ( JsonNode* node : parser.getObjects( "fixture_palettes" ) ) {
            PaletteEntry entry = parsePaletteEntry( node );
            fixture_map.emplace( entry.getTargetUID(), entry );
        }

        for ( JsonNode* node : parser.getObjects( "definition_palettes" ) ) {
            PaletteEntry entry = parsePaletteEntry( node );
            fixture_definition_map.emplace( entry.getTargetUID(), entry );
        }
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    Palette palette;

    if ( palette_id != 0 ) {
        if ( !venue->getPalette( palette_id, palette ) )
            throw RestServiceException( "Invalid palette UID" );
    }

    // Make sure number is unique
    if ( mode != UPDATE || (palette_id != 0 && number != palette.getNumber()) ) {
        if ( venue->getPalette<PaletteNumber>( number, palette ) ) 
            throw RestServiceException( "Palette number must be unique" );
    }

    palette.setName( name );
    palette.setPaletteNumber( number );
    palette.setDescription( description );
    palette.setType( type );
    palette.setPaletteColors( palette_colors );
    palette.setFixtureMap( fixture_map );
    palette.setFixtureDefinitionMap( fixture_definition_map );
    palette.setGlobalEntry( global );

    switch ( mode ) {
        case NEW:
        case COPY: {
            palette.setUID( NOUID );

            venue->addPalette( palette );
            break;
        }

        case UPDATE:
            venue->updatePalette( palette );
            break;
    }
}

// ----------------------------------------------------------------------------
//
void paletteToJson( Venue* venue, JsonBuilder& json, Palette* palette )
{
    json.startObject();
    json.add( "id", palette->getUID() );
    json.add( "number", palette->getPaletteNumber() );
    json.add( "name", palette->getName() );
    json.add( "description", palette->getDescription() );
    json.add( "created", palette->getCreated() );
    json.add( "type", palette->getType() );
    json.addArray<RGBWAArray>( "palette_colors", palette->getPaletteColors() );
    json.addArray<ColorWeights>( "palette_weights", palette->getPaletteWeights() );

    json.startArray( "fixture_palettes" );
    for ( PaletteEntryMap::value_type& entry : palette->getFixtureMap() )
        paletteEntryToJson( json,  NULL, &entry.second );
    json.endArray( "fixture_palettes" );

    json.startArray( "definition_palettes" );
    for ( PaletteEntryMap::value_type& entry : palette->getFixtureDefinitionMap() )
        paletteEntryToJson( json, NULL, &entry.second );
    json.endArray( "definition_palettes" );

    paletteEntryToJson( json, "default_palette", palette->getGlobalEntry() );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void paletteEntryToJson( JsonBuilder& json, LPCSTR object_name, PaletteEntry*entry ) 
{
    json.startObject( object_name );
    json.add( "target_uid", entry->getTargetUID() );
    json.add( "addressing", entry->getAddressing() );

    json.startArray( "channel_values" );
    for ( PaletteValues::iterator it=entry->getValues().begin(); it != entry->getValues().end(); it++ ) {
        json.startObject();
        json.add( "id", it->first );
        json.add( "value", (unsigned)it->second );
        json.endObject();
    }
    json.endArray( "channel_values" );

    json.endObject( object_name );
}

// ----------------------------------------------------------------------------
//
PaletteEntry parsePaletteEntry( JsonNode* entry_node ) {
    PaletteEntry entry;

    entry.setTargetUID( entry_node->get<UID>( "target_uid" ) );
    entry.setAddressing( (EntryAddressing)entry_node->get<UINT>( "addressing" ) );

    for ( JsonNode* node : entry_node->getObjects( "channel_values" ) )
        entry.addValue( node->get<unsigned>( "id" ), node->get<BYTE>( "value" ) );

    return entry;
}