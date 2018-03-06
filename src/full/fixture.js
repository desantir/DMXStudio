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

var fixtures = [];

var fixture_definition_cache = null;     // Fixture definition cache - populated only if needed
var group_colors = false;
var fixture_tile_lock = false;

var COLOR_CHANNELS_OWNER = -1;
var MASTER_DIMMER_OWNER = -2;

var fixture_state_listener = null;  // Listener for fixture state changes

var last_dmx_universe = 1;
var last_fuid = 0;

var slider_color_channels = [
    { "channel": 1, "type": 1, "label": "red", "name": "Master Red", "value": 0, "max_value":255, "ranges": null, "color": "rgb(255,0,0)", "linkable": true },
    { "channel": 2, "type": 2, "label": "green", "name": "Master Green", "value": 0, "max_value": 255, "ranges": null, "color": "rgb(0,255,0)", "linkable": true },
    { "channel": 3, "type": 3, "label": "blue", "name": "Master Blue", "value": 0, "max_value": 255, "ranges": null, "color": "rgb(0,0,255)", "linkable": true },
    { "channel": 5, "type": 5, "label": "white", "name": "Master White", "value": 0, "max_value": 255, "ranges": null, "color": "rgb(255,255,255)", "linkable": true },
    { "channel": 4, "type": 4, "label": "amber", "name": "Master Amber", "value": 0, "max_value": 255, "ranges": null, "color": "rgb(237,124,36)", "linkable": true }
];

var fixture_tile_panel = null;                                  // Maybe null on some pages

var channel_changer_id = Math.floor(Date.now() / 100000);     // Used to identify our channel changes and avoid "bounce"

// ----------------------------------------------------------------------------
// class Fixture (also Fixture Group)
//
function Fixture(fixture_data)
{
    // Constructor
    jQuery.extend(this, fixture_data);
}

// method getId
Fixture.prototype.getId = function () {
    return this.id;
}

// method getNumber
Fixture.prototype.getNumber = function () {
    return this.number;
}

// method getCreated
Fixture.prototype.getCreated = function () {
    return this.created;
}

// method getName
Fixture.prototype.getName = function () {
    return this.name;
}

// method getFullName
Fixture.prototype.getFullName = function () {
    return this.full_name;
}

// method getDescription
Fixture.prototype.getDescription = function () {
    return this.description;
}

// method getNumChannels
Fixture.prototype.getNumChannels = function () {
    return this.num_channels;
}

// method getChannels
Fixture.prototype.getChannels = function () {
    return this.channels;
}

// method getChannelValues (may be empty)
Fixture.prototype.hasChannelValues = function () {
    return this.has_channel_values;
}

// method getUserTypeName
Fixture.prototype.getUserTypeName = function () {
    return this.is_group ? "Fixture Group" : "Fixture";
}

// method isGroup
Fixture.prototype.isGroup = function () {
    return this.is_group;
}

// method isActive 
Fixture.prototype.isActive = function () {
    return this.is_active;
}

// method setActive 
Fixture.prototype.setActive = function ( active ) {
    this.is_active = active;
}

// method getFixtures
Fixture.prototype.getFixtureIds = function () {
    return this.fixture_ids;
}

// method getFUID
Fixture.prototype.getFUID = function () {
    return this.fuid;
}

// method getManufacturer
Fixture.prototype.getManufacturer = function () {
    return this.manufacturer;
}

// method getModel
Fixture.prototype.getModel = function () {
    return this.model;
}

// method getTypeName
Fixture.prototype.getTypeName = function () {
    return this.type_name;
}

// method getDMXAddress
Fixture.prototype.getDMXAddress = function () {
    return this.dmx_address;
}

// method getUniverse
Fixture.prototype.getUniverseId = function () {
    return this.dmx_universe;
}

// method getLabel
Fixture.prototype.getLabel = function () {
    var label = "";
    if (this.isGroup())
        label = "G";
    return label + this.getNumber() + ": " + this.getFullName();
}

// method getAllowDimming
Fixture.prototype.getAllowDimming = function () {
    return this.allow_dimming;
}

// method getAllowWhiteout
Fixture.prototype.getAllowWhiteout = function () {
    return this.allow_whiteout;
}

// method getAllowWhiteout
Fixture.prototype.getGridX = function () {
    return this.grid_x;
}

    // method getAllowWhiteout
Fixture.prototype.getGridY = function () {
    return this.grid_y;
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
function filterFixtures( filter ) {
    var objects = [];
    for (var i = 0; i < fixtures.length; i++)
        if ( filter( fixtures[i] ) )
            objects[objects.length] = fixtures[i];
    return objects;
}

// ----------------------------------------------------------------------------
//
function getUnusedFixtureGroupNumber() {
    return getNextUnusedNumber( filterFixtures( function( f ) { return f.isGroup(); } ) );
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
    return getNextUnusedNumber( filterFixtures( function( f ) { return !f.isGroup(); } ) );
}

// ----------------------------------------------------------------------------
// 
function getActiveFixtures() {
    return filterFixtures( function( f ) { return f.isActive(); } );
}

// ----------------------------------------------------------------------------
//
function updateCapturedFixtures(captured_fixtures) {
    for (var i = 0; i < fixtures.length; i++) {
        var fixture = fixtures[i];
        var captured = captured_fixtures.indexOf(fixture.id) > -1;
        var active = fixture.isActive();

        if (active && !captured) {
            slider_panel.releaseChannels(fixture.id);
            markActiveFixture(fixture.id, false);
        }
        else if (!active && captured) {
            var channel_data = null;

            var actor = getDefaultSceneActor( fixture.id );
            if ( actor != null ) {
                channel_data = [];
                for ( var a=0; a < fixture.getNumChannels(); a++ )
                    channel_data.push( actor.channels.length > a ? actor.channels[a].value : 0 );
            }

            loadFixtureChannels(fixture.id, channel_data, false);
        }
    }
}

// ----------------------------------------------------------------------------
//
function updateFixtures() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/fixtures/",
        cache: false,
        success: function (data) {
            fixtures = []

            var json = jQuery.parseJSON(data);
            $.map(json, function (fixture, index) {
                for (var i = 0; i < fixture.channels.length; i++)
                    fixture.channels[i].default_value = fixture.channels[i].value;

                fixtures.push( new Fixture(fixture) );
            } );

            createFixtureTiles( true );
        },

        error: onAjaxError
    });
}

var fixtureTileUpdateTimer = null;

// ----------------------------------------------------------------------------
//
function createFixtureTiles( initial_update ) {
    if ( fixture_tile_panel == null )
        return;

    if ( initial_update ) {
        arrangeSliders( );          // Reset sliders, add colors and other static channels

        _createFixtureTiles();
    }
    else
        delayUpdate( "fixtureTiles", 1, _createFixtureTiles );
}

function _createFixtureTiles() {
    fixture_tile_panel.empty();

    $("#copy_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");
    $("#clear_fixtures_button").removeClass("ui-icon-white").addClass("ui-icon");

    for (var i = 0; i < fixtures.length; i++) {
        var fixture = fixtures[i];

        if ( fixture.is_group )
            fixture_tile_panel.addTile(fixture.id, "G" + fixture.number, escapeForHTML(fixture.full_name), true, true, fixture.grid_x, fixture.grid_y, false, "FixtureGroup", "fixture group", "group-item-header");
        else {
            fixture_tile_panel.addTile(fixture.id, fixture.number, escapeForHTML(fixture.full_name), true, true, fixture.grid_x, fixture.grid_y, fixture.is_tracked );

            if ( fixture.is_tracked && fixture.hasOwnProperty( "fixture_state" ) )
                fixture_tile_panel.setTileChipColor( fixture.id, "#" + fixture.fixture_state, fixture.fixture_strobe );
        }

        if ( fixture.is_active ) {
            fixture.is_active = false;      // For the purposes of the current UI state the fixture is not active
            slider_panel.releaseChannels(fixture.id);

            loadFixtureChannels(fixture.id, null, false);
        }
    }

    highlightSceneFixtures(active_scene_id, true);

    setEditMode(edit_mode);         // Refresh editing icons on new tiles
}

// ----------------------------------------------------------------------------
// Called on fixture status event 
function changeFixtureStatusEvent(uid, category, value1, value2 ) {
    var fixture = getFixtureById( uid ); 
    if ( fixture == null )
        return;

    var color = rgbLong2hex( parseInt(value1) );

    fixture.fixture_state = color.substring( 1 );
    fixture.fixture_strobe = value2;

    fixture_tile_panel.setTileChipColor( uid, color, value2 );
}

// ----------------------------------------------------------------------------
// Called on fixture added event 
function newFixtureEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/fixture/" + uid,
        cache: false,
        async: false,
        success: function (data) {
            var fixture = new Fixture( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < fixtures.length; i++) {
                if (fixtures[i].getId() == fixture.getId() )
                    return;
                    
                if ( fixtures[i].getNumber() > fixture.getNumber() ) {
                    fixtures.splice( i, 0, fixture );
                    createFixtureTiles( false );
                    return;
                }
            }

            fixtures[fixtures.length] = fixture;
            createFixtureTiles( false );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on fixture changed event 
function changeFixtureEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/fixture/" + uid,
        cache: false,
        success: function (data) {
            var fixture = new Fixture( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < fixtures.length; i++) {
                if (fixtures[i].getId() == fixture.getId() ) {
                    fixtures[i] = fixture;

                    var number = ( fixture.is_group ) ? "G" + fixture.number : fixture.number;

                    fixture_tile_panel.updateTile( fixture.id, number, escapeForHTML(fixture.full_name), fixture.grid_x, fixture.grid_y );

                    if ( fixture.is_active ) {
                        fixture.is_active = false;      // For the purposes of the current UI state the fixture is not active
                        slider_panel.releaseChannels(fixture.id);
                        loadFixtureChannels(fixture.id, null, false);
                    }
                    break;
                }
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on fixture delete event
function deleteFixtureEvent( uid ) {
    for (var i = 0; i < fixtures.length; i++) {
        if (fixtures[i].getId() == uid) {
            slider_panel.releaseChannels(uid);
            fixtures.splice( i, 1 );
            createFixtureTiles( false );
            break;
        }
    }
}

// ----------------------------------------------------------------------------
// Called on channel change event
function changeChannelEvent( fixture_uid, channel, value, change_id ) {
    var fixture = getFixtureById( fixture_uid ); 
    if ( fixture == null || !fixture.isActive() )
        return;

    if (channel < fixture.getNumChannels() && fixture.getChannels()[channel].value != value) {
        fixture.getChannels()[channel].value = value;

        if (channel_changer_id != change_id) {
            var slider = slider_panel.findChannel(fixture_uid, channel);

            if (slider != null && !slider.isBusy() && !(slider.isSelected() && slider.isLinkable())) {
                slider.setValue(value);
                slider.changed = false;         // Don't send a change event
            }
        }
    }

    // Update default scene if needed
    var default_scene = getSceneById( getDefaultSceneId() );
    if ( default_scene == null )
        return;

    var scene_actor = default_scene.getActor( fixture_uid );
    if ( scene_actor == null )
        return;

    if ( channel < scene_actor.channels.length )
        scene_actor.channels[ channel ].value = value;
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
function highlightAllFixtures( highlight) {
    fixture_tile_panel.highlightAllTiles(highlight);
}

// ----------------------------------------------------------------------------
//
function controlFixtureGroup(event, fixture_group_id) {
    controlFixture( event, fixture_group_id );
}

// ----------------------------------------------------------------------------
//
function controlFixture(event, fixture_id) {
    stopEventPropagation(event);

    var fixture = getFixtureById(fixture_id);
    if (fixture == null)
        return;

    if ( !fixture.isActive() )
        captureFixture(fixture_id, null, null, true);
    else
        releaseFixture(fixture_id);
}

// ----------------------------------------------------------------------------
//
function moveFixtureGroup( fixture_id, x, y ) {
     moveFixture( fixture_id, x, y );
}

// ----------------------------------------------------------------------------
//
function moveFixture( fixture_id, x, y ) {
    var fixture = getFixtureById(fixture_id);
    if (fixture == null)
        return;

    fixture.grid_x = x;
    fixture.grid_y = y;

    var json = {
        "id": fixture_id,
        "grid_x": x,
        "grid_y": y
    };
    
    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/position/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function captureFixture(fixture_id, preload_channel_values, preload_palette_refs, tile_clicked) {
    var json = {
        "id": fixture_id,
        "is_capture": true
    };
    
    if ( preload_channel_values != null ) 
        json.channel_values = preload_channel_values;

    if ( preload_palette_refs != null ) 
        json.palette_refs = preload_palette_refs;

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        success: function (data) {
            // Make active (captured)
            var channel_data = jQuery.parseJSON(data);
            loadFixtureChannels(fixture_id, channel_data, tile_clicked);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function releaseFixture(fixture_id) {
    var json = {
        "id": fixture_id,
        "is_capture": false
    };
    
    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        success: function (data) {
            slider_panel.releaseChannels(fixture_id);
            markActiveFixture(fixture_id, false);
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
                    json.push({ "actor_id": fixture.getId(), "channel": channel.channel, "value": slider_color_channels[j].value, "capture": false });
                    skip = true;
                    break;
                }
            }
            
            if ( skip )
                continue;
        }

        channel.label = ((fixture.isGroup()) ? "G" : "") + fixture.getNumber() + "-" + (i + 1);
        channel.linkable = true;

        if ( updated_channel_data != null )
            channel.value = i < updated_channel_data.length ? updated_channel_data[i] : 0;

        channel.max_value = 255;

        channels_to_load.push(channel);
    }

    slider_panel.allocateChannels(fixture_id, fixture.getFullName(), channels_to_load, fixture_slider_callback );

    if (tile_clicked)
        slider_panel.highlightChannels(fixture_id, true);

    // Update colors with current values
    if (json.length > 0) {
        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/control/fixture/channel/",
            data: JSON.stringify( { "channel_data" : json } ),
            contentType: 'application/json',
            cache: false,
            success: function () {
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

var channel_updates = [];
var channel_timer = null;

function fixture_slider_callback(fixture_id, action, channel, value, header_event) {
    if ( action == "channel" ) {
        channel_updates.push( {
            "actor_id": fixture_id,
            "channel": channel,
            "value": value,
            "capture": true
        });

        if ( channel_timer == null ) {
            // Batch the channel updates in case we are setting bulk colors  
            channel_timer = setTimeout( function() {
                var json = {
                    change_id: channel_changer_id,
                    channel_data: channel_updates
                };

                channel_updates = [];
                channel_timer = null;

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/control/fixture/channel/",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    async: false,   // These cannot get out of sync
                    cache: false,
                    success: function () {
                        updateFixtureChannelValues(json);
                    },
                    error: onAjaxError
                });
             }, 1 );
        }
    }
    else if ( action == "header" ) {
        var fixture = getFixtureById( fixture_id );

        var itemList = {
                "title": { type: "html", html: "<span>" + escapeForHTML( fixture.getFullName() ) + "</span>" },
                "sep1": "---------",
                "palette": { name: "Palettes", callback: function( key, opt ) { openFixturePaletteDialog( fixture_id, false ); } }
        };

        if ( !fixture.isGroup() ) {
            itemList["preset"] = { name: "Preset", callback: function( key, opt ) { openFixturePaletteDialog( fixture_id, true ); } };
            itemList["save"] = { name: "Save Preset", callback: function( key, opt ) { saveFixturePresetDialog( fixture_id ); } };

            if ( fixture.getChannels()[channel].ranges.length > 0 )
                itemList["ranges"] = { name: "Ranges", callback: function (key, opt) { showFixtureRanges(fixture_id, channel, "setChannelValue" ); } };
        }

        $.contextMenu( 'destroy' );

        $.contextMenu({
            zIndex: 2000,
            selector: '.context-fixture-header', 
            trigger: 'none',
            animation: {duration: 250, show: 'fadeIn', hide: 'fadeOut'},
            items: itemList
        });

        $('.context-fixture-header').contextMenu( { x: header_event.pageX, y: header_event.pageY } );
    }
    else if ( action == "side-text" ) {
        var fixture = getFixtureById( fixture_id );

        showFixtureRanges( fixture_id, channel, "setChannelValue" );
    }
}

// ----------------------------------------------------------------------------
//
function showFixtureRanges( fixture_id, channel_id, callback_name ) {
    var fixture = getFixtureById( fixture_id );
    if ( fixture == null )
        return;

    var channel = fixture.getChannels()[channel_id];
    var toastId = "F" + fixture.getNumber() + "C" + (channel_id+1) + "_toast_helper";

    if ( $("#" + toastId).length > 0 )      // Channel help already is displayed
        return;

    var type_name = channel.name.indexOf(channel.type_name) != -1 ? "" : (" (" + escapeForHTML(channel.type_name) + ")");
    var message = fixture.getNumber() + "-" +  (channel_id+1) + ": " + fixture.getFullName() + "<br/>";
    
    message += channel.name + type_name + "<br/></br/>";
    message += '<div id="' + toastId + '" style="max-height: 300px; overflow-y: auto;">';

    for (var j = channel.ranges.length; j-- > 0; ) {
        message += '<div class="set_channel" onclick="' + callback_name + '(event,' + fixture_id + ',' + channel_id + ',' + channel.ranges[j].start + ');">';
        message += formatChannelRange( channel.ranges[j] );
        message += '</div>';
    }

    message += '</div>';

    return  $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 20 * 1000,
        sticky   : channel.ranges.length,
        position : 'top-right',
        type     : '',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function setChannelValue( event, fixture_id, channel_id, value ) {
    stopEventPropagation( event );

    var slider = slider_panel.findChannel( fixture_id, channel_id );
    if ( slider != null )
        slider.setValue( value );
}

// ----------------------------------------------------------------------------
//
function formatChannelRange( range ) {
    var start = "000" + range.start;
    var end = "000" + range.end;

    return start.slice(-3) + " - " + end.slice(-3) + " : " + escapeForHTML(range.name);
}

// ----------------------------------------------------------------------------
//
function fixture_slider_colors_callback(unused, action, channel_type, value) {
    if ( action != "channel" )
        return;

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
                json.push({ "actor_id": fixture.getId(), "channel": fixture.channels[j].channel, "value": value, "capture": false });
        }
    });

    if (json.length == 0)
        return;

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/fixture/channel/",
        data: JSON.stringify( { "channel_data" : json }),
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
function groupColorSlidersHandler( slider_panel, control ) {
    group_colors = !group_colors;

    arrangeSliders( );

    if (group_colors)
        control.removeClass('ui-icon').addClass('ui-icon-white');
    else
        control.removeClass('ui-icon-white').addClass('ui-icon');
}

// ----------------------------------------------------------------------------
//
function arrangeSlidersHandler( slider_panel, control ) {
    arrangeSliders( );
}

// ----------------------------------------------------------------------------
//
function resetLinkedSlidersHandler( slider_panel, control ) {
    slider_panel.resetLinks();
}

// ----------------------------------------------------------------------------
//
function arrangeSliders( ) {
    slider_panel.releaseAllChannels();

    // Add venue master dimmer
    slider_panel.allocateChannels(MASTER_DIMMER_OWNER, "Venue Dimmer", [master_dimmer_channel], master_dimmer_callback );
    master_dimmer_channel.slider = slider_panel.findChannel(MASTER_DIMMER_OWNER, master_dimmer_channel.channel);
    master_dimmer_channel.slider.setValue(master_dimmer_channel.value);
    master_dimmer_channel.slider.changed = false

    // Add color masters if enabled
    if (group_colors)
        slider_panel.allocateChannels(COLOR_CHANNELS_OWNER, "Colors", slider_color_channels, fixture_slider_colors_callback );

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
        if (dmx_address_map[d] == null) {
            default_dmx_address = d + 1;
            break;
        }

    openNewFixtureDialog("Create Fixture", {
        id: 0,
        name: "Somewhere",
        description: "",
        number: default_number,
        fuid: last_fuid,
        allow_overlap: false,
        dmx_universe: last_dmx_universe,
        dmx_address: default_dmx_address,
        allow_dimming: true,
        allow_whiteout: true,
        grid_x: 0,
        grid_y: 0
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
        dmx_universe: fixture.getUniverseId(),
        allow_dimming: fixture.getAllowDimming(),
        allow_whiteout: fixture.getAllowWhiteout(),
        grid_x: fixture.getGridX(),
        grid_y: fixture.getGridY()
    });
}

// ----------------------------------------------------------------------------
//
function openNewFixtureDialog(dialog_title, fixture_data) {
    if (fixture_definition_cache == null) {
        // Get fixture definitions only once
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/fixture/definitions/",
            cache: false,
            success: function ( data ) {
                fixture_definition_cache = JSON.parse(data);
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
    if (fixture_definition_cache == null)
        return;

    // Default to some fixture
    if (data.fuid == 0)
        data.fuid = fixture_definition_cache[0].models[0].personalities[0].fuid;

    var send_update = function () {
        var json = {
            id: data.id,
            name: $("#nfd_name").val(),
            description: $("#nfd_description").val(),
            number: parseInt($("#nfd_number").val()),
            fuid: $("#nfd_personality").val(),
            dmx_address: parseInt($("#nfd_dmx_address").val()),
            dmx_universe: parseInt($("#nfd_dmx_universe").val()),
            allow_dimming: $("#nfd_allow_dimming").is(":checked"),
            allow_whiteout: $("#nfd_allow_whiteout").is(":checked"),
            grid_x: parseInt($("#nfd_grid_x").val()),
            grid_y: parseInt($("#nfd_grid_y").val())
        };

        last_dmx_universe = json.dmx_universe;
        last_fuid = json.fuid;

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
                if ( dmx_address_map[i] != null && dmx_address_map[i].getId() != json.id ) {
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
            },
            error: onAjaxError
        });

        $("#new_fixture_dialog").dialog("close");
    };

    var dialog_buttons = [];

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
        height: 680,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_fixture_dialog").dialog("option", "title", dialog_title + " " + data.number);

    $("#nfd_number").spinner({ min: 1, max: 100000 }).val(data.number);
    $("#nfd_grid_x").spinner({ min: 0, max: 10000 }).val(data.grid_x);
    $("#nfd_grid_y").spinner({ min: 0, max: 10000 }).val(data.grid_y);
    $("#nfd_name").val(data.name);
    $("#nfd_description").val(data.description);
    $("#nfd_dmx_address").spinner({ min: 1, max: 512 }).val(data.dmx_address);
    $("#nfd_dmx_universe").spinner({ min: 1, max: DMX_MAX_UNIVERSES }).val(data.dmx_universe);
    $("#nfd_allow_overlap").attr('checked', data.allow_overlap);
    $("#nfd_allow_dimming").attr('checked', data.allow_dimming);
    $("#nfd_allow_whiteout").attr('checked', data.allow_whiteout);

    selectFixtureByFuid(data.fuid,false);

    function enable_features( fuid ) {
        var info = findFixtureDefinition(fuid);
        if ( info == null )
            return;

        if ( info.has_dimmer )
            $("#nfd_allow_dimming_div").show();
        else
            $("#nfd_allow_dimming_div").hide();

        if ( info.can_whiteout )
            $("#nfd_allow_whiteout_div").show();
        else
            $("#nfd_allow_whiteout_div").hide();
    }

    enable_features( data.fuid );

    $("#nfd_manufacturer").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select manufacturer' });
    $("#nfd_model").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select model' });
    $("#nfd_personality").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, noneSelectedText: 'Select personality' });

    if (data.id == 0) {
        $("#nfd_manufacturer").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            if (ui.checked)
                selectFixtureByFuid(fixture_definition_cache[parseInt(ui.value)].models[0].personalities[0].fuid, true);
        });

        $("#nfd_model").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            if (ui.checked) {
                selectFixtureByFuid(fixture_definition_cache[$("#nfd_manufacturer").val()].models[parseInt(ui.value)].personalities[0].fuid, true);
            }
        });

        $("#nfd_personality").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            enable_features( ui.value );
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
            data.id,
            info.num_channels,
            $("#nfd_allow_overlap").is(":checked") );
    });

    $("#new_fixture_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function buildDmxAddressMap(universe_id) {
    // Populate DMX addresses array
    dmx_address_map = [];

    for (var i = 0; i < 512; i++)
        dmx_address_map.push( null );

    for (var i = 0; i < fixtures.length; i++) {
        if (!fixtures[i].isGroup() && fixtures[i].getUniverseId() == universe_id) {
            var address = fixtures[i].getDMXAddress() - 1;
            for (var d = address; d < address + fixtures[i].getNumChannels() ; d++)
                dmx_address_map[d] = fixtures[i];
        }
    }

    return dmx_address_map;
}

// ----------------------------------------------------------------------------
//
function showDmxAddressChooser(universe_id, fixture_id, num_channels, allow_overlap) {
    $("#choose_dmx_address_dialog").dialog({
        autoOpen: false,
        width: 710,
        height: 700,
        modal: true,
        resizable: false,
        title: "Select Address in DMX Universe " + (universe_id)
    });

    var dmx_address_map = buildDmxAddressMap(universe_id);

    var addresses_div = $("#cda_addresses");

    var update_tiles = function ( mode ) {
        addresses_div.empty();

        var html = "<div onclick='selectDmxAddress(event);'>";
        var dmx = 1;

        for (var row = 0; row < 32; row++) {
            for (var col = 0; col < 16; col++) {
                var tile_locked = false;
                var fixture = dmx_address_map[dmx - 1];
                var clazz = (fixture == null) ? "dmx_tile_unused" : "dmx_tile_used";

                if (!allow_overlap) {
                    for (var i = 0; i < num_channels; i++) {
                        var index = dmx - 1 + i;
                        if (index >= 512 ||
                             (dmx_address_map[index] != null && dmx_address_map[index].getId() != fixture_id) ) { // Does not fit
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

                var contents = mode == 0 ? dmx : "";

                if ( fixture != null ) {
                    html += "title='" + escapeForHTML( fixture.getFullName() ) + "' ";
                    if (mode == 1)
                        contents = fixture.getNumber();
                }

                html += "style='" + style + "'>" + contents + "</div>";
                dmx++;
            }
        }

        html += "</div>";

        addresses_div.html(html);
    }

    $("#cda_show_buttons").buttonset().unbind( "change" );;
    $("#cda_show_buttons").change(function () {
        stopEventPropagation(event);
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
    if ( !fixture_definition_cache ) 
        return null;

    for (var m = 0; m < fixture_definition_cache.length; m++) {
        for (var f = 0; f < fixture_definition_cache[m].models.length; f++) {
            for (var p = 0; p < fixture_definition_cache[m].models[f].personalities.length; p++) {
                if (fuid == fixture_definition_cache[m].models[f].personalities[p].fuid) {
                    var personality = fixture_definition_cache[m].models[f].personalities[p];

                    return { 
                        manufacturer: m,
                        fixture: f,
                        personality: p,
                        "fuid": fuid,
                        num_channels: personality.num_channels,
                        has_dimmer: personality.has_dimmer,
                        can_whiteout: personality.can_whiteout
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

    for (var i=0; i < fixture_definition_cache.length; i++) {
        $("#nfd_manufacturer").append($('<option>', {
            value:i,
            text: fixture_definition_cache[i].manufacturer,
            selected: i == info.manufacturer
        }));
    }

    var models = fixture_definition_cache[info.manufacturer].models;
    for (var i = 0; i < models.length; i++) {
        $("#nfd_model").append($('<option>', {
            value: i,
            text: models[i].model,
            selected: i == info.fixture
        }));
    }

    var personalities = models[info.fixture].personalities;
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
                info += formatChannelRange( channels[i].ranges[j] ) + "<br/>"
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
    var default_fixtures = [];

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
        channel_data: null,
        grid_x: 0,
        grid_y: 0
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
        channel_data: $.extend( true, [], group.getChannels() ),
        grid_x: group.getGridX(),
        grid_y: group.getGridY()
    });
}

// ----------------------------------------------------------------------------
//
function openNewFixtureGroupDialog(dialog_title, data) {
    var sliders = [];

    var send_update = function () {
        var channel_data = [];
        for (var i = 0; i < sliders.length; i++)
            channel_data[i] = sliders[i].getValue();
        $("#nfgd_sliders").empty();

        var ids = $("#nfgd_fixtures").val();

        var json = {
            id: data.id,
            name: $("#nfgd_name").val(),
            description: $("#nfgd_description").val(),
            number: parseInt($("#nfgd_number").val()),
            fixture_ids: ids == null ? [] : ids,
            channel_values: channel_data,
            grid_x: parseInt($("#nfgd_grid_x").val()),
            grid_y: parseInt($("#nfgd_grid_y").val())
        }

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

    var dialog_buttons = [];

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
        height: 610,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_fixture_group_dialog").dialog("option", "title", dialog_title + " " + data.number);

    $("#nfgd_accordion").accordion({ heightStyle: "fill" });

    $("#nfgd_number").spinner({ min: 1, max: 100000 }).val(data.number);
    $("#nfgd_grid_x").spinner({ min: 0, max: 10000 }).val(data.grid_x);
    $("#nfgd_grid_y").spinner({ min: 0, max: 10000 }).val(data.grid_y);
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
        height: 380,
        modal: true,
        resizable: false,
        buttons: {
            "Copy Fixtures": function () {
                var selected_fixtures = $("#copy_fixture_fixtures").val();
                if ( selected_fixtures.length == 0 )
                    return;
                
                var selected_scenes = $("#copy_fixture_scene").val();
                if ( selected_scenes.length == 0 )
                    return;

                var json = {
                    scene_ids: selected_scenes,
                    clear: $("#copy_fixture_remove").is(':checked'),
                    fixture_ids: selected_fixtures,
                    keep_groups: $("#copy_fixture_keep_groups").is(":checked"),
                    keep_animations: $("#copy_fixture_keep_animations").is(":checked")
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
        multiple: true,
        noneSelectedText: "Select Target Scene(s)",
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

    var channel_info_html = "";
    if ( group.hasChannelValues() ) {
        channel_info_html = makeChannelInfoLine(group.getChannels(), null, "describe_fixture_group_channels", true);
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
function PanelProxy() {
    this.isBusy = function () { return false; }
    this.isTrackSlider = function () { return false; }
    this.fixture_slider_callback = function (fixture_id, channel, value) {}
    this.setSliderTitle = function (title) { }
}

// ----------------------------------------------------------------------------
//
function openFixturePaletteDialog( fixture_id, fixture_presets ) {
    var scene_actor = getDefaultSceneActor( fixture_id );
    if ( scene_actor == null )
        return;

    var fixture = getFixtureById( scene_actor.id );
    if ( fixture == null )
        return;

    var palette_dialog = $("#edit_fixture_palette_dialog");

    palette_dialog.dialog({
        title: fixture.getLabel() + ((fixture_presets) ? " Preset" : " Palettes"),
        autoOpen: false,
        width: 400,
        height: 600,
        modal: true,
        resizable: false,
        buttons: {
            "OK": function() {
                var json = [{
                    "actor_id": fixture_id,
                    "palette_refs": $("#efp_palette_pane").data( "select-list" ).getSelectedIds(),
                    "capture": false
                }];

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/control/fixture/palettes/",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    async: false,   // These cannot get out of sync
                    cache: false,
                    error: onAjaxError
                });

                $(this).dialog("close");
             },

             "Cancel": function() {
                $(this).dialog("close");
             }
        }
    });

    var palettes = [];
    for ( var i=0; i < scene_actor.palette_refs.length; i++ ) {
        var p = getPaletteById( scene_actor.palette_refs[i] );
        if ( p != null )
            palettes.push( { id: p.getId(), label: p.getName() })
    }

    // Populate PALETTES
    var select_list = new SelectList( { 
        name: fixture_presets ? "preset" : "palette",
        container: palette_dialog.find("#efp_palette_pane"),
        selected: palettes,
        render_template: $("#esa_palette_template")[0].innerHTML,
        width: 350,
        height: 300,
        list_height: 340,
        render_callback: null,
        select_callback: null,
        remove_callback: null,
        update_callback: null,

        add_callback: function ( select_control, item_id ) {
            return { id: item_id, label: getPaletteById( item_id ).getName() };
        },

        source_callback: function ( select_control ) {
            var palettes = select_control.getSelected();

            var palette_list = filterPalettes( function( palette ) { 
                // If already selected, don't add to the select list
                for (var palette_num=0; palette_num < palettes.length; palette_num++)
                    if ( palettes[palette_num].id == palette.getId() )
                        return false;

                // Return them all
                if ( !fixture_presets )
                    return true; 

                // Only return those specific to the fixture or fixture type

                var definition_palettes = palette.getDefinitionPalettes();
                for ( var i=0; i < definition_palettes.length; i++ )
                    if ( definition_palettes[i].target_uid == fixture.getFUID() )
                        return true;

                var fixture_palettes = palette.getFixturePalettes();
                for ( var i=0; i < fixture_palettes.length; i++ )
                    if ( fixture_palettes[i].target_uid == fixture.getId() )
                        return true;

                return false;
            } );

            var items = [];
            for (var i=0; i < palette_list.length; i++ )
                items.push( { id: palette_list[i].getId(), label: palette_list[i].getName() } );
            return items;
        }
    } );

    palette_dialog.dialog("open");
}

// ----------------------------------------------------------------------------
//
function saveFixturePresetDialog( fixture_id ) {
    var fixture = getFixtureById( fixture_id );
    var save_preset_dialog = $("#save_fixture_preset_dialog");

    $( "#sfpd_name" ).val( "" );
    $( "#sfpd_description" ).val( "" );
    $( "#sfpd_number" ).spinner({ min: 1, max: 100000 }).val ( getUnusedPaletteNumber() );
    $( "#sfpd_channels_div").buttonset();

    save_preset_dialog.dialog({
        title: "Save Preset: " + fixture.getFullName(),
        autoOpen: false,
        width: 600,
        height: 380,
        modal: true,
        resizable: false,
        buttons: {
            "Save": function() {
                var channel_select = parseInt( $("input[name=sfpd_channels]:checked").val() );
                var desired_number = parseInt( $( "#sfpd_number" ).val() );

               if ( getPaletteByNumber( desired_number ) != null ) {
                    messageBox("Palette number " + desired_number + " is already in use");
                    return;
                }

                var channel_data = [];

                for ( var channel=0; channel < fixture.getNumChannels(); ++channel ) {
                    var slider = slider_panel.findChannel( fixture_id, channel );
                    if ( slider == null )
                        continue;

                    if ( channel_select != 2 || slider.isSelected() )
                        channel_data.push( { id: channel, value: slider.getValue() } );
                }

                if ( channel_data.length == 0 ) {
                    messageBox("No channels are selected");
                    return;
                }

                var json = {
                    "id": 0,
                    "name": $( "#sfpd_name" ).val(),
                    "number": desired_number,
                    "description": $( "#sfpd_description" ).val(),
                    "type": PaletteType.PT_FIXTURE_PRESET,
                    "default_palette": { 
                        "target_uid": 0,
                        "addressing": 2,
                        "channel_values": []
                    },
                    "fixture_palettes": [],
                    "definition_palettes": [ {
                        "target_uid": fixture.getFUID(),
                        "addressing": 1,    // By channel
                        "channel_values": channel_data
                    } ]
                };

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/edit/palette/create/",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    cache: false,
                    success: function () {
                        save_preset_dialog.dialog("close");
                    },
                    error: onAjaxError
                });
             },

             "Cancel": function() {
                $(this).dialog("close");
             }
        }
    });
    
    save_preset_dialog.dialog( "open" );
}

// ----------------------------------------------------------------------------
//
function getFixtureTileLock() {
    return fixture_tile_lock;
}

// ----------------------------------------------------------------------------
//
function setFixtureTileLock( locked ) {
}

// ----------------------------------------------------------------------------
//
function getActiveFixturesInOrder() {

    var fixture_ids = []

    slider_panel.iterateChannels( function ( slider ) {
        var id = slider.getOwner();

        if ( id > 0 && (fixture_ids.length == 0 || fixture_ids[fixture_ids.length-1] != id ))
            fixture_ids.push( id );
    });

    return fixture_ids;
}