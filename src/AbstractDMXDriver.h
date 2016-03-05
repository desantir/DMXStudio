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

/**
    DMX driver base class
*/

#include "DMXStudio.h"
#include "Threadable.h"

#include <atomic>

#define DEFAULT_PACKET_DELAY_MS			100
#define DEFAULT_MINIMUM_DELAY_MS        5

// DMX status codes
typedef enum dmx_status {
    DMX_OK = 0,
    DMX_ERROR = 1,
    DMX_RUNNING = 2,
    DMX_BAD_CHANNEL = 3,
    DMX_SEND_ERROR = 4,
    DMX_NO_CONNECTION = 5,
    DMX_OPEN_FAILURE = 6,
    DMX_NO_DEVICE = 7,
    DMX_NOT_RUNNING = 8,
    DMX_RECEIVE_ERROR = 9
} DMX_STATUS;

class AbstractDMXDriver
{
    universe_t m_universe_id;                           // Universe ID (purely informational)

    unsigned m_packet_delay;							// Delay between packets
    unsigned m_packet_min_delay;                        // Minimum time between packets
    CString m_connection_info;							// Current connection information

    BYTE m_blackout_packet[DMX_PACKET_SIZE + 1];		// Blackout DMX packet
    BYTE m_dmx_packet[DMX_PACKET_SIZE + 1];			// New DMX packet (staging)

    bool m_blackout;									// Universe is backed out

protected:
    bool m_debug;										// Output packet contents

private:
    CCriticalSection m_write_mutex;						// Write mutex

    AbstractDMXDriver(AbstractDMXDriver& other) {}
    AbstractDMXDriver& operator=(AbstractDMXDriver& rhs) { return *this; }

public:
    AbstractDMXDriver( universe_t universe_id );
    virtual ~AbstractDMXDriver(void);

    DMX_STATUS start();
    DMX_STATUS stop();

    DMX_STATUS write( channel_t channel, BYTE value );
    DMX_STATUS write_all( BYTE *dmx_512 );

    DMX_STATUS read( channel_t channel, BYTE& channel_value );
    DMX_STATUS read_all( BYTE *dmx_512 );

    inline boolean is_running() {
        return dmx_is_running();
    }

    void setBlackout( bool blackout ) {
        if ( m_blackout != blackout ) {
            m_blackout = blackout;
            sendPacket();
        }
    }

    bool isBlackout( ) const {
        return m_blackout;
    }

    inline unsigned getPacketDelayMS() const {
        return m_packet_delay;
    }
    void setPacketDelayMS( unsigned delay ) {
        m_packet_delay = delay;
    }

    inline unsigned getMinimumDelayMS( ) const {
        return m_packet_min_delay;
    }
    void setMinimumDelayMS( unsigned packet_min_delay ) {
        m_packet_min_delay = packet_min_delay;
    }

    inline universe_t getId() const {
        return m_universe_id;
    }

    inline LPCSTR getConnectionInfo() const {
        return m_connection_info;
    }
    void setConnectionInfo( LPCSTR connection_info ) {
        m_connection_info = connection_info;
    }

    inline DMX_STATUS AbstractDMXDriver::sendPacket(void) {
        return dmx_send( m_blackout ? m_blackout_packet : m_dmx_packet );
    }

protected:
    virtual CString dmx_name() = 0;
    virtual DMX_STATUS dmx_send( BYTE* packet ) = 0;
    virtual DMX_STATUS dmx_open(void) = 0;
    virtual DMX_STATUS dmx_close( void ) = 0;
    virtual boolean dmx_is_running(void) = 0;
};

