/* 
Copyright (C) 2011,2012 Robert DeSantis
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

#include "IVisitor.h"
#include "DMXStudio.h"
#include "Fixture.h"

class SceneActor
{
    friend class VenueWriter;
    friend class VenueReader;

    UID			m_pfuid;									// Physical fixture UID
    size_t      m_channels;

    // m_channel_values always contains unmapped fixture channel values. For
    // example, if a fixture maps channel 3 to channel 0, the value for physical
    // channel 3 will be stored in m_channel_values[0].  The value will be 
    // remapped from channel 0 to 3 when added to the DMX packet.

    BYTE		m_channel_values[DMX_PACKET_SIZE];			// Not worth a container for this

public:
    SceneActor(void) :
        m_pfuid(0),
        m_channels(0) { reset_channel_values(); }

    SceneActor( Fixture * );
    ~SceneActor(void);

    void reset_channel_values( void ) {
        memset( m_channel_values, 0, sizeof(m_channel_values) );
    }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    inline UID getFUID() const {
        return m_pfuid;
    }

    inline BYTE getChannelValue( channel_t channel ) const {
        STUDIO_ASSERT( channel < m_channels, "Channel out of range" );
        return m_channel_values[ channel ];
    }

    inline void setChannelValue( channel_t channel, BYTE value ) {
        STUDIO_ASSERT( channel < m_channels, "Channel out of range" );
        m_channel_values[ channel ] = value;
    }

    inline size_t getChannelValues( BYTE * destination ) const {
        memcpy( destination, m_channel_values, m_channels );
        return m_channels;
    }
};

typedef std::map< UID, SceneActor > ActorMap;	
typedef std::vector< SceneActor * > ActorPtrArray;	
