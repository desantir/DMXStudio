/* 
Copyright (C) 2017 Robert DeSantis
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
var SAMPLE_INTERVAL_MS = 50;
var SAMPLE_DATA_POINTS = 200;

var time_mark_ms = 0;
var level_plot = null;
var chart_series = [];
var options;
var series_colors;
var xaxis_ms = 0;                      // X-axis miliseconds - 0=on change
var xaxis_label;

// ----------------------------------------------------------------------------
//
function showAnimationLevelVisualizer() {
    stopEventPropagation();

    var visualizer = $("#animation_level_visualizer_dialog");
    
    xaxis_label = visualizer.find( ".amplitude_xaxis" );

    // Setup amplitude visualizer dialog
    visualizer.dialog({
        autoOpen: false,
        width: 1090,
        height: 500,
        modal: false,
        close: function () {
        }
    });

    if ( visualizer.dialog("isOpen") ) {
        visualizer.dialog("close");
        return;
    }

    visualizer.dialog("open");

    $( "#animation_time_select" ).multiselect({ minWidth: 200, multiple: false, selectedList: 1, header: false, classes: "strobe_custom_dropdown" });
    $( "#animation_time_select" ).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        xaxis_ms = parseInt( ui.value );
        resetChart();
    });

    resetChart();

    updateAnimationLevels();
}

// ----------------------------------------------------------------------------
//
function findChartSeries( animation_id ) {
    var index = findChartSeriesIndex( animation_id );
    return index >= 0 ? chart_series[index] : null;
}

// ----------------------------------------------------------------------------
//
function findChartSeriesIndex( animation_id ) {
    for ( var s=0; s < chart_series.length; s++ ) {
        if ( chart_series[s].animation_id == animation_id )
            return s;
    }
    return -1;
}

// ----------------------------------------------------------------------------
//
function resetChart() {
    series_colors = [ "rgba(0,0,255,1)", "rgba(0,196,0,1)", "rgba(255,0,0,1)", "rgba(255,0,255,1)", "rgba(255,255,0,1)", "rgb( 255,255,255,1)" ];
    time_mark_ms = 0;
    chart_series = [];
    chart_bucket_ms = 0;

    options = {
        xaxis: {
            max: 0,
            tickFormatter: function (val, axis) {
                return val;
            }
        },
        yaxis: {
            ticks: MAXIMUM_LEVEL / 20,
            max: MAXIMUM_LEVEL,
            min: 0,
            tickFormatter: function (val, axis) {
                return val;
            }
        },
        legend: {
            show: true,
            noColumns: 0,
            labelBoxBorderColor: "black",
            container: $("#animation_level_legend")
        }
    };

    if ( xaxis_ms == 0 ) {
        xaxis_label.html( "Last " + SAMPLE_DATA_POINTS + " Level Changes" );

        options.xaxis.min = -SAMPLE_DATA_POINTS;
        options.xaxis.ticks = SAMPLE_DATA_POINTS / 10;
        options.xaxis.tickFormatter = function (val, axis) { return val; }
    }
    else {
        xaxis_label.html( "Time (seconds)" );  

        options.xaxis.min = -SAMPLE_DATA_POINTS * xaxis_ms;
        options.xaxis.ticks = SAMPLE_DATA_POINTS / 10;
        options.xaxis.tickFormatter = function (val, axis) { 
            return (val/1000);
        }
    }

    level_plot = $.plot( $("#animation_level_chart"), chart_series, options );
}

// ----------------------------------------------------------------------------
//
function updateSeries() {
    var scene_animation_refs = [];
    var new_chart_series = [];
    var changed = false;
    var scene = getSceneById( active_scene_id );
    
    if ( scene != null )
        scene_animation_refs = scene.getAnimationRefs();

    for ( var i=0; i < scene_animation_refs.length; i++ ) {
        var animation = getAnimationById( scene_animation_refs[i].id );

        // Animations that do not use the signal level
        if ( animation.getClassName() == "SceneStrobeAnimator" )
            continue;
        
        var series_index = findChartSeriesIndex( animation.getId() );
        if ( series_index == -1 ) {
            var color = null;

            if ( series_colors.length > 0 ) {
                color = series_colors[0];
                series_colors.splice( 0, 1 );
            }

            var series = {
                color: color,
                animation_id: animation.getId(),
                data: [],
                label: animation.getName(), 
                lines: { show: true, fill: false }
            };

            // Populate all the data points
            var tick_inc = ( xaxis_ms == 0 ) ? 1 : xaxis_ms;
            var tick_value = -(SAMPLE_DATA_POINTS * tick_inc);

            for ( var j=0; j <= SAMPLE_DATA_POINTS; j++ ) {
                series.data.push( [ tick_value, -1 ] );
                tick_value += tick_inc;
            }

            new_chart_series.push( series );
            changed = true;
        }
        else {
            new_chart_series.push( chart_series[series_index] );
            chart_series.splice( series_index, 1 );
        }
    }

    // Return unused colors
    for ( var u=chart_series.length; u-- > 0; )
        if ( chart_series[u].color != null )
            series_colors.splice( 0, 0, chart_series[u].color );
    
    changed |= ( chart_series.length > 0 );     // We have deleted series

    chart_series = new_chart_series;

    if ( changed ) {
        $("#animation_level_legend").html( "" );
        level_plot = $.plot( $("#animation_level_chart"), chart_series, options );
    }
}

// ----------------------------------------------------------------------------
//
function updateAnimationLevels() {
    if (!$("#animation_level_visualizer_dialog").dialog("isOpen"))
        return;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/animationlevels/0",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            // Handle any removed or new series
            updateSeries();

            updateSeriesData( json );

            if ( chart_series.length > 0 ) {
                level_plot.setData( chart_series );
                level_plot.draw();
            }

           setTimeout(updateAnimationLevels, SAMPLE_INTERVAL_MS);
        },

        error: function () {
            $("#animation_level_visualizer_dialog").dialog("close");
        }
    });
}

// ----------------------------------------------------------------------------
//
function updateSeriesData( data ) {
    // Update each data series from the new data: [{"animation_id": 994,"data": [{"ms": 257285421,"level": 100}, ...
    for ( var i=0; i < data.length; i++ ) {
        var series = findChartSeries( data[i].animation_id );
        if ( series == null )
            continue;

        var new_data = data[i].data;
        if ( new_data.length == 0 )
            continue;

        // Make sure we didn't get any old data (we shouldn't but just in case)
        var index = 0;
        var num_new_samples = new_data.length;

        // Put the samples in buckets based on selected X-axis time period
        if ( xaxis_ms > 0 ) {  
            var bucketized_data = [];

            // If we are missing bucket data points, we assume the same level as the previous bucket
            var last_total = -1;

            var time_bucket_start_ms = new_data[new_data.length-1].ms - ((SAMPLE_DATA_POINTS) * xaxis_ms);
            time_bucket_start_ms -= (time_bucket_start_ms % xaxis_ms);

            while ( index < new_data.length ) {
                var count = 0;
                var total = 0;
                
                while ( index < new_data.length ) {
                    if ( new_data[index].ms >= time_bucket_start_ms + xaxis_ms )
                        break;

                    if ( new_data[index].ms < time_bucket_start_ms ) {
                        index++;
                        continue;
                    }

                    count++;
                    total += new_data[index].level;
                    index++;
                }

                total = (count > 0) ? total/count : -1;
                bucketized_data.push( { level: total, ms: time_bucket_start_ms } );
 
                time_bucket_start_ms += xaxis_ms;
                last_total = total;
            }

            if ( bucketized_data.length > SAMPLE_DATA_POINTS+1 ) {
                toastWarning( "Error: Have " + bucketized_data.length + " buckets" );
                return;
            }

            // Fill in missing samples
            interpolateMissingValues( bucketized_data );

            new_data = bucketized_data;
            num_new_samples = bucketized_data.length;
            index = 0;
        }

        // Don't let the number of sample get larger than SAMPLE_DATA_POINTS+1
        if ( num_new_samples > SAMPLE_DATA_POINTS+1 ) {
            index = new_data.length - SAMPLE_DATA_POINTS - 1;
            num_new_samples = SAMPLE_DATA_POINTS + 1;
        }
        else if ( num_new_samples == 0 )
            continue;

        var tick_inc = ( xaxis_ms == 0 ) ? 1 : xaxis_ms;
        var tick_value = -(SAMPLE_DATA_POINTS * tick_inc);

        series.data = [];

        if ( num_new_samples < SAMPLE_DATA_POINTS+1 ) {
            // Populate the empty data points
            for ( var j=0; j <= SAMPLE_DATA_POINTS-num_new_samples; j++ ) {
                series.data.push( [ tick_value, -1 ] );
                tick_value += tick_inc;
            }
        }

        // Add new data samples
        while ( index < new_data.length ) {
            series.data.push( [ tick_value, new_data[index++].level ] );
            tick_value += tick_inc;
        }
    }
}

// ----------------------------------------------------------------------------
//
function interpolateMissingValues( bucketized_data ) {
    var start_level = -1;

    for ( var index=0; index < bucketized_data.length; index++ ) {
        var level = bucketized_data[index].level;
                
        if ( level == -1 ) {
            if ( start_level == -1 )
                continue;

            for (;;) {
                var count = 1;
                var end_level = start_level;

                while ( index+count < bucketized_data.length ) {
                    if ( bucketized_data[index+count].level != -1 ) {
                        end_level = bucketized_data[index+count].level;
                        break;
                    }
                    count++;
                }

                if ( index+count >= bucketized_data.length )
                    count--;

                // Set the midpoint and go back and fill in the rest
                if ( count > 2 ) {
                    var bucket_index = index + Math.floor(count/2) + 1;

                    try {
                        bucketized_data[bucket_index].level = (end_level+start_level) / 2;   
                    }
                    catch ( error ) {
                        alert( error + "[buckets=" + bucketized_data.length + ", index=" + bucket_index + "]" );
                    }

                    continue;
                }

                try {
                    var mid = (end_level+start_level) / 2;
            
                    if ( count == 2 ) {
                        bucketized_data[index].level = (mid+start_level) / 2;
                        bucketized_data[index+1].level = (mid+end_level) / 2;
                    }
                    else
                        bucketized_data[index].level = mid;
                }
                catch ( error ) {
                    alert( error + "[buckets=" + bucketized_data.length + ", index=" + index + "]" );
                }

                break;
            }

            start_level = end_level;
            index += count;
        }
        else
            start_level = level;
    }
}