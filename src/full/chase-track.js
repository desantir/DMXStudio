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

var myTrack = null;
var myPlaylist = null;
var myPosition = 0;
var myInterval = 0;
var myAmplitudeData = null;
var myBar = null;
var myLastStartPosition = 0;
var myInitialized = false;
var barWidth = 2;

// ----------------------------------------------------------------------------
//
function chaseTrack( event ) {
    stopEventPropagation( event );

    var chase_track_dialog = $("#chase_track_dialog");

    chase_track_dialog.dialog({
        autoOpen: false,
        width: 1030,
        height: 500,
        modal: false,
        open: function () {
            $("#ctd_visualizer").track_visualize_control( "option", "bar_height_zoom", 1 );      // Also resizes!

            $("#ctd_width_" + barWidth).prop("checked", true).button("refresh");
        },
        close: function () {
            $("#ctd_visualizer").track_visualize_control("stop");
        },
        resizeStop: function( event, ui ) {
            $("#ctd_visualizer").track_visualize_control( "option", "bar_height_zoom", chase_track_dialog.height() / 500 );       // Also resizes!
        }
    });

    if ( myInitialized ) {
        chase_track_dialog.dialog("open");
        return;
    }

    $("#ctd_visualizer").track_visualize_control({
        BAR_BOX_WIDTH: barWidth,
        EDGE_BORDER: 10,
        amplitude_data: myAmplitudeData,
        no_data_message: "SELECT TRACK TO CHASE",

        onclick: function (event, bar) {
            if ( bar != null && bar.selected ) {
                $("#ctd_selected_time").text( trackTime(bar.number * myInterval, true ) )

                $("#ctd_scene_select").show();
                $("#ctd_selected").show();

                myBar = bar;

                if ( bar.annotation != null && bar.annotation.data != null ) {
                    multiselectSelect( $("#ctd_scenelist"), bar.annotation.data.scene_uid );
                    $("#ctd_method_" + bar.annotation.data.load_method).prop("checked", true).button("refresh");
                }
            }
            else {
                $("#ctd_scene_select").hide();
                $("#ctd_selected").hide();
            
                myBar = null;
            }
        },

        onplay: function (state, bar, time_mark_ms) {

            // If we are over 2 seconds into the track, make sure we are tracking to 
            // the timecode from the server
            if ( time_mark_ms > myLastStartPosition + 2000 ) {
                if ( playing_track_link != myTrack.link ) {
                    chaseStopTrack( true );
                    return true;
                }

                // The server timecode is a bit laggy due to polling and work done per poll
                // so allow us to be a little off.

                if ( playing_track_position + (myInterval*4) < time_mark_ms ) {
                   $("#ctd_visualizer").track_visualize_control("play", time_mark_ms-(myInterval*2) );
                    return true;
                }
                else if ( playing_track_position - (myInterval*2) > time_mark_ms ) {
                    $("#ctd_visualizer").track_visualize_control("play", playing_track_position );
                    return true;
                }
            }

            myPosition = time_mark_ms;

            $("#ctd_position").text(trackTime(myPosition, true));
            $("#ctd_remaining").text("-" + trackTime(myTrack.duration - myPosition, true ));

            if ( bar != null && bar.annotation != null ) {
                var scene_uid = bar.annotation.data.scene_uid;
                var method = bar.annotation.data.load_method;
                
                setTimeout( function() {
                    $.ajax({
                        type: "GET",
                        url: "/dmxstudio/rest/control/scene/stage/" + scene_uid + "/" + method,
                        cache: false,
                        success: function () {
                            if ( method == 1 )
                                markActiveScene(scene_uid);
                        },
                        error: onAjaxError
                    });
                }, 0 );
            }

            return false;
        }
    });

    $("#ctd_select_track").button().click(function () {
        selectTrackDialog( function( playlist, track ) {
            myTrack = track;
            myPosition = 0;

            $("#ctd_visualizer").track_visualize_control( "option", "no_data_message", "NO DATA - PLAY FULL TRACK FIRST" );

            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/query/music/track/analyze/" + myTrack.link,
                cache: false,
                success: function (data) {
                    var json = jQuery.parseJSON(data);

                    myAmplitudeData = json.amplitude.data;
                    myInterval = json.amplitude.duration_ms;    // Sampling interval

                    chaseSetupTrack( myTrack, myAmplitudeData, myInterval );

                    // Check for a pre-existing chase for this track
                    $.ajax({
                        type: "GET",
                        url: "/dmxstudio/rest/query/music/matcher/search/" + myTrack.link,
                        cache: false,
                        success: function (data) {
                            var json = jQuery.parseJSON(data);

                            if ( json.length != 1 || json[0].type != 2 )
                                return;
    
                            var chase = getChaseById( json[0].id );
                            if ( chase == null )
                                return;
                            
                            // Load annotations
                            var time_ms = 0;
                            for ( var i=0; i < chase.getSteps().length; i++ ) {
                                var step = chase.getSteps()[i];
                                annotate_bar( time_ms/myInterval, step.id, step.load_method );
                                time_ms += step.delay_ms;
                            }
                        },
                        error: function () {
                        },
                    });
                },
                error: function () {
                    myAmplitudeData = null;
                    myInterval = 500;

                    chaseSetupTrack( myTrack, myAmplitudeData, myInterval );
                },
            });
        });
    });

    // Fill in scenes
    $("#ctd_scenelist").empty();

    for (var i = 0; i < scenes.length; i++) {
        $("#ctd_scenelist").append($('<option>', {
            value: scenes[i].getId(),
            text: scenes[i].getNumber() + ": " + scenes[i].getFullName(),
            selected: false
        }));
    }

    $("#ctd_scenelist").multiselect({
        classes: 'player_multilist',
        header: "Scenes", 
        noneSelectedText: 'select scene',
        minWidth: 450,
        multiple: false,
        selectedList: 1,
        header: false
    });
    
    $("#ctd_scenelist").multiselect("refresh");

    $("#ctd_play").button().click(function () {
        if ( myBar != null )
            myPosition = myBar.number * myInterval;

        myLastStartPosition = myPosition;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/track/" + myTrack.link + "/" + myPosition,
            cache: false,
            success: function(data) {
                $("#ctd_visualizer").track_visualize_control("play", myPosition );
                $("#ctd_stop").button("enable");
                $("#ctd_reset").button("enable");
            },
            error: onAjaxError
        });
    });

    $("#ctd_stop").button().click(function () {
        chaseStopTrack( false );
    });

    $("#ctd_reset").button().click(function () {
        chaseStopTrack( true );
    });

    $("#ctd_set_scene").button().click(function () {
        if ( myBar != null ) {
            var sceneId = $("#ctd_scenelist").val();
            var method = $("input[name=ctd_method]:checked").val();
                
            if ( annotate_bar( myBar.number, sceneId, method ) )
                $("#ctd_generate_container").show();
        }
    });

    $("#ctd_clear_scene").button().click(function () {
        if ( myBar != null ) {
            $("#ctd_visualizer").track_visualize_control( "clearAnnotation", myBar.number );

            var bars = $("#ctd_visualizer").track_visualize_control( "getBarData" );
            var haveData = false;

            for ( var i=0; i < bars.length; i++ ) {
                if ( bars[i].annotation != null && bars[i].annotation.data != null )
                    haveData = true;
            }

            if ( !haveData )
                $("#ctd_generate_container").hide();
        }
    });

    $("#ctd_generate").button().click(function () {
        generateChase();
    });

    $("#ctd_bar_widths").buttonset().on("change", function() {
        var value = $("input[name=ctd_width]:checked").val();
        $("#ctd_visualizer").track_visualize_control( "option", "BAR_BOX_WIDTH", parseInt(value) );
    });

    $("#ctd_method_div").buttonset();

    chase_track_dialog.dialog("open");

    myInitialized = true;
}

// ----------------------------------------------------------------------------
//
function annotate_bar( bar_number, sceneId, method ) {
    var scene = getSceneById( sceneId );
    if ( scene == null )
        return false;

    var action = "";

    if ( method == 2 )
        action = "Add ";
    else if ( method == 3 )
        action = "Remove ";

    var title = action + scene.getNumber() + ": " + scene.getFullName();

    $("#ctd_visualizer").track_visualize_control( "annotate", bar_number, title, { "scene_uid": sceneId, "load_method": method } );

    return true;
}

// ----------------------------------------------------------------------------
//
function chaseStopTrack( resetPosition ) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/track/stop/",
        cache: false,
        complete: function ( jqXHR, status ) {
            $("#ctd_visualizer").track_visualize_control("stop");

            if ( resetPosition ) {
                $("#ctd_visualizer").track_visualize_control( "clearHighlight" );
                $("#ctd_visualizer").track_visualize_control( "unselectAll" );

                myPosition = 0;
                myBar = null;

                $("#ctd_position").text(trackTime(0), true);
                $("#ctd_remaining").text(trackTime(myTrack.duration), true);
            }

            if ( status != "success" )
                onAjaxError();
        }
    });
}

// ----------------------------------------------------------------------------
//
function chaseSetupTrack( track, amplitude_data, interval_ms ) {
    $("#ctd_visualizer").track_visualize_control("stop");
    $("#ctd_visualizer").track_visualize_control("load", amplitude_data, interval_ms );

    $("#ctd_track_name").text( track.full_name );

    var bpm = track.audio_info != null ? "|&nbsp;&nbsp;" + Math.round(track.audio_info.bpm) + "&nbsp;BPM" : "";
    $("#ctd_bpm").html( bpm );

    $("#ctd_position").text(trackTime(0), true);
    $("#ctd_remaining").text( "-" + trackTime(track.duration), true);
    $("#ctd_length").text(trackTime(track.duration),false);

    $("#ctd_track_info").show();

    $("#ctd_play").button( "enable" );
    $("#ctd_generate_container").hide();
    $("#ctd_scene_select").hide();
    $("#ctd_selected").hide();

    if ( amplitude_data != null )
        $("#ctd_track_timing").show();
    else
       $("#ctd_track_timing").hide();
}

// ----------------------------------------------------------------------------
//
function generateChase() {
    var new_number = getUnusedChaseNumber();

    var bars = $("#ctd_visualizer").track_visualize_control( "getBarData" );
    var steps = [];
    var time_ms = 0;

    // Loop though all the bars looking for those with a scene annotation
    for ( var i=0; i < bars.length; i++ ) {
        if ( bars[i].annotation != null && bars[i].annotation.data != null ) {
            if ( steps.length > 0 )
                steps[ steps.length-1 ].delay_ms = time_ms;

            steps[ steps.length] = { id: bars[i].annotation.data.scene_uid, delay_ms: 0, load_method: bars[i].annotation.data.load_method };

            time_ms = 0;        // Reset the time to - this will be come our duration
        }
        else if ( i == 0 ) // If no starting scene, start with default
            steps[ steps.length] = { id: getDefaultSceneId(), delay_ms: 0 };

        time_ms += myInterval;  // Add one duration (usually 500ms) per bar
    }

    if ( steps.length > 0 )
        steps[ steps.length-1 ].delay_ms = time_ms;

    var chase_name = "Chase: " + myTrack.track_name;
    var chase_num = 1;

    while ( getChaseByName( chase_name ) != null )
        chase_name = "Chase: " + myTrack.track_name + " (" + chase_num++ + ")";

    openNewChaseDialog("Generate Chase", {
        id: 0,
        name: chase_name,
        description: "Automatically generated",
        number: new_number,
        fade_ms: 0,
        delay_ms: 0,
        steps: steps,
        repeat: false, 
        acts: current_act == 0 ? [] : [current_act],
        callback: function( chase_id ) {
            if ( $("#ctd_generate_mm").is(":checked") ) {
                var json = [ {
                    id: chase_id, 
                    track: myTrack.full_name,
                    link: myTrack.link,
                    type: 2
                } ];

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/edit/music/matcher/update",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    cache: false,
                    success: function () {
                        ;
                    },
                    error: onAjaxError
                });
            }
        }
    });
}