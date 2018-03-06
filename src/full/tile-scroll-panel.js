/* 
Copyright (C) 2012,2017 Robert DeSantis
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

// Event action type
var LayoutType = {
     SCROLL: 0,
     WRAP: 1,
     MAP: 2
};

// ----------------------------------------------------------------------------
// class ScrollPanel
//
function TileScrollPanel( panel_id, object_name, tooltip_name /*, layout_modes */ ) {
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
                return $("#" + this.panel_id + '_icon_' + id);
        }

        return null;
    }

    // method deleteTile
    this.deleteTile = function ( id ) {
        var tile = this.getTile(id);
        if (tile == null)
            return false;

        removeTileChipStrobe( id );

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

    // method fire_action
    this.fire_action = function( event, id, action_function ) {
        stopEventPropagation( event );

        var tile = $("#" + this.panel_id + '_icon_' + id );

        if ( tile.hasClass( 'dragging' ) ) {
            tile.removeClass( 'dragging' );
        }
        else {
            eval( action_function + '(null, ' + id + ');' );
        }
    }

    // method addTile
    this.addTile = function (id, number, title, allow_modify, allow_delete, grid_x, grid_y, tile_chip /*, object_name, tooltip_name, item_class */ ) {  
        var object_name = this.object_name;
        var tooltip_name = this.tooltip_name;
        var item_class = "tile_header";

        if (arguments.length == 11) {            // override object and tooltip names
            object_name = arguments[8];
            tooltip_name = arguments[9];
            item_class = arguments[10];
        }

        var content_div = $("#" + this.panel_id).find(".scroll-content");
        if (content_div == null) {
            messageBox("Assert: Cannot find #" + this.panel_id + " .scroll-content");
            return;
        }

        this.tiles[this.tiles.length] = id;

        var new_icon = '<div id="' + this.panel_id + '_icon_' + id + '" class="' + item_class + ' scroll-content-item ui-widget-content"' +
                        ' title="' + this.actions[this.ACTION_PLAY].action + " " + tooltip_name + '">';

        new_icon += '<div class="scroll-content-header"><span class="tile_number">' + number + "</span>";

        if ( tile_chip )
            new_icon += '<div class="tile_chip"></div>';

        for (var i=1; i < this.actions.length; i++) {   // Skip play icon
            if ( this.actions[i].tile_class == null || (!allow_modify && this.actions[i].tag_class == "edit_item") || (!allow_delete && this.actions[i].tag_class == "delete_item") )
                continue;

            new_icon += '<span class="tile_icon ' + this.actions[i].tile_class + " " + this.actions[i].tag_class +
                        ' icon_button" style="display:none; ' + this.actions[i].style + '" ' +
                        ' title="' + this.actions[i].action + " " + tooltip_name + '"></span>';
        }

        new_icon += '</div>';
        new_icon += '<div class="scroll-content-interior">' + title + '</div>';
        new_icon += '</div>';

        content_div.append(new_icon);

        var self = this;

        var tile = $("#" + this.panel_id + '_icon_' + id);

        tile.data( 'grid_x', grid_x );
        tile.data( 'grid_y', grid_y );
        tile.data( 'id', id );

        clickAction( tile, this, this.fire_action, id, this.actions[this.ACTION_PLAY].action + object_name );

        for (var i=1; i < this.actions.length; i++) {
            if ( this.actions[i].tile_class == null || (!allow_modify && this.actions[i].tag_class == "edit_item") || (!allow_delete && this.actions[i].tag_class == "delete_item") )
                continue;

            var icon = tile.find( "." + this.actions[i].tag_class );
            if ( icon.length == 1 )
                clickAction( icon, this, this.fire_action, id, this.actions[i].action + object_name );
        }

        tile.hover(
            function () { self.hover(id, true) },
            function () { self.hover(id, false) }
        );

        if ( this.tile_width == 0 ) {
            this.tile_width = tile.outerWidth(true);
            this.tile_height = tile.outerHeight(true);
        }

        if ( this.contentLayout == LayoutType.MAP )
            tile.css( "top", grid_y ).css( "left", grid_x ).css( "position", "absolute" );

        // Adjust tile positions - must be done after tile size is set
        this.adjustSize();

        return tile;
    }

    // method updateTile
    this.updateTile = function ( id, number, title, grid_x, grid_y ) {
        var tile = this.getTile( id );
        if ( tile == null )
            return false;

        tile.find( ".tile_number" ).html( number );
        tile.find( ".scroll-content-interior" ).html( title );

        tile.data( 'grid_x', grid_x );
        tile.data( 'grid_y', grid_y );

        if ( this.contentLayout == LayoutType.MAP )
            tile.css( "top", grid_y ).css( "left", grid_x );

        return true;
    }

    this.hover = function (id, is_in) {
        var tile = $("#" + this.panel_id + '_icon_' + id);

        var tile_chip = tile.find(".tile_chip");

        tile_chip.css('display', is_in ? 'none' : '');

        $.each(tile.find(".tile_icon"), function () {
            if (is_in) {
                if ($(this).hasClass("edit_item") || $(this).hasClass("new_item") || $(this).hasClass("delete_item")) {
                    if( edit_mode )
                        $(this).css('display', '');
                }
                else
                    $(this).css('display', '');
            }
            else
                $(this).css('display', 'none');

        });

        if (this.actions[this.ACTION_HOVER].used)
            eval('hover' + object_name + '(' + id + ',' + is_in + ')');
    }

    // method setTileChipColor
    this.setTileChipColor = function (id, color, strobe) {
        var tile = this.getTile(id);
        if (tile == null)
            return;

        var tile_chip = tile.find(".tile_chip");

        if ( !strobe ) {
            removeTileChipStrobe( id );
            tile_chip.css( 'background', color );
        }
        else {
            addTileChipStrobe( id, tile_chip, color );
            tile_chip.css( 'background', "#000000;" );
        }
    }

    // method selectTile
    this.selectTile = function (id, selected) {
        var tile = this.getTile(id);
        if (tile == null)
            return;

        var play_icon = tile.find(".play_item");

        if (!selected) {
            tile.removeClass("active_scroll_item");
            if ( play_icon != null )
                play_icon.removeClass(this.actions[0].selected_class).addClass(this.actions[0].tile_class);
        }
        else {
            tile.addClass("active_scroll_item");
            if (play_icon != null)
                play_icon.addClass(this.actions[0].selected_class).removeClass(this.actions[0].tile_class);
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

    // method highlightAllTile
    this.highlightAllTiles = function (highlight) {
        for (var i = 0; i < this.tiles.length; i++) {
            var tile = $("#" + this.panel_id + '_icon_' + this.tiles[i]);

            if (!highlight)
                tile.removeClass("highlight-scroll-item");
            else
                tile.addClass("highlight-scroll-item");
        }
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

        this.adjustSizeTimeout = setTimeout( function ( self ) {
            self.adjustSizeTimeout = null;
            
            var scrollPane = $("#" + self.panel_id);
            var content_div = scrollPane.find(".scroll-content");
            var scrollWrap = scrollPane.find(".scroll-bar-wrap");

            if (self.tiles.length == 0 || this.tile_height == 0 || this.tile_width == 0 ) {
                self.lock_icon.hide();
                scrollWrap.hide();
                content_div.css('width', '100%').css("margin-left", 0);
                return;
            }

            if ( self.getContentLayout() == LayoutType.MAP ) {
                var max_x = 0;
                var max_y = 0;
                var orphan = 0;

                if ( edit_mode )
                    self.lock_icon.show();

                for (var i = 0; i < self.tiles.length; i++) {
                    var tile = $("#" + self.panel_id + '_icon_' + self.tiles[i] );
                    tile.css( 'position', 'absolute' ).css( 'float', 'none' );

                    if ( !self.getTileLock() && edit_mode ) {
                        tile.draggable( { 
                            revert: "invalid",
                            containment: content_div,
                            grid: [ 1, 1 ],
                            start: function(event, ui) {
                                $(this).addClass('dragging');
                            },
                            zIndex: 100
                        } );
                        tile.data( 'drag_enabled', true );
                    }
                    else if ( tile.data( 'drag_enabled' ) ) {
                        tile.draggable( "destroy" );
                    }

                    var x = parseInt( tile.data('grid_x') );
                    var y = parseInt( tile.data('grid_y') );

                    // Position the orphans (those without coords) - sticky for the session
                    if ( x == 0 && y == 0 ) {
                        x = 1 + (orphan++ * self.tile_width);
                        y = 1;

                        tile.data('grid_y', y );
                        tile.data('grid_x', x );
                    }

                    if ( x > 0 || y > 0 ) {
                        max_x = Math.max( max_x, x );
                        max_y = Math.max( max_y, y );

                        tile.css( 'top', y + "px");
                        tile.css( 'left', x + "px");
                    }
                }

                var width = Math.max( max_x/self.tile_width+1, 18 ) * self.tile_width;
                var height = Math.max( max_y/self.tile_height+1, 4 ) * self.tile_height;

                content_div.css('width', width + "px" );
                content_div.css('height', height + "px" );

                if ( !self.getTileLock() && edit_mode ) {
                    content_div.droppable( { 
                        classes: { "ui-droppable": "highlight" },
                        "tolerance": "pointer",
                        drop: function( event, ui ) { 
                            ui.draggable.data( 'grid_x', ui.position.left ); 
                            ui.draggable.data( 'grid_y', ui.position.top );    
                        
                            eval( "move" + self.object_name + '(' + ui.draggable.data( 'id' ) + ',' + ui.position.left + ',' + ui.position.top + ')' );
                        
                            var scrollPane = $("#" + self.panel_id);
                            var content_div = scrollPane.find(".scroll-content");

                            if ( ui.position.top + self.tile_height + self.tile_height/3 > content_div.innerHeight() ) {
                                var height = Math.max( ui.position.top/self.tile_height+2, 4 ) * self.tile_height;
                                content_div.css('height', height + "px" );
                            }
                            if ( ui.position.left + self.tile_width + self.tile_width/3 > content_div.parent().innerWidth() ) {
                                var width = Math.max( ui.position.left/self.tile_width+2, 20 ) * self.tile_width;
                                content_div.css('width', width + "px" );
                            }
                        }
                    } );
                }

                scrollWrap.show();
                self.setupScroller();
            }
            else {
                content_div.css('height', '');
                self.lock_icon.hide();

                for (var i = 0; i < self.tiles.length; i++) {
                    var tile = $("#" + self.panel_id + '_icon_' + self.tiles[i]);
                    tile.css('position', 'static');
                    tile.css('float', 'left');
                }

                if (self.getContentLayout() == LayoutType.SCROLL) {
                    var width = self.tiles.length * self.tile_width;
                    content_div.css('width', width + 'px').css("margin-left", 0);
                    scrollWrap.show();
                    self.setupScroller();
                }
                else if ( self.getContentLayout() == LayoutType.WRAP ) {
                    scrollWrap.hide();
                    content_div.css('width', '100%').css("margin-left", 0);
                }
            }
        }, 100, this );
    }

    // method setupScroller
    this.setupScroller = function () {
        initializeScrollPane( $("#" + this.panel_id) );
    }

    // method setContentLayout
    this.setContentLayout = function ( layout ) {
        if ( typeof layout == "boolean" )
            layout = layout ? LayoutType.SCROLL : LayoutType.WRAP;

        this.contentLayout = layout;
        this.adjustSize();

        switch ( this.contentLayout ) {
            case LayoutType.SCROLL:
                this.scroll_or_grid_icon.addClass('ui-icon-arrowthickstop-1-n')
                                    .removeClass('ui-icon-arrowthickstop-1-s')
                                    .attr('title', 'wrap');
                break;

            case LayoutType.WRAP:
                if ( this.layout_modes.length == 3 ) {          // Grid mode is next
                    this.scroll_or_grid_icon.addClass('ui-icon-arrowthickstop-1-e')
                                    .removeClass('ui-icon-arrowthickstop-1-n')
                                    .attr('title', 'map');
                    break;
                }

                // fall through

            case LayoutType.MAP:
                this.scroll_or_grid_icon.addClass('ui-icon-arrowthickstop-1-s')
                                    .removeClass('ui-icon-arrowthickstop-1-n')
                                    .removeClass('ui-icon-arrowthickstop-1-e')
                                    .attr('title', 'scroll');
                break;
        }
    }

    // method getContentLayout
    this.getContentLayout = function () {
       return this.contentLayout;
    }

    // method setTileLock
    this.setTileLock = function ( locked ) {
       this.tile_lock = locked;
       this.adjustSize();

        if ( locked ) {
            this.lock_icon.addClass('ui-icon-locked')
                .removeClass('ui-icon-unlocked')
                .attr('title', 'unlock grid positions');
        }
        else {
            this.lock_icon.addClass('ui-icon-unlocked')
                .removeClass('ui-icon-locked')
                .attr('title', 'lock grid positions');
        }
    }

    // method isTileLock
    this.getTileLock = function ( ) {
        return this.tile_lock;
    }

    // Constructor
    this.panel_id = panel_id;
    this.object_name = object_name;
    this.tooltip_name = tooltip_name;
    this.layout_modes = (arguments.length == 3) ? [ LayoutType.SCROLL, LayoutType.WRAP ] : arguments[3];
    this.tile_lock = false;

    this.tile_width = 0;                        // Tile sizes will be set when a tile is constructed
    this.tile_height = 0;

    this.contentLayout = 1;                     // 1=scroll, 2=grid, 3=positioned
    this.tiles = []

    this.ACTION_PLAY = 0;
    this.ACTION_HOVER = 4;

    this.actions = [  // Action 0 is always the default and must have the selected_class
        { action: "play", tile_class: "ui-icon ui-icon-play", tag_class: "play_item", selected_class: "ui-icon-pause ui-icon-white", style: "float:right;", used: true },
        { action: "describe", tile_class: "ui-icon ui-icon-tag ", tag_class: "describe_item", style: "float:right;", used: true },
        { action: "delete", tile_class: "ui-icon ui-icon-trash ", tag_class: "delete_item", style: "float:right; display:none;", used: true },
        { action: "edit", tile_class: "ui-icon ui-icon-pencil ", tag_class: "edit_item", style: "float:right; display:none;", used: true },
        { action: "hover", tile_class: null, tag_class: null, style: "", used: false }
    ];

    var self = this;

    this.scroll_or_grid_icon = $("#" + this.panel_id).find(".scroll_or_grid_icon");
    this.scroll_or_grid_icon.addClass('ui-icon');
    this.scroll_or_grid_icon.click(function (event) {
        stopEventPropagation(event);
        self.setContentLayout( (self.getContentLayout() + 1) % self.layout_modes.length );
        client_config_update = true;
    });

    this.lock_icon = $("#" + this.panel_id).find(".tile_lock");
    this.lock_icon.hide();
    this.lock_icon.unbind('click').on( 'click', function ( event ) {
        stopEventPropagation( event );
        self.setTileLock( !self.getTileLock() );
        client_config_update = true;
    } );

    this.setContentLayout( LayoutType.SCROLL );
}

// Stobing color chip support functions
var strobeTimer = null;
var strobeChips = [];

function removeTileChipStrobe( id ) {
    for ( var i=0; i < strobeChips.length; i++ )
        if ( strobeChips[i].id == id ) {
            strobeChips.splice( i, 1 );
            break;
        }

    if ( strobeChips.length == 0 && strobeTimer != null ) {
        clearTimeout( strobeTimer );
        strobeTimer = null;
    }
}

function addTileChipStrobe( id, tile_chip, color ) {
    removeTileChipStrobe( id );

    strobeChips.push( { id: id, tile: tile_chip, color: color } );

    if ( strobeTimer == null )
        strobeTimer = setTimeout( strobe, 1000, true );
}

function strobe( on ) {
    for ( var i=0; i < strobeChips.length; i++ )
        strobeChips[i].tile.css( 'background', on ? strobeChips[i].color : "#000000" );

    strobeTimer = setTimeout( strobe, on ? 250 : 1000, !on );
}
