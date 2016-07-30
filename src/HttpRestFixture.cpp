/* 
Copyright (C) 2011-2016 Robert DeSantis
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
#include "SimpleJsonBuilder.h"

void append_channel_json( JsonBuilder& json, ChannelPtrArray& channels, BYTE* channel_values );
void fixtureToJson( Venue* venue, JsonBuilder& json, Fixture* fixture );
void fixtureGroupToJson( Venue* venue, JsonBuilder& json, FixtureGroup* group );

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;
    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        return false;

    Fixture* fixture = venue->getFixture( uid );
    if ( fixture != NULL ) {
        JsonBuilder json( response );
        json.startArray();
        fixtureToJson( venue, json, fixture );
        json.endArray();
        return true;
    }

    FixtureGroup* group = venue->getFixtureGroup( uid );
    if ( group != NULL ) {
        JsonBuilder json( response );
        json.startArray();
        fixtureGroupToJson( venue, json, group );
        json.endArray();
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    FixtureGroupPtrArray groups = venue->getFixtureGroups();
    std::sort( groups.begin(), groups.end(), CompareObjectNumber );

    JsonBuilder json( response );
    json.startArray();

    for ( FixtureGroupPtrArray::iterator it=groups.begin(); it != groups.end(); it++ ) {
        fixtureGroupToJson( venue, json, (*it ) );
    }

    FixturePtrArray fixtures = venue->getFixtures();
    std::sort( fixtures.begin(), fixtures.end(), CompareObjectNumber );

    for ( FixturePtrArray::iterator it=fixtures.begin(); it != fixtures.end(); it++ ) {
        fixtureToJson( venue, json, (*it ) );
    }

    json.endArray();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_fixture_definitions( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    JsonBuilder json( response );
    json.startArray();

    LPCSTRArray manufacturers = FixtureDefinition::getUniqueManufacturers();
    for ( LPCSTRArray::iterator it=manufacturers.begin(); it != manufacturers.end(); ++it ) {
        json.startObject();
        json.add( "manufacturer", (*it) );

        LPCSTRArray models = FixtureDefinition::getUniqueModels( (*it) );

        json.startArray( "fixtures" );
        for ( LPCSTRArray::iterator it2=models.begin(); it2 != models.end(); it2++ ) {
            json.startObject();
            json.add( "model", (*it2) );

            FixturePersonalityToFUID personalities=FixtureDefinition::getModelPersonalities( (*it), (*it2) );

            json.startArray( "personalities" );
            for ( FixturePersonalityToFUID::iterator it3=personalities.begin(); it3 != personalities.end(); it3++ ) {
                json.startObject();
                json.add( "fuid", (*it3).second );
                json.add( "num_channels", (*it3).first );
                json.endObject();
            }
            json.endArray( "personalities" );

            json.endObject();
        }
        json.endArray( "fixtures" );

        json.endObject();
    }

    json.endArray();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::delete_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( !venue->deleteFixture( fixture_id ) )
        return false;

    venue->clearAllCapturedActors();    // TEMP - for now so sliders and selected are not out of sync

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::delete_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    UID fixture_group_id;
        
    if ( sscanf_s( data, "%lu", &fixture_group_id ) != 1 )
        return false;

    if ( !venue->deleteFixtureGroup( fixture_group_id ) )
        return false;

    venue->clearAllCapturedActors();    // TEMP - for now so sliders and selected are not out of sync

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode ) {
    SimpleJsonParser parser;
    UID group_id;
    CString name, description;
    GroupNumber number;
    std::vector<ULONG> fixture_ids;
    std::vector<BYTE> channel_values;

    try {
        parser.parse( data );

        group_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        fixture_ids = parser.get<std::vector<ULONG>>( "fixture_ids" );
        channel_values = parser.get<std::vector<BYTE>>( "channel_values" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Make sure number is unique
    FixtureGroup* test = venue->getFixtureGroupByNumber( number );
    if ( test != NULL && group_id != test->getUID() )
        return false;

    switch ( mode ) {
        case NEW: {
            FixtureGroup group( 0L, number, name, description );
            group.setFixtures( UIDArrayToSet( fixture_ids ) );
            venue->addFixtureGroup( group );
            break;
        }

        case UPDATE: {
            FixtureGroup* group = venue->getFixtureGroup( group_id );
            if ( !group )
                return false;

            group->setGroupNumber( number );
            group->setName( name );
            group->setDescription( description );
            group->setFixtures( UIDArrayToSet( fixture_ids ) );
            group->setChannelValues( channel_values.size(), (channel_values.size() == 0) ? NULL : &channel_values[0] );
            break;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode )
{
    // {"id":9,"name":"Audio Center","description":"","number":9,"fuid":"984018742","dmx_address":7 }

    SimpleJsonParser parser;
    UID fixture_id;
    FUID fuid;
    CString name, description;
    FixtureNumber number;
    unsigned dmx_address, dmx_universe;

    try {
        parser.parse( data );

        fixture_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        fuid = parser.get<ULONG>( "fuid" );
        dmx_address = parser.get<ULONG>( "dmx_address" );
        dmx_universe = parser.get<ULONG>( "dmx_universe" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Make sure number is unique
    Fixture* test = venue->getFixtureByNumber( number );
    if ( test != NULL && fixture_id != test->getUID() )
        return false;

    switch ( mode ) {
        case NEW: {
            Fixture fixture( NOUID, number, dmx_universe, dmx_address, fuid, name, description );
            venue->addFixture( fixture );
            break;
        }

        case UPDATE: {
            Fixture* fixture = venue->getFixture( fixture_id );
            if ( !fixture )
                return false;

            fixture->setFixtureNumber( number );
            fixture->setName( name );
            fixture->setDescription( description );
            fixture->setAddress( dmx_address );
            fixture->setUniverseId( dmx_universe );
            break;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    UID fixture_id;
    bool is_capture;
    std::vector<BYTE> channel_values;

    try {
        parser.parse( data );

        fixture_id = parser.get<ULONG>( "id" );
        is_capture = parser.get<bool>( "is_capture" );

        if ( parser.has_key( "channel_values" ) )
            channel_values = parser.get<std::vector<BYTE>>( "channel_values" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( is_capture ) {
        SceneActor* actor = venue->captureFixture( fixture_id );
        if ( !actor )
            return false;

        // Pre-set channel values (optional)
        if ( channel_values.size() == actor->getNumChannels() ) {
            for ( channel_t channel=0; channel < channel_values.size(); channel++ )
                actor->setChannelValue( channel, channel_values[channel] );
        }

        // Return current channel values
        JsonBuilder json( response );
        json.startArray();
        for ( channel_t ch=0; ch < actor->getNumChannels(); ch++ )
            json.add( actor->getChannelValue( ch ) );
        json.endArray();
    }
    else {
        if ( fixture_id != 0 )
            venue->releaseActor( fixture_id );
        else
            venue->clearAllCapturedActors();
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_group( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    UID group_uid;
    bool is_capture;
    std::vector<BYTE> channel_values;

    try {
        parser.parse( data );

        group_uid = parser.get<ULONG>( "id" );
        is_capture = parser.get<bool>( "is_capture" );

        if ( parser.has_key( "channel_values" ) ) 
            channel_values = parser.get<std::vector<BYTE>>( "channel_values" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( is_capture ) {
        SceneActor* actor = venue->captureFixtureGroup( group_uid );
        if ( !actor )
            return false;

        // Pre-set channel values (optional)
        if ( channel_values.size() == actor->getNumChannels() ) {
            for ( channel_t channel=0; channel < channel_values.size(); channel++ )
                actor->setChannelValue( channel, channel_values[channel] );
        }

        // Return current channel values 
        JsonBuilder json( response );

        json.startArray();
        Fixture* pf = venue->getGroupRepresentative( group_uid );
        for ( channel_t ch=0; ch < actor->getNumChannels(); ch++ )
            json.add( actor->getChannelValue( ch ) );
        json.endArray();
    }
    else {
        venue->releaseActor( group_uid );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_channels( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;

    try {
        parser.parse( data );
        JsonNodePtrArray channel_info = parser.getObjects();

        for ( JsonNode* cp : channel_info ) {
            UID actor_id = cp->get<UID>( "actor_id" );
            channel_t channel = cp->get<channel_t>( "channel" );
            BYTE channel_value = cp->get<BYTE>( "value" );

            SceneActor* actor = venue->getDefaultScene()->getActor( actor_id );
            if ( actor )                                                               // Actor may have been dropped
                venue->captureAndSetChannelValue( *actor, channel, channel_value );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_release( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id != 0 )
        venue->releaseActor( fixture_id );
    else
        venue->clearAllCapturedActors();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_capture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID fixture_id;
    char what[11];
        
    if ( sscanf_s( data, "%10[^/]/%lu", what, 11, &fixture_id ) != 2 )
        return false;

    if ( fixture_id > 0 ) {
        SceneActor* actor = NULL;
        
        if ( strcmp( what, "group" ) == 0)
            actor = venue->captureFixtureGroup( fixture_id );
        else
            actor = venue->captureFixture( fixture_id );

        if ( !actor )
            return false;

        // Return current channel values
        JsonBuilder json( response );
        json.startArray();
        for ( channel_t ch=0; ch < actor->getNumChannels(); ch++ )
            json.add( actor->getChannelValue( ch ) );
        json.endArray();
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_channel( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID fixture_id;
    char what[11];
    channel_t channel;
    unsigned channel_value;

    if ( sscanf_s( data, "%10[^/]/%lu/%u/%u", what, 11, &fixture_id, &channel, &channel_value ) != 4 )
        return false;

    SceneActor actor;

    if ( strcmp( what, "group" ) == 0 ) {
        FixtureGroup* group = venue->getFixtureGroup( fixture_id );
        if ( !group )
            return false;
        actor = SceneActor( venue, group );
    }
    else {
        Fixture* pf = venue->getFixture( fixture_id );
        if ( !pf )
            return false;
        actor = SceneActor( pf );
    }

    venue->captureAndSetChannelValue( actor, channel, channel_value );

    return true;
}

// ----------------------------------------------------------------------------
//
void append_channel_json( JsonBuilder& json, ChannelPtrArray& channels, BYTE* channel_values )
{
    json.add( "num_channels", channels.size() );

    json.startArray( "channels" );
    for ( channel_t ch=0; ch < channels.size(); ch++ ) {
        Channel* channel = channels[ch];

        json.startObject();
        json.add( "channel", ch );
        json.add( "name", channel->getName() );
        json.add( "type", channel->getType() );
        json.add( "type_name", Channel::getTypeName(channel->getType()) );
        json.add( "value", channel_values ? channel_values[ch] : 0 );

        ChannelValueRangeArray cvra = channel->getRanges();

        json.startArray( "ranges" );
        for ( ChannelValueRangeArray::iterator it=cvra.begin(); it != cvra.end(); ++it ) {
            json.startObject();
            json.add( "start", (*it).getStart() );
            json.add( "end", (*it).getEnd() );
            json.add( "name", (*it).getName() );
            json.add( "extra", (*it).getExtra() );
            json.endObject();
        }
        json.endArray( "ranges" );

        json.endObject();
    }
    json.endArray( "channels" );
}

// ----------------------------------------------------------------------------
//
void fixtureToJson( Venue* venue, JsonBuilder& json, Fixture* fixture )
{
    BYTE channel_values[DMX_PACKET_SIZE];

    // Check if fixture is captured
    bool is_active = venue->getDefaultScene()->getActor( fixture->getUID() ) != NULL;

    ChannelPtrArray channels;

    for ( channel_t ch=0; ch < fixture->getNumChannels(); ch++ ) {
        channels.push_back( fixture->getChannel( ch ) );
        channel_values[ch] = venue->getChannelValue( fixture, ch );
    }

    json.startObject();
    json.add( "id", fixture->getUID() );
    json.add( "number", fixture->getFixtureNumber() );
    json.add( "created", fixture->getCreated() );
    json.add( "dmx_address", fixture->getAddress() );
    json.add( "dmx_universe", fixture->getUniverseId() );
    json.add( "fuid", fixture->getFUID() );
    json.add( "full_name", fixture->getFullName() );
    json.add( "is_group", false );
    json.add( "name", fixture->getName() );
    json.add( "description", fixture->getDescription() );
    json.addNull( "fixture_ids" );
    json.add( "is_active", is_active );
    json.add( "manufacturer", fixture->getManufacturer() );
    json.add( "model", fixture->getModel() );
    json.add( "type_name", fixture->getTypeName() );

    append_channel_json( json, channels, channel_values );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void fixtureGroupToJson( Venue* venue, JsonBuilder& json, FixtureGroup* group )
{
    BYTE channel_values[DMX_PACKET_SIZE];

    UIDSet fixtures = group->getFixtures();

    bool is_active = venue->getDefaultScene()->hasActor( group->getUID() );

    ChannelPtrArray channels;

    Fixture* pf = venue->getGroupRepresentative( group->getUID() );
    if ( pf != NULL ) {
        SceneActor actor( venue, group );

        for ( UINT ch=channels.size(); ch < pf->getNumChannels(); ch++ ) {
            channels.push_back( pf->getChannel( ch ) );

            if ( group->getNumChannelValues() > ch )
                channel_values[ch] = group->getChannelValue( ch );
            else
                channel_values[ch] = venue->getChannelValue( actor, ch );
        } 
    }

    json.startObject();
    json.add( "id", group->getUID() );
    json.add( "number", group->getGroupNumber() );
    json.add( "created", group->getCreated() );
    json.add( "dmx_address", 0 );
    json.add( "dmx_universe", 0 );
    json.add( "fuid", 0L );
    json.add( "full_name", group->getName() );
    json.add( "is_group", true );
    json.add( "name", group->getName() );
    json.add( "description", group->getDescription() );
    json.addArray<UIDSet>( "fixture_ids", fixtures );
    json.add( "is_active", is_active );
    json.add( "has_channel_values", group->getNumChannelValues() > 0 );

    append_channel_json( json, channels, channel_values );

    json.endObject();
}