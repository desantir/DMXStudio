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

var current_track_filter = null;                    // Current filter applied to track list
var current_playlist = null;                        // Currently selected playlist (global)

var FilterType = {
    NONE: 0,
    WORDS: 1,
    BPM: 2
};

// ----------------------------------------------------------------------------
//
function TrackSelector( track_container, track_control, playlist_select_callback, track_select_callback ) {

    // ----------------------------------------------------------------------------
    //
    this.playlist_change = function ( playlist_id ) {
        var self = this;

        self.track_list.empty();
        self.resetTrackListSelection();
        self.track_list_track_link = null;
        self.track_list.addClass( "track_list_wait" );

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_id,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                if ( self.playlist_select_callback != null )
                    self.playlist_select_callback(playlist_id);

                self.track_list_playlist_link = playlist_id;

                self.track_stop_button.unbind( 'click' ).click( function( event ) { stopEventPropagation(event); self.track_control.track_stop(); } );
                self.track_pause_button.unbind( 'click' ).click( function( event ) { stopEventPropagation(event); self.track_control.track_pause(); } );
                self.track_play_button.unbind( 'click' ).click( function ( event ) {
                    stopEventPropagation( event );

                    self.track_control.track_play_new( self.track_list_track_link, false );
                } );

               self.track_queue_button.unbind( 'click' ).click( function ( event ) {
                    stopEventPropagation( event );

                    if ( self.track_control.track_play_new( self.track_list_track_link, true ) )
                        rotateIcon( $(event.target), 1000 );
                } );

                self.track_metadata = [];

                self.track_list.removeClass( "track_list_wait" );

                $.each( json.tracks, function ( index, track ) {
                    var is_playing = track.link == self.track_control.getPlayingTrackLink();

                    if ( is_playing )
                        self.track_list_playing = self.track_control.getPlayingTrackLink();

                    track.html_id = "tl_" + index;          // Always assign a unique index

                    self.track_metadata[track.link] =  track;
                } );

                if ( self.track_filter_timeout != null ) {
                    clearTimeout( self.track_filter_timeout );
                    self.track_filter_timeout = null;
                }

                self.filterTracks( self.track_filter.val(), true );
            },
            error: onAjaxError
        });
    }

    // ----------------------------------------------------------------------------
    //
    this.scheduleTrackFilter = function( event ) {
        stopEventPropagation( event );

        var filter = this.track_filter.val();

        if ( filter == current_track_filter )
            return;

        if ( this.track_filter_timeout != null )
            clearTimeout(  this.track_filter_timeout );

        var self = this;

         this.track_filter_timeout = setTimeout( function( filter ) {
                 self.filterTracks( filter, false );
             }, 250, filter );
    }

    // ----------------------------------------------------------------------------
    //
    this.filterTracks = function( filter, create_list ) {
        var line = 0;

        var filter_type = FilterType.NONE;
        var filter_words;
        var filter_bpm_low, filter_bpm_high;

        if ( filter != null ) {
            filter.trim().toLowerCase();
            filter_words = ( filter.length > 0 ) ? filter.split( ' ' ) : [];

            if ( filter_words[0] == "bpm" && filter_words.length >= 2 && filter_words.length <= 3 ) {
                filter_bpm_low = parseInt( filter_words[1] );
                filter_bpm_high = filter_words.length == 3 ? parseInt( filter_words[2] ) : filter_bpm_low;
                filter_type = FilterType.BPM;
            }
            else if ( filter_words.length > 0 )
                filter_type = FilterType.WORDS;
        }

        if ( create_list )
            this.track_list.empty();

        var self = this;

        for ( track_id in this.track_metadata ) {
            var track = this.track_metadata[track_id];
            var match = true;

            // Apply filter if we have one
            switch ( filter_type ) {
                case FilterType.WORDS:
                    var track_name = track.track_name.toLowerCase();
                    var artist_name = track.artist_name.toLowerCase();

                    for ( var i=0; match && i < filter_words.length; i++ )
                        match = track_name.indexOf( filter_words[i] ) != -1 || artist_name.indexOf( filter_words[i] ) != -1;
                    break;

                case FilterType.BPM:
                    if ( track.audio_info != null ) {
                        var track_bpm = Math.round( track.audio_info.bpm );
                        match = track_bpm >= filter_bpm_low && track_bpm <= filter_bpm_high;
                    }
                    break;

                case FilterType.NONE:
                    match = true;
                    break;
            }

            if ( create_list ) {
                var css_class = (line++ & 1) ? "tl_odd" : "tl_even";
                var is_playing = track.link == this.track_control.getPlayingTrackLink();

                var html = '<div id="' + track.html_id + '" class="' + css_class + '" track_link="' + track.link + '" style="display:' + 
                    ((match) ? "inline" : "none" ) + ';">';

                var title = (!is_playing) ? "queue" : "stop";
                var icon = (!is_playing) ? "ui-icon-flag" : "ui-icon-volume-on";

                html += '<div class="tl_status tl_item tl_icon ui-icon ' + icon + '" title="' + title + '"></div>';
                html += '<div class="tl_item tl_track">' + track.track_name + '</div>';
                html += '<div class="tl_item tl_time">' + trackTime(track.duration,false) + '</div>';

                if (track.audio_info != null) {
                    html += '<div class="tl_item tl_bpm">' + Math.round( track.audio_info.bpm ) + '</div>';
                }
                else {
                    html += '<div class="tl_item tl_bpm">&nbsp;</div>';
                }

                html += '<div class="tl_item tl_artist">' + track.artist_name + '</div>';
                html += '</div>';

                var track_line = $( html );

                track_line.on( "click", function ( event ) {
                    var target = $( event.target );
                    var current_target = $( event.currentTarget );
            
                    if ( target.hasClass( "tl_icon" ) )
                        self.trackListAction( current_target.attr( "track_link" ) );
                    else
                        self.trackListSelect( current_target.attr( "track_link" ) );
                });

                this.track_list.append( track_line );
            }
            else {
                var track_line = $( "#" + track.html_id );
                if ( track_line.length != 1 )
                    continue;

                if ( match ) {
                    track_line.show();
                    track_line.removeClass().addClass( (line++ & 1) ? "tl_odd" : "tl_even" );
                }
                else {
                    track_line.hide();
                }
            }
        }

        current_track_filter = filter;
    }

    // ----------------------------------------------------------------------------
    //
    this.updateTrackIcon = function( id, is_playing ) {
        if ( !this.track_metadata.hasOwnProperty( id ) )
            return;

        this.track_list_playing = is_playing ? id : null;

        var track_div = $( "#" + this.track_metadata[id].html_id );
        if ( track_div.length == 0 )  // Track may be filtered
            return;

        var status_div = track_div.find( ".tl_status" );

        if ( is_playing ) {
            status_div.title = "stop";
            status_div.addClass( "ui-icon-volume-on" ).removeClass( "ui-icon-flag" );
        }
        else {
            status_div.title = "queue";
            status_div.removeClass( "ui-icon-volume-on" ).addClass( "ui-icon-flag" );
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.resetTrackListSelection = function() {
        this.track_name_div.html( "" );
        this.track_artist_div.html( "" );
        this.track_album_div.html( "" );
        this.track_time_div.html( "" );
        this.track_bpm_div.html( "" );
        this.track_image.hide();
        this.track_image.attr( 'src', '' );

        this.track_controls_div.hide();

        this.track_list_playlist_link = null;
        this.track_list_track_link = null;
    }

    // ----------------------------------------------------------------------------
    //
    this.reload = function () {
        this.resetTrackListSelection();

        initializePlaylistSelector( this.playlist_select, null, 770, function( playlist_id ) {
            self.playlist_change( playlist_id );
        } );
    }

    // ----------------------------------------------------------------------------
    //
    this.updateTrackSelectButtons = function( state, current_track_link ) {
        var stop=false, play=false, pause=false;

        switch ( state ) {
            case "stop":
            case "pause":
                play = true;
                break;

            case "init":
            case "start":
            case "resume":
                if ( this.track_list_track_link != current_track_link )
                    play = true;
                else {
                    stop = true;
                    play = this.playing_track_paused;
                    pause = !this.playing_track_paused;
                }
                break;

            default:
                return;
        }

        enable_player_button( this.track_stop_button, stop );
        enable_player_button( this.track_play_button, play );
        enable_player_button( this.track_pause_button, pause );   
        enable_player_button( this.track_queue_button, this.track_list_track_link != this.track_control.getPlayingTrackLink() );
    }

    // ----------------------------------------------------------------------------
    //
    this.trackListSelect = function( track_link ) {
        if ( !this.track_metadata.hasOwnProperty(track_link) )
            return;

        var track = this.track_metadata[track_link];

        this.track_list_track_link = track_link;

        this.track_name_div.html( track.track_name );
        this.track_artist_div.html( track.artist_name );
        this.track_album_div.html( track.album_name );
        this.track_time_div.html( trackTime( track.duration, false ) );

        if ( 'href_sm_image' in track ) {
            this.track_image.attr( 'src', track.href_sm_image );
            this.track_image.show();
        }

        if ( track.audio_info != undefined ) 
            this.track_bpm_div.html( "&nbsp;|&nbsp;" + Math.round( track.audio_info.bpm) + " BPM" );
        else
            this.track_bpm_div.html( "" );
            
        this.track_controls_div.show();

        this.updateTrackSelectButtons( "init", this.track_control.getPlayingTrackLink() );

        if ( this.track_select_callback != null )
            this.track_select_callback( track_link );
    }

    // ----------------------------------------------------------------------------
    //
    this.trackListAction = function (track_link) {
        if ( track_link == this.track_control.getPlayingTrackLink() ) {
            this.track_control.track_stop();
            return;
        }

        var queue = (this.track_control.getPlayingTrackLink() != null);

        if (queue)
            rotateIcon($(event.target), 1000 );

        this.track_control.track_play_new( track_link, queue );
    }

    // ----------------------------------------------------------------------------
    // CONSTRUCTOR

    this.track_container = track_container;
    this.track_control = track_control;
    this.playlist_select_callback = playlist_select_callback;
    this.track_select_callback = track_select_callback;

    this.track_metadata = [];
    this.track_filter_timeout = null;

    // Track list select dialog state
    this.track_list_playlist_link = null;
    this.track_list_track_link = null;
    this.track_list_playing = null;

    this.track_name_div = track_container.find('#tld_info .ti_track');
    this.track_artist_div = track_container.find('#tld_info .ti_artist');
    this.track_album_div = track_container.find('#tld_info .ti_album');
    this.track_time_div = track_container.find('#tld_info .ti_time');
    this.track_bpm_div = track_container.find('#tld_info .ti_bpm');
    this.track_image = track_container.find('#tld_info .ti_img');
    this.track_controls_div = track_container.find('#tld_info .ti_controls');
    this.playlist_select = track_container.find( "#tl_playlist_select" );
    this.track_list = track_container.find( "#tld_tracks" );
    this.track_filter = track_container.find( ".track_filter" );

    this.track_stop_button = track_container.find( ".track_stop" );
    this.track_pause_button = track_container.find( ".track_pause" );
    this.track_play_button = track_container.find( ".track_play" );
    this.track_queue_button = track_container.find( ".track_queue" );

    var self = this;

    this.track_filter.unbind( "input" ).on( "input", function( event ) { self.scheduleTrackFilter( event ); } );

    this.track_control.setNotifyCallback( function( action, id ) {
        self.updateTrackSelectButtons( action, id );

        switch ( action ) {
            case "start":
            case "resume": {
                var playing_id = self.track_list_playing;

                if ( playing_id == id )
                    break;

                if ( playing_id != null )
                    self.updateTrackIcon( playing_id, false );

                self.updateTrackIcon( id, true );
                break;
            }

            case "stop":
                if ( self.track_list_playing == id )
                    self.updateTrackIcon( id, false );                    
                break;

            default:
                break;
        }
    } );

    this.reload();
}

// ----------------------------------------------------------------------------
//
function initializePlaylistSelector( playlist_select, css_class, min_width, update_func ) {
    playlist_select.multiselect({
        minWidth: min_width, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: css_class, height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        update_func( ui.value );
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

            if ( current_playlist != null )         // Must send update to refresh tracks
                update_func( current_playlist );
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function getCurrentPlaylist() {
    return current_playlist;
}

// ----------------------------------------------------------------------------
//
function setCurrentPlaylist( playlist, selector_name ) {
    current_playlist = playlist;

    if ( selector_name != null ) {
        $("#" + selector_name + ' option[value="' + current_playlist + '"]').attr("selected", true);
        $("#" + selector_name ).multiselect("refresh");
    }
}