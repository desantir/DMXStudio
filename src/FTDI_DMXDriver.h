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

#pragma once

#include "AbstractDMXDriver.h"
#include "ftd2xx.h"

/**
   Common FTDI USB to Serial port methods
*/

class FTDI_DMXDriver : public AbstractDMXDriver
{
protected:
    FT_HANDLE m_fthandle;
    FT_DEVICE_LIST_INFO_NODE m_device_info;

public:
    FTDI_DMXDriver( universe_t universe_id );
    ~FTDI_DMXDriver();

    DMX_STATUS display_dmx_info(void);

private:
    int map_name_to_id( const char * com_port_name, FT_DEVICE_LIST_INFO_NODE *device_info );

protected:
    virtual DMX_STATUS dmx_send( BYTE* packet ) = 0;
    virtual CString dmx_name() = 0;    

    virtual boolean dmx_is_running(void);
    virtual DMX_STATUS dmx_open(void);
    virtual DMX_STATUS dmx_close(void);
};

