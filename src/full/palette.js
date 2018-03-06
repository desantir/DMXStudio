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

var palettes = [];
var color_chips = [];

var palette_update_callback = null;

// ----------------------------------------------------------------------------
// class Palette
//
function Palette(palette_data)
{
    // Constructor
    jQuery.extend(this, palette_data);
}

// method getId
Palette.prototype.getId = function () {
    return this.id;
}

// method getNumber
Palette.prototype.getNumber = function () {
    return this.number;
}

// method getName
Palette.prototype.getName = function () {
    return this.name;
}

// method getCreated
Palette.prototype.getCreated = function () {
    return this.created;
}
    
// method getDescription
Palette.prototype.getDescription = function () {
    return this.description;
}

// method getType
Palette.prototype.getType = function () {
    return this.type;
}

// method getGlobalPalette
Palette.prototype.getGlobalPalette = function () {
    return this.default_palette;
}

// method getFixturePalettes
Palette.prototype.getFixturePalettes = function() {
    return this.fixture_palettes;
}

// method getDefinitionPalettes
Palette.prototype.getDefinitionPalettes = function() {
    return this.definition_palettes;
}

// method getPaletteColors
Palette.prototype.getPaletteColors = function() {
    return this.palette_colors;
}

// method getPaletteColors
Palette.prototype.getPaletteWeights = function () {
    return this.palette_weights;
}

// ----------------------------------------------------------------------------
//
function getPaletteById(id) {
    for (var i = 0; i < palettes.length; i++)
        if (palettes[i].getId() == id)
            return palettes[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getPaletteByNumber(number) {
    for (var i = 0; i < palettes.length; i++)
        if (palettes[i].getNumber() == number)
            return palettes[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getPaletteByName(name) {
    for (var i = 0; i < palettes.length; i++)
        if (palettes[i].getName() == name)
            return palettes[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getUnusedPaletteNumber() {
    return getNextUnusedNumber( palettes );
}

// ----------------------------------------------------------------------------
//
function filterPalettes( filter ) {
    var objects = [];
    for (var i = 0; i < palettes.length; i++)
        if ( filter( palettes[i] ) )
            objects[objects.length] = palettes[i];
    return objects;
}

// ----------------------------------------------------------------------------
//
function updatePalettes() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/palettes/",
        cache: false,
        success: function (data) {
            //alert(data);
            palettes = [];
            var json = jQuery.parseJSON(data);
            $.map(json, function (palette, index) {
                palettes.push(new Palette(palette));
            });

            paletteUpdated();
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function paletteUpdated() {
    color_chips = [];

    if ( palette_update_callback != null )
        palette_update_callback();
}

// ----------------------------------------------------------------------------
// Called on palette added event 
function newPaletteEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/palette/" + uid,
        cache: false,
        async: false,
        success: function (data) {
            var palette = new Palette( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < palette.length; i++) {
                if (palette[i].getId() == palette.getId() )
                    return;
                    
                if ( palette[i].getNumber() > palette.getNumber() ) {
                    palettes.splice( i, 0, palette );
                    paletteUpdated();
                    return;
                }
            }

            palettes[palettes.length] = palette;
            paletteUpdated();
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on palette changed event 
function changePaletteEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/palette/" + uid,
        cache: false,
        success: function (data) {
            var palette = new Palette( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < palettes.length; i++) {
                if ( palettes[i].getId() == palette.getId() ) {
                    palettes[i] = palette;
                    paletteUpdated();
                    break;
                }
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on palette delete event
function deletePaletteEvent( uid ) {
    for (var i = 0; i < palettes.length; i++) {
        if (palettes[i].getId() == uid) {
            palettes.splice( i, 1 );
            paletteUpdated();
            break;
        }
    }
}

// ----------------------------------------------------------------------------
//
function getColorChips( )  {
    if ( color_chips.length == 16 )
        return color_chips;
        
    // Build the chips
    color_chips = [];
    
    var palette = getPaletteByNumber( 10000 );
    if ( palette == null || palette.getType() != PaletteType.PT_COLOR_PALETTE )
        return color_chips;

    var colors = palette.getPaletteColors();
    if ( colors == null )
        return color_chips;

    for ( var i=0; i < colors.length; i++ )
        color_chips.push( "#" + colors[i] );

    while ( color_chips.length < 16 )
        color_chips.push( "#000000" );

    return color_chips;
}

// ----------------------------------------------------------------------------
//
function setColorChip( chip_number, hex ) {
    if ( !edit_mode || chip_number < 1 || chip_number > 16 )
        return false;

    var palette = getPaletteByNumber( 10000 );
    if ( palette == null || palette.getType() != PaletteType.PT_COLOR_PALETTE )
        return false;

    palette.getPaletteColors()[chip_number-1] = hex;

    // Send update to server
    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/edit/palette/update/",
        data: JSON.stringify(palette),
        contentType: 'application/json',
        cache: false,
        error: onAjaxError
    });

    return true;
}