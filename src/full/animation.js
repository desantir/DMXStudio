/* 
Copyright (C) 2013-14 Robert DeSantis
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

var ApplySignal = ["Channel Values", "Sample Speed", "Values and Speed", "Sample Speed  (inverted)"];
var AnimationSignalInput = ["Timer", "Sound Level", "Avg. Sound Level", "Frequency Beat", "Frequency Level", "Random"];
var FadeWhat = ["Color channels", "Dimmer channels", "Color and Dimmer channels"];
var PatternDimmer = [ "Simple sequence", "Cylon", "Pairs", "Toward center", "Alternate", "All" ];
var AnimationStyle = ["Value List", "Value Range", "Scale Scene Value"];
var FaderEffect = [ "Color Change", "Strobe", "Color Blend", "All" ];
var PixelEffect = [ "Scrolling", "Stacked", "Stacked Left", "Stacked Right", "Beam", "Random", "Chase" ];

var MOVEMENT_ANIMATOR_ROOT = "SceneMovementAnimator";

var Animations = [
    { name: "Fixture sequencer", class_name: "FixtureSequencer", editor: editFixtureSequencer, synopsis: null,
      description: "Sequence fixtures based on timer (simple on/off)." },
    { name: "Channel fader", class_name: "SoundLevel", editor: editSceneSoundLevel, synopsis: getSceneSoundLevelSynopsis, 
      description: "Fade colors and/or dimmers based on venue sound level or beat." },
    { name: "Pattern sequencer", class_name: "ScenePatternDimmer", editor: editScenePatternDimmer, synopsis: getScenePatternDimmerSynopsis,
      description: "Light fixtures based on a pattern sequence." },
    { name: "Channel program", class_name: "SceneChannelAnimator", editor: editSceneChannelAnimator, synopsis: getSceneChannelAnimatorSynopsis,
      description: "Animate channel values based on signal input." },
    { name: "Color fader", class_name: "SceneColorSwitcher", editor: editSceneColorSwitcher, synopsis: getSceneColorSwitcherSynopsis,
      description: "Change fixture colors using color progression and signal input trigger. Randomly chooses color switch, blend or strobe effects." },
    {name: "Strobe", class_name: "SceneStrobeAnimator", editor: editSceneStrobeAnimator, synopsis: getSceneStrobeAnimatorSynopsis,
      description: "Strobe fixtures with selectable negative color." },
    { name: "Random movement", class_name: "SceneMovementAnimator.1", editor: editSceneMovementAnimator1, synopsis: getSceneMovementAnimator1Synopsis,
      description: "Generate random movement for pan/tilt fixtures" },
    { name: "Fan movement", class_name: "SceneMovementAnimator.2", editor: editSceneMovementAnimator2, synopsis: getSceneMovementAnimator2Synopsis,
      description: "Generate fan out movement for pan/tilt fixtures" },
    { name: "Rotate movement", class_name: "SceneMovementAnimator.3", editor: editSceneMovementAnimator3, synopsis: getSceneMovementAnimator3Synopsis,
      description: "Rotate movement for pan/tilt fixtures" },
    { name: "Up/down movement", class_name: "SceneMovementAnimator.4", editor: editSceneMovementAnimator4, synopsis: getSceneMovementAnimator4Synopsis,
      description: "Up/down movement for pan/tilt fixtures" },
    { name: "Beam cross movement", class_name: "SceneMovementAnimator.5", editor: editSceneMovementAnimator5, synopsis: getSceneMovementAnimator5Synopsis,
    description: "Beam cross movement for pan/tilt fixtures" },
    { name: "Programmed movement", class_name: "SceneMovementAnimator.6", editor: editSceneMovementAnimator6, synopsis: getSceneMovementAnimator6Synopsis,
      description: "Move to specific pan/tilt locations for pan/tilt fixtures" },
    { name: "Moonflower movement", class_name: "SceneMovementAnimator.7", editor: editSceneMovementAnimator7, synopsis: getSceneMovementAnimator7Synopsis,
      description: "Moonflower effect for pan/tilt fixtures" },
    { name: "Pixel animator", class_name: "ScenePixelAnimator", editor: editPixelAnimator, synopsis: getPixelAnimatorSynopsis,
      description: "Simple dot animations for single depth pixel devices" },
];

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
    }

    return "#" + rgb;
}


// ----------------------------------------------------------------------------
//
function findAnimation(what /* Overloaded for string or animation object */ ) {
    var class_name;

    if (typeof (what) === "string") {           // Class name
        class_name = what;
    }
    else {                                      // Assume this is an animation object
        class_name = what.class_name;

        if (class_name == MOVEMENT_ANIMATOR_ROOT)
            class_name += "." + what.SceneMovementAnimator.movement_type;
    }

    for (var i = 0; i < Animations.length; i++) {
        if ( Animations[i].class_name == class_name )
            return Animations[i];
    }
    return null;
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
//
function getSignalSynopsis(signal) {
    var synopsis = "";

    synopsis += AnimationSignalInput[signal.input_type - 1] + ": "
    synopsis += "Sampling  " + signal.sample_rate_ms + " ms, ";
    synopsis += "Decay " + signal.sample_decay_ms + "ms";

    if (signal.input_type != 1)
        synopsis += "<br/>Apply To: " + ApplySignal[signal.apply_to - 1];

    switch (signal.input_type) {
        case 1:     // Timer only
            break;

        case 2:     // Sound level
        case 3:     // Avg sound level
            synopsis += ", Level Curve: " + signal.scale_factor;
            synopsis += ", Maximum: " + signal.max_threshold;
            break;

        case 5:     // Frequency level
            synopsis += ", Level Curve: " + signal.scale_factor;
            synopsis += ", Maximum: " + signal.max_threshold;
        case 4:     // Frequency beat
            synopsis += ", Frequencies: " + signal.input_low + "-" + signal.input_high
            break;

        case 6:     // Random value
            synopsis += ", Random Values: " + signal.input_low + "-" + signal.input_high;
            break;
    }

    return synopsis;
}

// ----------------------------------------------------------------------------
//
function createAnimationButtonText(animation) {
    var anim_info = findAnimation(animation);

    var button_innards = "<div class='animation_button_title'>" + anim_info.name + "</div>";

    button_innards += "<div class='animation_button_fixtures'>";

    if (animation.actors.length > 0) {
        for (var i = 0; i < animation.actors.length; i++) {
            if (i > 0)
                button_innards += ", ";
            button_innards += getFixtureById(animation.actors[i]).getNumber();
        }
    }
    else
        button_innards += "NO FIXTURES";

    button_innards += "</div>";

    button_innards += "<div class='animation_button_signal'>";
    button_innards += getSignalSynopsis(animation.signal);
    button_innards += "</div>";

    return button_innards;
}

// ----------------------------------------------------------------------------
// Populate fixture select with currently selected scene fixtures
function populateSceneFixtures(multiselect, animation) {
    multiselect.empty();

    var actors = $("#nsd_fixtures").val();      // Get the currently selected set of fixtures

    if (actors != null) {
        // Fill in the scene's fixures
        for (var i = 0; i < actors.length; i++) {
            var fixture_id = actors[i];
            var fixture = getFixtureById(fixture_id);
            var is_selected = false;

            for (var j = 0; j < animation.actors.length; j++)
                if (animation.actors[j] == fixture_id) {
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

    multiselect.multiselect({ minWidth: 500, multiple: true, noneSelectedText: 'select fixtures' });
}

// ----------------------------------------------------------------------------
// 
function generateFixtureTiles(order_div, animation) {
    order_div.empty();
    var ul = $("<ul class='sortable_fixtures'>");
    for (var i = 0; i < animation.actors.length; i++) {
        var fixture = getFixtureById(animation.actors[i]);
        var label = (fixture.isGroup()) ? "G" : "F";

        var li = $("<li class='ui-state-default sortable_fixture'>" + label + fixture.getNumber() + "</li>");
        li.attr('title', fixture.getFullName());
        li.data('fixture_id', fixture.getId());
        ul.append(li);
    }
    order_div.append(ul);

    order_div.find(".sortable_fixtures").sortable().disableSelection();
}

// ----------------------------------------------------------------------------
// Populate fixture select with currently selected scene fixtures
function populateFixtureOrder(fixture_multiselect, order_div, animation) {
    generateFixtureTiles(order_div, animation);

    fixture_multiselect.unbind("multiselectclose");

    fixture_multiselect.bind("multiselectclose", function (event, ui) {
        stopEventPropagation(event);
        var fixture_ids = fixture_multiselect.val();

        if (fixture_ids == null) {
            animation.actors = [];
        }
        else if (animation.actors.length == 0) {
            animation.actors = fixture_ids;
        }
        else {
            animation.actors = [];

            $.each(order_div.find("li"), function (key, value) {
                var fixture_id = $(value).data('fixture_id');

                for (var j = 0; j < fixture_ids.length; j++) {
                    if (fixture_id == fixture_ids[j]) {
                        fixture_ids.splice(j, 1);
                        animation.actors.push(fixture_id);
                        break;
                    }
                }
            });

            $.each(fixture_ids, function (key, value) {
                animation.actors.push(parseInt(value));
            });
        }

        generateFixtureTiles(order_div, animation);
    });
}

// ----------------------------------------------------------------------------
//
function populateSignal(prefix, signal) {
    var container = $("#" + prefix + "_signal");
    var signal_html = animation_signal_template.innerHTML.replace(/NNN/g, prefix);

    var input_click_handler = function (type) {
        var apply_to_div = $("#" + prefix + "_apply_to_div");
        var sample_decay_ms_div = $("#" + prefix + "_sample_decay_ms_div");
        var frequency_div = $("#" + prefix + "_frequency_div");
        var random_div = $("#" + prefix + "_random_div");
        var level_div = $("#" + prefix + "_level_div");

        if (type == 1) {
            apply_to_div.hide();
            sample_decay_ms_div.hide();
        }
        else {
            apply_to_div.show();
            sample_decay_ms_div.show();
        }

        switch (type) {
            case 1:             // Timer
                frequency_div.hide();
                random_div.hide();
                level_div.hide();
                break;

            case 2:             // Sound Level
            case 3:             // Avg. Sound Level
                frequency_div.hide();
                random_div.hide();
                level_div.show();
                break;

            case 4:             // Frequency Beat
                frequency_div.show();
                random_div.hide();
                level_div.hide();
                break;

            case 5:             // Frequency Level
                frequency_div.show();
                random_div.hide();
                level_div.show();
                break;

            case 6:             // Random value
                frequency_div.hide();
                random_div.show();
                level_div.hide();
                break;
        }
    }

    container.empty();
    container.html(signal_html);

    var signal_type = $("#" + prefix + "_signal_type");
    signal_type.empty();

    for (var i = 0; i < AnimationSignalInput.length; i++) {
        signal_type.append($('<option>', {
            value: i+1,
            text: AnimationSignalInput[i],
            selected: i + 1 == signal.input_type
        }));
    }

    signal_type.multiselect({ minWidth: 300, multiple: false, selectedList: 1, header: false, height: "auto" });
    signal_type.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        input_click_handler(parseInt(ui.value));
    });

    var sample_rate_ms = $("#" + prefix + "_sample_rate_ms");
    sample_rate_ms.spinner({ min: 1, max: 10000 }).val(signal.sample_rate_ms);

    var sample_decay_ms = $("#" + prefix + "_sample_decay_ms");
    sample_decay_ms.spinner({ min: 1, max: 10000 }).val(signal.sample_decay_ms);

    var scale_factor = $("#" + prefix + "_scale_factor");
    scale_factor.spinner({ min: 1, max: 10 }).val(signal.scale_factor);

    var max_threshold = $("#" + prefix + "_max_threshold");
    max_threshold.spinner({ min: 1, max: 100 }).val(signal.max_threshold);

    var input_low = $("#" + prefix + "_input_low");
    input_low.spinner({ min: 1, max: 28000 }).val(signal.input_low );
    var input_high = $("#" + prefix + "_input_high");
    input_high.spinner({ min: 1, max: 28000 }).val(signal.input_high);

    var random_low = $("#" + prefix + "_random_low");
    random_low.spinner({ min: 1, max: 255 }).val(signal.input_low);
    var random_high = $("#" + prefix + "_random_high");
    random_high.spinner({ min: 1, max: 255 }).val(signal.input_high);

    var apply_to = $("#" + prefix + "_apply_to");
    apply_to.empty();
    for (var i = 0; i < ApplySignal.length; i++) {
        apply_to.append($('<option>', {
            value: i + 1,
            text: ApplySignal[i],
            selected: i + 1 == signal.apply_to
        }));
    }
    apply_to.multiselect({ minWidth: 300, multiple: false, selectedList: 1, header: false, height: "auto" });

    input_click_handler(signal.input_type);
}

// ----------------------------------------------------------------------------
//
function updateSignal(prefix, signal_target) {
    var signal = {
        input_type: parseInt( $("#" + prefix + "_signal_type").val() ),
        sample_rate_ms: parseInt( $("#" + prefix + "_sample_rate_ms").val() ),
        sample_decay_ms: parseInt( $("#" + prefix + "_sample_decay_ms").val() ),
        scale_factor: parseInt( $("#" + prefix + "_scale_factor").val() ),
        max_threshold: parseInt($("#" + prefix + "_max_threshold").val()),
        input_low: parseInt( $("#" + prefix + "_input_low").val() ),
        input_high:parseInt(  $("#" + prefix + "_input_high").val() ),
        apply_to: parseInt( $("#" + prefix + "_apply_to").val() )
    };

    switch (signal.input_type) {
        case 4:             // Frequency Beat
        case 5:             // Frequency Level
            if (signal.input_low > signal.input_high) {
                messageBox("Low frequency must by less than high frequency");
                return false;
            }
            break;

        case 6:             // Random value
            signal["input_low"] = parseInt($("#" + prefix + "_random_low").val());
            signal["input_high"] = parseInt($("#" + prefix + "_random_high").val());

            if (signal.input_low > signal.input_high) {
                messageBox("Random from value must by less than random to value");
                return false;
            }
            break;
    }

    $.extend(signal_target, signal);

    return true;
}

// ----------------------------------------------------------------------------
//
function updateSceneFixtures(multiselect, animation) {
    animation.actors = multiselect.val();
    if (animation.actors == null)
        animation.actors = [];
}

// ----------------------------------------------------------------------------
//
function updateOrderedSceneFixtures(order_div, animation) {
    animation.actors = [];

    $.each(order_div.find("li"), function (key, value) {
        animation.actors.push( $(value).data('fixture_id') );
    });
}

// ----------------------------------------------------------------------------
//
function editFixtureSequencer(anim_info, animation, success_callback) {
    var edit_dialog = $("#edit_" + anim_info.class_name + "_dialog");
    var prefix = "essd";

    animation.signal.input_type = 1;

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 620,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                updateOrderedSceneFixtures($("#" + prefix + "_fixture_order"), animation);
                animation.FixtureSequencer = {};
                success_callback(animation);
                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateFixtureOrder($("#" + prefix + "_fixtures"), $("#" + prefix + "_fixture_order"), animation);

    populateSignal( prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#" + prefix + "_signal_type_div").css( 'display', 'none' );

    edit_dialog.dialog("open");
}

// ----------------------------------------------------------------------------
//
function editSceneSoundLevel(anim_info, animation, success_callback) {
    var edit_dialog = $("#edit_" + anim_info.class_name + "_dialog");
    var prefix = "essl";

    if (animation.SoundLevel == null)
        animation.SoundLevel = {
            fade_what: 1
        };

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 620,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                animation.SoundLevel.fade_what = $("#essl_fade_what").val();

                updateSceneFixtures($("#" + prefix + "_fixtures"), animation);
                success_callback(animation);

                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#essl_fade_what").empty();
    for (var i = 0; i < FadeWhat.length; i++) {
        $("#essl_fade_what").append($('<option>', {
            value: i + 1,
            text: FadeWhat[i],
            selected: i + 1 == animation.SoundLevel.fade_what
        }));
    }
    $("#essl_fade_what").multiselect({ minWidth: 350, multiple: false, selectedList: 1, header: false, height: "auto" });

    edit_dialog.dialog("open");
}

function getSceneSoundLevelSynopsis(animation) {
    return "Fade: " + FadeWhat[animation.SoundLevel.fade_what - 1];
}

// ----------------------------------------------------------------------------
//
function editScenePatternDimmer(anim_info, animation, success_callback) {
    var edit_dialog =  $("#edit_" + anim_info.class_name + "_dialog" );
    var prefix = "espd";

    if (animation.ScenePatternDimmer == null)
        animation.ScenePatternDimmer = {
            dimmer_pattern: 1
        };

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 620,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                updateOrderedSceneFixtures($("#" + prefix + "_fixture_order"), animation);
                animation.ScenePatternDimmer.dimmer_pattern = $("#espd_pattern").val();
                success_callback(animation);
                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateFixtureOrder($("#" + prefix + "_fixtures"), $("#" + prefix + "_fixture_order"), animation);
    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#espd_pattern").empty();
    for (var i = 0; i < PatternDimmer.length; i++) {
        $("#espd_pattern").append($('<option>', {
            value: i + 1,
            text: PatternDimmer[i],
            selected: i + 1 == animation.ScenePatternDimmer.dimmer_pattern
        }));
    }
    $("#espd_pattern").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });

    edit_dialog.dialog("open");
}

function getScenePatternDimmerSynopsis(animation) {
    return "Pattern: " + PatternDimmer[animation.ScenePatternDimmer.dimmer_pattern - 1];
}

// ----------------------------------------------------------------------------
//
function editSceneStrobeAnimator(anim_info, animation, success_callback) {
    var edit_dialog = $("#edit_" + anim_info.class_name + "_dialog");
    var prefix = "essa";

    if (animation.SceneStrobeAnimator == null)
        animation.SceneStrobeAnimator = {
            strobe_neg_color: "#000000",
            strobe_pos_ms: 50,
            strobe_neg_ms: 50
        };

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 620,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                animation.SceneStrobeAnimator.strobe_pos_ms = $("#essa_strobe_pos_ms").val();
                animation.SceneStrobeAnimator.strobe_neg_ms = $("#essa_strobe_neg_ms").val();
                animation.SceneStrobeAnimator.strobe_neg_color = $("#essa_colorpicker").attr('value');
                updateSceneFixtures($("#" + prefix + "_fixtures"), animation);

                success_callback(animation);

                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#essa_strobe_pos_ms").spinner({ min: 1, max: 60000 }).val(animation.SceneStrobeAnimator.strobe_pos_ms);
    $("#essa_strobe_neg_ms").spinner({ min: 1, max: 60000 }).val(animation.SceneStrobeAnimator.strobe_neg_ms);
    $("#essa_colorpicker").css("background-color", "#" + animation.SceneStrobeAnimator.strobe_neg_color);
    $("#essa_colorpicker").attr('value', animation.SceneStrobeAnimator.strobe_neg_color);

    $("#essa_colorpicker").ColorPicker({
        color: "#000000",
        livePreview: true,
        autoClose: true,

        onShow: function (picker) {
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            $("#essa_colorpicker").css("background-color", "#" + hex);
            $("#essa_colorpicker").attr('value', hex);
        }
    });
    $("#essa_colorpicker").ColorPickerSetColor(animation.SceneStrobeAnimator.strobe_neg_color);

    edit_dialog.dialog("open");
}

function getSceneStrobeAnimatorSynopsis(animation) {
    return "Strobe: on " + animation.SceneStrobeAnimator.strobe_pos_ms + "ms off " +
           animation.SceneStrobeAnimator.strobe_neg_ms + "ms color " +
           colorName( animation.SceneStrobeAnimator.strobe_neg_color );
}

// ----------------------------------------------------------------------------
//
function editSceneColorSwitcher(anim_info, animation, success_callback) {
    var edit_dialog = $("#edit_" + anim_info.class_name + "_dialog");
    var prefix = "escs";

    if (animation.SceneColorSwitcher == null)
        animation.SceneColorSwitcher = {
            strobe_neg_color: "#000000",
            strobe_pos_ms: 50,
            strobe_neg_ms: 50,
            color_progression: []
        };

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 750,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                animation.SceneColorSwitcher.fader_effect = $("#escs_fader_effect").val();
                animation.SceneColorSwitcher.strobe_pos_ms = $("#escs_strobe_pos_ms").val();
                animation.SceneColorSwitcher.strobe_neg_ms = $("#escs_strobe_neg_ms").val();
                animation.SceneColorSwitcher.strobe_neg_color = $("#escs_colorpicker").attr('value');
                animation.SceneColorSwitcher.color_progression = $("#escs_color_progression").expandableContainer("values");

                updateSceneFixtures($("#" + prefix + "_fixtures"), animation);
                success_callback(animation);

                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#escs_strobe_pos_ms").spinner({ min: 1, max: 60000 }).val(animation.SceneColorSwitcher.strobe_pos_ms);
    $("#escs_strobe_neg_ms").spinner({ min: 1, max: 60000 }).val(animation.SceneColorSwitcher.strobe_neg_ms);
    $("#escs_colorpicker").css("background-color", "#" + animation.SceneColorSwitcher.strobe_neg_color);
    $("#escs_colorpicker").attr('value', animation.SceneColorSwitcher.strobe_neg_color);

    $("#escs_colorpicker").ColorPicker({
        color: "#000000",
        livePreview: true,

        onShow: function (picker) {
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            $("#escs_colorpicker").css("background-color", "#" + hex);
            $("#escs_colorpicker").attr('value', hex);
        }
    });
    $("#escs_colorpicker").ColorPickerSetColor(animation.SceneColorSwitcher.strobe_neg_color);

    $("#escs_color_progression").expandableContainer({
        item_template: '<div class="container_input" style="height:30px; width:30px; border: 1px solid #808080; border-radius: 6pt 6pt; cursor: pointer;"></div>',
        new_item_callback: function (item, value) {
            if (value == null)
                value = '#000000';

            var chip = item.find(".container_input");
            chip.attr('value', value);
            chip.css('background-color', "#" + value);

            chip.ColorPicker({
                color: value,
                livePreview: true,
                autoClose: true,

                onShow: function (picker) {
                    $(picker).fadeIn(500);
                    return false;
                },

                onHide: function (picker) {
                    $(picker).fadeOut(500);
                    return false;
                },

                onSubmit: function (hsb, hex, rgb) {
                    chip.css("background-color", "#" + hex);
                    chip.attr('value', hex);
                }
            });
            chip.ColorPickerSetColor(value);
        }
    });

    $("#escs_color_progression").expandableContainer("set_values", animation.SceneColorSwitcher.color_progression);

    $("#escs_fader_effect").empty();
    for (var i = 0; i < FaderEffect.length; i++) {
        $("#escs_fader_effect").append($('<option>', {
            value: i + 1,
            text: FaderEffect[i],
            selected: i + 1 == animation.SceneColorSwitcher.fader_effect
        }));
    }
    $("#escs_fader_effect").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });

    edit_dialog.dialog("open");
}

function getSceneColorSwitcherSynopsis(animation) {
    var synopsis = "Fade: " + FaderEffect[animation.SceneColorSwitcher.fader_effect-1] +
                   "\nStrobe: on " + animation.SceneColorSwitcher.strobe_pos_ms + "ms, off " +
                    animation.SceneColorSwitcher.strobe_neg_ms + "ms, off color " + colorName( animation.SceneColorSwitcher.strobe_neg_color );

    if (animation.SceneColorSwitcher.color_progression != null && animation.SceneColorSwitcher.color_progression.length > 0) {
        synopsis += "\nColors: ";
        for ( var i=0; i < animation.SceneColorSwitcher.color_progression.length; i++ ) {
            if ( i > 0 )
                synopsis += ", ";
            synopsis += colorName( animation.SceneColorSwitcher.color_progression[i] );
        }
    }

    return synopsis;
}

// ----------------------------------------------------------------------------
//
function add_new_animation(class_name, success_callback) {
    var fixtures = $("#nsd_fixtures").val();
    if (fixtures == null)
        fixtures = [];

    var animation = {
        class_name: class_name,
        name: findAnimation(class_name).name,
        number: 0,
        actors: fixtures,
        new_animation: true,
        signal: {
            sample_rate_ms: 100,
            input_type: 1,
            input_low: 1,
            input_high: 1,
            sample_decay_ms: 0,
            scale_factor: 1,
            max_threshold: 100,
            apply_to: 1
        }
    };

    // Unshred the movement animation if needed
    if (class_name.indexOf(MOVEMENT_ANIMATOR_ROOT) == 0) {
        var movement_type = parseInt(class_name.substring(MOVEMENT_ANIMATOR_ROOT.length + 1));
        animation.class_name = MOVEMENT_ANIMATOR_ROOT;
        animation.SceneMovementAnimator = {
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
        }
    }

    show_animation_edit_dialog(animation, success_callback);
}

// ----------------------------------------------------------------------------
//
function show_animation_edit_dialog(original_animation, success_callback) {
    var anim_info = findAnimation(original_animation);

    if (anim_info != null) {
        var animation = jQuery.extend(true, {}, original_animation);
        anim_info.editor(anim_info, animation, success_callback);
    }
}

// ----------------------------------------------------------------------------
//
function editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc) {
    var edit_dialog = $("#edit_SceneMovementAnimator_dialog");
    var prefix = "esma";

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 810,
        modal: true,
        resizable: false,
        title: (animation.new_animation ? "Create " : "Edit ") + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                updateOrderedSceneFixtures($("#" + prefix + "_fixture_order"), animation);

                animation.SceneMovementAnimator.speed = $("#esma_speed").spinner("value");
                animation.SceneMovementAnimator.head_number = $("#esma_head").spinner("value");
                animation.SceneMovementAnimator.dest_wait_periods = $("#esma_dest_wait_periods").spinner("value");
                animation.SceneMovementAnimator.home_wait_periods = $("#esma_home_wait_periods").spinner("value");
                animation.SceneMovementAnimator.group_size = $("#esma_group_size").spinner("value");
                animation.SceneMovementAnimator.run_once = $("#esma_run_once").is(':checked');
                animation.SceneMovementAnimator.blackout_return = $("#esma_blackout_return").is(':checked');
                animation.SceneMovementAnimator.alternate_groups = $("#esma_alternate_groups").is(':checked');
                animation.new_animation = false;

                if (!updateFunc(edit_dialog, animation))
                    return;

                success_callback(animation);

                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateFixtureOrder($("#" + prefix + "_fixtures"), $("#" + prefix + "_fixture_order"), animation);
    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#esma_speed").spinner({ min: 0, max: 255 }).val(animation.SceneMovementAnimator.speed);
    $("#esma_head").spinner({ min: 0, max: 32 }).val(animation.SceneMovementAnimator.head_number);
    $("#esma_dest_wait_periods").spinner({ min: 1, max: 10000 }).val(animation.SceneMovementAnimator.dest_wait_periods);
    $("#esma_run_once").attr('checked', animation.SceneMovementAnimator.run_once);
    $("#esma_home_wait_periods").spinner({ min: 1, max: 100 }).val(animation.SceneMovementAnimator.home_wait_periods);
    $("#esma_blackout_return").attr('checked', animation.SceneMovementAnimator.blackout_return);
    $("#esma_group_size").spinner({ min: 1, max: 1000 }).val(animation.SceneMovementAnimator.group_size);
    $("#esma_alternate_groups").attr('checked', animation.SceneMovementAnimator.alternate_groups);

    // Only reveal the movement div for this movement
    for (var i = 1; i <= 7; i++) {
        if (i != animation.SceneMovementAnimator.movement_type)
            $("#esma_SceneMovementAnimator" + i).hide();
        else
            $("#esma_SceneMovementAnimator" + i).show();
    }

    $("#esma_home_wait_periods_container").hide();
    $("#esma_blackout_return_container").hide();
    $("#esma_group_size_container").hide();
    $("#esma_alternate_groups_container").hide();
    $("#esma_fixture_order_container").hide();

    setupFunc(edit_dialog);

    edit_dialog.dialog("open");
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
// Random movement animator
//
function editSceneMovementAnimator1(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_group_size_container").show();

        $("#esma1_positions").spinner({ min: 1, max: 200 }).val(animation.SceneMovementAnimator.positions);
        $("#esma1_pan_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_start_angle);
        $("#esma1_pan_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_end_angle);
        $("#esma1_tilt_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_start_angle);
        $("#esma1_tilt_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_end_angle);
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.positions = $("#esma1_positions").spinner("value");
        animation.SceneMovementAnimator.pan_start_angle = $("#esma1_pan_start_angle").spinner("value");
        animation.SceneMovementAnimator.pan_end_angle = $("#esma1_pan_end_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_start_angle = $("#esma1_tilt_start_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_end_angle = $("#esma1_tilt_end_angle").spinner("value");
        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}

function getSceneMovementAnimator1Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) +
                   ", Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   ", Group size: " + animation.SceneMovementAnimator.group_size + ", Positions: " + animation.SceneMovementAnimator.positions;
    return synopsis;
}

// ----------------------------------------------------------------------------
// Fan movement animator
//
function editSceneMovementAnimator2(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_home_wait_periods_container").show();
        $("#esma_blackout_return_container").show();
        $("#esma_fixture_order_container").show();

        $("#esma2_pan_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_start_angle);
        $("#esma2_pan_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_end_angle);
        $("#esma2_pan_increment").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_increment);
        $("#esma2_tilt_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_start_angle);
        $("#esma2_tilt_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_end_angle);
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.pan_start_angle = $("#esma2_pan_start_angle").spinner("value");
        animation.SceneMovementAnimator.pan_end_angle = $("#esma2_pan_end_angle").spinner("value");
        animation.SceneMovementAnimator.pan_increment = $("#esma2_pan_increment").spinner("value");
        animation.SceneMovementAnimator.tilt_start_angle = $("#esma2_tilt_start_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_end_angle = $("#esma2_tilt_end_angle").spinner("value");
        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}

function getSceneMovementAnimator2Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) +
                   ", Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Increment: " + animation.SceneMovementAnimator.pan_increment +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   ", Return blackout: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ----------------------------------------------------------------------------
// Rotate movement animator
//
function editSceneMovementAnimator3(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_home_wait_periods_container").show();
        $("#esma_blackout_return_container").show();
        $("#esma_group_size_container").show();
        $("#esma_alternate_groups_container").show();
        $("#esma_fixture_order_container").show();

        $("#esma3_pan_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_start_angle);
        $("#esma3_pan_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_end_angle);
        $("#esma3_tilt_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_start_angle);
        $("#esma3_tilt_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_end_angle);
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.pan_start_angle = $("#esma3_pan_start_angle").spinner("value");
        animation.SceneMovementAnimator.pan_end_angle = $("#esma3_pan_end_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_start_angle = $("#esma3_tilt_start_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_end_angle = $("#esma3_tilt_end_angle").spinner("value");
        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}
    
function getSceneMovementAnimator3Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) +
                   ", Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   "," + (animation.SceneMovementAnimator.alternate_groups ? " Alternate" : "") + " Groups: " + animation.SceneMovementAnimator.group_size +
                   ", Return blackout: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ----------------------------------------------------------------------------
// Up/down movement animator
//
function editSceneMovementAnimator4(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_home_wait_periods_container").show();
        $("#esma_blackout_return_container").show();
        $("#esma_group_size_container").show();
        $("#esma_alternate_groups_container").show();
        $("#esma_fixture_order_container").show();

        $("#esma4_pan_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_start_angle);
        $("#esma4_pan_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_end_angle);
        $("#esma4_pan_increment").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_increment);
        $("#esma4_tilt_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_start_angle);
        $("#esma4_tilt_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_end_angle);
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.pan_start_angle = $("#esma4_pan_start_angle").spinner("value");
        animation.SceneMovementAnimator.pan_end_angle = $("#esma4_pan_end_angle").spinner("value");
        animation.SceneMovementAnimator.pan_increment = $("#esma4_pan_increment").spinner("value");
        animation.SceneMovementAnimator.tilt_start_angle = $("#esma4_tilt_start_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_end_angle = $("#esma4_tilt_end_angle").spinner("value");
        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}

function getSceneMovementAnimator4Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) +
                   ", Pan: " + animation.SceneMovementAnimator.pan_start_angle + "-" + animation.SceneMovementAnimator.pan_end_angle +
                   ", Increment: " + animation.SceneMovementAnimator.pan_increment +
                   ", Tilt: " + animation.SceneMovementAnimator.tilt_start_angle + "-" + animation.SceneMovementAnimator.tilt_end_angle +
                   "," + (animation.SceneMovementAnimator.alternate_groups ? " Alternate" : "") + " Groups: " + animation.SceneMovementAnimator.group_size +
                   ", Return blackout: " + animation.SceneMovementAnimator.blackout_return;
    return synopsis;
}

// ----------------------------------------------------------------------------
// Beam cross movement animator
//
function editSceneMovementAnimator5(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_home_wait_periods_container").show();
        $("#esma_blackout_return_container").show();
        $("#esma_group_size_container").show();
        $("#esma_alternate_groups_container").show();
        $("#esma_fixture_order_container").show();

        $("#esma5_pan_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_start_angle);
        $("#esma5_pan_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_end_angle);
        $("#esma5_tilt_start_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_start_angle);
        $("#esma5_tilt_end_angle").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.tilt_end_angle);
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.pan_start_angle = $("#esma5_pan_start_angle").spinner("value");
        animation.SceneMovementAnimator.pan_end_angle = $("#esma5_pan_end_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_start_angle = $("#esma5_tilt_start_angle").spinner("value");
        animation.SceneMovementAnimator.tilt_end_angle = $("#esma5_tilt_end_angle").spinner("value");
        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}

function getSceneMovementAnimator5Synopsis(animation) {
    return getSceneMovementAnimator3Synopsis(animation)
}

// ----------------------------------------------------------------------------
// Programmed movement animator
//
function editSceneMovementAnimator6(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_blackout_return_container").show();

        $("#esma6_coordinates").expandableContainer({
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
            $("#esma6_coordinates").expandableContainer("set_values", animation.SceneMovementAnimator.coordinates[i]);
        }
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.coordinates = [];
        $("#esma6_coordinates").expandableContainer("values", function (item) {
            var pan = item.find(".pan").spinner("value");
            var tilt = item.find(".tilt").spinner("value");
            animation.SceneMovementAnimator.coordinates.push({ "pan": pan, "tilt": tilt });
        });

        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}

function getSceneMovementAnimator6Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) +
                   ", Coordinates: ";

    for (var i = 0; i < animation.SceneMovementAnimator.coordinates.length; i++) {
        if (i > 0)
            synopsis += ", ";
        synopsis += "(" + animation.SceneMovementAnimator.coordinates[i].pan + ", " + animation.SceneMovementAnimator.coordinates[i].tilt + ")";
    }

    return synopsis;
}

// ----------------------------------------------------------------------------
// Moonflower movement animator
//
function editSceneMovementAnimator7(anim_info, animation, success_callback) {
    var setupFunc = function (dialog) {
        $("#esma_home_wait_periods_container").show();
        $("#esma_fixture_order_container").show();

        $("#esma7_pan_increment").spinner({ min: 0, max: 720 }).val(animation.SceneMovementAnimator.pan_increment);
        $("#esma7_home_x").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.home_x);
        $("#esma7_home_y").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.home_y);
        $("#esma7_height").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.height);
        $("#esma7_fixture_spacing").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.fixture_spacing);
        $("#esma7_radius").spinner({ step: 0.1, min: 0}).val(animation.SceneMovementAnimator.radius);
        $("#esma7_positions").spinner({ min: 1, max: 200 }).val(animation.SceneMovementAnimator.positions);
    }

    var updateFunc = function (dialog, animation) {
        animation.SceneMovementAnimator.pan_increment = $("#esma7_pan_increment").spinner("value");
        animation.SceneMovementAnimator.home_x = $("#esma7_home_x").spinner("value");
        animation.SceneMovementAnimator.home_y = $("#esma7_home_y").spinner("value");
        animation.SceneMovementAnimator.height = $("#esma7_height").spinner("value");
        animation.SceneMovementAnimator.fixture_spacing = $("#esma7_fixture_spacing").spinner("value");
        animation.SceneMovementAnimator.radius = $("#esma7_radius").spinner("value");
        animation.SceneMovementAnimator.positions = $("#esma7_positions").spinner("value");
        return true;
    }

    editSceneMovementAnimator(anim_info, animation, success_callback, setupFunc, updateFunc);
}
        
function getSceneMovementAnimator7Synopsis(animation) {
    var synopsis = movementSynopsis(animation.SceneMovementAnimator) +
                   ", Height: " + animation.SceneMovementAnimator.height + "'" +
                   ", Spacing: " + animation.SceneMovementAnimator.fixture_spacing + "'" +
                   ", Radius: " + animation.SceneMovementAnimator.radius + "'" +
                   ", Home: " + animation.SceneMovementAnimator.home_x + "," + animation.SceneMovementAnimator.home_y +
                   ", Positions: " + animation.SceneMovementAnimator.positions +
                   ", Increment: " + animation.SceneMovementAnimator.pan_increment;

    return synopsis;
}

// ----------------------------------------------------------------------------
//
function editSceneChannelAnimator(anim_info, animation, success_callback) {
    var edit_dialog = $("#edit_" + anim_info.class_name + "_dialog");
    var prefix = "esca";

    if (animation.SceneChannelAnimator == null) {
        animation.SceneChannelAnimator = {
            channel_animations: []
        };
    }

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 720,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                animation.SceneChannelAnimator.channel_animations = [];
                animation.actors = [];

                $("#esca_animations").expandableContainer("values", function (item) {
                    var esca_fixture = item.find(".esca_fixture");
                    var esca_channel = item.find(".esca_channel");
                    var esca_style = item.find(".esca_style");
                    var esca_channel_list = item.find(".esca_channel_list");
                    var esca_channel_range_start = item.find(".esca_channel_range_start");
                    var esca_channel_range_end = item.find(".esca_channel_range_end");

                    var channel_animation = {
                        style: parseInt( esca_style.val() ),
                        values: [],
                        channel: parseInt(esca_channel.val() ),
                        actor_uid: parseInt(esca_fixture.val())
                    };

                    if (channel_animation.style == 1) {
                        channel_animation.values = esca_channel_list.expandableContainer("values");
                    }
                    else if (channel_animation.style == 2) {
                        channel_animation.values.push(esca_channel_range_start.spinner("value"));
                        channel_animation.values.push(esca_channel_range_end.spinner("value"));
                    }

                    animation.SceneChannelAnimator.channel_animations.push(channel_animation);

                    // Finally, update actors if needed
                    for (var i = 0; i < animation.actors.length; i++)
                        if (animation.actors[i] == channel_animation.actor_uid)
                            return;

                    animation.actors.push(channel_animation.actor_uid);
                });

                success_callback(animation);

                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#esca_animations").expandableContainer({
        item_template: esca_channel_template.innerHTML,
        vertical: true,
        controls: "right",
        new_item_callback: new_channel_animation
    });

    $("#esca_animations").expandableContainer("set_values", animation.SceneChannelAnimator.channel_animations );

    edit_dialog.dialog("open");
}

function new_channel_animation(item, channel_animation) {
    var actors = $("#nsd_fixtures").val();      // Get the currently selected set of fixtures

    if (channel_animation == null) {
        channel_animation = {
            style: 1,
            values: [0, 255],
            channel: 0,
            actor_uid: actors.length > 0 ? actors[0] : 0
        }

        // Default to last values
        $("#esca_animations").expandableContainer("values", function (item) {
            var esca_fixture = item.find(".esca_fixture");
            var esca_channel = item.find(".esca_channel");
            var esca_style = item.find(".esca_style");

            channel_animation.style = parseInt(esca_style.val());
            channel_animation.channel = parseInt(esca_channel.val());
            channel_animation.actor_uid = parseInt(esca_fixture.val());
        });
    }

    var esca_fixture = item.find(".esca_fixture");
    var esca_channel = item.find(".esca_channel");
    var esca_style = item.find(".esca_style");
    var esca_channel_list_container = item.find(".esca_channel_list_container");
    var esca_channel_list = item.find(".esca_channel_list");
    var esca_channel_range_container = item.find(".esca_channel_range_container" );
    var esca_channel_range_start = item.find(".esca_channel_range_start");
    var esca_channel_range_end = item.find(".esca_channel_range_end");

    if (actors != null) {
        // Fill in the scene's fixures
        for (var i = 0; i < actors.length; i++) {
            var fixture_id = actors[i];
            var fixture = getFixtureById(fixture_id);

            esca_fixture.append($('<option>', {
                value: fixture_id,
                text: fixture.getLabel(),
                selected: channel_animation.actor_uid == fixture_id
            }));
        }
    }

    esca_fixture.multiselect({ minWidth: 500, multiple: false, selectedList: 1, header: false, noneSelectedText: 'select fixture', height: "auto" });

    var update_channels = function (fixture_id, selected) {
        esca_channel.empty();

        var fixture = getFixtureById(fixture_id);

        if (fixture != null ) {
            var channels = fixture.getChannels();

            for (var i = 0; i < channels.length; i++) {
                esca_channel.append($('<option>', {
                    value: i,
                    text: "Channel " + (i+1) + ": " + channels[i].name,
                    selected: i == selected
                }));
            }
        }

        esca_channel.multiselect("refresh");
    };

    esca_fixture.bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        update_channels(ui.value, 0);
    });

    esca_channel.multiselect({ minWidth: 500, multiple: false, selectedList: 1, header: false, noneSelectedText: 'select channel', height: "auto" });

    update_channels( esca_fixture.val(), channel_animation.channel );

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

// ----------------------------------------------------------------------------
//
function getSceneChannelAnimatorSynopsis(animation) {
    var synopsis = "";

    for (var i = 0; i < animation.SceneChannelAnimator.channel_animations.length; i++) {
        if (i > 0)
            synopsis += " \n";

        var channel_animation = animation.SceneChannelAnimator.channel_animations[i];
        var fixture = getFixtureById(channel_animation.actor_uid);

        var label = (fixture.isGroup()) ? "Fixture G" : "Fixture ";

        synopsis += label + fixture.getNumber() + ": " + 
                    "Channel " + (channel_animation.channel + 1) + ", " + AnimationStyle[channel_animation.style - 1];

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
function editPixelAnimator(anim_info, animation, success_callback) {
    var edit_dialog = $("#edit_" + anim_info.class_name + "_dialog");
    var prefix = "espa";

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

    edit_dialog.dialog({
        autoOpen: false,
        width: 780,
        height: 780,
        modal: true,
        resizable: false,
        title: "Edit " + anim_info.name,
        buttons: {
            OK: function () {
                if (!updateSignal(prefix, animation.signal))
                    return;

                animation.ScenePixelAnimator.pixel_effect = $("#espa_pixel_effect").val();
                animation.ScenePixelAnimator.pixel_off_color = $("#espa_pixel_off_color").attr('value');
                animation.ScenePixelAnimator.generations = $("#espa_generations").val();
                animation.ScenePixelAnimator.pixels = $("#espa_pixels").val();
                animation.ScenePixelAnimator.increment = $("#espa_increment").val();
                animation.ScenePixelAnimator.fade = $("#espa_fade").is(':checked');
                animation.ScenePixelAnimator.combine = $("#espa_combine").is(':checked');
                animation.ScenePixelAnimator.color_progression = $("#espa_color_progression").expandableContainer("values");

                updateOrderedSceneFixtures($("#" + prefix + "_fixture_order"), animation);
                success_callback(animation);

                edit_dialog.dialog("close");
            },
            Cancel: function () {
                edit_dialog.dialog("close");
            }
        }
    });

    populateSceneFixtures($("#" + prefix + "_fixtures"), animation);
    populateFixtureOrder($("#" + prefix + "_fixtures"), $("#" + prefix + "_fixture_order"), animation);

    populateSignal(prefix, animation.signal);
    $("#" + prefix + "_description").text(anim_info.description);

    $("#espa_generations").spinner({ min: 1, max: 500 }).val(animation.ScenePixelAnimator.generations);
    $("#espa_pixels").spinner({ min: 1, max: 1024 }).val(animation.ScenePixelAnimator.pixels);
    $("#espa_increment").spinner({ min: 1, max: 1024 }).val(animation.ScenePixelAnimator.increment);
    $("#espa_fade").attr('checked', animation.ScenePixelAnimator.fade);
    $("#espa_combine").attr('checked', animation.ScenePixelAnimator.combine);

    $("#espa_pixel_off_color").css("background-color", "#" + animation.ScenePixelAnimator.pixel_off_color);
    $("#espa_pixel_off_color").attr('value', animation.ScenePixelAnimator.pixel_off_color);

    $("#espa_pixel_off_color").ColorPicker({
        color: "#000000",
        livePreview: true,

        onShow: function (picker) {
            $(picker).fadeIn(500);
            return false;
        },

        onHide: function (picker) {
            $(picker).fadeOut(500);
            return false;
        },

        onSubmit: function (hsb, hex, rgb) {
            $("#espa_pixel_off_color").css("background-color", "#" + hex);
            $("#espa_pixel_off_color").attr('value', hex);
        }
    });
    $("#espa_pixel_off_color").ColorPickerSetColor(animation.ScenePixelAnimator.pixel_off_color);

    $("#espa_color_progression").expandableContainer({
        item_template: '<div class="container_input" style="height:30px; width:30px; border: 1px solid #808080; border-radius: 6pt 6pt; cursor: pointer;"></div>',
        new_item_callback: function (item, value) {
            if (value == null)
                value = '#000000';

            var chip = item.find(".container_input");
            chip.attr('value', value);
            chip.css('background-color', "#" + value);

            chip.ColorPicker({
                color: value,
                livePreview: true,
                autoClose: true,

                onShow: function (picker) {
                    $(picker).fadeIn(500);
                    return false;
                },

                onHide: function (picker) {
                    $(picker).fadeOut(500);
                    return false;
                },

                onSubmit: function (hsb, hex, rgb) {
                    chip.css("background-color", "#" + hex);
                    chip.attr('value', hex);
                }
            });
            chip.ColorPickerSetColor(value);
        }
    });
    $("#espa_color_progression").expandableContainer("set_values", animation.ScenePixelAnimator.color_progression);

    $("#espa_pixel_effect").empty();
    for (var i = 0; i < PixelEffect.length; i++) {
        $("#espa_pixel_effect").append($('<option>', {
            value: i + 1,
            text: PixelEffect[i],
            selected: i + 1 == animation.ScenePixelAnimator.pixel_effect
        }));
    }
    $("#espa_pixel_effect").multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, height: "auto" });

    var generations_div = $("#espa_generations_div");
    var pixels_div = $("#espa_pixels_div");
    var increment_div = $("#espa_increment_div");
    var fade_div = $("#espa_fade_div");

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
        }
    }

    $("#espa_pixel_effect").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        update_effect(ui.value);
    });

    update_effect(animation.ScenePixelAnimator.pixel_effect);

    edit_dialog.dialog("open");
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