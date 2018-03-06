/* 
Copyright (C) 2013-2017 Robert DeSantis
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

var MAXIMUM_LEVEL = 255;

var SignalSourceType = {
    SOURCE_HIGH: 1,
    SOURCE_AMPLITUDE: 2,
    SOURCE_AVG_AMPLITUDE: 3,
    SOURCE_BEAT_INTENSITY: 4, 
    SOURCE_FREQ: 5, 
    SOURCE_VOLUME: 6,
    SOURCE_LOW: 7,
    SOURCE_SQUAREWAVE:8, 
    SOURCE_SAWTOOTH: 9, 
    SOURCE_SINEWAVE: 10,
    SOURCE_TRIANGLEWAVE: 11,
    SOURCE_STEPWAVE: 12,
    SOURCE_STEPWAVE_FULL: 13, 
    SOURCE_RANDOM: 14,
    SOURCE_SOUND: 15
}

var SpeedLevelType = {
    SPEED_NORMAL: 1,
    SPEED_FAST: 2,
    SPEED_SLOW: 3
}

var SignalTriggerType = {
    TRIGGER_TIMER: 1,
    TRIGGER_FREQ_BEAT: 2,
    TRIGGER_AMPLITUDE_BEAT: 3,
    TRIGGER_RANDOM_TIMER: 4
}

var SpeedLevel = [ "Off", "Level decreases timer MS", "Level increases timer MS"];
var SignalTrigger = ["Timer", "Frequency Beat", "Amplitude Beat", "Random Timer", ];
var SignalSource = ["Always High", "Amplitude", "Avg. Amplitude", "Amplitude Beat Intensity", "Frequency", "Volume", "Always Low", "Square Wave", 
                    "Sawtooth Wave", "Sinewave", "Triangle Wave", "Step Wave (half)", "Step Wave (full)", "Random Level", "Sound Detect" ];

var FadeWhat = ["Color channels", "Dimmer channels", "Color and Dimmer channels"];
var PatternDimmer = ["Simple sequence", "Cylon", "Pairs", "Toward center", "Alternate", "On/Off", "Random", "Ramp Up", "Ramp Up/Down", "Random Ramp Up"];
var AnimationStyle = ["Value List", "Value Range", "Scale Scene Value", "Signal Level"];
var FaderEffect = ["Color Change (single)", "Color Change (multiple)", "Strobe", "Color blend (single)", "Color blend (multiple)", "All effects" ];
var PixelEffect = [ "Scrolling", "Stacked", "Stacked Left", "Stacked Right", "Beam", "Random", "Chase", "Color Chase" ];
var FilterEffect = [ "Sine Wave", "Ramp Up", "Ramp Down", "Step Wave", "Random" ];
var PulseEffect = [ "Strobe" ];
var DimmerMode = [ "Ramp Up", "Ramp Down", "Breath", "Random" ];

var MOVEMENT_ANIMATOR_ROOT = "SceneMovementAnimator";
var NUM_MOVEMENT_ANIMATIONS = 8;

var AnimationEditors = [
    { name: "Color fader", class_name: "SceneColorSwitcher", synopsis: getSceneColorSwitcherSynopsis,
      populate: populateSceneColorSwitcher, update: updateSceneColorSwitcher, show_reference: false,
      description: "Change fixture colors using color progression and signal input trigger. Randomly chooses color switch, blend or strobe effects." },

    { name: "Strobe", class_name: "SceneStrobeAnimator", synopsis: getSceneStrobeAnimatorSynopsis,
      populate: populateSceneStrobeAnimator, update: updateSceneStrobeAnimator, show_reference: false,
      description: "Simulated and fixture strobe control" },
      
    { name: "Fixture dimmer", class_name: "SceneFixtureDimmer", synopsis: null,
      populate: populateSceneFixtureDimmer, update: updateSceneFixtureDimmer, show_reference: false,
      description: "Dimmer channel-based effects" },

    { name: "Sound level fader", class_name: "SoundLevel", synopsis: getSoundLevelSynopsis, 
      populate: populateSoundLevel, update: updateSoundLevel, show_reference: false,
      description: "Fade colors and/or dimmers based on venue sound level or beat." },

    { name: "Pattern dimmer", class_name: "ScenePatternDimmer", synopsis: getScenePatternDimmerSynopsis,
      populate: populateScenePatternDimmer, update: updateScenePatternDimmer, show_reference: false,
      description: "Light fixtures based on a pattern sequence." },

    { name: "Channel program", class_name: "SceneChannelAnimator", synopsis: getSceneChannelAnimatorSynopsis,
      populate: populateSceneChannelAnimator, update: updateSceneChannelAnimator, show_reference: true,
      description: "Animate channel values based on signal input." },

    { name: "Random movement", class_name: "SceneMovementAnimator.1", synopsis: getSceneMovementAnimator1Synopsis,
      populate: populateSceneMovementAnimator1, update: updateSceneMovementAnimator1, show_reference: false,
      description: "Generate random movement for pan/tilt fixtures" },

    { name: "Fan movement", class_name: "SceneMovementAnimator.2", synopsis: getSceneMovementAnimator2Synopsis,
      populate: populateSceneMovementAnimator2, update: updateSceneMovementAnimator2, show_reference: false,
      description: "Generate fan out movement for pan/tilt fixtures" },

    { name: "Rotate movement", class_name: "SceneMovementAnimator.3", synopsis: getSceneMovementAnimator3Synopsis,
      populate: populateSceneMovementAnimator3, update: updateSceneMovementAnimator3, show_reference: false,
      description: "Rotate movement for pan/tilt fixtures" },

    { name: "Up/down movement", class_name: "SceneMovementAnimator.4", synopsis: getSceneMovementAnimator4Synopsis,
      populate: populateSceneMovementAnimator4, update: updateSceneMovementAnimator4, show_reference: false,
      description: "Up/down movement for pan/tilt fixtures" },

    { name: "Beam cross movement", class_name: "SceneMovementAnimator.5", synopsis: getSceneMovementAnimator5Synopsis,
      populate: populateSceneMovementAnimator5, update: updateSceneMovementAnimator5, show_reference: false,
      description: "Beam cross movement for pan/tilt fixtures" },

    { name: "Programmed movement", class_name: "SceneMovementAnimator.6", synopsis: getSceneMovementAnimator6Synopsis,
      populate: populateSceneMovementAnimator6, update: updateSceneMovementAnimator6, show_reference: false,
      description: "Move to specific pan/tilt locations for pan/tilt fixtures" },

    { name: "Moonflower movement", class_name: "SceneMovementAnimator.7", synopsis: getSceneMovementAnimator7Synopsis,
      populate: populateSceneMovementAnimator7, update: updateSceneMovementAnimator7, show_reference: false,
      description: "Moonflower effect for pan/tilt fixtures" },

    { name: "Sine wave movement", class_name: "SceneMovementAnimator.8", synopsis: getSceneMovementAnimator8Synopsis,
      populate: populateSceneMovementAnimator8, update: updateSceneMovementAnimator8, show_reference: false,
      description: "Sine wave effect for pan/tilt fixtures" },

    { name: "Pixel animator", class_name: "ScenePixelAnimator", synopsis: getPixelAnimatorSynopsis,
      populate: populateScenePixelAnimator, update: updateScenePixelAnimator, show_reference: false,
      description: "Simple dot animations for single depth pixel devices" },

    { name: "Channel filter", class_name: "SceneChannelFilter", synopsis: getChannelFilterSynopsis,
      populate: populateSceneChannelFilter, update: updateSceneChannelFilter, show_reference: true,
      description: "Filters applied to channel values" },

    { name: "Pulse generator", class_name: "ScenePulse", synopsis: getScenePulseSynopsis, 
      populate: populateScenePulse, update: updateScenePulse, show_reference: false,
      description: "Generate color pulse effects across multiple fixtures either sequencially or randomly." },

    { name: "Cue animator", class_name: "SceneCueAnimator", synopsis: getSceneCueAnimatorSynopsis, 
      populate: populateSceneCueAnimator, update: updateSceneCueAnimator, show_reference: false,
      description: "Apply palette cue lists to fixtures or fan across groups of fixtures." },

    { name: "Fixture sequencer (deprecated)", class_name: "FixtureSequencer", synopsis: null,
      populate: populateFixtureSequencer, update: updateFixtureSequencer, show_reference: false,
      description: "Deprecated - switch to Pattern Dimmer sequence" }
];

var animations = [];

// ----------------------------------------------------------------------------
// class Animation
//
function Animation(animation_data)
{
    // Constructor
    jQuery.extend(this, animation_data);
}

// method getId
Animation.prototype.getId = function () {
    return this.id;
}

// method getNumber
Animation.prototype.getNumber = function () {
    return this.number;
}

// method getName
Animation.prototype.getName = function () {
    return this.name;
}

// method getCreated
Animation.prototype.getCreated = function () {
    return this.created;
}

// method getFullName
Animation.prototype.getFullName = function () {
    return this.name;
}

// method getDescription
Animation.prototype.getDescription = function () {
    return this.description;
}

// method getUserTypeName
Animation.prototype.getUserTypeName = function () {
    return "Animation";
}

// method getClassName
Animation.prototype.getClassName = function () {
    return this.class_name;
}

// method getSignal
Animation.prototype.getSignal = function() {
    return this.signal;
}

// method getAnimationData
Animation.prototype.getAnimationData = function() {
    return this[this.getClassName()];
}

// method getReferenceFixtureId
Animation.prototype.getReferenceFixtureId = function() {
    return this.reference_fixture_id;
}

// method isActive 
Animation.prototype.isActive = function () {
    return this.is_active;
}

// method setActive 
Animation.prototype.setActive = function ( active ) {
    this.is_active = active;
}

// method getActs
Animation.prototype.getActs = function () {
    return this.acts;
}

// method isShared
Animation.prototype.isShared = function () {
    return this.is_shared;
}

// ----------------------------------------------------------------------------
//
function filterAnimations( filter ) {
    var objects = [];
    for (var i = 0; i < animations.length; i++)
        if ( filter( animations[i] ) )
            objects[objects.length] = animations[i];
    return objects;
}

// ----------------------------------------------------------------------------
// 
function getActiveAnimations() {
    return filterAnimations( function( f ) { return f.isActive(); } );
}

// ----------------------------------------------------------------------------
//
function getAnimationById(id) {
    for (var i = 0; i < animations.length; i++)
        if (animations[i].getId() == id)
            return animations[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getAnimationByNumber(number) {
    for (var i = 0; i < animations.length; i++)
        if (animations[i].getNumber() == number)
            return animations[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getAnimationByName(name) {
    for (var i = 0; i < animations.length; i++)
        if (animations[i].getName() === name)
            return animations[i];

    return null;
}

// ----------------------------------------------------------------------------
//
function getUnusedAnimationNumber() {
    return getNextUnusedNumber( animations );
}

// ----------------------------------------------------------------------------
//
function updateCapturedAnimations( animation_refs ) {
    for (var i = 0; i < animations.length; i++) {
        var animation = animations[i];
        var captured = false;
        
        for ( var j=0; j < animation_refs.length; j++ )
            if ( animation_refs[j].id == animation.id ) {
                captured = true;
                break;
            }
        
        var active = animation.isActive();

        if ( active && !captured ) {
            markActiveAnimation(animation.id, false);
        }
        else if (!active && captured) {
            markActiveAnimation(animation.id, true );
        }
    }
}

// ----------------------------------------------------------------------------
//
function updateAnimations() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/animations/",
        cache: false,
        success: function (data) {
            animations = [];
            var json = jQuery.parseJSON(data);
            $.map(json, function (animation, index) {
                animations.push( new Animation(animation) );
            });
            createAnimationTiles( true );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function createAnimationTiles( no_wait ) {
    if ( no_wait )
        _createAnimationTiles();
    else
        delayUpdate( "animationTiles", 1,  _createAnimationTiles );
}

function _createAnimationTiles() {
    animation_tile_panel.empty();
    //active_animation_id = 0;

    $.each( animations, function ( index, animation ) {
        if (animation.isShared() && (current_act == 0 || animation.acts.indexOf(current_act) != -1) ) {
            animation_tile_panel.addTile(animation.id, animation.number, escapeForHTML(animation.name), true, true, 0, 0, false );

            if (animation.is_active) {
                animation.is_active = false;      // For the purposes of the current UI state the animation is not active

                markActiveAnimation(animation.id, true);
            }
        }
    });

    highlightSceneAnimations(active_scene_id, true);

    setEditMode(edit_mode);         // Refresh editing icons on new tiles
}

// ----------------------------------------------------------------------------
//
function highlightAnimations(animation_refs, highlight) {
    for (var i = 0; i < animation_refs.length; i++)
        animation_tile_panel.highlightTile(animation_refs[i].id, highlight);
}

// ----------------------------------------------------------------------------
//
function highlightAllAnimations( highlight) {
    animation_tile_panel.highlightAllTiles(highlight);
}

// ----------------------------------------------------------------------------
// Called on animation added event 
function newAnimationEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/animation/" + uid,
        cache: false,
        async: false,
        success: function (data) {
            var animation = new Animation( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < animations.length; i++) {
                if (animations[i].getId() == animation.getId() )
                    return;
                    
                if ( animations[i].getNumber() > animation.getNumber() ) {
                    animations.splice( i, 0, animation );

                    if ( animation.is_shared )
                        createAnimationTiles( false );
                    return;
                }
            }

            animations[animations.length] = animation;

            if ( animation.is_shared )
                createAnimationTiles( false );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on animation changed event 
function changeAnimationEvent( uid ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/animation/" + uid,
        cache: false,
        success: function (data) {
            var animation = new Animation( jQuery.parseJSON(data)[0] );

            for (var i = 0; i < animations.length; i++) {
                if ( animations[i].getId() == animation.getId() ) {
                    animations[i] = animation;

                    animation_tile_panel.updateTile( animation.id, animation.number, escapeForHTML(animation.name), 0, 0 );

                    if (animation.is_active) {
                        animation.is_active = false;      // For the purposes of the current UI state the animation is not active
                        markActiveAnimation(animation.id, true);
                    }

                    break;
                }
            }
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
// Called on animation delete event
function deleteAnimationEvent( uid ) {
    for (var i = 0; i < animations.length; i++) {
        if (animations[i].getId() == uid) {
            var shared = animations[i].is_shared;

            animations.splice( i, 1 );

            if ( shared )
                createAnimationTiles( false );
            break;
        }
    }
}

// ----------------------------------------------------------------------------
//
function deleteAnimation(event, animation_id) {
    stopEventPropagation(event);

    deleteVenueItem(getAnimationById(animation_id), function (item) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/delete/animation/" + item.getId(),
            cache: false,
            success: function () { },
            error: onAjaxError
        });
    });
}

// ----------------------------------------------------------------------------
//
function playAnimation(event, animation_id) {
    stopEventPropagation(event);

    var animation = getAnimationById(animation_id);
    if (animation == null)
        return;

    var command = !animation.isActive() ? "start" : "stop";
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/animation/" + command + "/" + animation_id,
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function markActiveAnimation(animation_id, new_state) {
    var animation = getAnimationById(animation_id);
    if (animation == null)
        return;

    var is_active = animation.isActive();

    if (new_state != is_active) {
        animation_tile_panel.selectTile(animation_id, new_state);
        animation.setActive(new_state);
    }
}

// ----------------------------------------------------------------------------
//
function describeAnimation(event, animation_id) {
    stopEventPropagation(event);

    var animation = getAnimationById(animation_id);
    if (animation == null) {
        messageBox("No definition for animation " + animation_id + " in client");
        return;
    }

    var acts = "";
    if (animation.getActs().length > 0) {
        for (var i = 0; i < animation.getActs().length; i++)
            acts += animation.getActs()[i] + " ";
    }
    else
        acts = "default";

    $("#describe_animation_dialog").dialog({
        autoOpen: false,
        width: 600,
        height: 550,
        modal: false,
        draggable: true,
        resizable: true,
        title: (animation.isShared() ? "Animation " + animation.getNumber() : "Private Animation") + ": " + escapeForHTML(animation.getName())
    });

    $("#describe_animation_type").html( getAnimationName(animation) );
    $("#describe_animation_number").html( animation.isShared() ? animation.getNumber() : "P" );
    $("#describe_animation_name").html(escapeForHTML(animation.getName()));
    $("#describe_animation_acts").html(acts);
    $("#describe_animation_description").html(animation.getDescription() != null ? escapeForHTML(animation.getDescription()) : "");

    var signal = getSignalSynopsis(animation.signal);
    var synopsis = getAnimationSynopsis(animation);

    var anim_info = findAnimation(animation);
    if (anim_info != null )
        $("#describe_animation_summary").html( anim_info.description );
    else
        $("#describe_animation_summary").html( "Description not available" );

    $("#describe_animation_classname").html( getAnimationName(animation) );
    $("#describe_animation_synopsis").html( synopsis.replace(/\n/g, "<br />") );
    $("#describe_animation_signal").html(signal);

    var scene_list = filterScenes( function ( scene ) {
        for (var i = 0; i < scene.getAnimationRefs().length; i++) {
            if ( scene.getAnimationRefs()[i].id == animation_id )
                return true;
        }
        return false;
    } );

    var scene_ids = scene_list.length > 0 ? "" : "None";

    for ( var i=0; i < scene_list.length; i++ ) {
        scene_ids = scene_ids + "S" + scene_list[i].getNumber() + " ";
    }

    $("#describe_animation_scenes").text(scene_ids);

    $("#describe_animation_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function colorName(rgb) {
    switch (rgb) {
        case "000000":
            return "Black";
        case "FF0000":
            return "Red";
        case "00FF00":
            return "Green";
        case "0000FF":
            return "Blue";
        case "FFFFFF":
            return "White";
        case "FFFF00":
            return "Yellow";
    }

    return "#" + rgb;
}


// ----------------------------------------------------------------------------
//
function findAnimation(what /* Overloaded for string or animation object */ ) {
    var class_name;

    if ( what == null )
        return null;
        
    if (typeof (what) === "string") {           // Class name
        class_name = what;
    }
    else {                                      // Assume this is an animation object
        class_name = what.class_name;

        if (class_name == MOVEMENT_ANIMATOR_ROOT && what.SceneMovementAnimator != null )
            class_name += "." + what.SceneMovementAnimator.movement_type;
    }

    for (var i = 0; i < AnimationEditors.length; i++) {
        if ( AnimationEditors[i].class_name == class_name )
            return AnimationEditors[i];
    }
    return null;
}

// ----------------------------------------------------------------------------
//
function getAnimationName(animation) {
    var anim_info = findAnimation(animation);
    if (anim_info == null || anim_info.name == null)
        return "UNKNOWN";
    return anim_info.name;
}

// ----------------------------------------------------------------------------
//
function getAnimationSynopsis(animation) {
    var anim_info = findAnimation(animation);
    if (anim_info == null || anim_info.synopsis == null )
        return "";

    return anim_info.synopsis( animation );
}

// ----------------------------------------------------------------------------
// Movement synopsis
//
function movementSynopsis(movement) {
    return "Speed: " + movement.speed +
           ", Head: " + (movement.head_number == 0 ? "all" : movement.head_number) +
           ", Once: " + (movement.run_once) +
           ", Target wait: " + movement.dest_wait_periods;
}

// ----------------------------------------------------------------------------
//
function getSignalSynopsis(signal) {
    var synopsis = "Trigger: " + SignalTrigger[signal.trigger-1];

    switch ( signal.trigger ) {
        case SignalTriggerType.TRIGGER_TIMER:
            synopsis += ", Speed: " + signal.timer_ms + "ms";
            if ( signal.speed_adjust != SpeedLevelType.SPEED_NORMAL )
                synopsis += " (" + SpeedLevel[signal.speed_adjust - 1] + ")";
            break;
        
        case SignalTriggerType.TRIGGER_FREQ_BEAT:
            synopsis += ", Range: " + signal.freq_low_hz + "-" + signal.freq_high_hz + " Hz";
            break;

        case SignalTriggerType.TRIGGER_AMPLITUDE_BEAT:
            synopsis += ", Sensitivity: " + signal.sensitivity;
            break;

        case SignalTriggerType.TRIGGER_RANDOM_TIMER:
            synopsis += ", Speed: 0-" + signal.timer_ms + "ms";
            if ( signal.speed_adjust != SpeedLevelType.SPEED_NORMAL )
                synopsis += " (" + SpeedLevel[signal.speed_adjust - 1] + ")";
            break;
    }

    if ( signal.off_ms )
        synopsis += ", Off: " + signal.off_ms + "ms";

    synopsis += "<br/>Source: " + SignalSource[signal.source-1];

    switch ( signal.source ) {
        case SignalSourceType.SOURCE_HIGH:
        case SignalSourceType.SOURCE_LOW:
        case SignalSourceType.SOURCE_VOLUME:
        case SignalSourceType.SOURCE_SOUND:
            break;

        case SignalSourceType.SOURCE_AMPLITUDE:
        case SignalSourceType.SOURCE_AVG_AMPLITUDE:
        case SignalSourceType.SOURCE_BEAT_INTENSITY:
        case SignalSourceType.SOURCE_FREQ:
            synopsis += ", Hold periods: " + signal.level_hold;
            break;

        case SignalSourceType.SOURCE_SQUAREWAVE:
        case SignalSourceType.SOURCE_SAWTOOTH:
        case SignalSourceType.SOURCE_SINEWAVE:
        case SignalSourceType.SOURCE_TRIANGLEWAVE:
        case SignalSourceType.SOURCE_STEPWAVE:
        case SignalSourceType.SOURCE_STEPWAVE_FULL:
            synopsis += ", Periods: " + signal.level_periods + ", Hold periods: " + signal.level_hold;
            break;

        case SignalSourceType.SOURCE_RANDOM:
            synopsis += ", Periods: " + signal.level_periods;
            break;
    }
    
    synopsis += "<br/>Level Range: ";

    if ( !signal.level_invert )
        synopsis += signal.level_floor + "-" + signal.level_ceil;
    else
        synopsis += signal.level_ceil + "-" + signal.level_floor;

    synopsis += "<br/>Scale Factor: " + signal.level_scale;

    return synopsis;
}

// ----------------------------------------------------------------------------
//
function populateSignal(prefix, signal) {
    var container = $("#" + prefix + "_signal");
    var signal_html = animation_signal_template.innerHTML.replace(/NNN/g, prefix);

    container.empty();
    container.html(signal_html);

    var trigger = $("#" + prefix + "_trigger");
    var source = $("#" + prefix + "_source");
    var timer_ms = $("#" + prefix + "_timer_ms");
    var off_ms = $("#" + prefix + "_off_ms");
    var level_floor = $("#" + prefix + "_level_floor");
    var level_ceil = $("#" + prefix + "_level_ceil");
    var level_invert = $("#" + prefix + "_level_invert");
    var level_scale = $("#" + prefix + "_level_scale");
    var level_periods = $("#" + prefix + "_level_periods");
    var freq_low_hz = $("#" + prefix + "_freq_low_hz");
    var freq_high_hz = $("#" + prefix + "_freq_high_hz");
    var speed_adjust = $("#" + prefix + "_speed_adjust");
    var level_hold = $("#" + prefix + "_level_hold");
    var sensitivity = $("#" + prefix + "_sensitivity"); 

    var frequency_div = $("#" + prefix + "_frequency_div");
    var timer_div = $("#" + prefix + "_timer_ms_div");
    var level_periods_div = $("#" + prefix + "_level_periods_div");
    var level_hold_div = $("#" + prefix + "_level_hold_div");
    var speed_adjust_div = $("#" + prefix + "_speed_adjust_div");
    var sensitivity_div = $("#" + prefix + "_sensitivity_div"); 

    var input_click_handler = function ( trigger_val, source_val ) {
        if ( trigger_val == SignalTriggerType.TRIGGER_TIMER || trigger_val == SignalTriggerType.TRIGGER_RANDOM_TIMER ) {
            timer_div.show();
        }
        else {
            timer_div.hide();
        }

        if ( trigger_val == SignalTriggerType.TRIGGER_AMPLITUDE_BEAT ) {
            sensitivity_div.show();
        }
        else {
            sensitivity_div.hide();
        }

        if ( trigger_val == SignalTriggerType.TRIGGER_FREQ_BEAT || source_val == SignalSourceType.SOURCE_FREQ )
            frequency_div.show();
        else
            frequency_div.hide();

        if ( source_val >= SignalSourceType.SOURCE_SQUAREWAVE && source_val <= SignalSourceType.SOURCE_RANDOM ) {
            level_periods_div.show();

            if ( source_val == SignalSourceType.SOURCE_RANDOM )
                level_hold_div.hide();
            else
                level_hold_div.show();
        }
        else {
            level_periods_div.hide();

            if ( source_val >= SignalSourceType.SOURCE_AMPLITUDE && source_val <= SignalSourceType.SOURCE_FREQ )
                level_hold_div.show();
            else
                level_hold_div.hide();
        }
    }

    load_options( trigger, SignalTrigger, function( index ) { return index+1 == signal.trigger; } );
    trigger.multiselect({ minWidth: 250, multiple: false, selectedList: 1, header: false, height: "auto" });
    trigger.unbind("multiselectclick");
    trigger.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        input_click_handler(parseInt(ui.value), parseInt( source.val() ));
    });

    load_options( source, SignalSource, function( index ) { return index+1 == signal.source; } );
    source.multiselect({ minWidth: 300, multiple: false, selectedList: 1, header: false, height: "auto" });
    source.unbind("multiselectclick");
    source.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        input_click_handler( parseInt( trigger.val() ), parseInt(ui.value));
    });

    timer_ms.spinner({ min: 1, max: 60000 }).val(signal.timer_ms);
    off_ms.spinner({ min: 0, max: 60000 }).val(signal.off_ms);
    level_periods.spinner({ min: 1, max: 1000 }).val(signal.level_periods);
    level_scale.spinner({ min: 1, max: 10 }).val(signal.level_scale);
    freq_low_hz.spinner({ min: 1, max: 22000 }).val(signal.freq_low_hz );
    freq_high_hz.spinner({ min: 1, max: 22000 }).val(signal.freq_high_hz);
    level_floor.spinner({ min: 1, max: MAXIMUM_LEVEL }).val(signal.level_floor);
    level_ceil.spinner({ min: 1, max: MAXIMUM_LEVEL }).val(signal.level_ceil);
    level_invert.attr('checked', signal.level_invert);
    level_hold.spinner({ min: 0, max: 1000 }).val(signal.level_hold);
    sensitivity.spinner({ min: 0, max: 100 }).val(signal.sensitivity);

    load_options(speed_adjust, SpeedLevel, function (index) { return index + 1 == signal.speed_adjust; });

    speed_adjust.multiselect({ minWidth: 250, multiple: false, selectedList: 1, header: false, height: "auto" });

    input_click_handler( parseInt( trigger.val() ), parseInt( source.val() ) );
}

// ----------------------------------------------------------------------------
//
function updateSignal(prefix, signal_target) {
    var signal = {
        trigger: parseInt( $("#" + prefix + "_trigger").val() ),
        source: parseInt( $("#" + prefix + "_source").val() ),
        timer_ms: parseInt( $("#" + prefix + "_timer_ms").val() ),
        off_ms: parseInt( $("#" + prefix + "_off_ms").val() ),
        level_floor: parseInt( $("#" + prefix + "_level_floor").val() ),
        level_ceil: parseInt(  $("#" + prefix + "_level_ceil").val() ),
        level_invert: $("#" + prefix + "_level_invert").is(':checked'),
        level_scale: parseInt( $("#" + prefix + "_level_scale").val() ),
        level_periods: parseInt($("#" + prefix + "_level_periods").val()),
        freq_low_hz: parseInt( $("#" + prefix + "_freq_low_hz").val() ),
        freq_high_hz: parseInt(  $("#" + prefix + "_freq_high_hz").val() ),
        speed_adjust: parseInt( $("#" + prefix + "_speed_adjust").val() ),
        level_hold: parseInt( $("#" + prefix + "_level_hold").val() ),
        sensitivity: parseInt( $("#" + prefix + "_sensitivity").val() )
    };

    if ( signal.trigger == SignalTriggerType.TRIGGER_FREQ_BEAT || signal.source == SignalSourceType.SOURCE_FREQ ) {
        if (signal.freq_low_hz > signal.freq_high_hz) {
            messageBox( "Low frequency must by less than high frequency");
            return false;
        }
    }

    if (signal.level_floor > signal.level_ceil) {
        messageBox( "Low level must be less than high level");
        return false;
    }

    if ( (signal.trigger == SignalTriggerType.TRIGGER_TIMER || signal.trigger == SignalTriggerType.TRIGGER_RANDOM_TIMER) && 
          signal.off_ms > signal.timer_ms ) {
        messageBox( "Off MS must be less than or equal to timer MS" );
        return false;
    }

    $.extend(signal_target, signal);

    return true;
}

// ----------------------------------------------------------------------------
//
function createAnimation(event) {
    stopEventPropagation(event);

    var new_number = getUnusedAnimationNumber();

    var reference_fixture_id = 0;

    filterFixtures( function( f ) { 
        if ( f.isActive() && reference_fixture_id == 0 ) {
            if ( !f.isGroup() )
                reference_fixture_id = f.getId();
            else if ( f.getFixtureIds().length > 0 )
                reference_fixture_id = f.getFixtureIds()[0];
        }
        return false;
    } )

    createAnimation2( true, new_number, "New Animation " + new_number, reference_fixture_id, null );
}

// ----------------------------------------------------------------------------
//
function createAnimation2( shared, new_number, new_name, reference_fixture_id, callback ) {
    if ( reference_fixture_id == 0 && fixtures.length > 0 )
        reference_fixture_id = fixtures[0].getId();

    openNewAnimationDialog("New Animation", "Create Animation", {
        "id": 0,
        "is_shared": shared,
        "name": new_name,
        "description": "",
        "number": new_number,
        "reference_fixture_id": reference_fixture_id,
        "signal": {
                trigger: SignalTriggerType.TRIGGER_TIMER,
                source: SignalSourceType.SOURCE_HIGH,
                timer_ms: 500,
                off_ms: 0,
                level_floor: 0,
                level_ceil: MAXIMUM_LEVEL,
                level_invert: false,
                level_scale: 1,
                level_periods: 10,
                freq_low_hz: 0,
                freq_high_hz: 22000,
                speed_adjust: SpeedLevelType.SPEED_NORMAL,
                level_hold: 0,
                sensitivity: 50
        },

        "class_name": "SceneColorSwitcher",
        "FixtureSequencer": null,
        acts: current_act == 0 ? [] : [ current_act ],
    }, true, false, callback );
}

// ----------------------------------------------------------------------------
//
function editAnimation(event, animation_id) {
    editAnimation2( event, animation_id, true, null );
}

// ----------------------------------------------------------------------------
//
function editAnimation2(event, animation_id, extended_options, callback ) {
    stopEventPropagation(event);

    var animation = getAnimationById(animation_id);
    if (animation == null)
        return;

    var data = {
        "id": animation.getId(),
        "is_shared": animation.isShared(),
        "name": animation.getName(),
        "description": animation.getDescription(),
        "number": animation.getNumber(),
        "reference_fixture_id": animation.getReferenceFixtureId(),
        "class_name": animation.getClassName(),
        "signal": animation.getSignal(),
        "acts": animation.getActs()
    }

    data[animation.getClassName()] = jQuery.extend(true, {}, animation.getAnimationData() )

    var multiple_scenes = false;

    if ( animation.isShared() ) {
        var scene_list = filterScenes( function ( scene ) {
            for (var i = 0; i < scene.getAnimationRefs().length; i++) {
                if ( scene.getAnimationRefs()[i].id == animation.getId() )
                    return true;
            }
            return false;
        } );

        multiple_scenes = scene_list.length > 1;
    }

    var editTitle = "Edit Animation";
    
    if ( animation.isShared() )
        editTitle += (" " + animation.getNumber());

    openNewAnimationDialog( editTitle, "Update Animation", data, extended_options, multiple_scenes, callback );
}

// ----------------------------------------------------------------------------
//
function openNewAnimationDialog(dialog_title, action_name, data, extended_options, multiple_scenes, callback) {
    var animation_test = false;                 // Tracks if we are running an animation test

    var getAnimation = function () {
        var class_name = $("#nad_type").val();

        var acts = $("#nad_acts").val();
        if (acts == null)
            acts = [];

        var shared = $("#nad_shared") .is(':checked');

        var json = {
            "id": data.id,
            "is_shared": shared, 
            "name": $("#nad_name").val(),
            "description": $("#nad_description").val(),
            "number": shared ? parseInt($("#nad_number").val()) : 0,
            "class_name": class_name,
            "reference_fixture_id": parseInt($("#nad_reference_fixture").val()),
            "acts": acts,
            "signal": {}
        };

        // Re-flatten movement animators
        if ( class_name.indexOf( "SceneMovementAnimator") != -1 )
            json.class_name = "SceneMovementAnimator";

        var anim = findAnimation( class_name );
        if ( anim == null ) {
            messageBox( "Unexpected: Animation class name is invalid " + class_name );
            return null;
        }

        if ( !anim.update( "nad_animation", json ) ) {
            return null;
        }

        if ( !updateSignal( "nad", json.signal ) ) {
             messageBox( "Animation signal contains invalid field values" );     
            return null;    
        }

        return json;
    }

    var send_update = function ( make_copy ) {
        var json = getAnimation();
        if ( json == null )
            return;

        if ( json.is_shared ) {
            if ( (make_copy || json.number != data.number) && getAnimationByNumber(json.number) != null ) {
                messageBox("Animation number " + json.number + " is already in use");
                return;
            }
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
            url: "/dmxstudio/rest/edit/animation/" +action + "/",
            data: JSON.stringify(json),
            contentType: 'application/json',
            success: function ( data ) {
                if ( callback != null ) {
                    var json = jQuery.parseJSON(data);

                    // Time out to allow for updates from server
                    setTimeout( callback, 250, json.id );
                }
            },
            cache: false,
            error: onAjaxError
        });

        $("#new_animation_dialog").dialog("close");
    };

    var dialog_buttons = [];

    if ( extended_options ) {
        dialog_buttons[dialog_buttons.length] = {
            text: "Test Animation",
            click: function () { 
                var json = getAnimation();
                if ( json == null )
                    return;

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/control/animation/test/start/",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    cache: false,
                    success: function () {
                        animation_test = true;
                    },
                    error: onAjaxError
                });
            }
        };

        if (data.id != 0 && data.is_shared ) {
            dialog_buttons[dialog_buttons.length] = {
                text: "Make Copy",
                click: function () { send_update(true); }
            };
        }
    }

    dialog_buttons[dialog_buttons.length] = {
        text: action_name,
        click: function () { send_update(false); }
    };

    dialog_buttons[dialog_buttons.length] = {
        text: "Cancel",
        click: function () {
            $("#new_animation_dialog").dialog("close");
        }
    };

    $("#new_animation_dialog").dialog({
        autoOpen: false,
        width: 850,
        height: 780,
        modal: true,
        resizable: false,
        buttons: dialog_buttons,
        title: dialog_title,
        close: function() {
            if ( animation_test ) {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/animation/test/stop/",
                    cache: false
                });
            }
        }
    });

    $("#nad_accordion").accordion({ heightStyle: "fill" });

    if (data.id == 0)
        $("#nad_accordion").accordion({ active: 0 });

    $("#nad_shared").unbind( "change" ).change( function( event ) { 
        stopEventPropagation( event );

        if ( $("#nad_shared") .is(':checked') ) {
            $("#nad_number_div").show();

            if ( parseInt($("#nad_number").val()) == 0 )
                $("#nad_number").val( getUnusedAnimationNumber() );
        }
        else
            $("#nad_number_div").hide();
    } );

    $("#nad_shared").attr( "checked", data.is_shared );
    $("#nad_number").spinner({ min: 1, max: 100000 }).val( data.number );
    $("#nad_name").val(data.name);
    $("#nad_description").val(data.description);

    if ( data.is_shared )
        $("#nad_number_div").show();
    else
        $("#nad_number_div").hide();

    if ( multiple_scenes ) {
         $("#nad_share_div").hide();
         $("#nad_shared_div").show();
    }
    else {
         $("#nad_share_div").show();
         $("#nad_shared_div").hide();
    }

    $("#nad_reference_fixture").empty();

    // Fill in the reference fixures
    for (var i = 0; i < fixtures.length; i++) {
        if ( !fixtures[i].isGroup() ) {
            if ( data.reference_fixture_id == 0 || data.reference_fixture_id == undefined )
                data.reference_fixture_id = fixtures[i].getId();

            $("#nad_reference_fixture").append($('<option>', {
                value: fixtures[i].getId(),
                text: fixtures[i].getLabel(),
                selected: data.reference_fixture_id == fixtures[i].getId()
            }));
        }
    }

    $("#nad_reference_fixture").multiselect({ minWidth: 500, multiple: false, selectedList: 1, 
                    header: false, noneSelectedText: 'select reference fixture', height: 270 });

    $("#nad_reference_fixture").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);

        data.reference_fixture_id = parseInt(ui.value);

        // Save existing values
        var anim = findAnimation( $("#nad_type").val() );
        if ( anim != null ) {
            anim.update( "nad_animation", data );
            anim.populate( "nad_animation", data );
        }
    });
    
    populateSignal( "nad", data.signal );

    $("#nad_type").empty();

    var animation_class_name = findAnimation( data );
    if ( animation_class_name == null )
        animation_class_name = AnimationEditors[0].class_name;

    for (var i = 0; i < AnimationEditors.length; i++) {
        $("#nad_type").append($('<option>', {
            value: AnimationEditors[i].class_name,
            text: AnimationEditors[i].name,
            selected: AnimationEditors[i].class_name === animation_class_name.class_name
        }));
    }
    $("#nad_type").multiselect({ minWidth: 400, multiple: false, selectedList: 1, header: false, noneSelectedText: 'select animation', height: "auto" });

    // Function to setup the dialog based on the selected animation class
    var change_animation = function( animation_class_name ) {
        var anim = findAnimation( animation_class_name );

        anim.populate( "nad_animation", data );

        $("#nad_type_description").html( anim.description );

        if ( anim.show_reference )
            $("#nad_reference_fixture_div").show();
        else
            $("#nad_reference_fixture_div").hide();
    }

    $("#nad_type").unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);

        // Save existing values in case the user switches again. Unused values will not be sent to the server.
        var anim = findAnimation( $("#nad_type").val() );
        if ( anim != null )
            anim.update( "nad_animation", data );

        change_animation( ui.value );
    });

    change_animation( animation_class_name );

    $("#nad_acts").empty();

    for (var i=1; i <= 20; i++) {
        $("#nad_acts").append($('<option>', {
            value: i,
            text: i,
            selected: data.acts.indexOf( i ) > -1
        }));
    }

    $("#nad_acts").multiselect({
        minWidth: 300, multiple: true, noneSelectedText: 'default act',
        checkAllText: 'All acts', uncheckAllText: 'Clear acts', selectedList: 8
    });

    $("#new_animation_dialog").dialog("open");
}

// ---------------------------------------------------------------------------- FixtureSequencer
//
function populateFixtureSequencer( container_name, animation ) {
    var container = $( "#" + container_name );
    var anim_html = $( "#" + "FixtureSequencer_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);
}

function updateFixtureSequencer( container_name, animation ) {
    animation.FixtureSequencer = {};

    return true;
}

// ---------------------------------------------------------------------------- SoundLevel
//
function populateSoundLevel( container_name, animation ) {
    if (animation.SoundLevel == null)
        animation.SoundLevel = {
            fade_what: 1
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SoundLevel_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var fade_what = $("#sl_" + container_name + "_fade_what");

    fade_what.empty();
    for (var i = 0; i < FadeWhat.length; i++) {
        fade_what.append($('<option>', {
            value: i + 1,
            text: FadeWhat[i],
            selected: i + 1 == animation.SoundLevel.fade_what
        }));
    }
    fade_what.multiselect({ minWidth: 350, multiple: false, selectedList: 1, header: false, height: "auto" });
}

function updateSoundLevel( container_name, animation ) {
    var fade_what = $("#sl_" + container_name + "_fade_what");

    animation.SoundLevel = {
        fade_what: fade_what.val()
    };

    return true;
}

function getSoundLevelSynopsis(animation) {
    return "Fade: " + FadeWhat[animation.SoundLevel.fade_what - 1];
}

// ---------------------------------------------------------------------------- ScenePatternDimmer
//
function populateScenePatternDimmer( container_name, animation ) {
    if (animation.ScenePatternDimmer == null)
        animation.ScenePatternDimmer = {
            dimmer_pattern: 1
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "ScenePatternDimmer_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var pattern = $("#spd_" + container_name + "_pattern");

    pattern.empty();
    for (var i = 0; i < PatternDimmer.length; i++) {
        pattern.append($('<option>', {
            value: i + 1,
            text: PatternDimmer[i],
            selected: i + 1 == animation.ScenePatternDimmer.dimmer_pattern
        }));
    }
    pattern.multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });
}

function updateScenePatternDimmer( container_name, animation ) {
    var pattern = $("#spd_" + container_name + "_pattern");

    animation.ScenePatternDimmer = {
        dimmer_pattern: pattern.val()
    };

    return true;
}

function getScenePatternDimmerSynopsis(animation) {
    return "Pattern: " + PatternDimmer[animation.ScenePatternDimmer.dimmer_pattern - 1];
}

// ---------------------------------------------------------------------------- SceneStrobeAnimator
//
function populateSceneStrobeAnimator( container_name, animation ) {
    if (animation.SceneStrobeAnimator == null)
        animation.SceneStrobeAnimator = {
            strobe_type: 1,
            strobe_percent: 50,
            strobe_color: "#FFFFFF",
            strobe_neg_color: "#000000",
            strobe_pos_ms: 100,
            strobe_neg_ms: 500,
            strobe_flashes: 1,
            strobe_fade_in_ms: 0,
            strobe_fade_out_ms: 0
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneStrobeAnimator_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html) ;

    var strobe_percent = $("#ssa_" + container_name + "_strobe_percent");
    var strobe_pos_ms = $("#ssa_" + container_name + "_strobe_pos_ms" );
    var strobe_neg_ms = $("#ssa_" + container_name + "_strobe_neg_ms" );
    var strobe_flashes = $("#ssa_" + container_name + "_strobe_flashes" );
    var strobe_fade_in_ms = $("#ssa_" + container_name + "_strobe_fade_in_ms" );
    var strobe_fade_out_ms = $("#ssa_" + container_name + "_strobe_fade_out_ms" );
    var color = $("#ssa_" + container_name + "_color");
    var neg_color = $("#ssa_" + container_name + "_neg_color");
    var strobe_type = $("#ssa_" + container_name + "_strobe_type");
    var strobe_type_1 = $("#ssa_" + container_name + "_strobe_type_1");
    var strobe_type_2 = $("#ssa_" + container_name + "_strobe_type_2");

    strobe_type.multiselect({ minWidth: 160, multiple: false, selectedList: 1, header: false, height: "auto" });

    strobe_type.unbind("multiselectclick");
    strobe_type.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);

        if ( ui.value == 1 ) {
            strobe_type_2.hide();
            strobe_type_1.show();
        }
        else {
            strobe_type_1.hide();
            strobe_type_2.show();
        }
    });

    strobe_type.find('option[value=' + animation.SceneStrobeAnimator.strobe_type + ']').attr('selected', 'selected');
    strobe_type.multiselect("refresh");

    if ( animation.SceneStrobeAnimator.strobe_type == 1 ) {
        strobe_type_2.hide();
        strobe_type_1.show();
    }
    else {
        strobe_type_1.hide();
        strobe_type_2.show();
    }

    strobe_percent.spinner({ min: 1, max: 100 }).val(animation.SceneStrobeAnimator.strobe_percent);
    strobe_pos_ms.spinner({ min: 1, max: 60000 }).val(animation.SceneStrobeAnimator.strobe_pos_ms);
    strobe_neg_ms.spinner({ min: 1, max: 60000 }).val(animation.SceneStrobeAnimator.strobe_neg_ms);
    strobe_flashes.spinner({ min: 1, max: 50 }).val(animation.SceneStrobeAnimator.strobe_flashes);
    strobe_fade_in_ms.spinner({ min: 0, max: 60000 }).val(animation.SceneStrobeAnimator.strobe_fade_in_ms);
    strobe_fade_out_ms.spinner({ min: 0, max: 60000 }).val(animation.SceneStrobeAnimator.strobe_fade_out_ms);

    setupColorPicker(color, animation.SceneStrobeAnimator.strobe_color);
    setupColorPicker(neg_color, animation.SceneStrobeAnimator.strobe_neg_color);
}
 
function updateSceneStrobeAnimator(container_name, animation) {
    var strobe_type = $("#ssa_" + container_name + "_strobe_type");
    var strobe_percent = $("#ssa_" + container_name + "_strobe_percent");
    var strobe_pos_ms = $("#ssa_" + container_name + "_strobe_pos_ms" );
    var strobe_neg_ms = $("#ssa_" + container_name + "_strobe_neg_ms" );
    var strobe_flashes = $("#ssa_" + container_name + "_strobe_flashes");
    var color = $("#ssa_" + container_name + "_color");
    var neg_color = $("#ssa_" + container_name + "_neg_color" );
    var strobe_fade_in_ms = $("#ssa_" + container_name + "_strobe_fade_in_ms" );
    var strobe_fade_out_ms = $("#ssa_" + container_name + "_strobe_fade_out_ms" );

    if ( strobe_pos_ms.val() == "" || strobe_neg_ms.val() == "" || strobe_flashes.val() == "" ) {
        messageBox( "Invalid positive, negative and/or flash count values" );
        return false;
    }

    if ( parseInt(strobe_fade_in_ms.val()) > parseInt(strobe_pos_ms.val()) ) {
        messageBox( "Stobe fade in must be <= strobe on MS" );
        return false;
    }

    if ( parseInt(strobe_fade_out_ms.val()) > parseInt(strobe_neg_ms.val()) ) {
        messageBox( "Stobe fade out must be <= strobe off MS" );
        return false;
    }

    animation.SceneStrobeAnimator = {
        strobe_type: parseInt( strobe_type.val() ),
        strobe_percent: parseInt(strobe_percent.val()),
        strobe_pos_ms: parseInt(strobe_pos_ms.val()),
        strobe_neg_ms: parseInt(strobe_neg_ms.val()),
        strobe_flashes: parseInt(strobe_flashes.val()),
        strobe_color: color.attr('value'),
        strobe_neg_color: neg_color.attr('value'),
        strobe_fade_in_ms: parseInt(strobe_fade_in_ms.val()),
        strobe_fade_out_ms: parseInt(strobe_fade_out_ms.val())
    };

    return true;
}

function getSceneStrobeAnimatorSynopsis(animation) {
    var synopsis;

    if ( animation.SceneStrobeAnimator.strobe_type == 2 ) {
        synopsis = "Fixture strobe: " + animation.SceneStrobeAnimator.strobe_percent + "%";
    }
    else {
        synopsis = "Strobe: ";
    
        if ( animation.SceneStrobeAnimator.strobe_flashes > 1 )
            synopsis += animation.SceneStrobeAnimator.strobe_flashes + " flashes over " + animation.SceneStrobeAnimator.strobe_pos_ms + "ms";
        else
            synopsis += "on " + animation.SceneStrobeAnimator.strobe_pos_ms + "ms";

        if ( animation.SceneStrobeAnimator.strobe_fade_in_ms > 0 )
            synopsis += ", fade in " + animation.SceneStrobeAnimator.strobe_fade_in_ms + "ms";

        synopsis += ", off " + animation.SceneStrobeAnimator.strobe_neg_ms + "ms";
    
        if ( animation.SceneStrobeAnimator.strobe_fade_out_ms > 0 )
            synopsis += ", fade out " + animation.SceneStrobeAnimator.strobe_fade_out_ms + "ms";

        synopsis += ", color " + colorName(animation.SceneStrobeAnimator.strobe_color);
        synopsis += ", off color " + colorName(animation.SceneStrobeAnimator.strobe_neg_color);
    }

    return synopsis;
}

// ---------------------------------------------------------------------------- SceneColorSwitcher
//
function populateSceneColorSwitcher( container_name, animation ) {
    if (animation.SceneColorSwitcher == null)
        animation.SceneColorSwitcher = {
            fader_effect: 1,
            strobe_neg_color: "#000000",
            strobe_pos_ms: 100,
            strobe_neg_ms: 500,
            strobe_flashes: 1,
            strobe_fade_in_ms: 0,
            strobe_fade_out_ms: 0,
            color_progression: []
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneColorSwitcher_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var strobe_pos_ms = $("#scs_" + container_name + "_strobe_pos_ms" );
    var strobe_neg_ms = $("#scs_" + container_name + "_strobe_neg_ms" );
    var colorpicker = $("#scs_" + container_name + "_colorpicker" );
    var fader_effect = $("#scs_" + container_name + "_fader_effect" );
    var color_progression = $("#scs_" + container_name + "_color_progression" );
    var strobe_flashes = $("#scs_" + container_name + "_strobe_flashes" );
    var strobe_fade_in_ms = $("#scs_" + container_name + "_strobe_fade_in_ms" );
    var strobe_fade_out_ms = $("#scs_" + container_name + "_strobe_fade_out_ms" );

    strobe_pos_ms.spinner({ min: 1, max: 60000 }).val(animation.SceneColorSwitcher.strobe_pos_ms);
    strobe_neg_ms.spinner({ min: 1, max: 60000 }).val(animation.SceneColorSwitcher.strobe_neg_ms);
    strobe_flashes.spinner({ min: 1, max: 50 }).val(animation.SceneColorSwitcher.strobe_flashes);
    strobe_fade_in_ms.spinner({ min: 0, max: 60000 }).val(animation.SceneColorSwitcher.strobe_fade_in_ms);
    strobe_fade_out_ms.spinner({ min: 0, max: 60000 }).val(animation.SceneColorSwitcher.strobe_fade_out_ms);
    
    setupColorPicker( colorpicker, animation.SceneColorSwitcher.strobe_neg_color );

    color_progression.expandableContainer({
        item_template: '<div class="container_input color_chip"></div>',
        new_item_callback: function (item, value) {
            if (value == null)
                value = '#000000';

            var chip = item.find(".container_input");
            setupColorPicker( chip, value );
        }
    });

    color_progression.expandableContainer("set_values", animation.SceneColorSwitcher.color_progression);

    fader_effect.empty();
    for (var i = 0; i < FaderEffect.length; i++) {
        fader_effect.append($('<option>', {
            value: i + 1,
            text: FaderEffect[i],
            selected: i + 1 == animation.SceneColorSwitcher.fader_effect
        }));
    }
    fader_effect.multiselect({ minWidth: 360, multiple: false, selectedList: 1, header: false, height: "auto" });
}

function updateSceneColorSwitcher( container_name, animation ) {
    var strobe_pos_ms = $("#scs_" + container_name + "_strobe_pos_ms" );
    var strobe_neg_ms = $("#scs_" + container_name + "_strobe_neg_ms" );
    var colorpicker = $("#scs_" + container_name + "_colorpicker" );
    var fader_effect = $("#scs_" + container_name + "_fader_effect" );
    var color_progression = $("#scs_" + container_name + "_color_progression" );
    var strobe_flashes = $("#scs_" + container_name + "_strobe_flashes" );
    var strobe_fade_in_ms = $("#scs_" + container_name + "_strobe_fade_in_ms" );
    var strobe_fade_out_ms = $("#scs_" + container_name + "_strobe_fade_out_ms" );

    if ( strobe_pos_ms.val() == "" || strobe_neg_ms.val() == "" || strobe_flashes.val() == "" ) {
        messageBox( "Invalid positive, negative and/or flash count values" );
        return false;
    }

    if ( parseInt(strobe_fade_in_ms.val()) > parseInt(strobe_pos_ms.val()) ) {
        messageBox( "Stobe fade in must be <= strobe on MS" );
        return false;
    }

    if ( parseInt(strobe_fade_out_ms.val()) > parseInt(strobe_neg_ms.val()) ) {
        messageBox( "Stobe fade out must be <= strobe off MS" );
        return false;
    }

    animation.SceneColorSwitcher = {
        fader_effect: fader_effect.val(),
        strobe_pos_ms: parseInt(strobe_pos_ms.val()),
        strobe_neg_ms: parseInt(strobe_neg_ms.val()),
        strobe_flashes: parseInt(strobe_flashes.val()),
        strobe_neg_color: colorpicker.attr('value'),
        strobe_fade_in_ms: parseInt(strobe_fade_in_ms.val()),
        strobe_fade_out_ms: parseInt(strobe_fade_out_ms.val()),
        color_progression: color_progression.expandableContainer("values")
    };

    return true;
}

function getSceneColorSwitcherSynopsis(animation) {
    var synopsis = "Fade: " + FaderEffect[animation.SceneColorSwitcher.fader_effect-1];

    if (animation.SceneColorSwitcher.color_progression != null && animation.SceneColorSwitcher.color_progression.length > 0) {
        synopsis += "\nColors: ";
        for ( var i=0; i < animation.SceneColorSwitcher.color_progression.length; i++ ) {
            if ( i > 0 )
                synopsis += ", ";
            synopsis += colorName( animation.SceneColorSwitcher.color_progression[i] );
        }
    }

    synopsis += "\nStrobe: ";
    
    if (animation.SceneColorSwitcher.strobe_flashes > 1)
        synopsis += animation.SceneColorSwitcher.strobe_flashes + " flashes over " + animation.SceneColorSwitcher.strobe_pos_ms + "ms";
    else
        synopsis += "on " + animation.SceneColorSwitcher.strobe_pos_ms + "ms";

    if ( animation.SceneColorSwitcher.strobe_fade_in_ms > 0 )
        synopsis += ", fade in " + animation.SceneColorSwitcher.strobe_fade_in_ms + "ms";

    synopsis += ", off " + animation.SceneColorSwitcher.strobe_neg_ms + "ms";
    
    if ( animation.SceneColorSwitcher.strobe_fade_out_ms > 0 )
        synopsis += ", fade out " + animation.SceneColorSwitcher.strobe_fade_out_ms + "ms";

    synopsis += ", off color " + colorName(animation.SceneColorSwitcher.strobe_neg_color);

    return synopsis;
}

// ---------------------------------------------------------------------------- ScenePixelAnimator
//
function populateScenePixelAnimator( container_name, animation ) {
    if (animation.ScenePixelAnimator == null)
        animation.ScenePixelAnimator = {
            pixel_effect: 1,
            pixel_off_color: "#000000",
            generations: 1,
            pixels: 1,
            increment: 1,
            fade: false,
            combine: false,
            color_progression: []
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "ScenePixelAnimator_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html) ;

    var combine = $("#spa_" + container_name + "_combine" );
    var pixel_effect = $("#spa_" + container_name + "_pixel_effect" );
    var generations = $("#spa_" + container_name + "_generations" );
    var pixels = $("#spa_" + container_name + "_pixels" );
    var increment = $("#spa_" + container_name + "_increment" );
    var fade = $("#spa_" + container_name + "_fade" );
    var color_progression = $("#spa_" + container_name + "_color_progression" );
    var pixel_off_color = $("#spa_" + container_name + "_pixel_off_color" );

    generations.spinner({ min: 1, max: 500 }).val(animation.ScenePixelAnimator.generations);
    pixels.spinner({ min: 1, max: 1024 }).val(animation.ScenePixelAnimator.pixels);
    increment.spinner({ min: 1, max: 1024 }).val(animation.ScenePixelAnimator.increment);
    fade.attr('checked', animation.ScenePixelAnimator.fade);
    combine.attr('checked', animation.ScenePixelAnimator.combine);

    setupColorPicker( pixel_off_color, animation.ScenePixelAnimator.pixel_off_color );

    color_progression.expandableContainer({
        item_template: '<div class="container_input color_chip"></div>',
        new_item_callback: function (item, value) {
            if (value == null)
                value = '#000000';

            var chip = item.find(".container_input");
            setupColorPicker( chip, value );
        }
    });
    color_progression.expandableContainer("set_values", animation.ScenePixelAnimator.color_progression);

    pixel_effect.empty();
    for (var i = 0; i < PixelEffect.length; i++) {
        pixel_effect.append($('<option>', {
            value: i + 1,
            text: PixelEffect[i],
            selected: i + 1 == animation.ScenePixelAnimator.pixel_effect
        }));
    }
    pixel_effect.multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });

    var generations_div = $("#spa_" + container_name + "_generations_div");
    var pixels_div = $("#spa_" + container_name + "_pixels_div");
    var increment_div = $("#spa_" + container_name + "_increment_div");
    var fade_div = $("#spa_" + container_name + "_fade_div");

    var update_effect = function (effect) {
        generations_div.show();
        pixels_div.show();
        increment_div.show();
        fade_div.show();

        switch ( parseInt(effect) ) {
            case 2:
            case 3:
            case 4:
                generations_div.hide();
                pixels_div.hide();
                increment_div.hide();
                fade_div.hide();
                break;
        
            case 5:
                generations_div.hide();
                pixels_div.hide();
                increment_div.hide();
                break;

            case 6:
                increment_div.hide();
                fade_div.hide();
                break;

            case 7:
                increment_div.hide();
                break;

            case 8:
                generations_div.hide();
                increment_div.hide();
                fade_div.hide();
        }
    }

    pixel_effect.unbind("multiselectclick");
    pixel_effect.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        update_effect(ui.value);
    });

    update_effect(animation.ScenePixelAnimator.pixel_effect);
}

function updateScenePixelAnimator( container_name, animation ) {
    var combine = $("#spa_" + container_name + "_combine" );
    var pixel_effect = $("#spa_" + container_name + "_pixel_effect" );
    var generations = $("#spa_" + container_name + "_generations" );
    var pixels = $("#spa_" + container_name + "_pixels" );
    var increment = $("#spa_" + container_name + "_increment" );
    var fade = $("#spa_" + container_name + "_fade" );
    var color_progression = $("#spa_" + container_name + "_color_progression" );
    var pixel_off_color = $("#spa_" + container_name + "_pixel_off_color" );

    animation.ScenePixelAnimator = {
        pixel_effect: parseInt(pixel_effect.val()),
        pixel_off_color: pixel_off_color.attr('value'),
        generations: generations.val(),
        pixels: pixels.val(),
        increment: increment.val(),
        fade: fade.is(':checked'),
        combine: combine.is(':checked'),
        color_progression: color_progression.expandableContainer("values")
    };

    return true;
}

function getPixelAnimatorSynopsis(animation) {
    var synopsis = PixelEffect[animation.ScenePixelAnimator.pixel_effect - 1] + ": ";

    switch ( animation.ScenePixelAnimator.pixel_effect ) {
        case 1:
            synopsis += "generations " + animation.ScenePixelAnimator.generations +
                        ", pixels " + animation.ScenePixelAnimator.pixels +
                        ", increment " + animation.ScenePixelAnimator.increment + 
                        ", fade " + ((animation.ScenePixelAnimator.fade) ? "yes" : "no") +
                        ", ";
            break;

        case 5:
            synopsis += "pixels " + animation.ScenePixelAnimator.pixels +
                        ", fade " + ((animation.ScenePixelAnimator.fade) ? "yes" : "no") +
                        ", ";
            break;

        case 6:
            synopsis += ", generations " + animation.ScenePixelAnimator.generations +
                        ", pixels " + animation.ScenePixelAnimator.pixels +
                        ", ";
            break;

        case 7:
            synopsis += "generations " + animation.ScenePixelAnimator.generations +
                        ", pixels " + animation.ScenePixelAnimator.pixels +
                        ", increment " + animation.ScenePixelAnimator.increment +
                        ", ";
            break;


        case 8:
            synopsis += ", pixels " + animation.ScenePixelAnimator.pixels +
                        ", ";
            break;
    }

    synopsis += "pixel off color " + colorName( animation.ScenePixelAnimator.pixel_off_color ) +
                ", combine " + ((animation.ScenePixelAnimator.combine) ? "yes" : "no" );

    if (animation.ScenePixelAnimator.color_progression != null && animation.ScenePixelAnimator.color_progression.length > 0) {
        synopsis += "\nColors: ";
        for (var i = 0; i < animation.ScenePixelAnimator.color_progression.length; i++) {
            if (i > 0)
                synopsis += ", ";
            synopsis += colorName(animation.ScenePixelAnimator.color_progression[i]);
        }
    }

    return synopsis;
}

// ---------------------------------------------------------------------------- SceneChannelFilter
//
function populateSceneChannelFilter( container_name, animation ) {
    if (animation.SceneChannelFilter == null)
        animation.SceneChannelFilter = {
            filter: 1,
            channels: [0],
            step: 1,
            amplitude: 10,
            offset: 0
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneChannelFilter_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var channels = $("#scf_" + container_name + "_channels" );
    var filter = $("#scf_" + container_name + "_filter" );
    var step = $("#scf_" + container_name + "_step" );
    var amplitude = $("#scf_" + container_name + "_amplitude" );
    var offset = $("#scf_" + container_name + "_offset" );

    step.spinner({ min: 1, max: 255 }).val(animation.SceneChannelFilter.step);
    amplitude.spinner({ min: 1, max: 255 }).val(animation.SceneChannelFilter.amplitude);
    offset.spinner({ min: 0, max: 360 }).val(animation.SceneChannelFilter.offset);

    load_options( filter, FilterEffect, function( index ) { return index+1 == animation.SceneChannelFilter.filter; } );
    filter.multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });

    var step_div = $("#scf_" + container_name + "_step_div" );
    var amplitude_div = $("#scf_" + container_name + "_amplitude_div" );
    var offset_div = $("#scf_" + container_name + "_offset_div" );

    var update_filter = function (filter) {
        step_div.show();
        amplitude_div.show();
        offset_div.show();

        switch (parseInt(filter)) {
            case 1:					    // Sine wave (amplitude, angle step)
                break;

            case 2:					    // Ramp up (step)
                offset_div.hide();
                break;

            case 3:					    // Ramp down (step)
                offset_div.hide();
                break;

            case 4:				        // Step wave (step)
                amplitude_div.hide();
                offset_div.hide();
                break;

            case 5:                     // Random value (amplitude)
                step_div.hide();
                offset_div.hide();
                break;
        }
    }

    filter.unbind("multiselectclick");
    filter.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        update_filter(ui.value);
    });

    update_filter(animation.SceneChannelFilter.filter);

    var update_channels = function (selected_channels) {
        channels.empty();

        var fixture = getFixtureById( animation.reference_fixture_id );

        if (fixture != null) {
            var fixture_channels = fixture.getChannels();
            var html = "";

            for (var i = 0; i < fixture_channels.length; i++) {
                var selected = selected_channels.indexOf(i) != -1;

                html += '<option value="' + i + '"' + (selected ? " selected" : "") +
                        '>Channel ' + (i + 1) + ": " + fixture_channels[i].name + '</option>';
            }

            channels.html(html);
        }

        channels.multiselect("refresh");
    };

    channels.multiselect({ minWidth: 500, multiple: true, selectedList: 1, header: false, noneSelectedText: 'select 1 or more channels', height: "auto" });

    update_channels(animation.SceneChannelFilter.channels);
}

function updateSceneChannelFilter( container_name, animation ) {
    var channels = $("#scf_" + container_name + "_channels" );
    var filter = $("#scf_" + container_name + "_filter" );
    var step = $("#scf_" + container_name + "_step" );
    var amplitude = $("#scf_" + container_name + "_amplitude" );
    var offset = $("#scf_" + container_name + "_offset" );

    animation.SceneChannelFilter = {
        filter: parseInt(filter.val()),
        step: parseInt(step.val()),
        amplitude: parseInt(amplitude.val()),
        offset: parseInt(offset.val()),
        channels: []
    };

    if ( channels.val() != null )
       animation.SceneChannelFilter.channels = channels.val().map(Number);

    return true;
}

function getChannelFilterSynopsis(animation) {
    var synopsis = FilterEffect[animation.SceneChannelFilter.filter - 1] + ": ";

    if (animation.SceneChannelFilter.channels.length == 1) {
        synopsis += "channel " + (animation.SceneChannelFilter.channels[0] + 1);
    }
    else {
        var channels = "";
        for (var i = 0; i < animation.SceneChannelFilter.channels.length; i++) {
            if (i > 0)
                channels += ", ";
            channels += (animation.SceneChannelFilter.channels[i] + 1);
        }

        synopsis += "channels " + channels;
    }

    switch (animation.SceneChannelFilter.filter) {
        case 1:					    // Sine wave (amplitude, angle step)
            synopsis += " step " + animation.SceneChannelFilter.step +
                        " amplitude " + animation.SceneChannelFilter.amplitude +
                        " offset " + animation.SceneChannelFilter.offset;
            break;

        case 2:					    // Ramp up (step)
            synopsis += " step " + animation.SceneChannelFilter.step +
                        " maximum " + animation.SceneChannelFilter.amplitude;
            break;

        case 3:					    // Ramp down (step)
            synopsis += " step " + animation.SceneChannelFilter.step +
                        " minimum " + animation.SceneChannelFilter.amplitude;
            break;

        case 4:				        // Step wave (step)
            synopsis += " step " + animation.SceneChannelFilter.step;
            break;

        case 5:                     // Random value (amplitude)
            synopsis += " amplitude " + animation.SceneChannelFilter.amplitude;
            break;
    }

    return synopsis;
}

// ---------------------------------------------------------------------------- SceneChannelAnimator
//
function populateSceneChannelAnimator( container_name, animation ) {
    if (animation.SceneChannelAnimator == null) {
        animation.SceneChannelAnimator = {
            channel_animations: []
        };
    }

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneChannelAnimator_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var animations_div = $("#sca_" + container_name + "_animations" );

    function add_channel_animation(item, channel_animation) {
        if (channel_animation == null) {
            channel_animation = {
                style: 1,
                values: [0, 255],
                channel: 0
            }

            // Default to last values
            animations_div.expandableContainer("values", function (item) {
                var esca_fixture = item.find(".sca_fixture");
                var esca_channel = item.find(".sca_channel");
                var esca_style = item.find(".sca_style");

                channel_animation.style = parseInt(esca_style.val());
                channel_animation.channel = parseInt(esca_channel.val());
                channel_animation.actor_uid = parseInt(esca_fixture.val());
            });
        }

        var esca_channel = item.find(".sca_channel");
        var esca_style = item.find(".sca_style");
        var esca_channel_list_container = item.find(".sca_channel_list_container");
        var esca_channel_list = item.find(".sca_channel_list");
        var esca_channel_range_container = item.find(".sca_channel_range_container" );
        var esca_channel_range_start = item.find(".sca_channel_range_start");
        var esca_channel_range_end = item.find(".sca_channel_range_end");

        esca_channel.multiselect({ minWidth: 500, multiple: false, selectedList: 1, header: false, noneSelectedText: 'select channel', height: "auto" });

        esca_channel.empty();

        var fixture = getFixtureById( animation.reference_fixture_id );

        if (fixture != null ) {
            var channels = fixture.getChannels();

            for (var i = 0; i < channels.length; i++) {
                esca_channel.append($('<option>', {
                    value: i,
                    text: "Channel " + (i+1) + ": " + channels[i].name,
                    selected: i == channel_animation.channel
                }));
            }
        }

        esca_channel.multiselect("refresh");

        esca_style.find('option[value=' + channel_animation.style + ']').attr('selected', 'selected');
        esca_style.multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });

        var update_style = function ( style ) {
            esca_channel_list_container.hide();
            esca_channel_range_container.hide();

            if ( style == 1 ) 
                esca_channel_list_container.show();
            else if (style == 2)
                esca_channel_range_container.show();
        }


        esca_style.unbind("multiselectclick");
        esca_style.bind("multiselectclick", function (event, ui) {
            stopEventPropagation(event);
            update_style( ui.value );
        });

        esca_channel_range_start.spinner({ min: 0, max: 255 }).val(channel_animation.values[0]);
        esca_channel_range_end.spinner({ min: 0, max: 255 }).val(channel_animation.values[1]);

        esca_channel_list.expandableContainer({
            item_template: "<div><input class='container_input' type='text' style='width:55px;'><div>",
            new_item_callback: function (item, value) {
                var input = item.find(".container_input");
                input.spinner({ min: 0, max: 255 }).val((value == null) ? 0 : value);
            }
        });

        esca_channel_list.expandableContainer("set_values", channel_animation.values);

        update_style(channel_animation.style);
    }

    animations_div.expandableContainer({
        item_template: SceneChannelAnimatorChannel_template.innerHTML,
        vertical: true,
        controls: "right",
        new_item_callback: add_channel_animation
    });

    animations_div.expandableContainer("set_values", animation.SceneChannelAnimator.channel_animations );
}

function updateSceneChannelAnimator( container_name, animation ) {
    var animations_div = $("#sca_" + container_name + "_animations" );

    animation.SceneChannelAnimator = {
        channel_animations: []
    };

    animations_div.expandableContainer("values", function (item) {
        var esca_channel = item.find(".sca_channel");
        var esca_style = item.find(".sca_style");
        var esca_channel_list = item.find(".sca_channel_list");
        var esca_channel_range_start = item.find(".sca_channel_range_start");
        var esca_channel_range_end = item.find(".sca_channel_range_end");

        var channel_animation = {
            style: parseInt( esca_style.val() ),
            values: [],
            channel: parseInt(esca_channel.val() )
        };

        if (channel_animation.style == 1) {
            channel_animation.values = esca_channel_list.expandableContainer("values");
        }
        else if (channel_animation.style == 2) {
            channel_animation.values.push(esca_channel_range_start.spinner("value"));
            channel_animation.values.push(esca_channel_range_end.spinner("value"));
        }

        animation.SceneChannelAnimator.channel_animations.push(channel_animation);
    });

    return true;
}

function getSceneChannelAnimatorSynopsis(animation) {
    var synopsis = "";

    for (var i = 0; i < animation.SceneChannelAnimator.channel_animations.length; i++) {
        if (i > 0)
            synopsis += " \n";

        var channel_animation = animation.SceneChannelAnimator.channel_animations[i];
        var fixture = getFixtureById(channel_animation.actor_uid);

        synopsis += "Channel " + (channel_animation.channel + 1) + ", " + AnimationStyle[channel_animation.style - 1];

        if (channel_animation.style == 1) {
            synopsis += " " + channel_animation.values;
        }
        else if (channel_animation.style == 2) {
            synopsis += " " + channel_animation.values[0] + "-" + channel_animation.values[1];
        }
    }

    return synopsis;
}

// ----------------------------------------------------------------------------
//
function initMovementAnimator( movement_type ) {
    return {
        movement_type: movement_type,
        speed: 0,
        dest_wait_periods: 1,
        home_wait_periods: 1,
        run_once: false,
        group_size: 1,
        positions: 10,
        blackout_return: false,
        alternate_groups: true,
        pan_start_angle: 0,
        pan_end_angle: 360,
        pan_increment: 15,
        tilt_start_angle: 0,
        tilt_end_angle: 180,
        coordinates: [{ pan: 0, tilt: 0 }],
        home_x: 0,
        home_y: 0,
        height: 8,
        fixture_spacing: 1.5,
        radius: 6,
        head_number: 0
    };
}

// ---------------------------------------------------------------------------- 
//
function populateCommonMovementControls( prefix, animation ) {
    var speed = $("#" + prefix + "_speed");
    var head = $("#" + prefix + "_head");
    var dest_wait_periods = $("#" + prefix + "_dest_wait_periods");
    var run_once = $("#" + prefix + "_run_once");

    speed.spinner({ min: 0, max: 255 }).val(animation.SceneMovementAnimator.speed);
    head.spinner({ min: 0, max: 32 }).val(animation.SceneMovementAnimator.head_number);
    dest_wait_periods.spinner({ min: 1, max: 10000 }).val(animation.SceneMovementAnimator.dest_wait_periods);
    run_once.attr('checked', animation.SceneMovementAnimator.run_once);

    var home_wait_periods = $("#" + prefix + "_home_wait_periods");
    var blackout_return = $("#" + prefix + "_blackout_return");
    var group_size = $("#" + prefix + "_group_size");
    var alternate_groups = $("#" + prefix + "_alternate_groups");
    var positions = $("#" + prefix + "_positions");
    var pan_start_angle = $("#" + prefix + "_pan_start_angle");
    var pan_end_angle = $("#" + prefix + "_pan_end_angle");
    var tilt_start_angle = $("#" + prefix + "_tilt_start_angle");
    var tilt_end_angle = $("#" + prefix + "_tilt_end_angle");
    var pan_increment = $("#" + prefix + "_pan_increment");

    if ( home_wait_periods.length )
        home_wait_periods.spinner({ min: 1, max: 100 }).val(animation.SceneMovementAnimator.home_wait_periods);

    if ( blackout_return.length )
        blackout_return.attr('checked', animation.SceneMovementAnimator.blackout_return);

    if ( group_size.length )
        group_size.spinner({ min: 1, max: 1000 }).val(animation.SceneMovementAnimator.group_size);
    
    if ( alternate_groups.length )
        alternate_groups.attr('checked', animation.SceneMovementAnimator.alternate_groups);

    if ( positions.length )
        positions.spinner({ min: 1, max: 200 }).val(animation.SceneMovementAnimator.positions);

    if ( pan_start_angle.length )
        pan_start_angle.spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_start_angle);

    if ( pan_end_angle.length )
        pan_end_angle.spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_end_angle);

    if ( tilt_start_angle.length )
        tilt_start_angle.spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_start_angle);

    if ( tilt_end_angle.length )
        tilt_end_angle.spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_end_angle);

    if ( pan_increment.length )
        pan_increment.spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_increment);
}

// ---------------------------------------------------------------------------- 
//
function updateCommonMovementControls( prefix, animation, movement_type ) {
    var speed = $("#" + prefix + "_speed");
    var head = $("#" + prefix + "_head");
    var dest_wait_periods = $("#" + prefix + "_dest_wait_periods");
    var run_once = $("#" + prefix + "_run_once");

    animation.SceneMovementAnimator = initMovementAnimator( movement_type );

    animation.SceneMovementAnimator.speed = speed.spinner("value");
    animation.SceneMovementAnimator.head_number = head.spinner("value");
    animation.SceneMovementAnimator.dest_wait_periods = dest_wait_periods.spinner("value");
    animation.SceneMovementAnimator.run_once = run_once.is(':checked');

    var home_wait_periods = $("#" + prefix + "_home_wait_periods");
    var blackout_return = $("#" + prefix + "_blackout_return");
    var group_size = $("#" + prefix + "_group_size");
    var alternate_groups = $("#" + prefix + "_alternate_groups");
    var positions = $("#" + prefix + "_positions");
    var pan_start_angle = $("#" + prefix + "_pan_start_angle");
    var pan_end_angle = $("#" + prefix + "_pan_end_angle");
    var tilt_start_angle = $("#" + prefix + "_tilt_start_angle");
    var tilt_end_angle = $("#" + prefix + "_tilt_end_angle");
    var pan_increment = $("#" + prefix + "_pan_increment");

    if ( group_size.length )
        animation.SceneMovementAnimator.group_size = group_size.spinner("value");
    if ( home_wait_periods.length )
        animation.SceneMovementAnimator.home_wait_periods = home_wait_periods.spinner("value");
    if ( blackout_return.length )
        animation.SceneMovementAnimator.blackout_return = blackout_return.is(':checked');
    if ( alternate_groups.length )
        animation.SceneMovementAnimator.alternate_groups = alternate_groups.is(':checked');
    if ( positions.length )
        animation.SceneMovementAnimator.positions = positions.spinner("value");
    if ( pan_start_angle.length )
        animation.SceneMovementAnimator.pan_start_angle = pan_start_angle.spinner("value");
    if ( pan_end_angle.length )
        animation.SceneMovementAnimator.pan_end_angle = pan_end_angle.spinner("value");
    if ( tilt_start_angle.length )
        animation.SceneMovementAnimator.tilt_start_angle = tilt_start_angle.spinner("value");
    if ( tilt_end_angle.length )
        animation.SceneMovementAnimator.tilt_end_angle = tilt_end_angle.spinner("value");
    if ( pan_increment.length )
        animation.SceneMovementAnimator.pan_increment = pan_increment.spinner("value");
}

// ---------------------------------------------------------------------------- SceneMovementAnimator1 - Random movement
//
function populateSceneMovementAnimator1( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 1 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator1_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma1_" + container_name;
    
    populateCommonMovementControls( prefix, animation );
}

function updateSceneMovementAnimator1( container_name, animation ) {
    var prefix = "sma1_" + container_name;

    updateCommonMovementControls( prefix, animation, 1 );

    return true;
}

function getSceneMovementAnimator1Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   ", Group size: " + animation.SceneMovementAnimator.group_size + ", \nPositions: " + animation.SceneMovementAnimator.positions +
                   ", Blackout movement: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ---------------------------------------------------------------------------- SceneMovementAnimator2 -  Fan movement
//
function populateSceneMovementAnimator2( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 2 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator2_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma2_" + container_name;

    populateCommonMovementControls( prefix, animation );
}

function updateSceneMovementAnimator2( container_name, animation ) {
    var prefix = "sma2_" + container_name;

    updateCommonMovementControls( prefix, animation, 2 );

    return true;
}

function getSceneMovementAnimator2Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Increment: " + animation.SceneMovementAnimator.pan_increment +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   ", Return blackout: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ---------------------------------------------------------------------------- SceneMovementAnimator3 - Rotate movement
//
function populateSceneMovementAnimator3( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 3 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator3_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma3_" + container_name;

    populateCommonMovementControls( prefix, animation );
}
 
function updateSceneMovementAnimator3( container_name, animation ) {
    var prefix = "sma3_" + container_name;

    updateCommonMovementControls( prefix, animation, 3 );

    return true;
}
    
function getSceneMovementAnimator3Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   "," + (animation.SceneMovementAnimator.alternate_groups ? " Alternate" : "") + " Groups: " + animation.SceneMovementAnimator.group_size +
                   ", Return blackout: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ---------------------------------------------------------------------------- SceneMovementAnimator4 - Up/down movement
//
function populateSceneMovementAnimator4( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 4 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator4_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma4_" + container_name;

    populateCommonMovementControls( prefix, animation );
}

function updateSceneMovementAnimator4( container_name, animation ) {
    var prefix = "sma4_" + container_name;

    updateCommonMovementControls( prefix, animation, 4 );

    return true;
}

function getSceneMovementAnimator4Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Increment: " + animation.SceneMovementAnimator.pan_increment +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   "," + (animation.SceneMovementAnimator.alternate_groups ? " Alternate" : "") + " Groups: " + animation.SceneMovementAnimator.group_size +
                   ", Return blackout: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ---------------------------------------------------------------------------- SceneMovementAnimator5 -Beam cross movement
//
function populateSceneMovementAnimator5( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 5 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator5_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma5_" + container_name;

    populateCommonMovementControls( prefix, animation );
}

function updateSceneMovementAnimator5( container_name, animation ) {
    var prefix = "sma5_" + container_name;

    updateCommonMovementControls( prefix, animation, 5 );

    return true;
}

function getSceneMovementAnimator5Synopsis(animation) {
    return getSceneMovementAnimator3Synopsis(animation)
}

// ---------------------------------------------------------------------------- SceneMovementAnimator6 - Programmed movement
//
function populateSceneMovementAnimator6( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 6 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator6_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma6_" + container_name;

    populateCommonMovementControls( prefix, animation );

    $("#" + prefix + "_coordinates").expandableContainer({
        item_template: '<div>pan <input class="pan" style="width:70px;"><span style="margin-left:20px;">tilt </span><input class="tilt" style="width:70px;" ></div>',
        vertical: true,
        controls: "right",
        new_item_callback: function (item, value) {
            if (value == null)
                value = { pan: 0, tilt: 0 };
            item.find(".pan").spinner({ min: 0, max: 720 }).val(value.pan);
            item.find(".tilt").spinner({ min: 0, max: 720 }).val(value.tilt);
        }
    });

    for (var i=0; i < animation.SceneMovementAnimator.coordinates.length; i++) {
        $("#" + prefix + "_coordinates").expandableContainer("set_values", animation.SceneMovementAnimator.coordinates[i]);
    }
}

function updateSceneMovementAnimator6( container_name, animation ) {
    var prefix = "sma6_" + container_name;

    updateCommonMovementControls( prefix, animation, 6 );

    animation.SceneMovementAnimator.coordinates = [];
    $("#" + prefix + "_coordinates").expandableContainer("values", function (item) {
        var pan = item.find(".pan").spinner("value");
        var tilt = item.find(".tilt").spinner("value");
        animation.SceneMovementAnimator.coordinates.push({ "pan": pan, "tilt": tilt });
    });

    return true;
}

function getSceneMovementAnimator6Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Coordinates: ";

    for (var i = 0; i < animation.SceneMovementAnimator.coordinates.length; i++) {
        if (i > 0)
            synopsis += ", ";
        synopsis += "(" + animation.SceneMovementAnimator.coordinates[i].pan + ", " + animation.SceneMovementAnimator.coordinates[i].tilt + ")";
    }

    return synopsis;
}

// ---------------------------------------------------------------------------- SceneMovementAnimator7 - Moonflower movement
//
function populateSceneMovementAnimator7( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 7 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator7_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma7_" + container_name;

    populateCommonMovementControls( prefix, animation );

    $("#" + prefix + "_home_x").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.home_x);
    $("#" + prefix + "_home_y").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.home_y);
    $("#" + prefix + "_height").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.height);
    $("#" + prefix + "_fixture_spacing").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.fixture_spacing);
    $("#" + prefix + "_radius").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.radius);
}

function updateSceneMovementAnimator7( container_name, animation ) {
    var prefix = "sma7_" + container_name;

    updateCommonMovementControls( prefix, animation, 7 );

    animation.SceneMovementAnimator.home_x = $("#" + prefix + "_home_x").spinner("value");
    animation.SceneMovementAnimator.home_y = $("#" + prefix + "_home_y").spinner("value");
    animation.SceneMovementAnimator.height = $("#" + prefix + "_height").spinner("value");
    animation.SceneMovementAnimator.fixture_spacing = $("#" + prefix + "_fixture_spacing").spinner("value");
    animation.SceneMovementAnimator.radius = $("#" + prefix + "_radius").spinner("value");

    return true;
}
       
function getSceneMovementAnimator7Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Height: " + animation.SceneMovementAnimator.height + "'" +
                   ", Spacing: " + animation.SceneMovementAnimator.fixture_spacing + "'" +
                   ", Radius: " + animation.SceneMovementAnimator.radius + "'" +
                   ", Home: " + animation.SceneMovementAnimator.home_x + "," + animation.SceneMovementAnimator.home_y +
                   ", Positions: " + animation.SceneMovementAnimator.positions +
                   ", Increment: " + animation.SceneMovementAnimator.pan_increment;

    return synopsis;
}

// ---------------------------------------------------------------------------- SceneMovementAnimator8 - Sinewave movement
//
function populateSceneMovementAnimator8( container_name, animation ) {
    if ( animation.SceneMovementAnimator == null )
        animation.SceneMovementAnimator = initMovementAnimator( 8 );

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneMovementAnimator8_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html );

    var prefix = "sma8_" + container_name;

    populateCommonMovementControls( prefix, animation );
}

function updateSceneMovementAnimator8( container_name, animation ) {
    var prefix = "sma8_" + container_name;

    updateCommonMovementControls( prefix, animation, 8 );

    return true;
}

function getSceneMovementAnimator8Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) + "\n" +
                   "Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   ", Step: " + animation.SceneMovementAnimator.positions +
                   ", Offset: " + animation.SceneMovementAnimator.pan_increment +
                   ", Group size: " + animation.SceneMovementAnimator.group_size + ", Positions: " + animation.SceneMovementAnimator.positions;
    return synopsis;
}

// ---------------------------------------------------------------------------- ScenePulse
//
function populateScenePulse( container_name, animation ) {
    if (animation.ScenePulse == null)
        animation.ScenePulse = {
            pulse_effect: 1,
            pulse_color: "#FFFFFF",
            pulse_ms: 250,
            pulse_fixture_count: 1,
            select_random: false
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "ScenePulse_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var pulse_effect = $("#sp_" + container_name + "_pulse_effect" );
    var pulse_fixture_count = $("#sp_" + container_name + "_pulse_fixture_count" );
    var pulse_ms = $("#sp_" + container_name + "_pulse_ms" );
    var select_random = $("#sp_" + container_name + "_select_random" );
    var pulse_color = $("#sp_" + container_name + "_pulse_color" );

    select_random.attr('checked', animation.ScenePulse.select_random );
    pulse_ms.spinner({ min: 1, max: 60000 }).val(animation.ScenePulse.pulse_ms);
    pulse_fixture_count.spinner({ min: 1, max: 20 }).val(animation.ScenePulse.pulse_fixture_count);
    
   setupColorPicker( pulse_color, animation.ScenePulse.pulse_color );

    pulse_effect.empty();
    for (var i = 0; i < PulseEffect.length; i++) {
        pulse_effect.append($('<option>', {
            value: i + 1,
            text: PulseEffect[i],
            selected: i + 1 == animation.ScenePulse.pulse_effect
        }));
    }
    pulse_effect.multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });
}

function updateScenePulse( container_name, animation ) {
    var pulse_effect = $("#sp_" + container_name + "_pulse_effect" );
    var pulse_fixture_count = $("#sp_" + container_name + "_pulse_fixture_count" );
    var pulse_ms = $("#sp_" + container_name + "_pulse_ms" );
    var select_random = $("#sp_" + container_name + "_select_random" );
    var pulse_color = $("#sp_" + container_name + "_pulse_color" );

    if ( pulse_ms.val() == "" ) {
        messageBox( "Invalid pulse MS value" );
        return false;
    }

    animation.ScenePulse = {
        pulse_effect: parseInt(pulse_effect.val()),
        pulse_color: pulse_color.attr('value'),
        pulse_ms: parseInt(pulse_ms.val()),
        pulse_fixture_count: parseInt(pulse_fixture_count.val()),
        select_random: select_random.is(':checked')
    };

    return true;
}

function getScenePulseSynopsis(animation) {
    var synopsis = // "Pulse: " + PulseEffect[animation.ScenePulse.pulse_effect-1] +
                   " Duration: " + animation.ScenePulse.pulse_ms + "ms" +
                   " Color: " + colorName( animation.ScenePulse.pulse_color +
                   " Fixtures: " + animation.ScenePulse.pulse_fixture_count +
                   "\nRandom fixtures: " + animation.ScenePulse.select_random );


    return synopsis;
}

// ---------------------------------------------------------------------------- SceneCueAnimator
//
function populateSceneCueAnimator( container_name, animation ) {
    if (animation.SceneCueAnimator == null)
        animation.SceneCueAnimator = {
            tracking: false,
            group_size: 0,
            cues: []
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneCueAnimator_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var tracking = $("#sca_" + container_name + "_tracking" );
    var group_size = $("#sca_" + container_name + "_group_size" );
    var cues_div = $("#sca_" + container_name + "_cues" );

    tracking.attr('checked', animation.SceneCueAnimator.tracking );
    group_size.spinner({ min: 0, max: 100 }).val(animation.SceneCueAnimator.group_size );

    function add_cue( item, palette_refs ) {
        if ( palette_refs == null )
            palette_refs = []

        var select_palette = item.find( "select" );

        select_palette.empty();

        var palettes = filterPalettes( function() { return true; } );

        for (var i=0; i < palettes.length; i++) {
            select_palette.append($('<option>', {
                value: palettes[i].getId(),
                text: palettes[i].getName(),
                selected: palette_refs.indexOf( palettes[i].getId() ) != -1 
            }));
        }

        select_palette.multiselect({ minWidth: 595, multiple: true, selectedList: 4, header: true, noneSelectedText: 'select cues', height: "auto" });
    }

    cues_div.expandableContainer({
        item_template: SceneCueAnimatorCue_template.innerHTML,
        vertical: true,
        controls: "right",
        new_item_callback: add_cue
    });

    cues_div.expandableContainer("set_values", animation.SceneCueAnimator.cues );
}

function updateSceneCueAnimator( container_name, animation ) {
    var tracking = $("#sca_" + container_name + "_tracking" );
    var group_size = $("#sca_" + container_name + "_group_size" );
    var cues_div = $("#sca_" + container_name + "_cues" );
    
    animation.SceneCueAnimator = {
        group_size: parseInt(group_size.val()),
        tracking: tracking.is(':checked'),
        cues: []
    };

    cues_div.expandableContainer( "values", function (item) {
        animation.SceneCueAnimator.cues.push( item.find( "select" ).val() );
    });

    return true;
}

function getSceneCueAnimatorSynopsis(animation) {
    var synopsis = "Tracking: " + (animation.SceneCueAnimator.tracking ? "yes" : "no") +
                   ", Group size: " + animation.SceneCueAnimator.group_size;

    for ( var i=0; i < animation.SceneCueAnimator.cues.length; i++ ) {
        synopsis += "\nCue " + (i+1) + ": ";

        for ( var j=0; j < animation.SceneCueAnimator.cues[i].length; j++ ) {
            if ( j > 0 )
                synopsis += ", ";

            var palette = getPaletteById( animation.SceneCueAnimator.cues[i][j] );
            if ( palette != null )
                synopsis += palette.getName();
        }
    }

    return synopsis;
}

// ---------------------------------------------------------------------------- SceneFixtureDimmer
//
function populateSceneFixtureDimmer( container_name, animation ) {
    if (animation.SceneFixtureDimmer == null)
        animation.SceneFixtureDimmer = {
            "dimmer_mode": 1,
            "min_percent": 0,
            "max_percent": 100
        };

    var container = $( "#" + container_name );
    var anim_html = $( "#" + "SceneFixtureDimmer_template" ).get(0).innerHTML.replace( /NNN/g, container_name );

    container.empty();
    container.html( anim_html);

    var dimmer_mode = $("#sfd_" + container_name + "_dimmer_mode");
    var min_percent = $("#sfd_" + container_name + "_min_percent");
    var max_percent = $("#sfd_" + container_name + "_max_percent");

    min_percent.spinner({ min: 0, max: 100 }).val(animation.SceneFixtureDimmer.min_percent );
    max_percent.spinner({ min: 0, max: 100 }).val(animation.SceneFixtureDimmer.max_percent );

    dimmer_mode.empty();
    for (var i = 0; i < DimmerMode.length; i++) {
        dimmer_mode.append($('<option>', {
            value: i + 1,
            text: DimmerMode[i],
            selected: i + 1 == animation.SceneFixtureDimmer.dimmer_mode
        }));
    }
    dimmer_mode.multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });
}

function updateSceneFixtureDimmer( container_name, animation ) {
    var dimmer_mode = $("#sfd_" + container_name + "_dimmer_mode");
    var min_percent = $("#sfd_" + container_name + "_min_percent");
    var max_percent = $("#sfd_" + container_name + "_max_percent");

    if ( parseInt(min_percent.val()) >= parseInt(max_percent.val()) ) {
        messageBox( "Dimmer minumum % must be less than maximum %" );
        return false;
    }

    animation.SceneFixtureDimmer = {
        "dimmer_mode": dimmer_mode.val(),
        "min_percent": parseInt( min_percent.val() ),
        "max_percent": parseInt( max_percent.val() )
    };

    return true;
}

function getSceneFixtureDimmerSynopsis(animation) {
    return "Dimmer: " + DimmerMode[animation.ScenePatternDimmer.dimmer_mode - 1] + 
            " (" + animation.ScenePatternDimmer.min_percent + "% - " + animation.ScenePatternDimmer.max_percent + "%)";
}

// ----------------------------------------------------------------------------
//
function setupColorPicker(element, color) {
    updateColorChip( element, "#" + color, 3 );
    element.attr('value', color);

    element.ColorPicker({
        color: "#000000",
        livePreview: true,
        autoClose: true,

        onShow: function (picker) {
            $(picker).ColorPickerSetColorChips(getColorChips());
            $(picker).ColorPickerSetColor(color);
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onChange: function (hsb, hex, rgb) {
            updateColorChip( element, "#" + hex, 3 );
            element.attr('value', hex);
        },

        onChipSet: function (chip, hsb, hex, rgb) {
            return setColorChip(chip, hex);
        }
    });
}