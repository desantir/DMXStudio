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

// ----------------------------------------------------------------------------
//
var frequency_plot = null;

function showFrequencyVisualizer() {
    stopEventPropagation();

    // Setup frequency visualizer dialog
    $("#frequency_visualizer_dialog").dialog({
        autoOpen: false,
        width: 1100,
        height: 420,
        modal: false,
        close: function () {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/soundsampler/stop/",
                cache: false
            });
            $("#show_frequency_visualizer").removeClass("lg-icon-white").addClass("lg-icon-grey");
        }
    });

    if ($("#frequency_visualizer_dialog").dialog("isOpen")) {
        $("#frequency_visualizer_dialog").dialog("close");
        return;
    }

    initializeHorizontalSlider("frequency_sample_rate", 25, 1000, 50);

    var update_frequency_sample_rate = function () {
        $("#frequency_sample_value").html($("#frequency_sample_rate").slider("value") + " ms");
    }

    update_frequency_sample_rate();

    $("#frequency_sample_rate").unbind("slide slidestop").on("slide slidestop", update_frequency_sample_rate);

    $("#show_frequency_visualizer").addClass("lg-icon-white").removeClass("lg-icon-grey");

    $("#frequency_visualizer_dialog").dialog("open");

    var sound_data = [];
    for (var i = 1; i <= 29; i++)
        sound_data[i] = [i, 0];

    var options = {
        colors: ["rgba(0,0,255,1)"],
        xaxis: {
            max: 30,
            ticks: [[1, "32"], [2, "64"], [3, "128"], [4, ".3k"], [5, ".5k"], [6, ".8k"], [7, "1k"], [8, "2k"], [9, "4k"], [10, "5k"], [11, "8k"], [12, "10k"], [13, "16k"], [14, "20k"],
                    [15, ""],
                    [16, "32"], [17, "64"], [18, "128"], [19, ".3k"], [20, ".5k"], [21, ".8k"], [22, "1k"], [23, "2k"], [24, "4k"], [25, "5k"], [26, "8k"], [27, "10k"], [28, "16k"], [29, "20k"]
            ]
        },
        yaxis: {
            max: 100,
            tickFormatter: function (val, axis) {
                return val + "db";
            }
        }
    };

    frequency_plot = $.plot($("#frequency_visualizer_chart"), [
        {
            data: sound_data,
            bars: { show: true, fill: true, fillColor: "rgba(0,0,255,0.4)", barWidth: 0.4 }
        },
    ], options
    );

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/soundsampler/start/",
        cache: false,
        success: function () {
            setTimeout(updateFrequencies, 200);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function updateFrequencies() {
    if (!$("#frequency_visualizer_dialog").dialog("isOpen"))
        return;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/soundsampler/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            var sound_data = [];
            var offset = 0;
            for (var i = 1; i <= 28; i++) {
                if (i == 15) {                  // Add separator for channels (all I can think of)
                    sound_data[i] = [15, -1];
                    offset = 1;
                }

                sound_data[i + offset] = [i + offset, json.audio_data[i - 1]];
            }

            frequency_plot.setData([{
                data: sound_data,
                bars: { show: true, fill: true, fillColor: "rgba(0,0,255,0.4)", barWidth: 0.4 }
            }]);
            frequency_plot.draw();

            $("#frequency_sample_number").html(json.sample_number);

            setTimeout(updateFrequencies, $("#frequency_sample_rate").slider("value"));
        },
        error: function () {
            $("#frequency_visualizer_dialog").dialog("close");;
        }
    });
}
