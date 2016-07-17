/* 
Copyright (C) 2012-16 Robert DeSantis
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

var PLAYED_PLAYLIST_LINK = "local:playlist:played";
var QUEUED_PLAYLIST_LINK = "local:playlist:queued";

var playing_track_link = null;                      // Current music player track
var playing_track_name = "";
var playing_track_position = 0;                     // Tracks play time of current track
var playing_track_length = 0;
var playing_track_bpm = null;

var current_playlist = null;                        // Currently selected playlist (global)
var show_login_dialog = true;
var last_player_error = null;

var track_dialog_refresh = null;

var cache_now_playing = null;
var cache_track_remaining = null;
var cache_track_length = null;
var cache_track_status = null;
var cache_track_stop_button = null;
var cache_track_play_button = null;
var cache_track_pause_button = null;
var cache_track_back_button = null;
var cache_track_forward_button = null;
var cache_track_back_info_button = null;
var cache_track_forward_info_button = null;

// ----------------------------------------------------------------------------
//
function initializeMusicPlayer() {
    cache_track_stop_button = $("#track_stop");
    cache_track_play_button = $("#track_play");
    cache_track_pause_button = $("#track_pause");
    cache_track_back_button = $("#track_back");
    cache_track_forward_button = $("#track_forward");
    cache_track_back_info_button = $("#track_back_info");
    cache_track_forward_info_button = $("#track_forward_info");

    cache_now_playing = $('#now_playing');
    cache_track_remaining = $('#track_remaining');
    cache_track_length = $('#track_length');
    cache_track_status = $('#track_status');

    cache_now_playing.text( "" );
    cache_track_remaining.text( "" );
    cache_track_length.text("");
    cache_track_status.text("");

    $("#playlist_play").button().click(playlistPlay);
    $("#playlist_queue").button().click(playlistQueue);
    $("#playlist_tracks").button().click(playlistTracks);
    
    cache_track_back_info_button.click(track_back_info);
    cache_track_forward_info_button.click(track_forward_info);
    cache_track_back_button.click(track_back);
    cache_track_stop_button.click(track_stop);
    cache_track_play_button.click(track_play);
    cache_track_pause_button.click(track_pause);
    cache_track_forward_button.click(track_forward);

    cache_track_status.css('display', 'block');
    cache_track_remaining.css('display', 'block');
    $('#track_select').css('display', 'block');
    $('#track_controls').css('display', 'block');

    setupTrackSelect($("#playlist_list") );
}

// ----------------------------------------------------------------------------
//
function update_player_status(music_player_status) {
    if (music_player_status.logged_in) {
        var playing = music_player_status.playing;

        if ( playing == null || playing.link == null || playing.link === "" ) {
            stopTrackEvent( null );
        }
        else {
            startTrackEvent( playing.link, playing.length - playing.remaining );
        }

        changeTrackQueueEvent( music_player_status.played, music_player_status.queued )
    }
    else if (show_login_dialog) {
        show_login_dialog = false;  // Only once
        player_login(music_player_status.player_name, music_player_status.username);
    }

    if ( last_player_error != music_player_status.player_error ) {
        last_player_error = music_player_status.player_error;
        $('#player_error').text( (last_player_error != null) ? last_player_error : "" );
    }
}

// ----------------------------------------------------------------------------
//
function changeTrackQueueEvent( played_size, queued_size ) {
    enable_player_button(cache_track_back_button, played_size != 0);
    enable_player_button(cache_track_forward_button, queued_size != 0);

    enable_info_button(cache_track_back_info_button, played_size != 0);
    enable_info_button(cache_track_forward_info_button, queued_size != 0);
}

// ----------------------------------------------------------------------------
//
function startTrackEvent( track_link, position_ms ) {
       $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/venue/status/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                if ( json.music_player == null || json.music_player.playing == null || json.music_player.playing == undefined )
                    return;

                playing_track_link = json.music_player.playing.link;
                playing_track_name = json.music_player.playing.name;
                playing_track_position = position_ms;
                playing_track_length = json.music_player.playing.length;
                playing_track_bpm = json.music_player.playing.bpm;

                var track_length = trackTime( playing_track_length, false );

                cache_now_playing.text( playing_track_name );
                cache_track_length.text( playing_track_bpm == null ? track_length : track_length + " | " + Math.round(playing_track_bpm) + " BPM" );

                timeTrackEvent( track_link, position_ms );

                if ( json.music_player.playing.paused )
                    pauseTrackEvent( track_link );
                else
                    resumeTrackEvent( track_link );

                if (track_dialog_refresh != null)
                    track_dialog_refresh();
            },

            error: onAjaxError
        });
}

// ----------------------------------------------------------------------------
//
function stopTrackEvent( track_link ) {

    if ( playing_track_link == track_link ) {
        cache_track_remaining.text( "no track" );
        cache_track_status.text( "no track" );
        cache_now_playing.text( "" );
        cache_track_length.text( "" );

        enable_player_button(cache_track_stop_button, false);
        enable_player_button(cache_track_play_button, false);
        enable_player_button(cache_track_pause_button, false);

        playing_track_link = null;
        playing_track_name = "";
        playing_track_position = 0;
        playing_track_length = 0;
        playing_track_bpm = null;

        if (track_dialog_refresh != null)
            track_dialog_refresh();
    }
}

// ----------------------------------------------------------------------------
//
function pauseTrackEvent( track_link ) {
    if ( playing_track_link == track_link ) {
        enable_player_button(cache_track_stop_button, false );
        enable_player_button(cache_track_play_button, true );
        enable_player_button(cache_track_pause_button,  false );
    }
}

// ----------------------------------------------------------------------------
//
function resumeTrackEvent( track_link ) {
    if ( playing_track_link == track_link ) {
        enable_player_button(cache_track_stop_button, true );
        enable_player_button(cache_track_play_button, false );
        enable_player_button(cache_track_pause_button,  true );
    }
}

// ----------------------------------------------------------------------------
//
function timeTrackEvent( track_link, position_ms ) {
    if ( playing_track_link == track_link ) {
        playing_track_position = position_ms;

        var status = trackTime(playing_track_position,false) + " / -" + trackTime(playing_track_length - position_ms,false);

        cache_track_remaining.text( status );
        cache_track_status.text( status );
    }
}

// ----------------------------------------------------------------------------
//
function updateMusicUI() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/status/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            update_player_status(json.music_player);
        },
        error: onAjaxError
    });
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
function enable_player_button(player_button, enabled) {
    var current_enable = !player_button.hasClass("md-icon-disabled")

    if (current_enable != enabled) {
        player_button.attr('disabled', !enabled);

        if (enabled)
            player_button.removeClass("md-icon-disabled").addClass("md-icon-white");
        else
            player_button.removeClass("md-icon-white").addClass("md-icon-disabled");
    }
}
    
// ----------------------------------------------------------------------------
//
function enable_info_button(info_button, enabled) {

    var current_enable = !info_button.hasClass("ui-icon-blank")

    if (current_enable != enabled) {
        var icon = (info_button === cache_track_back_info_button) ? "ui-icon-star" : "ui-icon-star";

        info_button.attr('disabled', !enabled);

        if (enabled)
            info_button.removeClass("ui-icon-blank").addClass(icon);
        else
            info_button.removeClass(icon).addClass("ui-icon-blank");
    }
}

// ----------------------------------------------------------------------------
//
function getCurrentPlaylist() {
    return current_playlist;
}

// ----------------------------------------------------------------------------
//
function setCurrentPlaylist(playlist) {
    current_playlist = playlist;

    $("#playlist_list").multiselect();
    $('#playlist_list option[value="' + current_playlist + '"]').attr("selected", true);
    $("#playlist_list").multiselect("refresh");
}

// ----------------------------------------------------------------------------
//
function setupTrackSelect(playlist_select) {
    playlist_select.multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'player_multilist', height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        current_playlist = ui.value;
        client_config_update = true;
        stopEventPropagation(event);
    });

    // Populate playlists
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/playlists/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            playlist_select.empty();

            $.each(json['playlists'], function (index, playlist) {
                playlist_select.append($('<option>', {
                    value: playlist.link,
                    text: playlist.name.substring(0, 200),
                    selected: current_playlist == null ? index == 0 : current_playlist == playlist.link
                }));
            });

            playlist_select.multiselect("refresh");
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function playlistPlay(event) {
    stopEventPropagation(event);

    var playlist_link = $("#playlist_list").val();

    if (playlist_link != null && playlist_link.length> 0 ) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/0",
            cache: false,
            success: updateMusicUI,
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function playlistQueue(event) {
    stopEventPropagation(event);

    var playlist_link = $("#playlist_list").val();

    if (playlist_link != null && playlist_link.length > 0 ) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/1",
            cache: false,
            success: function() {
                updateMusicUI();

                if (track_dialog_refresh != null)
                    track_dialog_refresh();
            },
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function playlistTracks(event) {
    stopEventPropagation(event);

    var playlist_name = $('#playlist_list option:selected').text(); 
    var playlist_link = $("#playlist_list").val();

    if (playlist_link != null && playlist_link.length > 0 ) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_link,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);
                showTrackList( 'Playlist: ' + playlist_name, playlist_link, json.tracks );
            },
            error: onAjaxError
        });
    }
}

// ----------------------------------------------------------------------------
//
function track_back(event) {
    stopEventPropagation(event);
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/track/back/",
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_stop(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/track/stop/",
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_play(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/track/play/",
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_pause(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/track/pause/",
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function track_forward(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/track/forward/",
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function playTrack(event, track_link, queue_track ) {
    stopEventPropagation(event);

    var track_action = (queue_track) ? "queue" : "play";

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/" + track_action + "/track/" + track_link,
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function trackTime(time,show_frac) {
    var track_time = "";
    if (time > 60 * 60 * 1000) {
        track_time += Math.floor(time / (60 * 60 * 1000));
        track_time += ":";
        time %= (60 * 60 * 1000);
    }

    track_time += Math.floor(time / (60 * 1000));
    track_time += ":";
    time %= (60 * 1000);

    var seconds = Math.floor(time / (1000));
    if (seconds < 10)
        track_time += "0"
    track_time += seconds;

    if ( show_frac ) {
        time -= seconds * 1000;
        var frac = time / 100;
        track_time += "." + frac;
    }

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
            showStaticTrackList('Played Tracks', PLAYED_PLAYLIST_LINK, json.tracks);
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
            showStaticTrackList('Queued Tracks', QUEUED_PLAYLIST_LINK, json.tracks);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function trackListAction(event, track_link) {
    stopEventPropagation(event);

    var current = $(event.currentTarget);
    var source = $(event.target);
    var queue = (playing_track_link != "" && source.hasClass("tl_icon") && source.hasClass("ui-icon-flag"));

    if (queue) {
        source.css('rotation', 0);      // Reset rotation

        source.animate(
          { rotation: 360 },
          {
              duration: 1000,
              step: function (now, fx) {
                  $(this).css({ "transform": "rotate(" + now + "deg)" });
              }
          }
        );
    }
    else {
        var playing_element_icon = $('#tld_tracks .ui-icon-volume-on');
        if (playing_element_icon != null) {
            playing_element_icon.removeClass("ui-icon-volume-on").addClass("ui-icon-flag");
            playing_element_icon.attr("title", "queue");
        }

        var icon = current.find(".tl_icon");
        icon.removeClass("ui-icon-flag").addClass("ui-icon-volume-on");
        icon.attr("title", "playing");
    }

    var track_action = (queue) ? "queue" : "play";

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/" + track_action + "/track/" + track_link,
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function showTrackList( title, playlist_link, tracks ) {
    track_dialog_refresh = function () {
        var track_list = $('#tld_tracks');
        track_list.empty();

        var html = "";

        html += '<div class="tl_title">';
        html += '<div class="tl_item tl_icon">&nbsp;</div>';
        html += '<div class="tl_item tl_track">TRACK</div>';
        html += '<div class="tl_item tl_time">TIME</div>';
        html += '<div class="tl_item tl_bpm">BPM</div>';
        html += '<div class="tl_item tl_artist">ARTIST</div>';
        html += '<div class="tl_item tl_album">ALBUM</div>';
        html += '</div>'

        $.each(tracks, function (index, track) {
            var css_class = index & 1 ? "tl_odd" : "tl_even";

            html += '<div class="' + css_class + '" title="play" onclick="trackListAction(event,\'' + track.link + '\');">';

            if (track.link !== playing_track_link)
                html += '<div class="tl_item tl_icon ui-icon ui-icon-flag" title="queue"></div>';
            else
                html += '<div class="tl_item tl_icon ui-icon ui-icon-volume-on" title="playing"></div>';

            html += '<div class="tl_item tl_track">' + track.track_name + '</div>';
            html += '<div class="tl_item tl_time">' + trackTime(track.duration,false) + '</div>';

            if (track.audio_info != null) {
                html += '<div class="tl_item tl_bpm">' + Math.round( track.audio_info.bpm ) + '</div>';
            }
            else {
                html += '<div class="tl_item tl_bpm">&nbsp;</div>';
            }

            html += '<div class="tl_item tl_artist">' + track.artist_name + '</div>';
            html += '<div class="tl_item tl_album">' + track.album_name + '</div>';
            html += '</div>'
        });

        track_list.append(html);
    }

    track_dialog_refresh();

    $("#track_list_dialog").dialog({
        modal: false,
        height: 600,
        width: 1040,
        title: title,
        open: function () { // Stop main body scroll
            // $("body").css("overflow", "hidden");
        },
        close: function () {
            // $("body").css("overflow", "auto");
            track_dialog_refresh = null;
        },
        buttons: {
            'Play All': function () {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/0",
                    cache: false,
                    success: updateMusicUI,
                    error: onAjaxError
                });
                $(this).dialog("close");
            },
            'Queue All': function () {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/1",
                    cache: false,
                    success: updateMusicUI,
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
function showStaticTrackList( title, playlist_link ) {
    track_dialog_refresh = function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/" + (playlist_link === PLAYED_PLAYLIST_LINK ? "played" : "queued") + "/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                tracks = json.tracks;

                var track_list = $('#tld_tracks');
                track_list.empty();

                var html = "";

                html += '<div class="tl_title">';
                html += '<div class="tl_item tl_track">TRACK</div>';
                html += '<div class="tl_item tl_time">TIME</div>';
                html += '<div class="tl_item tl_artist">ARTIST</div>';
                html += '<div class="tl_item tl_album">ALBUM</div>';
                html += '</div>'

                $.each(tracks, function (index, track) {
                    var css_class = index & 1 ? "tl_odd" : "tl_even";

                    html += '<div class="' + css_class + '" title="play" onclick="trackListAction(event,\'' + track.link + '\');">';
                    html += '<div class="' + css_class + '";">';
                    html += '<div class="tl_item tl_track">' + track.track_name + '</div>';
                    html += '<div class="tl_item tl_time">' + trackTime(parseInt(track.duration),false) + '</div>';
                    html += '<div class="tl_item tl_artist">' + track.artist_name + '</div>';
                    html += '<div class="tl_item tl_album">' + track.album_name + '</div>';
                    html += '</div>';
                    html += '</div>';
                });

                track_list.append(html);
            },

            error: onAjaxError
        });
    }

    track_dialog_refresh();

    $("#track_list_dialog").dialog({
        modal: false,
        height: 600,
        width: 980,
        title: title,
        open: function () { // Stop main body scroll
            // $("body").css("overflow", "hidden");
        },
        close: function () {
            // $("body").css("overflow", "auto");
            track_dialog_refresh = null;
        },
        buttons: {
            'Close': function () {
                $(this).dialog("close");
            }
        }
    });
}


// ----------------------------------------------------------------------------
//
function showMusicMatch(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/matcher/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            $.each(json, function (entry) { entry.isnew = false; });
            musicMatchDialog(json);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function musicMatchDialog( selectionMap ) {
    var send_update = function ( ) {
        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/edit/music/matcher/load",
            data: JSON.stringify( $("#mmd_mappings").data("selectionMap") ),
            contentType: 'application/json',
            cache: false,
            success: function () {
                ;
            },
            error: onAjaxError
        });

        $("#music_match_dialog").dialog("close");
    };

    $("#music_match_dialog").dialog({
        title: "Music Match Mappings",
        autoOpen: false,
        width: 1048,
        height: 620,
        modal: true,
        resizable: false,
        open: function () { // Stop main body scroll
            // $("body").css("overflow", "hidden");
        },
        close: function () {
            // $("body").css("overflow", "auto");
        },
        buttons: {
            'Save': send_update,
            'Cancel': function () {
                $(this).dialog("close");
            }
        }
    });

    $("#mmd_add").button().click( music_map_add );
    $("#mmd_add_all").button().click( music_map_add_all );

    $("#mmd_add").button("disable");
    $("#mmd_add_all").button("disable");

    setupMusicMatchTrackSelect($("#mmd_playlist_list"), $("#mmd_track_list"));

    $("#mmd_mappings").data("selectionMap", jQuery.extend(true, [], selectionMap));

    populate_music_mappings();

    $("#music_match_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function setupMusicMatchTrackSelect(playlist_select, tracklist_select) {
    function selectPlaylist(tracklist_select, playlist_link) {
        $("#mmd_add_all").button("enable");

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_link,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                tracklist_select.empty();

                $.each(json['tracks'], function (index, track) {
                    tracklist_select.append($('<option>', {
                        value: track.link,
                        text: track.full_name.substring(0, 200),
                        selected: index == 0
                    }));
                });

                tracklist_select.multiselect("refresh");
                tracklist_select.multiselect("uncheckAll");
            },
            error: onAjaxError
        });
    }

    playlist_select.multiselect({
        minWidth: 400, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'player_multilist', height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        selectPlaylist(tracklist_select, ui.value);
    });

    tracklist_select.multiselect({
        minWidth: 400, multiple: false, selectedList: 1, header: "Playlist Tracks", noneSelectedText: 'select track', classes: 'player_multilist', height: 500
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        
        $("#mmd_add").button("enable");
    });;

    // Populate playlists
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/playlists/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            playlist_select.empty();
            tracklist_select.empty();

            $.each(json['playlists'], function (index, playlist) {
                playlist_select.append($('<option>', {
                    value: playlist.link,
                    text: playlist.name.substring(0, 200),
                    selected: index == 0
                }));
            });

            playlist_select.multiselect("refresh");
            playlist_select.multiselect("uncheckAll");
            tracklist_select.multiselect("refresh");

            // Populate tracks for selected playlist
            var playlist_link = playlist_select.val();
            if (playlist_link != null && playlist_link > 0)
                selectPlaylist(tracklist_select, playlist_link);
        },
        error: onAjaxError
    });
}

var mm_types = ["Invalid", "Scene", "Chase", "Random scene", "Random chase", "Random scene (BPM)"];
var current_edit = null;

// ----------------------------------------------------------------------------
//
function populate_music_mappings() {
    $("#mmd_mappings").empty();
    current_edit = null;

    var template = $("#mmd_map_template")[0].innerHTML;

    var selectionMap = $("#mmd_mappings").data("selectionMap");
    for (var index = 0; index < selectionMap.length; index++) {
        var container_elem = $(template);
        container_elem.addClass(index & 1 ? "mm_odd" : "mm_even");
        var mappings_elem = $("#mmd_mappings").append(container_elem);
        container_elem.data("mapping", selectionMap[index] );
        setTrackLabels(container_elem);
    }
}

// ----------------------------------------------------------------------------
//
function setTrackLabels(container_elem) {
    var label_elem = container_elem.find(".mm_track_label");
    var type_elem = container_elem.find(".mm_type");
    var select_elem = container_elem.find(".mm_select");
    var remove_elem = container_elem.find(".mm_remove");

    var mapping = container_elem.data("mapping");

    container_elem.click(music_map_edit);

    if (true == mapping.isnew)
        label_elem.text("NEW! " + mapping.track);
    else
        label_elem.text(mapping.track);

    function span(text) {
        return "<span style='margin-left: 5px;'>" + escapeForHTML(text) + "</span>"
    }

    type_elem.html( span(mm_types[mapping.type]) );

    if (mapping.type == 1) {
        var scene = getSceneById(mapping.id);
        select_elem.html(span((scene == null) ? "ERROR" : scene.getFullName()));
    }
    else if (mapping.type == 2) {
        var chase = getChaseById(mapping.id);
        select_elem.html(span((chase == null) ? "ERROR" : chase.getName()));
    }
    else
        select_elem.empty();

    if (mapping.special) {
        remove_elem.hide();
        label_elem.addClass('mm_special_track');
    }
    else {
        remove_elem.click(music_map_remove);
        remove_elem.data("mapping", mapping);
    }
}

// ----------------------------------------------------------------------------
//
function music_map_edit(event) {
    stopEventPropagation(event);

    if (current_edit != null) {
        setTrackLabels(current_edit);
        current_edit = null;
    }

    current_edit = $(this);
    current_edit.click(null);

    var type_elem = current_edit.find(".mm_type");
    var select_elem = current_edit.find(".mm_select");

    var mapping = current_edit.data("mapping");

    var type_select = $("<select>");
    for (var i = 1; i < mm_types.length; i++) {
        if ((i == 2 || i== 4) && chases.length == 0)
            continue;

        type_select.append($("<option>", {
            value: i,
            text: mm_types[i],
            selected: mapping.type == i
        }));
    }

    type_elem.empty();
    type_elem.append(type_select);

    type_select.multiselect({
        classes: 'player_multilist',
        minWidth: 150,
        height: "auto",
        multiple: false,
        selectedList: 1,
        header: false
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        mapping.type = ui.value;
        update_music_map_type();
    });

    var id_select = $("<select>");

    select_elem.empty();
    select_elem.append(id_select);

    id_select.multiselect({
        classes: 'player_multilist',
        minWidth: 320,
        height: "auto",
        multiple: false,
        selectedList: 1,
        header: false
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        mapping.id = ui.value;
    });

    update_music_map_type();
}

// ----------------------------------------------------------------------------
//
function update_music_map_type( ) {
    var select_elem = current_edit.find(".mm_select");

    var mapping = current_edit.data("mapping");

    if (mapping.type >= 3) {
        select_elem.hide();
        return;
    }

    var select = select_elem.find("select");

    select.empty();

    if (mapping.type == 1) {
        if (getSceneById(mapping.id) == null)
            mapping.id = getDefaultSceneId();

        $.map(scenes, function (scene, index) {
            select.append($("<option>", {
                value: scene.getId(),
                text: scene.getFullName(),
                selected: scene.getId() == mapping.id
            }));
        });
    }
    else {
        if (getChaseById(mapping.id) == null)
            mapping.id = chases[0].getId();

        $.map(chases, function (chase, index) {
            select.append( $("<option>", {
                value: chase.getId(),
                text: chase.getName(),
                selected: chase.getId() == mapping.id
            }));
        });
    }

    select.multiselect("refresh");
    select_elem.show();
}

// ----------------------------------------------------------------------------
//
function music_map_add(event) {
    stopEventPropagation(event);

    var mapping = { track: $("#mmd_track_list :selected").text(), 'link': $("#mmd_track_list").val(), 'type': 3, 'id': 1, 'isnew': true }

    var selectionMap = $("#mmd_mappings").data("selectionMap");

    for (var i = 0; i < selectionMap.length; i++) {
        if (selectionMap[i].link == mapping.link) {
            mapping.id = selectionMap[i].id;
            mapping.type = selectionMap[i].type;
            mapping.isnew = selectionMap[i].isnew;
            selectionMap.splice(i, 1);
            break;
        }
    }

    selectionMap.splice(0, 0, mapping);

    populate_music_mappings();

    return false;
}

// ----------------------------------------------------------------------------
//
function music_map_add_all(event) {
    stopEventPropagation(event);

    var selectionMap = $("#mmd_mappings").data("selectionMap");
    var insert_pos = 0;

    $("#mmd_track_list option").each(function () {
        var mapping = { 'track': this.text, link: this.value, 'type': 3, 'id': 1, 'isnew': true }
        for (var i = 0; i < selectionMap.length; i++) {
            if (selectionMap[i].link == mapping.link) {
                mapping.id = selectionMap[i].id;
                mapping.type = selectionMap[i].type;
                mapping.isnew = selectionMap[i].isnew;
                selectionMap.splice(i, 1);
                break;
            }
        }

        selectionMap.splice(insert_pos++, 0, mapping);
    });

    populate_music_mappings();

    return false;
}

// ----------------------------------------------------------------------------
//
function music_map_remove(event) {
    stopEventPropagation(event);

    var selectionMap = $("#mmd_mappings").data("selectionMap");
    var mapping = $(this).data("mapping");

    for (var i = 0; i < selectionMap.length; i++) {
        if (selectionMap[i].track == mapping.track) {
            selectionMap.splice(i, 1);
            populate_music_mappings();
            break;
        }
    }
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

    $("#std_playlists").multiselect({
        minWidth: 600, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'player_multilist', height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        populate_tracks( ui.value );
    });

    $("#std_tracks").multiselect({
        minWidth: 600, multiple: false, selectedList: 1, header: "Tracks", noneSelectedText: 'select track', classes: 'player_multilist', height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);

        var options = $("#std_tracks option");
        std_track = $(options[ui.value]).data("track");
        $("#std_select_track").button("enable");
    });

    // Populate playlists
    if ( $("#std_playlists option").length == 0 ) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlists/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                $("#std_playlists").empty();
                $("#std_tracks").empty();

                $.each(json['playlists'], function (index, playlist) {
                    $("#std_playlists").append($('<option>', {
                        value: playlist.link,
                        text: playlist.name.substring(0, 200),
                        selected: current_playlist == null ? index == 0 : current_playlist == playlist.link
                    }));
                });

                $("#std_select_track").button("disable");
                $("#std_playlists").multiselect("refresh");

                populate_tracks( $("#std_playlists").val() );
            },
            error: onAjaxError
        });
    }

    select_track_dialog.dialog("open");
}