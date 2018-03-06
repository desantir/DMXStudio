/* 
Copyright (C) 2014-2017 Robert DeSantis
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

#include "DMXStudio.h"
#include "RGBWA.h"
#include "Venue.h"

struct InsensitiveComparator {
    bool operator()(const CString& a, const CString& b) const {
        return a.CompareNoCase( b ) < 0;
    }
};

typedef std::map<CString, RGBWA, InsensitiveComparator> ColorMap;

RGBWA RGBWA::BLACK( 0x00, 0x00, 0x00, 0x00, 0x00 );
RGBWA RGBWA::WHITE( 0xFF, 0xFF, 0xFF, 0xFF, 0x00 );
RGBWA RGBWA::RED( 0xFF, 0x00, 0x00, 0x00, 0x00 );
RGBWA RGBWA::BLUE( 0x00, 0x00, 0xFF, 0x00, 0x00 );
RGBWA RGBWA::YELLOW( 0xFF, 0xFF, 0x00, 0x00, 0x00 );
RGBWA RGBWA::GREEN( 0x00, 0xFF, 0x00, 0x00, 0x00 );
RGBWA RGBWA::ORANGE( 0xFF, 0x40, 0x00, 0x00, 0x00 );
RGBWA RGBWA::TEAL( 0x00, 0xFF, 0x20, 0x00, 0x00 );
RGBWA RGBWA::CYAN( 0x10, 0xFF, 0x7F, 0x00, 0x00 );
RGBWA RGBWA::MAGENTA( 0xFF, 0x00, 0xFF, 0x00, 0x00 );
RGBWA RGBWA::VIOLET( 0x40, 0x00, 0xFF, 0x00, 0x00 );
RGBWA RGBWA::BLOOD_ORANGE( 0xFF, 0x20, 0x00, 0x00, 0x00 );
RGBWA RGBWA::LIGHT_BLUE( 0x00, 0x80, 0x80, 0x00, 0x00 );
RGBWA RGBWA::SALMON( 0xFF, 0x40, 0x20, 0x00, 0x00 );
RGBWA RGBWA::HOT_PINK( 0xFF, 0x00, 0x20, 0x00, 0x00 );
RGBWA RGBWA::LIME( 0x30, 0xFF, 0x05, 0x00, 0x00 );

static ColorMap generate_color_names() {
    ColorMap map;
    map["Black"] = RGBWA::BLACK;
    map["Red"] = RGBWA::RED;
    map["Blood orange"] = RGBWA::BLOOD_ORANGE;
    map["Orange"] = RGBWA::ORANGE;
    map["Salmon"] = RGBWA::SALMON;   
    map["Yellow"] = RGBWA::YELLOW;
    map["Lime"] = RGBWA::LIME;
    map["Green"] = RGBWA::GREEN;
    map["Teal"] = RGBWA::TEAL;
    map["Cyan"] = RGBWA::CYAN;
    map["Light blue"] = RGBWA::LIGHT_BLUE;
    map["Blue"] = RGBWA::BLUE;
    map["Violet"] = RGBWA::VIOLET;
    map["Magenta"] = RGBWA::MAGENTA;
    map["Hot pink"] = RGBWA::HOT_PINK;
    map["White"] = RGBWA::WHITE;

    return map;
}

static ColorMap predefinedColors = generate_color_names();

const RGBWA RAINBOW_PALETTE_DEFAULT_COLORS[252] = {
    0xff4e00, 0xff5400, 0xff5a00, 0xff6000, 0xff6600, 0xff6c00, 0xff7200, 0xff7800, 0xff7e00, 0xff8400, 0xff8a00, 0xff9000, 0xff9600, 0xff9c00, 0xffa200,
    0xffa800, 0xffae00, 0xffcc00, 0xffd200, 0xffd800, 0xffde00, 0xffe400, 0xffea00, 0xfff000, 0xfff600, 0xfffc00, 0xffff00, 0xf9ff00, 0xf3ff00, 0xedff00, 
    0xe7ff00, 0xe1ff00, 0xdbff00, 0xd5ff00, 0xcfff00, 0xc9ff00, 0xc3ff00, 0xbdff00, 0xb7ff00, 0xb1ff00, 0xabff00, 0xa5ff00, 0x9fff00, 0x99ff00, 0x93ff00,
    0x8dff00, 0x87ff00, 0x81ff00, 0x7bff00, 0x75ff00, 0x6fff00, 0x69ff00, 0x63ff00, 0x5dff00, 0x57ff00, 0x51ff00, 0x4bff00, 0x45ff00, 0x3fff00, 0x39ff00, 
    0x33ff00, 0x2dff00, 0x27ff00, 0x21ff00, 0x1bff00, 0x15ff00, 0x0fff00, 0x09ff00, 0x03ff00, 0x00ff00, 0x00ff06, 0x00ff0c, 0x00ff12, 0x00ff18, 0x00ff1e,
    0x00ff24, 0x00ff2a, 0x00ff30, 0x00ff36, 0x00ff3c, 0x00ff42, 0x00ff48, 0x00ff4e, 0x00ff54, 0x00ff5a, 0x00ff60, 0x00ff66, 0x00ff6c, 0x00ff72, 0x00ff78,
    0x00ff7e, 0x00ff84, 0x00ff8a, 0x00ff90, 0x00ff96, 0x00ff9c, 0x00ffa2, 0x00ffa8, 0x00ffae, 0x00ffb4, 0x00ffba, 0x00ffc0, 0x00ffc6, 0x00ffcc, 0x00ffd2,
    0x00ffd8, 0x00ffde, 0x00ffe4, 0x00ffea, 0x00fff0, 0x00fff6, 0x00fffc, 0x00ffff, 0x00fcff, 0x00f6ff, 0x00f0ff, 0x00eaff, 0x00e4ff, 0x00deff, 0x00d8ff, 
    0x00d2ff, 0x00ccff, 0x00c6ff, 0x00c0ff, 0x00baff, 0x00b4ff, 0x00aeff, 0x00a8ff, 0x00a2ff, 0x009cff, 0x0096ff, 0x0090ff, 0x008aff, 0x0084ff, 0x007eff, 
    0x0078ff, 0x0072ff, 0x006cff, 0x0066ff, 0x0060ff, 0x005aff, 0x0054ff, 0x004eff, 0x0048ff, 0x0042ff, 0x003cff, 0x0036ff, 0x0030ff, 0x002aff, 0x0024ff, 
    0x001eff, 0x0018ff, 0x0012ff, 0x000cff, 0x0006ff, 0x0000ff, 0x0300ff, 0x0900ff, 0x0f00ff, 0x1500ff, 0x1b00ff, 0x2100ff, 0x2700ff, 0x2d00ff, 0x3300ff,
    0x3900ff, 0x3f00ff, 0x4500ff, 0x4b00ff, 0x5100ff, 0x5700ff, 0x5d00ff, 0x6300ff, 0x6900ff, 0x6f00ff, 0x7500ff, 0x7b00ff, 0x8100ff, 0x8700ff, 0x8d00ff,
    0x9300ff, 0x9900ff, 0x9f00ff, 0xa500ff, 0xab00ff, 0xb100ff, 0xb700ff, 0xbd00ff, 0xc300ff, 0xc900ff, 0xcf00ff, 0xd500ff, 0xdb00ff, 0xe100ff, 0xe700ff, 
    0xed00ff, 0xf300ff, 0xf900ff, 0xff00ff, 0xff00fc, 0xff00f6, 0xff00f0, 0xff00ea, 0xff00e4, 0xff00de, 0xff00d8, 0xff00d2, 0xff00cc, 0xff00c6, 0xff00c0, 
    0xff00ba, 0xff00b4, 0xff00ae, 0xff00a8, 0xff00a2, 0xff009c, 0xff0096, 0xff0090, 0xff008a, 0xff0084, 0xff007e, 0xff0078, 0xff0072, 0xff006c, 0xff0066, 
    0xff0060, 0xff005a, 0xff0054, 0xff004e, 0xff0048, 0xff0042, 0xff003c, 0xff0036, 0xff0030, 0xff002a, 0xff0024, 0xff001e, 0xff0018, 0xff0012, 0xff0000, 
    0xff0600, 0xff0c00, 0xff1200, 0xff1800, 0xff1e00, 0xff2400, 0xff2a00, 0xff3000, 0xff3600, 0xff3c00, 0xff4200, 0xff4800
};

const RGBWA GLOBAL_PALETTE_DEFAULT_COLORS[16] = {
    RGBWA::WHITE,
    RGBWA::RED,
    RGBWA::GREEN,
    RGBWA::BLUE,
    RGBWA::YELLOW,
    RGBWA( 0x59, 0x00, 0xFF ),
    RGBWA( 0x60, 0x6D, 0xBC ),
    RGBWA( 0x10, 0xFF, 0x7F ),
    RGBWA::MAGENTA,
    RGBWA( 0x0f, 0x00, 0x61 ),
    RGBWA( 0xD4, 0x69, 0x0B ),
    RGBWA( 0x00, 0x80, 0x80 ),
    RGBWA( 0x00, 0xd9, 0xff ),
    RGBWA( 0xab, 0x87, 0xf5 ),
    RGBWA( 0xf5, 0x93, 0x3d ), 
    RGBWA( 0xb5, 0xe6, 0x12 )
};

// ----------------------------------------------------------------------------
//
bool RGBWA::getColorByName( LPCSTR color_name, RGBWA& rgb ) {
    ColorMap::iterator it = predefinedColors.find( color_name );
    if ( it == predefinedColors.end() )
        return false;

    rgb = (*it).second;
    return true;
}

// ----------------------------------------------------------------------------
//
LPCSTR RGBWA::findColorName( RGBWA rgb ) {
    for ( std::pair<CString, RGBWA> entry : predefinedColors )
        if ( entry.second == rgb )
            return entry.first;
    return NULL;
}

// ----------------------------------------------------------------------------
//
CString RGBWA::getColorName( ) {
    CString name;

    LPCSTR color_name = findColorName( *this );
    if ( color_name )
        name = color_name;
    else
        name.Format( "rgb(%d %d %d)", red(), green(), blue() );
    return name;
}


// ----------------------------------------------------------------------------
//
CString RGBWA::asString( ) {
    CString name;

    for ( size_t i=0; i < COLOR_CHANNELS; i++ )
        name.AppendFormat( "%02x", m_rgbwa[i] );

   return name;
}

// ----------------------------------------------------------------------------
//
std::vector<CString> RGBWA::getAllColorNames( ) {
    std::vector<CString> names;
    for ( std::pair<CString, RGBWA> entry : predefinedColors )
        names.push_back( entry.first );
    return names;
}

// ----------------------------------------------------------------------------
//
void RGBWA::getAllPredefinedColors( RGBWAArray& colors, bool include_black ) {
    for ( std::pair<CString, RGBWA> entry : predefinedColors )
        if ( include_black || entry.second != RGBWA::BLACK ) 
            colors.push_back( entry.second );
}

// ----------------------------------------------------------------------------
//
void RGBWA::resolveColor( const RGBWA& rgbwa, RGBWAArray& color_list ) {
    color_list.clear();

    // Typical case
    if ( rgbwa.red() > 4 || rgbwa.blue() > 4 || rgbwa.green() > 4 ) {
        color_list.push_back( rgbwa );
        return;
    }

    // Possible palette reference
    if ( rgbwa == SYSTEM_PALETTE_1 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_GLOBAL_COLORS , color_list );
    else if ( rgbwa == SYSTEM_PALETTE_2 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_PREDEFINED_COLORS, color_list );
    else if ( rgbwa == SYSTEM_PALETTE_3 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_RAINBOW_COLORS, color_list );
    else if ( rgbwa == SYSTEM_PALETTE_4 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_VIDEO_COLORS, color_list );
    else if ( rgbwa == USER_PALETTE_1 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_USER_1, color_list );
    else if ( rgbwa == USER_PALETTE_2 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_USER_2, color_list );
    else if ( rgbwa == USER_PALETTE_3 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_USER_3, color_list );
    else if ( rgbwa == USER_PALETTE_4 )
        studio.getVenue()->getSystemPalette( SYSTEM_PALETTE_USER_4, color_list );
    else
        color_list.push_back( rgbwa );
}

// ----------------------------------------------------------------------------
//
RGBWAArray RGBWA::toRGBWAArray( const RGBWA* array, size_t size ) {
    RGBWAArray colors;
    for ( size_t index=0; index < size; index++ )
        colors.push_back( *array++ );
    return colors;
}

// ----------------------------------------------------------------------------
//
HSV RGBWA::toHSV( )
{
    HSV         out;

    double min = std::min<BYTE>( red(), std::min<BYTE>( green(), blue() ) );
    double max = std::max<BYTE>( red(), std::max<BYTE>( green(), blue() ) );
    
    double delta = max - min;

    out.v = max / 255.0;                        // v

    if ( max == 0 || delta == 0 ) {             // White-gray-black 
        out.s = 0.0;
        out.h = 0.0;                            
        return out;
    }

    out.s = (delta / max);                        // s

    if ( red() >= max )                           // > is bogus, just keeps compilor happy
        out.h = ( green() - blue() ) / delta;        // between yellow & magenta
    else
        if ( green() >= max )
            out.h = 2.0 + ( blue() - red() ) / delta;  // between cyan & yellow
        else
            out.h = 4.0 + ( red() - green() ) / delta;  // between magenta & cyan

    out.h *= 60.0;                              // degrees

    if ( out.h < 0.0 )
        out.h += 360.0;

    return out;
}

// ----------------------------------------------------------------------------
//
void RGBWA::convertFromHSV( HSV& hsv )
{
    double value = 255.0 * hsv.v;

    if ( hsv.s <= 0.0 ) {
        setRGB( (BYTE)value, (BYTE)value, (BYTE)value );
        return;
    }

    double      hh, p, q, t, ff;

    hh = hsv.h;
    if( hh >= 360.0 ) 
        hh = 0.0;
    hh /= 60.0;

    long i = (long)hh;

    ff = hh - i;

    p = value * (1.0 - hsv.s);
    q = value * (1.0 - (hsv.s * ff));
    t = value * (1.0 - (hsv.s * (1.0 - ff)));

    switch(i) {
        case 0:     setRGB( (BYTE)value, (BYTE)t, (BYTE)p ); break;
        case 1:     setRGB( (BYTE)q, (BYTE)value, (BYTE)p ); break;
        case 2:     setRGB( (BYTE)p, (BYTE)value, (BYTE)t ); break;
        case 3:     setRGB( (BYTE)p, (BYTE)q, (BYTE)value ); break;
        case 4:     setRGB( (BYTE)t, (BYTE)p, (BYTE)value ); break;
        case 5:
        default:    setRGB( (BYTE)value, (BYTE)p, (BYTE)q ); break;
    }
}