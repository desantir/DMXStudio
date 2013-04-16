/* 
Copyright (C) 2013 Robert DeSantis
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

;

var ui_expandableContainer_id = 0;

(function($, undefined){
    $.widget("ui.expandableContainer", {

        pluginName: "expandableContainer",

        options: {
            remove_template: '<span class="ui-icon ui-icon-circle-minus" title="remove" style="cursor: pointer;"></span>',
            add_template: '<span class="ui-icon ui-icon-circle-plus" title="insert" style="cursor: pointer;"></span>',
            empty_template: '<span class="ui-icon ui-icon-circle-plus" title="insert first item" style="cursor: pointer;"></span>',
            item_template: "<input class='container_input' type='text' size=5'>",
            new_item_callback: null,
            vertical: false,
            controls: "right"
        },

        // Widget constructor (access with this.element)
        _create: function() {
            this.container = $('<div class="mainContainer"></div>');
            this.container.appendTo(this.element).show();
        },

        _init: function () {
            this.options.container_class_name = "container_item_" + ++ui_expandableContainer_id;
            this.options.add_item_class_name = this.options.container_class_name + "_add_item";
            this.container.empty();
            this._checkEmpty();
        },

        _add: function (after) {
            return this._add2( after, $(this.options.item_template), null );
        },

        _add2: function (after, widget, value) {
            // Remove the itolated add item icon
            var item_adder = this.container.find("." + this.options.add_item_class_name);
            if (item_adder != null) {
                item_adder.remove();
            }

            var style = (this.options.vertical) ? "clear:both; float:left; margin-bottom: 4px;" : "float:left; margin-right: 2px;";

            var item = $('<div class="' + this.options.container_class_name + '" style="' + style + '"></div>');

            var remove = $(this.options.remove_template);
            var add = $(this.options.add_template);

            if (this.options.vertical) {    // Put the controls right or left for vertical
                remove.css({ float: 'left', 'margin-left': '4px', 'margin-top' : '4px' } );
                add.css({ float: 'left', 'margin-right': '4px', 'margin-top' : '4px' });
                widget.css({ float: 'left' });
            }
            else {                          // Put the controls under for horizontal
                remove.css( { clear: 'both', float: 'left', 'margin-right': '1px' } );
                add.css({ float: 'left' });
                widget.css({ clear: 'both', float: 'left' });
            }

            if (this.options.controls == "left") {
                remove.appendTo(item);
                add.appendTo(item);
                widget.appendTo(item);
            }
            else {
                widget.appendTo(item);
                remove.appendTo(item);
                add.appendTo(item);
            }

            var self = this;

            add.bind("click", function (event) {
                self._add(event.target.parentNode);
                return false;
            });

            remove.bind("click", function (event) {
                self._remove( $(event.target.parentNode) );
                return false;
            });

            if (after != null)
                item.insertAfter(after);
            else
                item.appendTo(this.container);

            if (value != null) {
                var input = item.find(".container_input");
                if (input != null && input.length > 0) {
                    input.get(0).value = value;
                    input.get(0).val = value;
                    if (input.option != null)
                        input.option("value", value);
                }
            }

            if (this.options.new_item_callback)
                this.options.new_item_callback(item,value);

            return item;
        },

        _remove: function ( item ) {
            item.remove();
            this._checkEmpty();
        },

        _checkEmpty: function () {
            if (this.container.children().length == 0) {
                var add = $( this.options.empty_template );
                add.addClass(this.options.add_item_class_name);
                add.css({ 'margin-right': '4px', 'margin-top': '4px' });
                add.appendTo(this.container);

                var self = this;
                add.bind("click", function (event) {
                    self._add(null);
                    return false;
                });
            }
        },

        // Add any arbitrary widget
        items: function () {
            if (arguments.length == 0) {            // return items
                var items = this.container.find("." + this.options.container_class_name );
                return items;
            }
            else {                                  // set item(s)
                if (jQuery.isArray(arguments[0])) {
                    var self = this;
                    $.map(arguments[0], function (widget) { self._add2(null, $(widget), null ) });
                }
                else
                    this._add2(null,$(arguments[0]), null);

                return null;
            }
        },

        values: function (value_fetcher) {
            var values = [];

            if (value_fetcher == null) {
                value_fetcher = function (item) {
                    var item = item.find(".container_input");
                    if (item != null) {
                        item = item.get(0);
                        if (item.value != null)
                            return item.value;
                        else if (item.val != null)
                            return item.val;
                        else if (item.option != null)
                            return item.option("value");
                        else if (item.getAttribute("value") != null)
                            return item.getAttribute("value");
                    }
                    return null;
                }
            }

            this.container.find("." + this.options.container_class_name).each(function () {
                values.push(value_fetcher($(this)));
            });

            return values;
        },

        set_values: function (values) {
            if (values == null)
                return;
    
            if (jQuery.isArray(values)) {
                var self = this;

                $.map(values, function (value) {
                    var item = self._add2(null, $(self.options.item_template), value);
                });
            }
            else {
                this._add2(null, $(this.options.item_template), values);
            }
        },

        _setOption: function (key, value) {
            $.Widget.prototype._setOption.apply(this, arguments);

            this.options[ key ] = value;
        }
    });

})(jQuery);