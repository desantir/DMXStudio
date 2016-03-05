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
#include "OpenDMXDriver.h"

// ----------------------------------------------------------------------------
//
OpenDMXDriver::OpenDMXDriver( universe_t universe_id ) :
   FTDI_DMXDriver( universe_id ),
   Threadable("OpenDMXDriver")
{
    m_latch.store(false);
    memset(m_packet, 0, sizeof(m_packet));
}

// ----------------------------------------------------------------------------
//
OpenDMXDriver::~OpenDMXDriver(void)
{
}

// ----------------------------------------------------------------------------
//
CString OpenDMXDriver::dmx_name() {
    CString name;
    name.Format( "OPEN DMX on %s", getConnectionInfo() );
    return name;
}

// ----------------------------------------------------------------------------
// Return true if DMX is running
//
boolean OpenDMXDriver::dmx_is_running() {
    return FTDI_DMXDriver::dmx_is_running() && isRunning();
}

// ----------------------------------------------------------------------------
// Open DMX port
//
DMX_STATUS OpenDMXDriver::dmx_open() {

    DMX_STATUS status = FTDI_DMXDriver::dmx_open();
    if (status != DMX_OK)
        return status;

    FT_STATUS res = FT_SetBaudRate(m_fthandle, 250000);
    if (FT_OK != res)
        return DMX_OPEN_FAILURE;

    res = FT_SetDataCharacteristics(m_fthandle, 8, 2, 0);
    if (FT_OK != res)
        return DMX_OPEN_FAILURE;

    return startThread() ? DMX_OK : DMX_ERROR;
}

// ----------------------------------------------------------------------------
// Send current buffer
//
DMX_STATUS OpenDMXDriver::dmx_send( BYTE* packet ) {
    memcpy( m_pending_packet, packet, sizeof(m_packet) );

    if (!isRunning())                              // If not running, assume we are in "disconnected" mode
        return DMX_OK;

    m_latch.store(true);

    m_wake.SetEvent();

    const unsigned sleep_ms = 1;

    int max_wait = 5000 / sleep_ms;					// Max wait 5 seconds

    while (m_latch && max_wait--) {
        Sleep(sleep_ms);
    }

    return m_latch.load() ? DMX_ERROR : DMX_OK;
}

// ----------------------------------------------------------------------------
// Close DMX port
//
DMX_STATUS OpenDMXDriver::dmx_close(void) {
    DMX_STATUS status = FTDI_DMXDriver::dmx_close();

    if ( !stopThread() )
        return DMX_ERROR;

    return status;
}

// ----------------------------------------------------------------------------
//
UINT OpenDMXDriver::run(void) {
    DMXStudio::log_status("DMX universe %d driver started [%s]", getId(), (LPCSTR)dmx_name() );

    while (isRunning()) {
        DMX_STATUS status;

        status = send(DMX_PACKET_SIZE + 1, m_packet);
        if (status != DMX_OK) {
            DMXStudio::log("DMX send error %d", status);
        }

        DWORD next_frame = GetTickCount() + getPacketDelayMS();

        if (m_debug) {
            CString buffer;
            for (channel_t chan = 1; chan <= DMX_PACKET_SIZE; chan++) {
                buffer.Format("%03d=%02x ", chan, m_packet[chan]);
                if (chan % 16 == 0) {
                    DMXStudio::log(buffer);
                    buffer.Empty();
                }
            }
            if (buffer.GetLength() != 0)
                DMXStudio::log(buffer);
        }

        Sleep( getMinimumDelayMS() );		            // Minimal sleep time at least
                                                        // MTBP - Mark time between packets
        long sleep_ms = next_frame - GetTickCount();

        if (sleep_ms < 0)
            sleep_ms = 0;

        if (::WaitForSingleObject(m_wake.m_hObject, sleep_ms) == WAIT_OBJECT_0) {
            if (m_latch.load()) {
                memcpy(m_packet, m_pending_packet, sizeof(m_packet));
                m_latch.store(false);
            }
        }
    }

    DMXStudio::log_status("DMX universe %d driver stopped [%s]", getId(), (LPCSTR)dmx_name() );

    AfxEndThread(DMX_OK);

    return DMX_OK;
}

/**
    Send a packet frame consisting of a break and the packet supplied.  The
    supplied packet include the 0 command byte with at least 24 bytes of data
    to a maximum of 513 total bytes (command + 512 data).
*/
DMX_STATUS OpenDMXDriver::send( unsigned length, BYTE * packet ) {
    if ( !m_fthandle )
        return DMX_NO_CONNECTION;

    FT_SetBreakOn( m_fthandle );
    Sleep( 1 );
    FT_SetBreakOff( m_fthandle );

    DWORD written = 0;

    FT_STATUS status = FT_Write( m_fthandle, packet, length, &written );
    if ( status != FT_OK ) 
        return DMX_SEND_ERROR;
    if ( written != length )
        return DMX_SEND_ERROR;

    return DMX_OK;
}

