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

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_chases( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    ChasePtrArray chases = studio.getVenue()->getChases();

    std::sort( chases.begin(), chases.end(), CompareObjectNumber );
    
    JsonBuilder json( response );

    json.startArray();
    for ( ChasePtrArray::iterator it=chases.begin(); it != chases.end(); it++ ) {
        Chase* chase = (*it);

        json.startObject();
        json.add( "id", chase->getUID() );
        json.add( "number", chase->getChaseNumber() );
        json.add( "name", chase->getName() );
        json.add( "description", chase->getDescription() );
        json.add( "created", chase->getCreated() );
        json.add( "is_running", (chase->getUID() == studio.getVenue()->getRunningChase()) );
        json.add( "delay_ms", chase->getDelayMS() );
        json.add( "fade_ms", chase->getFadeMS() );
        json.add( "repeat", chase->isRepeat() );
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
    json.endArray();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::delete_chase( CString& response, LPCSTR data ) {
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID chase_id;
        
    if ( sscanf_s( data, "%lu", &chase_id ) != 1 )
        return false;

    return studio.getVenue()->deleteChase( chase_id );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_chase( CString& response, LPCSTR data, EditMode mode ) {
   if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

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

        PARSER_LIST step_parsers = parser.get<PARSER_LIST>( "steps" );

        for ( PARSER_LIST::iterator it=step_parsers.begin(); it != step_parsers.end(); ++it ) {
            steps.push_back( ChaseStep( (*it).get<ULONG>( "id" ), (*it).get<ULONG>( "delay_ms" ), (SceneLoadMethod)(*it).get<int>( "load_method" ) ) );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( chase_id != 0 ) {
        chase = studio.getVenue()->getChase( chase_id );
        if ( !chase )
            return false;
    }

    // Make sure number is unique
    if ( mode != UPDATE || (chase && number != chase->getNumber()) ) {
        if ( studio.getVenue()->getChaseByNumber( number ) != NULL ) 
            return false;
    }

    switch ( mode ) {
        case NEW: {
            Chase new_chase;
            chase_id = studio.getVenue()->addChase( new_chase );
            chase = studio.getVenue()->getChase( chase_id );
            break;
        }

        case COPY: {
            Chase new_chase( *chase );
            chase_id = studio.getVenue()->addChase( new_chase );
            chase = studio.getVenue()->getChase( chase_id );
            break;
        }
    }

    UID running_chase_id = studio.getVenue()->getRunningChase();
    if ( running_chase_id == chase->getUID() )
        studio.getVenue()->stopChase();

    chase->setName( name );
    chase->setChaseNumber( number );
    chase->setDescription( description );
    chase->setDelayMS( delay_ms );
    chase->setFadeMS( fade_ms );
    chase->setSteps( steps );
    chase->setActs( acts );
    chase->setRepeat( repeat );

    studio.getVenue()->chaseUpdated( chase->getUID() );

    if ( running_chase_id == chase->getUID() )
        studio.getVenue()->startChase( chase->getUID() );

    JsonBuilder json( response );
    json.startObject();
    json.add( "id", chase->getUID() );
    json.endObject();

    return true;
}
