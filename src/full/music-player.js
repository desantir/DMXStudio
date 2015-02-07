/* 
Copyright (C) 2012-14 Robert DeSantis
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
var playing_track_id = 0;                           // Current music player track

var cache_now_playing = null;
var cache_track_remaining = null;
var cache_track_length = null;
var cache_track_status = null;

var show_login_dialog = true;
var last_player_error = null;

var track_dialog_refresh = null;

var PLAYED_TRACKS_PLAYLIST = -1;
var QUEUED_TRACKS_PLAYLIST = -2;

// ----------------------------------------------------------------------------
//
function update_player_status(music_player_status) {

    if (music_player_status == null)
        return;

    if (music_player_status.logged_in) {
        if (!music_player_ui_ready)
            initialize_player_ui();

        var current_track_id = 0;
        var track_name = "";
        var track_length = "";
        var track_remaining = "";
        var track_status = "";
        var paused = false;

        if (music_player_status.playing != null) {
            current_track_id = music_player_status.playing.track;
            track_name = music_player_status.playing.name;
            track_status = "-" + trackTime(music_player_status.playing.remaining);
            track_length = trackTime(music_player_status.playing.length);
            track_remaining = "remaining " + trackTime(music_player_status.playing.remaining);
            paused = music_player_status.playing.paused;

            cache_track_remaining.text(track_remaining);        // Always update remaining time
            cache_track_status.text(track_status);              // Always update remaining time
        }

        if (current_track_id != playing_track_id) {
            cache_now_playing.text(track_name);
            cache_track_remaining.text(track_remaining);
            cache_track_length.text(track_length);
            cache_track_status.text(track_status);
            playing_track_id = current_track_id;

            if (track_dialog_refresh != null)
                track_dialog_refresh();
        }

        if (current_track_id == 0) {
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
function enable_player_button(id, enabled) {

    var player_button = $("#" + id);

    player_button.attr('disabled', !enabled);

    if (enabled)
        player_button.removeClass("md-icon-disabled").addClass("md-icon-white");
    else
        player_button.removeClass("md-icon-white").addClass("md-icon-disabled");
}
    
// ----------------------------------------------------------------------------
//
function enable_info_button(id, enabled) {

    var info_button = $("#" + id);
    var icon = (id === "track_back_info") ? "ui-icon-star" : "ui-icon-star";

    info_button.attr('disabled', !enabled);

    if (enabled)
        info_button.removeClass("ui-icon-blank").addClass(icon);
    else
        info_button.removeClass(icon).addClass("ui-icon-blank");
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

    $("#playlist_play").button().click(playlistPlay);
    $("#playlist_queue").button().click(playlistQueue);
    $("#playlist_tracks").button().click(playlistTracks);
    
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

    setupTrackSelect($("#playlist_list") );

    music_player_ui_ready = true;
}

// ----------------------------------------------------------------------------
//
function setupTrackSelect(playlist_select) {
    playlist_select.multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'player_multilist', height: 400
    }).bind("multiselectclick", function (event, ui) {
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
                    value: playlist.id,
                    text: playlist.name.substring(0, 200),
                    selected: index == 0
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

    var playlist_number = $("#playlist_list").val();

    if (playlist_number != null && playlist_number > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_number + "/0",
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

    var playlist_number = $("#playlist_list").val();

    if (playlist_number != null && playlist_number > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_number + "/1",
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

    var playlist_number = $("#playlist_list").val();
    var playlist_name = $('#playlist_list option:selected').text(); 

    if (playlist_number != null && playlist_number > 0) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_number,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);
                showTrackList('Playlist: ' + playlist_name, playlist_number, json.tracks);
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
function playTrack(event, playlist_number, track_number, queue_track ) {
    stopEventPropagation(event);

    var queue = (queue_track) ? "1" : "0";

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/play/track/" + playlist_number + "/" + track_number + "/" + queue,
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
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
            showStaticTrackList('Played Tracks', PLAYED_TRACKS_PLAYLIST, json.tracks);
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
            showStaticTrackList('Queued Tracks', QUEUED_TRACKS_PLAYLIST, json.tracks);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function trackListAction(event, playlist_number, track_number) {
    stopEventPropagation(event);

    var current = $(event.currentTarget);
    var source = $(event.target);
    var queue = (playing_track_id != 0 && source.hasClass("tl_icon") && source.hasClass("ui-icon-flag"));

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

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/music/play/track/" + playlist_number + "/" + track_number + "/" + (queue ? "1" : "0"),
        cache: false,
        success: updateMusicUI,
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function showTrackList(title, playlist_number, tracks) {
    track_dialog_refresh = function () {
        var track_list = $('#tld_tracks');
        track_list.empty();

        var html = "";

        html += '<div class="tl_title">';
        html += '<div class="tl_item tl_icon">&nbsp;</div>';
        html += '<div class="tl_item tl_track">TRACK</div>';
        html += '<div class="tl_item tl_time">TIME</div>';
        html += '<div class="tl_item tl_artist">ARTIST</div>';
        html += '<div class="tl_item tl_album">ALBUM</div>';
        html += '</div>'

        $.each(tracks, function (index, track) {
            var css_class = index & 1 ? "tl_odd" : "tl_even";

            html += '<div class="' + css_class + '" title="play" onclick="trackListAction(event,' + playlist_number + ',' + (index + 1) + ');">';

            if (track.id != playing_track_id)
                html += '<div class="tl_item tl_icon ui-icon ui-icon-flag" title="queue"></div>';
            else
                html += '<div class="tl_item tl_icon ui-icon ui-icon-volume-on" title="playing"></div>';

            html += '<div class="tl_item tl_track">' + track.track_name + '</div>';
            html += '<div class="tl_item tl_time">' + trackTime(parseInt(track.duration)) + '</div>';
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
            'Play All': function () {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_number + "/0",
                    cache: false,
                    success: updateMusicUI,
                    error: onAjaxError
                });
                $(this).dialog("close");
            },
            'Queue All': function () {
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_number + "/1",
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
function showStaticTrackList( title, playlist_number ) {
    track_dialog_refresh = function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/" + (playlist_number == PLAYED_TRACKS_PLAYLIST ? "played" : "queued") + "/",
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

                    html += '<div class="' + css_class + '";">';
                    html += '<div class="tl_item tl_track">' + track.track_name + '</div>';
                    html += '<div class="tl_item tl_time">' + trackTime(parseInt(track.duration)) + '</div>';
                    html += '<div class="tl_item tl_artist">' + track.artist_name + '</div>';
                    html += '<div class="tl_item tl_album">' + track.album_name + '</div>';
                    html += '</div>'
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
            url: "/dmxstudio/rest/edit/music/matcher/",
            data: JSON.stringify( $("#mmd_mappings").data("selectionMap") ),
            contentType: 'application/json',
            cache: false,
            success: function () {
                updateChases();
            },
            error: onAjaxError
        });

        $("#music_match_dialog").dialog("close");
    };

    $("#music_match_dialog").dialog({
        title: "Music Match Mappings",
        autoOpen: false,
        width: 1024,
        height: 620,
        modal: true,
        resizable: false,
        open: function () { // Stop main body scroll
            $("body").css("overflow", "hidden");
        },
        close: function () {
            $("body").css("overflow", "auto");
        },
        buttons: {
            'Save': send_update,
            'Cancel': function () {
                $(this).dialog("close");
            }
        }
    });

    if (music_player_ui_ready)
        setupMusicMatchTrackSelect($("#mmd_playlist_list"), $("#mmd_track_list"));
    else
        $("#mmd_track_div").hide();

    $("#mmd_add").button().click( music_map_add );
    $("#mmd_add_all").button().click( music_map_add_all );

    $("#mmd_mappings").data("selectionMap", jQuery.extend(true, [], selectionMap));

    populate_music_mappings();

    $("#music_match_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function setupMusicMatchTrackSelect(playlist_select, tracklist_select) {
    function selectPlaylist(tracklist_select, playlist_id) {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_id,
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
            },
            error: onAjaxError
        });
    }

    playlist_select.multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'player_multilist', height: 400
    }).bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        selectPlaylist(tracklist_select, ui.value);
    });

    tracklist_select.multiselect({
        minWidth: 300, multiple: false, selectedList: 1, header: "Playlist Tracks", noneSelectedText: 'select track', classes: 'player_multilist', height: 500
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
                    value: playlist.id,
                    text: playlist.name.substring(0, 200),
                    selected: index == 0
                }));
            });

            playlist_select.multiselect("refresh");

            // Populate tracks for selected playlist
            var playlist_id = playlist_select.val();
            if (playlist_id != null && playlist_id > 0)
                selectPlaylist(tracklist_select, playlist_id);
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
    }).bind("multiselectclick", function (event, ui) {
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
    }).bind("multiselectclick", function (event, ui) {
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