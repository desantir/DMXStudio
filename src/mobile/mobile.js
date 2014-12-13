/* 
Copyright (C) 2012-,2014 Robert DeSantistimeout_send_ajax_request
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

var SLIDER_TIMEOUT = 500;

var track_timer = null;
var track_remaining = 0;
var track_paused = false;
var tracks_queued = 0;
var server_track_sync_ms = 0;
var timeout = null;
var music_player = null;
var tracks_queued_flasher = 1;
var tracks_queued_flasher_interval;
var fixtures = null;                        // Only load fixture data once
var current_act = 0;

// #_name_ = DOM id and ._name = class_name 

// ----------------------------------------------------------------------------
//
function initializeUI() {

    $("#select_blackout").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/blackout/" + this.value,
            cache: false,
            error: onError
        });
    });

    $("#whiteoutChange").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/whiteout/" + $('input[name=select_whiteout]:checked').val(),
            cache: false,
            error: onError
        });
    });

    $("#whiteoutColor").minicolors({
        animationSpeed: 50,
        animationEasing: 'swing',
        change: change_whiteout_color,
        changeDelay: 0,
        control: 'hue',
        defaultValue: '',
        hide: null,
        hideSpeed: 100,
        inline: true,
        letterCase: 'lowercase',
        opacity: false,
        position: 'bottom left',
        show: null,
        showSpeed: 100,
        theme: 'bootstrap'
    });

    $("#sceneChange").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/scene/show/" + $('input[name=select_scene]:checked').val(),
            cache: false,
            error: onError
        });
    });

    $("#chaseChange").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/chase/show/" + $('input[name=select_chase]:checked').val(),
            cache: false,
            error: onError
        });
    });

    $("#control_fixture").click( capture_fixture );

    $("#venue").on("pageshow", show_venue );
    $("#scene").on("pageshow", show_scene);
    $("#chase").on("pageshow", show_chase);
    $("#fixture").on("pageshow", show_fixtures);
    $("#sound").on("pageshow", show_sound);

    $("#volume_mute").change(function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/volume/mute/" + $('input[name=select_volume_mute]:checked').val(),
            cache: false,
            error: onError
        });
    });

    $("#track_back_id").click(function () {
        clearTimeout(track_timer);

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/back",
            cache: false,
            error: onError
        });

        track_remaining = 0;
        track_paused = false;
        track_timer = setTimeout(track_timeout, 500);
    });

    $("#track_forward_id").click(function () {
        clearTimeout(track_timer);

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/forward",
            cache: false,
            error: onError
        });

        track_remaining = 0;
        track_paused = false;
        track_timer = setTimeout(track_timeout, 500);
    });

    $("#track_pause_id").click(function () {
        clearTimeout(track_timer);

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/pause",
            cache: false,
            error: onError
        });

        track_remaining = 0;
        track_paused = true;

        track_pause_id.style.display = "none";
        track_play_id.style.display = "";

        track_timer = setTimeout(track_timeout, 500);
    });

    $("#track_play_id").click(function () {
        clearTimeout(track_timer);

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/play",
            cache: false,
            error: onError
        });

        track_remaining = 0;
        track_paused = false;
        track_timer = setTimeout('track_timeout()', 500);
    });

    $("#playlist_select_id").change(function () {
        var playlist_id = playlist_select_id.value;

        // Get tracks
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_id,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);
                playlist = json['playlist'];

                var data = [];
                data = $.map(playlist, function (pl, index) {
                    return "<option value=" + pl.id + ">" + pl.name + "</option>";
                });

                data.unshift("<option value=0>Select Track</option>", "<option value=-1>PLAY ENTIRE PLAYLIST</option>");

                $("#playlist_tracks_id").html(data.join(""));

                // Select option and refresh the dropdown
                $('#playlist_tracks_id option').eq(0).attr("selected", true);
                $('#playlist_tracks_id').selectmenu('refresh', true);
            },
            error: onError
        });
    });

    $("#playlist_tracks_id").change(function () {
        var track_id = playlist_tracks_id.value;
        var playlist_id = playlist_select_id.value;
        if (track_id == 0 || playlist_id == 0)
            return;

        var queue = parseInt($('input[name=track_play_mode]:checked').val());

        if (track_id == -1) {                       // Play all tracks
            url_str = "/dmxstudio/rest/control/music/play/playlist/" + playlist_id + "/" + queue;
        }
        else {                                      // Play single track
            url_str = "/dmxstudio/rest/control/music/play/track/" + playlist_id + "/" + track_id + "/" + queue;
        }

        $.ajax({
            type: "GET",
            url: url_str,
            cache: false,
            success: function (data) {
                updateMusicPlayer();
            },
            error: onError
        });

        if (queue && tracks_queued) {       // Indicate to the user queue has changed
            flash_tracks_queued();
        }
        else {                              // Track name will change momentarily
            $('#playlist_tracks_id option').eq(0).attr("selected", true);
            $('#playlist_tracks_id').selectmenu('refresh', true);
        }
    });

    /* This stops page transitions ?!?
        $("#sound").live("pagehide", function (event, ui) {
            if (track_timer) {
                clearTimeout(track_timer);
                track_timer = NULL;
            }
        });
    */
}

// ----------------------------------------------------------------------------
//
function timeout_send_ajax_request() {
    $.ajax({
        type: "GET",
        url: document.timer_url,
        cache: false,
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function change_dimmer(dimmer_value) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/masterdimmer/" + dimmer_value,
        cache: false,
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function change_whiteout_color( rgb, opacity ) {
    clearTimeout(timeout);

    document.timer_url = "/dmxstudio/rest/control/venue/whiteout/color/" + rgb.substr(1)

    timeout = setTimeout(timeout_send_ajax_request, SLIDER_TIMEOUT);
}

// ----------------------------------------------------------------------------
//
function change_strobe(strobe_value) {
    clearTimeout(timeout);

    document.timer_url = "/dmxstudio/rest/control/venue/strobe/" + strobe_value

    timeout = setTimeout(timeout_send_ajax_request, SLIDER_TIMEOUT);
}

// ----------------------------------------------------------------------------
//
function change_animation_speed(speed) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/venue/animation_speed/" + speed,
        cache: false,
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function change_act(new_act) {
    current_act = parseInt(new_act);
}

// ----------------------------------------------------------------------------
//
function change_volume(volume_value) {
    clearTimeout(timeout);

    document.timer_url = "/dmxstudio/rest/control/venue/volume/master/" + volume_value

    timeout = setTimeout(timeout_send_ajax_request, 50);
}

// ----------------------------------------------------------------------------
//
function escapeForHTML(text) {
    return escape(text).replace(/%(..)/g, "&#x$1;");
}

// ----------------------------------------------------------------------------
//
function show_sound() {
    updateMusicPlayer( true );
}

// ----------------------------------------------------------------------------
//
function updateMusicPlayer(update_selections) {
    clearTimeout(track_timer);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/status/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            $('#volume_slider').val(json.master_volume);
            $('#volume_slider').slider("refresh");

            name = (json.mute) ? '#volume_mute_1' : '#volume_mute_0';
            $(name).prop("checked", true).checkboxradio("refresh");

            clearTimeout(timeout);                  // Stop events from going back to server

            if (!json.has_music_player) {           // There is no music player
                track_controls.style.display = "none";
                track_selector.style.display = "none";
                return;
            }

            music_player = json.music_player;

            if (music_player.playing) {
                id_track_name.innerHTML = music_player.playing.name;
                id_track_length.innerHTML = trackTime(music_player.playing.length)

                track_remaining = music_player.playing.remaining;
                track_paused = music_player.playing.paused;

                id_track_remaining.innerHTML = "Remaining: " + trackTime(track_remaining);

                track_pause_id.style.display = track_paused ? "none" : "";
                track_play_id.style.display = !track_paused ? "none" : "";
            }
            else {
                id_track_name.innerHTML = "NO TRACK";
                id_track_length.innerHTML = "";
                id_track_remaining.innerHTML = "";

                track_remaining = 0;
                track_paused = false;

                track_pause_id.style.display = "none";
                track_play_id.style.display = "none";
            }

            tracks_queued = music_player.queued;
            id_tracks_queued.innerHTML = tracks_queued + " in queue";

            track_timer = setTimeout(track_timeout, 1000);
            server_track_sync_ms = 0;

            if (update_selections) {
                // Get playlists
                $.ajax({
                    type: "GET",
                    url: "/dmxstudio/rest/query/music/playlists/",
                    cache: false,
                    success: function (data) {
                        var json = jQuery.parseJSON(data);
                        playlists = json.playlists;

                        var data = [];
                        data = $.map(playlists, function (pl, index) {
                            return "<option value=" + pl.id + ">" + pl.name + "</option>";
                        });

                        $("#playlist_select_id").html(data.join(""));
                        $("#playlist_tracks_id").html("");

                        // Select option and refresh the dropdown
                        $('#playlist_select_id option').eq(0).attr("selected", true);
                        $('#playlist_select_id').selectmenu('refresh', true);

                        $('#playlist_select_id').trigger('change');
                    },
                    error: onError
                });
            }
        },
    });
}

// ----------------------------------------------------------------------------
//
function flash_tracks_queued() {
    tracks_queued_flasher = 1;
    tracks_queued_flasher_interval = setInterval(function () {
        $('#id_tracks_queued').css("display", (tracks_queued_flasher & 1) ? "none" : "");
        tracks_queued_flasher += 1;
        if (tracks_queued_flasher == 11) {
            clearInterval(tracks_queued_flasher_interval);
            // Select option and refresh the dropdown
            $('#playlist_tracks_id option').eq(0).attr("selected", true);
            $('#playlist_tracks_id').selectmenu('refresh', true);
        }
    }, 100);
}

// ----------------------------------------------------------------------------
//
function track_timeout() {
    if (!track_paused && track_remaining > 1000 && server_track_sync_ms < 5000) {
        track_remaining -= 1000;
        id_track_remaining.innerHTML = "Remaining: " + trackTime(track_remaining);
        track_timer = setTimeout(track_timeout, 1000);
        server_track_sync_ms += 1000;
        return;
    }

    clearTimeout(track_timer);

    updateMusicPlayer();
}

// ----------------------------------------------------------------------------
//
function change_volume_mute(mute) {
    clearTimeout(timeout);

    document.timer_url = "/dmxstudio/rest/control/venue/mute_volume/" + (mute ? 1 : 0);

    timeout = setTimeout(timeout_send_ajax_request, 100);
}

// ----------------------------------------------------------------------------
//
function onError(data) {
    alert("Unable to complete server operation.");
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
function show_venue() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/status/",
        cache: false,
        success: function (data) {
            venue_content.style.display = "";

            var json = jQuery.parseJSON(data);

            $('#select_blackout option').eq(json.blackout ? "1" : "0").attr("selected", true);
            $('#select_blackout').slider('refresh');

            $("#whiteout_" + json.whiteout).prop("checked", true).checkboxradio("refresh");
            $("#whiteoutColor").minicolors('value', "#" + json.whiteout_color);

            $("#slider_whiteout_strobe").on("slidestop", function (event, ui) {
                change_strobe(this.value);
                return false;
            });

            $('#slider_whiteout_strobe').val(json.whiteout_strobe);
            $('#slider_whiteout_strobe').slider("refresh");

            $("#master_dimmer").slider({
                stop: function (event, ui) {
                    change_dimmer(this.value);
                    return false;
                }
            });

            $('#master_dimmer').val(json.dimmer);
            $('#master_dimmer').slider("refresh");

            var o = $("#animations_speed_id option[value='" + json.animation_speed + "']");
            var index = 0;

            if (o != null && o.index() != -1) {
                index = o.index();
            }

            $('#animations_speed_id option:eq(0)').text("Default: " + json.animation_speed + " ms");
            $('#animations_speed_id option:eq(0)').val(json.animation_speed);

            // Select option and refresh the dropdown
            $('#animations_speed_id option').eq(index).attr("selected", true);
            $('#animations_speed_id').selectmenu('refresh', true);

            clearTimeout(timeout); // Stop events from going back to server
        },
        error: function () {
            venue_content.style.display = "none";
            alert("Venue is not available");
        }
    });
}

// ----------------------------------------------------------------------------
//
function show_scene() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/scenes/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            var scene_container = $("#sceneChange");
            scene_container.empty();

            var scene_html = "";

            $.map(json, function (scene, index) {
                if (scene.number == 0 || current_act == 0 || scene.acts.indexOf(current_act) != -1) {
                    var id = 'scene_' + scene.id;

                    scene_html += "<input type='radio' name='select_scene' id='" + id + "' value='" + scene.id + "' data-theme='c' ";
                    if ( scene.is_running )
                        scene_html += " checked=checked";
                    scene_html += "/>";
                    scene_html += "<label for='" + id + "'>" + scene.name + "</label>";
                }
            });

            scene_container.append(scene_html);

            scene_container.trigger('create');      // Apply styles
        },
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function show_chase() {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/chases/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            var chase_container = $("#chaseChange");
            chase_container.empty();

            var chase_html = "<input type='radio' name='select_chase' id='chase_0' value=0 data-theme='c' checked=checked/>";
            chase_html += "<label for='chase_0'>Chase Stopped</label>";

            $.map(json, function (chase, index) {
                if (current_act == 0 || chase.acts.indexOf(current_act) != -1) {
                    var id = 'chase_' + chase.id;

                    chase_html += "<input type='radio' name='select_chase' id='" + id + "' value='" + chase.id + "' data-theme='c' ";
                    if (chase.is_running)
                        chase_html += " checked=checked";
                    chase_html += "/>";
                    chase_html += "<label for='" + id + "'>" + chase.name + "</label>";
                }
            });

            chase_container.append(chase_html);

            chase_container.trigger('create');      // Apply styles
        },
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function show_fixtures() {
    if (fixtures != null) {
        updateFixtures()
        return;
    }

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/fixtures/",
        cache: false,
        success: function (data) {
            fixtures = jQuery.parseJSON(data);
            updateFixtures();
        },
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function getFixtureById(id) {
    if (fixtures != null) {
        for (var i = 0; i < fixtures.length; i++)
            if (fixtures[i].id == id)
                return fixtures[i];
    }

    return null;
}

// ----------------------------------------------------------------------------
//
function updateFixtures() {
    var fixture_select = $("#fixture_select_id");
    fixture_select.empty();
    var fixtureSliders = $("#fixtureSliders");
    fixtureSliders.empty();

    var first = true;
    var captured = 0;

    $.map(fixtures, function (fixture, index) {
        var label = (fixture.is_group) ? "Group " : "";

        fixture_select.append($('<option>', {
            value: fixture.id,
            text: label + fixture.full_name,
            selected: fixture.is_active || first
        }));

        // Add sliders
        if (fixture.is_active) {
            var slider_div = $('<div class="ui-field-contain">');
            var title = "";
            if (fixture.is_group)
                title = "Group ";

            slider_div.append( $('<b>').text(title + fixture.full_name) );

            var form = $('<form>');

            $.map(fixture.channels, function (channel, index) {
                var id = 'slider_' + fixture.id + '_' + channel.channel;

                form
                    .append($('<label>', {
                        'for': id
                    }).text(channel.name))
                    .append( $('<input>', {
                        'type': 'range',
                        'class' : 'channel_slider',
                        'name': id,
                        'id': id,
                        'data-highlight': true,
                        'value': channel.value,
                        'min': 0,
                        'max': 255
                    })
                );
            });

            slider_div.append(form);

            fixtureSliders
                .append('<hr>')
                .append(slider_div)
                .append($('<a>', {
                    'data-icon': 'delete',
                    'data-role': 'button',
                    'data-theme': 'a',
                    'data-mini': true
                }).text('Release ' + fixture.full_name).on('click', function () { release_fixture(fixture.id); return false; }));

            captured++;
        }

        first = false;
    });

    if (captured > 1) {
        fixtureSliders.append($('<a>', {
            'data-icon': 'delete',
            'data-role': 'button',
            'data-theme' : 'a',
            'data-mini': true
        }).text('Release All').on( 'click', function() { release_fixture( 0 ); return false; } ) );
    }

    fixture_select.selectmenu("refresh");

    $('#fixture_content').trigger('create');      // Apply styles

    // Add handlers (this is better then onchange as we get a single event)
    $('#fixtureSliders .channel_slider').slider({
        stop: function (event, ui) {
            var data = this.id.split('_');
            change_fixture_channel( parseInt(data[1]), parseInt(data[2]), this.value);
        }
    });
}

// ----------------------------------------------------------------------------
//
function change_fixture_channel(fixture_id, channel, channel_value) {
    var fixture = getFixtureById(fixture_id);
    if (fixture == null)
        return;

    var what = (fixture.is_group) ? "group" : "fixture";

    fixture.channels[channel].value = channel_value;

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/channel/" + what + "/" + fixture_id + "/" + channel + "/" + channel_value,
        cache: false,
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function capture_fixture() {
    var fixture = getFixtureById(fixture_select_id.value);
    if (fixture == null)
        return;

    var what = (fixture.is_group) ? "group" : "fixture";

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/select/" + what + "/" + fixture_select_id.value,
        cache: false,
        success: function (data) {
            fixture.is_active = true;
            channel_data = jQuery.parseJSON(data);
            for (var ch = 0; ch < channel_data.length; ch++)
                fixture.channels[ch].value = channel_data[ch];
            updateFixtures();
        },
        error: onError
    });
}

// ----------------------------------------------------------------------------
//
function release_fixture(fixture_id) {
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/control/fixture/release/" + fixture_id,
        cache: false,
        success: function (data) {
            $.map(fixtures, function (fixture, index) {
                if (fixture_id == 0 || fixture_id == fixture.id)
                    fixture.is_active = false;
            });

            updateFixtures();
        },
        error: onError
    });
}