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

// ----------------------------------------------------------------------------
//
var AMPLITUDE_SAMPLES = 250;
var SAMPLE_INTERVAL_MS = 10;

var amplitude_plot = null;
var amplitude_data = null;
var avg_amplitude_data = null;
var beat_data = null;
var last_index = 0;

function showAmplitudeVisualizer() {
    stopEventPropagation();

    // Setup amplitude visualizer dialog
    $("#amplitude_visualizer_dialog").dialog({
        autoOpen: false,
        width: 1040,
        height: 430,
        modal: false,
        close: function () {
            $("#show_amplitude_visualizer").removeClass("lg-icon-white").addClass("lg-icon-grey");
        }
    });

    if ($("#amplitude_visualizer_dialog").dialog("isOpen")) {
        $("#amplitude_visualizer_dialog").dialog("close");
        return;
    }

    $("#show_amplitude_visualizer").addClass("lg-icon-white").removeClass("lg-icon-grey");

    $("#amplitude_visualizer_dialog").dialog("open");

    var options = {
        colors: ["rgba(0,0,255,1)", "rgba(0,196,0,1)", "rgba(255,0,0,0.5)" ],
        xaxis: {
			tickFormatter: function() {
				return "";
			}
        },
        yaxis: {
            ticks: 12,
            max: 1200,
            min: 0,
            tickFormatter: function (val, axis) {
                return val;
            }
        },
		legend: {
			show: true,
            position: "ne",
            noColumns: 0,
            labelBoxBorderColor: "black",
            margin: 2
		}
    };

    last_index = 0;
    amplitude_data = [];
    avg_amplitude_data = [];
    beat_data = [];

    amplitude_plot = $.plot( $("#amplitude_visualizer_chart"), getSeries(), options );

    updateAmplitudes();
}

// ----------------------------------------------------------------------------
//
function updateAmplitudes() {
    if (!$("#amplitude_visualizer_dialog").dialog("isOpen"))
        return;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/amplitude/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            if ( amplitude_data.length == AMPLITUDE_SAMPLES ) {
                amplitude_data.splice( 0, 1 );
                avg_amplitude_data.splice( 0, 1 );
                beat_data.splice( 0, 1 );
            }

            amplitude_data.push( json.amplitude );
            avg_amplitude_data.push( json.avg_amplitude );
            beat_data.push( ( json.beat > 0 && last_index != json.index ) ? json.amplitude : -1 )

            last_index = json.index;

            amplitude_plot.setData( getSeries() );
            amplitude_plot.draw();

            setTimeout(updateAmplitudes, SAMPLE_INTERVAL_MS);
        },

        error: function () {
            $("#amplitude_visualizer_dialog").dialog("close");
        }
    });
}

function getSeries() {
    var series1 = [];
    var series2 = [];
    var series3 = [];

    var i;

    for ( i=0; i < amplitude_data.length; i++ ) {
        series1.push( [ i, amplitude_data[i] ] );
        series2.push( [ i, avg_amplitude_data[i] ] );
        series3.push( [ i, beat_data[i] ] );
    }

    while ( i < AMPLITUDE_SAMPLES ) {
        series1.push( [ i, -1 ] );
        series2.push( [ i, -1 ] );
        series2.push( [ i, -1 ] );
        i++;
    }

    return [ {
            data: series1,
            label: "amplitude", 
            lines: { show: true, fill: true, fillColor: "rgba(0,0,255,0.4)" }
        },
        {
            data: series2,
            label: "avg. amplitude", 
            lines: {  show: true, fill: false }
        },
        {
            label: "beat",
            data: series3,
            points: { show: true, radius: 3, lineWidth: 1, fill: true, fillColor: "rgba(255,0,0,0.5)", symbol: "circle" }
        }
    ];
}
