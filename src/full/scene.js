/* 
Copyright (C) 2012-2016 Robert DeSantis
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
var sceneTileUpdateTimer = null;

// ----------------------------------------------------------------------------
// class Scene
//
function Scene(scene_data)
{
    // Constructor
    jQuery.extend(this, scene_data);
}

// method getId
Scene.prototype.getId = function () {
    return this.id;
}

// method getNumber
Scene.prototype.getNumber = function () {
    return this.number;
}

// method getName
Scene.prototype.getName = function () {
    return this.name;
}

// method getCreated
Scene.prototype.getCreated = function () {
    return this.created;
}

// method getBPMRating
Scene.prototype.getBPMRating = function () {
    return this.bpm_rating;
}

// method getFullName
Scene.prototype.getFullName = function () {
    return this.name;
}

// method getDescription
Scene.prototype.getDescription = function () {
    return this.description;
}

// method isDefault
Scene.prototype.isDefault = function () {
    return this.is_default;
}

// method getUserTypeName
Scene.prototype.getUserTypeName = function () {
    return "Scene";
}

// method isActive 
Scene.prototype.isActive = function () {
    return scene_tile_panel.isActive(this.id);
}

// method hasAnimations
Scene.prototype.hasAnimations = function () {
    return this.animations.length > 0;
}

// method getAnimations
Scene.prototype.getAnimationRefs = function () {
    return this.animation_refs;
}

// method getActors
Scene.prototype.getActors = function () {
    return this.actors;
}

// method getActs
Scene.prototype.getActs = function () {
    return this.acts;
}

// method getActorIds
Scene.prototype.getActorIds = function () {
    var actor_ids = [];
    for (var i = 0; i < this.actors.length; i++)
        actor_ids.push(this.actors[i].id);
    return actor_ids;
}

// method getActor( actor_id )
Scene.prototype.getActor = function (actor_id) {
    for (var i = 0; i < this.actors.length; i++)
        if (this.actors[i].id == actor_id)
            return this.actors[i];
    return null;
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
    return getNextUnusedNumber( scenes );
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
function filterScenes( filter ) {
    var objects = [];
    for (var i = 0; i < scenes.length; i++)
        if ( filter( scenes[i] ) )
            objects[objects.length] = scenes[i];
    return objects;
}

// ----------------------------------------------------------------------------
//
function getDefaultSceneActor( actor_id ) {
    var default_scene = getSceneById( getDefaultSceneId() );
    if ( default_scene == null )
        return null;

    return default_scene.getActor( actor_id );
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
            createSceneTiles( true );

        if (scene_update_listener)
            scene_update_listener( 0 );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createSceneTiles( no_wait ) {
    if ( no_wait )
        _createSceneTiles();
    else
        delayUpdate( "sceneTiles", 1, _createSceneTiles );
}

function _createSceneTiles() {
    // MUST reset all of these as active scene object may have changed
    highlightAllFixtures( false );
    highlightAllAnimations( false );

    scene_tile_panel.empty();
    active_scene_id = 0;

    $.each( scenes, function ( index, scene ) {
        if (scene.number == 0 || current_act == 0 || scene.acts.indexOf(current_act) != -1) {
            var title = escapeForHTML(scene.name);
            scene_tile_panel.addTile(scene.id, scene.number, title, true, !scene.is_default, 0, 0, false );
            if (scene.is_running)
                markActiveScene(scene.id);
        }
    });

    setEditMode(edit_mode);         // Refresh editing icons on new tiles
}

// ----------------------------------------------------------------------------
// Called on scene added event 
function newSceneEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/scene/" + uid,
        cache: false,
        async: false,
        success: function (data) {
            var scene = new Scene( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < scenes.length; i++) {
                if (scenes[i].getId() == scene.getId() )
                    return;
                    
                if ( scenes[i].getNumber() > scene.getNumber() ) {
                    scenes.splice( i, 0, scene );
                    createSceneTiles( false );
                    return;
                }
            }

            scenes[scenes.length] = scene;
            
            toastNotice("Scene #" + scene.getNumber() + " " + scene.getName()+ " created" );
            createSceneTiles( false );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on scene changed event 
function changeSceneEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/scene/" + uid,
        cache: false,
        success: function (data) {
            var scene = new Scene( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < scenes.length; i++) {
                if ( scenes[i].getId() == scene.getId() ) {
                    scenes[i] = scene;

                    // Special processing for default scene
                    if ( scene.isDefault() ) {
                        updateCapturedFixtures( scene.getActorIds() );
                        updateCapturedAnimations( scene.getAnimationRefs() );
                    }

                    scene_tile_panel.updateTile( scene.id, scene.number, escapeForHTML(scene.name), 0, 0 );
                    if (scene.is_running)
                        markActiveScene(scene.id);

                    if (scene_update_listener)
                        scene_update_listener( scene.getId() );

                    break;
                }
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on scene delete event
function deleteSceneEvent( uid ) {
    for (var i = 0; i < scenes.length; i++) {
        if (scenes[i].getId() == uid) {
            scenes.splice( i, 1 );

            createSceneTiles( false );
            break;
        }
    }
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
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createScene(event) {
    stopEventPropagation(event);

    var fixture_list = filterFixtures( function( f ) { return f.isActive() && f.isGroup(); } );

    if ( fixture_list.length > 0 ) 
        questionBox( "Create New Scene", "Keep fixtures grouped?", createScene2 )
    else
        createScene2( false );
}

function createScene2( keepFixtureGroups ) {
    var new_number = getUnusedSceneNumber();

    var actors = [];
    var actor_ids = [];

    var fixture_list = getActiveFixtures();
    for ( var i=0; i < fixture_list.length; i++ ) {
        var fixture = fixture_list[i];

        if ( fixture.isGroup() && !keepFixtureGroups ) {
            var fixture_ids = fixture.getFixtureIds();
            for ( var j=0; j < fixture_ids.length; j++ ) {
                var f2 = getFixtureById( fixture_ids[j] );
                if ( f2 != null ) {
                    actors.push( make_actor( f2, false, fixture ) );
                    actor_ids.push( f2.getId() );
                }
            }
        }
        else {
            actors.push( make_actor( fixture, false, null ) );
            actor_ids.push( fixture.getId() );
        }
    }

    var animation_refs = [];
    
    $.each( animations, function( index, animation ) {
        if ( animation.isActive() ) {
            animation_refs[ animation_refs.length ] = {
                id: animation.id,
                actors: actor_ids
            };
        }
    } );

    openNewSceneDialog("New Scene", "Create Scene", false, {
        id: 0,
        name: "New Scene " + new_number,
        description: "",
        number: new_number,
        bpm_rating: 0,
        animation_refs: animation_refs,
        acts: current_act == 0 ? [] : [ current_act ],
        actors: actors
    } );
}

// ----------------------------------------------------------------------------
//
function copyScene(event) {
    stopEventPropagation(event);

    var scene = getSceneById(active_scene_id);
    if (scene == null)
        return;

    var new_number = getUnusedSceneNumber();

    openNewSceneDialog("Copy Scene " + scene.getName(), "Create Scene", true, {
        id: scene.getId(),
        name: "Copy of " + scene.getName(),
        description: scene.getDescription(),
        number: new_number,
        bpm_rating: scene.getBPMRating(),
        animation_refs: scene.getAnimationRefs(),
        acts: scene.getActs(),
        actors: scene.getActors()
    });
}

// ----------------------------------------------------------------------------
//
function editScene(event, scene_id) {
    stopEventPropagation(event);

    var scene = getSceneById(scene_id);
    if (scene == null)
        return;

    openNewSceneDialog("Edit Scene " + scene.getName(), "Update Scene", false, {
        id: scene.getId(),
        name: scene.getName(),
        description: scene.getDescription(),
        number: scene.getNumber(),
        bpm_rating: scene.getBPMRating(),
        animation_refs: scene.getAnimationRefs(),
        actors: scene.getActors(),
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
            actors: $("#nsd_fixture_container").data( "select-list" ).getSelected(),
            acts: acts,
            animation_refs: $("#nsd_animations").data("animation_refs")
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
            },
            error: onAjaxError
        });

        $("#new_scene_dialog").dialog("close");
    };

    var dialog_buttons =[];

    if (data.id != 0 && !copy) {
        dialog_buttons[dialog_buttons.length] = {
            text: "Make Copy",
            click: function () { send_update(true); }
        };
    }

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
        width: 740,
        height: 700,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_scene_dialog").dialog( "option", "title", dialog_title );

    $("#nsd_accordion").accordion({ heightStyle: "fill" });

    if (data.id == 0)
        $("#nsd_accordion").accordion({ active: 0 });
    else if ( data.number == 0 )
        $("#nsd_accordion").accordion({ active: 1 });

    // Populate ACTS
    $("#nsd_acts").empty();

    for (var i=1; i <= 20; i++) {
        $("#nsd_acts").append($('<option>', {
            value: i,
            text: i,
            selected: data.acts.indexOf( i ) > -1
        }));
    }
    
    $("#nsd_acts").multiselect({
        minWidth: 300, multiple: true, noneSelectedText: 'default act',
        checkAllText: 'All acts', uncheckAllText: 'Clear acts', selectedList: 8
    });

    // Populate BPMs
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

    $("#nsd_number").spinner({ min: 1, max: 100000 }).val (data.number );
    $("#nsd_name").val(data.name);
    $("#nsd_description").val(data.description);
    $("#nsd_copy_captured_fixtures").attr('checked', data.copy_captured_fixtures);

    // If default scene 0, disable controls
    $("#nsd_number").spinner( data.number == 0 ? "disable" : "enable" );
    $("#nsd_bpm_rating").multiselect( data.number == 0 ? "disable" : "enable" );
    $("#nsd_acts").multiselect( data.number == 0 ? "disable" : "enable" );
    $("#nsd_name").prop( 'disabled', data.number == 0 );
    $("#nsd_description").prop( 'disabled', data.number == 0 );
    $("#nsd_copy_captured_fixtures").prop( 'disabled', data.number == 0 );

    $("#nsd_fixtures").empty();

    // Add labels to all actors
    var actors = jQuery.extend(true, [], data.actors);
    for ( var i=0; i < actors.length; i++ )
        actors[i].label = getFixtureById( actors[i].id ).getLabel();

    // Populate FIXTURES
    new SelectList( { 
        name: "fixture",
        width: 620,
        container: $("#nsd_fixture_container"),
        selected: actors,
        render_template: $("#nsd_scene_template")[0].innerHTML,
        render_callback: null,
        update_callback: null,

        source_callback: function ( select_control ) {
            var actors = select_control.getSelected();

            var fixture_list = filterFixtures( function( fixture ) { 
                for (var actor_num=0; actor_num < actors.length; actor_num++)
                    if ( actors[actor_num].id == fixture.getId() )
                        return false;

                return true; 
            } );

            var items = [];
            for (var i=0; i < fixture_list.length; i++ )
                items.push( { id: fixture_list[i].getId(), label: fixture_list[i].getLabel() } );
            return items;
        },

        add_callback: function ( select_control, item_id ) {
            return make_actor( getFixtureById( item_id ), true, null );
        },
        
        select_callback: function ( select_control, index, item ) {
            openSceneActorDialog( item );
        },

        remove_callback: function ( select_control ) {
            // After fixtures are changed, remove any animation actors that have been removed
            verifyAnimationActors( select_control.getSelected() );
        }
    } );

    // Populate ANIMATIONS
    $("#nsd_animations").data("animation_refs", jQuery.extend(true, [], data.animation_refs));  // Generate a deep copy
    $("#nsd_animations").data("scene", data.id);

    populate_animations();

    $("#nsd_new_animation_button").hover(function () {
            $(this).addClass('ui-state-hover');
        },
        function () {
            $(this).removeClass('ui-state-hover');
    });

    if ( animations.length != 0 ) {
        $("#nsd_new_animation_button").unbind("click").bind("click", function (event) {
            var animation_refs = $("#nsd_animations").data("animation_refs");

            for ( var i=0; i < animation_refs.length; i++ )
                animation_refs[i].edit = false;

            animation_refs[animation_refs.length] = { 
                id: animations[0].getId(), 
                actors: $("#nsd_fixtures").val(), 
                edit: true };

            populate_animations();
        });

        $("#nsd_new_animation_button").show();
    }
    else
        $("#nsd_new_animation_button").hide();

    $("#new_scene_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function make_actor( target_fixture, load_default_values, source_fixture ) {
    var scene_actor = {
        "id": target_fixture.getId(),
        "label": target_fixture.getLabel(),
        "is_group": target_fixture.isGroup(),
        "palette_refs": [],
        "channels": []
    };

    if ( source_fixture == null )
        source_fixture = target_fixture;

    var channels = source_fixture.getChannels();
    if ( channels != null ) {
        for ( var j=0; j < channels.length; j++ ) {
            var ch = channels[j];

            scene_actor.channels.push( {
                "channel": ch.channel,
                "name": ch.name,
                "value": load_default_values ? ch.default_value : ch.value
            });
        }
    }

    var default_scene_actor = getDefaultSceneActor( source_fixture.getId() );
    if ( default_scene_actor != null ) {
        for ( var j=0; j < default_scene_actor.palette_refs.length; j++ )
            scene_actor.palette_refs.push( default_scene_actor.palette_refs[j] );
    }

    return scene_actor;
}

// ----------------------------------------------------------------------------
//
function populate_animations() {
    $("#nsd_add_animation").empty();

    $("#nsd_add_animation").append($('<option>', {
            value: 0,
            text: "add animation",
            selected: true
        }));

    $("#nsd_add_animation").append($('<option>', {
            value: -1,
            text: "create new animation...",
            selected: false
        }));

    var all_animations = filterAnimations( function( x ) { return x.isShared(); } );

    for (var i=0; i < all_animations.length; i++) {
         $("#nsd_add_animation").append($('<option>', {
            value: all_animations[i].getId(),
            text: all_animations[i].getNumber() + ": " + all_animations[i].getName(),
            selected: false
        }));
    }

    $("#nsd_add_animation").multiselect({ minWidth: 620, multiple: false,  selectedList: 1, header: false, height: 400, classes: 'select_list'});

    $("#nsd_add_animation").unbind("multiselectclick").bind("multiselectclick", function ( event, ui ) {
        stopEventPropagation(event);

        if ( ui.value == 0 )
            return;

        var actors = $("#nsd_fixture_container").data( "select-list" ).getSelected();
        var actor_ids = [];

        for (var actor_num=0; actor_num < actors.length; actor_num++)
            actor_ids.push( actors[actor_num].id );

        var animation_refs = $("#nsd_animations").data("animation_refs");

        if ( ui.value == -1 ) {
            var new_name = "S"+ parseInt($("#nsd_number").val()) + " A" + (animation_refs.length+1);

            var reference_fixture_id = 0;

            for ( var i=0; reference_fixture_id == 0 && i < actor_ids.length; i++ ) {
                var fixture = getFixtureById( actor_ids[i] );
                if ( fixture != null ) {
                    if ( !fixture.isGroup() )
                        reference_fixture_id = fixture.getId();
                    else if ( fixture.getFixtureIds().length > 0 )
                        reference_fixture_id = fixture.getFixtureIds()[0]
                }
            }

            createAnimation2( false, 0, new_name, reference_fixture_id, function( animation_id ) {
                animation_refs[ animation_refs.length ] = {
                    id: animation_id,
                    actors: actor_ids
                };                

                populate_animations();
            } );

            $("#nsd_add_animation").multiselect("uncheckAll");
            $("#nsd_add_animation").multiselect("refresh");
            
            return;
        }

        var animation_id = parseInt(ui.value);

        animation_refs[ animation_refs.length ] = {
            id: animation_id,
            actors: actor_ids
        };

        populate_animations();
    });

    // Rebuild animation tiles
    var edit_animation_template = $("#nsd_edit_animation_template")[0].innerHTML;

    $("#nsd_animations").empty();

    var animation_refs = $("#nsd_animations").data("animation_refs");

    for (var a = 0; a < animation_refs.length; a++) {
        var animation_num = a + 1;

        var animation = getAnimationById( animation_refs[a].id );
        var anim_info = findAnimation(animation);

        $("#nsd_animations").append( edit_animation_template.replace(/NNN/g, animation_num) );

        // Add common header info
        var number = $("#nsd_animation_" + animation_num + "_number" );
        var name = $("#nsd_animation_" + animation_num + "_name" );
        var type = $("#nsd_animation_" + animation_num + "_type" );
        var synopsis = $("#nsd_animation_" + animation_num + "_synopsis" );
        var fixtures_select = $("#nsd_animation_" + animation_num + "_fixture_select" );
        var fixture_order = $("#nsd_animation_" + animation_num + "_fixture_order" );
        var edit_button = $("#nsd_animation_" + animation_num + "_edit_button" );

        if ( animation.is_shared )
            number.html( animation.number + ":&nbsp;" );

        name.html(animation.name);
        type.html( anim_info.description);
        synopsis.html( getAnimationSynopsis(animation) );
        
        edit_button.data( "animation_id", animation.getId() );

        edit_button.click( function( event ) {
            editAnimation2( event, $(this).data("animation_id"), false, populate_animations );
        });

        if ( !animation_refs[a].edit ) {
            fixture_order.show();
            fixtures_select.hide();

            var found = false;
            var fixture_text = "";
            var actors = animation_refs[a].actors;

            if ( actors.length > 0) {
                for (var i=0; i < actors.length; i++) {
                    if (i > 0)
                        fixture_text += ", ";

                    var fixture = getFixtureById( actors[i] );
                    if ( fixture != null ) {
                        fixture_text += (fixture.isGroup() ? "G" : "F") + fixture.getNumber();
                        found = true;
                    }
                }
            }
    
            if ( !found )
                fixture_text = "NO FIXTURES";

            fixture_order.html( fixture_text );
        }
        else {
            fixtures_select.show();
            fixture_order.show();

            populateSceneFixtures( fixtures_select, animation_num, animation_refs[a] );
            populateFixtureOrder( fixtures_select, fixture_order, animation_refs[a] );
        }

        buttonClickBehavior( animation_num, true );
    }
}

// ----------------------------------------------------------------------------
//
function buttonClickBehavior( animation_num, allow_clicks ) {
    var button =  $( "#esa_" + animation_num + "_button" );

    button.unbind( "click" );

    if ( allow_clicks ) {
         setTimeout( function () { 
            button.on( "click", function ( event ) { scene_edit_animation( event, animation_num, false ) } ) 
         }, 500 );
    }
}

// ----------------------------------------------------------------------------
// Populate fixture select with currently selected scene fixtures
function populateSceneFixtures(multiselect, animation_num, animation_ref ) {
    multiselect.empty();

    var actors = $("#nsd_fixture_container").data( "select-list" ).getSelected(); // Get the currently selected set of fixtures

    if (actors != null) {
        // Fill in the scene's fixures
        for (var i = 0; i < actors.length; i++) {
            var fixture_id = actors[i].id;
            var fixture = getFixtureById(fixture_id);
            var is_selected = false;

            for (var j = 0; j < animation_ref.actors.length; j++)
                if (animation_ref.actors[j] == fixture_id) {
                    is_selected = true;
                    break;
                }

            multiselect.append($('<option>', {
                value: fixture_id,
                text: fixture.getLabel(),
                selected: is_selected
            }));
        }
    }

    multiselect.multiselect({ 
        minWidth: 500, 
        multiple: true, 
        noneSelectedText: 'select animation fixtures', 
        classes: 'scene_animation_fixture_button',
        selectedList: 1,
        open: function( event, ui ) { buttonClickBehavior( animation_num, false ); },
        close: function( event, ui ) { buttonClickBehavior( animation_num, true ); }        
    });
}

// ----------------------------------------------------------------------------
// 
function generateFixtureTiles(order_div, animation_ref) {
    order_div.empty();
    var ul = $("<ul class='sortable_fixtures'>");
    for (var i = 0; i < animation_ref.actors.length; i++) {
        var fixture = getFixtureById(animation_ref.actors[i]);
        if (fixture == null)
            continue;

        var label = (fixture.isGroup()) ? "G" : "F";

        var li = $("<li class='ui-state-default sortable_fixture'>" + label + fixture.getNumber() + "</li>");
        li.attr('title', fixture.getFullName());
        li.data('fixture_id', fixture.getId());
        ul.append(li);
    }
    order_div.append(ul);

    order_div.find(".sortable_fixtures").sortable( {
        "stop": function() {
            animation_ref.actors = [];

            // Push existing actors in order
            $.each(order_div.find("li"), function (key, value) {
                animation_ref.actors.push( $(value).data('fixture_id') );
            });
        }
    }).disableSelection();
}

// ----------------------------------------------------------------------------
// Populate fixture select with currently selected scene fixtures
function populateFixtureOrder(fixture_multiselect, order_div, animation_ref) {
    fixture_multiselect.unbind("multiselectclose");

    fixture_multiselect.bind("multiselectclose", function (event, ui) {
        stopEventPropagation(event);
        var fixture_ids = fixture_multiselect.val();

        if (fixture_ids == null) {
            animation_ref.actors = [];
        }
        else if (animation_ref.actors.length == 0) {
            animation_ref.actors = fixture_ids;
        }
        else {
            animation_ref.actors = [];

            // Push existing actors in order
            $.each(order_div.find("li"), function (key, value) {
                var fixture_id = $(value).data('fixture_id');

                for (var j=0; j < fixture_ids.length; j++) {
                    if (fixture_id == fixture_ids[j]) {
                        fixture_ids.splice(j, 1);
                        animation_ref.actors.push(fixture_id);
                        break;
                    }
                }
            });

            // Add new actors to the end
            $.each(fixture_ids, function (key, value) {
                animation_ref.actors.push(parseInt(value));
            });
        }

        generateFixtureTiles(order_div, animation_ref);
    });

    generateFixtureTiles(order_div, animation_ref);
}

// ----------------------------------------------------------------------------
//
function verifyAnimationActors( actors ) {
    var animations = $("#nsd_animations").data("animation_refs");
    var changed = false;

    function containsFixture( value ) {
        for (var k=0; k < actors.length; k++)     // Don't use "in" as actor ID type can be either number or string
            if (actors[k].id == value)
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
function scene_remove_animation(event, anim_num) {
    stopEventPropagation(event);

    var animations = $("#nsd_animations").data("animation_refs");
    animations.splice(anim_num - 1, 1);
    populate_animations();
}

// ----------------------------------------------------------------------------
//
function scene_edit_animation(event, animation_num, edit) {
    stopEventPropagation(event);

    var animation_refs = $("#nsd_animations").data("animation_refs");

    animation_refs[animation_num - 1].edit = !animation_refs[animation_num - 1].edit;

    populate_animations();
}

// ----------------------------------------------------------------------------
//
function makeChannelInfoLine(channels, palettes, css_class, show_default_values ) {
    if (!channels.length)
        return "";

    var html = "<div class=\"" + css_class  + "\">";

    if ( palettes != null ) {
        for (var j=0; j < palettes.length; j++) {
            var palette = getPaletteById( palettes[j] );
            if ( palette == null )
                continue;

            html += "<div>";
            html += "Palette:  " + palette.getName();
            html += " </div>";
        }
    }

    for (var j = 0; j < channels.length; j++) {
        var channel = channels[j];

        html += "<div>";
        html += "Channel " + (channel.channel + 1) + ": " + escapeForHTML(channel.name) + " = ";
        html += show_default_values ? channel.default_value : channel.value;

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

            scene_update_listener = function ( uid ) {
                if ( uid == 0 || uid == scene_id )
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
        acts = "default";

    $("#describe_scene_number").html(scene.getNumber());
    $("#describe_scene_name").html(escapeForHTML(scene.getName()));
    $("#describe_scene_acts").html(acts);
    $("#describe_scene_fixture_count").html(scene.getActors().length);
    $("#describe_scene_animation_count").html(scene.getAnimationRefs().length);
    $("#describe_scene_description").html(scene.getDescription() != null ? escapeForHTML(scene.getDescription()) : "");
    $("#describe_scene_bpm_rating").html(escapeForHTML(BPMRatings[scene.getBPMRating()].name));

    if ( !scene.isDefault() )
        $("#describe_scene_number").unbind( "click" ).on('click', function (event) { sceneDescribeSelectAll(event, scene_id, true); });

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

    for (var i = 0; i < scene.getActors().length; i++) {
        var actor = scene.getActors()[i];
        var fixture = getFixtureById(actor.id);

        var channel_data = [];
        for (var j = 0; j < actor.channels.length; j++)
            channel_data.push(actor.channels[j].value);

        info += makeFixtureTitleLine(fixture);
        info += makeChannelInfoLine(actor.channels, actor.palette_refs, "describe_scene_channels", false );

        describe_data[describe_data.length] = { "fixture_id": actor.id, "channel_data": channel_data };
    }

    $("#describe_scene_fixtures").html(info);

    for (var i=0; i < describe_data.length; i++)
        update_describe_fixture_icon(describe_data[i].fixture_id);

    info = "";

    if (scene.getAnimationRefs().length > 0) {
        for (var i = 0; i < scene.getAnimationRefs().length; i++) {
            var animation_ref = scene.getAnimationRefs()[i];

            var fixtures = "";

            if (animation_ref.actors.length > 0) {
                for (var j = 0; j < animation_ref.actors.length; j++) {
                    if (j > 0)
                        fixtures += ", ";

                    var fixture = getFixtureById(animation_ref.actors[j]);

                    if (fixture.isGroup())
                        fixtures += "G";

                    fixtures += fixture.getNumber();
                }
            }
            else
                fixtures = "NONE";


            var animation = getAnimationById( animation_ref.id );
            if ( animation == null )
                continue;

            var signal = getSignalSynopsis(animation.signal);
            var synopsis = getAnimationSynopsis(animation);

            info += "<div style=\"clear:both; float: left; font-size: 10pt;\" >";

            if ( animation.isShared() )
                info += "Animation " + animation.getNumber() + ": " + animation.getName();
            else
                info += "Private animation: " + animation.getName();

            info += "</div>";

            info += "<div class=\"describe_scene_channels\">";
            info += "Type: " + getAnimationName(animation) + "<br />";

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
            if ( !getFixtureById(fixture_id).isActive() )
                captureFixture(fixture_id, describe_data[i].channel_data, null, false);
            else
                releaseFixture(fixture_id);

            update_describe_fixture_icon(fixture_id);
            break;
        }
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
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function markActiveScene(new_scene_id) {
    if (new_scene_id == active_scene_id)
        return;

    if (active_scene_id != 0) {
        getSceneById(active_scene_id).is_running = false;
        scene_tile_panel.selectTile(active_scene_id, false);
        highlightSceneFixtures(active_scene_id, false);
        highlightSceneAnimations(active_scene_id, false);
        active_scene_id = 0;
    }

    if (new_scene_id != 0) {
        var scene = getSceneById(new_scene_id);
        if ( scene == null )
            return;

        scene.is_running = true;
        scene_tile_panel.selectTile(new_scene_id, true);
        highlightSceneFixtures(new_scene_id, true);
        highlightSceneAnimations(new_scene_id, true);
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
    if (scene)
        highlightFixtures(scene.getActorIds(), highlight);
}

// ----------------------------------------------------------------------------
//
function highlightSceneAnimations(scene_id, highlight) {
    if (scene_id == 0)
        return;

    var scene = getSceneById(scene_id);
    if (scene )
        highlightAnimations(scene.getAnimationRefs(), highlight);
}

// ----------------------------------------------------------------------------
//

var scene_slider_panel = null;

function openSceneActorDialog( scene_actor ) {
    var fixture = getFixtureById( scene_actor.id );
    if ( fixture == null )
        return;

    var channels = fixture.getChannels();

    scene_slider_panel = new SliderPanel( "esa_slider_pane", channels.length, false );

    $("#edit_scene_actor_dialog").dialog({
        title: "Edit Scene Actor " + fixture.getLabel(),
        autoOpen: false,
        width: 1000,
        height: 560,
        modal: true,
        resizable: false,
        buttons: {
            "OK": function() {
                new_values = scene_slider_panel.getChannelValues( fixture.getId() );
                for ( var i=0; i < new_values.length; i++ )
                    scene_actor.channels[i].value = new_values[i];

                scene_actor.palette_refs = $("#esa_palette_container").data( "select-list" ).getSelectedIds();

                releaseFixture( fixture.getId() );

                $(this).dialog("close");
             },

             "Cancel": function() {
                releaseFixture( fixture.getId() );

                $(this).dialog("close");
             }
        }
    });

    var channels_to_load = [];
    var channel_values = [];

    // Generate labels and values
    for (var i = 0; i < channels.length; i++) {
        var channel = channels[i];

        channel.label = ((fixture.isGroup()) ? "G" : "") + fixture.getNumber() + "-" + (i + 1);
        channel.max_value = 255;
        channel.value = (scene_actor.channels != null && scene_actor.channels.length > i) ? scene_actor.channels[i].value : 0;
        channel.linkable = true;
        channels_to_load.push(channel);
        channel_values.push( channel.value );
    }

    var fixture_slider_callback = function(fixture_id, action, channel, value) {
        if ( action == "channel" ) {
            var json = [{
                "actor_id": fixture_id,
                "channel": channel,
                "value": value,
                "capture": false
            }];

            $.ajax({
                type: "POST",
                url: "/dmxstudio/rest/control/fixture/channel/",
                data: JSON.stringify( { "channel_data" : json } ),
                contentType: 'application/json',
                async: false,   // These cannot get out of sync
                cache: false,
                error: onAjaxError
            });
        }
        else if ( action == "side-text" ) {
            var fixture = getFixtureById( fixture_id );

            if ( fixture.getChannels()[channel].ranges.length > 0 )
                showFixtureRanges( fixture_id, channel, "setSceneChannelValue" );
        }
    }

    scene_slider_panel.allocateChannels( fixture.getId(), fixture.getFullName(), channels_to_load, fixture_slider_callback );

    var palettes = [];
    for ( var i=0; i < scene_actor.palette_refs.length; i++ ) {
        var p = getPaletteById( scene_actor.palette_refs[i] );
        if ( p != null )
            palettes.push( { id: p.getId(), label: p.getName() })
    }

    var update_palettes = function() {
        var json = [{
            "actor_id": fixture.getId(),
            "palette_refs": $("#esa_palette_container").data( "select-list" ).getSelectedIds(),
            "capture": false
        }];

        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/control/fixture/palettes/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            async: true,
            cache: false,
            error: onAjaxError
        });
    }

    // Populate PALETTES
    new SelectList( { 
        name: "palette",
        container: $("#esa_palette_container"),
        selected: palettes,
        render_template: $("#esa_palette_template")[0].innerHTML,
        width: 300,
        render_callback: null,
        select_callback: null,
        remove_callback: update_palettes,
        update_callback: update_palettes,

        add_callback: function ( select_control, item_id ) {
            return { id: item_id, label: getPaletteById( item_id ).getName() };
        },

        source_callback: function ( select_control ) {
            var palettes = select_control.getSelected();

            var palette_list = filterPalettes( function( palette ) { 
                for (var palette_num=0; palette_num < palettes.length; palette_num++)
                    if ( palettes[palette_num].id == palette.getId() )
                        return false;

                return true; 
            } );

            var items = [];
            for (var i=0; i < palette_list.length; i++ )
                items.push( { id: palette_list[i].getId(), label: palette_list[i].getName() } );
            return items;
        }
    } );

    captureFixture( fixture.getId(), channel_values, $("#esa_palette_container").data( "select-list" ).getSelectedIds(), false );

    $("#edit_scene_actor_dialog").dialog("open");

    // Unclear why this is needed - but if not set to visible, this is set to hidden and we loose channel range descriptions
    $("#esa_slider_pane").css( "overflow", "visible" );
}

// ----------------------------------------------------------------------------
//
function setSceneChannelValue( event, fixture_id, channel_id, value ) {
    stopEventPropagation( event );

    var slider = scene_slider_panel.findChannel( fixture_id, channel_id );
    if ( slider != null )
        slider.setValue( value );
}