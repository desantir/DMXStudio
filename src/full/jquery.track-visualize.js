/* 
Copyright (C) 2014 Robert DeSantis
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

(function($, undefined){
    $.widget("ui.track_visualize_control", {

        pluginName: "track_visualize_control",

        options: {
            BAR_BORDER_SIZE: 1,
            BAR_BOX_WIDTH: 2,
            EDGE_BORDER: 2,

            onplay: null,                   // Fired during "play" when annotation encountered
            onclick: null,                  // Fired whern a bar is selected/unselected

            amplitude_data: [],
            interval_ms: 500,

            min_width: 800,
            bar_height_zoom: 1,

            bar_border_color: "rgb( 0, 0, 255 )",
            bar_unselected_color: "rgb( 0, 0, 150 )",
            bar_highlight_color: "rgb( 0, 111, 128)",
            bar_selected_color: "white",
            bar_hover_color: "rgb( 81, 81, 194 )",
            background_color: "transparent",
            annotation_line_color: "white",
            annotation_text_color: "white",

            no_data_message: "NO AMPLITUDE DATA"
        },

        // Widget constructor (access with this.element)
        _create: function () {
            this.play_duration_ms = 0;

            this.positionDiv = $( '<div style="display: none; position:absolute; left:10px; top:10px; font:24pt arial; background-color: transparent; color:' + this.options.bar_hover_color + '; z-index: 100;">foo</div>' );

            var canvas_tag = '<canvas id="track_visualize_canvas" style="overflow-x: auto;"></canvas>';
            this.canvas = $(canvas_tag);

            this.container = $( '<div style="overflow-x: auto; overflow-y: hidden; background-color: transparent; width:100%; height:100%; position:relative;"></div>' );
            this.canvas.appendTo(this.container);
            this.container.appendTo(this.element);
            this.positionDiv.appendTo( this.container );

            var self = this;
            this.canvas.on("mousedown", function (event) { self._on_mouse_down(event); });
            this.canvas.on("mousemove", function (event) { self._on_mouse_move(event); });
            this.canvas.on("mouseout", function (event) { self._on_mouse_out(event); });
        },

        _init: function () {
            this.load( this.options.amplitude_data, this.options.interval_ms );
        },

        load: function ( amplitude_data, interval_ms ) {
            this.options.amplitude_data = amplitude_data;
            this.options.interval_ms = interval_ms;

            this.bars = [];

            if (amplitude_data != null && amplitude_data.length > 0) {
                for (var i = 0; i < this.options.amplitude_data.length; i++) {
                    this.bars[this.bars.length] = {
                        "number": i,
                        "x": 0, "y": 0, "height": 0,
                        "highlight": false, "selected": false, hover: false, "annotation": null
                    };
                }
            }

            this.resize();
        },

        resize: function () {
            this.container.scrollLeft(0);
            this.max_bar_height = 0;
            this.options.height = this.element.height();

            if ( this.options.amplitude_data == null || this.options.amplitude_data.length == 0) {
                this.canvasWidth = Math.max( this.options.min_width, this.container.width() );
            }
            else {
                this.canvasWidth = Math.max( (this.options.amplitude_data.length *
                        (this.options.BAR_BOX_WIDTH + this.options.BAR_BORDER_SIZE)) + (this.options.EDGE_BORDER * 2), this.options.min_width);

                if ( this.canvasWidth >= this.element.width() )
                    this.options.height -= 20;

                var x = this.options.EDGE_BORDER;

                for (var i = 0; i < this.bars.length; i++) {
                    var height = ((this.options.amplitude_data[i] / 32767.0) * 100) * this.options.bar_height_zoom;
                    var y = this.options.height - height - this.options.BAR_BORDER_SIZE;

                    this.bars[i].x = x;
                    this.bars[i].y = y;
                    this.bars[i].height = height;

                    x += (this.options.BAR_BOX_WIDTH + this.options.BAR_BORDER_SIZE);         // Borders overlap - hence 1 border

                    if (height > this.max_bar_height + 4)
                        this.max_bar_height = height + 4;
                }
            }

            this.canvas.attr({ height: this.options.height, width: this.options.min_width });
            this.canvas.height(this.options.height);

            this.container.hide().show(0);
            this.draw();
        },

        draw: function () {
            this.canvas.attr({ width: this.canvasWidth });
            this.canvas.width(this.options.canvasWidth);

            var ctx = this.canvas.get(0).getContext("2d");

            ctx.fillStyle = this.options.background_color;
            ctx.fillRect(0, 0, this.canvasWidth, this.options.height);

            if (this.bars.length == 0) {
                ctx.font = '42pt Arial';
                ctx.textAlign = 'left';
                ctx.fillStyle = "grey";
                ctx.fillText(this.options.no_data_message, 10, this.options.height/2 + 20 );
            }
            else {
                // HACK! Paint bars multiple times to avoid hover changing the colors. Need
                // to figure out why this is happening as the effect is not desired.

                for ( var y=0; y < 3; y++ )
                    for (var i = 0; i < this.bars.length; i++)
                        this._paint_bar(ctx, this.bars[i], true);
            }
        },

        play: function ( start_ms ) {
            this.stop();
            this.clearHighlight();

            var start_bar = (start_ms == 0 ) ? 0 : Math.round( start_ms / this.options.interval_ms );

            if (start_bar > 0) {
                this.highlightLeft(start_bar, true);
                this.play_duration_ms = start_bar * this.options.interval_ms;
            }
            else
                this.play_duration_ms = 0;

            this._timer( start_bar );
         },

        _timer: function( index ) {
            if ( index >= this.bars.length )
                return;

            this.play_duration_ms += this.options.interval_ms;

            this.highlight( [ index ], true );

            var bar = this.bars[index];

            if (this.options.onplay != null) {
                if ( this.options.onplay(1, bar, this.play_duration_ms) )
                    return;
            }

            if (this.canvas.width() > this.container.width() &&
                bar.x >= this.container.width() + this.container.scrollLeft() - (this.options.BAR_BOX_WIDTH + this.options.BAR_BORDER_SIZE)) {
                this.container.scrollLeft( bar.x - this.container.width()/2 ) ; 
            }

            if ( ++index < this.bars.length ) {
                var self = this;
                this.auto_timer = setTimeout( function() {
                    self._timer( index );
                }, this.options.interval_ms );
            }
            else
                if (this.options.onplay != null)
                    this.options.onplay(2, null, this.play_duration_ms);
        },

        stop: function ( ) {
            if ( this.auto_timer != null ) {
                clearTimeout( this.auto_timer );
                this.auto_timer = null;
            }
        },

        getPlayDuration: function() {
            return this.play_duration_ms;
        },

        highlight: function( bar_list, highlight ) {
            if ( bar_list == null )
                return;

            var ctx = this.canvas.get(0).getContext("2d");

            for ( var index=0; index < bar_list.length; index++ ) {
                var bar_number = bar_list[index];
                if (bar_number < 0 || bar_number >= this.bars.length)
                    continue;

                var bar = this.bars[bar_number];
                if (bar.highlight == highlight)
                    continue;

                bar.highlight = highlight;
                this._paint_bar(ctx, bar, false);
            }
        },

        clearHighlight: function () {
            this.highlightLeft(this.bars.length, false);
        },

        highlightLeft: function (num_bars, highlight) {
            var ctx = this.canvas.get(0).getContext("2d");

            for (var index = 0; index < num_bars && index < this.bars.length; index++) {
                var bar = this.bars[index];
                if (bar.highlight == highlight)
                    continue;

                bar.highlight = highlight;
                this._paint_bar(ctx, bar, false);
            }
        },

        annotate: function (bar_number, label, data ) {
            if (bar_number < 0 || bar_number >= this.bars.length)
                return;

            var ctx = this.canvas.get(0).getContext("2d");

            var bar = this.bars[bar_number];
            bar.annotation = { "label": label, "data": data };
            this.draw();
        },

        clearAnnotation: function ( bar_number ) {
            if (bar_number < 0 || bar_number >= this.bars.length)
                return;

            var ctx = this.canvas.get(0).getContext("2d");

            var bar = this.bars[bar_number];
            bar.annotation = null;
            this.draw();
        },

        _paint_bar: function( ctx, bar, drawAnnotation ) {
            ctx.beginPath();
            ctx.lineWidth = this.options.BAR_BORDER_SIZE;
            ctx.strokeStyle = this.options.bar_border_color;
            ctx.rect(bar.x, bar.y, this.options.BAR_BOX_WIDTH, bar.height);
            ctx.stroke();

            if (bar.selected)
                ctx.fillStyle = this.options.bar_selected_color;
            else if ( bar.hover)
                ctx.fillStyle = this.options.bar_hover_color;
            else if (bar.highlight)
                ctx.fillStyle = this.options.bar_highlight_color;
            else
                ctx.fillStyle = this.options.bar_unselected_color;

            ctx.fill();
            ctx.closePath();

            if (drawAnnotation && bar.annotation != null && bar.annotation.label != null) {

                var bar_width = this.options.BAR_BOX_WIDTH + this.options.BAR_BORDER_SIZE;

                ctx.save();
                ctx.translate(0, 0);
                ctx.rotate( 270 * Math.PI / 180);
                ctx.font = '7pt Arial';
                ctx.textAlign = 'left';
                ctx.fillStyle = this.options.annotation_text_color;
                ctx.fillText(bar.annotation.label, -(this.options.height - this.max_bar_height - 4), bar.x + bar_width/2 - 2);
                ctx.restore();

                var text_height = ctx.measureText(bar.annotation.label).width;
                var text_width = ctx.measureText('M').width;   // Just an approximation

                bar.annotation.textbox = {
                    "top": (this.options.height-this.max_bar_height) - text_height - 4, 
                    "left": bar.x+bar_width/2-2-text_width, 
                    "height": text_height, 
                    "width": text_width+2 };  // +2 for the line

                ctx.lineWidth = 1;
                ctx.strokeStyle = this.options.annotation_line_color;
                ctx.beginPath();
                ctx.moveTo(bar.x + bar_width/2, bar.y);
                ctx.lineTo(bar.x + bar_width/2, (this.options.height-this.max_bar_height) - text_height );
                ctx.stroke();
                ctx.closePath();
            }
        },

        getBarData: function() {
            return this.bars;
        },

        unselectAll: function () {
            var ctx = this.canvas.get(0).getContext("2d");

            for (var i = 0; i < this.bars.length; i++) {
                if (this.bars[i].selected) {
                    this.bars[i].selected = false;
                    this._paint_bar(ctx, this.bars[i], false)
                }
            }
        },

        _cancel_event : function( event ) {
            event = (event || window.event);

            if ( event != null ) {
                if (event.stopPropagation != null)
                    event.stopPropagation();
                else
                    event.cancelBubble = true;
            }
        },

        _get_bar: function( event ) {
            var canvas_x = $(window).scrollLeft() + event.clientX - Math.round(this.canvas.offset().left);
            var canvas_y = $(window).scrollTop() + event.clientY - Math.round(this.canvas.offset().top);

            if (canvas_y < this.options.height - this.max_bar_height - this.options.BAR_BORDER_SIZE) {
                // See if the hit is any a bar's annotation box
                for (var i = 0; i < this.bars.length; i++) {
                    var bar = this.bars[i];
                    if ( bar.annotation == null || bar.annotation.textbox == null )
                        continue;

                    var box = bar.annotation.textbox;
                    if ( canvas_x >= box.left && canvas_x < box.left+box.width && 
                         canvas_y >= box.top && canvas_y < box.top+box.height )
                        return bar;
                }

                return null;
            }

            var bar_number = Math.floor((canvas_x - this.options.EDGE_BORDER) / (this.options.BAR_BOX_WIDTH + this.options.BAR_BORDER_SIZE));

            if (bar_number < 0 || bar_number >= this.bars.length)
                return null;

            return this.bars[bar_number];
        },

        _on_mouse_down: function (event) {
            this._cancel_event(event);

            var bar = this._get_bar(event);
            if (bar == null) {
                this.unselectAll();
            }
            else {
                var selected = !bar.selected;

                this.unselectAll();

                if (selected) {
                    var ctx = this.canvas.get(0).getContext("2d");
                    bar.selected = true;
                    this._paint_bar(ctx, bar, false);
                }
            }

            if (this.options.onclick != null)
                this.options.onclick( event, bar );
        },

        _on_mouse_move: function (event) {
            this._cancel_event(event);

            var bar = this._get_bar(event);
            if (bar == null) {
                this._on_mouse_out(null);
                return;
            }

            var ctx = this.canvas.get(0).getContext("2d");

            for (var i = 0; i < this.bars.length; i++) {
                var hover = i <= bar.number;

                if (this.bars[i].hover != hover) {
                    this.bars[i].hover = hover;
                    this._paint_bar(ctx, this.bars[i], false);
                }
            }

            if ( this.positionTimer != null )
                clearTimeout( this.positionTimer );

            var self = this;
            this.positionTimer = setTimeout( function () {
                self._update_position( bar.number )
            }, 1 );
        },

        _update_position: function( bar_number ) {
            this.positionDiv.css( 'left', this.container.scrollLeft() + this.options.EDGE_BORDER );
            this.positionDiv.text( this._tracktime( bar_number * this.options.interval_ms ) );
            this.positionDiv.show();
        },

        _on_mouse_out: function( event ) {
            this._cancel_event(event);

            var ctx = this.canvas.get(0).getContext("2d");

            for (var i = 0; i < this.bars.length; i++) {
                if (this.bars[i].hover) {
                    this.bars[i].hover = false;
                    this._paint_bar(ctx, this.bars[i], false)
                }
            }

            if ( this.positionTimer != null )
                clearTimeout( this.positionTimer );

            this.positionDiv.hide();
        },

        _tracktime: function( time ) {
            var track_time = "";

            var minutes = Math.floor(time / (60 * 1000));
            time -= minutes * (60 * 1000);
            var seconds = Math.floor(time / (1000));
            time -= seconds * 1000;
            var frac = time / 100;

            track_time = minutes + ":";
            if (seconds < 10)
                track_time += "0"

            track_time += seconds;
            track_time += "." + frac;

            return track_time;
        },

        _setOption: function (key, value) {
            this.options[key] = value;
            $.Widget.prototype._setOption.apply(this, arguments);
            this.resize();
        }
    });

})(jQuery);