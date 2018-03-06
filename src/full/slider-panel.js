/* 
Copyright (C) 2012,2016 Robert DeSantis
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

var slider_panel_features = [
        { class_name: "scroll_or_grid_icon", handler: setPanelScroll },
        { class_name: "track_slider_icon", handler: setPanelSliderTrack }
];

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
    this.selectable = true;
    this.max_value = 255;

    this.panel = panel;
    this.slider_frame = slider_frame;
    this.header = slider_frame.find( ".slider_header" );
    this.side_text = slider_frame.find(".slider_side_text");
    this.slider = slider_frame.find(".slider_slider");
    this.footer = slider_frame.find(".slider_footer");
    this.footer_input = slider_frame.find(".slider_footer_input");
    this.slider_select = slider_frame.find(".slider_select");

    // Constructor
    var slider = this.getSlider()
    slider.empty().slider({ value: 0, range: "min", min: 0, max: this.max_value, animate: false, orientation: "vertical" });
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

    this.channel_hint = ui_handle.find(".slider_sidecar");

    // Setup event handlers
    slider.on("slidestart", function (event,ui) {
        stopEventPropagation(event);

        slider.attr("title", '');

        var slider_object = $(this).get(0).slider_object;
        slider_object.busy = true;

        slider_object.getFooter().addClass('slider_sliding_highlight');
        slider_object.getHeader().addClass('slider_sliding_highlight');
        slider_object.getLabel().addClass('slider_sliding_highlight');

        slider_object.getChannelHint().css("display", "inline");
        slider_object.getChannelHint().html(slider_object.getRangeDescription(ui.value));
    });

    slider.on("slidestop", function (event, ui) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;

        // Tooltip becomes range value
        $(this).attr("title", slider_object.getChannelHint().html());

        slider_object.getChannelHint().html("");
        slider_object.getChannelHint().css("display", "none");

        slider_object.getFooter().removeClass('slider_sliding_highlight');
        slider_object.getHeader().removeClass('slider_sliding_highlight');
        slider_object.getLabel().removeClass('slider_sliding_highlight');

        slider_object.changeEvent(ui.value);

        slider_object.busy = false;
    });

    slider.on("slide", function (event, ui) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;

        setTimeout( function ( slider_object, value ) {
            slider_object.getFooter().html(ui.value);
            slider_object.footer_input.val( ui.value );
            slider_object.getChannelHint().html(slider_object.getRangeDescription(value));

            if ( slider_object.panel.isTrackSlider() )
                slider_object.changeEvent(value);
        }, 1, slider_object, ui.value );

    });

    slider.keydown( function( event ) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;
        if ( event.which == 32 )
            slider_object.setSelected(!slider_object.isSelected());
    });

    this.getFooter().get(0).slider_object = this;
    this.getFooter().on( "click", function (event) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;
        if ( slider_object.isUsed() ) {
            slider_object.setFooterEdit( true );
        }
    });

    this.footer_input.get(0).slider_object = this;
    this.footer_input.on( "focusout", function (event) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;

        if ( slider_object.isUsed() ) {
            var val = parseInt( slider_object.footer_input.val() );
            if ( val >= 0 && val <= slider_object.max_value )
                slider_object.changeEvent( val );

            slider_object.setFooterEdit( false );
        }
    });

    this.footer_input.keydown( function( event ) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;

        if ( event.which == 13 ) {
            var val = parseInt( slider_object.footer_input.val() );
            if ( val >= 0 && val <= slider_object.max_value )
                slider_object.changeEvent( val );

            slider_object.setFooterEdit( false );
        }
        else if ( event.which == 27 )
            slider_object.setFooterEdit( false );
    });

    this.getSliderSelect().get(0).slider_object = this;
    this.getSliderSelect().on("click", function (event) {
        stopEventPropagation(event);

        var slider_object = $(this).get(0).slider_object;

        if ( slider_object.isUsed() && slider_object.selectable ) {
            if (event.ctrlKey) {
                slider_object.panel.iterateChannels(function (slider) {
                    if (slider.selectable && slider.isUsed()) {
                        slider.setSelected(slider.getLabel().text() === slider_object.getLabel().text());
                    }
                });
            }
            else
                slider_object.setSelected(!slider_object.isSelected());
        }
    });

    this.getHeader().get(0).slider_object = this;
    this.getHeader().on( "click", function( event ) {
        stopEventPropagation(event);

       var slider_object = $(this).get(0).slider_object;

        if ( slider_object.isUsed() && slider_object.callback )
            slider_object.callback(slider_object.owner, "header", slider_object.owner_channel, 0, event );
    });

    this.side_text.get(0).slider_object = this;
    this.side_text.on( "click", function( event ) {
        stopEventPropagation(event);

       var slider_object = $(this).get(0).slider_object;

        if ( slider_object.isUsed() && slider_object.callback )
            slider_object.callback(slider_object.owner, "side-text", slider_object.owner_channel, 0, event );
    });
}

// method isUsed
Slider.prototype.isUsed = function () {
    return this.in_use;
}

Slider.prototype.setUsed = function (used) {
    this.in_use = used;
}

Slider.prototype.getNumber = function () {
    return this.number;
}

// method getValue
Slider.prototype.getValue = function () {
    return this.value;
}

// method getOwner
Slider.prototype.getOwner = function () {
    return this.owner;
}

// method getOwnerChannel
Slider.prototype.getOwnerChannel = function () {
    return this.owner_channel;
}

// method getSliderFrame
Slider.prototype.getSliderFrame = function () {
    return this.slider_frame;
}

// method getSlider 
Slider.prototype.getSlider = function () {
    return this.slider;
}

// method getFooter 
Slider.prototype.getFooter = function () {
    return this.footer;
}

// method getHeader
Slider.prototype.getHeader = function () {
    return this.header;
}

// method getLabel
Slider.prototype.getLabel = function () {
    return this.side_text;
}

// method getChannelHint
Slider.prototype.getChannelHint = function() {
    return this.channel_hint;
}

// method getSliderSelect
Slider.prototype.getSliderSelect = function () {
    return this.slider_select;
}

// method getType
Slider.prototype.getType = function () {
    return this.type;
}

// method setRanges
Slider.prototype.setRanges = function (ranges) {
    this.ranges = ranges;
}

// method getRanges
Slider.prototype.getRanges = function () {
    return this.ranges;
}

// method isBusy
Slider.prototype.isBusy = function () {
    return this.busy;
}

// method getRangeDescription
Slider.prototype.getRangeDescription = function (value) {
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
Slider.prototype.highlight = function (highlight) {
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
Slider.prototype.setValue = function (value) {
    this.getSlider().slider("value", value);
    this.getFooter().html(value);
    this.getSlider().attr("title", this.getRangeDescription(value));
    this.valueChange(value);
}

// method valueChange
Slider.prototype.valueChange = function (new_value) {
    if (this.value != new_value) {
        this.value = new_value;
        this.changed = true;
        this.panel.channelChanged();
    }
    this.typedValue = 0;
}

// method isChanged
Slider.prototype.isChanged = function () {
    return this.changed;
}

// method sendValue
Slider.prototype.sendValue = function() {
    if ( this.callback )
        this.callback(this.owner, "channel", this.owner_channel, this.value, null);
    this.changed = false;
}

Slider.prototype.setSelected = function ( selected ) {
    if ( this.selectable ) {
        if ( selected )
            this.getSliderSelect().addClass('ui-icon-green').removeClass('ui-icon-active');
        else
            this.getSliderSelect().addClass('ui-icon-active').removeClass('ui-icon-green');
    }
}

Slider.prototype.enableSelect = function ( enabled ) {
    if ( enabled )
        this.getSliderSelect().addClass('ui-icon-active');
    else
        this.getSliderSelect().removeClass('ui-icon-active').removeClass('ui-icon-green');
}

Slider.prototype.isLinkable = function () {
    return this.linkable;
}

Slider.prototype.isSelected = function () {
    return this.getSliderSelect().hasClass('ui-icon-green');
}

Slider.prototype.setFooterEdit = function( edit ) {
    if ( edit ) {
        this.getFooter().hide();

        this.footer_input.val( this.value );
        this.footer_input.show();
        this.footer_input.focus();
        this.footer_input.select();
    }
    else {
        this.getSlider().slider("value", this.value);
        this.getSlider().attr("title", this.getRangeDescription(this.value));
        this.getFooter().html( this.value );

        this.getFooter().show();
        this.footer_input.hide();
    }
}

// method changeEvent
Slider.prototype.changeEvent = function (newValue) {
    if ( this.isLinkable() && this.isSelected() ) {
        this.panel.iterateChannels( function (slider) {
            if ( slider.isLinkable() && slider.isSelected() )
                slider.setValue(newValue);
        });
    }
    else
        this.valueChange(newValue);
}

// method allocate: channel data { channel, label, name, value, max_value, ranges }
Slider.prototype.allocate = function (owner, header_tip, slider_data, callback ) {
    this.owner = owner;
    this.in_use = true;
    this.ranges = slider_data.ranges;
    this.callback = callback;
    this.owner_channel = slider_data.channel;
    this.value = slider_data.value;
    this.type = slider_data.type;
    this.tooltip = header_tip;
    this.busy = false;
    this.typedValue = 0;
    this.max_value = slider_data.max_value;

    this.linkable = slider_data.linkable || false;
    this.selectable = slider_data.selectable || this.linkable;

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
    slider.slider("option", "max", this.max_value);

    this.enableSelect( this.selectable );

    if ( this.selectable && slider_data.selected == true )
        this.setSelected( true );

    this.setFooterEdit( false );
            
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
Slider.prototype.release = function () {
    this.owner = 0;
    this.in_use = false;
    this.ranges = null;
    this.callback = null;
    this.owner_channel = 0;
    this.value = 0;
    this.tooltip = "";
    this.busy = false;
    this.typedValue = 0;
    this.selectable = false;
    this.linkable = false;

    this.getHeader().text("");
    this.getLabel().text("");
    this.getFooter().text("");
    this.enableSelect( false );

    this.highlight(false);
    this.setFooterEdit( false );

    var slider = this.getSlider();
    slider.slider("option", "animate", false);    // Must turn this off before loading
    slider.slider("value", 0);
    slider.attr("title", "");
    slider.slider("option", "disabled", true);
}

// ----------------------------------------------------------------------------
// class SliderPanel
//
function SliderPanel(panel_id, num_sliders, track_slider) {

    // method resetLinks
    this.resetLinks = function () {
        this.iterateChannels(function (slider) {
            if (slider.isLinkable() && slider.isUsed() && slider.isSelected()) {
                slider.setSelected(false);
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
    this.allocateChannels = function (owner, owner_name, channel_data, callback ) {
        var num_channels = channel_data.length;
        if (num_channels == 0)
            return;

        var channel_start = this.getFreeChannelStart(num_channels);
        if (channel_start == -1) {
            messageBox("Error: Not enough free sliders for " + num_channels + " channels");
            return;
        }

        for (var i = 0; i < num_channels; i++ ) {
            this.sliders[i + channel_start].allocate(owner, owner_name, channel_data[i], callback, null );
        }
    }

    // method setSliderTitle
    this.setSliderTitle = function (title) {
        if ( this.title_div != null && this.title_div .length > 0 )
            this.title_div.text(title);
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
            if (this.sliders[i].isUsed() && this.sliders[i].getOwner() == owner && this.sliders[i].getOwnerChannel() == channel_id)
                return this.sliders[i];
        }

        return null;
    }

    // method getChannelValues
    this.getChannelValues = function ( owner ) {
        var values = []

        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].getOwner() == owner ) {
                var slider = this.sliders[i];
                values.push( slider.getValue() );
            }
        }

        return values;
    }

    // method getChannels (return channel, value, selected)
    this.getChannels = function ( owner ) {
        var data = []

        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].getOwner() == owner ) {
                var slider = this.sliders[i];
                data.push( { channel: slider.getOwnerChannel(), value: slider.getValue(), selected: slider.isSelected() } );
            }
        }

        return data;
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

    // channelChanged
    this.channelChanged = function() {
        if ( this.update_timer == null ) {
            this.update_timer = setTimeout(function ( panel ) { panel.sendUpdates(); }, 50, this );        
        }
    }
    
    this.sendUpdates = function () {
        for (var i = 0; i < this.num_sliders; i++) {
            if (this.sliders[i].isUsed() && this.sliders[i].isChanged()) {
                this.sliders[i].sendValue();
            }
        }

        this.update_timer = null;
    }

    // Constructor
    if ( num_sliders < 1 )
        num_sliders = 1;

    var panel_div = $("#" + panel_id);

    this.panel_id = panel_id;
    this.track_slider = track_slider;                       // Slider owner will be notified of every movement
    this.sliders = [];
    this.num_sliders = 0;
    this.default_num_sliders = num_sliders;
    this.title_div = panel_div.find(".slider_pane_title");

    var content_div = panel_div.find(".scroll-content");
    content_div.empty();

    // Populate with initial number of sliders
    for (var j = 0; j < num_sliders; j++)
        this.addSlider();

    // Add slider features
    for ( var i=0; i < slider_panel_features.length; i++ ) {
        var feature = slider_panel_features[i];

        var feature_icon = panel_div.find( "." + feature.class_name );
        if ( feature_icon.length == 0 )
            continue;

        if ( feature.title != null && feature.title != undefined )
            feature_icon.attr( "title", feature.title );

        if ( feature.attach != null && feature.attach != undefined )
            feature.attach( this, feature_icon );

        if ( feature.handler != null && feature.handler != undefined ) {
            feature_icon.data( "handler", feature.handler );

            var self = this;
            feature_icon.unbind( "click").click(function (event) {
                stopEventPropagation(event);
                $(this).data( "handler" )( self, $(this) )
            });
        }
    }

    this.setScrollContent(true);
    this.setTrackSlider(this.track_slider);
}

// ----------------------------------------------------------------------------
//
function initializeScrollPane( scrollPane ) {
    var scrollbar = scrollPane.find(".scroll-bar");
    var scrollContent = scrollPane.find(".scroll-content");
    var scrollWrap = scrollPane.find(".scroll-bar-wrap");

    //change overflow to hidden now that slider handles the scrolling
    scrollPane.css("overflow", "hidden");
    
    // Initially hide the scrollbar otherwise we get a bounce due to the timeout
    scrollWrap.hide();

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

// ----------------------------------------------------------------------------
//
function setPanelScroll( slider_panel, icon_element ) {
    slider_panel.setScrollContent( !slider_panel.isScrollContent() );
    client_config_update = true;    // Set global client config
}

// ----------------------------------------------------------------------------
//
function setPanelSliderTrack( slider_panel, icon_element ) {
    slider_panel.setTrackSlider( !slider_panel.isTrackSlider() );
}
