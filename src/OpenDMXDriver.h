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
	Open DMX driver class utilizing FTDI USB to Serial port
*/

#include "abstractdmxdriver.h"
#include "ftd2xx.h"

class OpenDMXDriver :
	public AbstractDMXDriver
{
	FT_HANDLE m_fthandle;

public:
	OpenDMXDriver(void);
	virtual ~OpenDMXDriver(void);

	DMX_STATUS display_dmx_info( void );

protected:
	virtual DMX_STATUS dmx_send( unsigned length, BYTE * packet );
	virtual DMX_STATUS dmx_open( const char * connection_info );
	virtual DMX_STATUS dmx_close( void );

private:
	int map_name_to_id( const char * com_port_name );
};

