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

// Event sources
var ES_STUDIO=1;
var ES_VENUE=2;
var ES_SCENE=3;
var ES_CHASE=4;
var ES_FIXTURE=5;
var ES_FIXTURE_GROUP=6;
var ES_CHANNEL=7;
var ES_WHITEOUT=8;
var ES_BLACKOUT=9;
var ES_MUTE_BLACKOUT=10;
var ES_TRACK=11;
var ES_PLAYLIST=12;
var ES_VOLUME=13;
var ES_VOLUME_MUTE=14;
var ES_MUSIC_MATCH=15;
var ES_ANIMATION_SPEED=16;
var ES_MUSIC_PLAYER=17;
var ES_MASTER_DIMMER=18;
var ES_WHITEOUT_STROBE=19;
var ES_WHITEOUT_COLOR=20;
var ES_TRACK_QUEUES=21;

var sources = [];

// Event actions
var EA_START=1;
var EA_STOP=2;
var EA_DELETED=3;
var EA_PAUSE=4;
var EA_RESUME=5;
var EA_NEW=6;
var EA_CHANGED=7;
var EA_ERROR=8;
var EA_TIME=9;

var actionNames = [ null, "START", "STOP", "DELETED", "PAUSE", "RESUME", "NEW", "CHANGED", "ERROR", "TIME" ];

// ----------------------------------------------------------------------------
//
function start_event_processing() {
    if ( eventRequestTimer != null )
        clearTimeout( eventRequestTimer );

    if ( eventHandlerTimer != null )
        clearTimeout( eventHandlerTimer );

    sources[ES_STUDIO] = { "name": "STUDIO", "handler": null };
    sources[ES_VENUE] = { "name": "VENUE", "handler": null };
    sources[ES_SCENE] = { "name": "SCENE", "handler": handleSceneEvent };
    sources[ES_CHASE] = { "name": "CHASE", "handler": handleChaseEvent };
    sources[ES_FIXTURE] = { "name": "FIXTURE", "handler": handleFixtureEvent };
    sources[ES_FIXTURE_GROUP] = { "name": "FIXTURE_GROUP", "handler": handleFixtureEvent };
    sources[ES_CHANNEL] = { "name": "CHANNEL", "handler": handleChannelEvent };
    sources[ES_WHITEOUT] = { "name": "WHITEOUT", "handler": handleWhiteoutEvent };
    sources[ES_BLACKOUT] = { "name": "BLACKOUT", "handler": handleBlackoutEvent };
    sources[ES_MUTE_BLACKOUT] = { "name": "MUTE_BLACKOUT", "handler": handleMuteBlackoutEvent };
    sources[ES_TRACK] = { "name": "TRACK", "handler": handleTrackEvent };
    sources[ES_PLAYLIST] = { "name": "PLAYLIST", "handler": null };
    sources[ES_VOLUME] = { "name": "VOLUME", "handler": handleVolumeEvent };
    sources[ES_VOLUME_MUTE] = { "name": "VOLUME_MUTE", "handler": handleVolumeMuteEvent };
    sources[ES_MUSIC_MATCH] = { "name": "MUSIC_MATCH", "handler": handleMusicMatchEvent };
    sources[ES_ANIMATION_SPEED] = { "name": "ANIMATION_SPEED", "handler": handleAnimationSpeedEvent };
    sources[ES_MUSIC_PLAYER] = { "name": "MUSIC_PLAYER", "handler": null };
    sources[ES_MASTER_DIMMER] = { "name": "MASTER_DIMMER", "handler": handleDimmerEvent };
    sources[ES_WHITEOUT_STROBE] = { "name": "WHITEOUT_STROBE", "handler": handleWhiteoutStrobeEvent };
    sources[ES_WHITEOUT_COLOR] = { "name": "WHITEOUT_COLOR", "handler": handleWhiteoutColorEvent };
    sources[ES_TRACK_QUEUES] = { "name": "TRACK_QUEUES", "handler": handleTrackQueuesEvent };

    eventRequestTimer = setTimeout( eventRequest, 100 );
    eventHandlerTimer = setTimeout( eventHandler, 100 );
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

            eventRequestTimer = setTimeout(eventRequest, 100);
        },

        error: function () {
            if (system_status) {
                cache_status_icon.removeClass("ui-icon-green").addClass("ui-icon-red");
                cache_status_icon.attr("title", "status: disconnected");
                system_status = false;
            }

            eventRequestTimer = setTimeout(eventRequest, 1000);
        }
    });
}

// ----------------------------------------------------------------------------
//
function eventHandler() {
    eventHandlerTimer = null;

    while ( pending_events.length > 0 ) {
        var event = pending_events.shift();

        var source = sources[ event.source ];
        if ( source == null ) {
            toastWarning( "UNKNOWN EVENT: " + actionNames[ event.action] + " " + event.source + " UID " + event.uid + 
                                " [" + event.val1 + "] " + event.text );
            continue;
        }

        if ( DEBUG_EVENTS )
            toastNotice( "Event: " + actionNames[ event.action] + " " + source.name + " UID " + event.uid );

        if ( source.handler == null )
            continue;

        var processed = source.handler( event );

        if ( !processed )
            toastWarning( "UNPROCESSED EVENT: " + actionNames[ event.action] + " " + source.name + " UID " + event.uid + 
                                " [" + event.val1 + "] " + event.text );
    }

    eventHandlerTimer = setTimeout( eventHandler, 100 );
}

// ----------------------------------------------------------------------------
//
function handleVolumeEvent( event ) {

    if ( event.action == EA_CHANGED ) {
        updateVolume( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleVolumeMuteEvent( event ) {

    if ( event.action == EA_START || event.action == EA_STOP ) {
        updateVolumeMute( event.action == EA_START );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleDimmerEvent( event ) {
    if ( event.action == EA_CHANGED ) {
        updateMasterDimmer( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleBlackoutEvent( event ) {
    if ( event.action == EA_START || event.action == EA_STOP ) {
        updateBlackout( event.action == EA_START );
        return true;
    }    

    return false;
}

// ----------------------------------------------------------------------------
//
function handleMuteBlackoutEvent( event ) {
    if ( event.action == EA_START || event.action == EA_STOP ) {
        updateMuteBlackout( event.action == EA_START );
        return true;
    }    

    if ( event.action == EA_CHANGED ) {
        return true;
    } 

    return false;
}

// ----------------------------------------------------------------------------
//
function handleMusicMatchEvent( event ) {
    if ( event.action == EA_START || event.action == EA_STOP ) {
        updateMusicMatch( event.action == EA_START );
        return true;
    }   

    if ( event.action == EA_CHANGED ) {
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleAnimationSpeedEvent( event ) {
    if ( event.action == EA_CHANGED ) {
        updateAnimationSpeed( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutEvent( event ) {

    if ( event.action == EA_CHANGED ) {
        updateWhiteout( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutStrobeEvent( event ) {

    if ( event.action == EA_CHANGED ) {
        updateWhiteoutStrobe( event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleWhiteoutColorEvent( event ) {
    if ( event.action == EA_CHANGED ) {
        updateWhiteoutColor( event.text );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleSceneEvent( event ) {
    if ( event.action == EA_START ) {
        markActiveScene( event.uid );
        return true;
    }

    if ( event.action == EA_STOP ) {
        markActiveScene( 0 );
        return true;
    }

    if ( event.action == EA_NEW ) {
        newSceneEvent( event.uid );
        return true;
    }

    if ( event.action == EA_DELETED ) {
        deleteSceneEvent( event.uid );
        return true;
    }

    if ( event.action == EA_CHANGED ) {
        changeSceneEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleChaseEvent( event ) {
    if ( event.action == EA_START ) {
        markActiveChase( event.uid );
        return true;
    }

    if ( event.action == EA_STOP ) {
        markActiveChase( 0 );
        return true;
    }

    if ( event.action == EA_NEW ) {
        newChaseEvent( event.uid );
        return true;
    }

    if ( event.action == EA_DELETED ) {
        deleteChaseEvent( event.uid );
        return true;
    }

    if ( event.action == EA_CHANGED ) {
        changeChaseEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleFixtureEvent( event ) {
    if ( event.action == EA_NEW ) {
        newFixureEvent( event.uid );
        return true;
    }

    if ( event.action == EA_DELETED ) {
        deleteFixtureEvent( event.uid );
        return true;
    }

    if ( event.action == EA_CHANGED ) {
        changeFixtureEvent( event.uid );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleChannelEvent( event ) {

    if ( event.action == EA_CHANGED ) {
        changeChannelEvent( event.uid, event.val1, event.val2 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleTrackEvent( event ) {
    if ( event.action == EA_START ) {
        startTrackEvent( event.text, event.val1 );
        return true;
    }

    if ( event.action == EA_STOP ) {
        stopTrackEvent( event.text );
        return true;
    }

    if ( event.action == EA_PAUSE ) {
        pauseTrackEvent( event.text );
        return true;
    }

    if ( event.action == EA_RESUME ) {
        resumeTrackEvent( event.text );
        return true;
    }

    if ( event.action == EA_TIME ) {
        timeTrackEvent( event.text, event.val1 );
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function handleTrackQueuesEvent( event ) {
    if ( event.action == EA_CHANGED ) {
        changeTrackQueueEvent( event.val1, event.val2 );
        return true;
    } 

    return false;
}



