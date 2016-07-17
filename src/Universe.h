/* 
Copyright (C) 2015-16 Robert DeSantis
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
#include "IVisitor.h"
#include "AbstractDMXDriver.h"

enum UniverseType {
    OPEN_DMX = 0,
    ENTTEC_USB_PRO = 1
} ;

class Universe {

    friend class VenueWriter;
    friend class VenueReader;

    universe_t              m_id;                               // Universe ID 1 .. n
    UniverseType            m_type;                             // Universe type

    CString			        m_dmx_port;							// DMX port connection information
    unsigned                m_dmx_packet_delay;					// DMX delay between packets
    unsigned                m_dmx_packet_min_delay;             // DMX minimum time between packets
    AbstractDMXDriver*      m_driver;                           // DMX driver

public:
    Universe( ) :
        m_id( 0 ),
        m_type( OPEN_DMX ),
        m_dmx_packet_delay( 0 ),
        m_dmx_packet_min_delay( 0 ), 
        m_driver( NULL )
    {}

    Universe( universe_t id, UniverseType type, LPCSTR port_name, unsigned delay_ms=DEFAULT_PACKET_DELAY_MS, unsigned min_delay_ms=DEFAULT_MINIMUM_DELAY_MS ) :
        m_id( id ),
        m_type( type ),
        m_dmx_port( port_name ),
        m_dmx_packet_delay( delay_ms ),
        m_dmx_packet_min_delay( min_delay_ms ), 
        m_driver( NULL )
    {}

    Universe( Universe& other ) :
        m_id( other.m_id ),
        m_type( other.m_type ),
        m_dmx_port( other.m_dmx_port ),
        m_dmx_packet_delay( other.m_dmx_packet_delay ),
        m_dmx_packet_min_delay( other.m_dmx_packet_min_delay ), 
        m_driver( NULL )
    {}
   
    ~Universe() {
        stop();
    }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    DMX_STATUS start();
    void stop();

    void setBlackout( bool hard_blackout ) {
        if ( m_driver )
            m_driver->setBlackout( hard_blackout );
    }

    inline boolean isRunning() const {
        return m_driver != NULL && m_driver->is_running();
    }

    inline universe_t getId() const {
        return m_id;
    }
    inline void setId( universe_t id ) {
        m_id = id;
    }

    inline UniverseType getType() const {
        return m_type;
    }
    inline void setType( UniverseType type ) {
        m_type = type;
    }

    inline const char* getDmxPort() const {
        return m_dmx_port.GetLength() == 0 ? NULL : (const char *)m_dmx_port;
    }
    inline void setDmxPort( const char* dmx_port ) {
        if ( dmx_port == NULL )
            m_dmx_port.Empty();
        else
            m_dmx_port = dmx_port;
    }

    inline unsigned getDmxPacketDelayMS() const {
        return m_dmx_packet_delay;
    }
    inline void setDmxPacketDelayMS( unsigned delay ) {
        m_dmx_packet_delay = delay;
    }

    inline unsigned getDmxMinimumDelayMS( ) const {
        return m_dmx_packet_min_delay;
    }
    inline void setDmxMinimumDelayMS( unsigned packet_min_delay) {
        m_dmx_packet_min_delay = packet_min_delay;
    }

    inline DMX_STATUS write( channel_t channel, BYTE value ) {
        if ( !m_driver )
            return DMX_NOT_RUNNING;

        return m_driver->write( channel, value );
    }
    
    inline DMX_STATUS write_all( BYTE *dmx_512 ) {
        if ( !m_driver )
            return DMX_NOT_RUNNING;

        return m_driver->write_all( dmx_512 );
    }

    inline DMX_STATUS read( channel_t channel, BYTE& channel_value ) {
        if ( !m_driver )
            return DMX_NOT_RUNNING;

        return m_driver->read( channel, channel_value );
    }

    inline DMX_STATUS read_all( BYTE *dmx_512 ) {
        if ( !m_driver )
            return DMX_NOT_RUNNING;

        return m_driver->read_all( dmx_512 );
    }
};