/* 
Copyright (C) 2011-15 Robert DeSantis
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
    Open DMX driver class utilizing FTDI USB to Serial port
*/

#include "FTDI_DMXDriver.h"

class OpenDMXDriver : public FTDI_DMXDriver, Threadable
{
    CEvent m_wake;										// Wake up DMX transmitter for new packets
    CCriticalSection m_lock;                            // Lock to prevent packet access collissions

	channel_value m_packet[DMX_PACKET_SIZE + 1];					// Current DMX packet
	channel_value m_pending_packet[DMX_PACKET_SIZE + 1];			// Current staged DMX packet

    UINT run(void);

    DMX_STATUS send(unsigned length, channel_value* packet);

public:
    OpenDMXDriver( universe_t universe_id );
    virtual ~OpenDMXDriver(void);

protected:
    virtual CString dmx_name();
    virtual DMX_STATUS dmx_send( channel_value* packet_513 );
    virtual DMX_STATUS dmx_open();
    virtual DMX_STATUS dmx_close(void);
    virtual boolean dmx_is_running();
};

