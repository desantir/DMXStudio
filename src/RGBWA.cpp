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

#include "DMXStudio.h"
#include "RGBWA.h"

struct InsensitiveComparator {
    bool operator()(const CString& a, const CString& b) const {
        return a.CompareNoCase( b ) < 0;
    }
};

typedef std::map<CString, RGBWA, InsensitiveComparator> ColorMap;

RGBWA RGBWA::BLACK( 0x00, 0x00, 0x00, 0x00, 0x00 );
RGBWA RGBWA::WHITE( 0xFF, 0xFF, 0xFF, 0x00, 0x00 );
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