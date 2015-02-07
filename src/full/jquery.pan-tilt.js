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
    $.widget("ui.pan_tilt_control", {

        pluginName: "pan_tilt_control",

        options: {
            pan_start:0,
            pan_end: 360,
            tilt_start: 0,
            tilt_end: 360,
            pan: 180,
            tilt: 180,
            onchange: null,
            on_release_only: false,
            width: 150,
            background_color: "#FFFFFF",
            pan_color: "#808080",
            tilt_color: "#404040",
            text_color: "black",
            marker_color: "red",
            pan_factor: 2,
            tilt_factor: 2
        },

        // Widget constructor (access with this.element)
        _create: function () {
            this.canvas = $('<canvas id="pan_tilt_canvas"></canvas>');
            this.canvas.width(this.options.width);
            this.canvas.appendTo(this.element);
            var self = this;
            this.canvas.on("mousedown", function (event) { self._on_mouse_down(event); });
            this.canvas.on("mousewheel", function (event) { self._on_mouse_wheel(event); });
        },

        _init: function () {
            var height = this.canvas.get(0).height;

            this.radius_outer = height / 2 - 10;
            this.radius_space = this.radius_outer * .69;
            this.radius_inner = this.radius_outer * .58;

            this.cx = height / 2;
            this.cy = height / 2;

            this._draw();
        },

        _draw: function () {
            var ctx = this.canvas.get(0).getContext("2d");

            ctx.beginPath();
            ctx.fillStyle = this.options.background_color;
            ctx.rect(0, 0, this.options.width, this.options.height);
            ctx.fill();
            ctx.closePath();

            ctx.strokeStyle = this.options.background_color;
            ctx.lineWidth = 1;

            var panRads = this.options.pan / 180 * Math.PI;

            if (this.options.pan > 0) {
                ctx.beginPath();
                ctx.fillStyle = this.options.pan_color;
                ctx.moveTo(this.cx, this.cy);
                ctx.arc(this.cx, this.cy, this.radius_outer, 0, panRads, false);
                ctx.fill();
                ctx.closePath();
            }

            if (panRads < 2 * Math.PI) {
                ctx.beginPath();
                ctx.strokeStyle = this.options.background_color;
                ctx.fillStyle = this.options.background_color;
                ctx.moveTo(this.cx, this.cy);
                ctx.arc(this.cx, this.cy, this.radius_outer, panRads, 2 * Math.PI, false);
                ctx.fill();
                ctx.stroke();
                ctx.closePath();
            }

            ctx.beginPath();
            ctx.strokeStyle = this.options.marker_color;
            ctx.moveTo(this.cx, this.cy);
            ctx.arc(this.cx, this.cy, this.radius_outer, panRads, panRads, false);
            ctx.stroke();
            ctx.closePath();

            var y;
            
            if ( this.options.pan % 360 > 45 && this.options.pan % 360 < 135 )
                y = this.cy - this.radius_space - 6;
            else
                y = this.cy + this.radius_space + 12;

            ctx.font = '6pt Arial';
            ctx.textAlign = 'center';
            ctx.fillStyle = this.options.text_color;
            ctx.fillText("Pan " + this.options.pan + "°", this.cx, y );

            this._drawTilt();
        },

        _drawTilt: function() {
            var ctx = this.canvas.get(0).getContext("2d");

            // Fill space circle
            ctx.beginPath();
            ctx.strokeStyle = this.options.background_color
            ctx.fillStyle = this.options.background_color;
            ctx.moveTo(this.cx, this.cy);
            ctx.arc(this.cx, this.cy, this.radius_space, 0, Math.PI * 2, false);
            ctx.fill();
            ctx.closePath();

            ctx.beginPath();
            ctx.strokeStyle = "#000000";
            ctx.lineWidth = 2;
            ctx.fillStyle = this.options.tilt_color;
            ctx.moveTo(this.cx, this.cy);
            ctx.arc(this.cx, this.cy, this.radius_inner, 0, 2 * Math.PI, false);
            ctx.stroke();
            ctx.fill();
            ctx.closePath();

            ctx.beginPath();
            ctx.lineWidth = 1;

            // Tilt lines from 90 degrees to 270 degrees

            var tilt_radians = (90.0 + (this.options.tilt/360.0) * 180) / 180.0 * Math.PI;

            var x1 = this.radius_inner * Math.cos(tilt_radians) + this.cx;
            var y = this.radius_inner * Math.sin(tilt_radians) + this.cy;

            ctx.moveTo(x1, y);

            tilt_radians = (90.0 - (this.options.tilt / 360.0) * 180) / 180.0 * Math.PI;

            var x2 = this.radius_inner * Math.cos(tilt_radians) + this.cx;

            ctx.strokeStyle = this.options.marker_color;
            ctx.lineTo(x2, y);
            ctx.closePath();
            ctx.stroke();

            var x3 = (x2 - x1 + 1);

            if (this.options.pan <= 180)
                x3 = x2 - (x3 * (this.options.pan / 180));
            else if (this.options.pan <= 360)
                x3 = x1 + (x3 * ((this.options.pan - 180) / 180));
            else if (this.options.pan <= 540)
                x3 = x2 - (x3 * ((this.options.pan - 360) / 180));
            else 
                x3 = x1 + (x3 * ((this.options.pan - 540) / 180));

            ctx.beginPath();
            ctx.fillStyle = this.options.marker_color;
            ctx.arc(x3, y, 3, 0, 2 * Math.PI, false);
            ctx.fill();
            ctx.closePath();
            
            if (this.options.tilt > 180)
                y += 12;
            else
                y -= 5;

            ctx.font = '6pt Arial';
            ctx.textAlign = 'center';
            ctx.fillStyle = this.options.text_color;
            ctx.fillText( "Tilt " + this.options.tilt + "°", this.cx, y);
        },

        set_location: function (location) {
            if (location == null)
                return;

            var tilt = location.tilt;
            var pan = location.pan;

            if ( tilt != null && tilt >= this.options.tilt_start && tilt <= this.options.tilt_end )
                this.options.tilt = tilt;
            if ( pan != null && pan >= this.options.pan_start && pan <= this.options.pan_end )
                this.options.pan = pan;

            this._draw();
        },

        _on_mouse_down: function ( event ) {
            if (event = (event || window.event)) {
                if (event.stopPropagation != null)
                    event.stopPropagation();
                else
                    event.cancelBubble = true;
            }

            this.mouseHomeX = event.screenX;
            this.mouseHomeY = event.screenY;
            this.mouseHomeTilt = this.options.tilt;
            this.mouseHomePan = this.options.pan;

            // this._debug(this.mouseHomeX + ", " + this.mouseHomeY);

            var self = this;
            this.mouse_button = event.button;

            this.canvas.on("mouseup", function ( event ) { self._on_mouse_up( event ); });
            this.canvas.bind("mousemove", function ( event) { self._on_mouse_move( event ); });
            this.canvas.get(0).setCapture();
            return false;
        },

        _on_mouse_up: function (event) {
            if (event = (event || window.event)) {
                if (event.stopPropagation != null)
                    event.stopPropagation();
                else
                    event.cancelBubble = true;
            }

            this.canvas.off("mouseup");
            this.canvas.off("mousemove");

            this.canvas.get(0).releaseCapture();

            if (this.options.onchange && this.options.on_release_only)
                this.options.onchange(this.get_location());

            return false;
        },

        _on_mouse_wheel: function (event) {
            if (event = (event || window.event)) {
                if (event.stopPropagation != null)
                    event.stopPropagation();
                else
                    event.cancelBubble = true;
            }

            var newTilt = this.options.tilt + event.originalEvent.wheelDelta / 12;
            if (newTilt < this.options.tilt_start)
                newTilt = this.options.tilt_start;
            else if (newTilt > this.options.tilt_end)
                newTilt = this.options.tilt_end;

            this.options.tilt = newTilt;
            this._draw();

            if (this.options.onchange && !this.options.on_release_only)
                this.options.onchange(this.get_location());

            return false;
        },

        _on_mouse_move: function (event) {
            if (event = (event || window.event)) {
                if (event.stopPropagation != null)
                    event.stopPropagation();
                else
                    event.cancelBubble = true;
            }

            if (this.mouse_button != 2) {
                var newTilt = this.mouseHomeTilt + -((event.screenY - this.mouseHomeY) * this.options.tilt_factor);
                if (newTilt < this.options.tilt_start)
                    newTilt = this.options.tilt_start;
                else if (newTilt > this.options.tilt_end)
                    newTilt = this.options.tilt_end;
                this.options.tilt = newTilt;
            }

            var newPan = this.mouseHomePan + -((event.screenX - this.mouseHomeX) * this.options.pan_factor);
            if (newPan < this.options.pan_start)
                newPan = this.options.pan_start;
            else if (newPan > this.options.pan_end)
                newPan = this.options.pan_end;
            this.options.pan = newPan;

            this._draw();

            if (this.options.onchange && !this.options.on_release_only)
                this.options.onchange(this.get_location());

            return false;
        },

        _debug: function( msg ) {
            var ctx = this.canvas.get(0).getContext("2d");

            ctx.beginPath();
            ctx.fillStyle = this.options.background_color;
            ctx.rect( 0, 0, 250, 14);
            ctx.fill();
            ctx.closePath();

            ctx.beginPath();
            ctx.font = '8pt Arial';
            ctx.fillStyle = 'red';
            ctx.fillText( msg, 30, 10, 250 );
        },

        get_location: function () {
            return { 'pan': this.options.pan, 'tilt': this.options.tilt };
        },

        _setOption: function (key, value) {
            this.options[key] = value;
            $.Widget.prototype._setOption.apply(this, arguments);
        }
    });

})(jQuery);