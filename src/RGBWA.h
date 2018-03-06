/* 
Copyright (C) 2014-17 Robert DeSantis
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

#include "stdafx.h"

enum ColorChannels {
    RED_INDEX=0,
    GREEN_INDEX=1,
    BLUE_INDEX=2,

    // NOTE: Only RGB values will be persited when the RGBWA object is saved
    // as we currently don't allow user entry of the other color channels

    WHITE_INDEX=3,
    AMBER_INDEX=4,
    UV_INDEX=5,
    COLOR_CHANNELS
};

#if COLOR_CHANNELS > 8
#error RGBWA size exceeds the size of a ULONG64
#endif

class RGBWA;

typedef std::vector<RGBWA> RGBWAArray;
typedef std::vector<double> ColorWeights;

struct HSV {
    double h;       // hue: angle in degrees 0-360
    double s;       // saturation: percent
    double v;       // value: percent

    HSV() {}

    HSV( double hue, double sat, double value ) :
        h( hue ),
        s( sat ),
        v( value )
    {}
};

class RGBWA {
    BYTE    m_rgbwa[sizeof(ULONG64)];
    
public:
    RGBWA() {
        *(ULONG64 *)m_rgbwa = 0;
    }

    RGBWA( BYTE* rgbwa ) {
        *(ULONG64 *)m_rgbwa = 0;
        memcpy( m_rgbwa, rgbwa, COLOR_CHANNELS );
    }

    RGBWA( BYTE red, BYTE green, BYTE blue, BYTE white=0, BYTE amber=0, BYTE uv=0 ) {
        *(ULONG64 *)m_rgbwa = 0;

        m_rgbwa[RED_INDEX] = red;
        m_rgbwa[GREEN_INDEX] = green;
        m_rgbwa[BLUE_INDEX] = blue;
        m_rgbwa[WHITE_INDEX] = white;
        m_rgbwa[AMBER_INDEX] = amber;
        m_rgbwa[UV_INDEX] = uv;
    }

    RGBWA( ULONG color ) {
        *(ULONG64 *)m_rgbwa = 0;

        red( (color >> 16) & 0xFF );
        green( (color >> 8) & 0xFF );
        blue( color & 0xFF );
        white( 0 );
        amber( 0 );
        uv(0);
    }

    RGBWA( HSV hsv ) {
        *(ULONG64 *)m_rgbwa = 0;

        convertFromHSV( hsv );
        white( 0 );
        amber( 0 );
        uv(0);
    }

    static bool getColorByName( LPCSTR color_name, RGBWA& rgb );
    static LPCSTR findColorName( RGBWA rgb );
    static std::vector<CString> getAllColorNames( );
    static void getAllPredefinedColors( RGBWAArray& dest, bool include_black=false );
    static void resolveColor( const RGBWA& rgbwa, RGBWAArray& color_list );
    static RGBWAArray toRGBWAArray( const RGBWA* array, size_t size );

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
        return *(ULONG64 *)m_rgbwa == *(ULONG64 *)other.m_rgbwa;
    }

	inline bool isBlack( ) const {
        return *(ULONG64 *)m_rgbwa == 0;
	}

    inline void adjustChannel( size_t channel_number, unsigned amount ) {
        if ( channel_number < ColorChannels::COLOR_CHANNELS )
            m_rgbwa[channel_number] = (BYTE)((unsigned)m_rgbwa[channel_number] + amount );
    }

    inline BYTE red() const { return m_rgbwa[RED_INDEX]; }
    inline BYTE green() const { return m_rgbwa[GREEN_INDEX]; }
    inline BYTE blue() const { return m_rgbwa[BLUE_INDEX]; }
    inline BYTE white() const { return m_rgbwa[WHITE_INDEX]; }
    inline BYTE amber() const { return m_rgbwa[AMBER_INDEX]; }
    inline BYTE uv() const { return m_rgbwa[UV_INDEX]; }

    inline void red( BYTE value ) { m_rgbwa[RED_INDEX] = value; }
    inline void green( BYTE value ) { m_rgbwa[GREEN_INDEX] = value; }
    inline void blue( BYTE value ) { m_rgbwa[BLUE_INDEX] = value; }
    inline void white( BYTE value ) { m_rgbwa[WHITE_INDEX] = value; }
    inline void amber( BYTE value ) { m_rgbwa[AMBER_INDEX] = value; }
    inline void uv( BYTE value ) { m_rgbwa[UV_INDEX] = value; }

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
    CString asString( );

    HSV toHSV( );

	inline bool isPalette() const {
		return red() == 1 && green() >= 1 && green() <= 2 && blue() >= 1 && blue() <= 4 && white() == 0 && amber() == 0 && uv() == 0;
	}

private:
    void convertFromHSV( HSV& hsv );
    
    inline void setRGB( BYTE red, BYTE green, BYTE blue ) { 
        m_rgbwa[RED_INDEX] = red;
        m_rgbwa[GREEN_INDEX] = green;
        m_rgbwa[BLUE_INDEX] = blue;
        m_rgbwa[WHITE_INDEX] = 0;
        m_rgbwa[AMBER_INDEX] = 0;
        m_rgbwa[UV_INDEX] = 0;
    }
};

#define SYSTEM_PALETTE_1    RGBWA(1,1,1)
#define SYSTEM_PALETTE_2    RGBWA(1,1,2)
#define SYSTEM_PALETTE_3    RGBWA(1,1,3)
#define SYSTEM_PALETTE_4    RGBWA(1,1,4)

#define USER_PALETTE_1      RGBWA(1,2,1)
#define USER_PALETTE_2      RGBWA(1,2,2)
#define USER_PALETTE_3      RGBWA(1,2,3)
#define USER_PALETTE_4      RGBWA(1,2,4)

extern const RGBWA GLOBAL_PALETTE_DEFAULT_COLORS[16];
extern const RGBWA RAINBOW_PALETTE_DEFAULT_COLORS[252];