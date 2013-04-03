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

var scenes = new Array();

// ----------------------------------------------------------------------------
// class Scene
//
function Scene(scene_data)
{
    // Constructor
    jQuery.extend(this, scene_data);

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
        return this.name;
    }

    // method getDescription
    this.getDescription = function () {
        return this.description;
    }

    // method isDefault
    this.isDefault = function () {
        return this.is_default;
    }

    // method isPrivate
    this.isPrivate = function () {
        return this.is_private;
    }

    // method getUserTypeName
    this.getUserTypeName = function () {
        return "Scene";
    }

    // method isActive 
    this.isActive = function () {
        return scene_tile_panel.isActive(this.id);
    }

    // method hasAnimations
    this.hasAnimations = function () {
        return this.animations.length > 0;
    }

    // method getAnimations
    this.getAnimations = function () {
        return this.animations;
    }

    // method getActors
    this.getActors = function () {
        return this.actors;
    }
}

// ----------------------------------------------------------------------------
//
function getSceneById(id) {
    for (var i = 0; i < scenes.length; i++)
        if (scenes[i].getId() == id)
            return scenes[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getSceneByNumber(number) {
    for (var i = 0; i < scenes.length; i++)
        if (scenes[i].getNumber() == number)
            return scenes[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getUnusedSceneNumber() {
    for ( var i=1; i < 50000; i++ )
        if ( getSceneByNumber(i) == null )
            return i;
    return 99999;
}

// ----------------------------------------------------------------------------
//
function updateScenes() {
    scene_tile_panel.empty();

    $.ajax({
        type: "GET",
        url: "/dmxstudio/full/query/scenes/",
        cache: false,
        success: function (data) {
            //alert(data);
            var json = jQuery.parseJSON(data);
            scenes.length = 0;
            active_scene_id = 0;

            $.map(json, function (scene, index) {
                var title = escapeForHTML(scene.name);
                scene_tile_panel.addTile(scene.id, scene.number, title, !scene.is_default);
                scenes.push( new Scene(scene) );
                if (scene.is_running)
                    markActiveScene(scene.id);
            });

            setEditMode(edit_mode);     // Refresh editing icons on new tiles
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function playScene(event, scene_id) {
    stopEventPropagation(event);

    if (scene_id == active_scene_id)
        scene_id = 0;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/full/control/scene/show/" + scene_id,
        cache: false,
        success: function () {
            markActiveScene(scene_id);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createScene(event) {
    stopEventPropagation(event);

    var new_number = getUnusedSceneNumber();

    var actors = [];

    var list = getActiveFixtures();

    for (var i = 0; i < list.length; i++) {
        if (list[i].isGroup()) { // Flatten out groups for now
            for (var j = 0; j < list[i].getFixtureIds().length; j++)
                actors.push(list[i].getFixtureIds()[j]);
        }
        else
            actors.push(list[i].getId());
    }

    openNewSceneDialog("Create Scene", {
        id: 0,
        name: "New Scene " + new_number,
        description: "",
        number: new_number,
        private_scene: false,
        animations: [],
        actors: actors
    } );
}

// ----------------------------------------------------------------------------
//
function editScene(event, scene_id) {
    stopEventPropagation(event);

    var scene = getSceneById(scene_id);
    if (scene == null)
        return;

    var actors = [];
    for (var i = 0; i < scene.getActors().length; i++)
        actors.push(scene.getActors()[i].id);

    openNewSceneDialog("Update Scene", {
        id: scene.getId(),
        name: scene.getName(),
        description: scene.getDescription(),
        number: scene.getNumber(),
        private_scene: scene.isPrivate(),
        animations: scene.getAnimations(),
        actors: actors
    });
}

// ----------------------------------------------------------------------------
//
function openNewSceneDialog(dialog_title, data) {

    var send_update = function ( make_copy ) {
        var json = {
            id: data.id,
            name: $("#nsd_name").val(),
            description: $("#nsd_description").val(),
            number: parseInt($("#nsd_number").val()),
            is_private: $("#nsd_private_scene").is(":checked"),
            actors: $("#nsd_fixtures").val(),
            animations: $("#nsd_animations").data("animations")
        };

        if ( (make_copy || json.number != data.number) && getSceneByNumber(json.number) != null ) {
            alert("Scene number " + json.number + " is already in use");
            return;
        }

        var action;
        if (make_copy)
            action = "copy";
        else if (json.id == 0)
            action = "create";
        else
            action = "update";

        $.ajax({
            type: "POST",
            url: "/dmxstudio/full/edit/scene/" +action + "/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            success: function () {
                updateScenes();

                if ( action != "update" )
                    updateFixtures();
            },
            error: onAjaxError
        });

        $("#new_scene_dialog").dialog("close");
    };

    var dialog_buttons = new Array();

    if (data.id != 0) {
        dialog_buttons[dialog_buttons.length] = {
            text: "Make Copy",
            click: function () { send_update(true); }
        };
    }

    dialog_buttons[dialog_buttons.length] = {
        text: dialog_title,
        click: function () { send_update(false); }
    };

    dialog_buttons[dialog_buttons.length] = {
        text: "Cancel",
        click: function () {
            $("#new_scene_dialog").dialog("close");
        }
    };

    $("#new_scene_dialog").dialog({
        autoOpen: false,
        width: 780,
        height: 620,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_scene_dialog").dialog( "option", "title", dialog_title + " " + data.number );

    $("#nsd_accordion").accordion({ heightStyle: "fill" });

    $("#nsd_number").spinner({ min: 1, max: 100000 }).val (data.number );
    $("#nsd_name").val(data.name);
    $("#nsd_description").val(data.description);
    $("#nsd_copy_captured_fixtures").attr('checked', data.copy_captured_fixtures);
    $("#nsd_private_scene").attr('checked', data.private_scene);

    $("#nsd_animations").data("animations", jQuery.extend(true, [], data.animations));
    $("#nsd_animations").data("scene", data.id);

    $("#nsd_fixtures").empty();

    // Fill in the scene's fixures
    for (var i = 0; i < data.actors.length; i++) {
        var fixture_id = data.actors[i];
        var fixture = getFixtureById(fixture_id);

        $("#nsd_fixtures").append($('<option>', {
            value: fixture_id,
            text: fixture.getNumber() + ": " + fixture.getFullName(),
            selected: true
        }));
    }

    $("#nsd_fixtures").multiselect({ minWidth: 500, multiple: true, noneSelectedText: 'select fixtures' });

    populate_animations();

    $("#nsd_new_animation_button").hover(function () {
            $(this).addClass('ui-state-hover');
        },
        function () {
            $(this).removeClass('ui-state-hover');
    });

    $("#nsd_new_animation_button").bind("click", function (event) {
       scene_add_animation(event);
    });

    $("#nsd_new_animation_type").empty();
    for (var i = 0; i < Animations.length; i++) {
        $("#nsd_new_animation_type").append($('<option>', {
            value: Animations[i].class_name,
            text: Animations[i].name,
            selected: i == 0
        }));
    }
    $("#nsd_new_animation_type").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'select animation', height: "auto" });

    $("#nsd_new_animation_type").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        $("#nsd_new_animation_description").text(findAnimation(ui.value).description);
    });

    $("#nsd_new_animation_description").text(Animations[0].description);

    $("#new_scene_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function populate_animations() {

    $("#nsd_animations").empty();

    var animations = $("#nsd_animations").data("animations");

    var template = $("#nsd_animation_template")[0].innerHTML;

    for (var a = 0; a < animations.length; a++) {
        var animation_num = a + 1;

        var animation_div_text = template.replace(/NNN/g, animation_num);

        $("#nsd_animations").append(animation_div_text);
        $("#nsd_animation_" + animation_num + "_edit").button().html(createAnimationButtonText(animations[a]));
        $("#nsd_animation_" + animation_num + "_edit").attr("title", getAnimationSynopsis(animations[a]));
    }
}

// ----------------------------------------------------------------------------
//
function scene_remove_animation(event, anim_num) {
    stopEventPropagation(event);

    var animations = $("#nsd_animations").data("animations");
    animations.splice(anim_num - 1, 1);
    populate_animations();
}

// ----------------------------------------------------------------------------
//
function scene_edit_animation(event, anim_num) {
    stopEventPropagation(event);

    var animations = $("#nsd_animations").data("animations");
    var scene = getSceneById($("#nsd_animations").data("scene"));

    show_animation_edit_dialog(animations[anim_num - 1], function (animation) {
        animations[anim_num - 1] = animation;
        populate_animations();
    });
}

// ----------------------------------------------------------------------------
//
function scene_add_animation(event) {
    stopEventPropagation(event);

    var class_name = $("#nsd_new_animation_type").val();

    add_new_animation(class_name, function (new_animation) {
        var animations = $("#nsd_animations").data("animations");
        animations[animations.length] = new_animation;
        populate_animations();
    });
}

// ----------------------------------------------------------------------------
//
function deleteScene(event, scene_id) {
    stopEventPropagation(event);

    deleteVenueItem(getSceneById(scene_id), function (item) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/full/delete/scene/" + item.getId(),
            cache: false,
            success: updateScenes,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function describeScene(event, scene_id) {
    stopEventPropagation(event);

    var scene = getSceneById(scene_id);
    if (scene == null) {
        alert("No definition for scene " + scene_id + " in client");
        return;
    }

    $("#describe_scene_dialog").dialog({
        autoOpen: false,
        width: 600,
        height: 550,
        modal: false,
        draggable: true,
        resizable: true,
        title: "Scene " + scene.getNumber() + ": " + escapeForHTML(scene.getName())
    });

    $("#describe_scene_number").html(scene.getNumber());
    $("#describe_scene_name").html(escapeForHTML(scene.getName()));
    $("#describe_scene_fixture_count").html(scene.getActors().length);
    $("#describe_scene_animation_count").html(scene.getAnimations().length);
    $("#describe_scene_description").html(scene.getDescription() != null ? escapeForHTML(scene.getDescription()) : "");

    var info = "";
    var clazz = fixture_tile_panel.actions[0].tile_class;

    for (var i=0; i < scene.getActors().length; i++) {
        var actor = scene.getActors()[i];
        var fixture = getFixtureById(actor.id);

        info += "<div style=\"clear:both; float: left;\" >";
        info += "<div class=\"" + clazz + "\" style=\"float:left; margin-right: 4px; cursor: pointer;\" onclick=\"controlFixture( event, " + fixture.getId() + ");\" title=\"toggle control fixture\"></div>";
        info += "Fixture " + fixture.getNumber() + ": " + escapeForHTML(fixture.getFullName());
        info += "<span class=\"describe_scene_dmx\">DMX " + actor.address + "</span>";
        info += "</div>";

        if (actor.channels.length) {
            info += "<div class=\"describe_scene_channels\">"
            for (var j = 0; j < actor.channels.length; j++) {
                var channel = actor.channels[j];

                info += "<div>";
                info += "Channel " + (channel.channel + 1) + ": " + escapeForHTML(channel.name) + " = " + channel.value;

                if (channel.range_name.length > 0)
                    info += " (" + escapeForHTML(channel.range_name) + ")";

                info += " </div>";
            }
            info += " </div>";
        }
    }

    $("#describe_scene_fixtures").html(info);

    info = "";

    if (scene.getAnimations().length > 0) {
        info += "<hr style=\"clear: both; margin-top: 10px;\"/>";

        for (var i = 0; i < scene.getAnimations().length; i++) {
            var animation = scene.getAnimations()[i];

            var signal = getSignalSynopsis(animation.signal);
            var synopsis = getAnimationSynopsis( animation );
            var fixtures = "";

            if (animation.actors.length > 0) {
                for (var j = 0; j < animation.actors.length; j++) {
                    if (j > 0)
                        fixtures += ", ";
                    fixtures += getFixtureById(animation.actors[j]).getNumber();
                }
            }
            else
                fixtures = "NONE";

            info += "<div style=\"clear:both; float: left;\" >";
            info += "Animation: " + animation.name;
            info += "</div>";

            info += "<div class=\"describe_scene_channels\">";

            if (synopsis.length > 0)
                info += synopsis + "<br />";
 
            info += "Fixtures: " + fixtures +  "<br />";
            info += "Signal: " + signal;
            info += "</div>";
        }
    }

    $("#describe_scene_animations").html(info);

    $("#describe_scene_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function markActiveScene(new_scene_id) {
    if (new_scene_id == active_scene_id)
        return;

    if (active_scene_id != 0) {
        scene_tile_panel.selectTile(active_scene_id, false);
        active_scene_id = null;
    }

    if (new_scene_id != 0) {
        scene_tile_panel.selectTile(new_scene_id, true);
        active_scene_id = new_scene_id;
    }
}
