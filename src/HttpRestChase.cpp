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

void chaseToJson( Venue* venue, JsonBuilder& json, Chase* scene );

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) 
{
    UID uid;
    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    Chase* chase = venue->getChase( uid );
    if ( chase == NULL )
        throw RestServiceException( "Invalid chase UID" );

    JsonBuilder json( response );
    json.startArray();
    chaseToJson( venue, json, chase );
    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_chases( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    ChasePtrArray chases = venue->getChases();
    std::sort( chases.begin(), chases.end(), CompareObjectNumber );
    
    JsonBuilder json( response );

    json.startArray();

    for ( ChasePtrArray::iterator it=chases.begin(); it != chases.end(); it++ ) {
        chaseToJson( venue, json, (*it) );
    }

    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_chase_step( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    int steps;

    if ( sscanf_s( data, "%d", &steps ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->chaseStep( steps );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::delete_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID chase_id;
        
    if ( sscanf_s( data, "%lu", &chase_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !venue->deleteChase( chase_id ) )
        throw RestServiceException( "Invalid chase UID" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_chase( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode )
{
    SimpleJsonParser parser;
    UID chase_id;
    CString name, description;
    SceneNumber number;
    ULONG delay_ms;
    ULONG fade_ms;
    ChaseStepArray steps;
    Chase* chase = NULL;
    Acts acts;
    bool repeat;
    ChaseStepTrigger step_trigger;

    try {
        parser.parse( data );

        chase_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        delay_ms = parser.get<ULONG>( "delay_ms" );
        fade_ms = parser.get<ULONG>( "fade_ms" );
        acts = parser.get<Acts>( "acts" );
        repeat = parser.get<bool>( "repeat" );
        step_trigger = (ChaseStepTrigger)parser.get<unsigned>( "step_trigger" );

        for ( JsonNode* step : parser.getObjects( "steps" ) ) {
            steps.emplace_back( step->get<ULONG>( "id" ), step->get<ULONG>( "delay_ms" ), (SceneLoadMethod)step->get<int>( "load_method" ) );
        }
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    UID running_chase_id = venue->getRunningChase();

    if ( chase_id != 0 ) {
        chase = venue->getChase( chase_id );
        if ( !chase )
            throw RestServiceException( "Invalid chase UID" );

        if ( running_chase_id == chase->getUID() )
            venue->stopChase();
    }

    // Make sure number is unique
    if ( mode != UPDATE || (chase && number != chase->getNumber()) ) {
        if ( venue->getChaseByNumber( number ) != NOUID ) 
            throw RestServiceException( "Chase number must be unique" );
    }

    switch ( mode ) {
        case NEW: {
            Chase new_chase;
            chase_id = venue->addChase( new_chase );
            chase = venue->getChase( chase_id );
            break;
        }

        case COPY: {
            Chase new_chase( *chase );
            new_chase.setUID( NOUID );
            chase_id = venue->addChase( new_chase );
            chase = venue->getChase( chase_id );
            break;
        }

        // Fall through 

        case UPDATE:
            break;
    }

    chase->setName( name );
    chase->setChaseNumber( number );
    chase->setDescription( description );
    chase->setDelayMS( delay_ms );
    chase->setFadeMS( fade_ms );
    chase->setSteps( steps );
    chase->setActs( acts );
    chase->setRepeat( repeat );
    chase->setStepTrigger( step_trigger );

    venue->chaseUpdated( chase->getUID() );

    if ( running_chase_id == chase->getUID() )
        venue->startChase( chase->getUID() );

    JsonBuilder json( response );
    json.startObject();
    json.add( "id", chase->getUID() );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void chaseToJson( Venue* venue, JsonBuilder& json, Chase* chase )
{
    json.startObject();
    json.add( "id", chase->getUID() );
    json.add( "number", chase->getChaseNumber() );
    json.add( "name", chase->getName() );
    json.add( "description", chase->getDescription() );
    json.add( "created", chase->getCreated() );
    json.add( "is_running", (chase->getUID() == venue->getRunningChase()) );
    json.add( "delay_ms", chase->getDelayMS() );
    json.add( "fade_ms", chase->getFadeMS() );
    json.add( "repeat", chase->isRepeat() );
    json.add( "step_trigger", chase->getStepTrigger() );
    json.addArray<Acts>( "acts", chase->getActs() );

    // Add chase steps
    ChaseStepArray steps = chase->getSteps();

    json.startArray( "steps" );
    for ( ChaseStepArray::iterator steps_it=steps.begin(); steps_it != steps.end(); steps_it++ ) {
        json.startObject();
        json.add( "id", (*steps_it).getSceneUID() );
        json.add( "delay_ms", (*steps_it).getDelayMS() );
        json.add( "load_method", (*steps_it).getMethod() ); 
        json.endObject();
    }
    json.endArray( "steps" );

    json.endObject();
}