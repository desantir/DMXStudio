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

// ----------------------------------------------------------------------------
//
void HttpRestServices::fetch_events( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    ULONG now = GetTickCount();

    // Wait up to 5 seconds for an event then release
    while ( !session->hasPendingEvents() && GetTickCount() < now + (1000 * 5) ) {
        Sleep(10);
    }

    JsonBuilder json( response );
    json.startObject();

    json.add( "success", true );

    json.startArray( "events" );

    Event event;

    while ( session->nextEvent( event ) ) {
        json.startObject();
        json.add( "action", event.m_action );
        json.add( "source", event.m_source );
        json.add( "uid", event.m_uid );
        json.add( "text", event.m_text );

        json.add( "val1", event.m_val1 );
        json.add( "val2", event.m_val2 );
        json.add( "val3", event.m_val3 );
        json.add( "val4", event.m_val4 );
        json.endObject();
    }

    json.endArray( "events" );

    json.endObject();
}
