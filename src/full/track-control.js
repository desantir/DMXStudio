/* 
Copyright (C) 2012-2017 Robert DeSantis
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

// NOTE: Some of these globals are accessed outside of this class

var playing_track_link = null;                      // Current music player track
var playing_track_name = "";
var playing_track_position = 0;                     // Tracks play time of current track
var playing_track_length = 0;
var playing_track_bpm = null;
var playing_track_paused = false;

// ----------------------------------------------------------------------------
//
function TrackControl( track_control_id, track_status_id ) {

    // CONSTRUCTOR
    var track_control = $("#"+track_control_id);

    this.notify_callback = null;
    this.last_player_error = null;

    this.cache_track_stop_button = track_control.find( ".track_stop" );
    this.cache_track_play_button = track_control.find( ".track_play");
    this.cache_track_pause_button = track_control.find(".track_pause");
    this.cache_track_back_button = track_control.find(".track_back");
    this.cache_track_forward_button = track_control.find(".track_forward");
    this.cache_track_back_info_button = track_control.find(".track_back_info");
    this.cache_track_forward_info_button = track_control.find(".track_forward_info");
    this.cache_track_title = track_control.find('.track_title');
    this.cache_track_remaining = track_control.find('.track_remaining');
    this.cache_track_length = track_control.find('.track_length');
    this.cache_track_status = $('#' + track_status_id);

    this.cache_track_title.text( "" );
    this.cache_track_remaining.text( "" );
    this.cache_track_length.text("");
    this.cache_track_status.text("");

    track_control.css('display', 'block');
    this.cache_track_status.css('display', 'block');
    this.cache_track_remaining.css('display', 'block');

    var self = this;

    this.cache_track_back_info_button.click( function( event ) { stopEventPropagation(event); self.track_back_info(); } );
    this.cache_track_forward_info_button.click( function( event ) { stopEventPropagation(event); self.track_forward_info(); } );
    this.cache_track_back_button.click( function( event ) { stopEventPropagation(event); self.track_back(); } );
    this.cache_track_stop_button.click( function( event ) { stopEventPropagation(event); self.track_stop(); } );
    this.cache_track_play_button.click( function( event ) { stopEventPropagation(event); self.track_play(); } );
    this.cache_track_pause_button.click( function( event ) { stopEventPropagation(event); self.track_pause(); } );
    this.cache_track_forward_button.click( function( event ) { stopEventPropagation(event); self.track_forward(); } );

    // ----------------------------------------------------------------------------
    //
    this.update_status = function( music_player_status ) {
        if ( music_player_status.logged_in && music_player_status.playing != null ) {
            var playing = music_player_status.playing;

            if ( playing == null || playing.link == null || playing.link === "" ) {
                this.stopTrackEvent( null );
            }
            else {
                var start = playing_track_link != playing.link;

                playing_track_link = playing.link;
                playing_track_name = playing.name;
                playing_track_position = playing.length - playing.remaining;
                playing_track_length = playing.length;
                playing_track_bpm = playing.bpm;
                playing_track_paused = playing.paused;

                var track_length = trackTime( playing_track_length, false );

                this.cache_track_title.text( playing_track_name );
                this.cache_track_length.text( playing_track_bpm == null ? track_length : track_length + " | " + Math.round(playing_track_bpm) + " BPM" );

                this.timeTrackEvent( playing_track_link, playing_track_position );

                if ( playing.paused )
                    this.pauseTrackEvent( playing_track_link );
                else if ( start ) 
                    this.startTrackEventInternal( playing_track_link );
                else 
                    this.resumeTrackEvent( playing_track_link );
            }

            this.changeTrackQueueEvent( music_player_status.played, music_player_status.queued );
        }

        // Handle any new music error
        if ( this.last_player_error != music_player_status.player_error ) {
            this.last_player_error = music_player_status.player_error;
            if ( this.last_player_error != null )
                toastError( this.last_player_error );
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.updateUI = function( music_player_status ) {
        if ( music_player_status != null && music_player_status != undefined ) {
            this.update_status( music_player_status );
            return;
        }

        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/venue/status/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);
                self.update_status( json.music_player );
            },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.setNotifyCallback = function( notify_callback ) {
        this.notify_callback = notify_callback;
    }

    // ----------------------------------------------------------------------------
    //
    this.notify = function( action, id ) {
        if (this.notify_callback != null)
            this.notify_callback( action, id );
    }

    // ----------------------------------------------------------------------------
    //
    this.getPlayingTrackLink = function () {
        return playing_track_link;
    }

    // ----------------------------------------------------------------------------
    //
    this.pauseTrackEvent = function( track_link ) {
        if ( playing_track_link == track_link ) {
            enable_player_button(this.cache_track_stop_button, false );
            enable_player_button(this.cache_track_play_button, true );
            enable_player_button(this.cache_track_pause_button,  false );

            this.notify( "pause", track_link );
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.stopTrackEvent = function( track_link ) {
        if ( playing_track_link == track_link ) {
            this.cache_track_remaining.text( "no track" );
            this.cache_track_status.text( "no track" );
            this.cache_track_title.text( "" );
            this.cache_track_length.text( "" );

            enable_player_button(this.cache_track_stop_button, false);
            enable_player_button(this.cache_track_play_button, false);
            enable_player_button(this.cache_track_pause_button, false);

            playing_track_link = null;
            playing_track_name = "";
            playing_track_position = 0;
            playing_track_length = 0;
            playing_track_bpm = null;

            this.notify( "stop", track_link );
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.resumeTrackEvent = function( track_link ) {
        if ( playing_track_link == track_link ) {
            enable_player_button(this.cache_track_stop_button, true );
            enable_player_button(this.cache_track_play_button, false );
            enable_player_button(this.cache_track_pause_button,  true );

            this.notify( "resume", track_link );
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.startTrackEvent = function( track_link, position_ms ) {
        // Fetch all track data on start and update the UI
        this.updateUI();
    }

    // ----------------------------------------------------------------------------
    //
    this.startTrackEventInternal = function( track_link ) {
        enable_player_button(this.cache_track_stop_button, true );
        enable_player_button(this.cache_track_play_button, false );
        enable_player_button(this.cache_track_pause_button,  true );

        this.notify( "start", track_link  );
    }

    // ----------------------------------------------------------------------------
    //
    this.timeTrackEvent = function( track_link, position_ms ) {
        if ( playing_track_link == track_link ) {
            playing_track_position = position_ms;

            var status = trackTime(playing_track_position,false) + " / -" + trackTime(playing_track_length - position_ms,false);

            this.cache_track_remaining.text( status );
            this.cache_track_status.text( status );

            // this.notify( "time", track_link );
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.changeTrackQueueEvent = function( played_size, queued_size ) {
        enable_player_button(this.cache_track_back_button, played_size != 0);
        enable_player_button(this.cache_track_forward_button, queued_size != 0);

        enable_info_button(this.cache_track_back_info_button, played_size != 0);
        enable_info_button(this.cache_track_forward_info_button, queued_size != 0);

        this.notify( "list", null );
    }

    // ----------------------------------------------------------------------------
    //
    this.track_back = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/back/",
            cache: false,
            success: function() { self.updateUI(); },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.track_stop = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/stop/",
            cache: false,
            success: function() { self.updateUI(); },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.track_pause = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/pause/",
            cache: false,
            success: function() { self.updateUI(); },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.track_forward = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/forward/",
            cache: false,
            success: function() { self.updateUI(); },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.track_play = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/track/play/",
            cache: false,
            success: function() { self.updateUI(); },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.track_play_new = function( track_link, queue_track ) {
        // If requested track = current, then may be paused, just start it
        if ( track_link == playing_track_link ) {
            this.track_play();
            return false;
        }

        var self = this;

        var track_action = (queue_track) ? "queue" : "play";

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/music/" + track_action + "/track/" + track_link,
            cache: false,
            success: function() { self.updateUI(); },
            error: onAjaxError
        });

        return true;
    }

    // ----------------------------------------------------------------------------
    //
    this.track_back_info = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/played/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);
                showStaticTrackList( self, 'Played Tracks', PLAYED_PLAYLIST_LINK, json.tracks );
            },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.track_forward_info = function() {
        var self = this;

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/queued/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);
                showStaticTrackList( self, 'Queued Tracks', QUEUED_PLAYLIST_LINK, json.tracks );
            },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.playlist_play = function( playlist_link ) {
        var self = this;

        if (playlist_link != null && playlist_link.length> 0 ) {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/0",
                cache: false,
                success: function() { self.updateUI(); },
                error: onAjaxError
            });
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.playlist_queue = function( playlist_link ) {
        var self = this;

        if (playlist_link != null && playlist_link.length > 0 ) {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/music/play/playlist/" + playlist_link + "/1",
                cache: false,
                success: function() {
                    self.updateUI();
                    self.notify();
                },
                error: onAjaxError
            });
        }
    }
}

// ----------------------------------------------------------------------------
//
function enable_info_button(info_button, enabled) {
    var current_enable = !info_button.hasClass("ui-icon-blank")

    if (current_enable != enabled) {
        info_button.attr('disabled', !enabled);

        if (enabled)
            info_button.removeClass("ui-icon-blank").addClass("ui-icon-star");
        else
            info_button.removeClass("ui-icon-star").addClass("ui-icon-blank");
    }
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
var static_track_list_dialog = null;

function showStaticTrackList( track_control, title, playlist_link ) {
   var update_func = function () {
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/" + (playlist_link === PLAYED_PLAYLIST_LINK ? "played" : "queued") + "/",
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                tracks = json.tracks;

                var track_list = $('#stld_tracks');
                track_list.empty();

                var html = "";

                $.each(tracks, function (index, track) {
                    var css_class = index & 1 ? "tl_odd" : "tl_even";

                    html += '<div class="' + css_class + '" title="play" onclick="stopEventPropagation(event); track_control.track_play_new(\'' + track.link + '\', false);">';
                    html += '<div class="tl_item tl_track" style="padding-left: 4px;">' + track.track_name + '</div>';
                    html += '<div class="tl_item tl_time">' + trackTime(parseInt(track.duration),false) + '</div>';
                    html += '<div class="tl_item tl_artist">' + track.artist_name + '</div>';
                    html += '</div>';
                });

                track_list.append(html);
            },

            error: onAjaxError
        });
    }

    if ( static_track_list_dialog == null ) {
        static_track_list_dialog = $(
            '<div id="static_track_list_dialog" style="display:none;">' +
            '    <div class="tl_title">' +
            '        <div class="tl_item tl_track" style="padding-left: 4px;">TRACK</div>' +
            '        <div class="tl_item tl_time">TIME</div>' +
            '        <div class="tl_item tl_artist">ARTIST</div>' +
            '    </div>' +
            '    <div id="stld_tracks" class="track_list" style="clear: both; float: left; width: 720px !important;"></div>' +
            '</div>' );
    }

    if ($('#static_track_list_dialog').dialog('isOpen') === true) {
        $('#static_track_list_dialog').dialog('option', 'title', title);
        update_func();
    }
    else {
        track_control.setNotifyCallback(function (action, id) {
            if (action == "list")
                update_func();
        });

        static_track_list_dialog.dialog({
            modal: false,
            height: 620,
            width: 750,
            title: title,
            open: function () { // Stop main body scroll
                // $("body").css("overflow", "hidden");
                update_func();
            },
            close: function () {
                // $("body").css("overflow", "auto");
                track_control.setNotifyCallback(null);
            },
            buttons: {
                'Close': function () {
                    $(this).dialog("close");
                }
            }
        });
    }
}