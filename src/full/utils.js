/* 
Copyright (C) 2012-2017 Robert DeSantis
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

var PLAYED_PLAYLIST_LINK = "local:playlist:played";
var QUEUED_PLAYLIST_LINK = "local:playlist:queued";

// Event action type
var EventActionType = {
     EA_START: 1,
     EA_STOP: 2,
     EA_DELETED: 3,
     EA_PAUSE: 4,
     EA_RESUME: 5,
     EA_NEW: 6,
     EA_CHANGED: 7,
     EA_ERROR: 8,
     EA_TIME: 9,
     EA_MESSAGE: 10,
     EA_NUM_ACTIONS: 10
};

var eventActionNames = [ null, "START", "STOP", "DELETED", "PAUSE", "RESUME", "NEW", "CHANGED", "ERROR", "TIME" ];

// Event source type
var EventSourceType = {
    ES_STUDIO: 1,
    ES_VENUE: 2,
    ES_SCENE: 3,
    ES_CHASE: 4,
    ES_FIXTURE: 5,
    ES_FIXTURE_GROUP: 6,
    ES_CHANNEL: 7,
    ES_WHITEOUT: 8,
    ES_BLACKOUT: 9,
    ES_MUTE_BLACKOUT: 10,
    ES_TRACK: 11,
    ES_PLAYLIST: 12,
    ES_VOLUME: 13,
    ES_VOLUME_MUTE: 14,
    ES_MUSIC_MATCH: 15,
    ES_ANIMATION_SPEED: 16,
    ES_MUSIC_PLAYER: 17,
    ES_MASTER_DIMMER: 18,
    ES_WHITEOUT_STROBE: 19,
    ES_WHITEOUT_COLOR: 20,
    ES_TRACK_QUEUES: 21,
    ES_ANIMATION: 22,
    ES_PALETTE: 23,
    ES_WHITEOUT_EFFECT: 24,
    ES_VIDEO_PALETTE: 25,
    ES_FIXTURE_STATUS: 26,
    ES_NUM_SOURCES: 27
}

var eventSourceNames = [];

eventSourceNames[EventSourceType.ES_STUDIO] = "STUDIO";
eventSourceNames[EventSourceType.ES_VENUE] = "VENUE";
eventSourceNames[EventSourceType.ES_SCENE] = "SCENE";
eventSourceNames[EventSourceType.ES_CHASE] = "CHASE";
eventSourceNames[EventSourceType.ES_FIXTURE] = "FIXTURE";
eventSourceNames[EventSourceType.ES_FIXTURE_GROUP] = "FIXTURE_GROUP";
eventSourceNames[EventSourceType.ES_CHANNEL] = "CHANNEL";
eventSourceNames[EventSourceType.ES_WHITEOUT] = "WHITEOUT";
eventSourceNames[EventSourceType.ES_BLACKOUT] = "BLACKOUT";
eventSourceNames[EventSourceType.ES_MUTE_BLACKOUT] = "MUTE_BLACKOUT";
eventSourceNames[EventSourceType.ES_TRACK] = "TRACK";
eventSourceNames[EventSourceType.ES_PLAYLIST] = "PLAYLIST";
eventSourceNames[EventSourceType.ES_VOLUME] = "VOLUME";
eventSourceNames[EventSourceType.ES_VOLUME_MUTE] = "VOLUME_MUTE";
eventSourceNames[EventSourceType.ES_MUSIC_MATCH] = "MUSIC_MATCH";
eventSourceNames[EventSourceType.ES_UNUSED] = "UNUSED";
eventSourceNames[EventSourceType.ES_MUSIC_PLAYER] = "MUSIC_PLAYER";
eventSourceNames[EventSourceType.ES_MASTER_DIMMER] = "MASTER_DIMMER";
eventSourceNames[EventSourceType.ES_WHITEOUT_STROBE] = "WHITEOUT_STROBE";
eventSourceNames[EventSourceType.ES_WHITEOUT_COLOR] = "WHITEOUT_COLOR";
eventSourceNames[EventSourceType.ES_TRACK_QUEUES] = "TRACK_QUEUES";
eventSourceNames[EventSourceType.ES_ANIMATION] = "ANIMATION";
eventSourceNames[EventSourceType.ES_PALETTE] = "PALETTE";
eventSourceNames[EventSourceType.ES_ANIMATION_SPEED] = "ANIMATION_SPEED";
eventSourceNames[EventSourceType.ES_WHITEOUT_EFFECT] = "WHITEOUT_EFFECT";

var PaletteType = {
    PT_UNSPECIFIED: 0,
    PT_LOCATION: 1,
    PT_COLOR: 2,
    PT_DIMMER: 3,
    PT_STROBE: 4,
    PT_UNUSED: 5,
    PT_GOBO: 6,
    PT_FIXTURE_PRESET: 7,
    PT_COLOR_PALETTE: 8
};

var ChannelType = {
    CHNLT_UNKNOWN: 0,
    CHNLT_RED: 1,
    CHNLT_GREEN: 2,
    CHNLT_BLUE: 3,
    CHNLT_AMBER: 4,
    CHNLT_WHITE: 5,
    CHNLT_DIMMER: 6,
    CHNLT_STROBE: 7,
    CHNLT_GOBO: 8,
    CHNLT_PAN: 9,
    CHNLT_TILT: 10,
    CHNLT_AUTOPROG: 11,
    CHNLT_COLOR_MACRO: 12,
    CHNLT_PAN_FINE: 13,
    CHNLT_TILT_FINE: 14,
    CHNLT_MOVEMENT_SPEED: 15,
    CHNLT_COLOR_SPEED: 16,
    CHNLT_MOVEMENT_MACRO: 17,
    CHNLT_CONTROL: 18,
    CHNLT_DIMMER_AND_STROBE: 19,
    CHNLT_PROG_SPEED: 20,
    CHNLT_LASER: 21,
    CHNLT_UV: 22
};

var channelTypeNames = [];

channelTypeNames[ChannelType.CHNLT_UNKNOWN] = "UNKNOWN";
channelTypeNames[ChannelType.CHNLT_RED] = "Red";
channelTypeNames[ChannelType.CHNLT_GREEN] = "Green";
channelTypeNames[ChannelType.CHNLT_BLUE] = "Blue";
channelTypeNames[ChannelType.CHNLT_AMBER] = "Amber";
channelTypeNames[ChannelType.CHNLT_WHITE] = "White";
channelTypeNames[ChannelType.CHNLT_DIMMER] = "Dimmer";
channelTypeNames[ChannelType.CHNLT_STROBE] = "Strobe";
channelTypeNames[ChannelType.CHNLT_GOBO] = "GOBO";
channelTypeNames[ChannelType.CHNLT_PAN] = "Pan";
channelTypeNames[ChannelType.CHNLT_TILT] = "Tilt";
channelTypeNames[ChannelType.CHNLT_AUTOPROG] = "Auto program";
channelTypeNames[ChannelType.CHNLT_COLOR_MACRO] = "Color macro";
channelTypeNames[ChannelType.CHNLT_PAN_FINE] = "Pan fine";
channelTypeNames[ChannelType.CHNLT_TILT_FINE] = "Tilt Fine";
channelTypeNames[ChannelType.CHNLT_MOVEMENT_SPEED] = "Movement speed";
channelTypeNames[ChannelType.CHNLT_COLOR_SPEED] = "Color speed";
channelTypeNames[ChannelType.CHNLT_MOVEMENT_MACRO] = "Movement macro";
channelTypeNames[ChannelType.CHNLT_CONTROL] = "Control";
channelTypeNames[ChannelType.CHNLT_DIMMER_AND_STROBE] = "Dimmer/Strobe";
channelTypeNames[ChannelType.CHNLT_PROG_SPEED] = "Program Speed";
channelTypeNames[ChannelType.CHNLT_LASER] = "Laser";
channelTypeNames[ChannelType.CHNLT_UV] = "UV";

var pendingUpdates = [];

// ----------------------------------------------------------------------------
//
function delayUpdate( id, timeout_ms, func ) {
    if ( pendingUpdates[ id ] != null )
        clearTimeout( pendingUpdates[ id ] );

    pendingUpdates[ id ] = setTimeout( function () {
        pendingUpdates[ id ] = null;
        func();
    }, timeout_ms );
}

// ----------------------------------------------------------------------------
//
function messageBox(message) {
    $("#message_box_dialog").dialog({
        title: "DMXStudio",
        autoOpen: true,
        width: 640,
        height: 260,
        modal: true,
        resizable: false,
        closeOnEscape: true,
        hide: {
            effect: "explode",
            duration: 500
        },
        buttons: {
            "OK": function () {
                $(this).dialog("close");
            }
        }
    });

    $("#mbd_contents").text(message);

    $("#message_box_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function questionBox( title, question, callback ) {
    $("#message_box_dialog").dialog({
        title: title,
        autoOpen: true,
        width: 540,
        height: 260,
        modal: true,
        resizable: false,
        closeOnEscape: true,
        buttons: {
            "Yes": function () {
                $(this).dialog("close");
                callback( true );
            },
            "No": function() {
                $(this).dialog("close");
                callback( false );
            }
        }
    });

    $("#mbd_contents").text(question);

    $("#message_box_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function toastNotice( message ) {
    return $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 5000,
        sticky   : false,
        position : 'top-right',
        type     : 'notice',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function toastWarning( message ) {
    return  $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 5000,
        sticky   : false,
        position : 'top-right',
        type     : 'warning',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function toastError( message ) {
    var callback = null;

    if ( arguments.length == 2 )
        callback = arguments[1];

    return  $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 3000,
        sticky   : true,
        position : 'top-right',
        type     : 'error',
        close    : callback
    });
}

// ----------------------------------------------------------------------------
//
function stopEventPropagation(event) {
    event = (event || window.event);

    if ( event != null ) {
        if (event.stopPropagation != null)
            event.stopPropagation();
        else
            event.cancelBubble = true;
    }
}

// ----------------------------------------------------------------------------
//
function put_user_on_ice(reason) {
    $("#wd_reason").text(reason);

    var ice_dialog = $("#wait_dialog").dialog({
        autoOpen: true,
        width: 300,
        height: 220,
        modal: true,
        resizable: false,
        closeOnEscape: false,
        open: function() { $(this).dialog("widget").find(".ui-dialog-titlebar").hide(); }
    });

    return ice_dialog;
}

// ----------------------------------------------------------------------------
//
function onAjaxError( jqXHR, textError, errorThrown ) {
    var title = (textError != null && textError != undefined) ? textError.toUpperCase() : "UNKNOWN ERROR";

    messageBox( title + ": Unable to complete server operation [" + errorThrown + " ]");
}

// ----------------------------------------------------------------------------
// @see http://stackoverflow.com/questions/5796718/html-entity-decode for a better implementation
//
function unencode(entity_ridden_string) {
    if (entity_ridden_string == null)
        return null;

    entity_ridden_string = entity_ridden_string.replace(/&amp;/g, "&");
    entity_ridden_string = entity_ridden_string.replace(/&lt;/g, "<");
    entity_ridden_string = entity_ridden_string.replace(/&gt;/g, ">");
    entity_ridden_string = entity_ridden_string.replace(/&quot;/g, "\"");
    entity_ridden_string = entity_ridden_string.replace(/&lsquo;/g, "'");
    entity_ridden_string = entity_ridden_string.replace(/\\/g, "\\");

    return entity_ridden_string;
}

// ----------------------------------------------------------------------------
//
function escapeForHTML(text) {
    return escape(text).replace(/%(..)/g, "&#x$1;");
}

// ----------------------------------------------------------------------------
//
function multiselectSelect( control, value ) {

    var o = control.find("option[value='" + value + "']");

    if (o != null && o.length == 1 ) {
        o.attr("selected", true);
        control.multiselect('refresh');
        return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
//
function getNextUnusedNumber( objects ) {
    var used_numbers = [];
    var max_created = 0;
    var start_number = 0;

    for ( var i=0; i < objects.length; i++ ) {
        used_numbers[ used_numbers.length ] = objects[i].number;
        if ( objects[i].created > max_created ) {
            start_number = objects[i].number;
            max_created = objects[i].created;
        }
    }

    while ( ++ start_number < 50000 ) {
        if ( used_numbers.indexOf( start_number ) == -1 )
            return start_number;
    }

    return 99999;
}

// ----------------------------------------------------------------------------
//
function toastEvent( event ) {
    var actionName = ( event.action >= 1 && event.action < EventActionType.EA_NUM_ACTIONS ) ? eventActionNames[ event.action] : "UNKNOWN";
    var sourceName = ( event.source >= 1 && event.source < EventSourceType.ES_NUM_SOURCES ) ? eventSourceNames[ event.source ] : "UNKNOWN";

    toastNotice( "Event: " + actionName + " " + sourceName + " UID " + event.uid );
}

// ----------------------------------------------------------------------------
//
function panelPanTiltHandler( target_slider_panel, control ) {
    $("#pan_tilt_popup").dialog({
        autoOpen: false,
        show: { effect: 'fade', speed: 500 },
        hide: { effect: 'fade', speed: 500 },
        width: 340,
        height: 210,
        modal: false,
        resizable: false,
        closeOnEscape: true,
        dialogClass: "titlelessDialog",
        position: { my: "right top", at: "left bottom", of: control },
        open: function (event, ui) {
            $(this).parent().css('height', '190px');
            $(document).bind('mousedown', function (event) {
                stopEventPropagation(event);

                // If click on a child of the dialog, ignore it
                for ( var parent=event.target.parentNode; parent != null && parent != undefined && parent != document; parent = parent.parentNode )
                    if ( parent == pan_tilt_popup )
                        return;

                $("#pan_tilt_popup").dialog('close').data('hide_ms', new Date().getTime());
            });

            control.removeClass('ui-icon').addClass('ui-icon-white');
        },
        close: function (event, ui) {
            $(document).unbind('mousedown');

            control.removeClass('ui-icon-white').addClass('ui-icon');
        }
    });

    $("#ptp_control").pan_tilt_control({
        pan_start: 0,
        pan_end: 540,
        tilt_start: 0,
        tilt_end: 360,
        pan: 180,
        tilt: 180,
        width: 320,
        background_color: "black",
        pan_color: "#444444",
        tilt_color: "#808080",
        text_color: "black",
        marker_color: "red",
        onchange: function (location) {
            $("#ptp_pan").val(location.pan);
            $("#ptp_tilt").val(location.tilt);

            delayUpdate( "pan", 250, function () { 
                setPanTiltChannels(target_slider_panel, location.pan, location.tilt);
            } );
        }
    });

    function set_degrees(event, ui) {
        stopEventPropagation(event);

        var pan_degrees = parseInt($("#ptp_pan").spinner('value'));
        var tilt_degrees = parseInt($("#ptp_tilt").spinner('value'));

        $("#ptp_control").pan_tilt_control('set_location', { pan: pan_degrees, tilt: tilt_degrees });

        setPanTiltChannels(target_slider_panel, pan_degrees, tilt_degrees);
    }

    $("#ptp_pan").spinner({
        min: $("#ptp_control").pan_tilt_control('option', 'pan_start'),
        max: $("#ptp_control").pan_tilt_control('option', 'pan_end'),
        spin: set_degrees,
        change: set_degrees
    }).val($("#ptp_control").pan_tilt_control('option','pan'));


    $("#ptp_tilt").spinner({
        min: $("#ptp_control").pan_tilt_control('option', 'tilt_start'),
        max: $("#ptp_control").pan_tilt_control('option', 'tilt_end'),
        spin: set_degrees,
        change: set_degrees
    }).val($("#ptp_control").pan_tilt_control('option', 'tilt'));

    var hide_ms = $("#pan_tilt_popup").data('hide_ms');
    if (hide_ms != null) {
        var now = new Date().getTime() - 250;
        // Stop the "bounce" when closing the dialog
        if (hide_ms > now)
            return;   // Stops any addition jQuery event handlers
    }

    if ($("#pan_tilt_popup").dialog("isOpen")) {
        $("#pan_tilt_popup").dialog("close");
        return;
    }

    $("#pan_tilt_popup").dialog("open");

    var pan_degrees = parseInt($("#ptp_pan").spinner('value'));
    var tilt_degrees = parseInt($("#ptp_tilt").spinner('value'));

    setPanTiltChannels( target_slider_panel, pan_degrees, tilt_degrees);
}

// ----------------------------------------------------------------------------
//
function setPanTiltChannels( target_slider_panel, pan, tilt) {
    function getValue(slider, target_degrees) {
        var ranges = slider.getRanges();
        if (ranges != null) {
            var i;

            for (i = 0; i < ranges.length; i++) {
                var range = ranges[i];
                var degrees = range.extra;
                if (target_degrees < degrees) {
                    if (i > 0)
                        i--;
                    return ranges[i].start;
                }
            }
        }

        return slider.getValue();
    }

    target_slider_panel.iterateChannels(function (slider) {
        switch (slider.getType()) {
            case ChannelType.CHNLT_PAN:
                slider.setValue(getValue(slider,pan));
                break;
            case ChannelType.CHNLT_TILT:
                slider.setValue(getValue(slider, tilt));
                break;
        }
    });
}

// ----------------------------------------------------------------------------
//
function attachPanelColorPicker( target_slider_panel, control ) {
    control.ColorPicker({
        color: "#FFFFFF",
        livePreview: true,
        showPalettes: false,

        onShow: function (picker) {
            var color = getColorChannelRGB( target_slider_panel );
            if ( color != null )
                control.ColorPickerSetColor(color);
            control.removeClass('ui-icon').addClass('ui-icon-white');

            $(picker).ColorPickerSetColorChips( getColorChips() );
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            control.removeClass('ui-icon-white').addClass('ui-icon');

            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            setColorChannelRGB(target_slider_panel, hsb, hex, rgb);
        },

        onChange: function (hsb, hex, rgb) {
            setColorChannelRGB(target_slider_panel, hsb, hex, rgb);
        },

        onChipSet: function( chip, hsb, hex, rgb ) {
            return setColorChip( chip, hex );
        }
    });
}

// ----------------------------------------------------------------------------
//
function getColorChannelRGB( target_slider_panel ) {
    var red = 0;
    var blue = 0;
    var green = 0;

    target_slider_panel.iterateChannels(function (slider) {
        switch (slider.getType()) {
            case ChannelType.CHNLT_RED:
                red = slider.getValue();
                break;
            case ChannelType.CHNLT_GREEN:
                green = slider.getValue();
                break;
            case ChannelType.CHNLT_BLUE:
                blue = slider.getValue();
                break;
        }
    });

    if (red > 0 || green > 0 || blue > 0)
       return makeHexRGB( red, green, blue );

    return null;
}

// ----------------------------------------------------------------------------
//
function rgb2hex( rgb ) {
    colors = rgb.substr( 4, rgb.length-5).split( "," );

    return makeHexRGB( parseInt( colors[0] ), parseInt( colors[2] ), parseInt( colors[2] ) );
}

// ----------------------------------------------------------------------------
//
function rgbLong2hex( value ) {
    return makeHexRGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF )
}


// ----------------------------------------------------------------------------
//
function makeHexRGB( red, green, blue ) {
	var hex = [ red.toString(16), green.toString(16), blue.toString(16) ];

	$.each(hex, function (nr, val) {
		if (val.length == 1) {
			hex[nr] = '0' + val;
		}
	});

	return "#" + hex.join('');
}

// ----------------------------------------------------------------------------
//
function setColorChannelRGB( target_slider_panel, hsb, hex, rgb ) {
    target_slider_panel.iterateChannels(function (slider) {
        switch (slider.getType()) {
            case ChannelType.CHNLT_RED:
                slider.setValue(rgb.r);
                break;
            case ChannelType.CHNLT_GREEN:
                slider.setValue(rgb.g);
                break;
            case ChannelType.CHNLT_BLUE:
                slider.setValue(rgb.b);
                break;
        }
    });
}

// ----------------------------------------------------------------------------
//
function rotateIcon( icon, duration ) {
    icon.css('rotation', 0);      // Reset rotation

    icon.animate(
        { rotation: 360 },
        {
            duration: duration,
            step: function (now, fx) {
                $(this).css({ "transform": "rotate(" + now + "deg)" });
            }
        }
    );
}

// ----------------------------------------------------------------------------
//
function rotateX( icon, duration ) {
    icon.css('rotation', 0);      // Reset rotation

    icon.animate(
        { rotation: 360 },
        {
            duration: duration,
            step: function (now, fx) {
                $(this).css({ "transform": "rotateX(" + now + "deg)" });
            }
        }
    );
}

// ----------------------------------------------------------------------------
//
function rotateY( icon, duration ) {
    icon.css('rotation', 0);      // Reset rotation

    icon.animate(
        { rotation: 360 },
        {
            duration: duration,
            step: function (now, fx) {
                $(this).css({ "transform": "rotateY(" + now + "deg)" });
            }
        }
    );
}

// ----------------------------------------------------------------------------
//
function initializeCollapsableSections() {
    $(document.body).find(".collapsable_section").each(function (index, collapsable_section) {
        var collapse_icon = $(collapsable_section).find(".collapsable_icon");
        var collapse_content = $(collapsable_section).find(".collapsable_content");

        if (collapse_icon == null || collapse_content == null)
            return;

        collapse_icon.addClass('ui-icon ui-icon-triangle-1-s');
        collapse_icon.attr( 'title', 'hide');

        collapse_icon.click(function (event) {
            stopEventPropagation(event);
            _expand_collapse_section(collapsable_section, collapse_content.is(":visible"));
            client_config_update = true;
        });
    } );
}

// ----------------------------------------------------------------------------
//
function initializeHorizontalSlider(name, min_value, max_value, init_value) {
    name = "#" + name;

    $( name ).slider({
        orientation: "horizontal",
        min: min_value,
        max: max_value,
        value: init_value,
        animate: true,
        change: function (event, ui) {
            $(name + "_value").html( ui.value );
        },
        slide: function (event, ui ) {
            $(name + "_value").html( ui.value );
        }
    });
}

// ----------------------------------------------------------------------------
//
function isSectionCollapsed( section_id ) {
    var collapse_content = $(document.body).find("#" + section_id + " .collapsable_content");
    if (collapse_content == null)
        return false;

    return !collapse_content.is(":visible");
}

// ----------------------------------------------------------------------------
//
function setSectionCollapsed(section_id,collapsed) {
    var collapsable_section = $(document.body).find("#" + section_id);
    if (collapsable_section == null)
        return;

    _expand_collapse_section(collapsable_section, collapsed);
}

function _expand_collapse_section(collapsable_section, collapsed) {
    var collapse_icon = $(collapsable_section).find(".collapsable_icon");
    var collapse_content = $(collapsable_section).find(".collapsable_content");

    if (collapsed) {
        collapse_icon.removeClass('ui-icon-triangle-1-s').addClass('ui-icon-triangle-1-e');
        collapse_icon.attr('title', 'show');
        collapse_content.hide();
    }
    else {
        collapse_icon.removeClass('ui-icon-triangle-1-e').addClass('ui-icon-triangle-1-s');
        collapse_icon.attr('title', 'hide');
        collapse_content.show();
    }
}

// ----------------------------------------------------------------------------
//
function trackTime(time,show_frac) {
    var track_time = "";
    if (time > 60 * 60 * 1000) {
        track_time += Math.floor(time / (60 * 60 * 1000));
        track_time += ":";
        time %= (60 * 60 * 1000);
    }

    track_time += Math.floor(time / (60 * 1000));
    track_time += ":";
    time %= (60 * 1000);

    var seconds = Math.floor(time / (1000));
    if (seconds < 10)
        track_time += "0"
    track_time += seconds;

    if ( show_frac ) {
        time -= seconds * 1000;
        var frac = time / 100;
        track_time += "." + frac;
    }

    return track_time;
}

// ----------------------------------------------------------------------------
//
function clickAction( jqelement, object, func /*, function arguments */ ) {
    jqelement.unbind( "click" );

    var function_args = [];

    for ( var i=3; i < arguments.length; i++ )
        function_args.push( arguments[i] );

    jqelement.click( function( event ) {
        var newargs = [ event ];
        Array.prototype.push.apply( newargs, function_args );
        func.apply( object, newargs );
    } );
}

// ----------------------------------------------------------------------------
//
function getUrlParameter(name) {
    name = name.replace(/[\[]/, '\\[').replace(/[\]]/, '\\]');
    var regex = new RegExp('[\\?&]' + name + '=([^&#]*)');
    var results = regex.exec(location.search);
    return results === null ? null : decodeURIComponent(results[1].replace(/\+/g, ' '));
};

// ----------------------------------------------------------------------------
//
function updateColorChip( chip, color, top_padding ) {
    var chip_name = null;
    var palette_number = 0;

    if ( color != null && color.substr( 0, 2 ) == "rgb" )
        color = rgb2hex( color );

    if ( color == "#010101" ) {
        chip_name = "pal1";
        palette_number = 10000;
    }
    else if ( color == "#010102" ) {
        chip_name = "pal2";
        palette_number = 10001;
    }
    else if ( color == "#010103" ) {
        chip_name = "pal3";
        palette_number = 10002;
    }
    else if ( color == "#010104" ) {
        chip_name = "pal4";
        palette_number = 10003;
    }
    else if ( color == "#010201" ) {
        chip_name = "usr1";
        palette_number = 10004;
    }
    else if ( color == "#010202" ) {
        chip_name = "usr2";
        palette_number = 10005;
    }
    else if ( color == "#010203" ) {
        chip_name = "usr3";
        palette_number = 10006;
    }
    else if ( color == "#010204" ) {
        chip_name = "usr4";
        palette_number = 10007;
    }

    var html = '';
    var palette = ( palette_number != 0 ) ? getPaletteByNumber( palette_number ) : null;

    if ( palette != null && palette.getType() == PaletteType.PT_COLOR_PALETTE && palette.getPaletteColors().length > 0 ) 
        html = makePaletteChip( chip, palette.getName(), palette.getPaletteColors(), palette.getPaletteWeights() );
    else if ( chip_name != null )
        html = '<div style="padding-top:' + top_padding + 'px">' + chip_name + '</div>';

    chip.html( html );
    chip.css( 'background-color', color );
}

// ----------------------------------------------------------------------------
//
function makePaletteChip( chip, name, colors, weights ) {
    var width = chip.innerWidth();
    var height = chip.innerHeight();

    if ( weights != null && weights != undefined && weights.length == colors.length )
        return makeWeightedPaletteChip( colors, weights, height, width );

    var color_width = Math.max( 1, width / colors.length );

    var html = '<div style="position: relative; width: ' + width + 'px; height: ' + height + 'px; overflow: hidden;" title="' + name + '">';
    var left = 0;

    for ( var index=0; index < colors.length && left < width; left += color_width, index++ ) {
        html += '<div style="position: absolute; left: ' + left + 'px; width: ' + color_width + 'px; height: ' + height + 'px; background-color: #' + colors[index] + ';"></div>';
    }

    html += '</div>';

    return html;
}

// ----------------------------------------------------------------------------
//
function makeWeightedPaletteChip( colors, weights, chip_height, chip_width ) {
    var palette = [];
    var sorted_weights = [];
    var sorted_colors = [];

    $.each(colors, function (index, color) {
        var weight = weights != null && index < weights.length ? weights[index] : 1 / colors.length;
        palette.push({ color: color, weight: weight });
    });

    palette.sort(function (a, b) { return b.weight - a.weight; });

    $.each(palette, function (index, item) { sorted_weights.push(item.weight); sorted_colors.push(item.color); });

    var html = '<div style="position: relative; overflow: hidden; height: ' + chip_height + 'px; width: ' + chip_width + 'px;">';
    var left = 0;

    for (var index = 0; index < sorted_colors.length; index++) {
        var weight = sorted_weights != null && sorted_weights.length > index ? (sorted_weights[index] * 100) + "%" : "";
        var width = chip_width * sorted_weights[index];

        html += '<div class="weighted_palette_chip" ';
        html += 'style="left: ' + left + 'px; width: ' + width + 'px; background-color: #' + sorted_colors[index] + ';" title="' + weight + '">';
        html += '</div>';

        left += width;
    }

    html += '</div>';

    return html;
}

// ----------------------------------------------------------------------------
//
function convertISO8601( duration ) {
    if ( duration.indexOf( 'S' ) == -1 )
        duration += "0S";

    var H_index = duration.indexOf( 'H' );
    if ( H_index != -1 && duration.indexOf( 'M' ) == -1 )
        duration = duration.substr( 0, H_index+1 ) + "0M" + duration.substr( H_index+1 );

    var result = duration.replace( /[HM]/g, ":" );
    result = result.replace( /[^:\d]/g, "" );

    for ( var i=0; i < result.length; i++ ) 
        if ( result[i] == ':' && (i+2 >= result.length || result[i+2] == ':') )
            result = result.substr( 0, i+1 ) + "0" + result.substr(i+1 );

    return result;
}

// ----------------------------------------------------------------------------
//
function largest_index( item_array ) {
    var largest_index = 0;
    var largest = 0;

    for (var i = 0; i < item_array.length; i++)
        if (item_array[i] > largest) {
            largest = item_array[i];
            largest_index = i;
        }

    return largest_index;
}
