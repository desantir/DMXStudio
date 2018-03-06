/* 
Copyright (C) 2017 Robert DeSantis
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
#include "FixtureDefinition.h"

class FixtureState {
    UID             m_fixture_uid;
    PixelArray*     m_pixel_array;
    Channel*        m_dimmer_channel;
    Channel*        m_strobe_channel;
	channel_address m_channel_base;
    bool            m_strobing;

    // State
    RGBWAArray      m_colors;

public:
    FixtureState() :
        m_fixture_uid( NOUID ),
        m_strobing( false ),
        m_pixel_array( NULL ),
        m_dimmer_channel( NULL ),
        m_strobe_channel( NULL ),
        m_channel_base( 0 )
    {};

    FixtureState( Fixture* pf );

    RGBWAArray& colors( ) {
        return m_colors;
    }

   void colors( RGBWAArray& new_colors ) {
        m_colors = new_colors;
    }

    PixelArray* getPixels( ) {
        return m_pixel_array;
    }

    Channel* getDimmerChannel( ) const {
        return m_dimmer_channel;
    }

    Channel* getStrobeChannel( ) const {
        return m_strobe_channel;
    }

    inline channel_address getPacketAddress( channel_address offset ) const {
        return m_channel_base + offset;
    }

    inline UID getFixtureUID( ) const {
        return m_fixture_uid;
    }

    inline bool isStrobing() const {
        return m_strobing;
    } 

    inline void setStrobing( bool strobing ) {
        m_strobing = strobing;
    }

};

typedef std::unordered_map<UID, FixtureState> FixtureStateMap;
