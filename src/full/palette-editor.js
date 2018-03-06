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
 
along with DMX Studio; see the file _COPYING.txt.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.
*/

var slider_panel = null;

// ----------------------------------------------------------------------------
//
function initializePaletteUI() {
    // Add features to slider panel
    slider_panel_features.push( { class_name: "colorpicker_icon", attach: attachPanelColorPicker, title: "show color palette" } );
    slider_panel_features.push( { class_name: "pantilt_icon", handler: panelPanTiltHandler, title: "show pan tilt" } );

    window.addEventListener( "message", function( event ) {
        if ( event.data.method === "eventBroadcast" ) {
            handleEvent( event.data.event );
        }
    });

    palette_update_callback = populate_palettes;

    $("#new_palette").button();
    $("#palette_save").button();
    $("#palette_cancel").button();
    $("#channel_sliders_cancel").button();
    $("#channel_sliders_save").button();

    $("#palette_number").spinner({ min: 1, max: 100000 });

    $("#palette_type").multiselect({ height: 240, minWidth: 200, multiple: false, selectedList: 1, header: false, classes: "palette_font" });

    $("#palette_select_section").show();

    $("#new_palette").click( function () {
        new_palette();
    });

    $("#palette_save").click( function () {
        save_palette();
    });

    $("#palette_cancel").click( startup_panels );

    $("#palette_select_section").show( );

    $("#palette_generate_from_image").button().click( generatePaletteFromImage );

    updateFixtureDefinitions();
    updatePalettes();
    updateFixtures();
}

// ----------------------------------------------------------------------------
//
function startup_panels() {
    $("#palette_select_section").show("slow" );
    $("#palette_edit_section").hide("slow");
    $("#fixture_select_section").hide("slow" );
    $("#channel_sliders_section").hide("slow" );
    $("#global_channels_section").hide("slow" );
    $("#palette_colors_section").hide();
    $("#select_button_section").hide();

    current_palette = null;
}

// ----------------------------------------------------------------------------
//
function populate_palettes() {
    // Make sure we stay registered with our opener
    if (  window.opener != null && ! window.opener.closed )
        window.opener.postMessage( { "method": "registerEventListener" }, "*" );

    var container = $( "#palette_container" );
    var template = $( "#palette_list_template" )[0].innerHTML;

    container.empty();

    for (var index=0; index < palettes.length; index++) {
        var item_element = $( template.replace(/NNN/g, index) );
        var palette = palettes[index];

        container.append( item_element );

        var remove_button = item_element.find( ".palette_remove" );
        if ( remove_button != null && remove_button.length > 0 ) {
            remove_button.data( "palette_id", palette.getId() );            
            remove_button.on( 'click', function ( event ) { 
                stopEventPropagation(event); 
                remove_palette( $(this).data("palette_id") ); 
            } );
        }

        var select_button = item_element.find( ".palette_select_div" );
        if ( select_button != null && select_button.length > 0) {
            select_button.data( "palette_id", palette.getId() );   
            select_button.on( 'click', function ( event ) { 
                stopEventPropagation(event); 
                
                var palette = getPaletteById( $(this).data("palette_id") );
                
                if ( palette != null )
                    edit_palette( $.extend( {}, palette ) );
            } );
        }

        var palette_label = item_element.find( ".palette_label" );
        if ( palette_label != null )
            palette_label.html( palette.getNumber() + ": " + palette.getName() );
    }
}

// ----------------------------------------------------------------------------
//
function remove_palette( palette_id ) {
    var palette = getPaletteById( palette_id );
    if ( palette == null )
        return;

    questionBox( "Remove Palette", "Are you sure you want to remove palette '" + palette.getName() + "'?", function ( remove ) {
        if ( remove ) {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/delete/palette/" + palette_id,
                cache: false,
                async: false,
                error: onAjaxError
            });        
        }
    } );
}

var current_palette = null;

// ----------------------------------------------------------------------------
//
function new_palette() {
    var palette = {
        id: 0,
        name: "New Palette",
        number: getUnusedPaletteNumber(),
        description: "",
        type: 0,
        default_palette: { 
            channel_values: []
        },
        fixture_palettes: [],
        definition_palettes: []
    };

    edit_palette( new Palette(palette) );
}

// ----------------------------------------------------------------------------
//
function showConditionalSections( type ) {
    if ( type != PaletteType.PT_FIXTURE_PRESET && type != PaletteType.PT_COLOR_PALETTE )
        $("#global_channels_section").show();
    else
        $("#global_channels_section").hide();

    if ( type == PaletteType.PT_COLOR_PALETTE ) {
        $("#palette_colors_section").show();
        $("#fixture_select_section").hide();
    }
    else {
        $("#palette_colors_section").hide();
        $("#fixture_select_section").show();
    }

    if (type == PaletteType.PT_COLOR)
        $("#global_color_button").show();
    else
        $("#global_color_button").hide();
}

// ----------------------------------------------------------------------------
//
function edit_palette( palette ) {
    if ( palette == null )
        return;

    current_palette = palette;

    $( "#palette_name" ).val( current_palette.getName() );
    $( "#palette_description" ).val( current_palette.getDescription() );
    $( "#palette_number" ).spinner({ min: 1, max: 100000 }).val ( current_palette.getNumber() );

    multiselectSelect( $( "#palette_type" ), current_palette.getType() );

    $("#palette_select_section").hide( "slow" );
    $("#palette_edit_section").show();
    $("#select_button_section").show( );

    showConditionalSections( current_palette.getType() );

    $( "#palette_type" ).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);

        var type = parseInt(ui.value);

        defaultGlobalChannels( type );
        showConditionalSections(type);
    });

    var global_color_button = $("#global_color_button");

    global_color_button.ColorPicker({
        livePreview: false,
        autoClose: true,
        showPalettes: false,

        onShow: function (picker) {
            $(picker).ColorPickerSetColorChips(getColorChips());
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            var select_list = $("#global_container").data("select-list");

            var global_channels = select_list.getSelected();

            for (var i = 0; i < global_channels.length; i++) {
                if (global_channels[i].id == ChannelType.CHNLT_RED)
                    global_channels[i].value = rgb.r;
                else if (global_channels[i].id == ChannelType.CHNLT_GREEN)
                    global_channels[i].value = rgb.g;
                else if (global_channels[i].id == ChannelType.CHNLT_BLUE)
                    global_channels[i].value = rgb.b;
            }

            select_list.setSelected(global_channels);
            select_list.populate();
        },

        onChipSet: function (chip, hsb, hex, rgb) {
            return setColorChip(chip, hex);
        }
    });

    global_color_button.button().unbind("click").bind("click", function (event) {
        var global_channels = $("#global_container").data("select-list").getSelected();
        var red = 0, green = 0, blue = 0;

        for (var i = 0; i < global_channels.length; i++) {
            if ( global_channels[i].id == ChannelType.CHNLT_RED )
                red = parseInt(global_channels[i].value);
            else if (global_channels[i].id == ChannelType.CHNLT_GREEN )
                green = parseInt(global_channels[i].value);
            else if (global_channels[i].id == ChannelType.CHNLT_BLUE)
                blue = parseInt(global_channels[i].value);
        }

        global_color_button.ColorPickerSetColor( makeHexRGB(red, green, blue) );
        global_color_button.ColorPickerShow(event);
    });

    // POPULATE GLOBAL CHANNELS
    var globalPalette = current_palette.getGlobalPalette();
    var selected_channels = [];

    for ( var i=0; i < globalPalette.channel_values.length; i++ ) {
        var channel = globalPalette.channel_values[i];
        channel.label = channelTypeNames[channel.id];
        selected_channels.push( channel );
    }

    new SelectList( { 
        name: "channel",
        width: 292,
        container: $("#global_container"),
        selected: selected_channels,
        render_template: $("#global_channel_template")[0].innerHTML,

        render_callback: function ( select_control, index, item, element ) {
            var input = element.find( ".channel_value" );
            if ( input != null ) {
                input.spinner({ min: 0, max: 255 }).val ( item.value );                
                input.on( "spinchange", function ( event ) {
                    item.value = input.val();
                } );
            }
        },
        
        update_callback: null,

        source_callback: function ( select_control ) {
            var selected_ids = select_control.getSelectedIds();

            var items = [];
            for (var i=1; i < channelTypeNames.length; i++ ) {
                if ( selected_ids.indexOf(i) != -1 )
                    continue;
                    
                items.push( { id: i, label: channelTypeNames[i] } );
            }

            return items;
        },

        add_callback: function ( select_control, item_id ) {
            return { id: item_id, label: channelTypeNames[item_id], value: 0 };
        },
        
        select_callback: function ( select_control, index, item ) {
        },

        remove_callback: null
    } );

    // POPULATE PALETTE COLORS
    var color_picker_host = $("#palette_colors_container");
    var palette_color_progression = $("#palette_color_progression" );

    // Attach a single color picker that will be used for all chips (else performance issue with many chips)
    color_picker_host.ColorPicker({
        livePreview: true,
        autoClose: true,
        showPalettes: false,

        onShow: function (picker) {
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb, owner, chip) {
            chip.css("background-color", "#" + hex);
            chip.attr('value', hex);
            chip.html( "#" + hex.toUpperCase() );
        },
    });

    color_picker_host.ColorPickerSetColorChips( getColorChips() );

    palette_color_progression.expandableContainer({
        vertical: true,
        item_template: '<div class="palette_color_chip container_input" ></div>',
        
        new_item_callback: function (item, value) {
            if (value == null)
                value = '000000';

            var chip = item.find(".palette_color_chip");
            chip.attr('value', value);
            chip.css('background-color', "#" + value);
            chip.html( "#" + value );

            chip.click( function(event) {
                color_picker_host.ColorPickerSetAnchor(chip);
                color_picker_host.ColorPickerSetColor( "#" + chip.attr('value') );
                color_picker_host.ColorPickerShow( event );
            } );
        }
    });

    // Use style not css here as jQuery 1.9.2 does not recognize column-count as a pure # and adds 'px'
    palette_color_progression[0].style.columnCount = current_palette.palette_colors.length <= 24 ? 1 : 6;
        
    palette_color_progression.expandableContainer("set_values", current_palette.palette_colors );

    // Populate fixtures
    populate_fixtures();

    // Populate Fixure Definitions
    populate_fixture_definitions();    
}

// ----------------------------------------------------------------------------
//
function defaultGlobalChannels( palette_type ) {

    var global_select_list = $("#global_container").data( "select-list" );
    var global_channels = global_select_list.getSelected();

    for ( var i=0; i < global_channels.length; i++ )
        if ( global_channels[i].value != 0 )
            return;

    var new_channels = [];

    var make_channel = function( type ) {
        return { id: type, value: 0, label: channelTypeNames[type] }
    }

    switch ( palette_type ) {
        case PaletteType.PT_LOCATION:
            new_channels.push( make_channel( ChannelType.CHNLT_PAN ) );
            new_channels.push( make_channel( ChannelType.CHNLT_PAN_FINE ) );
            new_channels.push( make_channel( ChannelType.CHNLT_TILT ) );
            new_channels.push( make_channel( ChannelType.CHNLT_TILT_FINE ) );
            break;

        case PaletteType.PT_DIMMER:
            new_channels.push( make_channel( ChannelType.CHNLT_DIMMER ) );
            break;

        case PaletteType.PT_STROBE:
            new_channels.push( make_channel( ChannelType.CHNLT_STROBE ) );
            break;

        case PaletteType.PT_COLOR:
        case PaletteType.PT_PALETTE_COLOR:                    
            new_channels.push( make_channel( ChannelType.CHNLT_RED ) );
            new_channels.push( make_channel( ChannelType.CHNLT_GREEN ) );
            new_channels.push( make_channel( ChannelType.CHNLT_BLUE ) );
            new_channels.push( make_channel( ChannelType.CHNLT_WHITE ) );
            new_channels.push( make_channel( ChannelType.CHNLT_AMBER ) );
            break;

        case PaletteType.PT_GOBO:
            new_channels.push( make_channel( ChannelType.CHNLT_GOBO ) );
            break;

        case PaletteType.PT_FIXTURE_PRESET:
            break;
    }
    
    global_select_list.setSelected( new_channels );
}

// ----------------------------------------------------------------------------
//
function save_palette( ) {
    var getPalettes = function ( select_list_container ) {
        var selected = select_list_container.data( "select-list" ).getSelected();
        var container = [];

        for ( var i=0; i < selected.length; i++ ) {
            var fixture_palette = {
                "target_uid": selected[i].id,
                "addressing": 1,
                "channel_values": []
            };

            for ( var j=0; j < selected[i].channel_values.length; j++ ) {
                var value_entry = selected[i].channel_values[j];
                fixture_palette.channel_values.push( { "id": value_entry.id, "value": value_entry.value } );
            }

            container.push( fixture_palette );
        }

        return container;
    }

    var palette_color_progression = $("#palette_color_progression" );
    var palette_colors = palette_color_progression.expandableContainer("values");

    var json = {
        "id": current_palette.id,
        "name": $( "#palette_name" ).val(),
        "number": parseInt( $( "#palette_number" ).val() ),
        "description": $( "#palette_description" ).val(),
        "type": parseInt( $( "#palette_type" ).val() ),
        "default_palette": { 
            "target_uid": 0,
            "addressing": 2,
            "channel_values": []
        },
        "palette_colors": palette_colors,
        "fixture_palettes": getPalettes( $("#fixture_container") ),
        "definition_palettes": getPalettes( $("#definition_container") )
    };

    if ( (json.id == 0) && getPaletteByNumber(json.number) != null ) {
        messageBox("Palette number " + json.number + " is already in use");
        return;
    }

    // Get global palette
    var global_channels = $("#global_container").data( "select-list" ).getSelected();
    for ( var i=0; i < global_channels.length; i++ )
        json.default_palette.channel_values.push( { "id": global_channels[i].id, "value": global_channels[i].value } );

    // Send update to server
    var action = (json.id == 0) ? "create" : "update";

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/edit/palette/" +action + "/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        success: function () {
            startup_panels();
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function populate_fixtures() {
    var fixturesPalette = current_palette.getFixturePalettes();

    var selected_fixtures = [];
    for ( var i=0; i < fixturesPalette.length; i++ ) {
        var fixture = getFixtureById( fixturesPalette[i].target_uid );
        selected_fixtures.push( { 
            id: fixture.getId(), 
            label: fixture.getLabel(),
            channel_values: $.extend( true, [], fixturesPalette[i].channel_values )
        } );
    }

    new SelectList( { 
        name: "fixture",
        width: 300,
        container: $("#fixture_container"),
        selected: selected_fixtures,
        render_template: $("#fixture_template")[0].innerHTML,
        render_callback: null,
        update_callback: null,
        remove_callback: null,

        source_callback: function ( select_control ) {
            var selected = select_control.getSelected();

            var fixture_list = filterFixtures( function( fixture ) { 
                if ( fixture.isGroup() )
                    return false;

                for (var i=0; i < selected.length; i++)
                    if ( selected[i].id == fixture.getId() )
                        return false;

                return true; 
            } );

            var items = [];
            for (var i=0; i < fixture_list.length; i++ )
                items.push( { id: fixture_list[i].getId(), label:  fixture_list[i].getLabel() } );
            return items;
        },

        add_callback: function ( select_control, item_id ) {
            return { id: item_id, label: getFixtureById( item_id ).getLabel(), channel_values: [] }
        },
        
        select_callback: function ( select_control, index, item ) {
            var fixture = getFixtureById( item.id );
            if ( fixture == null )
                return;

            var fd = getFixtureDefinitionById( fixture.getFUID() );
            if ( fd == null )
                return;

            populate_channels( item, fd, [ fixture ] );
        },
    } );
}

// ----------------------------------------------------------------------------
//
function populate_fixture_definitions() {
    var definitionPalettes = current_palette.getDefinitionPalettes();

    var selected_definitions = [];
    for ( var i=0; i < definitionPalettes.length; i++ ) {
        var def = getFixtureDefinitionById( definitionPalettes[i].target_uid );
        selected_definitions.push( { 
            id: def.getId(),  
            label: def.getLabel(), 
            channel_values: $.extend( true, [], definitionPalettes[i].channel_values ) 
        } );
    }

    new SelectList( { 
        name: "fixture definition",
        width: 300,
        container: $("#definition_container"),
        selected: selected_definitions,
        render_template: $("#fixture_template")[0].innerHTML,
        render_callback: null,
        update_callback: null,
        remove_callback: null,

        source_callback: function ( select_control ) {
            var selected = select_control.getSelected();

            var definition_list = filterFixtureDefinitions( function( definition ) { 
                for (var i=0; i < selected.length; i++)
                    if ( selected[i].id == definition.getId() )
                        return false;
                return true; 
            } );

            var items = [];
            for (var i=0; i < definition_list.length; i++ )
                items.push( { id: definition_list[i].getId(),  label: definition_list[i].getLabel() } )
            return items;
        },

        add_callback: function ( select_control, item_id ) {
            var def = getFixtureDefinitionById( item_id );
            return { id: def.getId(),  label: def.getLabel(), channel_values: [] }
        },
        
        select_callback: function ( select_control, index, item ) {
            var fd = getFixtureDefinitionById( item.id );
            if ( fd == null )
                return;

            populate_channels( item, fd, filterFixtures( function( fixture ) { return fixture.getFUID() == fd.getId(); } ) );
        },
    } );
}

// ----------------------------------------------------------------------------
//
function populate_channels( item, fd, fixtures ) {
    $("#palette_edit_section").hide("slow");
    $("#select_button_section").hide();
    $("#global_channels_section").hide("slow");
    $("#fixture_select_section").hide("slow");
    $("#channel_sliders_section").show( "slow" );

    $("#channel_sliders_title").text( fd.getManufacturer().toUpperCase() + " " + fd.getModel().toUpperCase() + " CHANNELS" );

    var channels_to_load = [];
    var channel_values = [];

    var channels = $.extend( true, [], fd.getChannels() );

    // Generate labels and values
    for (var i = 0; i < channels.length; i++) {
        var channel = channels[i];

        channel.label = "CH" + (i + 1);
        channel.max_value = 255;
        channel.selectable = true;
        channel.linkable = false;
        channel.value = channel.default_value;
        channel.selected = false;

        // Set value and select this channel if it has been saved
        for ( var k=0; k < item.channel_values.length; k++ ) {
            if ( item.channel_values[k].id == i ) {
                channel.value = item.channel_values[k].value;
                channel.selected = true;
                break;
            }
         }

        channels_to_load.push(channel);
        channel_values.push( channel.value );
    }

    if ( item.channel_values.length == 0 )              // Automatically select applicable channels
        defaultFixtureChannels( channels );

    slider_panel = new SliderPanel( "channel_sliders_pane", channels.length, false );

    var fixture_slider_callback = function(fixture_id, action, channel, value) {
        if ( action == "channel" ) {
            // Automatically select if channel value is changed
            if ( channel_values[channel] != value )
                slider_panel.findChannel( 0, channel ).setSelected( true );

            // If there are any actual fixtures, update them in real-time
            if ( fixtures.length > 0 ) {
                var channel_data = [];

                for ( var i=0; i < fixtures.length; i++ )
                    channel_data.push( {
                        "actor_id": fixtures[i].getId(),
                        "channel": channel,
                        "value": value,
                        "capture": false
                    } );

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/control/fixture/channel/",
                    data: JSON.stringify( { "channel_data": channel_data } ),
                    contentType: 'application/json',
                    async: false,   // These cannot get out of sync
                    cache: false,
                    error: onAjaxError
                });
            }
        }
        else if ( action === "side-text" ) {
            showFixtureDefinitionRanges( fd, channel );        
        }
    }

    // Else cart tip gets cut off
    $("#channel_sliders_pane").css("overflow", "visible");

    slider_panel.allocateChannels( 0, "", channels_to_load, fixture_slider_callback );

    // Capture all fixtures (can be 0..n)
    for ( var i=0; i < fixtures.length; i++ ) {
        var json = {
            "id": fixtures[i].getId(),
            "is_capture": true,
            "channel_values": channel_values
        };
    
        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/control/fixture/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            error: onAjaxError
        });
    }

    var cleanup_function = function () {
        for ( var i=0; i < fixtures.length; i++ ) {
            var json = {
                "id": fixtures[i].getId(),
                "is_capture": false
            };
    
            $.ajax({
                type: "POST",
                url: "/dmxstudio/rest/control/fixture/",
                data: JSON.stringify(json),
                contentType: 'application/json',
                cache: false,
                error: onAjaxError
            });        
        }

        $("#channel_sliders_section").hide();
        $("#palette_edit_section").show("slow");
        showConditionalSections( parseInt( $( "#palette_type" ).val() ) );
        $("#fixture_select_section").show("slow");
        $("#select_button_section").show("slow");
    }

    $("#channel_sliders_cancel").unbind( "click" ).click( cleanup_function );

    $("#channel_sliders_save").unbind( "click" ).click( function () {
        var channel_data = slider_panel.getChannels( 0 );

        item.channel_values = [];

        for (var i=0; i < channel_data.length; i++)
            if ( channel_data[i].selected )
                item.channel_values.push( { "id": channel_data[i].channel, "value": channel_data[i].value } );

        cleanup_function();
    });
}

// ----------------------------------------------------------------------------
//
function showFixtureDefinitionRanges( fd, channel_id ) {
    var channel = fd.getChannels()[channel_id];

    var type_name = channel.name.indexOf(channel.type_name) != -1 ? "" : (" (" + escapeForHTML(channel.type_name) + ")");
    var message = "CH" +  (channel_id+1) + ": " + fd.getManufacturer() + " " + fd.getModel() + "<br/>";
    
    message += channel.name + type_name + "<br/></br/>";
    message += '<div style="max-height: 300px; overflow-y: auto;">';

    for (var j = channel.ranges.length; j-- > 0; ) {
        message += '<div class="set_channel" onclick="setChannelValue(event, 0,' + channel_id + ',' + channel.ranges[j].start + ');">';
        message += formatChannelRange( channel.ranges[j] );
        message += '</div>';
    }

    message += '</div>';

    return  $().toastmessage( 'showToast', {
        text     : message,
        stayTime : 20 * 1000,
        sticky   : channel.ranges.length > 0,
        position : 'top-right',
        type     : '',
        close    : null
    });
}

// ----------------------------------------------------------------------------
//
function setSliderValue( event, owner, channel_id, value ) {
    stopEventPropagation( event );

    var slider = slider_panel.findChannel( owner, channel_id );
    if ( slider != null )
        slider.setValue( value );
}

// ----------------------------------------------------------------------------
//
function defaultFixtureChannels( channels ) {

    var select_channel = function( type ) {
        for ( var i=0; i < channels.length; i++ )
            if ( channels[i].type == type ) // Can be more than one of a type
                channels[i].selected = true;
    }

    switch ( parseInt( $( "#palette_type" ).val() ) ) {
        case PaletteType.PT_LOCATION:
            select_channel( ChannelType.CHNLT_PAN );
            select_channel( ChannelType.CHNLT_PAN_FINE );
            select_channel( ChannelType.CHNLT_TILT );
            select_channel( ChannelType.CHNLT_TILT_FINE );
            break;

        case PaletteType.PT_DIMMER:
            select_channel( ChannelType.CHNLT_DIMMER );
            break;

        case PaletteType.PT_STROBE:
            select_channel( ChannelType.CHNLT_STROBE );
            break;

        case PaletteType.PT_COLOR:
            select_channel( ChannelType.CHNLT_RED );
            select_channel( ChannelType.CHNLT_GREEN );
            select_channel( ChannelType.CHNLT_BLUE );
            select_channel( ChannelType.CHNLT_WHITE );
            select_channel( ChannelType.CHNLT_AMBER );
            break;

        case PaletteType.PT_GOBO:
            select_channel( ChannelType.CHNLT_GOBO );
            break;
    }
}

// ----------------------------------------------------------------------------
//
function generatePaletteFromImage(event) {
    stopEventPropagation(event);

    $("#create_palette_dialog").dialog({
        autoOpen: false,
        width: 640,
        height: 450,
        modal: true,
        resizable: false,
        buttons: {
            "Generate Palette": function () {
                if ($("#cpd_upload_file").val() != "") {
                    window.onerror = function () {
                        window.onerror = null;
                        $("#wait_dialog").dialog("close");
                        messageBox( "Server error encountered while generating palette" );
                    };

                    put_user_on_ice("Generating palette ... please wait");
                    $("#cpd_form")[0].submit();
                }
                else
                    $("#cpd_upload_file").click();

                $(this).dialog("close");
            },
            "Close": function() {
                $(this).dialog("close");
            }
        }
    }).dialog("open");

    $("#cpd_colors").spinner({ min: 1, max: 32 }).val( 16 );

    $("#create_palette_uploadFrame").load( function () { 
        window.parent.generatePaletteResponse(create_palette_uploadFrame.document.body.innerText);
        window.onerror = null;
    });
}

// ----------------------------------------------------------------------------
//
function generatePaletteResponse( response ) {
    $("#wait_dialog").dialog( "close" );

    var json = jQuery.parseJSON( response );

    if ( !json.success ) { 
        messageBox( json.error );
        return;
    }

    var palette_color_progression = $("#palette_color_progression" );
    palette_color_progression.expandableContainer("clear");
    palette_color_progression.expandableContainer("set_values", json.palette );
}