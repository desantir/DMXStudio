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

function VolumeControl( volume_id ) {

    // ----------------------------------------------------------------------------
    //
    this.setMute = function( mute ) {
        if (mute != this.volume_mute_element.hasClass("ui-icon-volume-off")) {
            if (!mute) {
                this.volume_mute_element.attr('class', "ui-icon ui-icon-volume-on");
                this.volume_mute_element.attr('title', 'mute');
            }
            else {
                this.volume_mute_element.attr('class', "ui-icon-red ui-icon-volume-off");
                this.volume_mute_element.attr('title', 'unmute');
            }
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.setVolume = function( master_volume ) {
        if (this.master_volume_element.slider("value") != master_volume && !this.master_volume_handle_element.is(':focus') ) {
            this.master_volume_element_value.html(master_volume);
            this.master_volume_element.slider("value", master_volume);
        }
    }

    // ----------------------------------------------------------------------------
    //
    this.sendMasterVolumeUpdate = function() {
        var self = this;

        delayUpdate( "volume", 10, function() {
            $.ajax({
                type: "GET",
                url: "/dmxstudio/rest/control/venue/volume/master/" + self.master_volume_element.slider("value"),
                cache: false,
                error: onAjaxError
            });
        });
    }

    // Constructor
    this.volume_container = $("#"+ volume_id );
    this.master_volume_element_value = this.volume_container.find( ".volume_value" );
    this.master_volume_element = this.volume_container.find( ".volume" );
    this.volume_mute_element = this.volume_container.find( ".volume_mute" );

    // setup master volume
    initializeHorizontalSlider( volume_id + " .volume", 0, 100, 0);

    this.master_volume_handle_element = this.master_volume_element.find( ".ui-slider-handle" );

    var self = this;

    this.master_volume_element.on("slide slidestop", function() { self.sendMasterVolumeUpdate(); } );

    this.volume_mute_element.click(function () {
        var mute = $(this).hasClass("ui-icon-volume-on"); // Toggle mute
        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/control/venue/volume/mute/" + ((mute) ? 1 : 0),
            cache: false,
            error: onAjaxError
        });
    });
}

