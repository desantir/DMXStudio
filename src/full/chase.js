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

var chases = new Array();

// ----------------------------------------------------------------------------
// class Chase
//
function Chase(chase_data)
{
    // Constructor
    jQuery.extend(this, chase_data);

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

    // method getUserTypeName
    this.getUserTypeName = function () {
        return "Chase";
    }

    // method getFadeMS
    this.getFadeMS = function () {
        return this.fade_ms;
    }

    // method getDelayMS
    this.getDelayMS = function () {
        return this.delay_ms;
    }

    // method getSteps
    this.getSteps = function () {
        return this.steps;
    }

    // method isActive
    this.isActive = function () {
        return chase_tile_panel.isActive(this.id);
    }

    // method getActs
    this.getActs = function () {
        return this.acts;
    }
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
function getUnusedChaseNumber() {
    for ( var i=1; i < 50000; i++ )
        if ( getChaseByNumber(i) == null )
            return i;
    return 99999;
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
            createChaseTiles();
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createChaseTiles() {
    chase_tile_panel.empty();
    active_chase_id = 0;

    $.each( chases, function ( index, chase ) {
        if (current_act == 0 || chase.acts.indexOf(current_act) != -1) {
            chase_tile_panel.addTile(chase.id, chase.number, escapeForHTML(chase.name), true);
            if (chase.is_running)
                markActiveChase(chase.id);
        }
    });

    setEditMode(edit_mode);         // Refresh editing icons on new tiles
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
            markActiveChase(chase_id);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createChase(event) {
    stopEventPropagation(event);

    var new_number = getUnusedChaseNumber();

    openNewChaseDialog("Create Chase", {
        id: 0,
        name: "New Chase " + new_number,
        description: "",
        number: new_number,
        fade_ms: 0,
        delay_ms: 1000,
        steps: [{ id: 0, delay_ms: 0 }],
        acts: current_act == 0 ? [] : [current_act]
    });
}

// ----------------------------------------------------------------------------
//
function editChase(event, chase_id) {
    stopEventPropagation(event);

    var chase = getChaseById(chase_id);
    if (chase == null)
        return;

    openNewChaseDialog("Update Chase", {
        id: chase.getId(),
        name: chase.getName(),
        description: chase.getDescription(),
        number: chase.getNumber(),
        fade_ms: chase.getFadeMS(),
        delay_ms: chase.getDelayMS(),
        steps: chase.getSteps().slice(),
        acts: chase.getActs()
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

        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/edit/chase/" + action + "/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            cache: false,
            success: function () {
                updateChases();
            },
            error: onAjaxError
        });

        $("#new_chase_dialog").dialog("close");
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
            $("#new_chase_dialog").dialog("close");
        }
    };

    $("#new_chase_dialog").dialog({
        autoOpen: false,
        width: 780,
        height: 620,
        modal: true,
        resizable: false,
        buttons: dialog_buttons
    });

    $("#new_chase_dialog").dialog("option", "title", dialog_title + " " + data.number );

    $("#ncd_accordion").accordion({ heightStyle: "fill" });

    if (data.id == 0)
        $("#ncd_accordion").accordion({ active: 0 });

    $("#ncd_number").spinner({ min: 1, max: 100000 }).val(data.number);
    $("#ncd_name").val(data.name);
    $("#ncd_description").val(data.description);
    $("#ncd_fade_ms").spinner({ min: 0, max: 100000 }).val(data.fade_ms);
    $("#ncd_delay_ms").spinner({ min: 0, max: 100000 }).val(data.delay_ms);

    $("#ncd_acts").empty();

    for (var i = 1; i <= 20; i++) {
        $("#ncd_acts").append($('<option>', {
            value: i,
            text: i,
            selected: data.acts.indexOf(i) > -1
        }));
    }

    $("#ncd_acts").multiselect({
        minWidth: 300, multiple: true, noneSelectedText: 'None',
        checkAllText: 'All acts', uncheckAllText: 'Clear acts', selectedList: 8
    });

    $("#ncd_steps").data("steps", jQuery.extend(true, [], data.steps));

    populate_chase_steps();

    $("#new_chase_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function populate_chase_steps() {

    $("#ncd_steps").empty();

    var steps = $("#ncd_steps").data("steps");

    var template = $("#ncd_step_template")[0].innerHTML;

    for (var s = 0; s < steps.length; s++) {
        var step_num = s + 1;

        var step_div_text = template.replace(/NNN/g, step_num);

        $("#ncd_steps").append(step_div_text);
        $("#ncd_step_" + step_num + "_delay_ms").spinner({ min: 0, max: 100000 }).val(steps[s].delay_ms);
        $("#ncd_step_" + step_num + "_remove").css("display", steps.length == 1 ? "none" : "inline");

        // Adding "unselected" option since multiselect is forcing a selection to the first item when none are selected
        $("#ncd_step_" + step_num + "_scene").append($('<option>', {
            value: 0,
            text: "Select Scene",
            selected: false 
        }));

        // Fill in scenes
        for (var i = 0; i < scenes.length; i++) {
            if (!scenes[i].isDefault()) {
                $("#ncd_step_" + step_num + "_scene").append($('<option>', {
                    value: scenes[i].getId(),
                    text: scenes[i].getNumber() + ": " + scenes[i].getFullName(),
                    selected: scenes[i].getId() == steps[s].id
                }));
            }
        }

        // If we don't do this after population, multiselect is empty (refresh may work)
        $("#ncd_step_" + step_num + "_scene").multiselect({
            minWidth: 450,
            multiple: false,
            selectedList: 1,
            header: false
        });
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
    }

    return steps;
}

// ----------------------------------------------------------------------------
//
function chase_insert_step(event, step_num) {
    stopEventPropagation(event);

    reload_steps_data().splice(step_num, 0, { id: 0, delay_ms: 0 });
    populate_chase_steps();
}

// ----------------------------------------------------------------------------
//
function chase_remove_step(event, step_num) {
    stopEventPropagation(event);

    reload_steps_data().splice(step_num-1, 1);
    populate_chase_steps();
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
            success: updateChases,
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
        title: "Chase " + chase.getNumber() + ": " + escapeForHTML(chase.getName()),
        open: function () { // Stop main body scroll
            $("body").css("overflow", "hidden");
        },
        close: function () {
            $("body").css("overflow", "auto");
        }
    });

    var acts = "";
    if (chase.getActs().length > 0) {
        for (var i=0; i < chase.getActs().length; i++)
            acts += chase.getActs()[i] + " ";
    }
    else
        acts = "None";

    $("#describe_chase_number").html(chase.getNumber());
    $("#describe_chase_name").html(escapeForHTML(chase.getName()));
    $("#describe_chase_acts").html(acts);
    $("#describe_chase_delay").html(chase.getDelayMS() + " ms");
    $("#describe_chase_fade").html(chase.getFadeMS() + " ms");
    $("#describe_chase_description").html(chase.getDescription() != null ? escapeForHTML(chase.getDescription()) : "" );

    var info = "";
    for (var i = 0; i < chase.getSteps().length; i++) {
        var scene = getSceneById(chase.getSteps()[i].id);

        info += "<div style=\"clear:both; float: left; margin-right:6px;\" class=\"describe_chase_step\">Step " + (i + 1) + ": </div>";
        info += "<div style=\"float: left;\" >Scene " + scene.getNumber() + " " + escapeForHTML(scene.getName() ) + "</div>";

        if (chase.getSteps()[i].delay > 0)
            info += "<div style=\"float: left; margin-left: 8px;\" class=\"describe_chase_step\">(delay " + chase.getSteps()[i].delay + " ms)</div>";
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
        chase_tile_panel.selectTile(active_chase_id, false);
        active_chase_id = null;
    }

    if (new_chase_id != 0) {
        chase_tile_panel.selectTile(new_chase_id, true);
        active_chase_id = new_chase_id;
    }
}
