/* 
Copyright (C) 2016 Robert DeSantis
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

var select_list_template =  '<div style="margin: 0px 0px 10px 0px;">' +
                            '      <select class="select_listbox"></select>' +
                            '<hr class="select-list-hr" />' +
                            '</div>' +
                            '<div class="selected_container" style="margin: 8px 0px 10px 0px; overflow-y: auto;">' +
                            '</div>';

// ----------------------------------------------------------------------------
// class SelectList
//
function SelectList( options ) {

    // method getSelected
    this.getSelected = function() {
        return this.selected;
    }

    // method getSelectedIds
    this.getSelectedIds = function() {
        var ids = []
        for ( var i=0; i < this.selected.length; i++ )
            ids.push( this.selected[i].id );
        return ids;
    }

    // method setSelected
    this.setSelected = function( selected ) {
        this.selected = selected;

        this.populate();
    }

    // method populate
    this.populate = function() {
        // Rebuild source select list
        var selectable_items = this.source_callback( this );

        this.select_listbox.empty();

        for (var i = 0; i < selectable_items.length; i++) {
            var item = selectable_items[i];

            this.select_listbox.append($('<option>', {
                value: item.id,
                text: item.label,
                selected: false
            }));
        }

        var self = this;

        this.select_listbox.css( "max-width", this.width );
        this.select_listbox.multiselect( { minWidth: this.width, multiple: false,  selectedList: 0, noneSelectedText: 'add ' + this.name,
            header: false, classes: 'select_list', selectedText: 'add ' + this.name, height: this.height });

        this.select_listbox.unbind("multiselectclick").bind("multiselectclick", function ( event, ui ) {
            stopEventPropagation(event);

            var item_id = parseInt(ui.value);

            for (var i=0; i < self.selected.length; i++)
                if ( self.selected[i] == item_id )
                    return;

            self.selected[ self.selected.length ] = self.add_callback( self, item_id );

            self.populate();

            if ( self.update_callback )
                self.update_callback( self );
        });

        // Rebuild list tiles
        this.selected_container.empty();

        for (var index=0; index < this.selected.length; index++) {
            var item_element = $( this.render_template.replace(/NNN/g, index) );

            this.selected_container.append( item_element );

            var remove_button = item_element.find( ".select_remove" );
            if ( remove_button != null && remove_button.length > 0 ) {
                remove_button.data( "select_index", index );            
                remove_button.on( 'click', function ( event ) { stopEventPropagation(event); self._remove( $(this).data("select_index") ); } );
            }

            var select_button = item_element.is(".select_action") ? item_element : item_element.find( ".select_action" );
            if ( select_button != null && select_button.length > 0) {
                select_button.data( "select_index", index );   
                select_button.on( 'click', function ( event ) { stopEventPropagation(event); self._select( $(this).data( "select_index" ) ); } );
            }

            var select_label = item_element.find( ".select_label" );
            if ( select_label != null )
                select_label.html( this.selected[index].label );

            if ( this.render_callback != null )
                this.render_callback( this, index, this.selected[index], item_element );
        }
    }

    // method _remove (internal)
    this._remove = function( index ) {
        this.selected.splice(index, 1);

        this.populate();

        if ( this.remove_callback != null )
            this.remove_callback( this );
    }

    // method _select (internal)
    this._select = function( index ) {
        if ( this.select_callback == null )
            return;

        this.select_callback( this, index, this.selected[index] );  
    }

    // Constructor
    this.container = options.container;
    this.name = options.name;
    this.selected = options.selected;
    this.source_callback = options.source_callback;
    this.add_callback = options.add_callback;
    this.render_template = options.render_template;
    this.render_callback = options.render_callback;
    this.select_callback = options.select_callback;
    this.remove_callback = options.remove_callback;
    this.update_callback = options.update_callback;
    this.width = options.width;
    this.height = options.hasOwnProperty("height") ? options.height : 400;
    this.list_height = options.hasOwnProperty("list_height") ? options.list_height : null;

    this.container.data( "select-list", this );

    this.container.empty();
    this.container.html( select_list_template );

    this.container.find( ".select-list-hr").css( "width", (this.width+5) + "px" );

    this.select_listbox = this.container.find( ".select_listbox" );
    this.selected_container = this.container.find( ".selected_container" );

    if ( this.list_height != null ) 
        this.selected_container.css( "height", this.list_height + "px" );
    this.populate();
}



