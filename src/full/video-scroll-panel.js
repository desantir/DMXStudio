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
// class VideoScrollPanel
//
function VideoScrollPanel( panel_id ) {
        // Constructor
    this.panel_id = panel_id;
    this.videos = []
    this.videos_index = [];
    this.video_container_id = 0;
    this.video_template = video_template.innerHTML;

    var scroll_or_grid_icon = $("#" + this.panel_id).find(".scroll_or_grid_icon");
    if (scroll_or_grid_icon != null) {
        var self = this;
        scroll_or_grid_icon.addClass('ui-icon');
        scroll_or_grid_icon.click(function (event) {
            stopEventPropagation(event);
            self.setScrollContent(!self.isScrollContent());
            client_config_update = true;
        });
    }

    this.setScrollContent(true);
}

// method getPanelId
VideoScrollPanel.prototype.getPanelId = function () {
    return this.panel_id;
}

// method getVideoContainer
VideoScrollPanel.prototype.getVideoContainer = function (video_id) {
    var video = this.getVideo( video_id );

    return video == null ? null : $("#" + video.container_name );
}

// method getVideo
VideoScrollPanel.prototype.getVideo = function (video_id) {
    if ( this.videos_index.hasOwnProperty( video_id ) )
        return this.videos[ this.videos_index[video_id] ];
    return null;
}

// method hasVideo
VideoScrollPanel.prototype.hasVideo = function (video_id) {
    return this.videos_index.hasOwnProperty( video_id );
}

// method deleteVideoById
VideoScrollPanel.prototype.deleteVideoById = function ( video_id ) {
    var tile = this.getVideoContainer( video_id );
    if (tile == null)
        return false;

    tile.remove();

    var index = this.videos_index[ video_id ];
    delete this.videos_index[ video_id ];
    this.videos.splice(index, 1);

    this.adjustScrollbar();

    return true;
}

// method addVideo
VideoScrollPanel.prototype.addVideo = function( video ) {  
    var content_div = $("#" + this.panel_id).find(".scroll-content");
    if (content_div == null) {
        messageBox("Assert: Cannot find #" + this.panel_id + " .scroll-content");
        return null;
    }

    var container_id = this.video_container_id++;
    var video_id = video.video_id;
    var default_palette = video.default_palette;

    var video_div = $( this.video_template.replace( /NNN/g, ""+ container_id ) );
    video_div.data( 'video_id', video_id );

    var video_title_div = video_div.find( ".video_title" );
    video_title_div.html( video.title );

    video_div.find( ".video_image" )[0].src = video.thumbnails.medium.url;
    video_div.find( ".video_image" ).attr( "title", video.description );

    if ( video.duration != null && video.duration.length > 0 ) {
        var time_div = video_div.find( ".video_time" );
        time_div.show();
        time_div.text( convertISO8601( video.duration ) );
    }

    // Add in the palette if we have one
    this._addPalette( video_div, default_palette );

    content_div.append(video_div);
        
    video.container_name = this.panel_id + '_video_' + container_id;

    this.videos[this.videos.length] = video;
    this.videos_index[video_id] = this.videos.length-1;

    this.adjustSize();

    return video_div;
}

// method updateVideoPalette
VideoScrollPanel.prototype.updateVideoPalette = function( video_id, default_palette, video_palette ) {
    var video = this.getVideo( video_id );
    if ( video == null )
        return;

    var video_div = this.getVideoContainer( video_id );
    if ( video_div == null )
        return;

    video.default_palette = default_palette;
    video.video_palette = video_palette;

    video.palette_time_indexes = [];
    for ( var i=0; i < video_palette.length; i++ )
        video.palette_time_indexes.push( video_palette[i].time_ms );

    var icon = video_div.find( ".video_play_with_palette" );
    icon.stop( true, false );
    icon.css('rotation', 0);      // Reset rotation

    if ( video_palette.length > 0 )
        icon.removeClass( "md-icon-white" ).addClass( "md-icon-blue" );
    else
        icon.removeClass( "md-icon-blue" ).addClass( "md-icon-white" );

    this._addPalette( video_div, default_palette );
}

// method _addPalette
VideoScrollPanel.prototype._addPalette = function( video_div, palette ) {
    var palette_html = "";

    for ( var p=0; p < palette.length; p++ )
        palette_html += '<div class="video_palette_chip" style="background-color: #' + palette[p] + ';"></div>'

    video_div.find( ".video_palette" ).html( palette_html );
    video_div.find( ".video_play_with_palette" ).show();
}

// method empty
VideoScrollPanel.prototype.empty = function () {
    var content_div = $("#" + this.panel_id).find(".scroll-content");
    if (content_div == null) {
        messageBox("Assert: Cannot find #" + this.panel_id + " .scroll-content");
        return;
    }

    content_div.empty();
    this.videos_index = [];
    this.videos = [];
    this.video_container_id = 0;

    this.adjustSize();
}

// method iterate
VideoScrollPanel.prototype.iterate = function (callable) {
    for (var i = 0; i < this.videos.length; i++)
        callable( this.videos[i] );
}

// method adjustSize
VideoScrollPanel.prototype.adjustSize = function () {
    if (this.adjustSizeTimeout != null)
        clearTimeout(this.adjustSizeTimeout);

    var self = this;

    this.adjustSizeTimeout = setTimeout(function () {
        var scrollPane = $("#" + self.panel_id);
        var content_div = scrollPane.find(".scroll-content");
        var scrollWrap = scrollPane.find(".scroll-bar-wrap");

        var tile_width = 0;
        var tile_height = 0;

        if (self.videos.length > 0) {
            tile_width = self.getVideoContainer(self.videos[0].video_id).outerWidth(true);
            tile_height = self.getVideoContainer(self.videos[0].video_id).outerHeight(true);
        }

        if (self.scrollContent) {
            var width = self.videos.length * tile_width;
            content_div.css('width', width + 'px');
            scrollWrap.show();
            self.setupScroller();
        }
        else {
            scrollWrap.hide();
            content_div.css('width', '100%').css( 'margin-left', 0);
        }

    }, 1 );
}

// method setupScroller
VideoScrollPanel.prototype.setupScroller = function () {
    initializeScrollPane( $("#" + this.panel_id) );
}

// method setScrollContent
VideoScrollPanel.prototype.setScrollContent = function (scroll) {
    this.scrollContent = scroll;
    this.adjustSize();

    var scroll_or_grid_icon = $("#" + this.panel_id).find(".scroll_or_grid_icon");
    if (scroll_or_grid_icon != null) {
        if (!scroll)
            scroll_or_grid_icon.removeClass('ui-icon-arrowthickstop-1-s')
                                .addClass('ui-icon-arrowthickstop-1-n')
                                .attr('title', 'scroll');
        else
            scroll_or_grid_icon.removeClass('ui-icon-arrowthickstop-1-n')
                                .addClass('ui-icon-arrowthickstop-1-s')
                                .attr('title', 'grid');
    }
}

// method setScrollContent
VideoScrollPanel.prototype.isScrollContent = function (scroll) {
    return this.scrollContent;
}

