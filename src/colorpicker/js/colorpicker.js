/**
 *
 * Color picker
 * Author: Stefan Petre www.eyecon.ro
 * 
 * Dual licensed under the MIT and GPL licenses
 * 
 */
(function ($) {
	var ColorPicker = function () {
	    var
			ids = {},
			inAction,
			charMin = 65,
			visible,
            hide_ms = null,
			tpl = '<div class="colorpicker">' +
                  '<div class="colorpicker_color"><div><div></div></div></div>' +
                  '<div class="colorpicker_hue"><div></div></div>' +
                  '<div class="colorpicker_new_color"></div>' +
                  '<div class="colorpicker_current_color"></div>' +
                  '<div class="colorpicker_hex"><input type="text" maxlength="6" size="6" /></div>' +
                  '<div class="colorpicker_rgb_r colorpicker_field"><input type="text" maxlength="3" size="3" /><span></span></div>' +
                  '<div class="colorpicker_rgb_g colorpicker_field"><input type="text" maxlength="3" size="3" /><span></span></div>' +
                  '<div class="colorpicker_rgb_b colorpicker_field"><input type="text" maxlength="3" size="3" /><span></span></div>' +
                  '<div class="colorpicker_hsb_h colorpicker_field"><input type="text" maxlength="3" size="3" /><span></span></div>' +
                  '<div class="colorpicker_hsb_s colorpicker_field"><input type="text" maxlength="3" size="3" /><span></span></div>' +
                  '<div class="colorpicker_hsb_b colorpicker_field"><input type="text" maxlength="3" size="3" /><span></span></div>' +
                  '<div class="colorpicker_submit"></div>' +

                  '<div class="colorpicker_chip1 colorpicker_chip" data-chip="1"></div>' +
                  '<div class="colorpicker_chip2 colorpicker_chip" data-chip="2"></div>' +
                  '<div class="colorpicker_chip3 colorpicker_chip" data-chip="3"></div>' +
                  '<div class="colorpicker_chip4 colorpicker_chip" data-chip="4"></div>' +
                  '<div class="colorpicker_chip5 colorpicker_chip" data-chip="5"></div>' +
                  '<div class="colorpicker_chip6 colorpicker_chip" data-chip="6"></div>' +
                  '<div class="colorpicker_chip7 colorpicker_chip" data-chip="7"></div>' +
                  '<div class="colorpicker_chip8 colorpicker_chip" data-chip="8"></div>' +
                  '<div class="colorpicker_chip9 colorpicker_chip" data-chip="9"></div>' +
                  '<div class="colorpicker_chip10 colorpicker_chip" data-chip="10"></div>' +
                  '<div class="colorpicker_chip11 colorpicker_chip" data-chip="11"></div>' +
                  '<div class="colorpicker_chip12 colorpicker_chip" data-chip="12"></div>' +
                  '<div class="colorpicker_chip13 colorpicker_chip" data-chip="13"></div>' +
                  '<div class="colorpicker_chip14 colorpicker_chip" data-chip="14"></div>' +
                  '<div class="colorpicker_chip15 colorpicker_chip" data-chip="15"></div>' +
                  '<div class="colorpicker_chip16 colorpicker_chip" data-chip="16"></div>' +

                  '<div class="colorpicker_palette1 colorpicker_chip palette_chip" title="palette color sequence"></div>' +
                  '<div class="colorpicker_palette2 colorpicker_chip palette_chip" title="predefined color sequence"></div>' +
                  '<div class="colorpicker_palette3 colorpicker_chip palette_chip" title="rainbow color sequence"></div>' +
                  '<div class="colorpicker_palette4 colorpicker_chip palette_chip" title="video color sequence"></div>' +

                  '<div class="colorpicker_palette5 colorpicker_chip palette_chip" title="user palette 1"></div>' +
                  '<div class="colorpicker_palette6 colorpicker_chip palette_chip" title="user palette 2"></div>' +
                  '<div class="colorpicker_palette7 colorpicker_chip palette_chip" title="user palette 3"></div>' +
                  '<div class="colorpicker_palette8 colorpicker_chip palette_chip" title="user palette 4"></div>' +
                  '</div>',
			defaults = {
				eventName: 'click',
				onShow: function () {},
				onBeforeShow: function(){},
				onChange: function () {},
				onSubmit: function () {},
				color: 'ff0000',
				livePreview: true,
				flat: false,
                autoClose: false,
                showPalettes: true,
                anchor: null
			},
			fillRGBFields = function  (hsb, cal) {
				var rgb = HSBToRGB(hsb);
				$(cal).data('colorpicker').fields
					.eq(1).val(rgb.r).end()
					.eq(2).val(rgb.g).end()
					.eq(3).val(rgb.b).end();
			},
			fillHSBFields = function  (hsb, cal) {
				$(cal).data('colorpicker').fields
					.eq(4).val(hsb.h).end()
					.eq(5).val(hsb.s).end()
					.eq(6).val(hsb.b).end();
			},
			fillHexFields = function (hsb, cal) {
				$(cal).data('colorpicker').fields
					.eq(0).val(HSBToHex(hsb)).end();
			},
			setSelector = function (hsb, cal) {
				$(cal).data('colorpicker').selector.css('backgroundColor', '#' + HSBToHex({h: hsb.h, s: 100, b: 100}));
				$(cal).data('colorpicker').selectorIndic.css({
					left: parseInt(150 * hsb.s/100, 10),
					top: parseInt(150 * (100-hsb.b)/100, 10)
				});
			},
			setHue = function (hsb, cal) {
				$(cal).data('colorpicker').hue.css('top', parseInt(150 - 150 * hsb.h/360, 10));
			},
			setCurrentColor = function (hsb, cal) {
				$(cal).data('colorpicker').currentColor.css('backgroundColor', '#' + HSBToHex(hsb));
			},
			setNewColor = function (hsb, cal) {
				$(cal).data('colorpicker').newColor.css('backgroundColor', '#' + HSBToHex(hsb));
			},
			keyDown = function (ev) {
				var pressedKey = ev.charCode || ev.keyCode || -1;
				if ((pressedKey > charMin && pressedKey <= 90) || pressedKey == 32) {
					return false;
				}
				var cal = $(this).parent().parent();
				if (cal.data('colorpicker').livePreview === true) {
					change.apply(this);
				}
			},

            chipSet = function ( ev ) {
                var chip = $(this);
                var chip_number = chip.attr( "data-chip" );
                if ( chip_number == null || chip_number == undefined )
                    return;

                var cal = chip.parent();
                var el = cal.data('colorpicker');

                if ( el.onChipSet == null )
                    return;

				var col = cal.data('colorpicker').color;
                var hex = HSBToHex(col);

				if ( cal.data('colorpicker').onChipSet( parseInt( chip_number), col, hex, HSBToRGB(col) ) )
                    chip.css( 'background-color', "#" + hex );
            },

            chipClick = function (ev) {
                var chip = $(this);
                var cal = chip.parent();
                var background_color = chip.css("background-color").toLowerCase();

                // Convert rgb( #, #, # [,#] to hex
                if ( background_color.indexOf( "rgb(" ) == 0 ) {
                    var rgb = background_color.match(/^rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/);

                    background_color = (parseInt( rgb[1] ) << 16 | parseInt( rgb[2] ) << 8 | parseInt( rgb[3] )).toString( 16 );

                    while ( background_color.length < 6 )
                        background_color = "0" + background_color;

                    background_color = "#" + background_color;
                }
                else
                    background_color = fixHex( background_color ) ;

                var col = HexToHSB( background_color );
                
                cal.data('colorpicker').color = col;

                if (ev) {
                    fillRGBFields(col, cal.get(0));
                    fillHexFields(col, cal.get(0));
                    fillHSBFields(col, cal.get(0));
                }

                setSelector(col, cal.get(0));
                setHue(col, cal.get(0));
                setNewColor(col, cal.get(0));

                cal.data('colorpicker').onChange.apply(cal, [col, HSBToHex(col), HSBToRGB(col)]);

                return false;
            },
			change = function (ev) {
				var cal = $(this).parent().parent(), col;
				if (this.parentNode.className.indexOf('_hex') > 0) {
					cal.data('colorpicker').color = col = HexToHSB(fixHex(this.value));
				} else if (this.parentNode.className.indexOf('_hsb') > 0) {
				    cal.data('colorpicker').color = col = fixHSB({
				        h: parseInt(cal.data('colorpicker').fields.eq(4).val(), 10),
				        s: parseInt(cal.data('colorpicker').fields.eq(5).val(), 10),
				        b: parseInt(cal.data('colorpicker').fields.eq(6).val(), 10)
				    });
				} else {
					cal.data('colorpicker').color = col = RGBToHSB(fixRGB({
						r: parseInt(cal.data('colorpicker').fields.eq(1).val(), 10),
						g: parseInt(cal.data('colorpicker').fields.eq(2).val(), 10),
						b: parseInt(cal.data('colorpicker').fields.eq(3).val(), 10)
					}));
				}
				if (ev) {
					fillRGBFields(col, cal.get(0));
					fillHexFields(col, cal.get(0));
					fillHSBFields(col, cal.get(0));
				}
				setSelector(col, cal.get(0));
				setHue(col, cal.get(0));
				setNewColor(col, cal.get(0));
				cal.data('colorpicker').onChange.apply(cal, [col, HSBToHex(col), HSBToRGB(col)]);
			},
			blur = function (ev) {
				var cal = $(this).parent().parent();
				cal.data('colorpicker').fields.parent().removeClass('colorpicker_focus');
			},
			focus = function () {
				charMin = this.parentNode.className.indexOf('_hex') > 0 ? 70 : 65;
				$(this).parent().parent().data('colorpicker').fields.parent().removeClass('colorpicker_focus');
				$(this).parent().addClass('colorpicker_focus');
			},
			downIncrement = function (ev) {
				var field = $(this).parent().find('input').focus();
				var current = {
					el: $(this).parent().addClass('colorpicker_slider'),
					max: this.parentNode.className.indexOf('_hsb_h') > 0 ? 360 : (this.parentNode.className.indexOf('_hsb') > 0 ? 100 : 255),
					y: ev.pageY,
					field: field,
					val: parseInt(field.val(), 10),
					preview: $(this).parent().parent().data('colorpicker').livePreview					
				};
				$(document).bind('mouseup', current, upIncrement);
				$(document).bind('mousemove', current, moveIncrement);
			},
			moveIncrement = function (ev) {
				ev.data.field.val(Math.max(0, Math.min(ev.data.max, parseInt(ev.data.val + ev.pageY - ev.data.y, 10))));
				if (ev.data.preview) {
					change.apply(ev.data.field.get(0), [true]);
				}
				return false;
			},
			upIncrement = function (ev) {
				change.apply(ev.data.field.get(0), [true]);
				ev.data.el.removeClass('colorpicker_slider').find('input').focus();
				$(document).unbind('mouseup', upIncrement);
				$(document).unbind('mousemove', moveIncrement);
				return false;
			},
			downHue = function (ev) {
				var current = {
					cal: $(this).parent(),
					y: $(this).offset().top
				};
				current.preview = current.cal.data('colorpicker').livePreview;
				$(document).bind('mouseup', current, upHue);
				$(document).bind('mousemove', current, moveHue);
			},
			moveHue = function (ev) {
				change.apply(
					ev.data.cal.data('colorpicker')
						.fields
						.eq(4)
						.val(parseInt(360*(150 - Math.max(0,Math.min(150,(ev.pageY - ev.data.y))))/150, 10))
						.get(0),
					[ev.data.preview]
				);
				return false;
			},
			upHue = function (ev) {
				fillRGBFields(ev.data.cal.data('colorpicker').color, ev.data.cal.get(0));
				fillHexFields(ev.data.cal.data('colorpicker').color, ev.data.cal.get(0));
				$(document).unbind('mouseup', upHue);
				$(document).unbind('mousemove', moveHue);
				return false;
			},
			downSelector = function (ev) {
				var current = {
					cal: $(this).parent(),
					pos: $(this).offset()
				};
				current.preview = current.cal.data('colorpicker').livePreview;
				$(document).bind('mouseup', current, upSelector);
				$(document).bind('mousemove', current, moveSelector);
			},
			moveSelector = function (ev) {
				change.apply(
					ev.data.cal.data('colorpicker')
						.fields
						.eq(6)
						.val(parseInt(100*(150 - Math.max(0,Math.min(150,(ev.pageY - ev.data.pos.top))))/150, 10))
						.end()
						.eq(5)
						.val(parseInt(100*(Math.max(0,Math.min(150,(ev.pageX - ev.data.pos.left))))/150, 10))
						.get(0),
					[ev.data.preview]
				);
				return false;
			},
			upSelector = function (ev) {
				fillRGBFields(ev.data.cal.data('colorpicker').color, ev.data.cal.get(0));
				fillHexFields(ev.data.cal.data('colorpicker').color, ev.data.cal.get(0));
				$(document).unbind('mouseup', upSelector);
				$(document).unbind('mousemove', moveSelector);
				return false;
			},
			enterSubmit = function (ev) {
				$(this).addClass('colorpicker_focus');
			},
			leaveSubmit = function (ev) {
				$(this).removeClass('colorpicker_focus');
			},
			clickSubmit = function (ev) {
				var cal = $(this).parent();
				var col = cal.data('colorpicker').color;
				cal.data('colorpicker').origColor = col;
				setCurrentColor(col, cal.get(0));
				cal.data('colorpicker').onSubmit(col, HSBToHex(col), HSBToRGB(col), cal.data('colorpicker').el, cal.data('colorpicker').anchor );
				if (cal.data('colorpicker').autoClose)
				    cal.ColorPickerHide.apply($(cal.data('colorpicker').el));
			},
			show = function (ev) {
			    var cal = $('#' + $(this).data('colorpickerId'));

			    if ( cal.data('hide_ms') != null) {
			        var now = new Date().getTime() - 500;
			        // Stop the "bounce" when closing the color picker
			        if (cal.data('hide_ms') > now)
			            return;   // Stops any addition jQuery event handlers
			    }

                var palette_chips = cal.find( ".palette_chip" );
                if ( cal.data('colorpicker').showPalettes )
                    palette_chips.show();
                else
                    palette_chips.hide();

				cal.data('colorpicker').onBeforeShow.apply(this, [cal.get(0)]);
				
                var anchor = cal.data('colorpicker').anchor;
                if ( anchor == null )
                    anchor = $(this);

                var pos = anchor.offset();
				var viewPort = getViewport();
				var top = pos.top + anchor[0].offsetHeight;
				var left = pos.left;
				if (top + 176 > viewPort.t + viewPort.h) {
					top -= anchor[0].offsetHeight + 176;
				}
				if (left + 356 > viewPort.l + viewPort.w) {
					left -= 356;
				}

				top -= viewPort.t;
				left -= viewPort.l;

				cal.css({left: left + 'px', top: top + 'px', "z-index": 2000, position: 'fixed' });
				if (cal.data('colorpicker').onShow.apply(this, [cal.get(0)]) != false) {
					cal.show();
				}
				$(document).bind('mousedown', {cal: cal}, document_hide);
				return false;
			},
            document_hide = function (ev) {
				if (!isChildOf(ev.data.cal.get(0), ev.target, ev.data.cal.get(0))) {
				    if (ev.data.cal.data('colorpicker').onHide.apply(this, [ev.data.cal.get(0)]) != false) {
						ev.data.cal.hide();
				    }
				    ev.data.cal.data('hide_ms', new Date().getTime());
                    $(document).unbind('mousedown', document_hide);
				}
			},
			isChildOf = function(parentEl, el, container) {
				if (parentEl == el) {
					return true;
				}
				if (parentEl.contains) {
					return parentEl.contains(el);
				}
				if ( parentEl.compareDocumentPosition ) {
					return !!(parentEl.compareDocumentPosition(el) & 16);
				}
				var prEl = el.parentNode;
				while(prEl && prEl != container) {
					if (prEl == parentEl)
						return true;
					prEl = prEl.parentNode;
				}
				return false;
			},
			getViewport = function () {
				var m = document.compatMode == 'CSS1Compat';
				return {
					l : window.pageXOffset || (m ? document.documentElement.scrollLeft : document.body.scrollLeft),
					t : window.pageYOffset || (m ? document.documentElement.scrollTop : document.body.scrollTop),
					w : window.innerWidth || (m ? document.documentElement.clientWidth : document.body.clientWidth),
					h : window.innerHeight || (m ? document.documentElement.clientHeight : document.body.clientHeight)
				};
			},
			fixHSB = function (hsb) {
				return {
					h: Math.min(360, Math.max(0, hsb.h)),
					s: Math.min(100, Math.max(0, hsb.s)),
					b: Math.min(100, Math.max(0, hsb.b))
				};
			}, 
			fixRGB = function (rgb) {
				return {
					r: Math.min(255, Math.max(0, rgb.r)),
					g: Math.min(255, Math.max(0, rgb.g)),
					b: Math.min(255, Math.max(0, rgb.b))
				};
			},

			fixHex = function (hex) {
				var len = 6 - hex.length;
				if (len > 0) {
					var o = [];
					for (var i=0; i<len; i++) {
						o.push('0');
					}
					o.push(hex);
					hex = o.join('');
				}
				return hex;
			}, 
			HexToRGB = function (hex) {
				var hex = parseInt(((hex.indexOf('#') > -1) ? hex.substring(1) : hex), 16);
				return {r: hex >> 16, g: (hex & 0x00FF00) >> 8, b: (hex & 0x0000FF)};
			},
			HexToHSB = function (hex) {
				return RGBToHSB(HexToRGB(hex));
			},
			RGBToHSB = function (rgb) {
				var hsb = {
					h: 0,
					s: 0,
					b: 0
				};
				var min = Math.min(rgb.r, rgb.g, rgb.b);
				var max = Math.max(rgb.r, rgb.g, rgb.b);
				var delta = max - min;
				hsb.b = max;
				if (max != 0) {
					
				}
				hsb.s = max != 0 ? 255 * delta / max : 0;
				if (hsb.s != 0) {
					if (rgb.r == max) {
						hsb.h = (rgb.g - rgb.b) / delta;
					} else if (rgb.g == max) {
						hsb.h = 2 + (rgb.b - rgb.r) / delta;
					} else {
						hsb.h = 4 + (rgb.r - rgb.g) / delta;
					}
				} else {
					hsb.h = -1;
				}
				hsb.h *= 60;
				if (hsb.h < 0) {
					hsb.h += 360;
				}
				hsb.s *= 100/255;
				hsb.b *= 100/255;
				return hsb;
			},
			HSBToRGB = function (hsb) {
				var rgb = {};
				var h = Math.round(hsb.h);
				var s = Math.round(hsb.s*255/100);
				var v = Math.round(hsb.b*255/100);
				if(s == 0) {
					rgb.r = rgb.g = rgb.b = v;
				} else {
					var t1 = v;
					var t2 = (255-s)*v/255;
					var t3 = (t1-t2)*(h%60)/60;
					if(h==360) h = 0;
					if(h<60) {rgb.r=t1;	rgb.b=t2; rgb.g=t2+t3}
					else if(h<120) {rgb.g=t1; rgb.b=t2;	rgb.r=t1-t3}
					else if(h<180) {rgb.g=t1; rgb.r=t2;	rgb.b=t2+t3}
					else if(h<240) {rgb.b=t1; rgb.r=t2;	rgb.g=t1-t3}
					else if(h<300) {rgb.b=t1; rgb.g=t2;	rgb.r=t2+t3}
					else if(h<360) {rgb.r=t1; rgb.g=t2;	rgb.b=t1-t3}
					else {rgb.r=0; rgb.g=0;	rgb.b=0}
				}
				return {r:Math.round(rgb.r), g:Math.round(rgb.g), b:Math.round(rgb.b)};
			},
			RGBToHex = function (rgb) {
				var hex = [
					rgb.r.toString(16),
					rgb.g.toString(16),
					rgb.b.toString(16)
				];
				$.each(hex, function (nr, val) {
					if (val.length == 1) {
						hex[nr] = '0' + val;
					}
				});
				return hex.join('');
			},
			HSBToHex = function (hsb) {
				return RGBToHex(HSBToRGB(hsb));
			},
			restoreOriginal = function () {
				var cal = $(this).parent();
				var col = cal.data('colorpicker').origColor;
				cal.data('colorpicker').color = col;
				fillRGBFields(col, cal.get(0));
				fillHexFields(col, cal.get(0));
				fillHSBFields(col, cal.get(0));
				setSelector(col, cal.get(0));
				setHue(col, cal.get(0));
				setNewColor(col, cal.get(0));
            }; 

        return {
            showPicker: function () {
                return this.each(function () {
                    if ($(this).data('colorpickerId')) {
                        show.apply(this);
                    }
                });
            },
            hidePicker: function () {
                return this.each(function () {
                    if ($(this).data('colorpickerId')) {
                        var cal = $('#' + $(this).data('colorpickerId'));
                        cal.hide();
                        $(document).unbind('mousedown', cal.data( 'document_hide' ) );
                    }
                });
            },
			init: function (opt) {
				opt = $.extend({}, defaults, opt||{});

				if (typeof opt.color == 'string') {
					opt.color = HexToHSB(opt.color);
				} else if (opt.color.r != undefined && opt.color.g != undefined && opt.color.b != undefined) {
					opt.color = RGBToHSB(opt.color);
				} else if (opt.color.h != undefined && opt.color.s != undefined && opt.color.b != undefined) {
					opt.color = fixHSB(opt.color);
				} else {
					return this;
				}
				return this.each(function () {
					if (!$(this).data('colorpickerId')) {
						var options = $.extend({}, opt);
						options.origColor = opt.color;
						var id = 'collorpicker_' + parseInt(Math.random() * 1000);
						$(this).data('colorpickerId', id);
						var cal = $(tpl).attr('id', id);
						if (options.flat) {
							cal.appendTo(this).show();
						} else {
							cal.appendTo(document.body);
						}
						options.fields = cal.find('input')
												.bind('keyup', keyDown)
												.bind('change', change)
												.bind('blur', blur)
												.bind('focus', focus);
						cal
							.find('span').bind('mousedown', downIncrement).end()
							.find('>div.colorpicker_current_color').bind('click', restoreOriginal);

						cal.find('.colorpicker_chip')
                                .click( chipClick )
                                .contextmenu( chipSet );

						options.selector = cal.find('div.colorpicker_color').bind('mousedown', downSelector);
						options.selectorIndic = options.selector.find('div div');
						options.el = this;
						options.hue = cal.find('div.colorpicker_hue div');
						cal.find('div.colorpicker_hue').bind('mousedown', downHue);
						options.newColor = cal.find('div.colorpicker_new_color');
						options.currentColor = cal.find('div.colorpicker_current_color');
						cal.data('colorpicker', options);
						cal.find('div.colorpicker_submit')
							.bind('mouseenter', enterSubmit)
							.bind('mouseleave', leaveSubmit)
							.bind('click', clickSubmit);
						fillRGBFields(options.color, cal.get(0));
						fillHSBFields(options.color, cal.get(0));
						fillHexFields(options.color, cal.get(0));
						setHue(options.color, cal.get(0));
						setSelector(options.color, cal.get(0));
						setCurrentColor(options.color, cal.get(0));
						setNewColor(options.color, cal.get(0));

						if (options.flat) {
							cal.css({
								position: 'relative',
								display: 'block'
							});
						} else {
							$(this).bind(options.eventName, show);
						}
					}
				});
			},
			setColor: function(col) {
				if (typeof col == 'string') {
					col = HexToHSB(col);
				} else if (col.r != undefined && col.g != undefined && col.b != undefined) {
					col = RGBToHSB(col);
				} else if (col.h != undefined && col.s != undefined && col.b != undefined) {
					col = fixHSB(col);
				} else {
					return this;
				}

                // Can be called on the colorpicker div or the launcher
                var cal = ( $(this).hasClass( "colorpicker" ) )? $(this) : $('#' + $(this).data('colorpickerId'));

                if ( cal != null && cal.length > 0 ) {
					cal.data('colorpicker').color = col;
					cal.data('colorpicker').origColor = col;
					fillRGBFields(col, cal.get(0));
					fillHSBFields(col, cal.get(0));
					fillHexFields(col, cal.get(0));
					setHue(col, cal.get(0));
					setSelector(col, cal.get(0));
					setCurrentColor(col, cal.get(0));
					setNewColor(col, cal.get(0));
				}
			},

			setAnchor: function(anchor) {
                // Can be called on the colorpicker div or the launcher
                var cal = ( $(this).hasClass( "colorpicker" ) )? $(this) : $('#' + $(this).data('colorpickerId'));

                if ( cal != null && cal.length > 0 ) {
					cal.data('colorpicker').anchor = anchor;
				}
			},

			setColorChips: function( chip_colors ) {
                // Can be called on the colorpicker div or the launcher
                var cal = ( $(this).hasClass( "colorpicker" ) )? $(this) : $('#' + $(this).data('colorpickerId'));

                if ( cal != null && cal.length > 0 ) {
                    for ( var i=0; i < 16; i++ ) {
                        var chip = cal.find( ".colorpicker_chip" + (i+1) );
                        if ( chip.length == 1 && chip_colors.length > i )
                            chip.css( 'background-color', chip_colors[i] ).show();
                        else
                            chip.hide();
                    }

                    var palette_codes = [ "#010101","#010102","#010103","#010104","#010201","#010202","#010203","#010204" ];
                    for ( var i=0; i < palette_codes.length; i++ ) {
                        var chip = cal.find( ".colorpicker_palette" + (i+1) );
                        updateColorChip( chip, palette_codes[i], 1 );
                    }
                }
            }
		};
	}();
	$.fn.extend({
		ColorPicker: ColorPicker.init,
		ColorPickerHide: ColorPicker.hidePicker,
		ColorPickerShow: ColorPicker.showPicker,
		ColorPickerSetColor: ColorPicker.setColor,
		ColorPickerSetColorChips: ColorPicker.setColorChips,
		ColorPickerSetAnchor: ColorPicker.setAnchor
	});
})(jQuery)