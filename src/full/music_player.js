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

var music_player_ui_ready = false;                  // Music player UI and controls intialized
var playing_track = 0;                              // Current music player track

var cache_now_playing = null;
var cache_track_remaining = null;
var cache_track_length = null;
var cache_track_status = null;

// ----------------------------------------------------------------------------
//
function update_player_status(music_player_status) {

    if (music_player_status == null)
        return;

    if (music_player_status.logged_in) {
        if (!music_player_ui_ready)
            initialize_player_ui();

        var current_track = 0;
        var track_name = "";
        var track_length = "";
        var track_remaining = "";
        var track_status = "";
        var paused = false;

        if (music_player_status.playing != null) {
            current_track = music_player_status.playing.track;
            track_name = music_player_status.playing.name;
            track_status = "-" + trackTime(music_player_status.playing.remaining);
            track_length = trackTime(music_player_status.playing.length);
            track_remaining = "remaining " + trackTime(music_player_status.playing.remaining);
            paused = music_player_status.playing.paused;

            cache_track_remaining.text(track_remaining);        // Always update remaining time
            cache_track_status.text(track_status);              // Always update remaining time
        }

        if (current_track != playing_track) {
            cache_now_playing.text(track_name);
            cache_track_remaining.text(track_remaining);
            cache_track_length.text(track_length);
            cache_track_status.text(track_status);
            playing_track = current_track;
        }

        if (current_track == 0) {
            enable_player_button('track_stop', false);
            enable_player_button('track_play', false);
            enable_player_button('track_pause', false);
        }
        else {
            enable_player_button('track_stop', !paused);
            enable_player_button('track_play', paused);
            enable_player_button('track_pause', !paused);
        }

        enable_player_button('track_back', music_player_status.played != 0);
        enable_player_button('track_forward', music_player_status.queued != 0);

        enable_info_button('track_back_info', music_player_status.played != 0);
        enable_info_button('track_forward_info', music_player_status.queued != 0);
    }
    else {
        alert("MUSIC PLAYER LOGIN NOT IMPLEMENTED");
    }

    if (music_player_status.player_error != null) {
        $('#player_error').text(music_player_status.player_error);
    }
    else {
        $('#player_error').text("");
    }
}

// ----------------------------------------------------------------------------
//
function enable_player_button(id, enabled) {

    var player_button = $("#" + id);

    player_button.attr('disabled', !enabled);

    if (enabled)
        player_button.removeClass("lg-icon-disabled").addClass("lg-icon-white");
    else
        player_button.removeClass("lg-icon-white").addClass("lg-icon-disabled");
}
    
// ----------------------------------------------------------------------------
//
function enable_info_button(id, enabled) {

    var info_button = $("#" + id);

    info_button.attr('disabled', !enabled);

    if (enabled)
        info_button.removeClass("ui-icon ui-icon-blank").addClass("ui-icon-white ui-icon-info");
    else
        info_button.removeClass("ui-icon-white ui-icon-info").addClass("ui-icon ui-icon-blank");
}

// ----------------------------------------------------------------------------
//
function initialize_player_ui() {
    if (!cache_now_playing) {
        cache_now_playing = $('#now_playing');
        cache_track_remaining = $('#track_remaining');
        cache_track_length = $('#track_length');
        cache_track_status = $('#track_status');
    }

    cache_now_playing.text( "" );
    cache_track_remaining.text( "" );
    cache_track_length.text("");
    cache_track_status.text("");

    $("#playlist_list").multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'player_multilist', height: 400
    }).bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        selectPlaylist( ui.value );
    });

    $("#track_list").multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: "Playlist Tracks", noneSelectedText: 'select track', classes: 'player_multilist', height: 500
    });

    $("#playlist_play").button().click(playlistPlay);
    $("#playlist_queue").button().click(playlistQueue);

    $("#track_list_play").button().click(tracklistPlay).hide();
    $("#track_list_queue").button().click(tracklistQueue).hide();

    $('#track_back_info').click(track_back_info);
    $('#track_forward_info').click(track_forward_info);
    $('#track_back').click(track_back);
    $('#track_stop').click(track_stop);
    $('#track_play').click(track_play);
    $('#track_pause').click(track_pause);
    $('#track_forward').click(track_forward);

    $('#track_status' ).css('display', 'block');
    $('#track_remaining').css('display', 'block');
    $('#track_select').css('display', 'block');
    $('#track_controls').css('display', 'block');

    // Populate playlists
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/playlists/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            var playlist_list = $("#playlist_list");
            playlist_list.empty();

            $.each(json['playlists'], function (index, playlist) {
                playlist_list.append($('<option>', {
                    value: playlist.id,
                    text: playlist.name.substring( 0, 200 ),
                    selected: index == 0
                }));
            });

            playlist_list.multiselect("refresh");

            // Populate tracks for selected playlist
            var playlist_id = playlist_list.val();
            if (playlist_id != null && playlist_id > 0)
                selectPlaylist(playlist_id);
        },
        error: onAjaxError
    });

    music_player_ui_ready = true;
}

// ----------------------------------------------------------------------------
//
function selectPlaylist(playlist_id) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_id,
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            var track_list = $("#track_list");
            track_list.empty();

            $.each(json['playlist'], function (index, track) {
                track_list.append($('<option>', {
                    value: track.id,
                    text: track.name.substring( 0, 200 ),
                    selected: index == 0
                }));
            });

            track_list.multiselect("refresh");

            var state = json['playlist'].length > 0 ? "inline" : "none";

            $("#track_list_play").css('display', state);
            $("#track_list_queue").css('display', state);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_back(event) {
    stopEventPropagation(event);
    $.ajax({
        type: "GET",
        url: "control/music/track/back/",
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_stop(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "control/music/track/stop/",
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_play(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "control/music/track/play/",
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_pause(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "control/music/track/pause/",
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_forward(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "control/music/track/forward/",
        cache: false,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function playlistPlay(event) {
    stopEventPropagation(event);

    var playlist_id = $("#playlist_list").val();

    if (playlist_id != null && playlist_id > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_id + "/0",
            cache: false,
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function playlistQueue(event) {
    stopEventPropagation(event);

    var playlist_id = $("#playlist_list").val();

    if (playlist_id != null && playlist_id > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_id + "/1",
            cache: false,
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function tracklistPlay(event) {
    stopEventPropagation(event);

    var playlist_id = $("#playlist_list").val();
    var track_id = $("#track_list").val();

    if (track_id != null && track_id > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/track/" + playlist_id + "/" + track_id + "/0",
            cache: false,
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function tracklistQueue(event) {
    stopEventPropagation(event);

    stopEventPropagation(event);

    var playlist_id = $("#playlist_list").val();
    var track_id = $("#track_list").val();

    if (track_id != null && track_id > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/track/" + playlist_id + "/" + track_id + "/1",
            cache: false,
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function trackTime(time) {
    var track_time = "";
    if (time > 60 * 60 * 1000) {
        track_time += Math.floor(time / (60 * 60 * 1000));
        track_time += ":";
        time %= (60 * 60 * 1000);
    }

    track_time += Math.floor(time / (60 * 1000));
    track_time += ":";
    time %= (60 * 1000);
    time = Math.floor(time / (1000))

    if (time < 10)
        track_time += "0"

    track_time += time;

    return track_time;
}

// ----------------------------------------------------------------------------
//
function track_back_info(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/played/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            showTrackList('Played Tracks', json.tracks);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_forward_info(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/queued/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            showTrackList('Queued Tracks', json.tracks);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function showTrackList(title, tracks) {
    var track_list = $('#tld_tracks');
    track_list.empty();

    var html = "<ol type='1'>";
    $.each(tracks, function (index, track) {
        html += "<li>" + track.name + "</li>";
    });
    html += "</ol>";

    track_list.append(html);

    $("#track_list_dialog").dialog({
        modal: title,
        height: 500,
        width: 700,
        title: title,
        buttons: {
            Ok: function () {
                $(this).dialog("close");
            }
        }
    });
}

// ----------------------------------------------------------------------------
//
function showMusicMatch(event) {
    stopEventPropagation(event);

    alert("MUSIC MAPPING UI NOT IMPLEMENTED - USE COMMAND LINE");
}