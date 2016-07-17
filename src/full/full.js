/* 
Copyright (C) 2012-15 Robert DeSantis
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

var FIXTURE_SLIDERS = 40;                          

var system_status = false;                          // UI is live and pinging the server
var edit_mode = false;                              // True when UI is in edit mode
var active_scene_id = 0;                            // UID of currently active scene
var active_chase_id = 0;                            // UID of currently active chase
var auto_blackout = false;                          // Venue is in auto backout mode
var client_config_update = false;                   // Client layou has changed -  update server
var current_act = 0;                                // All acts
var whiteout_color = '#FFFFFF';                     // Whiteout color
var num_universes = 1;
var default_scene_uid = 0;                          // Default scene UID

var scene_tile_panel;
var chase_tile_panel;
var fixture_tile_panel;
var slider_panel;

var whiteout_ms = [25, 50, 75, 100, 125, 150, 200, 250, 350, 500, 750, 1000, 1500, 2000];

var venue_filename = "";

var master_dimmer_channel = {
    "channel": 1, "label": "venue", "name": "Master Dimmer", "value": 0, "max_value": 100, "ranges": null, "type": -1, "color": null, "slider": null
};

var client_config = null;
/*
    {
        "edit_mode": true,
        "delete_mode": false,

        "sections": {
            "venue_pane": { collapsed: false },
            "music_pane": { collapsed: true },
            "act_pane": { collapsed: false },
            "scene_tiles_pane": { collapsed: false, scroll: false },
            "chase_tiles_pane": { collapsed: true, scroll: true },
            "fixture_tiles_pane": { collapsed: false, scroll: false },
            "slider_pane": { collapsed: false, scroll: false }
        }
    };
*/

// No need to keep looking these up on every update and it seems expensive
var cache_master_volume = null;
var cache_whiteout_custom_value = null;
var cache_master_volume_value = null;
var cache_master_volume_handle = null;
var cache_volume_mute = null;
var cache_animation_speed = null;
var cache_blackout_buttons = null;
var cache_whiteout_buttons = null;
var cache_music_match_buttons = null;
var cache_animation_speed = null;
var cache_blackout_on = null;
var cache_blackout_off = null;
var cache_music_match_on = null;
var cache_music_match_off = null;
var cache_status_icon = null;

// ----------------------------------------------------------------------------
//
function initializeUI() {
    scene_tile_panel = new TileScrollPanel("scene_tiles_pane", "Scene", "scene");
    chase_tile_panel = new TileScrollPanel("chase_tiles_pane", "Chase", "chase");
    fixture_tile_panel = new TileScrollPanel("fixture_tiles_pane", "Fixture", "fixture");
    slider_panel = new SliderPanel("slider_pane", FIXTURE_SLIDERS, true );

    // Cutomizations for fixtures and fixture groups
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_PLAY].action = "control";
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_PLAY].tile_class = "ui-icon ui-icon-plus";
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_PLAY].selected_class = "ui-icon-minus ui-icon-white";
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_HOVER].used = true;

    initializeCollapsableSections();

    // setup act master
    $("#master_act").empty();

    for (var i = 0; i <= 20; i++) {
        $("#master_act").append($('<option>', {
            value: i,
            text: (i==0) ? "all" : i,
            selected: false
        }));
    }
    // setup master volume
    initializeHorizontalSlider("master_volume", 0, 100, 0);

    $("#blackout_buttons").buttonset();
    $("#whiteout_buttons").buttonset();
    $("#music_match_buttons").buttonset();
    $("#act_buttons").buttonset();

    $("#act_buttons").on("change", function () {
        current_act = parseInt($('input[name=act]:checked').val());
        createSceneTiles();
        createChaseTiles();
        update_current_act();
        client_config_update = true;
    });

    update_current_act();

    for (var i = 0; i < whiteout_ms.length; i++) {
        $("#whiteout_custom_value").append($('<option>', {
            value: whiteout_ms[i],
            text: whiteout_ms[i] + " ms",
            selected: false
        }));
    }

    $("#whiteout_custom_value").multiselect({ minWidth: 120, multiple: false, selectedList: 1, header: false, noneSelectedText:'Select ms' });

    $("#animations_speed").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select ms' });

    $("#blackout_buttons").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/blackout/" + $('input[name=blackout]:checked').val(),
            cache: false,
            error: onAjaxError
        });
    });

    $("#music_match_buttons").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/music_match/" + $('input[name=music_match_enable]:checked').val(),
            cache: false,
            error: onAjaxError
        });
    });

    $("#whiteout_buttons").change(function () {
        var value = $('input[name=whiteout]:checked').val();
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/whiteout/" + value,
            cache: false,
            error: onAjaxError
        });
    });

    $("#master_dimmer").on("slide slidestop", sendMasterDimmerUpdate);

    $("#whiteout_custom_value").on("change", function () {
        var ms = $(this).val();
        if ( ms >= 25 && ms <= 10000 ) {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/venue/strobe/" + ms,
                cache: false,
                error: onAjaxError
            });
        }
    });

    $("#master_volume").on("slide slidestop", sendMasterVolumeUpdate);

    $("#animations_speed").on("change", function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/animation_speed/" + $(this).val(),
            cache: false,
            error: onAjaxError
        });
    });

    $("#volume_mute").click(function () {
        var mute = $(this).hasClass("ui-icon-volume-on"); // Toggle mute
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/volume/mute/" + ((mute) ? 1 : 0),
            cache: false,
            error: onAjaxError
        });
    });

    $("#toggle_edit_mode").click(function () {
        setEditMode(!edit_mode);
        client_config_update = true;
    });

    $("#configure_venue").click( configureVenue );

    $("#save_load_venue").click( loadSaveVenue );

    $("#show_frequency_visualizer").click( showFrequencyVisualizer );

    $("#show_beat_visualizer").click(showBeatVisualizer);

    $("#show_music_matcher").click(showMusicMatch);

    $("#chaseTrack").click(chaseTrack);

    initializeHorizontalSlider("frequency_sample_rate", 25, 1000, 50);

    $("#channel_panel_colorpicker").ColorPicker({
        color: "#FFFFFF",
        livePreview: true,

        onShow: function (picker) {
            var color = getColorChannelRGB();
            if ( color != null )
                $("#channel_panel_colorpicker").ColorPickerSetColor(color);
            $("#channel_panel_colorpicker").removeClass('ui-icon').addClass('ui-icon-white');
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $("#channel_panel_colorpicker").removeClass('ui-icon-white').addClass('ui-icon');
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            setColorChannelRGB(hsb, hex, rgb);
        },

        onChange: function (hsb, hex, rgb) {
            setColorChannelRGB(hsb, hex, rgb);
        }
    });

    $("#whiteout_colorpicker").ColorPicker({
        color: "#FFFFFF",
        livePreview: true,

        onShow: function (picker) {
            $("#whiteout_colorpicker").ColorPickerSetColor( whiteout_color );
            $("#whiteout_colorpicker").removeClass('ui-icon').addClass('ui-icon-white');
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $("#whiteout_colorpicker").removeClass('ui-icon-white').addClass('ui-icon');
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            setWhiteoutColor(rgb);
        },

        onChange: function (hsb, hex, rgb) {
            setWhiteoutColor(rgb);
        }

    });

    // Appear after setup because some require initialization (such as sliders)
    cache_master_volume = $('#master_volume');
    cache_whiteout_custom_value = $('#whiteout_custom_value');
    cache_master_volume_value = $('#master_volume_value');
    cache_master_volume_handle = $('#master_volume .ui-slider-handle')
    cache_volume_mute = $("#volume_mute");
    cache_blackout_buttons = $("#blackout_buttons");
    cache_whiteout_buttons = $("#whiteout_buttons");
    cache_music_match_buttons = $("#music_match_buttons");
    cache_animation_speed = $('#animations_speed');
    cache_blackout_on = $('#blackout_1');
    cache_blackout_off = $('#blackout_0');
    cache_music_match_on = $('#music_match_1');
    cache_music_match_off = $('#music_match_0');
    cache_status_icon = $("#system_status_icon");

    initializeMusicPlayer();

    // START THE VENUE

    startUserInterface();
}

// ----------------------------------------------------------------------------
//
function startUserInterface() {
    // Update the UI from server's current state
    updateVenueLayout();
    updateScenes();
    updateChases();
    updateFixtures();

    var one_shot_updated = function() {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/venue/status/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                DMX_MAX_UNIVERSES = json.dmx_max_universes;

                default_scene_uid = json.default_scene_uid;
                venue_filename = json.venue_filename;

                updateMasterDimmer( json.dimmer );
                updateVolume( json.master_volume );
                updateMuteBlackout( json.auto_blackout );
                updateWhiteoutStrobe( json.whiteout_strobe );
                updateWhiteoutColor( json.whiteout_color );
                updateBlackout( json.blackout );
                updateWhiteout( json.whiteout );
                updateMusicMatch( json.music_match );
                updateVolumeMute( json.mute );
                updateAnimationSpeed( json.animation_speed );
                updateCapturedFixtures( json.captured_fixtures );

                cache_status_icon.addClass("ui-icon-green").removeClass("ui-icon-red");
                cache_status_icon.attr("title", "status: running");
                system_status = true;

                if ( json.music_player != null )
                    update_player_status( json.music_player );

                // Start event processing if needed
                start_event_processing();
            },

            error: function () {
                toastError( "VENUE UPDATE FAILED" );
            }
        })
    };
    
    setTimeout( one_shot_updated, 100 );
}

// ----------------------------------------------------------------------------
//
function updateBlackout( blackout ) {
    var blackout = blackout ? cache_blackout_on :cache_blackout_off;
    if (!blackout.prop("checked") && !cache_blackout_buttons.is(":focus"))
        blackout.prop("checked", true).button("refresh");
}

// ----------------------------------------------------------------------------
//
function updateWhiteout( whiteout ) {
    var whiteout_button = $("#whiteout_" + whiteout);
    if (!whiteout_button.prop("checked") && !cache_whiteout_buttons.is(":focus"))
        whiteout_button.prop("checked", true).button("refresh");
}

// ----------------------------------------------------------------------------
//
function updateWhiteoutColor( hex_color ) {
    whiteout_color = hex_color;
    $("#whiteout_colorchip").css( 'background-color', "#" + hex_color );
}

// ----------------------------------------------------------------------------
//
function updateMusicMatch( music_match ) {
    var mm = music_match ? cache_music_match_on : cache_music_match_off;
    if (!mm.prop("checked") && !cache_music_match_buttons.is(":focus"))
            mm.prop("checked", true).button("refresh");
}

// ----------------------------------------------------------------------------
//
function updateMuteBlackout( blackout ) {
    if (auto_blackout != blackout) {
        auto_blackout = blackout;

        if (auto_blackout)
            $("#system_blackout_icon").show();
        else
            $("#system_blackout_icon").hide();

        toastNotice( auto_blackout ? "auto blackout on" : "auto blackout off" );
    }
}

// ----------------------------------------------------------------------------
//
function updateVolumeMute( mute ) {
    if (mute != cache_volume_mute.hasClass("ui-icon-volume-off")) {
        if (!mute) {
            cache_volume_mute.attr('class', "ui-icon ui-icon-volume-on");
            cache_volume_mute.attr('title', 'mute');
        }
        else {
            cache_volume_mute.attr('class', "ui-icon-red ui-icon-volume-off");
            cache_volume_mute.attr('title', 'unmute');
        }
    }
}

// ----------------------------------------------------------------------------
//
function updateAnimationSpeed( animation_speed ) {
    if (cache_animation_speed.val() != animation_speed && !cache_animation_speed.multiselect("isOpen")) {
        var o = cache_animation_speed.find("option[value='" + animation_speed + "']");

        if (o != null && o.length == 1 )
            o.attr("selected", true);
        else {
            cache_animation_speed.append($('<option>', {
                value: animation_speed,
                text: "custom: " + animation_speed + " ms",
                selected: true
            }));
        }

        cache_animation_speed.multiselect('refresh');
    }
}

// ----------------------------------------------------------------------------
//
function updateVolume( master_volume ) {
    if (cache_master_volume.slider("value") != master_volume && !cache_master_volume_handle.is(':focus') ) {
        cache_master_volume_value.html(master_volume);
        cache_master_volume.slider("value", master_volume);
    }
}

// ----------------------------------------------------------------------------
//
function updateWhiteoutStrobe( whiteout_strobe ) {
    if (cache_whiteout_custom_value.val() != whiteout_strobe && !cache_whiteout_custom_value.multiselect( "isOpen" )) {
        cache_whiteout_custom_value.empty();
        var found = false;
        for (var i = 0; i < whiteout_ms.length; i++) {
            if (whiteout_ms[i] == whiteout_strobe)
                found = true;
            cache_whiteout_custom_value.append($('<option>', {
                value: whiteout_ms[i],
                text: whiteout_ms[i] + " ms",
                selected: whiteout_ms[i] == whiteout_strobe
            }));
        }
        if (!found) {
            cache_whiteout_custom_value.append($('<option>', {
                value: whiteout_strobe,
                text: whiteout_strobe + " ms",
                selected: true
            }));
        }
        cache_whiteout_custom_value.multiselect("refresh");
    }
}

// ----------------------------------------------------------------------------
//
function updateMasterDimmer( master_dimmer ) {
    if (  master_dimmer_channel.value != master_dimmer  ) {
        var slider = master_dimmer_channel.slider;
            if ( slider != null && !slider.isBusy() ) {
                slider.setValue(master_dimmer);
                slider.changed = false;
            }
    
        // Always set as the dimmer may not yet be setup on startup
        master_dimmer_channel.value = master_dimmer;
    }
}

// ----------------------------------------------------------------------------
//
function update_current_act() {
    $("#current_act").text(current_act == 0 ? "all acts" : ("act " + current_act));
}

// ----------------------------------------------------------------------------
//
function messageBox(message) {
    $("#message_box_dialog").dialog({
        title: "DMXStudio",
        autoOpen: true,
        width: 540,
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
function toastNotice( message ) {
    $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 3000,
        sticky   : false,
        position : 'top-right',
        type     : 'notice',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function toastWarning( message ) {
    $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 10000,
        sticky   : false,
        position : 'top-right',
        type     : 'warning',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function toastError( message ) {
    $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 3000,
        sticky   : true,
        position : 'top-right',
        type     : 'error',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function setWhiteoutColor(rgb) {
    var color = (rgb.r << 16) | (rgb.g << 8) | rgb.b;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/whiteout/color/" + color.toString(16),
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function sendMasterVolumeUpdate() {
    delayUpdate( "volume", 10, function() {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/volume/master/" + $("#master_volume").slider("value"),
            cache: false,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function sendMasterDimmerUpdate() {
    delayUpdate( "dimmer", 10, function() {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/masterdimmer/" + $("#master_dimmer").slider("value"),
            cache: false,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
var pendingUpdates = [];

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
function master_dimmer_callback(unused, channel_type, value) {
    master_dimmer_channel.value = value;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/masterdimmer/" + value,
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function stopEventPropagation(event) {
    if ( event = (event || window.event ) ) {
        if (event.stopPropagation != null)
            event.stopPropagation();
        else
            event.cancelBubble = true;
    }
}

// ----------------------------------------------------------------------------
//
function setEditMode(mode) {
    edit_mode = mode;

    var state;

    if (edit_mode) {
        $("#toggle_edit_mode").addClass("lg-icon-white").removeClass("lg-icon-grey");
        state = 'inline';
    }
    else {
        $("#toggle_edit_mode").removeClass("lg-icon-white").addClass("lg-icon-grey");
        state = 'none';
    }

    $(".edit_mode").each(function () { $(this).css('display', state); });
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
function deleteVenueItem( item, callback ) {

    $("#delete_item_name").html(item.getUserTypeName() + " " + item.getNumber() + ": " + item.getFullName());

    $("#delete_item_dialog").dialog({
        autoOpen: false,
        width: 400,
        height: 210,
        modal: true,
        resizable: false,
        buttons: {
            "Delete Item": function () {
                callback(item);
                $(this).dialog("close");
            },
            Cancel: function () {
                $(this).dialog("close");
            }
        }
    });

   // $("#delete_item_dialog").data("item", item );

    $("#delete_item_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function onAjaxError( jqXHR, textError, errorThrown ) {
    messageBox("Unable to complete server operation [" + textError + " (" + errorThrown + ") ]");
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
function unencode(entity_ridden_string) {
    if (entity_ridden_string == null)
        return null;

    entity_ridden_string = entity_ridden_string.replace(/&amp;/g, "&");
    entity_ridden_string = entity_ridden_string.replace(/&lt;/g, "<");
    entity_ridden_string = entity_ridden_string.replace(/&gt;/g, ">");
    entity_ridden_string = entity_ridden_string.replace(/&quot;/g, "\"");
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
function load_options( element, options, selected_func  ) {
    element.empty();
    var html = "";
    for (var i = 0; i < options.length; i++) {
        html += '<option value=' + (i + 1) + ' ' + (selected_func(i) ? "selected" : "") +
                '>' + options[i] + '</option>';
    }
    element.html(html);
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