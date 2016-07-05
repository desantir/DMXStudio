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

var DMX_MAX_UNIVERSES = 4;
var UNUSED = "OFF";

// ----------------------------------------------------------------------------
//
function configureVenue(event) {
    stopEventPropagation(event);

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
function openConfigureVenueDialog(config_json) {
    var capture_devices = config_json.capture_devices;

    $("#configure_venue_dialog").dialog({
        autoOpen: false,
        width: 750,
        height: 550,
        modal: true,
        resizable: false,
        buttons: {
            "Update Venue": function () {

                var json = {
                    name: $("#cvd_name").val(),
                    description: $("#cvd_description").val(),
                    audio_boost: $("#cvd_audio_boost").val(),
                    audio_boost_floor: $("#cvd_audio_boost_floor").val(),
                    audio_sample_size: $("#cvd_audio_sample_size").val(),
                    audio_capture_device: capture_devices[$("#cvd_audio_capture_device").val()],
                    auto_blackout: $("#cvd_auto_blackout").val(),
                    universes: []
                };

                for (var universe = 1; universe <= DMX_MAX_UNIVERSES; universe++) {
                    var port = $("#cvd_dmx_port_" + universe).val();

                    if (port != "") {
                        for (var i = 0; i < json.universes.length; i++)
                            if (json.universes[i].dmx_port == port) {
                                messageBox("Universe " + universe + " port " + port + " is already allocated");
                                return;
                            }

                        var delay = $("#cvd_packet_delay_" + universe).val();
                        var min_delay = $("#cvd_minimum_delay_" + universe).val();
                        var type = $("#cvd_dmx_type_" + universe).val();

                        json.universes.push({ "id": universe, "dmx_port": port, "type": type, "packet_delay_ms": delay, "minimum_delay_ms": min_delay });
                    }
                }

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

    $("#cvd_audio_capture_device").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false });
    $("#cvd_audio_sample_size").multiselect({ minWidth: 150, multiple: false, selectedList: 1, header: false, height: "auto" });

    $("#cvd_audio_capture_device").empty();
    $("#cvd_audio_sample_size").empty();

    $("#cvd_audio_boost").spinner({ min: 0, max: 100, value: 0.0, step: .5 });
    $("#cvd_audio_boost_floor").spinner({ min: 0, max: 100, value: 0.0, step: .1 });
    $("#cvd_auto_blackout").spinner({ min: 0, max: 65000, value: 0, step: 1000 });

    $("#cvd_name").val(config_json.name);
    $("#cvd_description").val(config_json.description);

    $("#cvd_universes").empty();

    var template = $("#cvd_universe_template")[0].innerHTML;

    config_json.ports.splice(0, 0, UNUSED);

    for (var universe_num = 1; universe_num <= DMX_MAX_UNIVERSES; universe_num++) {
        var dmx_port = UNUSED;
        var delay = 100;
        var min_delay = 5;
        var type = 0;

        for (var i = 0; i < config_json.universes.length; i++) {
            if (config_json.universes[i].id == universe_num) {
                dmx_port = config_json.universes[i].dmx_port;
                delay = config_json.universes[i].packet_delay_ms;
                min_delay = config_json.universes[i].minimum_delay_ms;
                type = config_json.universes[i].type;
                break;
            }
        }

        var universe_div_text = template.replace(/NNN/g, universe_num);

        $("#cvd_universes").append(universe_div_text);

        $("#cvd_universe_title_" + universe_num).html("U" + universe_num);

        $("#cvd_dmx_port_" + universe_num).multiselect({ minWidth: 100, multiple: false, selectedList: 1, header: false });
        $("#cvd_dmx_type_" + universe_num).multiselect({ minWidth: 170, multiple: false, selectedList: 1, header: false });
        $("#cvd_packet_delay_" + universe_num).spinner({ min: 50, max: 1000, value: 0 }).val(delay);
        $("#cvd_minimum_delay_" + universe_num).spinner({ min: 0, max: 1000, value: 0 }).val(min_delay);

        for (var i = 0; i < config_json.ports.length; i++) {
            var port = config_json.ports[i];
            $("#cvd_dmx_port_" + universe_num).append($('<option>', {
                value: port == UNUSED ? "" : port,
                text: port,
                selected: port == dmx_port
            }));
        }

        $("#cvd_dmx_port_" + universe_num).multiselect("refresh");

        for (var i = 0; i < config_json.driver_types.length; i++) {
            var driver_type = config_json.driver_types[i];
            $("#cvd_dmx_type_" + universe_num).append($('<option>', {
                value: i,
                text: driver_type,
                selected: type == i
            }));
        }

        $("#cvd_dmx_type_" + universe_num).multiselect("refresh");
    }

    $("#cvd_audio_boost").spinner().val(config_json.audio_boost);
    $("#cvd_audio_boost_floor").spinner().val(config_json.audio_boost_floor);
    $("#cvd_auto_blackout").spinner("value", config_json.auto_blackout);

    for (var i = 1024; i <= 1024 * 8; i *= 2) {
        $("#cvd_audio_sample_size").append($('<option>', {
            value: i,
            text: i,
            selected: i == config_json.audio_sample_size
        }));
    }

    $("#cvd_audio_sample_size").multiselect("refresh");

    for (var i = 0; i < config_json.capture_devices.length; i++) {
        var device = config_json.capture_devices[i];
        $("#cvd_audio_capture_device").append($('<option>', {
            value: i,
            text: device,
            selected: device == config_json.audio_capture_device
        }));
    }

    $("#cvd_audio_capture_device").multiselect("refresh");

    $("#configure_venue_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function loadSaveVenue(event) {
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
                text: "New Venue",
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
                                window.location.reload(true);
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

    $("#load_save_dialog_uploadFrame").load(function () { location.reload(true); });
}

// ----------------------------------------------------------------------------
//
function updateVenueLayout() {

    $.getJSON("/dmxstudio/rest/query/venue/layout/", function (data) {
        client_config = data;
        for (var prop in client_config) {
            if (prop == "edit_mode")
                setEditMode(client_config.edit_mode);
            else if (prop == "playlist") 
                setCurrentPlaylist(client_config.playlist);
            else if (prop == "act") {
                current_act = client_config.act;
                $("#act_" + current_act).prop("checked", true).button("refresh");
                createSceneTiles();
                createChaseTiles();
                update_current_act();
            }
            else if (prop == "sections") {
                for (section in client_config.sections) {
                    if (section == "act_pane") {
                        setSectionCollapsed("act_pane", client_config.sections.act_pane.collapsed);
                    }
                    else if (section == "scene_tiles_pane") {
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
    });
}

// ----------------------------------------------------------------------------
//
function saveVenueLayout() {
    client_config = {};
    client_config.edit_mode = edit_mode;
    client_config.act = current_act;
    client_config.playlist = getCurrentPlaylist();

    client_config.sections = {
        "venue_pane": {
            "collapsed": isSectionCollapsed("venue_pane")
        },
        "music_pane": {
            "collapsed": isSectionCollapsed("music_pane")
        },
        "act_pane": {
            "collapsed": isSectionCollapsed("act_pane")
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