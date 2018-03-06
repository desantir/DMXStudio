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
var pending_events = [];
var eventRequestTimer = null;
var eventHandlerTimer = null;
var eventListeners = [];       // All events will also be sent to all registered "child" window listeners

var eventHandlers = [ EventSourceType.ES_NUM_SOURCES ];

eventHandlers[EventSourceType.ES_STUDIO] = handleStudioEvent;
eventHandlers[EventSourceType.ES_VENUE] = handleVenueEvent;
eventHandlers[EventSourceType.ES_SCENE] = handleSceneEvent;
eventHandlers[EventSourceType.ES_CHASE] = handleChaseEvent;
eventHandlers[EventSourceType.ES_FIXTURE] = handleFixtureEvent;
eventHandlers[EventSourceType.ES_FIXTURE_GROUP] = handleFixtureEvent;
eventHandlers[EventSourceType.ES_CHANNEL] = handleChannelEvent;
eventHandlers[EventSourceType.ES_WHITEOUT] = handleWhiteoutEvent;
eventHandlers[EventSourceType.ES_BLACKOUT] = handleBlackoutEvent;
eventHandlers[EventSourceType.ES_MUTE_BLACKOUT] = handleMuteBlackoutEvent;
eventHandlers[EventSourceType.ES_TRACK] = handleTrackEvent;
eventHandlers[EventSourceType.ES_PLAYLIST] = null;
eventHandlers[EventSourceType.ES_VOLUME] = handleVolumeEvent;
eventHandlers[EventSourceType.ES_VOLUME_MUTE] = handleVolumeMuteEvent;
eventHandlers[EventSourceType.ES_MUSIC_MATCH] = handleMusicMatchEvent;
eventHandlers[EventSourceType.ES_ANIMATION_SPEED] = handleAnimationSpeedEvent;
eventHandlers[EventSourceType.ES_MUSIC_PLAYER] = null;
eventHandlers[EventSourceType.ES_MASTER_DIMMER] = handleDimmerEvent;
eventHandlers[EventSourceType.ES_WHITEOUT_STROBE] = handleWhiteoutStrobeEvent;
eventHandlers[EventSourceType.ES_WHITEOUT_COLOR] = handleWhiteoutColorEvent;
eventHandlers[EventSourceType.ES_TRACK_QUEUES] = handleTrackQueuesEvent;
eventHandlers[EventSourceType.ES_ANIMATION] = handleAnimationEvent;
eventHandlers[EventSourceType.ES_PALETTE] = handlePaletteEvent;
eventHandlers[EventSourceType.ES_WHITEOUT_EFFECT] = handleWhiteoutEffectEvent;
eventHandlers[EventSourceType.ES_VIDEO_PALETTE] = null;
eventHandlers[EventSourceType.ES_FIXTURE_STATUS] = handleFixtureStatusEvent;

// ----------------------------------------------------------------------------
//
function start_event_processing() {
    if ( eventRequestTimer != null )
        clearTimeout( eventRequestTimer );

    if ( eventHandlerTimer != null )
        clearTimeout( eventHandlerTimer );

    eventRequestTimer = setTimeout( eventRequest, 100 );
}

// ----------------------------------------------------------------------------
//
function eventRequest() {
    eventRequestTimer = null;

    // Push venue configuration changes to the server if needed
    if ( client_config_update ) {
        saveVenueLayout();
    }

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/events",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            if (!system_status) {
                startUserInterface();

                // Start UI will also restart event pinger
                return;
            }

            pending_events = pending_events.concat( json.events );

            if ( eventHandlerTimer == null && pending_events.length > 0 )
                eventHandlerTimer = setTimeout( eventHandler, 50 );

            eventRequestTimer = setTimeout(eventRequest, 100);
        },

        error: function () {
            startUserInterface();
        }
    });
}

// ----------------------------------------------------------------------------
//
function eventHandler() {
    eventHandlerTimer = null;

    while ( pending_events.length > 0 ) {
        var event = pending_events.shift();

        if ( DEBUG_EVENTS )
            toastEvent( event );

        var handler = (event.source > 0 && event.source < EventSourceType.ES_NUM_SOURCES ) ? eventHandlers[ event.source ] : null;

        if ( handler != null && handler != undefined ) {
            if ( !handler( event ) )
                toastWarning( "UNPROCESSED EVENT ACTION: " + eventActionNames[ event.action] + " " + eventSourceNames[ event.source ] + 
                              " UID " + event.uid + " [" + event.val1 + "] " + event.text );
        }

        broadcastEvent( event );
    }
}

// ----------------------------------------------------------------------------
//
function broadcastEvent( event ) {
    for ( var index=0; index < eventListeners.length; ) {
        var listener = eventListeners[index];

        if ( !listener.closed ) {
            listener.postMessage( { "method": "eventBroadcast", "event": event }, "*" );
            index++;
        }
        else {
            eventListeners.splice( index, 1 );
        }
    }
}

// ----------------------------------------------------------------------------
//
function registerEventListener( listener ) {
    // Make sure the window is not already registered
    for ( var index=0; index < eventListeners.length; index++ )
        if ( eventListeners[index] == listener )
            return;

    eventListeners.push( listener );
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
function handleVenueEvent( event ) {

    if ( event.action == EventActionType.EA_START ) {
        if ( eventRequestTimer != null )
            clearTimeout( eventRequestTimer );

        if ( eventHandlerTimer != null )
            clearTimeout( eventHandlerTimer );

        pending_events = [];

        startUserInterface();
        return true;
    }

    if ( event.action == EventActionType.EA_STOP || event.action == EventActionType.EA_CHANGED ) {
        // Ignore
        return true;
    }

    return false;
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
function handleDimmerEvent( event ) {
    if ( event.action == EventActionType.EA_CHANGED ) {
        updateMasterDimmer( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleBlackoutEvent( event ) {
    if ( event.action == EventActionType.EA_START || event.action == EventActionType.EA_STOP ) {
        updateBlackout( event.action == EventActionType.EA_START );
        return true;
    }    

    return false;
}

// ----------------------------------------------------------------------------
//
function handleMuteBlackoutEvent( event ) {
    if ( event.action == EventActionType.EA_START || event.action == EventActionType.EA_STOP ) {
        updateMuteBlackout( event.action == EventActionType.EA_START );
        return true;
    }    

    if ( event.action == EventActionType.EA_CHANGED ) {
        return true;
    } 

    return false;
}

// ----------------------------------------------------------------------------
//
function handleMusicMatchEvent( event ) {
    if ( event.action == EventActionType.EA_START || event.action == EventActionType.EA_STOP ) {
        updateMusicMatch( event.action == EventActionType.EA_START );
        return true;
    }   

    if ( event.action == EventActionType.EA_CHANGED ) {
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutEvent( event ) {

    if ( event.action == EventActionType.EA_CHANGED ) {
        updateWhiteout( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutEffectEvent( event ) {

    if ( event.action == EventActionType.EA_CHANGED ) {
        updateWhiteoutEffect( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutStrobeEvent( event ) {

    if ( event.action == EventActionType.EA_CHANGED ) {
        updateWhiteoutStrobe( event.val1, event.val2 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutColorEvent( event ) {
    if ( event.action == EventActionType.EA_CHANGED ) {
        updateWhiteoutColor( event.text );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleAnimationSpeedEvent( event ) {

    if ( event.action == EventActionType.EA_CHANGED ) {
        updateAnimationSpeed( event.val1 );
        return true;
    }

    return false;
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

// ----------------------------------------------------------------------------
//
function handleSceneEvent( event ) {
    if ( event.action == EventActionType.EA_START ) {
        markActiveScene( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_STOP ) {
        markActiveScene( 0 );
        return true;
    }

    if ( event.action == EventActionType.EA_NEW ) {
        newSceneEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_DELETED ) {
        deleteSceneEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_CHANGED ) {
        changeSceneEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleChaseEvent( event ) {
    if ( event.action == EventActionType.EA_START ) {
        markActiveChase( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_STOP ) {
        markActiveChase( 0 );
        return true;
    }

    if ( event.action == EventActionType.EA_NEW ) {
        newChaseEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_DELETED ) {
        deleteChaseEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_CHANGED ) {
        changeChaseEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleFixtureEvent( event ) {
    if ( event.action == EventActionType.EA_NEW ) {
        newFixtureEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_DELETED ) {
        deleteFixtureEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_CHANGED ) {
        changeFixtureEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleAnimationEvent( event ) {
    if ( event.action == EventActionType.EA_START ) {
        markActiveAnimation( event.uid, true );
        return true;
    }

    if ( event.action == EventActionType.EA_STOP ) {
        markActiveAnimation( event.uid, false );
        return true;
    }

    if ( event.action == EventActionType.EA_NEW ) {
        newAnimationEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_DELETED ) {
        deleteAnimationEvent( event.uid );
        return true;
    }

    if ( event.action == EventActionType.EA_CHANGED ) {
        changeAnimationEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleChannelEvent( event ) {

    if ( event.action == EventActionType.EA_CHANGED ) {
        changeChannelEvent( event.uid, event.val1, event.val2, event.val3 );
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
function handleFixtureStatusEvent( event ) {
    if (event.action == EventActionType.EA_CHANGED) {
        changeFixtureStatusEvent( event.uid, event.val1, event.val2, event.val3 );
        return true;
    } 

    return false;
}

