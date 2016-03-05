/* 
Copyright (C) 2012-14 Robert DeSantis
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

var fixtures = new Array();

var fixture_definitions = null;     // Fixture definitions - populated only if needed
var group_colors = false;

var COLOR_CHANNELS_OWNER = -1;
var MASTER_DIMMER_OWNER = -2;

var ignore_next_update = false;     // Prevent race condition when selecting fixtures

var fixture_state_listener = null;  // Listern for fixture state changes

var slider_color_channels = [
    { "channel": 1, "label": "red", "name": "Master Red", "value": 0, "max_value":255, "ranges": null, "type": 1, "color": "rgb(255,0,0)" },
    { "channel": 2, "label": "green", "name": "Master Green", "value": 0, "max_value": 255, "ranges": null, "type": 2, "color": "rgb(0,255,0)" },
    { "channel": 3, "label": "blue", "name": "Master Blue", "value": 0, "max_value": 255, "ranges": null, "type": 3, "color": "rgb(0,0,255)" },
    { "channel": 5, "label": "white", "name": "Master White", "value": 0, "max_value": 255, "ranges": null, "type": 5, "color": "rgb(255,255,255)" }
];

// ----------------------------------------------------------------------------
// class Fixture (also Fixture Group)
//
function Fixture(fixture_data)
{
    // Constructor
    jQuery.extend(this, fixture_data);

    // method getId
    this.getId = function () {
        return this.id;
    }

    // method getNumber
    this.getNumber = function () {
        return this.number;
    }

    // method getName
    this.getName = function () {
        return this.name;
    }

    // method getFullName
    this.getFullName = function () {
        return this.full_name;
    }

    // method getDescription
    this.getDescription = function () {
        return this.description;
    }

    // method getNumChannels
    this.getNumChannels = function () {
        return this.num_channels;
    }

    // method getChannels
    this.getChannels = function () {
        return this.channels;
    }

    // method getChannelValues (may be empty)
    this.hasChannelValues = function () {
        return this.has_channel_values;
    }

    // method getUserTypeName
    this.getUserTypeName = function () {
        return this.is_group ? "Fixture Group" : "Fixture";
    }

    // method isGroup
    this.isGroup = function () {
        return this.is_group;
    }

    // method isActive 
    this.isActive = function () {
        return this.is_active;
    }

    // method setActive 
    this.setActive = function ( active ) {
        this.is_active = active;
    }

    // method getFixtures
    this.getFixtureIds = function () {
        return this.fixture_ids;
    }

    // method getFUID
    this.getFUID = function () {
        return this.fuid;
    }

    // method getManufacturer
    this.getManufacturer = function () {
        return this.manufacturer;
    }

    // method getModel
    this.getModel = function () {
        return this.model;
    }

    // method getTypeName
    this.getTypeName = function () {
        return this.type_name;
    }

    // method getDMXAddress
    this.getDMXAddress = function () {
        return this.dmx_address;
    }

    // method getUniverse
    this.getUniverseId = function () {
        return this.dmx_universe;
    }

    // method getLabel
    this.getLabel = function () {
        var label = "";
        if (this.isGroup())
            label = "G";
        return label + this.getNumber() + ": " + this.getFullName();
    }
}

// ----------------------------------------------------------------------------
//
function getFixtureById(id) {
    for (var i = 0; i < fixtures.length; i++)
        if (fixtures[i].getId() == id)
            return fixtures[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getFixtureGroupByNumber(number) {
    for (var i = 0; i < fixtures.length; i++)
        if (fixtures[i].isGroup() && fixtures[i].getNumber() == number)
            return fixtures[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getUnusedFixtureGroupNumber() {
    for (var i = 1; i < 50000; i++)
        if (getFixtureGroupByNumber(i) == null)
            return i;
    return 99999;
}

// ----------------------------------------------------------------------------
//
function getFixtureByNumber(number) {
    for (var i = 0; i < fixtures.length; i++)
        if (!fixtures[i].isGroup() && fixtures[i].getNumber() == number)
            return fixtures[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getUnusedFixtureNumber() {
    for (var i = 1; i < 50000; i++)
        if (getFixtureByNumber(i) == null)
            return i;
    return 99999;
}

// ----------------------------------------------------------------------------
// 
function getActiveFixtures() {
    var list = [];
    for (var i = 0; i < fixtures.length; i++)
        if (fixtures[i].isActive())
            list.push(fixtures[i]);
    return list;
}

// ----------------------------------------------------------------------------
//
function updateCapturedFixtures(captured_fixtures) {
    if (!ignore_next_update) {
        for (var i = 0; i < fixtures.length; i++) {
            var fixture = fixtures[i];
            var captured = captured_fixtures.indexOf(fixture.id) > -1;
            var active = fixture.isActive();

            if (active && !captured) {
                slider_panel.releaseChannels(fixture.id);
                markActiveFixture(fixture.id, false);
            }
            else if (!active && captured) {
                loadFixtureChannels(fixture.id, null, false);
            }
        }
    }
    else
        ignore_next_update = false;
}

// ----------------------------------------------------------------------------
//
function updateFixtures() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/fixtures/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            fixture_tile_panel.empty();
            fixtures = []

            $("#copy_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");
            $("#clear_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");

            arrangeSliders(null);          // Reset sliders, add colors and other static channels

            $.map(json, function (fixture, index) {
                for (var i = 0; i < fixture.channels.length; i++)
                    fixture.channels[i].default_value = fixture.channels[i].value;

                fixture = new Fixture(fixture);     // Make sure we are using the "real" object going forward
                fixtures.push(fixture);

                var tile;
                if (fixture.is_group)
                    tile = fixture_tile_panel.addTile(fixture.id, "G" + fixture.number, escapeForHTML(fixture.full_name), true, "FixtureGroup", "fixture group");
                else
                    tile = fixture_tile_panel.addTile(fixture.id, fixture.number, escapeForHTML(fixture.full_name), true);

                if (fixture.is_active) {
                    fixture.is_active = false;      // For the purposes of the current UI state the fixture is not active

                    loadFixtureChannels(fixture.id, null, false);
                }
            });

            highlightSceneFixtures(active_scene_id, true);

            setEditMode(edit_mode);         // Refresh editing icons on new tiles
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function hoverFixtureGroup(id, enter) {
    hoverFixture(is, enter);
}

// ----------------------------------------------------------------------------
//
function hoverFixture(id, enter) {
    var fixture = getFixtureById(id);
    if (fixture != null && fixture.isActive())
        slider_panel.highlightChannels(id,enter);
}

// ----------------------------------------------------------------------------
//
function highlightFixtures(fixtureIds, highlight) {
    for (var i = 0; i < fixtureIds.length; i++)
        fixture_tile_panel.highlightTile(fixtureIds[i], highlight);
}

// ----------------------------------------------------------------------------
//
function controlFixture(event, fixture_id) {
    controlFixture2(event, fixture_id, null, true);
}

// ----------------------------------------------------------------------------
//
function controlFixtureGroup(event, fixture_group_id) {
    controlFixture2(event, fixture_group_id, null, true);
}

// ----------------------------------------------------------------------------
//
function controlFixture2(event, fixture_id, preload_channel_values, tile_clicked) {
    stopEventPropagation(event);

    var fixture = getFixtureById(fixture_id);
    if (fixture == null)
        return;

    var json = {
        "id": fixture_id,
        "is_capture": !fixture.isActive()
    };
    
    if ( preload_channel_values != null ) 
        json.channel_values = preload_channel_values;

    var what = fixture.isGroup() ? "fixturegroup" : "fixture";

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/" + what + "/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        success: function (data) {
            if ( !fixture.isActive() ) {        // Make active (captured)
                var channel_data = jQuery.parseJSON(data);
                loadFixtureChannels(fixture_id, channel_data, tile_clicked);
            }
            else {                              // Release
                slider_panel.releaseChannels(fixture_id);
                markActiveFixture(fixture_id, false);
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function loadFixtureChannels( fixture_id, updated_channel_data, tile_clicked ) {
    markActiveFixture(fixture_id, true);

    var fixture = getFixtureById(fixture_id);
    var channels = fixture.getChannels();

    var channels_to_load = [];
    var json = [];

    // Generate labels and values
    for (var i = 0; i < channels.length; i++) {
        var channel = channels[i];

        if (group_colors) {
            var skip = false;
            
            for (var j = 0; j < slider_color_channels.length; j++) {
                if (channel.type == slider_color_channels[j].type) {
                    json.push({ "actor_id": fixture.getId(), "channel": channel.channel, "value": slider_color_channels[j].value });
                    skip = true;
                    break;
                }
            }
            
            if ( skip )
                continue;
        }

        channel.label = ((fixture.isGroup()) ? "G" : "") + fixture.getNumber() + "-" + (i + 1);
        if ( updated_channel_data != null )
            channel.value = updated_channel_data[i];

        channel.max_value = 255;

        channels_to_load.push(channel);
    }

    slider_panel.allocateChannels(fixture_id, fixture.getFullName(), channels_to_load, fixture_slider_callback);

    if (tile_clicked)
        slider_panel.highlightChannels(fixture_id, true);

    ignore_next_update = tile_clicked;      // Prevent race condition with venue status update capture list

    // Update colors with current values
    if (json.length > 0) {
        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/control/fixture/channels/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            success: function () {
                updateFixtureChannelValues(json);
            },
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function clearFixtures(event) {
    stopEventPropagation(event);

    var json = { "id": 0, "is_capture": false };

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        success: function ( ) {
            var active_fixtures = getActiveFixtures();
            for (var i = 0; i < active_fixtures.length; i++) {
                slider_panel.releaseChannels(active_fixtures[i].getId());
                markActiveFixture(active_fixtures[i].getId(), false);
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function fixture_slider_callback(fixture_id, channel, value) {
    var json = [{
        "actor_id": fixture_id,
        "channel": channel,
        "value": value
    }];

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/channels/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        async: false,   // These cannot get out of sync
        cache: false,
        success: function () {
            updateFixtureChannelValues(json);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function fixture_slider_colors_callback(unused, channel_type, value) {
    var json = [];

    // Store color current value
    for (var j = 0; j < slider_color_channels.length; j++) {
        if (channel_type == slider_color_channels[j].type) {
            slider_color_channels[j].value = value;
            break;
        }
    }

    // Re-populate all fixtures
    $.map(getActiveFixtures(), function (fixture) {
        for (var j = 0; j < fixture.channels.length; j++) {
            if (fixture.channels[j].type == channel_type)
                json.push({ "actor_id": fixture.getId(), "channel": fixture.channels[j].channel, "value": value });
        }
    });

    if (json.length == 0)
        return;

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/channels/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        async: false,   // These cannot get out of sync
        cache: false,
        success: function () {
            updateFixtureChannelValues(json);
        },

        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Update local channel values based on server packet info
//
function updateFixtureChannelValues( channel_info ) {
    for (var i = 0; i < channel_info.length; i++) {
        var fixture = getFixtureById(channel_info[i].actor_id);
        if (channel_info[i].channel < fixture.getNumChannels())
            fixture.getChannels()[channel_info[i].channel].value = channel_info[i].value;
    }
}

// ----------------------------------------------------------------------------
//
function groupFixtureSliderColors(event) {
    group_colors = !group_colors;
    arrangeSliders(event);
}

// ----------------------------------------------------------------------------
//
function arrangeSliders(event) {
    stopEventPropagation(event);

    slider_panel.releaseAllChannels();

    // Add venue master dimmer
    slider_panel.allocateChannels(MASTER_DIMMER_OWNER, "Venue Dimmer", [master_dimmer_channel], master_dimmer_callback);
    master_dimmer_channel.slider = slider_panel.findChannel(MASTER_DIMMER_OWNER, master_dimmer_channel.channel);

    // Add color masters if enabled
    if (group_colors) {
        $("#channel_panel_groupcolors").removeClass('ui-icon').addClass('ui-icon-white');

        slider_panel.allocateChannels(COLOR_CHANNELS_OWNER, "Colors", slider_color_channels, fixture_slider_colors_callback);
    }
    else {
        $("#channel_panel_groupcolors").removeClass('ui-icon-white').addClass('ui-icon');
    }

    $.map(getActiveFixtures(), function (fixture) {
        loadFixtureChannels(fixture.getId(), null, false);
    });

    slider_panel.trimUnused();
}

// ----------------------------------------------------------------------------
//
function createFixture(event) {
    stopEventPropagation(event);

    var default_number = getUnusedFixtureNumber();
    var default_dmx_address = 0;

    // Default to an unused address if needed
    var dmx_address_map = buildDmxAddressMap( 1 );

    for (var d = 0; d < 512; d++)
        if (dmx_address_map[d] == 0) {
            default_dmx_address = d + 1;
            break;
        }

    openNewFixtureDialog("Create Fixture", {
        id: 0,
        name: "Somewhere",
        description: "",
        number: default_number,
        fuid: 0,
        allow_overlap: false,
        dmx_universe: 1,
        dmx_address: default_dmx_address
    });
}

// ----------------------------------------------------------------------------
//
function editFixture(event, fixture_id) {
    stopEventPropagation(event);

    var fixture = getFixtureById(fixture_id);
    if (fixture == null)
        return;

    openNewFixtureDialog("Update Fixture", {
        id: fixture.getId(),
        name: fixture.getName(),
        description: fixture.getDescription(),
        number: fixture.getNumber(),
        fuid: fixture.getFUID(),
        dmx_address: fixture.getDMXAddress(),
        allow_overlap: false,
        dmx_universe: fixture.getUniverseId()
    });
}

// ----------------------------------------------------------------------------
//
function openNewFixtureDialog(dialog_title, fixture_data) {
    if (fixture_definitions == null) {
        // Get fixture definitions only once
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/fixture/definitions/",
            cache: false,
            success: function ( data ) {
                fixture_definitions = JSON.parse(data);
                createNewFixtureDialog(dialog_title, fixture_data);
            },
            error: onAjaxError
        });

        return;
    }

    createNewFixtureDialog(dialog_title, fixture_data);
}

// ----------------------------------------------------------------------------
//
function createNewFixtureDialog(dialog_title, data) {
    if (fixture_definitions == null)
        return;

    // Default to some fixture
    if (data.fuid == 0)
        data.fuid = fixture_definitions[0].fixtures[0].personalities[0].fuid;

    var send_update = function () {
        var json = {
            id: data.id,
            name: $("#nfd_name").val(),
            description: $("#nfd_description").val(),
            number: parseInt($("#nfd_number").val()),
            fuid: $("#nfd_personality").val(),
            dmx_address: parseInt($("#nfd_dmx_address").val()),
            dmx_universe: parseInt($("#nfd_dmx_universe").val())
        };

        if (json.fuid == null) {
            messageBox("You must select a fixture");
            return;
        }

        if (json.dmx_universe < 0 || json.dmx_universe >= DMX_MAX_UNIVERSES ) {
            messageBox("Invalid universe - choose a universe 1 through " + DMX_MAX_UNIVERSES);
            return;
        }


        // Make sure this is the same fixture number or unused
        if (json.number != data.number && getFixtureByNumber(json.number) != null) {
            messageBox("Fixture number " + json.number + " is already in use");
            return;
        }

        // Make sure no DMX addresses overlap (unless allowed)
        if (!$("#nfd_allow_overlap").is(":checked")) {
            var info = findFixtureDefinition(json.fuid);
            var dmx_address_map = buildDmxAddressMap(json.dmx_universe);
            
            for (var i=json.dmx_address-1; i < json.dmx_address-1 + info.num_channels; i++) {
                if (dmx_address_map[i] != 0 && dmx_address_map[i] != json.id) {
                    messageBox("DMX address overlaps existing fixture at address " + (i + 1));
                    return;
                }
            }
        }

        var action = (json.id == 0) ? "create" : "update";

        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/edit/fixture/" + action + "/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            success: function () {
                updateFixtures();
            },
            error: onAjaxError
        });

        $("#new_fixture_dialog").dialog("close");
    };

    var dialog_buttons = new Array();

    dialog_buttons[dialog_buttons.length] = {
        text: dialog_title,
        click: send_update
    };

    dialog_buttons[dialog_buttons.length] = {
        text: "Cancel",
        click: function () {
            $("#new_fixture_dialog").dialog("close");
        }
    };

    $("#new_fixture_dialog").dialog({
        autoOpen: false,
        width: 540,
        height: 620,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_fixture_dialog").dialog("option", "title", dialog_title + " " + data.number);

    $("#nfd_number").spinner({ min: 1, max: 100000 }).val(data.number);
    $("#nfd_name").val(data.name);
    $("#nfd_description").val(data.description);
    $("#nfd_dmx_address").spinner({ min: 1, max: 512 }).val(data.dmx_address);
    $("#nfd_dmx_universe").spinner({ min: 1, max: DMX_MAX_UNIVERSES }).val(data.dmx_universe);
    $("#nfd_allow_overlap").attr('checked', data.allow_overlap);

    selectFixtureByFuid(data.fuid,false);

    $("#nfd_manufacturer").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select manufacturer' });
    $("#nfd_model").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select model' });
    $("#nfd_personality").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select personality' });

    if (data.id == 0) {
        $("#nfd_manufacturer").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            if (ui.checked)
                selectFixtureByFuid(fixture_definitions[parseInt(ui.value)].fixtures[0].personalities[0].fuid, true);
        });

        $("#nfd_model").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            if (ui.checked) {
                selectFixtureByFuid(fixture_definitions[$("#nfd_manufacturer").val()].fixtures[parseInt(ui.value)].personalities[0].fuid, true);
            }
        });

        $("#nfd_manufacturer").multiselect("enable");
        $("#nfd_model").multiselect("enable");
        $("#nfd_personality").multiselect("enable");
    }
    else {
        $("#nfd_manufacturer").multiselect("disable");
        $("#nfd_model").multiselect("disable");
        $("#nfd_personality").multiselect("disable");
    }

    $("#nfd_choose_dmx").click(function (event) {
        stopEventPropagation(event);

        var info = findFixtureDefinition($("#nfd_personality").val());

        showDmxAddressChooser(
            parseInt($("#nfd_dmx_universe").val()),
            parseInt($("#nfd_number").val()),
            info.num_channels);
    });

    $("#new_fixture_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function buildDmxAddressMap(universe_id) {
    // Populate DMX addresses array
    dmx_address_map = [];

    for (var i = 0; i < 512; i++)
        dmx_address_map.push(0);

    for (var i = 0; i < fixtures.length; i++) {
        if (!fixtures[i].isGroup() && fixtures[i].getUniverseId() == universe_id) {
            var address = fixtures[i].getDMXAddress() - 1;
            for (var d = address; d < address + fixtures[i].getNumChannels() ; d++)
                dmx_address_map[d] = fixtures[i].getId();
        }
    }

    return dmx_address_map;
}

// ----------------------------------------------------------------------------
//
function showDmxAddressChooser(universe_id, fixture_number, num_channels) {
    $("#choose_dmx_address_dialog").dialog({
        autoOpen: false,
        width: 710,
        height: 700,
        modal: true,
        resizable: false,
        title: "Select Address in DMX Universe " + (universe_id)
    });

    var dmx_address_map = buildDmxAddressMap(universe_id);

    var update_tiles = function ( mode ) {
        $("#cda_addresses").empty();

        var html = "<div onclick='selectDmxAddress(event);'>";
        var dmx = 1;
        var allow_overlap = $("#nfd_allow_overlap").is(":checked");
        var fixture = null;

        for (var row = 0; row < 32; row++) {
            for (var col = 0; col < 16; col++) {
                var tile_locked = false;
                var clazz = (dmx_address_map[dmx - 1] == 0) ? "dmx_tile_unused" : "dmx_tile_used";

                if (!allow_overlap) {
                    for (var i = 0; i < num_channels; i++) {
                        var index = dmx - 1 + i;
                        if (index >= 512 ||
                             (dmx_address_map[index] != 0 && dmx_address_map[index] != fixture_number)) { // Does not fit
                            tile_locked = true;
                            break;
                        }
                    }
                }

                if (allow_overlap || !tile_locked)
                    clazz += " dmx_tile_unlocked";
                else
                    clazz += " dmx_tile_locked";

                html += "<div class='dmx_tile " + clazz + "' id='dmxaddr_" + dmx + "' ";

                var style = "";
                if (row < 31)
                    style += "border-bottom: 0px; ";
                if (col < 15)
                    style += "border-right: 0px; ";
                if (col == 0)
                    style += "clear:both;";

                if (dmx_address_map[dmx - 1] != 0) {
                    if (fixture == null || fixture.getId() != dmx_address_map[dmx - 1])
                        fixture = getFixtureById(dmx_address_map[dmx - 1])

                    html += "title='" + escapeForHTML(fixture.getFullName()) + "' ";
                }
                else
                    fixture = null;

                var contents = "";
                if (mode == 0)
                    contents = dmx;
                else if ( fixture != null )
                    contents = fixture.getNumber();

                html += "style='" + style + "'>" + contents + "</div>";
                dmx++;
            }
        }

        html += "</div>";

        $("#cda_addresses").html(html);
    }

    $("#cda_show_buttons").buttonset();
    $("#cda_show_buttons").change(function () {
        update_tiles($('input[name=cda_show]:checked').val());
    });

    update_tiles( $('input[name=cda_show]:checked').val() );

    $("#choose_dmx_address_dialog").dialog("open");
}

function selectDmxAddress(event, address) {
    stopEventPropagation(event);

    var tile = $(event.srcElement);
    if (!tile.hasClass("dmx_tile_unlocked"))
        return;

    var address = parseInt( tile.attr('id').substr( 8 ) );
    $("#choose_dmx_address_dialog").dialog("close");
    $("#nfd_dmx_address").spinner("value", address);
}

// ----------------------------------------------------------------------------
//
function findFixtureDefinition(fuid) {
    if ( !fixture_definitions ) 
        return null;

    for (var m = 0; m < fixture_definitions.length; m++) {
        for (var f = 0; f < fixture_definitions[m].fixtures.length; f++) {
            for (var p = 0; p < fixture_definitions[m].fixtures[f].personalities.length; p++) {
                if (fuid == fixture_definitions[m].fixtures[f].personalities[p].fuid) {
                    return { 
                        manufacturer: m,
                        fixture: f,
                        personality: p,
                        "fuid": fuid,
                        num_channels: fixture_definitions[m].fixtures[f].personalities[p].num_channels
                    };
                }
            }
        }
    }

    return null;
}

// ----------------------------------------------------------------------------
//
function selectFixtureByFuid(fuid,refresh) {
    $("#nfd_manufacturer").empty();
    $("#nfd_model").empty();
    $("#nfd_personality").empty();

    var info = findFixtureDefinition(fuid);
    if ( info == null )
        return;

    for (var i=0; i < fixture_definitions.length; i++) {
        $("#nfd_manufacturer").append($('<option>', {
            value:i,
            text: fixture_definitions[i].manufacturer,
            selected: i == info.manufacturer
        }));
    }

    var fixtures = fixture_definitions[info.manufacturer].fixtures;
    for (var i = 0; i < fixtures.length; i++) {
        $("#nfd_model").append($('<option>', {
            value: i,
            text: fixtures[i].model,
            selected: i == info.fixture
        }));
    }

    var personalities = fixtures[info.fixture].personalities;
    for (var i = 0; i < personalities.length; i++) {
        $("#nfd_personality").append($('<option>', {
            value: personalities[i].fuid,
            text: personalities[i].num_channels > 1 ? (personalities[i].num_channels + " channels") : "1 channel",
            selected: i == info.personality
        }));
    }

    if (refresh) {
        $("#nfd_manufacturer").multiselect("refresh");
        $("#nfd_model").multiselect("refresh");
        $("#nfd_personality").multiselect("refresh");
    }
}

// ----------------------------------------------------------------------------
//
function deleteFixture(event, fixture_id) {
    stopEventPropagation(event);

    deleteVenueItem(getFixtureById(fixture_id), function (item) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/delete/fixture/" + item.getId(),
            cache: false,
            success: updateFixtures,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function describeFixture(event, fixture_id) {
    stopEventPropagation(event);

    var fixture = getFixtureById( fixture_id );

    $("#describe_fixture_dialog").dialog({
        autoOpen: false,
        width: 600,
        height: 500,
        modal: false,
        draggable: true,
        resizable: false,
        title: "Fixture " + fixture.getNumber() + ": " + escapeForHTML(fixture.getManufacturer()) + " " + escapeForHTML(fixture.getModel())
    });

    $("#describe_fixture_number").html(escapeForHTML(fixture.getNumber()));
    $("#describe_fixture_type").html(escapeForHTML(fixture.getTypeName()));
    $("#describe_fixture_manufacturer").html(escapeForHTML(fixture.getManufacturer()));
    $("#describe_fixture_model").html(escapeForHTML(fixture.getModel()));
    $("#describe_fixture_name").html(escapeForHTML(fixture.getName()));
    $("#describe_fixture_description").html(escapeForHTML(fixture.getDescription()));
    $("#describe_fixture_address").html(fixture.getUniverseId() + " - " + fixture.getDMXAddress());

    var info = "";
    var channels = fixture.getChannels();

    for (var i = 0; i < channels.length; i++) {
        var type_name = channels[i].name.indexOf(channels[i].type_name) != -1 ? "" : (" (" + escapeForHTML(channels[i].type_name) + ")");

        info += "<div style=\"clear:both; float: left;\" >CH " + (channels[i].channel + 1) + " " + channels[i].name + type_name + "</div>";
        if (channels[i].ranges.length > 0) {
            info += "<div class=\"ui-icon-info ui-icon-white\" style=\"float:left; margin-left:10px; margin-top:2px;\"";
            info += " onclick=\"describe_fixture_show_range( event, " + i + ");\"></div>";

            info += "<div id=\"range_" + i + "\" class=\"describe_fixture_range\">";
            for (var j = 0; j < channels[i].ranges.length; j++) {
                info += channels[i].ranges[j].start + " - " + channels[i].ranges[j].end + " : " + escapeForHTML(channels[i].ranges[j].name) + "<br/>"
            }
            info += "</div>";
        }
    }

    $("#describe_fixture_channel_info").html(info);
    $("#describe_fixture_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function describe_fixture_show_range(event, channel_number) {
    stopEventPropagation(event);

    var range = $("#range_" + channel_number);

    if (range.css("display") == "none")
        range.css("display", "inline");
    else
        range.css("display", "none");
}

// ----------------------------------------------------------------------------
//
function createFixtureGroup(event) {
    stopEventPropagation(event);

    var new_number = getUnusedFixtureGroupNumber();
    var default_fixtures = new Array();

    fixture_tile_panel.iterate(function (id) {
        var fixture = getFixtureById(id);
        if (!fixture.isGroup() && fixture.isActive())
            default_fixtures[default_fixtures.length] = fixture.getId();
    });

    openNewFixtureGroupDialog("Create Fixture Group", {
        id: 0,
        name: "New Fixture group " + new_number,
        description: "",
        number: new_number,
        fixture_ids: default_fixtures,
        channel_data: null
    });
}

// ----------------------------------------------------------------------------
//
function editFixtureGroup(event, fixture_group_id) {
    stopEventPropagation(event);

    var group = getFixtureById(fixture_group_id);
    if (group == null)
        return;

    openNewFixtureGroupDialog("Update Fixture Group", {
        id: group.getId(),
        name: group.getName(),
        description: group.getDescription(),
        number: group.getNumber(),
        fixture_ids: group.getFixtureIds(),
        channel_data: $.extend( true, [], group.getChannels() )
    });
}

// ----------------------------------------------------------------------------
//
function openNewFixtureGroupDialog(dialog_title, data) {
    var sliders = new Array();

    var send_update = function () {
        var channel_data = new Array();
        for (var i = 0; i < sliders.length; i++)
            channel_data[i] = sliders[i].getValue();
        $("#nfgd_sliders").empty();

        var json = {
            id: data.id,
            name: $("#nfgd_name").val(),
            description: $("#nfgd_description").val(),
            number: parseInt($("#nfgd_number").val()),
            fixture_ids: $("#nfgd_fixtures").val(),
            channel_values: channel_data
        };

        if (json.number != data.number && getFixtureGroupByNumber(json.number) != null) {
            messageBox("Fixture group number " + json.number + " is already in use");
            return;
        }

        if (fixtures.length == 0) {
            messageBox("No fixtures are selected");
            return;
        }

        var action = (json.id == 0) ? "create" : "update";

        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/edit/fixturegroup/" + action + "/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            success: function () {
                updateFixtures();
            },
            error: onAjaxError
        });

        $("#new_fixture_group_dialog").dialog("close");
    };

    var dialog_buttons = new Array();

    dialog_buttons[dialog_buttons.length] = {
        text: dialog_title,
        click: send_update
    };

    dialog_buttons[dialog_buttons.length] = {
        text: "Cancel",
        click: function () {
            $("#new_fixture_group_dialog").dialog("close");
        }
    };

    $("#new_fixture_group_dialog").dialog({
        autoOpen: false,
        width: 620,
        height: 600,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_fixture_group_dialog").dialog("option", "title", dialog_title + " " + data.number);

    $("#nfgd_accordion").accordion({ heightStyle: "fill" });

    $("#nfgd_number").spinner({ min: 1, max: 100000 }).val(data.number);
    $("#nfgd_name").val(data.name);
    $("#nfgd_description").val(data.description);

    $("#nfgd_fixtures").empty();

    fixture_tile_panel.iterate(function (id) {
        var fixture = getFixtureById(id);
        if (!fixture.isGroup()) {
            $("#nfgd_fixtures").append($('<option>', {
                value: fixture.getId(),
                text: fixture.getNumber() + ": " + fixture.getFullName(),
                selected: jQuery.inArray(fixture.getId(), data.fixture_ids) != -1
            }));
        }
    });

    $("#nfgd_fixtures").multiselect({
        minWidth: 500,
        multiple: true,
        noneSelectedText: "Select Fixture(s) in group",
        checkAllText: "Select all fixtures"
    });

    var proxy = new PanelProxy();

    if (data.channel_data != null) {
        var channel_sliders = $("#nfgd_sliders");
        $("#nfgd_sliders").css("width", data.channel_data.length * 45 + 50);
        channel_sliders.empty();

        for (var i = 0; i < data.channel_data.length; i++) {
            var slider_frame = $(slider_template.innerHTML);
            channel_sliders.append(slider_frame);
            data.channel_data[i].value = data.channel_data[i].default_value;
            sliders[i] = new Slider(proxy, i, slider_frame);
            data.channel_data[i].label = "CH " + (i + 1);
            sliders[i].allocate(1, "test", data.channel_data[i], proxy.fixture_slider_callback );
        }
    }

    $("#new_fixture_group_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function deleteFixtureGroup(event, fixture_group_id) {
    stopEventPropagation(event);

    deleteVenueItem(getFixtureById(fixture_group_id), function (item) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/delete/fixturegroup/" + item.getId(),
            cache: false,
            success: updateFixtures,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function copyFixtures(event) {
    stopEventPropagation(event);

    if (!fixture_tile_panel.anyActive())
        return;

    $("#copy_workspace_fixtures_dialog").dialog({
        autoOpen: false,
        width: 530,
        height: 340,
        modal: true,
        resizable: false,
        buttons: {
            "Copy Fixtures": function () {
                var selected_fixtures = $("#copy_fixture_fixtures").val();
                if ( selected_fixtures.length == 0 )
                    return;

                var json = {
                    scene_id: $("#copy_fixture_scene").val(),
                    clear: $("#copy_fixture_remove").is(':checked'),
                    fixture_ids: selected_fixtures,
                    keep_groups: $("#copy_fixture_keep_groups").is(":checked")
                };

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/edit/scene/copy_fixtures/",
                    data: JSON.stringify( json ),
                    contentType: 'application/json',
                    cache: false,
                    async: false,
                    success: function () {
                        updateScenes();

                        if (json.clear)
                            clearFixtures(null);
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

    $("#copy_fixture_scene").empty();
    $("#copy_fixture_fixtures").empty();

    // Fill in selected fixtures
    fixture_tile_panel.iterate(function (id) {
        var fixture = getFixtureById(id);
        if (fixture.isActive()) {
            var title = fixture.getNumber() + ": " + fixture.getFullName();
            if (fixture.isGroup())
                title = "G" + title;

            $("#copy_fixture_fixtures").append($('<option>', {
                value: fixture.getId(),
                text: title,
                selected: true
            }));
        }
    });

    // Fill in scenes
    for (var i = 0; i < scenes.length; i++) {
        if (scenes[i].isDefault())
            continue;

        $("#copy_fixture_scene").append($('<option>', {
            value: scenes[i].getId(),
            text: scenes[i].getNumber() + ": " + scenes[i].getFullName(),
            selected: scenes[i].isActive()
        }));
    }

    $("#copy_fixture_fixtures").multiselect({
        minWidth: 500,
        multiple: true,
        noneSelectedText: "Select Fixture(s) To Copy",
        checkAllText: "Copy all fixtures"
    });

    $("#copy_fixture_scene").multiselect({
        minWidth: 500,
        multiple: false,
        noneSelectedText: "Select Scene",
        selectedList: 1,
        header: false
    });

    $("#copy_workspace_fixtures_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function describeFixtureGroup(event, fixture_group_id) {
    stopEventPropagation(event);

    var group = getFixtureById(fixture_group_id);

    $("#describe_fixture_groups_dialog").dialog({
        autoOpen: false,
        width: 600,
        height: 500,
        modal: false,
        draggable: true,
        resizable: false,
        title: "Fixture Group " + group.getNumber() + ": " + escapeForHTML(group.getName())
    });

    $("#describe_fixturegroup_number").html("G" + group.getNumber());
    $("#describe_fixturegroup_name").html(escapeForHTML(group.getName()));
    $("#describe_fixturegroup_description").html(escapeForHTML(group.getDescription()));

    var fixture_ids = group.getFixtureIds();
    var info = "";
    for (var i = 0; i < fixture_ids.length; i++) {
        var fixture = getFixtureById(fixture_ids[i]);
        info += "<div style=\"clear:both; float: left;\" >Fixture " + fixture.getNumber() + " " +
                escapeForHTML(fixture.getFullName()) + "</div>";
    }

    var channel_data = "";
    if ( group.hasChannelValues() ) {
        channel_info_html = makeChannelInfoLine(group.getChannels(), "describe_fixture_group_channels");
    }

    $("#describe_fixture_group_channels").html(channel_info_html);
    $("#describe_fixture_group_info").html(info);
    $("#describe_fixture_groups_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function markActiveFixture(fixture_id, new_state) {
    var active_scroll_item = fixture_tile_panel.getTile(fixture_id);
    var fixture = getFixtureById(fixture_id);
    if (fixture == null)
        return;

    var is_active = fixture.isActive();

    if (new_state != is_active) {
        fixture_tile_panel.selectTile(fixture_id, new_state);

        fixture.setActive(new_state);

        if ( fixture_state_listener != null )
            fixture_state_listener(fixture_id, new_state);
    }

    if (fixture_tile_panel.anyActive()) {
        $("#copy_fixtures_button").removeClass("ui-icon").addClass("ui-icon-white");
        $("#clear_fixtures_button").removeClass("ui-icon").addClass("ui-icon-white");
    }
    else {
        $("#copy_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");
        $("#clear_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");
    }
}

// ----------------------------------------------------------------------------
//
function getColorChannelRGB() {

    var red = 0;
    var blue = 0;
    var green = 0;

    slider_panel.iterateChannels(function (slider) {
        switch (slider.getType()) {
            case 1:
                red = slider.getValue();
                break;
            case 2:
                green = slider.getValue();
                break;
            case 3:
                blue = slider.getValue();
                break;
        }
    });

    if (red > 0 || green > 0 || blue > 0)
       return "#" + (red).toString(16) + (green).toString(16) + (blue).toString(16);

    return null;
}

// ----------------------------------------------------------------------------
//
function setColorChannelRGB(hsb, hex, rgb) {
    slider_panel.iterateChannels(function (slider) {
        switch (slider.getType()) {
            case 1:
                slider.setValue(rgb.r);
                break;
            case 2:
                slider.setValue(rgb.g);
                break;
            case 3:
                slider.setValue(rgb.b);
                break;
        }
    });
}

// ----------------------------------------------------------------------------
//
function setPanTiltChannels(pan, tilt) {

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

    slider_panel.iterateChannels(function (slider) {
        switch (slider.getType()) {
            case 9: // pan
                slider.setValue(getValue(slider,pan));
                break;
            case 10: //tilt
                slider.setValue(getValue(slider, tilt));
                break;
        }
    });
}

// ----------------------------------------------------------------------------
//
function PanelProxy() {
    this.isBusy = function () { return false; }
    this.isTrackSlider = function () { return false; }
    this.fixture_slider_callback = function (fixture_id, channel, value) {}
    this.setSliderTitle = function (title) { }
}

// ----------------------------------------------------------------------------
//
var panTiltInit = true;
var panTimer = null;

function panTilt(event) {
    stopEventPropagation(event);

    if (panTiltInit) {
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
            position: { my: "right top", at: "left bottom", of: channel_pan_tilt },
            open: function (event, ui) {
                $(this).parent().css('height', '190px');
                $(document).bind('mousedown', function (event) {
                    stopEventPropagation(event);
                    $("#pan_tilt_popup").dialog('close').data('hide_ms', new Date().getTime());
                });

                $("#channel_pan_tilt").removeClass('ui-icon').addClass('ui-icon-white');

                $("body").css("overflow", "hidden");
            },
            close: function (event, ui) {
                $(document).unbind('mousedown');
                $("#channel_pan_tilt").removeClass('ui-icon-white').addClass('ui-icon');
                $("body").css("overflow", "auto");
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

                if (panTimer)
                    clearTimeout(panTimer);

                panTimer = setTimeout(function () { setPanTiltChannels(location.pan, location.tilt) }, 250);
            }
        });

        function set_degrees(event, ui) {
            stopEventPropagation(event);

            var pan_degrees = parseInt($("#ptp_pan").spinner('value'));
            var tilt_degrees = parseInt($("#ptp_tilt").spinner('value'));

            $("#ptp_control").pan_tilt_control('set_location', { pan: pan_degrees, tilt: tilt_degrees });

            setPanTiltChannels(pan_degrees, tilt_degrees);
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

        panTiltInit = false;
    }

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

    setPanTiltChannels(pan_degrees, tilt_degrees);
}