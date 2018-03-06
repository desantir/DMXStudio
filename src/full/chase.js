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

var ChaseStepTriggerType = {
    CT_TIMER: 1,
    CT_MANUAL: 2
};

var chases = [];

var SCENE_LOAD_METHODS = [ "", "Load", "Add", "Remove" ];

// ----------------------------------------------------------------------------
// class Chase
//
function Chase(chase_data)
{
    // Constructor
    jQuery.extend(this, chase_data);
}

// method getId
Chase.prototype.getId = function () {
    return this.id;
}

// method getNumber
Chase.prototype.getNumber = function () {
    return this.number;
}

// method getName
Chase.prototype.getName = function () {
    return this.name;
}

// method getCreated
Chase.prototype.getCreated = function () {
    return this.created;
}

// method getFullName
Chase.prototype.getFullName = function () {
    return this.name;
}

// method getDescription
Chase.prototype.getDescription = function () {
    return this.description;
}

// method getUserTypeName
Chase.prototype.getUserTypeName = function () {
    return "Chase";
}

// method getFadeMS
Chase.prototype.getFadeMS = function () {
    return this.fade_ms;
}

// method getDelayMS
Chase.prototype.getDelayMS = function () {
    return this.delay_ms;
}

// method getSteps
Chase.prototype.getSteps = function () {
    return this.steps;
}

// method getStepTrigger
Chase.prototype.getStepTrigger = function() {
    return this.step_trigger;
}

// method isActive
Chase.prototype.isActive = function () {
    return chase_tile_panel.isActive(this.id);
}

// method getActs
Chase.prototype.getActs = function () {
    return this.acts;
}

// method isRepeat
Chase.prototype.isRepeat = function () {
    return this.repeat;
}

// ----------------------------------------------------------------------------
//
function getChaseById(id) {
    for (var i = 0; i < chases.length; i++)
        if (chases[i].getId() == id)
            return chases[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getChaseByNumber(number) {
    for (var i = 0; i < chases.length; i++)
        if (chases[i].getNumber() == number)
            return chases[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getChaseByName(name) {
    for (var i = 0; i < chases.length; i++)
        if (chases[i].getName() === name)
            return chases[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getActiveChase( ) {
    for (var i=0; i < chases.length; i++)
        if (chases[i].isActive() )
            return chases[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getUnusedChaseNumber() {
    return getNextUnusedNumber( chases );
}

// ----------------------------------------------------------------------------
//
function updateChases() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/chases/",
        cache: false,
        success: function (data) {
            chases = [];
            var json = jQuery.parseJSON(data);
            $.map(json, function (chase, index) {
                chases.push(new Chase(chase));
            });
            createChaseTiles( true );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createChaseTiles( no_wait ) {
    if ( no_wait )
        _createChaseTiles();
    else
        delayUpdate( "chaseTiles", 1,  _createChaseTiles );
}

function _createChaseTiles() {
    chase_tile_panel.empty();
    active_chase_id = 0;

    $("#chaseAdvance").hide();
    $("#chaseBack").hide();

    $.each( chases, function ( index, chase ) {
        if (current_act == 0 || chase.acts.indexOf(current_act) != -1) {
            chase_tile_panel.addTile(chase.id, chase.number, escapeForHTML(chase.name), true, true, 0, 0, false );
            if (chase.is_running)
                markActiveChase(chase.id);
        }
    });

    setEditMode(edit_mode);         // Refresh editing icons on new tiles
}

// ----------------------------------------------------------------------------
// Called on chase added event 
function newChaseEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/chase/" + uid,
        cache: false,
        async: false,
        success: function (data) {
            var chase = new Chase( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < chases.length; i++) {
                if (chases[i].getId() == chase.getId() )
                    return;
                    
                if ( chases[i].getNumber() > chase.getNumber() ) {
                    chases.splice( i, 0, chase );
                    createChaseTiles( false );
                    return;
                }
            }

            chases[chases.length] = chase;

            toastNotice("Chase #" + chase.getNumber() + " " + chase.getName()+ " created" );
            createChaseTiles( false );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on chase changed event 
function changeChaseEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/chase/" + uid,
        cache: false,
        success: function (data) {
            var chase = new Chase( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < chases.length; i++) {
                if ( chases[i].getId() == chase.getId() ) {
                    chases[i] = chase;

                    chase_tile_panel.updateTile( chase.id, chase.number, escapeForHTML(chase.name), 0, 0 );
                    if (chase.is_running)
                        markActiveChase(chase.id);

                    break;
                }
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on chase delete event
function deleteChaseEvent( uid ) {
    for (var i = 0; i < chases.length; i++) {
        if (chases[i].getId() == uid) {
            chases.splice( i, 1 );
            createChaseTiles( false );
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
function playChase(event, chase_id) {
    stopEventPropagation(event);

    if (chase_id == active_chase_id)
        chase_id = 0;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/chase/show/" + chase_id,
        cache: false,
        success: function () {
            // EVENT
            // markActiveChase(chase_id);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function chaseStep( event, steps ) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/chase/step/" + steps,
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createChase(event) {
    stopEventPropagation(event);

    var new_number = getUnusedChaseNumber();

    openNewChaseDialog("New Chase", {
        id: 0,
        name: "New Chase " + new_number,
        description: "",
        number: new_number,
        fade_ms: 0,
        delay_ms: 1000,
        steps: [{ id: 0, delay_ms: 0 }],
        acts: current_act == 0 ? [] : [current_act],
        repeat: true,
        step_trigger: ChaseStepTriggerType.CT_TIMER,
        callback: null
    });
}

// ----------------------------------------------------------------------------
//
function editChase(event, chase_id) {
    stopEventPropagation(event);

    var chase = getChaseById(chase_id);
    if (chase == null)
        return;

    openNewChaseDialog("Update Chase " + chase.getNumber(), {
        id: chase.getId(),
        name: chase.getName(),
        description: chase.getDescription(),
        number: chase.getNumber(),
        fade_ms: chase.getFadeMS(),
        delay_ms: chase.getDelayMS(),
        steps: chase.getSteps().slice(),
        acts: chase.getActs(),
        repeat: chase.isRepeat(), 
        step_trigger: chase.getStepTrigger(),
        callback: null
    });
}

// ----------------------------------------------------------------------------
//
function openNewChaseDialog(dialog_title, data) {
    var send_update = function (make_copy) {
        var steps = reload_steps_data();
        if (steps.length == 0) {
            messageBox("No scene steps have been selected");
            return;
        }

        for (var s = 0; s < steps.length; s++) {
            if (steps[s].id == 0) {
                messageBox("Step " + (s + 1) + " does not have a selected scene");
                return;
            }
        }

        var acts = $("#ncd_acts").val();
        if (acts == null)
            acts = [];

        var json = {
            id: data.id,
            name: $("#ncd_name").val(),
            description: $("#ncd_description").val(),
            number: parseInt($("#ncd_number").val()),
            fade_ms: $("#ncd_fade_ms").val(),
            delay_ms: $("#ncd_delay_ms").val(),
            steps: steps,
            acts: acts,
            repeat: $("#ncd_repeat").is(":checked"),
            step_trigger:  $("#ncd_step_trigger").val()
        };

        if ((make_copy || json.number != data.number) && getChaseByNumber(json.number) != null) {
            messageBox("Chase number " + json.number + " is already in use");
            return;
        }

        var action;
        if (make_copy)
            action = "copy";
        else if (json.id == 0)
            action = "create";
        else
            action = "update";

        var action_callback = data.callback;

        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/edit/chase/" + action + "/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            success: function (data)  {
                if ( action_callback != null ) {
                    var response = JSON.parse( data );
                    action_callback( response.id );
                }

            },
            error: onAjaxError
        });

        $("#new_chase_dialog").dialog("close");
    };

    var dialog_buttons = [];

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
            $("#new_chase_dialog").dialog("close");
        }
    };

    $("#new_chase_dialog").dialog({
        autoOpen: false,
        width: 800,
        height: 680,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_chase_dialog").dialog("option", "title", dialog_title );

    $("#ncd_accordion").accordion({ heightStyle: "fill" });

    if (data.id == 0)
        $("#ncd_accordion").accordion({ active: 0 });

    $("#ncd_number").spinner({ min: 1, max: 100000 }).val(data.number);
    $("#ncd_name").val(data.name);
    $("#ncd_description").val(data.description);
    $("#ncd_fade_ms").spinner({ min: 0, max: 100000 }).val(data.fade_ms);
    $("#ncd_delay_ms").spinner({ min: 0, max: 100000 }).val(data.delay_ms);
    $("#ncd_repeat").attr('checked', data.repeat);

    $("#ncd_acts").empty();

    for (var i = 1; i <= 20; i++) {
        $("#ncd_acts").append($('<option>', {
            value: i,
            text: i,
            selected: data.acts.indexOf(i) > -1
        }));
    }

    $("#ncd_acts").multiselect({
        minWidth: 300, multiple: true, noneSelectedText: 'default act',
        checkAllText: 'All acts', uncheckAllText: 'Clear acts', selectedList: 8
    });

    $("#ncd_step_trigger").multiselect({ multiple: false, noneSelectedText: 'default act',  selectedList: 1, minWidth: 300 });
    
    multiselectSelect( $("#ncd_step_trigger"), data.step_trigger );

    function showDuration( trigger ) {
        if ( trigger == ChaseStepTriggerType.CT_TIMER )
            $("#ncd_delay_div").show();
        else
            $("#ncd_delay_div").hide();

        populate_chase_steps( trigger );
    }

    $("#ncd_step_trigger").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        showDuration( parseInt(ui.value) );
    });

    $("#ncd_steps").data("steps", jQuery.extend(true, [], data.steps));

    showDuration( data.step_trigger );

    $("#new_chase_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function populate_chase_steps( trigger ) {
    var show_duration = trigger == ChaseStepTriggerType.CT_TIMER;

    $("#ncd_steps").empty();

    var steps = $("#ncd_steps").data("steps");

    var template = $("#ncd_step_template")[0].innerHTML;

    for (var s = 0; s < steps.length; s++) {
        var step_num = s + 1;

        var step_div_text = template.replace(/NNN/g, step_num);

        $("#ncd_steps").append(step_div_text);
        $("#ncd_step_" + step_num + "_delay_ms").spinner({ min: 0, max: 100000 }).val(steps[s].delay_ms);
        $("#ncd_step_" + step_num + "_remove").css("display", steps.length == 1 ? "none" : "inline");
        $("#ncd_step_" + step_num + "_delay_div").css("display", !show_duration ? "none" : "inline");

        // Adding "unselected" option since multiselect is forcing a selection to the first item when none are selected
        $("#ncd_step_" + step_num + "_scene").append($('<option>', {
            value: 0,
            text: "Select Scene",
            selected: false 
        }));

        // Fill in scenes
        for (var i = 0; i < scenes.length; i++) {
            $("#ncd_step_" + step_num + "_scene").append($('<option>', {
                value: scenes[i].getId(),
                text: scenes[i].getNumber() + ": " + scenes[i].getFullName(),
                selected: scenes[i].getId() == steps[s].id
            }));
        }

        // If we don't do this after population, multiselect is empty (refresh may work)
        $("#ncd_step_" + step_num + "_scene").multiselect({
            minWidth: 380,
            multiple: false,
            selectedList: 1,
            header: false,
            classes: 'small_multilist'
        });

        $("#ncd_step_" + step_num + "_method_div").buttonset();
        $("#ncd_step_" + step_num + "_method_" + steps[s].load_method).prop("checked", true).button("refresh");

        if ( step_num == 1 ) {
            $("#ncd_step_" + step_num + "_method_div").hide();
        }
    }
}

// ----------------------------------------------------------------------------
//
function reload_steps_data() {

    var steps = $("#ncd_steps").data("steps");

    for (var s = 0; s < steps.length; s++) {
        var step_num = s + 1;

        steps[s].id = $("#ncd_step_" + step_num + "_scene").val();
        steps[s].delay_ms = $("#ncd_step_" + step_num + "_delay_ms").val();
        steps[s].load_method = $("input[name=ncd_step_" + step_num + "_method]:checked").val();
    }

    return steps;
}

// ----------------------------------------------------------------------------
//
function chase_insert_step(event, step_num) {
    stopEventPropagation(event);

    reload_steps_data().splice(step_num, 0, { id: 0, delay_ms: 0 });
    populate_chase_steps( parseInt( $("#ncd_step_trigger").val() ) );
}

// ----------------------------------------------------------------------------
//
function chase_remove_step(event, step_num) {
    stopEventPropagation(event);

    reload_steps_data().splice(step_num-1, 1);
    populate_chase_steps( parseInt( $("#ncd_step_trigger").val() ) );
}

// ----------------------------------------------------------------------------
//
function deleteChase(event, chase_id) {
    stopEventPropagation(event);

    deleteVenueItem(getChaseById(chase_id), function (item) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/delete/chase/" + item.getId(),
            cache: false,
            success: function () {
                // EVENT
                //updateChases
            },
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function describeChase(event, chase_id) {
    stopEventPropagation(event);

    var chase = getChaseById(chase_id);
    if (chase == null) {
        messageBox("No definition for chase " + chase_id + " in client");
        return;
    }

    $("#describe_chase_dialog").dialog({
        autoOpen: false,
        width: 600,
        height: 500,
        modal: false,
        draggable: true,
        resizable: false,
        title: "Chase " + chase.getNumber() + ": " + escapeForHTML(chase.getName())
    });

    var acts = "";
    if (chase.getActs().length > 0) {
        for (var i=0; i < chase.getActs().length; i++)
            acts += chase.getActs()[i] + " ";
    }
    else
        acts = "default";

    var show_duration = chase.getStepTrigger() == ChaseStepTriggerType.CT_TIMER;

    var delay_desc = show_duration ? chase.getDelayMS() + " ms" : "Manual Advance";

    $("#describe_chase_number").html(chase.getNumber());
    $("#describe_chase_name").html(escapeForHTML(chase.getName()));
    $("#describe_chase_acts").html(acts);
    $("#describe_chase_delay").html(delay_desc);
    $("#describe_chase_fade").html(chase.getFadeMS() + " ms");
    $("#describe_chase_description").html(chase.getDescription() != null ? escapeForHTML(chase.getDescription()) : "" );
    $("#describe_chase_repeat").html( chase.isRepeat() ? "Continuous" : "Once" );

    var info = "";
    for ( i=0; i < chase.getSteps().length; i++) {
        var step = chase.getSteps()[i];
        var scene = getSceneById(step.id);

        info += "<div style=\"clear:both; float: left; margin-right:6px;\" class=\"describe_chase_step\">Step " + (i + 1) + ": </div>";
        info += "<div style=\"float: left;\" >";
        info +=  SCENE_LOAD_METHODS[ step.load_method ];
        info += " scene " + scene.getNumber() + " " + escapeForHTML(scene.getName() ) + "</div>";

        if ( show_duration && step.delay_ms > 0)
            info += "<div style=\"float: left; margin-left: 8px;\" class=\"describe_chase_step\">(duration " + step.delay_ms + " ms)</div>";
    }

    $("#describe_chase_info").html(info);

    $("#describe_chase_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function markActiveChase(new_chase_id) {
    if (new_chase_id == active_chase_id)
        return;

    if (active_chase_id != 0) {
        getChaseById(active_chase_id).is_running = false;
        chase_tile_panel.selectTile(active_chase_id, false);
        active_chase_id = 0;
    }

    var show_advance_buttons = false;

    if (new_chase_id != 0) {
        var chase = getChaseById(new_chase_id);
        if ( chase == null )
            return;

        chase.is_running = true;
        chase_tile_panel.selectTile(new_chase_id, true);
        active_chase_id = new_chase_id;

        show_advance_buttons = ( chase.getStepTrigger() == ChaseStepTriggerType.CT_MANUAL );
    }

    if ( show_advance_buttons ) {
        $("#chaseAdvance").show();
        $("#chaseBack").show();
    }
    else {
        $("#chaseAdvance").hide();
        $("#chaseBack").hide();
    }
}
