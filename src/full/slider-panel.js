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
// class Slider
//
function Slider(panel, number, slider_frame) {
    this.number = number;
    this.in_use = false;
    this.ranges = null;
    this.owner = 0;
    this.callback = null;
    this.owner_channel = 0;
    this.value = 0;
    this.type = 0;
    this.busy = false;
    this.changed = false;
    this.linkable = false;

    this.panel = panel;
    this.slider_frame = slider_frame;
    this.header = slider_frame.find( ".slider_header" );
    this.side_text = slider_frame.find(".slider_side_text");
    this.slider = slider_frame.find(".slider_slider");
    this.footer = slider_frame.find(".slider_footer");
    this.linked = slider_frame.find(".slider_select");

    // method isUsed
    this.isUsed = function () {
        return this.in_use;
    }

    this.setUsed = function (used) {
        this.in_use = used;
    }

    this.getNumber = function () {
        return this.number;
    }

    // method getValue
    this.getValue = function () {
        return this.value;
    }

    // method getOwner
    this.getOwner = function () {
        return this.owner;
    }

    // method getOwnerChannel
    this.getOwnerChannel = function () {
        return this.owner_channel;
    }

    // method getSliderFrame
    this.getSliderFrame = function () {
        return this.slider_frame;
    }

    // method getSlider 
    this.getSlider = function () {
        return this.slider;
    }

    // method getFooter 
    this.getFooter = function () {
        return this.footer;
    }

    // method getHeader
    this.getHeader = function () {
        return this.header;
    }

    // method getLabel
    this.getLabel = function () {
        return this.side_text;
    }

    // method getLinked
    this.getLinked = function () {
        return this.linked;
    }

    // method getType
    this.getType = function () {
        return this.type;
    }

    // method setRanges
    this.setRanges = function (ranges) {
        this.ranges = ranges;
    }

    // method getRanges
    this.getRanges = function () {
        return this.ranges;
    }

    // method isBusy
    this.isBusy = function () {
        return this.busy;
    }

    // method getRangeDescription
    this.getRangeDescription = function (value) {
        if (this.ranges != null) {
            for (var i = 0; i < this.ranges.length; i++) {
                var range = this.ranges[i];
                if (value >= range.start && value <= range.end)
                    return range.name;
            }
        }
        return "";
    }

    // method highlight
    this.highlight = function (highlight) {
        if (highlight) {
            if (this.isBusy() || !this.panel.isBusy()) {
                this.getFooter().addClass('slider_highlight');
                this.getHeader().addClass('slider_highlight');
                this.getLabel().addClass('slider_highlight');
            }
        }
        else {
            this.getFooter().removeClass('slider_highlight');
            this.getHeader().removeClass('slider_highlight');
            this.getLabel().removeClass('slider_highlight');
        }
    }

    // method setValue
    this.setValue = function (value) {
        this.getSlider().slider("value", value);
        this.getFooter().html(value);
        this.getSlider().attr("title", escapeForHTML(this.getRangeDescription(value)));
        this.valueChange(value);
    }

    // method valueChange
    this.valueChange = function (new_value) {
        if (this.value != new_value) {
            this.value = new_value;
            this.changed = true;
        }
    }

    // method isChanged
    this.isChanged = function () {
        return this.changed;
    }

    // method sendValue
    this.sendValue = function() {
        if ( this.callback )
            this.callback(this.owner, this.owner_channel, this.value);
        this.changed = false;
    }

    this.setLinked = function (linked) {
        if ( linked )
            this.getLinked().addClass('ui-icon-green').removeClass('ui-icon-active');
        else
            this.getLinked().addClass('ui-icon-active').removeClass('ui-icon-green');
    }

    this.hideLinked = function () {
        this.getLinked().removeClass('ui-icon-active').removeClass('ui-icon-green');
    }

    this.isLinked = function (linked) {
        return this.getLinked().hasClass('ui-icon-green');
    }

    this.isLinkable = function () {
        return this.linkable;
    }

    // method changeEvent
    this.changeEvent = function (newValue) {
        if (this.isLinked()) {
            this.panel.iterateChannels(function (slider) {
                if (slider.isLinked())
                    slider.setValue(newValue);
            });
        }
        else
            this.valueChange(newValue);
    }

    // method allocate: channel data { channel, label, name, value, max_value, ranges }
    this.allocate = function (owner, header_tip, slider_data, callback, linkable ) {
        this.owner = owner;
        this.in_use = true;
        this.ranges = slider_data.ranges;
        this.callback = callback;
        this.owner_channel = slider_data.channel;
        this.value = slider_data.value;
        this.type = slider_data.type;
        this.tooltip = header_tip;
        this.busy = false;
        this.linkable = linkable;

        this.getHeader().text(slider_data.label);
        this.getHeader().attr("title", header_tip);
        this.getHeader().css('color', slider_data.color ? slider_data.color : "");

        this.getLabel().text(slider_data.name);
        this.getFooter().text(slider_data.value);

        var slider = this.getSlider();
        slider.slider("value", slider_data.value);
        slider.attr("title", this.getRangeDescription(slider_data.value));
        slider.slider("option", "disabled", false);
        slider.slider("option", "animate", true);
        slider.slider("option", "max", slider_data.max_value);

        if (this.isLinkable())
            this.setLinked(false);
        else
            this.hideLinked();
            
        this.getSliderFrame().hover(
            function () {
                var slider_object = $(this).get(0).slider_object;
                slider_object.highlight(true);

                if ( !slider_object.panel.isBusy() )
                    slider_object.panel.setSliderTitle(slider_object.tooltip);
            },
            function () {
                var slider_object = $(this).get(0).slider_object;
                slider_object.highlight(false);
                slider_object.panel.setSliderTitle("");
            }
        );
    }

    // method release
    this.release = function () {
        this.owner = 0;
        this.in_use = false;
        this.ranges = null;
        this.callback = null;
        this.owner_channel = 0;
        this.value = 0;
        this.tooltip = "";
        this.busy = false;

        this.getHeader().text("");
        this.getLabel().text("");
        this.getFooter().text("");
        this.hideLinked();

        this.highlight(false);

        var slider = this.getSlider();
        slider.slider("option", "animate", false);    // Must turn this off before loading
        slider.slider("value", 0);
        slider.attr("title", "");
        slider.slider("option", "disabled", true);
    }

    // Constructor
    var slider = this.getSlider()
    slider.empty().slider({ value: 0, range: "min", min: 0, max: 255, animate: false, orientation: "vertical" });
    slider.slider("option", "disabled", true);

    slider.get(0).setAttribute("slider_number", this.number);
    slider.get(0).slider_object = this;
    this.getFooter().html("");

    this.getSliderFrame().get(0).slider_object = this;

    // Stylize bottom area
    var slider_range = slider_frame.find(".ui-slider-range");
    slider_range.css('background', 'url() rgb( 3, 11, 46 ) 0px 0px');

    // Add sidecar for range information
    var ui_handle = slider.find('.ui-slider-handle');
    ui_handle.append('<div class="slider_sidecar" style="display:none; max-width: 140px;"></div>');

    // Setup event handlers
    var channel_hint = null;
    var slider_footer = null;

    slider.on("slidestart", function (event,ui) {
        slider.attr("title", '');
        channel_hint = $(this).find(".slider_sidecar");
        channel_hint.css("display", "inline");

        var slider_object = $(this).get(0).slider_object;
        slider_object.busy = true;

        slider_object.getFooter().addClass('slider_sliding_highlight');
        slider_object.getHeader().addClass('slider_sliding_highlight');
        slider_object.getLabel().addClass('slider_sliding_highlight');

        slider_footer = slider_object.getFooter();
        channel_hint.html(slider_object.getRangeDescription(ui.value));
    });

    slider.on("slidestop", function (event, ui) {
        // Tooltip becomes range value
        $(this).attr("title", channel_hint.html());

        channel_hint.html("");
        channel_hint.css("display", "none");
        channel_hint = slider_footer = null;

        var slider_object = $(this).get(0).slider_object;
        slider_object.busy = false;

        slider_object.getFooter().removeClass('slider_sliding_highlight');
        slider_object.getHeader().removeClass('slider_sliding_highlight');
        slider_object.getLabel().removeClass('slider_sliding_highlight');

        slider_object.changeEvent(ui.value);
    });

    var panel = this.panel;

    slider.on("slide", function (event, ui) {
        var slider_object = $(this).get(0).slider_object;
        slider_footer.html(ui.value);
        channel_hint.html(slider_object.getRangeDescription(ui.value));

        if ( panel.isTrackSlider() )
            slider_object.changeEvent(ui.value);
    });

    var linked = $(this.getLinked());
    var self = this;

    linked.on("click", function (event) {
        if (event.ctrlKey) {
            self.panel.iterateChannels(function (slider) {
                if (slider.isLinkable() && slider.isUsed()) {
                    slider.setLinked(slider.getLabel().text() === self.getLabel().text());
                }
            });
        }
        else
            self.setLinked(!self.isLinked());
    });
}

// ----------------------------------------------------------------------------
// class SliderPanel
//
function SliderPanel(panel_id, num_sliders, track_slider) {

    // method resetLinks
    this.resetLinks = function () {
        this.iterateChannels(function (slider) {
            if (slider.isLinkable() && slider.isUsed() && slider.isLinked()) {
                slider.setLinked(false);
            }
        });
    }

    // method getFreeChannelStart
    this.getFreeChannelStart = function (needed_sliders) {
        var consecutive_free = 0;
        var start_slider = -1;

        for (var index = 0; index < this.num_sliders; index++) {
            if (this.sliders[index].isUsed()) {
                start_slider = -1;
                consecutive_free = 0;
            }
            else {
                if (start_slider == -1)
                    start_slider = index;
                consecutive_free += 1;
                if (consecutive_free == needed_sliders)
                    return start_slider;
            }
        }

        // Add more sliders
        if (consecutive_free != 0)                // Unused sliders at the end
            needed_sliders -= consecutive_free;
        else                                      // Need to create them all
            start_slider = this.num_sliders;

        for (var i = 0; i < needed_sliders; i++)
            this.addSlider();

        this.adjustSize();

        return start_slider;
    }

    // method allocateChannels
    this.allocateChannels = function (owner, owner_name, channel_data, callback, linkable ) {
        var num_channels = channel_data.length;
        if (num_channels == 0)
            return;

        var channel_start = this.getFreeChannelStart(num_channels);
        if (channel_start == -1) {
            messageBox("Error: Not enough free sliders for " + num_channels + " channels");
            return;
        }

        for (var i = 0; i < num_channels; i++ ) {
            this.sliders[i + channel_start].allocate(owner, owner_name, channel_data[i], callback, linkable );
        }
    }

    // method setSliderTitle
    this.setSliderTitle = function (title) {
        $("#slider_pane_title").text(title);
    }

    // method releaseChannels
    this.releaseChannels = function (owner) {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].getOwner() == owner)
                this.sliders[i].release();
        }
    }

    // method releaseChannels
    this.highlightChannels = function( owner, highlight ) {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].getOwner() == owner)
                this.sliders[i].highlight( highlight );
        }
    }

    // method releaseAllChannels
    this.releaseAllChannels = function () {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() )
                this.sliders[i].release();
        }
    }

    // method iterateChannels
    this.iterateChannels = function ( callback ) {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed())
                callback( this.sliders[i] );
        }
    }

    // method findChannel
    this.findChannel = function ( owner, channel_id ) {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].getOwner() == owner && this.sliders[i].getOwnerChannel())
                return this.sliders[i];
        }

        return null;
    }

    // method isBusy
    this.isBusy = function () {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isBusy())
                return true;
        }
        return false;
    }

    // method isTrackSlider
    this.isTrackSlider = function () {
        return this.track_slider;
    }

    // method setTrackSlider
    this.setTrackSlider = function ( track ) {
        this.track_slider = track;

        var track_slider_icon = $("#" + this.panel_id).find(".track_slider_icon");
        if (track_slider_icon != null) {
            if (track)
                track_slider_icon.removeClass('ui-icon-radio-off')
                                .addClass('ui-icon-bullet')
                                .attr('title', 'update on movement');
            else
                track_slider_icon.removeClass('ui-icon-bullet')
                                .addClass('ui-icon-radio-off')
                                .attr('title', 'update on release');
        }
    }

    // method adjustSize
    this.adjustSize = function () {
        var scrollPane = $("#" + self.panel_id);
        var content_div = scrollPane.find(".scroll-content");
        var scrollWrap = scrollPane.find(".scroll-bar-wrap");

        var slider_width = this.sliders[0].getSliderFrame().outerWidth(true);
        var slider_height = this.sliders[0].getSliderFrame().outerHeight(true);

        if (this.scrollContent) {
            var width = this.num_sliders * slider_width;
            content_div.css('width', width + 'px');
            scrollWrap.show();
            self.setupScroller();
        }
        else {
            scrollWrap.hide();
            content_div.css('width', '100%').css('margin-left', 0);
        }
    }

    // method setScrollContent
    this.isScrollContent = function (scroll) {
        return this.scrollContent;
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

    // method setupScroller
    this.setupScroller = function () {
        initializeScrollPane($("#" + this.panel_id));
    }

    // methodAddSlider
    this.addSlider = function () {
        var slider_frame = $(slider_template.innerHTML);
        content_div.append(slider_frame);
        this.sliders[this.num_sliders] = new Slider(this, this.num_sliders, slider_frame);
        return this.num_sliders++;
    }

    // trimUnused
    this.trimUnused = function () {
        while (this.num_sliders > this.default_num_sliders ) {
            var slider = this.sliders[this.num_sliders - 1];
            
            if (slider.getOwner() != 0)
                break;

            slider.getSliderFrame().remove();
            this.sliders.length = this.sliders.length - 1;
            this.num_sliders -= 1;
        }
    }

    this.sendUpdates = function () {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].isChanged()) {
                this.sliders[i].sendValue();
                break;
            }
        }

        var _this = this;
        this.update_timer = setTimeout(function () { _this.sendUpdates(); }, 50);
    }

    // Constructor
    this.panel_id = panel_id;
    this.track_slider = track_slider;                       // Slider owner will be notified of every movement
    this.sliders = new Array();
    this.num_sliders = 0;
    this.default_num_sliders = num_sliders;

    var content_div = $("#" + this.panel_id).find(".scroll-content");

    // Populate with initial number of sliders
    for (var i = 0; i < num_sliders; i++)
        this.addSlider();

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

    var track_slider_icon = $("#" + this.panel_id).find(".track_slider_icon");
    if (track_slider_icon != null) {
        var self = this;
        track_slider_icon.addClass( 'ui-icon' );
        track_slider_icon.click(function (event) {
            stopEventPropagation(event);
            self.setTrackSlider(!self.isTrackSlider());
        });
    }

    this.setScrollContent(true);
    this.setTrackSlider(this.track_slider);

    // Setup timer to send changes to the server
    this.sendUpdates();
}



