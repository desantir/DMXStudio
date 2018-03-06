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

var fixture_definitions = [];

// ----------------------------------------------------------------------------
// class FixtureDefinition
//
function FixtureDefinition(fixture_data)
{
    // Constructor
    jQuery.extend(this, fixture_data);
}

// method getId
FixtureDefinition.prototype.getId = function () {
    return this.id;
}

// method getManufacturer
FixtureDefinition.prototype.getManufacturer = function () {
    return this.manufacturer;
}

// method getModel
FixtureDefinition.prototype.getModel = function () {
    return this.model;
}

// method getLabel
FixtureDefinition.prototype.getLabel = function () {
    return this.getManufacturer() + " " + this.getModel() + " (" + this.getNumChannels() + " channel)"
}

// method getTypeName
FixtureDefinition.prototype.getTypeName = function () {
    return this.type_name;
}

// method getNumChannels
FixtureDefinition.prototype.getNumChannels = function () {
    return this.num_channels;
}

// method getChannels
FixtureDefinition.prototype.getChannels = function () {
    return this.channels;
}

// ----------------------------------------------------------------------------
// 
function getFixtureDefinitionById( id ) {
    for (var i = 0; i < fixture_definitions.length; i++)
        if (fixture_definitions[i].getId() == id)
            return fixture_definitions[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function filterFixtureDefinitions( filter ) {
    var objects = [];
    for (var i = 0; i < fixture_definitions.length; i++)
        if ( filter( fixture_definitions[i] ) )
            objects[objects.length] = fixture_definitions[i];
    return objects;
}

// ----------------------------------------------------------------------------
//
function updateFixtureDefinitions() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/fixture/definitions/",
        cache: false,
        success: function ( data ) {
            var definitions = JSON.parse(data);

            fixture_definitions = [];

            // Flatten out the definitions
            for (var m = 0; m < definitions.length; m++) {
                for (var f = 0; f < definitions[m].models.length; f++) {
                    for (var p = 0; p < definitions[m].models[f].personalities.length; p++) {
                        fixture_definitions.push( new FixtureDefinition( { 
                                "manufacturer": definitions[m].manufacturer,
                                "model": definitions[m].models[f].model,
                                "id": definitions[m].models[f].personalities[p].fuid,
                                "num_channels": definitions[m].models[f].personalities[p].num_channels,
                                "type": definitions[m].models[f].personalities[p].type,
                                "channels": definitions[m].models[f].personalities[p].channels
                        } ) );
                    }
                }
            }
        },
        error: onAjaxError
    });
}
