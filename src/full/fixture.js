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

var fixtures = new Array();

var fixture_definitions = null;     // Fixture definitions - populated only if needed
var dmx_addresses = null;
var group_colors = false;

var slider_color_channels = [
    { "number": 1, "label": "red", "title": "Red", "value": 0, "ranges": null, "type": 1, "color": "rgb(255,0,0)" },
    { "number": 2, "label": "green", "title": "Green", "value": 0, "ranges": null, "type": 2, "color": "rgb(0,255,0)" },
    { "number": 3, "label": "blue", "title": "Blue", "value": 0, "ranges": null, "type": 3, "color": "rgb(0,0,255)" },
    { "number": 5, "label": "white", "title": "White", "value": 0, "ranges": null, "type": 5, "color": "rgb(255,255,255)" }
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
        return fixture_tile_panel.isActive(this.id);
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
function updateFixtures() {
    fixture_tile_panel.empty();
    $("#copy_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");
    $("#clear_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");

    slider_panel.releaseAllChannels();        // TEMP - for now so sliders and selected are not out of sync

    if (group_colors)
        slider_panel.allocateChannels(-1, "Colors", slider_color_channels, fixture_slider_colors_callback);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/full/query/fixtures/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            fixtures.length = 0;
            $.map(json, function (fixture, index) {
                fixtures.push(new Fixture(fixture));

                var tile;
                if (fixture.is_group)
                    tile = fixture_tile_panel.addTile(fixture.id, "G" + fixture.number, escapeForHTML(fixture.full_name), true, "FixtureGroup", "fixture group");
                else
                    tile = fixture_tile_panel.addTile(fixture.id, fixture.number, escapeForHTML(fixture.full_name), true);

                tile.data("fixture_channels", fixture.num_channels).data("fixture_active", false);

                if (fixture.is_active)
                    loadFixtureChannels(fixture.id, null);
            });

            setEditMode(edit_mode);     // Refresh editing icons on new tiles
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function controlFixture(event, fixture_id) {
    stopEventPropagation(event);

    var fixture_active = fixture_tile_panel.isActive(fixture_id);

    if (!fixture_active) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/control/fixture/capture/" + fixture_id,
            cache: false,
            success: function (data) {
                var channel_data = jQuery.parseJSON(data);
                loadFixtureChannels(fixture_id, channel_data);
            },
            error: onAjaxError
        });
    }
    else {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/control/fixture/release/" + fixture_id,
            cache: false,
            success: function () {
                slider_panel.releaseChannels(fixture_id);
                markActiveFixture(fixture_id, false);
            },
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function controlFixtureGroup(event, fixture_group_id) {
    stopEventPropagation(event);

    var fixture_active = fixture_tile_panel.isActive(fixture_group_id);

    if (!fixture_active) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/control/fixturegroup/capture/" + fixture_group_id,
            cache: false,
            success: function (data) {
                var channel_data = jQuery.parseJSON(data);
                loadFixtureChannels(fixture_group_id, channel_data);
            },
            error: onAjaxError
        });
    }
    else {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/control/fixturegroup/release/" + fixture_group_id,
            cache: false,
            success: function () {
                slider_panel.releaseChannels(fixture_group_id);
                markActiveFixture(fixture_group_id, false);
            },
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function loadFixtureChannels( fixture_id, updated_channel_data ) {
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
                    if (fixture.isGroup()) {
                        var fixture_ids = fixture.getFixtureIds();
                        for (var k = 0; k < fixture_ids.length; k++) {
                            json.push({ "fixture_id": fixture_ids[k], "channel": i, "value": slider_color_channels[j].value });
                        }
                    }
                    else {
                        json.push({ "fixture_id": fixture.getId(), "channel": i, "value": slider_color_channels[j].value });
                    }

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

        channels_to_load.push(channel);
    }

    slider_panel.allocateChannels(fixture_id, fixture.getFullName(), channels_to_load,
            fixture.isGroup() ? fixturegroup_slider_callback : fixture_slider_callback);

    // Update colors with current values
    if (json.length > 0) {
        $.ajax({
            type: "POST",
            url: "/dmxstudio/full/control/fixture/channels/",
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
function fixture_slider_callback(fixture_id, channel, value) {
    var json = [{
        "fixture_id": fixture_id,
        "channel": channel,
        "value": value
    }];

    $.ajax({
        type: "POST",
        url: "/dmxstudio/full/control/fixture/channels/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        success: function () {
            updateFixtureChannelValues(json);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function fixturegroup_slider_callback(fixture_group_id, channel, value) {
    var group = getFixtureById(fixture_group_id);
    var json = [];
    var fixture_ids = group.getFixtureIds();

    for (var i = 0; i < fixture_ids.length; i++) {
        json.push({ "fixture_id": fixture_ids[i], "channel": channel, "value": value });
    }

    $.ajax({
        type: "POST",
        url: "/dmxstudio/full/control/fixture/channels/",
        data: JSON.stringify(json),
        contentType: 'application/json',
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
    var active_fixtures = getActiveFixtures();
    for (var i=0; i < active_fixtures.length; i++) {
        var fixture = active_fixtures[i];
        for (var j=0; j < fixture.channels.length; j++) {
            if (fixture.channels[j].type == channel_type) {
                if (fixture.isGroup()) {
                    var fixture_ids = fixture.getFixtureIds();
                    for (var k=0; k < fixture_ids.length; k++) {
                        json.push({ "fixture_id": fixture_ids[k], "channel": j, "value": value });
                    }
                }
                else {
                    json.push({ "fixture_id": fixture.getId(), "channel": j, "value": value });
                }
            }
        }
    }

    if (json.length == 0)
        return;

    $.ajax({
        type: "POST",
        url: "/dmxstudio/full/control/fixture/channels/",
        data: JSON.stringify(json),
        contentType: 'application/json',
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
        var fixture = getFixtureById(channel_info[i].fixture_id);
        if (channel_info[i].channel < fixture.getNumChannels())
            fixture.getChannels()[channel_info[i].channel].value = channel_info[i].value;
    }
}

// ----------------------------------------------------------------------------
//
function groupFixtureSliderColors(event) {
    stopEventPropagation(event);

    group_colors = !group_colors;

    slider_panel.releaseAllChannels();

    if (group_colors) {
        $("#channel_panel_groupcolors").removeClass('ui-icon').addClass('ui-icon-white');

        slider_panel.allocateChannels(-1, "Colors", slider_color_channels, fixture_slider_colors_callback);
    }
    else {
        $("#channel_panel_groupcolors").removeClass('ui-icon-white').addClass('ui-icon');
    }

    // Re-populate all fixtures
    var active_fixtures = getActiveFixtures();
    for (var i = 0; i < active_fixtures.length; i++) {
        loadFixtureChannels(active_fixtures[i].getId(), null);
    }
}

// ----------------------------------------------------------------------------
//
function createFixture(event) {
    stopEventPropagation(event);

    var new_number = getUnusedFixtureNumber();

    openNewFixtureDialog("Create Fixture", {
        id: 0,
        name: "Somewhere",
        description: "",
        number: new_number,
        fuid: 0,
        dmx_address: 0,
        allow_overlap: false
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
        allow_overlap: false
    });
}

// ----------------------------------------------------------------------------
//
function openNewFixtureDialog(dialog_title, fixture_data) {
    if (fixture_definitions == null) {
        // Get fixture definitions only once
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/query/fixture/definitions/",
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

    // Default to something
    if (data.fuid == 0)
        data.fuid = fixture_definitions[0].fixtures[0].personalities[0].fuid;

    // Populate DMX addresses array
    dmx_addresses = [];
    for (var i = 0; i < 512; i++)
        dmx_addresses.push(0);

    for (var i = 0; i < fixtures.length; i++) {
        if (!fixtures[i].isGroup()) {
            var address = fixtures[i].getDMXAddress() - 1;
            for (var d = address; d < address + fixtures[i].getNumChannels() ; d++)
                dmx_addresses[d] = fixtures[i].getNumber();
        }
    }

    // Default to some unused address
    if (data.dmx_address == 0) {
        for (var d = 0; d < 512; d++)
            if (dmx_addresses[d] == 0) {
                data.dmx_address = d + 1;
                break;
            }
    }

    var send_update = function () {
        var json = {
            id: data.id,
            name: $("#nfd_name").val(),
            description: $("#nfd_description").val(),
            number: parseInt($("#nfd_number").val()),
            fuid: $("#nfd_personality").val(),
            dmx_address: parseInt($("#nfd_dmx_address").val())
        };

        if (json.fuid == null) {
            alert("You must select a fixture");
            return;
        }

        // Make sure this is the same fixture number or unused
        if (json.number != data.number && getFixtureByNumber(json.number) != null) {
            alert("Fixture number " + json.number + " is already in use");
            return;
        }

        // Make sure no DMX addresses overlap (unless allowed)
        if (!$("#nfd_allow_overlap").is("checked")) {
            var info = findFixtureDefinition(json.fuid);
            
            for (var i = json.dmx_address - 1; i < json.dmx_address - 1 + info.num_channels; i++) {
                if (dmx_addresses[i] != 0 && dmx_addresses[i] != json.number) {
                    alert("DMX address overlaps existing fixture at address " + (i + 1));
                    return;
                }
            }
        }

        var action = (json.id == 0) ? "create" : "update";

        $.ajax({
            type: "POST",
            url: "/dmxstudio/full/edit/fixture/" + action + "/",
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
    $("#nfd_allow_overlap").attr('checked', data.allow_overlap);

    selectFixtureByFuid(data.fuid,false);

    $("#nfd_manufacturer").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select manufacturer' });
    $("#nfd_model").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select model' });
    $("#nfd_personality").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select personality' });

    if (data.id == 0) {
        $("#nfd_manufacturer").bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            if (ui.checked)
                selectFixtureByFuid(fixture_definitions[parseInt(ui.value)].fixtures[0].personalities[0].fuid, true);
        });

        $("#nfd_model").bind("multiselectclick", function (event, ui) {
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
        var info = findFixtureDefinition( $("#nfd_personality").val() );
        showDmxAddressChooser(parseInt($("#nfd_number").val()), info.num_channels);
    });

    $("#new_fixture_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function showDmxAddressChooser(fixture_number, num_channels) {
    $("#choose_dmx_address_dialog").dialog({
        autoOpen: false,
        width: 710,
        height: 700,
        modal: true,
        resizable: false,
        title: "Select DMX Address"
    });

    var update_tiles = function ( mode ) {
        $("#cda_addresses").empty();

        var html = "";
        var dmx = 1;
        var allow_overlap = $("#nfd_allow_overlap").is(":checked");

        for (var row = 0; row < 32; row++) {
            for (var col = 0; col < 16; col++) {
                var tile_locked = false;
                var clazz = (dmx_addresses[dmx - 1] == 0) ? "dmx_tile_unused" : "dmx_tile_used";

                if (!allow_overlap) {
                    for (var i = 0; i < num_channels; i++) {
                        var index = dmx - 1 + i;
                        if (index >= 512 ||
                             (dmx_addresses[index] != 0 && dmx_addresses[index] != fixture_number) ) { // Does not fit
                            tile_locked = true;
                            break;
                        }
                    }
                }

                if (allow_overlap || !tile_locked)
                    clazz += " dmx_tile_unlocked";
                else
                    clazz += " dmx_tile_locked";

                html += "<div class='dmx_tile " + clazz + "' ";

                if (!tile_locked)
                    html += 'onclick="selectDmxAddress(event,' + dmx + ');"';

                var style = "";
                if (row < 31)
                    style += "border-bottom: 0px; ";
                if (col < 15)
                    style += "border-right: 0px; ";
                if (col == 0)
                    style += "clear:both;";

                var contents = "";
                if (mode == 0)
                    contents = dmx;
                else if (dmx_addresses[dmx - 1] != 0)
                    contents = dmx_addresses[dmx - 1];

                html += "style='" + style + "'>" + contents + "</div>";
                dmx++;
            }
        }

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
            url: "/dmxstudio/full/delete/fixture/" + item.getId(),
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
    $("#describe_fixture_address").html(fixture.getDMXAddress());

    var info = "";
    var channels = fixture.getChannels();

    for (var i = 0; i < channels.length; i++) {
        var type_name = channels[i].title.indexOf(channels[i].type_name) != -1 ? "" : (" (" + escapeForHTML(channels[i].type_name) + ")");

        info += "<div style=\"clear:both; float: left;\" >CH " + (i+1) + " " + channels[i].title + type_name + "</div>";
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
        fixture_ids: default_fixtures
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
        fixture_ids: group.getFixtureIds()
    });
}

// ----------------------------------------------------------------------------
//
function openNewFixtureGroupDialog(dialog_title, data) {

    var send_update = function ( ) {
        var json = {
            id: data.id,
            name: $("#nfgd_name").val(),
            description: $("#nfgd_description").val(),
            number: parseInt($("#nfgd_number").val()),
            fixture_ids: $("#nfgd_fixtures").val()
        };

        if (json.number != data.number && getFixtureGroupByNumber(json.number) != null) {
            alert("Fixture group number " + json.number + " is already in use");
            return;
        }

        if (fixtures.length == 0) {
            alert("No fixtures are selected");
            return;
        }

        var action = (json.id == 0) ? "create" : "update";

        $.ajax({
            type: "POST",
            url: "/dmxstudio/full/edit/fixturegroup/" + action + "/",
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
        width: 540,
        height: 550,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_fixture_group_dialog").dialog("option", "title", dialog_title + " " + data.number);

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

    $("#new_fixture_group_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function deleteFixtureGroup(event, fixture_group_id) {
    stopEventPropagation(event);

    deleteVenueItem(getFixtureById(fixture_group_id), function (item) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/delete/fixturegroup/" + item.getId(),
            cache: false,
            success: updateFixtures,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function clearFixtures(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/full/control/fixture/release/0",
        cache: false,
        success: function () {
            slider_panel.releaseAllChannels();
            fixture_tile_panel.iterate(function (id) {
                markActiveFixture(id, false);
            });
        },
        error: onAjaxError
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
        height: 320,
        modal: true,
        resizable: false,
        buttons: {
            "Copy Fixtures": function () {
                var selected_fixtures = $("#copy_fixture_fixtures").val();
                var selected_scene = $("#copy_fixture_scene").val();
                var clear = $("#copy_fixture_remove").is(':checked');

                if ( selected_fixtures.length == 0 )
                    return;

                var json = { scene_id: selected_scene, clear: clear, fixture_ids: selected_fixtures };

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/full/edit/scene/copy_fixtures/",
                    data: JSON.stringify( json ),
                    contentType: 'application/json',
                    cache: false,
                    async: false,
                    success: function () {
                        updateScenes();

                        if (clear)
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

    $("#describe_fixture_group_info").html(info);
    $("#describe_fixture_groups_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function markActiveFixture(fixture_id, new_state) {
    var active_scroll_item = fixture_tile_panel.getTile(fixture_id);
    var is_active = fixture_tile_panel.isActive(fixture_id);

    if (new_state != is_active) {
        fixture_tile_panel.selectTile(fixture_id, new_state);
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