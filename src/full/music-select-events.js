/* 
Copyright (C) 2017 Robert DeSantis
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

eventHandlers[EventSourceType.ES_STUDIO] = handleStudioEvent;
eventHandlers[EventSourceType.ES_TRACK] = handleTrackEvent;
eventHandlers[EventSourceType.ES_VOLUME] = handleVolumeEvent;
eventHandlers[EventSourceType.ES_VOLUME_MUTE] = handleVolumeMuteEvent;
eventHandlers[EventSourceType.ES_TRACK_QUEUES] = handleTrackQueuesEvent;
eventHandlers[EventSourceType.ES_VIDEO_PALETTE] = handleVideoPaletteEvent;
eventHandlers[EventSourceType.ES_MUSIC_PLAYER] = handleMusicPlayerEvent;

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
function handleVolumeEvent( event ) {

    if ( event.action == EventActionType.EA_CHANGED ) {
        updateVolume( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleVolumeMuteEvent( event ) {

    if ( event.action == EventActionType.EA_START || event.action == EventActionType.EA_STOP ) {
        updateVolumeMute( event.action == EventActionType.EA_START );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleTrackEvent( event ) {
    if ( event.action == EventActionType.EA_START ) {
        startTrackEvent( event.text, event.val1 );
        return true;
    }

    if ( event.action == EventActionType.EA_STOP ) {
        stopTrackEvent( event.text );
        return true;
    }

    if ( event.action == EventActionType.EA_PAUSE ) {
        pauseTrackEvent( event.text );
        return true;
    }

    if ( event.action == EventActionType.EA_RESUME ) {
        resumeTrackEvent( event.text );
        return true;
    }

    if ( event.action == EventActionType.EA_TIME ) {
        timeTrackEvent( event.text, event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleTrackQueuesEvent( event ) {
    if ( event.action == EventActionType.EA_CHANGED ) {
        changeTrackQueueEvent( event.val1, event.val2 );
        return true;
    } 

    return false;
}

// ----------------------------------------------------------------------------
//
function handleVideoPaletteEvent( event ) {
    if ( event.action == EventActionType.EA_NEW ) {
        videoPaletteUpdateEvent( event.text, event.val1 );
        return true;
    } 
    else if ( event.action == EventActionType.EA_ERROR ) {
        videoPaletteErrorEvent( event.text, event.val1 );
        return true;
    } 

    return false;
}

// ----------------------------------------------------------------------------
//
function handleStudioEvent( event ) {
    if ( event.action == EventActionType.EA_MESSAGE ) {
        if ( event.val1 == 1 )
            toastWarning( event.text );
        else if ( event.val1 == 2 )
            toastError( event.text );
        else 
            toastNotice( event.text );

        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleMusicPlayerEvent( event ) {
    if ( event.action == EventActionType.EA_START ) {
        if ( track_selector != null )
            track_selector.reload();
        return true;
    }

    return false;
}

