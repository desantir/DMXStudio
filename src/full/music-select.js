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

var client_config_update = false;                   // Client layout has changed -  update server

var volume_control = null;
var track_control = null;

var played_video_cache = [];

var video_window = null;
var video_window_ready = false;
var pending_video_request = null;

var video_panel = null;

var video_search_limit = 25;
var track_table_size = 0;

var session_id = null;

var track_selector = null;

var recent_videos = [];

// ----------------------------------------------------------------------------
//
function initializeMusicSelectUI() {
    initializeCollapsableSections();

    volume_control = new VolumeControl( "master_volume" );
    track_control = new TrackControl( "track_controls", "track_status" );

    video_panel = new VideoScrollPanel( "video_pane" );

    var track_pane_sizer = $("#track_pane").find(".table_size_icon");
    if (track_pane_sizer != null) {
        track_pane_sizer.addClass('ui-icon');
        track_pane_sizer.click(function (event) {
            stopEventPropagation(event);
            setTrackTableSize( track_table_size == 6 ? 19 : 6 );
            client_config_update = true;
        });
    }

    $("#video_limit_buttons").buttonset();

    $("#video_limit_buttons").change(function () {
        video_search_limit = $('input[name=video_limit]:checked').val();
        client_config_update = true;
    });

    $("#show_frequency_visualizer").click( showFrequencyVisualizer );

    $("#show_beat_visualizer").click( showBeatVisualizer );

    $("#show_amplitude_visualizer").click( showAmplitudeVisualizer );

    $("#spotify_signon").click( spotify_service_signon );

    $("#playlist_play").button().click(playlistPlay);
    $("#playlist_queue").button().click(playlistQueue);

    $("#video_pause").button().click(pauseVideoButton);
    $("#video_next").button().click(nextVideoButton);
    $("#video_recent").button().click(recentVideoButton);

    $( "#tl_video_search" ).attr("disabled", false);
    $( "#tl_video_search_query" ).attr("disabled", false);
    
    $( "#tl_video_search" ).click( function( event ) { videoSearch( event, tl_video_search_query.value ); } );
    $( "#tl_video_search_query" ).on( "keydown", function( event ) { if ( event.keyCode == 13 ) { videoSearch( event, tl_video_search_query.value ); } } );

    window.addEventListener( "message", function( event ) {
        if ( event.data.method === "videoStateChange" ) {
            // toastNotice( event.data.method + " " + event.data.action );
            handleVideoEvent( event.data );
        }
        else if ( event.data.method === "eventBroadcast" ) {
            handleEvent( event.data.event );
        }
    });

    // Make sure we stay registered with our opener
    if (  window.opener != null && ! window.opener.closed )
        window.opener.postMessage( { "method": "registerEventListener" }, "*" );

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/layout/",
        cache: false,
        success: function (data) {
            var client_config = jQuery.parseJSON( data );
            restoreVenueLayout( client_config );
            track_selector = new TrackSelector( $( "#track_list" ), track_control, changeCurrentPlaylist, trackVideos );
        },

        error: onAjaxError
    });

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/venue/status/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            session_id = json.session_id;

            updateVolume( json.master_volume );
            updateVolumeMute( json.mute );

            track_control.updateUI( json.music_player );
        },
        error: onAjaxError
    });

    venueLayoutUpdater();
}

// ----------------------------------------------------------------------------
//
function venueLayoutUpdater() {
    if ( client_config_update ) {
        var config_update = {
            "playlist": getCurrentPlaylist(),
            "sound_pane": {
                "collapsed": isSectionCollapsed("sound_pane")
            },
            "track_pane": {
                "collapsed": isSectionCollapsed("track_pane"),
                "table_size": track_table_size
            }, 
            "video_pane": {
                "scroll": video_panel.isScrollContent(),
                "collapsed": isSectionCollapsed("video_pane"),
                "video_search_limit": video_search_limit,
                "recent_videos": recent_videos
            },
        };

        if (  window.opener != null && ! window.opener.closed ) {
            window.opener.postMessage( { "method": "updateClientConfig", "config": config_update }, "*" );
            client_config_update = false;
        }
    }

    setTimeout( venueLayoutUpdater, 500 );
}

// ----------------------------------------------------------------------------
//
function updateRecentVideos(video_id) {

    for ( var index = 0; index < recent_videos.length; index++ )
        if ( recent_videos[index] === video_id ) {
            recent_videos.splice(index, 1);
            break;
        }

    recent_videos.splice(0, 0, video_id);

    // Keep length at 25 or less
    if ( recent_videos.length > 40 )
        recent_videos.splice(40, 1);

    client_config_update = true;
}

// ----------------------------------------------------------------------------
//
function removeRecentVideo(video_id) {
    for (var index = 0; index < recent_videos.length; index++)
        if (recent_videos[index] === video_id) {
            recent_videos.splice(index, 1);
            client_config_update = true;
            break;
        }

    recentVideoButton(null);
}

// ----------------------------------------------------------------------------
//
function restoreVenueLayout( client_config ) {
    for (var prop in client_config) {
        if (prop == "playlist")
            setCurrentPlaylist( client_config.playlist, null );
        else if (prop == "sections") {
            for (section in client_config.sections) {
                if (section == "sound_pane") {
                    setSectionCollapsed("sound_pane", client_config.sections.sound_pane.collapsed);
                }
                else if (section == "track_pane") {
                    setSectionCollapsed("track_pane", client_config.sections.track_pane.collapsed);
                    setTrackTableSize( client_config.sections.track_pane.table_size );
                }
                else if (section == "video_pane") {
                    video_panel.setScrollContent(client_config.sections.video_pane.scroll);
                    setSectionCollapsed("video_pane", client_config.sections.video_pane.collapsed);
                    video_search_limit = client_config.sections.video_pane.video_search_limit;

                    if (client_config.sections.video_pane.hasOwnProperty( "recent_videos" ) ) 
                        recent_videos = client_config.sections.video_pane.recent_videos;

                    $( "input:radio[name=video_limit]" ).filter( "[value='" + video_search_limit + "']" ).prop( "checked", true );

                    $("#video_limit_buttons").buttonset( "refresh" );
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
function setTrackTableSize( table_size ) {
    track_table_size = table_size;

    $( "#tld_tracks" ).css( "height", (26 * table_size) + "px" );

    var track_pane_sizer = $("#track_pane").find(".table_size_icon");

    if (table_size == 6)
        track_pane_sizer.removeClass('ui-icon-arrowthickstop-1-s')
                            .addClass('ui-icon-arrowthickstop-1-n')
                            .attr('title', 'large');
    else
        track_pane_sizer.removeClass('ui-icon-arrowthickstop-1-n')
                            .addClass('ui-icon-arrowthickstop-1-s')
                            .attr('title', 'small');
}

// ----------------------------------------------------------------------------
//
function updateVolumeMute( mute ) {
    volume_control.setMute( mute );
}

// ----------------------------------------------------------------------------
//
function updateVolume( master_volume ) {
    volume_control.setVolume( master_volume );
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
    // Pause video when tracked played
    if ( video_window_ready )
        video_window.postMessage( JSON.stringify( { "action": "pause" } ), "*" );

    track_control.resumeTrackEvent( track_link );
}

// ----------------------------------------------------------------------------
//
function startTrackEvent( track_link, position_ms ) {
    // Pause video when tracked played
    if ( video_window_ready )
        video_window.postMessage( JSON.stringify( { "action": "pause" } ), "*" );

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
function pauseVideoButton( event ) {
    stopEventPropagation( event );

    if ( video_window != null && !video_window.closed && video_window_ready )
        video_window.postMessage( JSON.stringify( { "action": "toggle_pause" } ), "*" );
}

// ----------------------------------------------------------------------------
//
function recentVideoButton(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/query/video/ids/",
        data: JSON.stringify({ "video_ids": recent_videos }),
        cache: false,
        contentType: 'application/json',
        success: function (data) {
            var json = jQuery.parseJSON(data);

            updateVideos( json.videos, null, true );
        },

        error: function (data) {
            onAjaxError(null, data.statusText);
        }
    });


}

// ----------------------------------------------------------------------------
//
function nextVideoButton( event ) {
    stopEventPropagation( event );

    if ( video_window != null && !video_window.closed && video_window_ready )
        video_window.postMessage( JSON.stringify( { "action": "next" } ), "*" );
}

// ----------------------------------------------------------------------------
//
function playlistPlay(event) {
    stopEventPropagation(event);

    track_control.playlist_play( $("#tl_playlist_select").val() );
}

// ----------------------------------------------------------------------------
//
function playlistQueue(event) {
    stopEventPropagation(event);

    track_control.playlist_queue( $("#tl_playlist_select").val() );
}

// ----------------------------------------------------------------------------
//
function changeCurrentPlaylist(playlist) {
    setCurrentPlaylist( playlist, "tl_playlist_select" )
    client_config_update = true;
}

// ----------------------------------------------------------------------------
//
function trackVideos( track_link ) {
    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/query/music/videos/",
        data: JSON.stringify( { "track_link": track_link, "count": video_search_limit } ),
        cache: false,
        contentType: 'application/json',
        success: function( data ) {
            var json = jQuery.parseJSON(data);

            updateVideos( json.videos, track_link, false );
        },

        error: function( data ) {
            resetTrackListSelection();
                
            onAjaxError( null, data.statusText );
        }
    });
}

// ----------------------------------------------------------------------------
//
function videoSearch(event, search) {
    stopEventPropagation(event);

    if ( search == null || search.length == 0 )
        return;

    var video_search = $( "#tl_video_search" );
    var video_search_query = $( "#tl_video_search_query" );

    video_search.attr("disabled", true);
    video_search_query.attr("disabled", true);

    rotateX( video_search, 2000 );

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/query/video/search/",
        data: JSON.stringify( { "search": search, "count": video_search_limit } ),
        contentType: 'application/json',
        cache: false,
        success: function( data ) {
            video_search.attr("disabled", false);
            video_search_query.attr("disabled", false);

            var json = jQuery.parseJSON(data);

            updateVideos( json.videos, null, false );
        },

        error: function( data ) {
            video_search.attr("disabled", false);
            video_search_query.attr("disabled", false);

            resetTrackListSelection();
                
            track_videos_div.html( "Error reading track video list." );

            onAjaxError( null, data.statusText );
        }
    });
}

// ----------------------------------------------------------------------------
//
function videoPaletteUpdateEvent( videoId, palette_type /* 0=default, 1=video */) 
{
    var video = video_panel.getVideo( videoId );

    if ( video != null ) {
        if ( palette_type == 1 )
            toastNotice( "Video palette for video '" + video.title + "' ready" );

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/video/palette/" + videoId,
            cache: false,
            success: function( data ) {
                var json = jQuery.parseJSON(data);
                video_panel.updateVideoPalette( videoId, json.default_palette, json.video_palette );
            }
        });
    }
}

// ----------------------------------------------------------------------------
//
function videoPaletteErrorEvent( videoId, palette_type /* 0=default, 1=video */ ) 
{
    var video = video_panel.getVideo( videoId );
    if ( video != null && palette_type == 1 ) {
        toastWarning( "Could not produce video palette for video '" + video.title + "'" );
        video_panel.updateVideoPalette( videoId, video.default_palette, video.video_palette );
    }
}

// ----------------------------------------------------------------------------
//
function updateVideos( videos, track_link, recents )
{
    video_panel.empty();

    // Populate videos
    $.each( videos, function( index, video ) {
        video.track_link = track_link;
        video.palette_time_indexes = [];

        for ( var i=0; i < video.video_palette.length; i++ )
            video.palette_time_indexes.push( video.video_palette[i].time_ms );

        var video_div = video_panel.addVideo( video );
        if ( video_div == null )
            return;
        
        if ( video.video_palette.length > 0 )
            video_div.find( ".video_play_with_palette" ).removeClass( "md-icon-white" ).addClass( "md-icon-blue" );

        var video_title_div = video_div.find( ".video_title" );
        video_title_div.css( "cursor", "pointer" );

        video_title_div.click( function( event ) { 
            stopEventPropagation(event); 

            var itemList = {
                    "title": { type: "html", html: "<span>Video Menu</span>" },
                    "sep1": "---------",
                    "palette": { name: "Make Full Video Palette", callback: function (key, opt) { generateVideoPalette( video.video_id, video_div); } }
                    // "thumbnmail": { name: "Make Thumbnail Palette (debug)", callback: function (key, opt) { remakeSimplePalette( video.video_id, video_div); } },
            };

            if (recents)
                itemList["remove"] = { name: "Remove From Recent Videos", callback: function (key, opt) { removeRecentVideo( video.video_id ); } };

            itemList["debug"] = { name: "Show Palettes (debug)", callback: function (key, opt) { showPalettes(video.video_id); } };

            $.contextMenu( 'destroy' );

            $.contextMenu({
                zIndex: 2000,
                selector: '.context-fixture-header', 
                trigger: 'none',
                animation: {duration: 250, show: 'fadeIn', hide: 'fadeOut'},
                items: itemList
            });

            $('.context-fixture-header').contextMenu( { x: event.pageX, y: event.pageY } );
        });

        video_div.click( video_div_click );
    } );
}

// ----------------------------------------------------------------------------
//
function video_div_click ( event ) { 
    stopEventPropagation( event );

    var target = $(event.target);
    var video_id = $(this).data( 'video_id' );

    if ( target.hasClass( "video_image" ) || target.hasClass( "video_play" ) || target.hasClass( "video_time" ) ) {
        showVideo( video_id, 0, true, false );
        rotateIcon( target, 1000 );
    }
    else if ( target.hasClass( "video_play_with_palette" ) ) {
        showVideo( video_id, 0, true, true );
        rotateIcon( target, 1000 );
    }
    else if ( target.hasClass( "video_queue" ) ) {
        showVideo( video_id, 0, false, false );
        rotateIcon( target, 1000 );
    }
    else if ( target.hasClass( "video_palette_chip" ) ) {
        if ( event.ctrlKey  || event.altKey ) {
            var video = video_panel.getVideo( video_id );
            ShowPalette( video.default_palette, video_info.default_weights, 2000, -1 );
            rotateY( target.parent(), 1000 );
        }
        else {
            showVideo( video_id, 0, false, true );
            rotateX( target.parent(), 1000 );
        }
    }
}

// ----------------------------------------------------------------------------
//
function showPalettes( video_id ) {
    var video = video_panel.getVideo( video_id );
    if ( video == null )
        return;

    var mydialog = $("#video_palette_visualizer_dialog");

    var video_image = mydialog.find( ".video_image" );
    var default_palette = mydialog.find( ".default_palette" );
    var video_palettes = mydialog.find( ".video_palettes" );

    video_image[0].src = video.thumbnails.medium.url;
    video_image.attr( "title", video.description );

    var html = makeWeightedPaletteChip(video.default_palette, video.default_weights, 30, 300 );
    default_palette.html( html );

    var vpalette = '';

    $.each( video.video_palette, function( index, palette ) {
        vpalette += '<div class="palette_visualizer_title" style="position:absolute; margin-top: 7px; left: 0px; top:'+ index*35 + 'px; width:46px; text-align: right;">' + trackTime(palette.time_ms, false) + '</div>';
        vpalette += '<div class="video_palette_debug" style="position:absolute; left: 50px; top:'+ index*35 + 'px; cursor: pointer;" ';
        vpalette += 'onclick="showVideo(\'' + video_id + '\',' + palette.time_ms/1000 + ', true, true );">' +
            makeWeightedPaletteChip( palette.palette, palette.weights, 30, 300 ) + '</div>';
    } );

    video_palettes.html( vpalette );

    video_image.unbind("click").on("click", function (event) {
        stopEventPropagation( event );
        showVideo( video_id, 0, true, true );
    });

    mydialog.dialog({
        title: "Video Palettes - " + video_id,
        autoOpen: false,
        width: 760,
        height: 700,
        modal: false,
        resizable: false,
        buttons: {
            Close: function () {
                $(this).dialog("close");
            }
        }
    }).dialog("open");
}

// ----------------------------------------------------------------------------
//
function generateVideoPalette( video_id, video_div ) {
    var json = {
        video_id: video_id
    };

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/create/video/palette/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        async: true,
        success: function ( data ) {
            var json = jQuery.parseJSON(data);

            if ( json.success ) {
                var icon = video_div.find( ".video_play_with_palette" );
                icon.removeClass( "md-icon-blue" ).addClass( "md-icon-white" );
                palette_animate( icon );
            }
            else
                toastWarning( "Unable to schedule video palette production - try again later" );

        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function remakeSimplePalette(video_id, video_div) {
    var json = {
        video_id: video_id
    };

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/create/image/palette/",
        data: JSON.stringify(json),
        contentType: 'application/json',
        cache: false,
        async: true,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            if ( json.success ) {
                var icon = video_div.find( ".video_play_with_palette" );
                icon.removeClass( "md-icon-blue" ).addClass( "md-icon-white" );
                palette_animate( icon );
            }
            else
                toastWarning("Unable to schedule palette production - try again later");

        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function palette_animate( icon ) {
    icon.css('rotation', 0);      // Reset rotation

    icon.animate(
        { rotation: 360 },
        {
            duration: 2000,
            step: function (now, fx) {
                $(this).css({ "transform": "rotateZ(" + now + "deg)" });
            },
            complete: function () {
                palette_animate( icon );
            }
        }
    );
}

// ----------------------------------------------------------------------------
//
function showVideo( video_id, start_seconds, play_now, show_palette ) {
    var video = video_panel.getVideo( video_id );
    if ( video == null )
        return;

    if ( playing_track_link != null && !playing_track_paused ) 
        track_control.track_pause( );

    if ( video_window == null || video_window.closed ) {
        var video_href = "video-player.htm";
        video_window_ready = false;

        video_window = window.open( video_href, "DMXSTUDIO_VIDEO PLAYER", "channelmode=yes,resizable=yes,scrollbars=yes" );
    }

    played_video_cache[video.video_id] = video;

    var request = { 
        "action": play_now ? "play" : "queue", 
        "video_id": video.video_id,
        "start_seconds": start_seconds, 
        "title": video.title,
        "track_id": video.track_link,
        "show_palette": show_palette,
        "palette_time_indexes": video.palette_time_indexes
   };

    if ( video_window_ready ) {
        video_window.postMessage( JSON.stringify( request ), "*" );
        pending_video_request = null;
    }
    else
        pending_video_request = request;
}

// ----------------------------------------------------------------------------
//
function handleVideoEvent( message ) {
    if ( message.action === "ready" ) {
        video_window_ready = true;

        video_window.onunload = function() {
            video_window = null;
            updateVideoState( "closed", null );
        }

        if ( pending_video_request != null ) {
            video_window.postMessage( JSON.stringify( pending_video_request ), "*" );    
            pending_video_request = null;
        }
    }
    else if ( message.action === "playing" ) {
        var video_info = played_video_cache[ message.video_info.video_id ];

        if ( message.video_info.show_palette && video_info != null && video_info.video_palette.length == 0 )
            ShowPalette(video_info.default_palette, video_info.default_weights, 2000, -1 );
        else
            $( "#video_palette_sample_div" ).hide();

        updateVideoState(message.action, message.video_info);

        updateRecentVideos( message.video_info.video_id );
    }
    else if ( message.action === "select_palette" ) {
        var video_info = played_video_cache[ message.video_info.video_id ];

        if (video_info != null && message.video_info.palette_index < video_info.video_palette.length) {
            var p = video_info.video_palette[message.video_info.palette_index];

            ShowPalette( p.palette, p.weights, 2000, p.time_ms);
        }
    }
    else if ( message.action === "ended" ||  message.action === "paused" ) {
        if ( message.video_info != null && message.video_info.show_palette ) {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/venue/whiteout/0",
                cache: false,
                error: onAjaxError
            });
        }

        updateVideoState( message.action, message.video_info );
    }
    else if ( message.action == "queued" )
        updateVideoState( message.action, message.video_info );
}

function updateVideoState( command, video_info ) {
    var state = "";
    var title = "";
    var queued = 0;
    var paused = false;
    var queue_list = "";

    if ( video_info != null && video_info.queue != null && video_info.queue.length > 0 ) {
        for ( var i=0; i < video_info.queue.length; i++ )
            queue_list += (i+1) + ": " + video_info.queue[i].title + "\n";
    }

    switch ( command ) {
        case "queued": {
            queued = video_info.queue.length;
            state = null;
            title = null;
            break;
        }

        case "playing": {
            queued = video_info.queue.length;
            title = video_info.title;
            state = "PLAYING";
            break;
        }

        case "paused":
            queued = video_info.queue.length;
            title = video_info.title;
            state = "PAUSED";
            paused = true;
            break;

        case "ended":
        case "closed":
            break;
    }

    if ( title != null )
        $("#video_status_name").html( title );

    if ( state != null )
        $("#video_status_state").html( state );

    $("#video_status_state").css( 'color', paused ? "red": "white" );

    $("#video_status_queued").html( queued == 0 ? "" : "(" + queued + " queued)" );

    $("#video_status_queued").attr( 'title', queue_list );
}

// ----------------------------------------------------------------------------
// see https://developer.spotify.com/web-api/authorization-guide/

function spotify_service_signon( event ) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/player/web_login/",
        cache: false,
        success: function (data) {
            var login_info = jQuery.parseJSON(data);

            var url = login_info.authorization_request_url
                .replace( "<REDIRECT>", "http%3A%2F%2Flocalhost%2Fdmxstudio%2Ffull%2Fspotify%2F" )
                .replace("<STATE>", session_id);

            window.open( url, "Spotify Login", "toolbar=no,status=no,menubar=no,width=600,height=800,location=no" );
        },

        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function ShowPalette( palette, weights, duration, time_index_ms /* -1 means entire video */ ) {
    var json = {
        "palette": palette,
        "weights": weights,
        "duration_ms": duration
    };

    $.ajax({
        type: "POST",
        url: "/dmxstudio/rest/control/venue/whiteout/palette",
        data: JSON.stringify( json ),
        contentType: 'application/json',
        cache: false,
        success: function () {
            var chip = $( "#video_palette_sample" );
            var html = makePaletteChip( chip, "video palette sample", palette, weights );
            chip.html( html );

            var time = $( "#video_palette_time" );
            time.html( ( time_index_ms >= 0 ) ? trackTime( time_index_ms, false ) : "" );

            $( "#video_palette_sample_div" ).show();
        },
        error: onAjaxError
    });
}