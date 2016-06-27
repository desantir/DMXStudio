/* 
Copyright (C) 2012-2015 Robert DeSantis
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

var scenes = [];

var BPMRatings = [
    { name: "no rating", lower: 0, upper: 999 },
    { name: "very slow (< 60)", lower: 0, upper: 59 },
    { name: "slow (60-79)", lower: 60, upper: 79 },
    { name: "medium (80-119)", lower: 80, upper: 119 },
    { name: "fast (120-149)", lower: 120, upper: 149 },
    { name: "very fast (>= 150)", lower: 150, upper: 999 },
];

var last_animation_type = null;                         // restores last select new animation type in scene edit
var scene_update_listener = null;

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

    // method getBPMRating
    this.getBPMRating = function () {
        return this.bpm_rating;
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

    // method getActs
    this.getActs = function () {
        return this.acts;
    }

    // method getActorIds
    this.getActorIds = function () {
        var actor_ids = [];
        for (var i = 0; i < this.actors.length; i++)
            actor_ids.push(this.actors[i].id);
        return actor_ids;
    }

    // method getActor( actor_id )
    this.getActor = function (actor_id) {
        for (var i = 0; i < this.actors.length; i++)
            if (this.actors[i].id == actor_id)
                return this.actors[i];
        return null;
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
function getDefaultSceneId() {
    for (var i=0; i < scenes.length; i++)
        if (scenes[i].isDefault())
            return scenes[i].getId();
    return 0;
}

// ----------------------------------------------------------------------------
//
function updateScenes() {
    $("#copy_scene_button").hide();

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/scenes/",
        cache: false,
        success: function (data) {
            //alert(data);
            scenes = [];
            var json = jQuery.parseJSON(data);
            $.map(json, function (scene, index) {
                scenes.push(new Scene(scene));
            });
            createSceneTiles();

            if (scene_update_listener)
                scene_update_listener();
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createSceneTiles() {
    highlightSceneFixtures(active_scene_id, false);
    scene_tile_panel.empty();
    active_scene_id = 0;

    $.each( scenes, function ( index, scene ) {
        if (scene.number == 0 || current_act == 0 || scene.acts.indexOf(current_act) != -1) {
            var title = escapeForHTML(scene.name);
            scene_tile_panel.addTile(scene.id, scene.number, title, !scene.is_default);
            if (scene.is_running)
                markActiveScene(scene.id);
        }
    });

    setEditMode(edit_mode);         // Refresh editing icons on new tiles
}

// ----------------------------------------------------------------------------
// NOTE: This method is invoked by scene tile clicks only
function playScene(event, scene_id) {
    if (scene_id == active_scene_id)
        scene_id = getDefaultSceneId();

    selectScene( event, scene_id );
}

// ----------------------------------------------------------------------------
//
function selectScene(event, scene_id) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/scene/show/" + scene_id,
        cache: false,
        success: function () {
            markActiveScene(scene_id);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function copyScene(event) {
    stopEventPropagation(event);

    var scene = getSceneById(active_scene_id);
    if (scene == null)
        return;

    var new_number = getUnusedSceneNumber();

    var actors = [];
    for (var i = 0; i < scene.getActors().length; i++)
        actors.push(scene.getActors()[i].id);

    openNewSceneDialog("Copy Scene " + scene.getName(), "Create Scene", true,  {
        id: scene.getId(),
        name: "Copy of " + scene.getName(),
        description: scene.getDescription(),
        number: new_number,
        bpm_rating: scene.getBPMRating(),
        animations: scene.getAnimations(),
        acts: scene.getActs(),
        actors: actors
    });
}

// ----------------------------------------------------------------------------
//
function createScene(event) {
    stopEventPropagation(event);

    var new_number = getUnusedSceneNumber();
    var list = getActiveFixtures();

    var actors = [];
    for (var i = 0; i < list.length; i++)
        actors.push(list[i].getId());

    openNewSceneDialog("New Scene", "Create Scene", false, {
        id: 0,
        name: "New Scene " + new_number,
        description: "",
        number: new_number,
        bpm_rating: 0,
        animations: [],
        acts: current_act == 0 ? [] : [ current_act ],
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

    openNewSceneDialog("Edit Scene " + scene.getName(), "Update Scene", false, {
        id: scene.getId(),
        name: scene.getName(),
        description: scene.getDescription(),
        number: scene.getNumber(),
        bpm_rating: scene.getBPMRating(),
        animations: scene.getAnimations(),
        actors: actors,
        acts: scene.getActs()
    });
}

// ----------------------------------------------------------------------------
//
function openNewSceneDialog(dialog_title, action_title, copy, data) {

    var send_update = function ( make_copy ) {
        var acts = $("#nsd_acts").val();
        if (acts == null)
            acts = [];

        var json = {
            id: data.id,
            name: $("#nsd_name").val(),
            description: $("#nsd_description").val(),
            number: parseInt($("#nsd_number").val()),
            bpm_rating: parseInt($("#nsd_bpm_rating").val()),
            keep_groups: $("#nsd_keep_groups").is(":checked"),
            actors: $("#nsd_fixtures").val(),
            acts: acts,
            animations: $("#nsd_animations").data("animations")
        };

        if (json.actors == null)
            json.actors = [];

        if ( (make_copy || json.number != data.number) && getSceneByNumber(json.number) != null ) {
            messageBox("Scene number " + json.number + " is already in use");
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
            url: "/dmxstudio/rest/edit/scene/" +action + "/",
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

    if (data.id != 0 && !copy) {
        dialog_buttons[dialog_buttons.length] = {
            text: "Make Copy",
            click: function () { send_update(true); }
        };
    }

    if (data.id != 0)
        $("#nsd_keep_groups_section").hide();
    else
        $("#nsd_keep_groups_section").show();

    dialog_buttons[dialog_buttons.length] = {
        text: action_title,
        click: function () { send_update(copy); }
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
        height: 680,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_scene_dialog").dialog( "option", "title", dialog_title );

    $("#nsd_accordion").accordion({ heightStyle: "fill" });

    if (data.id == 0)
        $("#nsd_accordion").accordion({ active: 0 });

    $("#nsd_number").spinner({ min: 1, max: 100000 }).val (data.number );
    $("#nsd_name").val(data.name);
    $("#nsd_description").val(data.description);
    $("#nsd_copy_captured_fixtures").attr('checked', data.copy_captured_fixtures);

    $("#nsd_animations").data("animations", jQuery.extend(true, [], data.animations));  // Generate a deep copy
    $("#nsd_animations").data("scene", data.id);

    $("#nsd_fixtures").empty();

    // Fill in the scene's fixures
    for (var i = 0; i < data.actors.length; i++) {
        var fixture_id = data.actors[i];
        var fixture = getFixtureById(fixture_id);

        $("#nsd_fixtures").append($('<option>', {
            value: fixture_id,
            text: fixture.getLabel(),
            selected: true
        }));
    }

    $("#nsd_fixtures").multiselect({ minWidth: 500, multiple: true, noneSelectedText: 'select fixtures' });

    // After fixtures are changed, remove any animation actors that have been removed
    $("#nsd_fixtures").unbind("multiselectclose").bind("multiselectclose", function (event, ui) {
        verifyAnimationActors();
    });
    
    $("#nsd_acts").empty();

    for (var i=1; i <= 20; i++) {
        $("#nsd_acts").append($('<option>', {
            value: i,
            text: i,
            selected: data.acts.indexOf( i ) > -1
        }));
    }

    $("#nsd_acts").multiselect({
        minWidth: 300, multiple: true, noneSelectedText: 'all acts',
        checkAllText: 'All acts', uncheckAllText: 'Clear acts', selectedList: 8
    });

    $("#nsd_bpm_rating").empty();

    for (var i=0; i < BPMRatings.length; i++) {
        $("#nsd_bpm_rating").append($('<option>', {
            value: i,
            text: BPMRatings[i].name,
            selected: data.bpm_rating == i
        }));
    }

    $("#nsd_bpm_rating").multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: false, height: "auto"
    });

    populate_animations();

    $("#nsd_new_animation_button").hover(function () {
            $(this).addClass('ui-state-hover');
        },
        function () {
            $(this).removeClass('ui-state-hover');
    });

    $("#nsd_new_animation_button").unbind("click").bind("click", function (event) {
       scene_add_animation(event);
    });

    if ( last_animation_type == null )
        last_animation_type = Animations[0].class_name; 

    $("#nsd_new_animation_type").empty();
    for (var i = 0; i < Animations.length; i++) {
        $("#nsd_new_animation_type").append($('<option>', {
            value: Animations[i].class_name,
            text: Animations[i].name,
            selected: Animations[i].class_name == last_animation_type
        }));
    }
    $("#nsd_new_animation_type").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'select animation', height: "auto" });

    $("#nsd_new_animation_type").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        last_animation_type = ui.value;
        $("#nsd_new_animation_description").text(findAnimation(ui.value).description);
    });

    $("#nsd_new_animation_description").text(Animations[0].description);

    $("#new_scene_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function verifyAnimationActors() {
    var fixtures = $("#nsd_fixtures").val();
    if (fixtures == null)
        fixtures = [];

    var animations = $("#nsd_animations").data("animations");
    var changed = false;

    function containsFixture( value ) {
        for (var k=0; k < fixtures.length; k++)     // Don't use "in" as actor ID type can be either number or string
            if (fixtures[k] == value)
                return true;
       return false;
    }

    for (var i = 0; i < animations.length; i++) {
        for (var j = 0; j < animations[i].actors.length;) {
            if (!containsFixture(animations[i].actors[j])) {
                animations[i].actors.splice(j, 1);
                changed = true;
            }
            else
                j++;
        }
    }

    if ( changed )
        populate_animations();
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
            url: "/dmxstudio/rest/delete/scene/" + item.getId(),
            cache: false,
            success: updateScenes,
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function makeChannelInfoLine(channels, css_class) {
    if (!channels.length)
        return "";

    var html = "<div class=\"" + css_class  + "\">";

    for (var j = 0; j < channels.length; j++) {
        var channel = channels[j];

        html += "<div>";
        html += "Channel " + (channel.channel + 1) + ": " + escapeForHTML(channel.name) + " = " + channel.value;

        if (channel.range_name != null && channel.range_name.length > 0) {
            html += " (" + escapeForHTML(channel.range_name) + ")";
        }
        else if (channel.ranges != null) {
            for (var k = 0; k < channel.ranges.length; k++) {
                var range = channel.ranges[k];
                if (channel.value >= range.start && channel.value <= range.end) {
                    html += " (" + escapeForHTML(range.name) + ")";
                    break;
                }
            }
        }
        html += " </div>";
    }
    html += " </div>";

    return html;
}

// ----------------------------------------------------------------------------
//
function describeScene(event, scene_id) {
    stopEventPropagation(event);

    var scene = getSceneById(scene_id);
    if (scene == null) {
        messageBox("No definition for scene " + scene_id + " in client");
        return;
    }

    var is_default_scene = scene.isDefault();

    $("#describe_scene_dialog").dialog({
        autoOpen: false,
        width: 600,
        height: 550,
        modal: false,
        draggable: true,
        resizable: true,
        title: "Scene " + scene.getNumber() + ": " + escapeForHTML(scene.getName()),
        open: function () {
            fixture_state_listener = function (fixture_id, new_state) {
                if ( !is_default_scene )
                    update_describe_fixture_icon(fixture_id);
                else
                    describe_scene_content(scene_id);
            }

            scene_update_listener = function () {
                describe_scene_content(scene_id);
            }
        },

        close: function () {
            fixture_state_listener = null;
            scene_update_listener = null;
            describe_data = null;
        },
    });

    $("#describe_scene_dialog").dialog("open");

    describe_scene_content(scene_id);
}

var describe_data = null;

function update_describe_fixture_icon(fixture_id) {
    var icon = $("#describe_" + fixture_id);
    if (icon != null && icon.length > 0) {
        if (!getFixtureById(fixture_id).isActive()) {
            icon.removeClass("ui-icon-close").addClass("ui-icon-pin-s");
            icon.attr('title', 'control fixture');
        }
        else {
            icon.removeClass("ui-icon-pin-s").addClass("ui-icon-close");
            icon.attr('title', 'release fixture');
        }
    }
}

function describe_scene_content( scene_id )
{
    var scene = getSceneById(scene_id);
    if (scene == null) {
        messageBox("No definition for scene " + scene_id + " in client");
        return;
    }

    describe_data = [];

    var acts = "";
    if (scene.getActs().length > 0) {
        for (var i = 0; i < scene.getActs().length; i++)
            acts += scene.getActs()[i] + " ";
    }
    else
        acts = "None";

    $("#describe_scene_number").html(scene.getNumber());
    $("#describe_scene_name").html(escapeForHTML(scene.getName()));
    $("#describe_scene_acts").html(acts);
    $("#describe_scene_fixture_count").html(scene.getActors().length);
    $("#describe_scene_animation_count").html(scene.getAnimations().length);
    $("#describe_scene_description").html(scene.getDescription() != null ? escapeForHTML(scene.getDescription()) : "");
    $("#describe_scene_bpm_rating").html(escapeForHTML(BPMRatings[scene.getBPMRating()].name));

    if ( !scene.isDefault() )
        $("#describe_scene_number").unbind().on('click', function (event) { sceneDescribeSelectAll(event, scene_id, true); });

    function makeFixtureTitleLine(fixture) {
        var html = "<div style='clear:both; float: left; font-size: 10pt;'>";

        html += "<div id='describe_" + fixture.getId() + "' class='ui-icon' ";
        html += "style='margin-right: 5px; cursor: pointer; float: left;' ";
        html += "onclick='sceneDescribeSelect(event," + fixture.getId() + ");'";
        html += "></div>";

        html += "<div style='float: left;'>Fixture " + (fixture.isGroup() ? "Group G" : "") + fixture.getNumber();
        html += ": " + escapeForHTML(fixture.getFullName()) + "</div>";

        if (!fixture.isGroup()) {
            html += "<div class='describe_scene_dmx' style='float: left; margin-top: 2px;'>";
            html += " DMX " + fixture.getUniverseId() + "&nbsp;-&nbsp;" + fixture.getDMXAddress() + "</div>";
        }
        html += "</div>";

        return html;
    }

    var info = "";

    if (!scene.isDefault()) {
        for (var i = 0; i < scene.getActors().length; i++) {
            var actor = scene.getActors()[i];
            var fixture = getFixtureById(actor.id);

            var channel_data = [];
            for (var j = 0; j < actor.channels.length; j++)
                channel_data.push(actor.channels[j].value);

            info += makeFixtureTitleLine(fixture);
            info += makeChannelInfoLine(actor.channels, "describe_scene_channels");

            describe_data[describe_data.length] = { "fixture_id": actor.id, "channel_data": channel_data };
        }
    }
    else {
        var fixtures = getActiveFixtures();
        for (var i = 0; i < fixtures.length; i++) {
            var fixture = fixtures[i];
            info += makeFixtureTitleLine(fixture);
            info += makeChannelInfoLine(fixture.getChannels(), "describe_scene_channels");

            describe_data[describe_data.length] = { "fixture_id": fixture.getId(), "channel_data": null };
        }
    }

    $("#describe_scene_fixtures").html(info);

    for (var i=0; i < describe_data.length; i++)
        update_describe_fixture_icon(describe_data[i].fixture_id);

    info = "";

    if (scene.getAnimations().length > 0) {
        for (var i = 0; i < scene.getAnimations().length; i++) {
            var animation = scene.getAnimations()[i];

            var signal = getSignalSynopsis(animation.signal);
            var synopsis = getAnimationSynopsis(animation);
            var fixtures = "";

            if (animation.actors.length > 0) {
                for (var j = 0; j < animation.actors.length; j++) {
                    if (j > 0)
                        fixtures += ", ";

                    var fixture = getFixtureById(animation.actors[j]);

                    if (fixture.isGroup())
                        fixtures += "G";

                    fixtures += fixture.getNumber();
                }
            }
            else
                fixtures = "NONE";

            info += "<div style=\"clear:both; float: left; font-size: 10pt;\" >";
            info += "Animation: " + getAnimationName(animation);
            info += "</div>";

            info += "<div class=\"describe_scene_channels\">";

            if (synopsis.length > 0)
                info += synopsis.replace(/\n/g, "<br />") + "<br />";

            info += "Fixtures: " + fixtures + "<br />";
            info += "Signal: " + signal;
            info += "</div>";
        }

        $("#describe_scene_hr").show();
    }
    else
        $("#describe_scene_hr").hide();

    $("#describe_scene_animations").html(info);
}

function sceneDescribeSelectAll(event, scene_id, select) {
    stopEventPropagation(event);

    for (var i=0; i < describe_data.length; i++) {
        var fixture = getFixtureById(describe_data[i].fixture_id);
        if ( !fixture.isActive() )
            sceneDescribeSelect(null, fixture.getId());
    }
}

function sceneDescribeSelect(event, fixture_id) {
    stopEventPropagation(event);

    for (var i = 0; i < describe_data.length; i++)
        if (describe_data[i].fixture_id == fixture_id) {
            controlFixture2(null, fixture_id, describe_data[i].channel_data, false);
            update_describe_fixture_icon(fixture_id);
            break;
        }
}

// ----------------------------------------------------------------------------
//
function markActiveScene(new_scene_id) {
    if (new_scene_id == active_scene_id)
        return;

    if (active_scene_id != 0) {
        scene_tile_panel.selectTile(active_scene_id, false);
        highlightSceneFixtures(active_scene_id, false);
        active_scene_id = 0;
    }

    if (new_scene_id != 0) {
        scene_tile_panel.selectTile(new_scene_id, true);
        highlightSceneFixtures(new_scene_id, true);
        active_scene_id = new_scene_id;
    }

    if ( active_scene_id != 0 )
        $("#copy_scene_button").show();
    else
        $("#copy_scene_button").hide();
}

// ----------------------------------------------------------------------------
//
function highlightSceneFixtures(scene_id, highlight) {
    if (scene_id == 0)
        return;

    var scene = getSceneById(scene_id);
    if (scene && !scene.isDefault())
        highlightFixtures(scene.getActorIds(), highlight);
}
