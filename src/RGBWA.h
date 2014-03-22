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

#pragma once

#include "DMXStudio.h"

enum ColorChannels {
    RED_INDEX=0,
    GREEN_INDEX=1,
    BLUE_INDEX=2,
    WHITE_INDEX=3,
    AMBER_INDEX=4,
    COLOR_CHANNELS
};

class RGBWA {
    BYTE    m_rgbwa[COLOR_CHANNELS];
    
public:
    RGBWA() {
        memset( m_rgbwa, 0, COLOR_CHANNELS );
    }

    RGBWA( BYTE* rgbwa ) {
        memcpy( m_rgbwa, rgbwa, COLOR_CHANNELS );
    }

    RGBWA( BYTE red, BYTE green, BYTE blue, BYTE white=0, BYTE amber=0 ) {
        m_rgbwa[RED_INDEX] = red;
        m_rgbwa[GREEN_INDEX] = green;
        m_rgbwa[BLUE_INDEX] = blue;
        m_rgbwa[WHITE_INDEX] = white;
        m_rgbwa[AMBER_INDEX] = amber;
    }

    RGBWA( ULONG color ) {
        red( (color >> 16) & 0xFF );
        green( (color >> 8) & 0xFF );
        blue( color & 0xFF );
        white( 0 );
        amber( 0 );
    }

    static bool getColorByName( LPCSTR color_name, RGBWA& rgb );
    static LPCSTR findColorName( RGBWA rgb );
    static std::vector<CString> getAllColorNames( );
    static void getAllPredefinedColors( std::vector<RGBWA>& dest, bool include_black=false );

    inline RGBWA getColor() {
        return RGBWA( m_rgbwa );
    }

    inline BYTE& operator[]( size_t index ) {
        return m_rgbwa[ index ];
    }

    inline const BYTE& operator[]( size_t index ) const {
        return m_rgbwa[ index ];
    }

    inline operator unsigned long() const {
        return ((ULONG)red()) << 16 |
               ((ULONG)green()) << 8 |
               ((ULONG)blue());
    }

    inline bool operator==( RGBWA& other ) const {
        for ( size_t index=0; index < COLOR_CHANNELS; index++ )
            if ( m_rgbwa[index] != other.m_rgbwa[index] )
                return false;
        return true;
    }

    inline BYTE red() const { return m_rgbwa[RED_INDEX]; }
    inline BYTE green() const { return m_rgbwa[GREEN_INDEX]; }
    inline BYTE blue() const { return m_rgbwa[BLUE_INDEX]; }
    inline BYTE white() const { return m_rgbwa[WHITE_INDEX]; }
    inline BYTE amber() const { return m_rgbwa[AMBER_INDEX]; }

    inline void red( BYTE value ) { m_rgbwa[RED_INDEX] = value; }
    inline void green( BYTE value ) { m_rgbwa[GREEN_INDEX] = value; }
    inline void blue( BYTE value ) { m_rgbwa[BLUE_INDEX] = value; }
    inline void white( BYTE value ) { m_rgbwa[WHITE_INDEX] = value; }
    inline void amber( BYTE value ) { m_rgbwa[AMBER_INDEX] = value; }

    static RGBWA BLACK;
    static RGBWA WHITE;
    static RGBWA RED;
    static RGBWA BLUE;
    static RGBWA YELLOW;
    static RGBWA GREEN;
    static RGBWA ORANGE;
    static RGBWA TEAL;
    static RGBWA CYAN;
    static RGBWA MAGENTA;
    static RGBWA VIOLET;
    static RGBWA BLOOD_ORANGE;
    static RGBWA LIGHT_BLUE;
    static RGBWA SALMON;
    static RGBWA HOT_PINK;
    static RGBWA LIME;

    CString getColorName( );
};

typedef std::vector<RGBWA> RGBWAArray;