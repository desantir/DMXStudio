/* 
Copyright (C) 2012,2013 Robert DeSantis
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
var delete_mode = false;                            // True when UI is in delete mode
var active_scene_id = 0;                            // UID of currently active scene
var active_chase_id = 0;                            // UID of currently active chase
var auto_blackout = false;                          // Venue is in auto backout mode
var client_config_update = false;                   // Client layou has changed -  update server

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
            "scene_tiles_pane": { collapsed: false, scroll: false },
            "chase_tiles_pane": { collapsed: true, scroll: true },
            "fixture_tiles_pane": { collapsed: false, scroll: false },
            "slider_pane": { collapsed: false, scroll: false }
        }
    };
*/

// ----------------------------------------------------------------------------
//
function initializeUI() {
    scene_tile_panel = new TileScrollPanel("scene_tiles_pane", "Scene", "scene");
    chase_tile_panel = new TileScrollPanel("chase_tiles_pane", "Chase", "chase");
    fixture_tile_panel = new TileScrollPanel("fixture_tiles_pane", "Fixture", "fixture");
    slider_panel = new SliderPanel("slider_pane", FIXTURE_SLIDERS, true );

    // Cutomizations for fixtures and fixture groups
    fixture_tile_panel.actions[0].action = "control";
    fixture_tile_panel.actions[0].tile_class = "ui-icon ui-icon-plus";
    fixture_tile_panel.actions[0].selected_class = "ui-icon-minus ui-icon-white";

    initializeCollapsableSections();

    // setup master volume
    initializeHorizontalSlider("master_volume", 0, 100, 0);

    $("#blackout_buttons").buttonset();
    $("#whiteout_buttons").buttonset();
    $("#music_match_buttons").buttonset();

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

    $("#toggle_delete_mode").click(function () {
        setDeleteMode(!delete_mode);
        client_config_update = true;
    });

    $("#configure_venue").click( configureVenue );

    $("#save_load_venue").click( loadSaveVenue );

    $("#show_frequency_visualizer").click( showFrequencyVisualizer );

    $("#show_beat_visualizer").click(showBeatVisualizer);

    $("#show_music_matcher").click(showMusicMatch);

    initializeHorizontalSlider("frequency_sample_rate", 25, 1000, 100);

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
        }
    });

    // Update the UI from server's current state
    updateVenueLayout();
    updateScenes();
    updateChases();
    updateFixtures();

    // Start background updates, but allow all data loads to finish
    setTimeout( updateUI(), 100 );
}

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
// No need to keep looking these up on every update and it seems expensive
//var cache_master_dimmer = null;
var cache_master_volume = null;
var cache_whiteout_custom_value = null;
var cache_master_dimmer_value = null;
var cache_master_volume_value = null;
var cache_master_dimmer_handle = null;
var cache_master_volume_handle = null;
var cache_volume_mute = null;
var cache_animation_speed = null;
var animation_default = null;

function updateUI() {
    if (!cache_master_volume) {
        // cache_master_dimmer = $('#master_dimmer');
        cache_master_volume = $('#master_volume');
        cache_whiteout_custom_value = $('#whiteout_custom_value');
        cache_master_dimmer_value = $('#master_dimmer_value');
        cache_master_volume_value = $('#master_volume_value');
        cache_master_dimmer_handle = $('#master_dimmer .ui-slider-handle');
        cache_master_volume_handle = $('#master_volume .ui-slider-handle')
        cache_volume_mute = $("#volume_mute");
    }

    // See if we need to push configuration updates to the server
    if (client_config_update) {
        saveVenueLayout();
    }

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/status/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            markActiveScene(json.current_scene);
            markActiveChase(json.current_chase);

            if (master_dimmer_channel.slider != null && master_dimmer_channel.value != json.dimmer && !master_dimmer_channel.slider.slider.is(':focus') ) {
                master_dimmer_channel.value = json.dimmer;
                master_dimmer_channel.slider.setValue(json.dimmer);
            }

            if (!cache_master_volume_handle.is(':focus') && cache_master_volume.slider("value") != json.master_volume) {
                cache_master_volume_value.html(json.master_volume);
                cache_master_volume.slider("value", json.master_volume);
            }

            if (auto_blackout != json.auto_blackout) {
                auto_blackout = json.auto_blackout;

                if (auto_blackout)
                    $("#system_blackout_icon").show();
                else
                    $("#system_blackout_icon").hide();
            }

            if (!cache_whiteout_custom_value.multiselect( "isOpen" ) && cache_whiteout_custom_value.val() != json.whiteout_strobe) {
                $("#whiteout_custom_value").empty();
                var found = false;
                for (var i = 0; i < whiteout_ms.length; i++) {
                    if (whiteout_ms[i] == json.whiteout_strobe)
                        found = true;
                    $("#whiteout_custom_value").append($('<option>', {
                        value: whiteout_ms[i],
                        text: whiteout_ms[i] + " ms",
                        selected: whiteout_ms[i] == json.whiteout_strobe
                    }));
                }
                if (!found) {
                    $("#whiteout_custom_value").append($('<option>', {
                        value: json.whiteout_strobe,
                        text: json.whiteout_strobe + " ms",
                        selected: true
                    }));
                }
                $("#whiteout_custom_value").multiselect("refresh");
            }

            var blackout_id = json.blackout ? "#blackout_1" : "#blackout_0";
            if (!$("#blackout_buttons").is(":focus") && !$(blackout_id).prop("checked"))
                $(blackout_id).prop("checked", true).button("refresh");

            if ( !$("#whiteout_buttons").is(":focus") && !$("#whiteout_" + json.whiteout).prop("checked"))
                $("#whiteout_" + json.whiteout).prop("checked", true).button("refresh");

            if (!$("#music_match_buttons").is(":focus")) {
                var mm_id = json.music_match ? "#music_match_1" : "#music_match_0";
                if (!$(mm_id).prop("checked"))
                    $(mm_id).prop("checked", true).button("refresh");
            }

            if (json.mute != cache_volume_mute.hasClass("ui-icon-volume-off")) {
                if (!json.mute) {
                    cache_volume_mute.attr('class', "ui-icon ui-icon-volume-on");
                    cache_volume_mute.attr('title', 'mute');
                }
                else {
                    cache_volume_mute.attr('class', "ui-icon-red ui-icon-volume-off");
                    cache_volume_mute.attr('title', 'unmute');
                }
            }
 
            var animation_speed = $('#animations_speed');

            // Establish the default speed 
            if (animation_default == null) {
                animation_default = animation_speed.find('option[value=0]');
                if (animation_default != null) {
                    animation_default.html("Default: " + json.animation_speed + " ms");
                    animation_default.val(json.animation_speed);
                    animation_speed.multiselect('refresh');
                }
            }

            if (!animation_speed.multiselect("isOpen") && animation_speed.val() != json.animation_speed) {
                var o = animation_speed.find("option[value='" + json.animation_speed + "']");
                var index = 0;

                if (o != null && o.index() != -1)
                    index = o.index();

                animation_speed.find('option').eq(index).attr("selected", true);
                animation_speed.multiselect('refresh');
            }

            venue_filename = json.venue_filename;

            update_player_status( json.music_player );

            if (!system_status) {
                var status_icon = $("#system_status_icon");
                status_icon.addClass("ui-icon-green").removeClass("ui-icon-red");
                status_icon.attr("title", "status: running");
                system_status = true;
            }

            setTimeout(updateUI(), 500);
        },
        error: function () {
            if (system_status) {
                var status_icon = $("#system_status_icon");
                status_icon.removeClass("ui-icon-green").addClass("ui-icon-red");
                status_icon.attr("title", "status: disconnected");
                system_status = false;
            }

            setTimeout(updateUI(), 500);
        }
    });
}

// ----------------------------------------------------------------------------
//
function sendMasterVolumeUpdate() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/volume/master/" + $("#master_volume").slider("value"),
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function sendMasterDimmerUpdate() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/masterdimmer/" + $("#master_dimmer").slider("value"),
        cache: false,
        error: onAjaxError
    });
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

    $(".edit_item").each(function () { $(this).css('display', state); });
    $(".new_item").each(function () { $(this).css('display', state); });
}

// ----------------------------------------------------------------------------
//
function setDeleteMode(mode) {
    delete_mode = mode;

    var state;

    if (delete_mode) {
        $("#toggle_delete_mode").addClass("lg-icon-white").removeClass("lg-icon-grey");
        state = 'inline';
    }
    else {
        $("#toggle_delete_mode").removeClass("lg-icon-white").addClass("lg-icon-grey");
        state = 'none';
    }

    $(".delete_item").each(function () { $(this).css('display', state); });
}

// ----------------------------------------------------------------------------
//
function configureVenue(event) {
    stopEventPropagation(event);

    $("#configure_venue_dialog").dialog({
        autoOpen: false,
        width: 540,
        height: 550,
        modal: true,
        resizable: false,
        buttons: {
            "Update Venue": function () {

                var json = {
                    name: $("#cvd_name").val(),
                    description: $("#cvd_description").val(),
                    dmx_port: $("#cvd_dmx_port").val(),
                    dmx_packet_delay_ms: $("#cvd_dmx_packet_delay_ms").val(),
                    dmx_minimum_delay_ms: $("#cvd_dmx_minimum_delay_ms").val(),
                    audio_boost: $("#cvd_audio_boost").val(),
                    audio_boost_floor: $("#cvd_audio_boost_floor").val(),
                    audio_sample_size: $("#cvd_audio_sample_size").val(),
                    audio_capture_device: $("#cvd_audio_capture_device").val(),
                    auto_blackout: $("#cvd_auto_blackout").val()
                };

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/edit/venue/update/",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    cache: false,
                    async: false,
                    success: function () {
                        updateScenes();
                        updateChases();
                        updateFixtures();
                    },

                    error: onAjaxError
                });

                $(this).dialog("close");
            },
            Cancel: function () {
                $(this).dialog("close");
            }
        }
    });

    $("#cvd_accordion").accordion({ heightStyle: "fill" });

    $("#cvd_dmx_port").multiselect({ minWidth: 150, multiple: false, selectedList: 1, header: false });
    $("#cvd_audio_capture_device").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false });
    $("#cvd_audio_sample_size").multiselect({ minWidth: 150, multiple: false, selectedList: 1, header: false, height: "auto" });

    $("#cvd_dmx_port").empty();
    $("#cvd_audio_capture_device").empty();
    $("#cvd_audio_sample_size").empty();

    $("#cvd_dmx_packet_delay_ms").spinner({ min: 50, max: 1000, value: 0 });
    $("#cvd_dmx_minimum_delay_ms").spinner({ min: 0, max: 1000, value: 0 });
    $("#cvd_audio_boost").spinner({ min: 0, max: 100, value: 0.0, step: .5 });
    $("#cvd_audio_boost_floor").spinner({ min: 0, max: 100, value: 0.0, step: .1 });
    $("#cvd_auto_blackout").spinner({ min: 0, max: 65000, value: 0, step: 1000 });

    $("#configure_venue_dialog").dialog("open");

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/describe/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            openConfigureVenueDialog(json);
        },
        error: function (jqXHR, textError, errorThrown) {
            $("#configure_venue_dialog").dialog("close");
            onAjaxError(jqXHR, textError, errorThrown);
        }
    });
}

// ----------------------------------------------------------------------------
//
function openConfigureVenueDialog(json) {
    $("#cvd_name").val(json.name);
    $("#cvd_description").val(json.description);

    for (var i = 0; i < json.ports.length; i++) {
        var port = json.ports[i];
        $("#cvd_dmx_port").append($('<option>', {
            value: port,
            text: port,
            selected: port == json.dmx_port
        }));
    }

    $("#cvd_dmx_port").multiselect("refresh");

    $("#cvd_dmx_packet_delay_ms").spinner().val( json.dmx_packet_delay_ms );
    $("#cvd_dmx_minimum_delay_ms").spinner().val (json.dmx_minimum_delay_ms );
    $("#cvd_audio_boost").spinner().val( json.audio_boost );
    $("#cvd_audio_boost_floor").spinner().val (json.audio_boost_floor );
    $("#cvd_auto_blackout").spinner( "value", json.auto_blackout );

    for (var i = 1024; i <= 1024*8; i *= 2 ) {
        $("#cvd_audio_sample_size").append($('<option>', {
            value: i,
            text: i,
            selected: i == json.audio_sample_size
        }));
    }

    $("#cvd_audio_sample_size").multiselect("refresh");

    for (var i = 0; i < json.capture_devices.length; i++) {
        var device = json.capture_devices[i];
        $("#cvd_audio_capture_device").append($('<option>', {
            value: device,
            text: device,
            selected: device == json.audio_capture_device
        }));
    }

    $("#cvd_audio_capture_device").multiselect("refresh");
}

// ----------------------------------------------------------------------------
//
function loadSaveVenue( event ) {
    stopEventPropagation(event);

    $("#load_save_dialog").dialog({
        autoOpen: false,
        width: 640,
        height: 450,
        modal: true,
        resizable: false,
        buttons: [
            {
                id: "new_venue_button",
                text:"New Venue",
                click: function () {
                    $("#load_save_dialog").dialog("close");
                    var ice_dialog = put_user_on_ice("Working ... Please wait");

                    if ($("#lsd_accordion").accordion('option', 'active') == 2) {
                        var json = {
                            reset_what: $('input[name=lsd_new_venue]:checked').val()
                        };

                        $.ajax({
                            type: "POST",
                            url: "/dmxstudio/rest/edit/venue/new/",
                            data: JSON.stringify(json),
                            contentType: 'application/json',
                            cache: false,
                            success: function () {
                                ice_dialog.dialog("close");
                                window.location.reload(true);
                            },
                            error: function () {
                                ice_dialog.dialog("close");
                                onAjaxError();
                            }
                        });
                    }
                }
            },
            {
                id: "save_venue_button",
                text: "Save Venue",
                click: function () {
                    $("#load_save_dialog").dialog("close");
                    var ice_dialog = put_user_on_ice("Working ... Please wait");

                    if ($("#lsd_accordion").accordion('option', 'active') == 0) {
                        var json = { venue_filename: $("#lsd_file_name").val() };

                        $.ajax({
                            type: "POST",
                            url: "/dmxstudio/rest/edit/venue/save/",
                            data: JSON.stringify(json),
                            contentType: 'application/json',
                            cache: false,
                            success: function () {
                                ice_dialog.dialog("close");
                                messageBox("Server venue file saved");
                            },
                            error: function () {
                                ice_dialog.dialog("close");
                                messageBox("Cannot save server venue file!");
                            }
                        });
                    }
                    else {
                        document.getElementById("load_save_dialog_downloadFrame").src = "/dmxstudio/rest/venue/download/";
                        setTimeout(function () { ice_dialog.dialog("close"); }, 500);
                    }
                }
            },
            {
                id: "load_venue_button",
                text: "Load Venue",
                click: function () {
                    if ($("#lsd_accordion").accordion('option', 'active') == 0) {
                        var json = { venue_filename: $("#lsd_file_name").val() };
                        $("#load_save_dialog").dialog("close");
                        var ice_dialog = put_user_on_ice("Working ... Please wait");

                        $.ajax({
                            type: "POST",
                            url: "/dmxstudio/rest/edit/venue/load/",
                            data: JSON.stringify(json),
                            contentType: 'application/json',
                            cache: false,
                            success: function () {
                                window.location.reload( true );
                            },
                            error: function () {
                                ice_dialog.dialog("close");
                                messageBox("Cannot load server venue file '" + json.venue_filename + "'");
                            }
                        });
                    }
                    else {
                        if ($("#lsd_upload_file").val() != "") {
                            put_user_on_ice("Loading venue ... please wait");               // This is all or nothing - disable the dialog
                            $("#lsd_form")[0].submit();
                        }
                        else
                            $("#lsd_upload_file").click();
                    }
                }
            },
            {
                text: "Cancel",
                click: function () {
                    $(this).dialog("close");
                }
            }
        ]
    });

    function setupButtons() {
        if ($("#lsd_accordion").accordion('option', 'active') == 2) {
            $('#new_venue_button').show();
            $('#load_venue_button').hide();
            $('#save_venue_button').hide();
        }
        else {
            $('#new_venue_button').hide();
            $('#load_venue_button').show();
            $('#save_venue_button').show();
        }
    }

    $("#lsd_accordion").accordion({ heightStyle: "fill" }).on("accordionactivate", function (event, ui) {
        setupButtons();
    });

    setupButtons();

    $("#lsd_file_name").attr("value", venue_filename);

    $("#load_save_dialog").dialog("open");

    $("#load_save_dialog_uploadFrame").load( function () { location.reload( true ); });
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
function updateVenueLayout() {

    $.getJSON( "/dmxstudio/rest/query/venue/layout/", function (data) {
        client_config = data;
        for (var prop in client_config) {
            if (prop == "edit_mode")
                setEditMode(client_config.edit_mode);
            else if (prop == "delete_mode")
                setDeleteMode(client_config.delete_mode);
            else if (prop == "sections") {
                for (section in client_config.sections) {
                    if (section == "scene_tiles_pane") {
                        scene_tile_panel.setScrollContent(client_config.sections.scene_tiles_pane.scroll);
                        setSectionCollapsed("scene_tiles_pane", client_config.sections.scene_tiles_pane.collapsed);
                    }
                    else if (section == "chase_tiles_pane") {
                        chase_tile_panel.setScrollContent(client_config.sections.chase_tiles_pane.scroll);
                        setSectionCollapsed("chase_tiles_pane", client_config.sections.chase_tiles_pane.collapsed);
                    }
                    else if (section == "fixture_tiles_pane") {
                        fixture_tile_panel.setScrollContent(client_config.sections.fixture_tiles_pane.scroll);
                        setSectionCollapsed("fixture_tiles_pane", client_config.sections.fixture_tiles_pane.collapsed);
                    }
                    else if (section == "slider_pane") {
                        slider_panel.setScrollContent(client_config.sections.slider_pane.scroll);
                        setSectionCollapsed("slider_pane", client_config.sections.slider_pane.collapsed);
                    }
                    else if (section == "venue_pane") {
                        setSectionCollapsed("venue_pane", client_config.sections.venue_pane.collapsed);
                    }
                    else if (section == "music_pane") {
                        setSectionCollapsed("music_pane", client_config.sections.music_pane.collapsed);
                    }
                }
            }
        }
    } );
}

// ----------------------------------------------------------------------------
//
function saveVenueLayout() {
    client_config = {};
    client_config.edit_mode = edit_mode;
    client_config.delete_mode = delete_mode;

    client_config.sections = {
        "venue_pane": {
            "collapsed": isSectionCollapsed("venue_pane")
        },
        "music_pane": {
            "collapsed": isSectionCollapsed("music_pane")
        },
        "scene_tiles_pane": {
            "scroll": scene_tile_panel.isScrollContent(),
            "collapsed": isSectionCollapsed("scene_tiles_pane")
        },
        "chase_tiles_pane": {
            "scroll": chase_tile_panel.isScrollContent(),
            "collapsed": isSectionCollapsed("chase_tiles_pane")
        },
        "fixture_tiles_pane": {
            "scroll": fixture_tile_panel.isScrollContent(),
            "collapsed": isSectionCollapsed("fixture_tiles_pane")
        },
        "slider_pane": {
            "scroll": slider_panel.isScrollContent(),
            "collapsed": isSectionCollapsed("slider_pane")
        }
    };

    client_config_update = false;

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/edit/venue/layout/save",
        data: JSON.stringify(client_config),
        contentType: 'application/json',
        cache: false,
        error: onAjaxError
    });
}