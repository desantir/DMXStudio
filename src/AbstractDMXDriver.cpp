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


#include "DMXStudio.h"
#include "AbstractDMXDriver.h"

// ----------------------------------------------------------------------------
//
AbstractDMXDriver::AbstractDMXDriver(void) :
    Threadable( "AbstractDMXDriver" ),
    m_blackout( false ),
    m_latch( false ),
    m_packet_delay( DEFAULT_PACKET_DELAY_MS ),
    m_packet_min_delay( DEFAULT_MINIMUM_DELAY_MS ),
    m_debug( false )
{
    memset( m_packet, 0 , sizeof( m_packet ) );
    memset( m_pending_packet, 0 , sizeof( m_pending_packet ) );
    memset( m_blackout_packet, 0, sizeof( m_blackout_packet ) );
}

// ----------------------------------------------------------------------------
//
AbstractDMXDriver::~AbstractDMXDriver(void)
{
}

// ----------------------------------------------------------------------------
// Start DMX communications by connecting the interface and beginning transmission
//
DMX_STATUS AbstractDMXDriver::start( const char * connection_info ) {
    if ( isRunning() )
        return DMX_RUNNING;

    // Connect
    m_connection_info = connection_info;

    DMXStudio::log_status( "Starting DMX driver [%s]", m_connection_info );

    DMX_STATUS status = dmx_open( m_connection_info );
    if ( status != DMX_OK )
        return status;

    return startThread() ? DMX_OK : DMX_ERROR;
}

// ----------------------------------------------------------------------------
// Stop the thread and DMX interface
//
DMX_STATUS AbstractDMXDriver::stop() {
    if ( !stopThread() )
        return DMX_ERROR;

    // Disconnect interface
    return dmx_close();
}

// ----------------------------------------------------------------------------
//
UINT AbstractDMXDriver::run(void) {
    DMXStudio::log_status( "DMX driver started [%s]", m_connection_info );

    while ( isRunning() ) {
        DMX_STATUS status;
        
        status = dmx_send( DMX_PACKET_SIZE+1, m_blackout ? m_blackout_packet : m_packet );
        if ( status != DMX_OK ) {
            DMXStudio::log( "DMX send error %d", status );
        }
        
        DWORD next_frame = GetTickCount() + getPacketDelayMS();

        if ( m_debug ) {
            CString buffer;
            for ( channel_t chan=1; chan <= DMX_PACKET_SIZE; chan++ ) {
                buffer.Format( "%03d=%02x ", chan, m_packet[chan] );
                if ( chan % 16 == 0 ) {
                    DMXStudio::log( buffer );
                    buffer.Empty();
                }
            }
            if ( buffer.GetLength() != 0 )
                DMXStudio::log( buffer );
        }

        Sleep( m_packet_min_delay );		                // Minimal sleep time

        // MTBP - Mark time between packets
        long sleep_ms = next_frame - GetTickCount();

        if ( ::WaitForSingleObject( m_wake.m_hObject, sleep_ms ) == WAIT_OBJECT_0 ) {
            if ( m_latch ) {
                memcpy( m_packet, m_pending_packet, sizeof(m_packet) );
                m_latch = false;
            }
        }
    }

    DMXStudio::log_status( "DMX driver stopped [%s]", m_connection_info );

    AfxEndThread( DMX_OK );

    return DMX_OK;
}

// Write a single channel value to the pending packet (valid channel ID 1-512)
//
DMX_STATUS AbstractDMXDriver::write( channel_t channel, BYTE value, bool update ) {
    CSingleLock lock( &m_write_mutex, TRUE );

    if ( channel < 1 || channel > DMX_PACKET_SIZE )
        return DMX_BAD_CHANNEL;

    m_pending_packet[ channel ] = value;

    if ( update ) {			// Don't bother latching in a single byte
        m_packet[ channel ] = value;
        m_wake.SetEvent();
    }

    return DMX_OK;
}

// Read a single channel value from running values (valid channel ID 1-512)
//
DMX_STATUS AbstractDMXDriver::read( channel_t channel, bool pending, BYTE& channel_value ) {
    if ( channel < 1 || channel > DMX_PACKET_SIZE )
        return DMX_ERROR;

    CSingleLock lock( &m_write_mutex, TRUE );

    channel_value = ( pending ) ? m_pending_packet[ channel ] : m_packet[ channel ];

    return DMX_OK;
}

// Write all 512 channel values to the pending packet and latch
//
DMX_STATUS AbstractDMXDriver::write_all( BYTE *dmx_512 ) {
    CSingleLock lock( &m_write_mutex, TRUE );

    memcpy( &m_pending_packet[1], dmx_512, DMX_PACKET_SIZE );

    return latch();
}

// Read all current 512 channel values
//
DMX_STATUS AbstractDMXDriver::read_all( BYTE *dmx_512 ) {
    CSingleLock lock( &m_write_mutex, TRUE );
    memcpy( dmx_512, &m_pending_packet[1], DMX_PACKET_SIZE );
    return DMX_OK;
}

DMX_STATUS AbstractDMXDriver::latch() {
    m_latch = true;

    m_wake.SetEvent();

    const unsigned sleep_ms = 1;

    int max_wait = 5000 / sleep_ms;					// Max wait 5 seconds

    while ( m_latch && max_wait-- ) {
        Sleep( sleep_ms );
    }

    return m_latch ? DMX_ERROR : DMX_OK;
}
