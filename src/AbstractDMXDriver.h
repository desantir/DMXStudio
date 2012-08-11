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
    DMX_NOT_RUNNING = 8
} DMX_STATUS;

class AbstractDMXDriver : public Threadable
{
    BYTE m_packet[ DMX_PACKET_SIZE+1 ];					// Current DMX packet
    BYTE m_blackout_packet[ DMX_PACKET_SIZE+1 ];		// Blackout DMX packet

    BYTE m_pending_packet[ DMX_PACKET_SIZE+1 ];			// New DMX packet (staging)
    volatile bool m_latch;								// Latch DMX packet
    unsigned m_packet_delay;							// Delay between packets
    unsigned m_packet_min_delay;                        // Minimum time between packets

    bool m_blackout;									// Universe is backed out
    bool m_debug;										// Output packet contents
    CString m_connection_info;							// Current connection information

    CCriticalSection m_write_mutex;						// Write mutex
    CEvent m_wake;										// Wake up DMX transmitter

    UINT run(void);

    AbstractDMXDriver(AbstractDMXDriver& other) {}
    AbstractDMXDriver& operator=(AbstractDMXDriver& rhs) { return *this; }

public:
    AbstractDMXDriver(void);
    virtual ~AbstractDMXDriver(void);

    DMX_STATUS start( const char * connection_info );
    DMX_STATUS stop();

    DMX_STATUS write( channel_t channel, BYTE value, bool update=false );
    DMX_STATUS write_all( BYTE *dmx_512 );
    DMX_STATUS latch();
    DMX_STATUS read( channel_t channel, bool pending, BYTE& channel_value );
    BYTE read( channel_t channel, bool pending=false );
    DMX_STATUS read_all( BYTE *dmx_512 );

    void setBlackout( bool blackout ) {
        m_blackout = blackout;
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

    inline unsigned getMinimumSleepMS( ) const {
        return m_packet_min_delay;
    }
    void setMinimumDelayMS( unsigned packet_min_delay ) {
        m_packet_min_delay = packet_min_delay;
    }

protected:
    /**
        Send a packet frame consiting of a break and the packet supplied.  The
        supplied packet include the 0 command byte with at least 24 bytes of data
        to a maximum of 513 total bytes (command + 512 data).
    */
    virtual DMX_STATUS dmx_send( unsigned length, BYTE * packet ) = 0;

    virtual DMX_STATUS dmx_open( const char * connection_info ) = 0;

    virtual DMX_STATUS dmx_close( void ) = 0;
};

