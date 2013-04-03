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

var beat_frequencies = [
    // { start_freq: 64, end_freq: 64, beat: false },
    // { start_freq: 128, end_freq: 128, beat: false },
    // { start_freq: 300, end_freq: 300, beat: false },
    // { start_freq: 500, end_freq: 500, beat: false },
    { start_freq: 64, end_freq: 500, beat: false },

    // { start_freq: 800, end_freq: 800, beat: false },
    // { start_freq: 1000, end_freq: 1000, beat: false },
    { start_freq: 800, end_freq: 1000, beat: false },

    { start_freq: 2000, end_freq: 2000,beat: false },
    { start_freq: 4000, end_freq: 4000, beat: false },
    { start_freq: 5000, end_freq: 5000, beat: false },
    { start_freq: 8000, end_freq: 8000, beat: false },
    { start_freq: 10000, end_freq: 10000,beat: false },
    { start_freq: 16000, end_freq: 16000, beat: false }
];

var vizUpdaterInterval = null;
var beat_objects = [];

// ----------------------------------------------------------------------------
//
function showBeatVisualizer() {
    stopEventPropagation();

    // Setup beat visualizer dialog
    $("#beat_visualizer_dialog").dialog({
        autoOpen: false,
        width: 1000,
        height: 200,
        modal: false,
        close: function () {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/full/control/beatsampler/stop/",
                cache: false
            });
            $("#show_beat_visualizer").removeClass("lg-icon-white").addClass("lg-icon-grey");
        }
    });

    if ($("#beat_visualizer_dialog").dialog("isOpen")) {
        $("#beat_visualizer_dialog").dialog("close");
        clearInterval(vizUpdaterInterval);
        return;
    }

    $("#show_beat_visualizer").addClass("lg-icon-white").removeClass("lg-icon-grey");

    $("#beat_visualizer_data").empty();

    var html = "";

    for (var i = 0; i < beat_frequencies.length; i++) {
        html += "<div style='float:left; width:100px; height: 100px; margin: 15px 0px 15px 20px;'>";
        html += "<canvas id='beat_bin_" + i + "' width='100' height='100'></canvas>";
        html += "<div style='font-size: 8pt; color: #808080; width: 100px; text-align: center;'>" + beat_frequencies[i].start_freq + "-" + beat_frequencies[i].end_freq + " Hz</div>";
        html += "</div>";
    }

    $("#beat_visualizer_data").append(html);

    beat_objects = [];
    for (var i = 0; i < beat_frequencies.length; i++)
        beat_objects.push($("#beat_bin_" + i));

    $("#beat_visualizer_dialog").dialog("open");

    $.ajax({
        type: "POST",
        url: "/dmxstudio/full/control/beatsampler/start/",
        data: JSON.stringify(beat_frequencies),
        contentType: 'application/json',
        cache: false,
        success: function () {
            setTimeout(updateBeats, 1);
        },
        error: onAjaxError
    });

    vizUpdaterInterval = setInterval(updateBeatsUI, 25);
}

var BEAT_ANIMATION = [
    { radius: 10, color: "rgb(0,0,255)" },
    { radius: 20, color: "rgb(0,0,80)" },
    { radius: 30, color: "rgb(0,0,180)" },
    { radius: 40, color: "rgb(0,0,255)" },
    { radius: 50, color: "rgb(0,0,255)" },
    { radius: 30, color: "rgb(0,0,180)" }
];

// var beat_decay_ms = 200;

// ----------------------------------------------------------------------------
//
function updateBeatsUI() {
    for ( var index=0; index < beat_frequencies.length; index++ ) {
        // Start new beat sequence
        if (beat_objects[index].beat && !beat_objects[index].beating) {
            beat_objects[index].beat = false;
            beat_objects[index].stage = 0;
            beat_objects[index].beating = true;
        }

        if (beat_objects[index].beating) {
            var ctx = beat_objects[index].get(0).getContext('2d');
            ctx.clearRect(0, 0, 100, 100);

            if (beat_objects[index].stage >= BEAT_ANIMATION.length) {
                beat_objects[index].beating = false;
            }
            else {
                var anim = BEAT_ANIMATION[beat_objects[index].stage];

                ctx.fillStyle = anim.color;

                ctx.beginPath();
                ctx.arc(50, 50, anim.radius, 0, Math.PI * 2, true);       // angle -> rads PI/180*degrees
                ctx.fill();

                beat_objects[index].stage += 1;
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
function updateBeats() {
    if (!$("#beat_visualizer_dialog").dialog("isOpen"))
        return;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/full/query/beatsampler/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            $.map(json, function (beat_info, index) {
                if (beat_info.beat)
                    beat_objects[index].beat = true;
            });

            setTimeout(updateBeats, 100);
        },
        error: function () {
            setTimeout(restartBeats, 1000);
        }
    });
}

// ----------------------------------------------------------------------------
//
function restartBeats() {
    $.ajax({
        type: "POST",
        url: "/dmxstudio/full/control/beatsampler/start/",
        data: JSON.stringify(beat_frequencies),
        contentType: 'application/json',
        cache: false,
        success: function () {
            setTimeout(updateBeats, 1);
        },
        error: function () {
            setTimeout(restartBeats, 1000);
        }
    });
}
