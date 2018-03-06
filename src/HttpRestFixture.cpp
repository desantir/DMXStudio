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

void append_channel_json( JsonBuilder& json, ChannelPtrArray& channels, BYTE* channel_values, BYTE* default_values );
void fixtureToJson( Venue* venue, JsonBuilder& json, Fixture* fixture );
void fixtureGroupToJson( Venue* venue, JsonBuilder& json, FixtureGroup* group );

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;
    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    Fixture* fixture = venue->getFixture( uid );
    if ( fixture != NULL ) {
        JsonBuilder json( response );
        json.startArray();
        fixtureToJson( venue, json, fixture );
        json.endArray();
        return;
    }

    FixtureGroup* group = venue->getFixtureGroup( uid );
    if ( group != NULL ) {
        JsonBuilder json( response );
        json.startArray();
        fixtureGroupToJson( venue, json, group );
        json.endArray();
        return;
    }

    throw RestServiceException( "Invalid fixture UID" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
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
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_fixture_definitions( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
	channel_value channel_values[DMX_PACKET_SIZE];
	channel_value default_values[DMX_PACKET_SIZE];

    JsonBuilder json( response );
    json.startArray();

    LPCSTRArray manufacturers = FixtureDefinition::getUniqueManufacturers();
    for ( LPCSTRArray::iterator it=manufacturers.begin(); it != manufacturers.end(); ++it ) {
        json.startObject();
        json.add( "manufacturer", (*it) );

        LPCSTRArray models = FixtureDefinition::getUniqueModels( (*it) );

        json.startArray( "models" );
        for ( LPCSTRArray::iterator it2=models.begin(); it2 != models.end(); it2++ ) {
            json.startObject();
            json.add( "model", (*it2) );

            FixturePersonalityToFUID personalities=FixtureDefinition::getModelPersonalities( (*it), (*it2) );

            json.startArray( "personalities" );
            for ( FixturePersonalityToFUID::iterator it3=personalities.begin(); it3 != personalities.end(); it3++ ) {
                FUID fuid = (*it3).second;

                FixtureDefinition* fd = FixtureDefinition::lookupFixture( fuid );

                if ( fd == NULL )
                    continue;

                ChannelPtrArray channels;

                for ( channel_address ch=0; ch < fd->getNumChannels(); ch++ ) {
                    Channel* channel = fd->getChannel( ch );
                    channels.push_back( channel );
                    channel_values[ch] = 0;
                    default_values[ch] = channel->getDefaultValue();
                }

                json.startObject();
                json.add( "fuid", fd->getFUID() );
                json.add( "type", fd->getType() );
                json.add( "has_dimmer", fd->hasDimmer() );
                json.add( "can_whiteout", fd->canWhiteout() );

                append_channel_json( json, channels, channel_values, default_values );
                
                json.endObject();
            }
            json.endArray( "personalities" );

            json.endObject();
        }
        json.endArray( "models" );

        json.endObject();
    }

    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::delete_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !venue->deleteFixture( fixture_id ) )
        throw RestServiceException( "Invalid fixture UID" );

    venue->clearAllCapturedActors();    // TEMP - for now so sliders and selected are not out of sync
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::delete_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    UID fixture_group_id;
        
    if ( sscanf_s( data, "%lu", &fixture_group_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !venue->deleteFixtureGroup( fixture_group_id ) )
        throw RestServiceException( "Invalid fixture UID" );

    venue->clearAllCapturedActors();    // TEMP - for now so sliders and selected are not out of sync
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_fixturegroup( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode ) {
    SimpleJsonParser parser;
    UID group_id;
    CString name, description;
    GroupNumber number;
    std::vector<ULONG> fixture_ids;
    std::vector<channel_value> channel_values;
	GridPosition position;

    try {
        parser.parse( data );

        group_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        fixture_ids = parser.get<std::vector<ULONG>>( "fixture_ids" );
        channel_values = parser.get<std::vector<channel_value>>( "channel_values" );
		position.m_x = parser.get<long>( "grid_x" );
		position.m_y = parser.get<long>( "grid_y" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Make sure number is unique
   UID other_uid = venue->getFixtureGroupByNumber( number );
    if ( other_uid != NOUID && group_id != other_uid )
        throw RestServiceException( "Fixture group number must be unique" );

    switch ( mode ) {
        case NEW: {
            FixtureGroup group( 0L, number, name, description );
            group.setFixtures( UIDArrayToSet( fixture_ids ) );
			group.setGridPosition( position );
            venue->addFixtureGroup( group );
            break;
        }

        case UPDATE: {
            FixtureGroup* group = venue->getFixtureGroup( group_id );
            if ( !group )
                throw RestServiceException( "Invalid fixture group UID" );

            group->setGroupNumber( number );
            group->setName( name );
            group->setDescription( description );
            group->setFixtures( UIDArrayToSet( fixture_ids ) );
            group->setChannelValues( channel_values.size(), (channel_values.size() == 0) ? NULL : &channel_values[0] );
			group->setGridPosition( position );

            venue->fireEvent( ES_FIXTURE_GROUP, group->getUID(), EA_CHANGED );

            break;
        }
    }
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode )
{
    // {"id":9,"name":"Audio Center","description":"","number":9,"fuid":"984018742","dmx_address":7 }

    SimpleJsonParser parser;
    UID fixture_id;
    FUID fuid;
    CString name, description;
    FixtureNumber number;
    unsigned dmx_address, dmx_universe;
    bool allow_dimming, allow_whiteout;
	GridPosition position;

    try {
        parser.parse( data );

        fixture_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        fuid = parser.get<ULONG>( "fuid" );
        dmx_address = parser.get<ULONG>( "dmx_address" );
        dmx_universe = parser.get<ULONG>( "dmx_universe" );
        allow_dimming = parser.get<bool>( "allow_dimming" );
        allow_whiteout = parser.get<bool>( "allow_whiteout" );
		position.m_x = parser.get<long>("grid_x");
		position.m_y = parser.get<long>("grid_y");
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Make sure number is unique
    UID other_uid = venue->getFixtureByNumber( number );
    if ( other_uid != NOUID && fixture_id != other_uid )
        throw RestServiceException( "Fixture number must be unique" );

    switch ( mode ) {
        case NEW: {
            Fixture fixture( NOUID, number, dmx_universe, dmx_address, fuid, name, description, allow_dimming, allow_whiteout );
			fixture.setGridPosition(position);
            venue->addFixture( fixture );
            break;
        }

        case UPDATE: {
            Fixture* fixture = venue->getFixture( fixture_id );
            if ( !fixture )
                throw RestServiceException( "Invalid fixture UID" );

            fixture->setFixtureNumber( number );
            fixture->setName( name );
            fixture->setDescription( description );
            fixture->setAddress( dmx_address );
            fixture->setUniverseId( dmx_universe );
            fixture->setAllowMasterDimming( allow_dimming );
            fixture->setAllowWhiteout( allow_whiteout );
			fixture->setGridPosition(position);

            venue->fixtureUpdated( fixture_id );

            break;
        }
    }
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_fixture_position(Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type)
{
	SimpleJsonParser parser;
	UID fixture_id;
	GridPosition position;

	try {
		parser.parse(data);

		fixture_id = parser.get<ULONG>("id");
		position.m_x = parser.get<long>("grid_x");
		position.m_y = parser.get<long>("grid_y");
	}
	catch ( std::exception& e ) {
		throw RestServiceException("JSON parser error (%s) data (%s)", e.what( ), data);
	}

	FixtureGroup* group = venue->getFixtureGroup( fixture_id );
	if ( group != NULL ) {
		group->setGridPosition(position);
	}
	else {
		Fixture* fixture = venue->getFixture(fixture_id);
		if ( !fixture )
			throw RestServiceException("Invalid fixture UID");
		fixture->setGridPosition(position);
	}
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_fixture( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    UID actor_id;
    bool is_capture;
    std::vector<channel_value> channel_values;
    UIDArray palette_refs;

    try {
        parser.parse( data );

        actor_id = parser.get<ULONG>( "id" );
        is_capture = parser.get<bool>( "is_capture" );

        if ( parser.has_key( "channel_values" ) )
            channel_values = parser.getArrayAsVector<channel_value>( "channel_values" );

        if ( parser.has_key( "palette_refs" ) )
            palette_refs = parser.getArrayAsVector<UID>( "palette_refs" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( is_capture ) {
        SceneActor* actor = venue->captureFixture( actor_id, &channel_values, &palette_refs );
        if ( !actor )
            throw RestServiceException( "Invalid fixture UID" );

        // Return current channel values
        JsonBuilder json( response );
        json.startArray();
        for ( channel_address ch=0; ch < actor->getNumChannels(); ch++ )
            json.add( actor->getBaseChannelValue( ch ) );
        json.endArray();
    }
    else if ( actor_id != 0 )
        venue->releaseActor( actor_id );
    else
        venue->clearAllCapturedActors();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_fixture_channel( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;

    try {
        parser.parse( data );

		DWORD change_id = parser.get<DWORD>( "change_id", 0L );
        JsonNodePtrArray channel_info = parser.getObjects( "channel_data" );
        SetChannelArray channel_sets;

        for ( JsonNode* cp : channel_info ) {
            UID actor_id = cp->get<UID>( "actor_id" );
            bool capture = cp->get<bool>( "capture");

            if ( !capture && venue->getDefaultScene()->getActor( actor_id ) == NULL)    // Actor may have been dropped
                continue;

            channel_sets.emplace_back( actor_id, cp->get<channel_address>( "channel" ), cp->get<channel_value>( "value" ) );
        }
        
        if ( channel_sets.size() > 0 )
            venue->captureAndSetChannelValue( channel_sets, change_id );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_fixture_palettes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;

    try {
        parser.parse( data );
        JsonNodePtrArray channel_info = parser.getObjects();

        for ( JsonNode* cp : channel_info ) {
            UID actor_id = cp->get<UID>( "actor_id" );
            UIDArray palette_refs= cp->getArrayAsVector<UID>( "palette_refs" );
            bool capture = cp->get<bool>( "capture");

            if ( !capture && venue->getDefaultScene()->getActor( actor_id ) == NULL)    // Actor may have been dropped
                continue;

            venue->captureAndSetPalettes( actor_id, palette_refs );
        }
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }
}

// ----------------------------------------------------------------------------
//
void append_channel_json( JsonBuilder& json, ChannelPtrArray& channels, channel_value* channel_values, channel_value* default_values )
{
    json.add( "num_channels", channels.size() );

    json.startArray( "channels" );
    for ( channel_address ch=0; ch < channels.size(); ch++ ) {
        Channel* channel = channels[ch];

        json.startObject();
        json.add( "channel", ch );
        json.add( "name", channel->getName() );
        json.add( "type", channel->getType() );
        json.add( "type_name", Channel::getTypeName(channel->getType()) );
        json.add( "value", channel_values ? channel_values[ch] : 0 );
        json.add( "default_value", default_values ? default_values[ch] : 0 );

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
	channel_value channel_values[DMX_PACKET_SIZE];
	channel_value default_values[DMX_PACKET_SIZE];

    // Check if fixture is captured (Ocalled active in clients)
    bool is_active = venue->getDefaultScene()->hasActor( fixture->getUID() );

    ChannelPtrArray channels;

    for ( size_t ch=0; ch < fixture->getNumChannels(); ch++ ) {
        Channel* channel = fixture->getChannel( ch );
        channels.push_back( channel );
        channel_values[ch] = venue->getBaseChannelValue( fixture, ch );
        default_values[ch] = channel->getDefaultValue();
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
    json.add( "allow_dimming", fixture->getAllowMasterDimming() );
    json.add( "allow_whiteout", fixture->getAllowWhiteout() );
	json.add( "grid_x", fixture->getGridPosition().m_x );
	json.add( "grid_y", fixture->getGridPosition().m_y );
    json.add( "is_tracked", venue->isFixtureTracked( fixture->getUID() ) );

    RGBWA  state_color;
    bool state_strobing;

    venue->getFixtureState( fixture->getUID(), state_color, state_strobing );
    json.add( "fixture_state", state_color );
    json.add( "fixture_strobe", state_strobing );

    append_channel_json( json, channels, channel_values, default_values );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void fixtureGroupToJson( Venue* venue, JsonBuilder& json, FixtureGroup* group )
{
	channel_value channel_values[DMX_PACKET_SIZE];
	channel_value default_values[DMX_PACKET_SIZE];

    UIDSet fixtures = group->getFixtures();

    bool is_active = venue->getDefaultScene()->hasActor( group->getUID() );

    ChannelPtrArray channels;

    Fixture* pf = venue->getGroupRepresentative( group->getUID() );
    if ( pf != NULL ) {
        for ( size_t ch=channels.size(); ch < pf->getNumChannels(); ch++ ) {
            channels.push_back( pf->getChannel( ch ) );
            channel_values[ch] = venue->getBaseChannelValue( group, ch );
            default_values[ch] = ch < group->getNumChannelValues() ? group->getChannelValue(ch) : 0;
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
	json.add( "grid_x", group->getGridPosition( ).m_x );
	json.add( "grid_y", group->getGridPosition( ).m_y );

    append_channel_json( json, channels, channel_values, default_values );

    json.endObject();
}