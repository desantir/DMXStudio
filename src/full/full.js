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

var FIXTURE_SLIDERS = 40;                          

var system_status = false;                          // UI is live and pinging the server
var edit_mode = false;                              // True when UI is in edit mode
var active_scene_id = 0;                            // UID of currently active scene
var active_chase_id = 0;                            // UID of currently active chase
var auto_blackout = false;                          // Venue is in auto backout mode
var client_config_update = false;                   // Client layout has changed -  update server
var current_act = 0;                                // All acts
var whiteout_color = '#FFFFFF';                     // Whiteout color
var num_universes = 1;
var default_scene_uid = 0;                          // Default scene UID
var volume_control = null;
var track_fixtures = false;

var scene_tile_panel;
var chase_tile_panel;
var slider_panel;
var animation_tile_panel;
var palette_window = null;
var music_select_window = null;
var macro_color = "#0000FF";
var macro_multi_color = false;

var whiteout_ms = [0, 75, 100, 125, 150, 250, 300, 400, 500, 750, 1000, 1500, 2000, 3000, 5000, 10000];

var animation_percent = [ 
    { percent: 10, label: "10 x" },
    { percent: 20, label: "5 x" },
    { percent: 25, label: "4 x" },
    { percent: 33, label: "3 x" },
    { percent: 50, label: "2 x" },
    { percent: 100, label: "1 x" },
    { percent: 200, label: "1/2 x" },
    { percent: 300, label: "1/3 x" },
    { percent: 500, label: "1/5 x" },
    { percent: 1000, label: "1/10 x" },
    { percent: 2000, label: "1/20 x" },
    { percent: 5000, label: "1/50 x" },
    { percent: 0, label: "Custom" } ];

var venue_filename = "";

var master_dimmer_channel = {
    "channel": 1, "label": "venue", "name": "Master Dimmer", "value": 0, "max_value": 100, "ranges": null, "type": -1, "color": null, "slider": null, "selectable": false
};

// No need to keep looking these up on every update and it seems expensive

var cache_whiteout_custom_value = null;
var cache_whiteout_fade_value = null;
var cache_whiteout_effect_0 = null;
var cache_whiteout_effect_1 = null;
var cache_whiteout_effect_2 = null;
var cache_animation_speed = null;
var cache_blackout_buttons = null;
var cache_whiteout_buttons = null;
var cache_music_match_buttons = null;
var cache_blackout_on = null;
var cache_blackout_off = null;
var cache_music_match_on = null;
var cache_music_match_off = null;
var cache_status_icon = null;

// ----------------------------------------------------------------------------
//
function initializeUI() {
    // Add features to slider panel
    slider_panel_features.push( { class_name: "colorpicker_icon", attach: attachPanelColorPicker, title: "show color palette" } );
    slider_panel_features.push( { class_name: "pantilt_icon", handler: panelPanTiltHandler, title: "show pan tilt" } );
    slider_panel_features.push( { class_name: "groupcolors_icon", handler: groupColorSlidersHandler, title: "group color channels" } );
    slider_panel_features.push( { class_name: "rearrange_icon", handler: arrangeSlidersHandler, title: "arrange channels" } );
    slider_panel_features.push( { class_name: "clear_links_icon", handler: resetLinkedSlidersHandler, title: "clear linked" } );

    scene_tile_panel = new TileScrollPanel("scene_tiles_pane", "Scene", "scene" );
    chase_tile_panel = new TileScrollPanel("chase_tiles_pane", "Chase", "chase" );
    animation_tile_panel = new TileScrollPanel("animation_tiles_pane", "Animation", "animation" );
    fixture_tile_panel = new TileScrollPanel("fixture_tiles_pane", "Fixture", "fixture", [ LayoutType.SCROLL, LayoutType.WRAP, LayoutType.MAP ] );
    slider_panel = new SliderPanel("slider_pane", FIXTURE_SLIDERS, true );

    // Cutomizations for fixtures and fixture groups
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_PLAY].action = "control";
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_PLAY].tile_class = "ui-icon ui-icon-plus";
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_PLAY].selected_class = "ui-icon-minus ui-icon-white";
    fixture_tile_panel.actions[fixture_tile_panel.ACTION_HOVER].used = true;

    initializeCollapsableSections();
    setFixtureTileLock( false );

    // setup act master
    $("#master_act").empty();

    for (var i = 0; i <= 20; i++) {
        $("#master_act").append($('<option>', {
            value: i,
            text: (i==0) ? "all" : i,
            selected: false
        }));
    }

    volume_control = new VolumeControl( "master_volume" );

    $("#blackout_buttons").buttonset();
    $("#whiteout_buttons").buttonset();
    $("#music_match_buttons").buttonset();
    $("#act_buttons").buttonset();

    $("#act_buttons").on("change", function () {
        current_act = parseInt($('input[name=act]:checked').val());
        createSceneTiles();
        createChaseTiles();
        createAnimationTiles();
        update_current_act();
        client_config_update = true;
    });

    update_current_act();

    for (var i = 0; i < whiteout_ms.length; i++) {
        if ( whiteout_ms[i] != 0 ) {
            $("#whiteout_custom_value").append($('<option>', {
                value: whiteout_ms[i],
                text: whiteout_ms[i] + " ms",
                selected: false
            }));
        }

        $("#whiteout_fade_value").append($('<option>', {
            value: whiteout_ms[i],
            text: whiteout_ms[i] > 0 ? whiteout_ms[i] + " ms" : "no fade",
            selected: false
        }));
    }

    $("#whiteout_custom_value").multiselect( {
        minWidth: 120, 
        multiple: false, 
        selectedList: 1, 
        header: false, 
        classes: "animation_speed",
        noneSelectedText: 'Select ms' } ).on("change", sendWhiteoutCustomStrobe );

    $("#whiteout_fade_value").multiselect( {
        minWidth: 120, 
        multiple: false, 
        selectedList: 1, 
        header: false, 
        classes: "animation_speed",
        noneSelectedText: 'Select ms' } ).on("change", sendWhiteoutCustomStrobe );

    $("#whiteout_effect_0").click(function (event) { stopEventPropagation(event); sendWhiteoutEffect(0); } );
    $("#whiteout_effect_1").click(function (event) { stopEventPropagation(event); sendWhiteoutEffect(1); } );
    $("#whiteout_effect_2").click(function (event) { stopEventPropagation(event); sendWhiteoutEffect(2); });

    for (var i = 0; i < animation_percent.length; i++) {
        $("#animation_speed").append($('<option>', {
            value: animation_percent[i].percent,
            text: animation_percent[i].label,
            selected: animation_percent[i].percent == 100
        }));
    }
    $("#animation_speed").multiselect({ minWidth: 135, multiple: false, selectedList: 1, header: false, noneSelectedText:'Select %', classes: "animation_speed" });

    $("#animation_speed").on("change", function () {
        if ( $(this).val() > 0 ) {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/venue/animationspeed/" + $(this).val(),
                cache: false,
                error: onAjaxError
            });
        }
    });

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

    $("#toggle_edit_mode").click(function () {
        setEditMode(!edit_mode);
        client_config_update = true;
    });

    $("#configure_venue").click( configureVenue );

    $("#save_load_venue").click( loadSaveVenue );

    $("#show_palette_editor").click( function() { 
        if ( palette_window != null && !palette_window.closed )
            palette_window.close();

        palette_window = window.open( "palette-editor.htm", "palette editor" );
    } );

    $("#show_music_select").click( function() { 
        if ( music_select_window != null && !music_select_window.closed )
            music_select_window.close();

        music_select_window = window.open( "music-select.htm", "music select" );
    } );

    $("#show_frequency_visualizer").click( showFrequencyVisualizer );

    $("#show_beat_visualizer").click(showBeatVisualizer);

    $("#show_music_matcher").click(showMusicMatch);

    $("#show_amplitude_visualizer").click( showAmplitudeVisualizer );

    $("#chaseTrack").click(chaseTrack);

    $("#chaseAdvance").click( function( event ) { chaseStep( event, 1 ); } );
    $("#chaseBack").click( function( event ) { chaseStep( event, -1 ); } );

    $("#whiteout_colorpicker").ColorPicker({
        color: "#FFFFFF",
        livePreview: true,

        onShow: function (picker) {
            $(picker).ColorPickerSetColor( whiteout_color );
            $(picker).ColorPickerSetColorChips( getColorChips() );
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            setWhiteoutColor(rgb);
        },

        onChange: function (hsb, hex, rgb) {
            setWhiteoutColor(rgb);
        },

        onChipSet: function( chip, hsb, hex, rgb ) {
            return setColorChip( chip, hex );
        }
    });

    $("#macro_effect").multiselect({
        minWidth: 140, multiple: false, selectedList: 1, noneSelectedText: 'select effect', classes: 'small_multilist', height: 170, header: false
    }).bind("multiselectclick", function() { client_config_update = true; } );

    $("#macro_color_speed").multiselect({
        minWidth: 110, multiple: false, selectedList: 1, noneSelectedText: 'select speed', classes: 'small_multilist', height: 170, header: false
    }).bind("multiselectclick", function() { client_config_update = true; } );

    $("#macro_move_speed").multiselect({
        minWidth: 110, multiple: false, selectedList: 1, noneSelectedText: 'select speed', classes: 'small_multilist', height: 170, header: false
    }).bind("multiselectclick", function () { client_config_update = true; });

    $("#macro_movement").multiselect({
        minWidth: 140, multiple: false, selectedList: 1, noneSelectedText: 'select movement', classes: 'small_multilist', height: 210, header: false
    }).bind("multiselectclick", function() { client_config_update = true; } );

    $("#macro_color").ColorPicker({
        color: "#FFFFFF",
        livePreview: false,
        autoClose: true,

        onShow: function (picker) {
            $(picker).ColorPickerSetColor( macro_color );
            $(picker).ColorPickerSetColorChips( getColorChips() );
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onChange: function (hsb, hex, rgb) {
            setMacroColor( "#" + hex );
            client_config_update = true;
        }
    });

    $("#macro_multi_color").click( macroMultiColor );
    $("#macro_go").button().click(macroEffectRun);
    $("#macro_stop").button().click(macroEffectStop);

    // Appear after setup because some require initialization (such as sliders)
    cache_whiteout_custom_value = $('#whiteout_custom_value');
    cache_blackout_buttons = $("#blackout_buttons");
    cache_whiteout_buttons = $("#whiteout_buttons");
    cache_music_match_buttons = $("#music_match_buttons");
    cache_blackout_on = $('#blackout_1');
    cache_blackout_off = $('#blackout_0');
    cache_music_match_on = $('#music_match_1');
    cache_music_match_off = $('#music_match_0');
    cache_status_icon = $("#system_status_icon");
    cache_animation_speed = $("#animation_speed");
    cache_whiteout_fade_value = $("#whiteout_fade_value");
    cache_whiteout_effect_0 = $("#whiteout_effect_0");
    cache_whiteout_effect_1 = $("#whiteout_effect_1");
    cache_whiteout_effect_2 = $("#whiteout_effect_2");

    initializeMusicPlayer();

    // Accept post messages
    window.addEventListener( "message", function( event ) {
        if ( event.data.method === "registerEventListener" ) {
            // toastNotice( event.data.method );
            registerEventListener( event.source );
        }
        else if ( event.data.method === "updateClientConfig" ) {
            updateClientConfig( event.data.config );
        }
    });

    palette_update_callback = function () {
        setMacroColor( macro_color );
        updateWhiteoutColor( whiteout_color );
    };

    // START THE VENUE
    startUserInterface();
}

var server_toast = null;

// ----------------------------------------------------------------------------
//
function startUserInterface() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/status/",
        cache: false,
        success: function (data) {
            $(".ui-widget-overlay").remove();

            cache_status_icon.addClass("ui-icon-green").removeClass("ui-icon-red");
            cache_status_icon.attr("title", "status: running");
            system_status = true;

            if ( server_toast ) {
                $().toastmessage( 'removeToast', server_toast, null );
                server_toast = null;
            }

            updateVenueLayout();

            setTimeout( start, 300 );
        },

        error: function () {
            if (system_status) {
                cache_status_icon.removeClass("ui-icon-green").addClass("ui-icon-red");
                cache_status_icon.attr("title", "status: disconnected");
                system_status = false;

                server_toast = toastError( "SERVER IS NOT RESPONDING" );
            }

            setTimeout( startUserInterface, 2500 );
        }
    });
}

function start()
{
    // Update the UI from server's current state
    updatePalettes();
    updateScenes();
    updateChases();
    updateAnimations();

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

                updateVolume( json.master_volume );
                updateVolumeMute( json.mute );

                updateMasterDimmer( json.dimmer );
                updateMuteBlackout( json.auto_blackout );
                updateWhiteoutStrobe( json.whiteout_strobe_ms, json.whiteout_fade_ms );
                updateWhiteoutColor( json.whiteout_color );
                updateAnimationSpeed( json.animation_speed );
                updateBlackout( json.blackout );
                updateWhiteout( json.whiteout );
                updateWhiteoutEffect( json.whiteout_effect );
                updateMusicMatch( json.music_match );
                updateCapturedFixtures( json.captured_fixtures );

                cache_status_icon.addClass("ui-icon-green").removeClass("ui-icon-red");
                cache_status_icon.attr("title", "status: running");
                system_status = true;

                track_fixtures = json.track_fixtures;

                updateFixtures();

                if ( json.music_player != null )
                    updatePlayerStatus( json.music_player );

                // Start event processing if needed
                start_event_processing();
            },

            error: function () {
                toastWarning( "VENUE UPDATE FAILED" );
                startUserInterface();
            }
        });
    };
    
    setTimeout( one_shot_updated, 100 );
}

// ----------------------------------------------------------------------------
//
function keyEvent( event ) {
    if ( event.which == 37 || event.which == 39 ) {
        var chase = getActiveChase();
        if ( chase != null && chase.getStepTrigger() == ChaseStepTriggerType.CT_MANUAL )
            chaseStep( event, event.which == 37 ? -1 : 1 );
    }
}

// ----------------------------------------------------------------------------
//
function updateBlackout( blackout ) {
    var blackout_button = blackout ? cache_blackout_on : cache_blackout_off;
    if (!blackout_button.prop("checked") && !cache_blackout_buttons.is(":focus"))
        blackout_button.prop("checked", true).button("refresh");
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
function updateWhiteoutEffect( effect ) {
    cache_whiteout_effect_0.removeClass( "whiteout_button_selected" );
    cache_whiteout_effect_1.removeClass( "whiteout_button_selected" );
    cache_whiteout_effect_2.removeClass( "whiteout_button_selected" );

    if ( effect == 0 ) {
        cache_whiteout_effect_0.addClass( "whiteout_button_selected" );
    }
    else if ( effect == 1 ) {
        cache_whiteout_effect_1.addClass( "whiteout_button_selected" );
    }
    else {
        cache_whiteout_effect_2.addClass("whiteout_button_selected");
    }
}

// ----------------------------------------------------------------------------
//
function updateWhiteoutColor( hex_color ) {
    whiteout_color = hex_color;
    updateColorChip( $("#whiteout_colorpicker"), "#" + hex_color );
}

//
function updateWhiteoutStrobe( whiteout_strobe_ms, whiteout_fade_ms ) {
    if (cache_whiteout_custom_value.val() != whiteout_strobe_ms && !cache_whiteout_custom_value.multiselect( "isOpen" )) {
        cache_whiteout_custom_value.find("option[value='" + whiteout_strobe_ms + "']").attr("selected", true);
        cache_whiteout_custom_value.multiselect("refresh");
    }

    if (cache_whiteout_fade_value.val() != whiteout_fade_ms && !cache_whiteout_fade_value.multiselect( "isOpen" )) {
        cache_whiteout_fade_value.find("option[value='" + whiteout_fade_ms + "']").attr("selected", true);
        cache_whiteout_fade_value.multiselect("refresh");
    }
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
function updateAnimationSpeed( animation_speed ) {
    if (cache_animation_speed.val() != animation_speed && !cache_animation_speed.multiselect( "isOpen" )) {
        if ( !multiselectSelect( cache_animation_speed, animation_speed ) )
            multiselectSelect( cache_animation_speed, 0 );
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
    $("#current_act").text(current_act == 0 ? "default act" : ("act " + current_act));
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
function sendWhiteoutCustomStrobe() {
    var strobe_ms = $("#whiteout_custom_value").val();
    var fade_ms = $("#whiteout_fade_value").val();

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/strobe/" + strobe_ms + "/" + fade_ms,
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function sendWhiteoutEffect( effect ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/whiteout/effect/" + effect,
        cache: false,
        error: onAjaxError
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
function master_dimmer_callback(unused, action, channel_type, value) {
    if ( action != "channel" )
        return;

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

    // Need to update fixture pane for tile lock
    fixture_tile_panel.setTileLock( fixture_tile_panel.getTileLock() );
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
function updateVolumeMute( mute ) {
    volume_control.setMute( mute );
}

// ----------------------------------------------------------------------------
//
function updateVolume( master_volume ) {
    volume_control.setVolume( master_volume );
}

// ----------------------------------------------------------------------------
//
function macroMultiColor( event ) {
    stopEventPropagation(event);

    setMacroMultiColor( !macro_multi_color );

    client_config_update = true;
}

// ----------------------------------------------------------------------------
//
function setMacroMultiColor( multi_color ) {
    stopEventPropagation(event);

    var button = $("#macro_multi_color" );

    macro_multi_color = multi_color;

    if ( macro_multi_color )
        button.addClass( "whiteout_button_selected" );
    else
        button.removeClass( "whiteout_button_selected" );
}

// ----------------------------------------------------------------------------
//
function setMacroColorSpeed( color_speed ) {
    $("#macro_color_speed").find('option[value=' + color_speed + ']').attr('selected', 'selected');
    $("#macro_color_speed").multiselect("refresh");
}

// ----------------------------------------------------------------------------
//
function setMacroMoveSpeed(move_speed) {
    $("#macro_move_speed").find('option[value=' + move_speed + ']').attr('selected', 'selected');
    $("#macro_move_speed").multiselect("refresh");
}

// ----------------------------------------------------------------------------
//
function setMacroEffect( macro_effect ) {
    $("#macro_effect").find('option[value=' + macro_effect + ']').attr('selected', 'selected');
    $("#macro_effect").multiselect("refresh");
}

// ----------------------------------------------------------------------------
//
function setMacroColor( color ) {
    updateColorChip( $("#macro_color"), color, 3 );
    macro_color = color;
}

// ----------------------------------------------------------------------------
//
function setMacroMovement( movement ) {
    $("#macro_movement").find('option[value=' + movement + ']').attr('selected', 'selected');
    $("#macro_movement").multiselect("refresh");
}

// ----------------------------------------------------------------------------
//
function macroEffectStop(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/venue/control/quickscene/stop",
        async: true,
        cache: false,
        success: function () {
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function macroEffectRun( event ) {
    stopEventPropagation(event);

    var fixture_ids = getActiveFixturesInOrder();
    if ( fixture_ids.length == 0 )
        return;

    var json = {
        "fixtures": fixture_ids,
        "effect": $("#macro_effect").val(),
        "color_speed_ms": $("#macro_color_speed").val(),
        "move_speed_ms": $("#macro_move_speed").val(),
        "movement": $("#macro_movement").val(),
        "color": macro_color,
        "multi_color": macro_multi_color
    };

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/venue/create/quickscene/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        async: true,
        cache: false,
        success: function () {
        },
        error: onAjaxError
    });
}
