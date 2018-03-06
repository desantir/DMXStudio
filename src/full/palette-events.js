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

var DEBUG_EVENTS = false;

var eventHandlers = [ EventSourceType.ES_NUM_SOURCES ];

eventHandlers[EventSourceType.ES_PALETTE] = handlePaletteEvent;

// ----------------------------------------------------------------------------
//
function handleEvent( event ) {
    if ( DEBUG_EVENTS )
        toastEvent( event );

    var handler = (event.source > 0 && event.source < EventSourceType.ES_NUM_SOURCES ) ? eventHandlers[ event.source ] : null;

    if ( handler != null && handler != undefined ) {
        if ( !handler( event ) )
            toastWarning( "UNPROCESSED EVENT ACTION: " + eventActionNames[ event.action] + " " + eventSourceNames[ event.source ] + 
                            " UID " + event.uid + " [" + event.val1 + "] " + event.text );
    }
}
// ----------------------------------------------------------------------------
//
function handlePaletteEvent( event ) {
    if ( event.action == EventActionType.EA_NEW ) {
        newPaletteEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_DELETED ) {
        deletePaletteEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_CHANGED ) {
        changePaletteEvent( event.uid );
        return true;
    }

    return false;
}