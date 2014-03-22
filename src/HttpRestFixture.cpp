/* 
Copyright (C) 2011-2013 Robert DeSantis
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

// ----------------------------------------------------------------------------
//
static void append_channel_json( JsonBuilder& json, ChannelPtrArray& channels, BYTE* channel_values )
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
            json.endObject();
        }
        json.endArray( "ranges" );

        json.endObject();
    }
    json.endArray( "channels" );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_fixture_definitions( CString& response, LPCSTR data )
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
bool HttpRestServices::query_fixtures( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    FixtureGroupPtrArray groups = studio.getVenue()->getFixtureGroups();

    std::sort( groups.begin(), groups.end(), CompareObjectNumber );

    JsonBuilder json( response );
    json.startArray();

    BYTE channel_values[DMX_PACKET_SIZE];

    for ( FixtureGroupPtrArray::iterator it=groups.begin(); it != groups.end(); it++ ) {
        FixtureGroup* group = (*it);
        UIDSet fixtures = group->getFixtures();

        bool is_active = studio.getVenue()->getDefaultScene()->hasActor( group->getUID() );
        
        ChannelPtrArray channels;

        Fixture* pf = studio.getVenue()->getGroupRepresentative( group->getUID() );
        if ( pf != NULL ) {
            SceneActor actor( studio.getVenue(), group );

            for ( UINT ch=channels.size(); ch < pf->getNumChannels(); ch++ ) {
                channels.push_back( pf->getChannel( ch ) );
                channel_values[ch] = studio.getVenue()->getChannelValue( actor, ch );
            } 
        }

        json.startObject();
        json.add( "id", group->getUID() );
        json.add( "number", group->getGroupNumber() );
        json.add( "dmx_address", 0 );
        json.add( "fuid", 0L );
        json.add( "full_name", group->getName() );
        json.add( "is_group", true );
        json.add( "name", group->getName() );
        json.add( "description", group->getDescription() );
        json.addArray<UIDSet>( "fixture_ids", fixtures );
        json.add( "is_active", is_active );

        append_channel_json( json, channels, channel_values );

        json.endObject();
    }

    FixturePtrArray fixtures = studio.getVenue()->getFixtures();

    std::sort( fixtures.begin(), fixtures.end(), CompareObjectNumber );

    for ( FixturePtrArray::iterator it=fixtures.begin(); it != fixtures.end(); it++ ) {
        Fixture* pf = (*it);

        // Check if fixture is captured
        bool is_active = studio.getVenue()->getDefaultScene()->getActor( pf->getUID() ) != NULL;

        ChannelPtrArray channels;

        for ( channel_t ch=0; ch < (*it)->getNumChannels(); ch++ ) {
            channels.push_back((*it)->getChannel( ch ) );
            channel_values[ch] = studio.getVenue()->getChannelValue( pf, ch );
        }

        json.startObject();
        json.add( "id", pf->getUID() );
        json.add( "number", pf->getFixtureNumber() );
        json.add( "dmx_address", pf->getAddress() );
        json.add( "fuid", pf->getFUID() );
        json.add( "full_name", pf->getFullName() );
        json.add( "is_group", false );
        json.add( "name", pf->getName() );
        json.add( "description", pf->getDescription() );
        json.addNull( "fixture_ids" );
        json.add( "is_active", is_active );
        json.add( "manufacturer", pf->getManufacturer() );
        json.add( "model", pf->getModel() );
        json.add( "type_name", pf->getTypeName() );

        append_channel_json( json, channels, channel_values );

        json.endObject();
    }

    json.endArray();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::delete_fixture( CString& response, LPCSTR data ) {
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( !studio.getVenue()->deleteFixture( fixture_id ) )
        return false;

    studio.getVenue()->clearAllCapturedActors();    // TEMP - for now so sliders and selected are not out of sync

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::delete_fixturegroup( CString& response, LPCSTR data ) {
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_group_id;
        
    if ( sscanf_s( data, "%lu", &fixture_group_id ) != 1 )
        return false;

    if ( !studio.getVenue()->deleteFixtureGroup( fixture_group_id ) )
        return false;

    studio.getVenue()->clearAllCapturedActors();    // TEMP - for now so sliders and selected are not out of sync

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_fixturegroup( CString& response, LPCSTR data, EditMode mode ) {
   if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    UID group_id;
    CString name, description;
    GroupNumber number;
    std::vector<ULONG> fixture_ids;

    try {
        parser.parse( data );

        group_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        fixture_ids = parser.get<std::vector<ULONG>>( "fixture_ids" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Make sure number is unique
    FixtureGroup* test = studio.getVenue()->getFixtureGroupByNumber( number );
    if ( test != NULL && group_id != test->getUID() )
        return false;

    switch ( mode ) {
        case NEW: {
            FixtureGroup group( studio.getVenue()->allocUID(), number, name, description );
            group.setFixtures( UIDArrayToSet( fixture_ids ) );
            studio.getVenue()->addFixtureGroup( group );
            break;
        }

        case UPDATE: {
            FixtureGroup* group = studio.getVenue()->getFixtureGroup( group_id );
            if ( !group )
                return false;

            group->setGroupNumber( number );
            group->setName( name );
            group->setDescription( description );
            group->setFixtures( UIDArrayToSet( fixture_ids ) );
            break;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_fixture( CString& response, LPCSTR data, EditMode mode )
{
    // {"id":9,"name":"Audio Center","description":"","number":9,"fuid":"984018742","dmx_address":7 }

   if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    UID fixture_id;
    FUID fuid;
    CString name, description;
    FixtureNumber number;
    int dmx_address;

    try {
        parser.parse( data );

        fixture_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        fuid = parser.get<ULONG>( "fuid" );
        dmx_address = parser.get<ULONG>( "dmx_address" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Make sure number is unique
    Fixture* fixture = studio.getVenue()->getFixtureByNumber( number );
    if ( fixture != NULL && fixture_id != fixture->getUID() )
        return false;

    switch ( mode ) {
        case NEW: {
            Fixture fixture( studio.getVenue()->allocUID(), number, 1, dmx_address, fuid, name, description );
            studio.getVenue()->addFixture( fixture );
            break;
        }

        case UPDATE: {
            if ( !fixture )
                return false;

            fixture->setFixtureNumber( number );
            fixture->setName( name );
            fixture->setDescription( description );
            fixture->setAddress( dmx_address );
            break;
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
        SceneActor* actor = studio.getVenue()->captureFixture( fixture_id );
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
            studio.getVenue()->releaseActor( fixture_id );
        else
            studio.getVenue()->clearAllCapturedActors();

        // If releasing, reload the current scene
        studio.getVenue()->loadScene();
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_group( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    UID group_uid;
    bool is_capture;

    try {
        parser.parse( data );

        group_uid = parser.get<ULONG>( "id" );
        is_capture = parser.get<bool>( "is_capture" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( is_capture ) {
        SceneActor* actor = studio.getVenue()->captureFixtureGroup( group_uid );
        if ( !actor )
            return false;

        // Return current channel values 
        JsonBuilder json( response );

        json.startArray();
        Fixture* pf = studio.getVenue()->getGroupRepresentative( group_uid );
        for ( channel_t ch=0; ch < actor->getNumChannels(); ch++ )
            json.add( actor->getChannelValue( ch ) );
        json.endArray();
    }
    else {
        studio.getVenue()->releaseActor( group_uid );

        // If releasing, reload the current scene
        studio.getVenue()->loadScene();
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_channels( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
   if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    // [{ "fixture_id": fixture_id, "channel": channel, "value": value }];

    SimpleJsonParser parser;
    PARSER_LIST channel_info;

    try {
        parser.parse( data );
        channel_info = parser.get<PARSER_LIST>( "" );

        for ( PARSER_LIST::iterator it=channel_info.begin(); it != channel_info.end(); ++it ) {
            SimpleJsonParser& cp = (*it);

            UID actor_id = cp.get<UID>( "actor_id" );
            channel_t channel = cp.get<channel_t>( "channel" );
            BYTE channel_value = cp.get<BYTE>( "value" );

            SceneActor* actor = studio.getVenue()->getDefaultScene()->getActor( actor_id );
            if ( actor )                                                               // Actor may have been dropped
                studio.getVenue()->captureAndSetChannelValue( *actor, channel, channel_value );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_release( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id != 0 )
        studio.getVenue()->releaseActor( fixture_id );
    else
        studio.getVenue()->clearAllCapturedActors();

    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_capture( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
    char what[11];
        
    if ( sscanf_s( data, "%10[^/]/%lu", what, 11, &fixture_id ) != 2 )
        return false;

    if ( fixture_id > 0 ) {
        SceneActor* actor = NULL;
        
        if ( strcmp( what, "group" ) == 0)
            actor = studio.getVenue()->captureFixtureGroup( fixture_id );
        else
            actor = studio.getVenue()->captureFixture( fixture_id );

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
bool HttpRestServices::control_fixture_channel( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
    char what[11];
    channel_t channel;
    unsigned channel_value;

    if ( sscanf_s( data, "%10[^/]/%lu/%u/%u", what, 11, &fixture_id, &channel, &channel_value ) != 4 )
        return false;

    SceneActor actor;

    if ( strcmp( what, "group" ) == 0 ) {
        FixtureGroup* group = studio.getVenue()->getFixtureGroup( fixture_id );
        if ( !group )
            return false;
        actor = SceneActor( studio.getVenue(), group );
    }
    else {
        Fixture* pf = studio.getVenue()->getFixture( fixture_id );
        if ( !pf )
            return false;
        actor = SceneActor( pf );
    }

    studio.getVenue()->captureAndSetChannelValue( actor, channel, channel_value );

    return true;
}
