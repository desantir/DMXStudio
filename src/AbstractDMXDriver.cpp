/* 
Copyright (C) 2011-16 Robert DeSantis
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
AbstractDMXDriver::AbstractDMXDriver( universe_t universe_id ) :
    m_universe_id( universe_id ),
    m_blackout( false ),
    m_packet_delay( DEFAULT_PACKET_DELAY_MS ),
    m_packet_min_delay( DEFAULT_MINIMUM_DELAY_MS ),
    m_debug( false )
{
    memset( m_dmx_packet, 0 , sizeof( m_dmx_packet ) );
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
DMX_STATUS AbstractDMXDriver::start() {
    DMXStudio::log_status("Starting DMX universe %d [%s]", m_universe_id, (LPCSTR)dmx_name() );

    if ( dmx_is_running() )
        return DMX_RUNNING;

    // Connect
    return dmx_open();
}

// ----------------------------------------------------------------------------
// Stop the DMX interface
//
DMX_STATUS AbstractDMXDriver::stop() {
    return dmx_close();
}

// ----------------------------------------------------------------------------
// Write all 512 channel values to the pending packet and latch
//
DMX_STATUS AbstractDMXDriver::write_all(channel_value *dmx_512) {
    CSingleLock lock(&m_write_mutex, TRUE);

    memcpy(&m_dmx_packet[1], dmx_512, DMX_PACKET_SIZE);

    return sendPacket();
}

// ----------------------------------------------------------------------------
// Write a single channel value to the pending packet (valid channel ID 1-512)
//
DMX_STATUS AbstractDMXDriver::write( channel_address channel, channel_value value ) {
    CSingleLock lock( &m_write_mutex, TRUE );

    if ( channel < 1 || channel > DMX_PACKET_SIZE )
        return DMX_BAD_CHANNEL;

    m_dmx_packet[ channel ] = value;
    
    return sendPacket();
}

// ----------------------------------------------------------------------------
// Read a single channel value from running values (valid channel ID 1-512)
//
DMX_STATUS AbstractDMXDriver::read( channel_address channel, channel_value& channel_value ) {
    if ( channel < 1 || channel > DMX_PACKET_SIZE )
        return DMX_ERROR;

    CSingleLock lock( &m_write_mutex, TRUE );

    channel_value = m_dmx_packet[ channel ];

    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Read all current 512 channel values
//
DMX_STATUS AbstractDMXDriver::read_all( channel_value *dmx_512 ) {
    CSingleLock lock( &m_write_mutex, TRUE );
    memcpy( dmx_512, &m_dmx_packet[1], DMX_PACKET_SIZE );
    return DMX_OK;
}
