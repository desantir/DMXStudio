/* 
Copyright (C) 2012-17 Robert DeSantis
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

var track_control = null;

var show_login_dialog = true;

// ----------------------------------------------------------------------------
//
function initializeMusicPlayer() {
    track_control = new TrackControl( "track_controls", "track_status" );

    $("#playlist_play").button().click(playlistPlay);
    $("#playlist_queue").button().click(playlistQueue);
    $("#playlist_tracks").button().click(playlistTracks);
    
    $('#track_select').css('display', 'block');

    initializePlaylistSelector( $("#playlist_list" ), 'small_multilist', 340, function ( playlist_id ) {
        if ( getCurrentPlaylist() != playlist_id ) {
            setCurrentPlaylist( playlist_id, null );
            client_config_update = true;
        }
    });
}

// ----------------------------------------------------------------------------
//
function updatePlayerStatus(music_player_status) {
    if ( !music_player_status.logged_in && show_login_dialog ) {
        show_login_dialog = false;  // Only once
        player_login(music_player_status.player_name, music_player_status.username);
    }

    track_control.updateUI( music_player_status);
}

// ----------------------------------------------------------------------------
//
function pauseTrackEvent( track_link ) {
    track_control.pauseTrackEvent( track_link );
}

// ----------------------------------------------------------------------------
//
function stopTrackEvent( track_link ) {
    track_control.stopTrackEvent( track_link );
}

// ----------------------------------------------------------------------------
//
function resumeTrackEvent( track_link ) {
    track_control.resumeTrackEvent( track_link );
}

// ----------------------------------------------------------------------------
//
function startTrackEvent( track_link, position_ms ) {
    track_control.startTrackEvent( track_link, position_ms );
}

// ----------------------------------------------------------------------------
//
function timeTrackEvent( track_link, position_ms ) {
    track_control.timeTrackEvent( track_link, position_ms );
}

// ----------------------------------------------------------------------------
//
function changeTrackQueueEvent( played_size, queued_size ) {
    track_control.changeTrackQueueEvent( played_size, queued_size );
}

// ----------------------------------------------------------------------------
//
function changeCurrentPlaylist(playlist) {
    setCurrentPlaylist( playlist, "playlist_list" );
}

// ----------------------------------------------------------------------------
//
function playlistPlay(event) {
    stopEventPropagation(event);

    track_control.playlist_play( $("#playlist_list").val() );
}

// ----------------------------------------------------------------------------
//
function playlistQueue(event) {
    stopEventPropagation(event);

    track_control.playlist_queue( $("#playlist_list").val() );
}

// ----------------------------------------------------------------------------
//
function playlistTracks(event) {
    stopEventPropagation(event);

    var playlist_link = $("#playlist_list").val();

    if (playlist_link != null && playlist_link.length > 0 )
        showTrackList( );
}

// ----------------------------------------------------------------------------
//
function player_login( player_name, username ) {

    $("#music_player_login_dialog").dialog({
        autoOpen: false,
        width: 540,
        height: 360,
        modal: true,
        resizable: false,
        title: player_name + " Login",
        buttons: {
            "Login": function () {
                var json = {
                    username: $("#mpl_username").val(),
                    password: $("#mpl_password").val(),
                };

                $("#mpl_error").empty();

                var ice_dialog = put_user_on_ice("Working ... Please wait");

                $.ajax({
                    type: "POST",
                    url: "/dmxstudio/rest/control/music/player/login/",
                    data: JSON.stringify(json),
                    contentType: 'application/json',
                    cache: false,
                    async: true,
                    success: function (data) {
                        ice_dialog.dialog("close");

                        var json = jQuery.parseJSON(data);
                        if (json.logged_in) {
                            $("#music_player_login_dialog").dialog("close");
                            return;
                        }

                        $("#mpl_error").text(json.login_error);
                        $("#mpl_password").val("");
                    },

                    error: onAjaxError
                });
            },
            Cancel: function () {
                $(this).dialog("close");
            }
        }
    });

    $("#mpl_error").empty();
    $("#mpl_password").empty();
    $("#mpl_username").val(username);

    $("#music_player_login_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function showTrackList( ) {
    $("#track_list_dialog").dialog({
        modal: false,
        height: 550,
        width: 1250,
        title: "Playlist",
        open: function () { // Stop main body scroll
            // $("body").css("overflow", "hidden");
            
            new TrackSelector( $(this), track_control, changeCurrentPlaylist, null );
        },
        close: function () {
            // $("body").css("overflow", "auto");
            
            track_control.setNotifyCallback( null );
        },
        buttons: {
            'Play All': function () {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/0",
                    cache: false,
                    success: function() { track_control.updateUI(); },
                    error: onAjaxError
                });
                $(this).dialog("close");
            },
            'Queue All': function () {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/1",
                    cache: false,
                    success: function() { track_control.updateUI(); },
                    error: onAjaxError
                });
                $(this).dialog("close");
            },
            'Close': function () {
                $(this).dialog("close");
            }
        }
    });
}

// ----------------------------------------------------------------------------
//
var std_playlist = null;
var std_track = null;

function selectTrackDialog( callback ) {
    var select_track_dialog = $("#select_track_dialog");

    select_track_dialog.dialog({
        autoOpen: false,
        width: 800,
        height: 300,
        modal: true,
        buttons:  [ 
            {
            id: "std_select_track",
            text: "Select Track",
            click: function () {
                    select_track_dialog.dialog("close");
                    callback( std_playlist, std_track );
                }
            },
            {
            text: "Close",
            click: function () {
                    select_track_dialog.dialog("close");
                }
            }
        ]
    });

    if ( select_track_dialog.dialog("isOpen")) {
         select_track_dialog.dialog("close");
        return;
    }

    var populate_tracks = function( playlist ) {
       std_playlist = playlist;

       $("#std_tracks").empty();

       if ( playlist == null) {
            $("#std_tracks").multiselect("refresh");
            return;
       }

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                $.each( json.tracks, function (index, track) {
                    $("#std_tracks").append($('<option>', {
                        value: index,
                        text: track.full_name.substring(0, 200),
                        selected: false,
                        data: { "track": track }
                    }));
                });

                $("#std_tracks").multiselect("refresh");
                $("#std_tracks").multiselect("uncheckAll");
            },
            error: onAjaxError
        });
    }

    $("#std_tracks").multiselect({
        minWidth: 600, multiple: false, selectedList: 1, header: "Tracks", noneSelectedText: 'select track', classes: 'small_multilist', height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);

        var options = $("#std_tracks option");
        std_track = $(options[ui.value]).data("track");
        $("#std_select_track").button("enable");
    });

    // Populate playlists
    initializePlaylistSelector( $("#std_playlists" ), 'small_multilist', 600, function ( playlist_id ) {
        $("#std_select_track").button("disable");
        $("#std_tracks").empty();

        populate_tracks( playlist_id );
    });

    select_track_dialog.dialog("open");
}
