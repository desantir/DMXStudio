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

// ----------------------------------------------------------------------------
// class ScrollPanel
//
function TileScrollPanel( panel_id, object_name, tooltip_name ) {
    // method getPanelId
    this.getPanelId = function () {
        return this.panel_id;
    }

    // method getTile
    this.getTile = function (id) {
        // Interestly enough, when the tile container div is emptied, it seems we can still find the
        // tile in the DOM.  Not sure what or who's bug this is at the moment, but verify ID is in
        // our list before DOM lookup.

        for (var i = 0; i < this.tiles.length; i++) {
            if (this.tiles[i] == id)
                return $("#" + this.panel_id + '_' + '_icon_' + id);
        }

        return null;
    }

    // method deleteTile
    this.deleteTile = function ( id ) {
        var tile = this.getTile(id);
        if (tile == null)
            return false;

        tile.remove();

        // Remove tile ID
        for (var i = 0; i < this.tiles.length; i++) {
            if (this.tiles[i] == id) {
                this.tiles.splice(i, 1);
                break;
            }
        }

        this.adjustScrollbar();

        return true;
    }

    // method make_click_event
    this.make_click_event = function (action, id, object_name, tooltip_name) {
        if (!this.actions[action].used)
            return "";

        return 'onclick="' + this.actions[action].action + object_name + '(event, ' + id + ');" title="' +
               this.actions[action].action + " " + tooltip_name + '"';
    }

    // method addTile
    this.addTile = function (id, number, title, allow_modify /*, object_name, tooltip_name */ ) {  
        object_name = this.object_name;
        tooltip_name = this.tooltip_name;

        if (arguments.length == 6) {            // override object and tooltip names
            object_name = arguments[4];
            tooltip_name = arguments[5];
        }

        var content_div = $("#" + this.panel_id).find(".scroll-content");
        if (content_div == null) {
            messageBox("Assert: Cannot find #" + this.panel_id + " .scroll-content");
            return;
        }

        this.tiles[this.tiles.length] = id;

        var new_icon = '<div id="' + this.panel_id + '_' + '_icon_' + id + '" class="scroll-content-item ui-widget-header" tile_id="' + id + '" ' +
                        this.make_click_event(0, id, object_name, tooltip_name) + '>';

        new_icon += '<div class="scroll-content-header">' + number;

        for (var i = 0; i < this.actions.length; i++) {
            if (!allow_modify && (this.actions[i].tag_class == "edit_item" || this.actions[i].tag_class == "delete_item"))
                continue;

            if (this.actions[i].used) {
                new_icon += '<span class="' + this.actions[i].tile_class + " " + this.actions[i].tag_class +
                            '" style="' + this.actions[i].style + '" ' +
                            this.make_click_event(i, id, object_name, tooltip_name) + '></span>';
            }
        }

        new_icon += '</div>';
        new_icon += '<div class="scroll-content-interior">' + title + '</div>';
        new_icon += '</div>';

        content_div.append(new_icon);

        this.adjustSize();

        return $("#" + this.panel_id + '_' + '_icon_' + id);
    }

    // method selectTile
    this.selectTile = function (id, selected) {
        var tile = this.getTile(id);
        if (tile == null)
            return;

        if (!selected) {
            tile.removeClass("active_scroll_item");
            tile.find(".play_item").removeClass(this.actions[0].selected_class).addClass(this.actions[0].tile_class);
        }
        else {
            tile.addClass("active_scroll_item");
            tile.find(".play_item").addClass(this.actions[0].selected_class).removeClass(this.actions[0].tile_class);
        }
    }

    // method isActive
    this.isActive = function (id) {
        var tile = this.getTile(id);
        if (tile == null)
            return false;
        return tile.hasClass("active_scroll_item");
    }

    // method highlightTile
    this.highlightTile = function (id, highlight) {
        var tile = this.getTile(id);
        if (tile == null)
            return;

        if (!highlight)
            tile.removeClass("highlight-scroll-item");
        else
            tile.addClass("highlight-scroll-item");
    }

    // method empty
    this.empty = function () {
        var content_div = $("#" + this.panel_id).find(".scroll-content");
        if (content_div == null) {
            messageBox("Assert: Cannot find #" + this.panel_id + " .scroll-content");
            return;
        }

        content_div.empty();
        this.tiles.length = 0;

        this.adjustSize();
    }

    // method iterate
    this.iterate = function (callable) {
        for (var i = 0; i < this.tiles.length; i++)
            callable(this.tiles[i]);
    }

    // method anyActive
    this.anyActive = function () {
        for (var i = 0; i < this.tiles.length; i++)
            if ( this.isActive( this.tiles[i] ) )
                return true;
        return false;
    }

    // method adjustSize
    this.adjustSize = function () {
        if (this.adjustSizeTimeout != null)
            clearTimeout(this.adjustSizeTimeout);

        var self = this;

        this.adjustSizeTimeout = setTimeout(function () {
            var scrollPane = $("#" + self.panel_id);
            var content_div = scrollPane.find(".scroll-content");
            var scrollWrap = scrollPane.find(".scroll-bar-wrap");

            var tile_width = 0;
            var tile_height = 0;

            if (self.tiles.length > 0) {
                tile_width = self.getTile(self.tiles[0]).outerWidth(true);
                tile_height = self.getTile(self.tiles[0]).outerWidth(true);
            }

            if (self.scrollContent) {
                var width = self.tiles.length * tile_width;
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
    this.setupScroller = function () {
        initializeScrollPane( $("#" + this.panel_id) );
    }

    // method setScrollContent
    this.setScrollContent = function (scroll) {
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
    this.isScrollContent = function (scroll) {
       return this.scrollContent;
    }

    // Constructor
    this.panel_id = panel_id;
    this.object_name = object_name;
    this.tooltip_name = tooltip_name;
    this.tiles = new Array();

    this.actions = [  // Action 0 is always the default and must have the selected_class
        { action: "play", tile_class: "ui-icon ui-icon-play", tag_class: "play_item", selected_class: "ui-icon-pause ui-icon-white", style: "float:right;", used: true },
        { action: "describe", tile_class: "ui-icon ui-icon-tag ", tag_class: "describe_item", style: "float:right;", used: true },
        { action: "delete", tile_class: "ui-icon ui-icon-trash ", tag_class: "delete_item", style: "float:right; display:none;", used: true },
        { action: "edit", tile_class: "ui-icon ui-icon-pencil ", tag_class: "edit_item", style: "float:right; display:none;", used: true }
    ];

    var scroll_or_grid_icon = $("#" + this.panel_id).find(".scroll_or_grid_icon");
    if (scroll_or_grid_icon != null) {
        var self = this;
        scroll_or_grid_icon.addClass('ui-icon');
        scroll_or_grid_icon.click(function (event) {
            stopEventPropagation(event);
            self.setScrollContent(!self.isScrollContent());
        });
    }

    this.setScrollContent(true);
}

// ----------------------------------------------------------------------------
//
function initializeScrollPane( scrollPane ) {
    var scrollbar = scrollPane.find(".scroll-bar");
    var scrollContent = scrollPane.find(".scroll-content");
    var scrollWrap = scrollPane.find(".scroll-bar-wrap");

    //change overflow to hidden now that slider handles the scrolling
    scrollPane.css("overflow", "hidden");

    function isScrollable() {
        return (scrollContent.width() > scrollPane.width());
    }

    //build slider
    scrollbar.slider({
        min: 0,
        max: 100,
        slide: function (event, ui) {
            if ( isScrollable() ) {
                scrollContent.css("margin-left", Math.round(ui.value / 100 * (scrollPane.width() - scrollContent.width())) + "px");
            } else {
                scrollContent.css("margin-left", 0);
            }
        }
    });

    // Because the scroll bar is sized and centered in the scroll wrapper, this leaves empty zones before and after
    // the scroll bar.  This handler on the wrapper will move the scroll bar to the appropriate min and max.
    scrollWrap.mousedown( function (event) {
        stopEventPropagation(event);

        if ( isScrollable() ) {
            if (event.pageX < scrollbar.offset().left) {
                scrollbar.slider("value", 0);
                scrollContent.css("margin-left", 0);
            }
            else if (event.pageX > scrollbar.offset().left + scrollbar.width()) {
                scrollbar.slider("value", 100);
                scrollContent.css("margin-left", Math.round((scrollPane.width() - scrollContent.width())) + "px");
            }
        }
    });

    //append icon to handle
    var handleHelper = scrollbar.find(".ui-slider-handle").parent();
    scrollbar.find(".ui-slider-handle").html("<span class='ui-icon ui-icon-grip-dotted-vertical'></span>");

    //size scrollbar and handle proportionally to scroll distance
    function sizeScrollbar() {
        if (isScrollable()) {
            scrollWrap.show();

            scrollContent.css("margin-left", Math.round(scrollbar.slider("value") / 100 * (scrollPane.width() - scrollContent.width())) + "px");

            var remainder = scrollContent.width() - scrollPane.width();
            var proportion = remainder / scrollContent.width();
            var handleSize = scrollPane.width() - (proportion * scrollPane.width());

            scrollbar.find(".ui-slider-handle").css({ width: handleSize, "margin-left": -handleSize / 2 });
            handleHelper.width("").width(scrollbar.width() - handleSize);
        }
        else {
            scrollWrap.hide();
            scrollContent.css("margin-left", 0);
        }
    }

    //reset slider value based on scroll content position        
    function resetValue() {
        var remainder = scrollPane.width() - scrollContent.width();
        var leftVal = scrollContent.css("margin-left") === "auto" ? 0 : parseInt(scrollContent.css("margin-left"));
        var percentage = Math.round(leftVal / remainder * 100);
        scrollbar.slider("value", percentage);
    }

    //if the slider is 100% and window gets larger, reveal content        
    function reflowContent() {
        if (isScrollable()) {
            var showing = scrollContent.width() + parseInt(scrollContent.css("margin-left"), 10);
            var gap = scrollPane.width() - showing;
            if (gap > 0) {
                scrollContent.css("margin-left", parseInt(scrollContent.css("margin-left"), 10) + gap);
            }
        }
    }

    //change handle position on window resize        
    $(window).resize(function () {
        resetValue(); sizeScrollbar(); reflowContent();
    });

    //init scrollbar size        
    setTimeout(sizeScrollbar, 10);//safari wants a timeout
}
