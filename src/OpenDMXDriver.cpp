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
#include "OpenDMXDriver.h"

//
OpenDMXDriver::OpenDMXDriver(void) :
    m_fthandle(0)
{
}

//
OpenDMXDriver::~OpenDMXDriver(void)
{
}

/**
    Send a packet frame consiting of a break and the packet supplied.  The
    supplied packet include the 0 command byte with at least 24 bytes of data
    to a maximum of 513 total bytes (command + 512 data).
*/
DMX_STATUS OpenDMXDriver::dmx_send( unsigned length, BYTE * packet ) {

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

// Open DMX port
//
DMX_STATUS OpenDMXDriver::dmx_open( const char * connection_info ) {
    if ( !connection_info )
        return DMX_ERROR;

    int device = map_name_to_id( connection_info );
    if ( device == -1 )
        return DMX_NO_DEVICE;

    FT_STATUS res = FT_Open( device, &m_fthandle );
    if ( FT_OK != res )
        return DMX_OPEN_FAILURE;

    res = FT_SetBaudRate( m_fthandle, 250000 );
    if ( FT_OK != res )
        return DMX_OPEN_FAILURE;

    res = FT_SetDataCharacteristics( m_fthandle, 8, 2, 0 );
    if ( FT_OK != res )
        return DMX_OPEN_FAILURE;

    return DMX_OK;
}

// Close DMX port
//
DMX_STATUS OpenDMXDriver::dmx_close( void ) {

    if ( !m_fthandle )
        return DMX_NO_CONNECTION;

    FT_Close( m_fthandle );

    m_fthandle = 0;

    return DMX_OK;
}

// Utility function for testing
//
DMX_STATUS OpenDMXDriver::display_dmx_info( void ) {
    FT_STATUS res;
    DWORD device_count;

    res = FT_CreateDeviceInfoList( &device_count );
    if ( FT_OK != res ) {
        printf( "Unable to get device count\n" );
        return DMX_ERROR;
    }

    FT_DEVICE_LIST_INFO_NODE *device_info_nodes = 
            (FT_DEVICE_LIST_INFO_NODE *)calloc( sizeof(FT_DEVICE_LIST_INFO_NODE), device_count );

    res = FT_GetDeviceInfoList( device_info_nodes, &device_count );
    if ( FT_OK != res ) {
        printf( "Unable to get device information\n" );
        free( device_info_nodes );
        return DMX_ERROR;
    }

    for ( unsigned i=0; i < device_count; i++ ) {
        printf( "%d: Flags=%ld, Type=%ld, ID=%ld, Location ID=%ld, s/n=%s, description=%s, handle=%ld\n",
                    i, device_info_nodes[i].Flags, device_info_nodes[i].Type, 
                    device_info_nodes[i].ID, device_info_nodes[i].LocId, 
                    device_info_nodes[i].SerialNumber, device_info_nodes[i].Description,
                    (DWORD)device_info_nodes[i].ftHandle );
    }

    free( device_info_nodes );

    return DMX_OK;
}

int OpenDMXDriver::map_name_to_id( const char * com_port_name ) {
    FT_STATUS res;
    DWORD device_count;
    int device = -1;

    res = FT_CreateDeviceInfoList( &device_count );
    if ( FT_OK != res )
        return -1;

    FT_DEVICE_LIST_INFO_NODE *device_info_nodes = 
            (FT_DEVICE_LIST_INFO_NODE *)calloc( sizeof(FT_DEVICE_LIST_INFO_NODE), device_count );

    res = FT_GetDeviceInfoList( device_info_nodes, &device_count );
    if ( FT_OK != res ) {
        free( device_info_nodes );
        return -1;
    }

    for ( unsigned i=0; i < device_count; i++ ) {
        LONG comport = -1;
        FT_HANDLE ftHandle;

        res = FT_Open( i, &ftHandle );
        if ( res != FT_OK )
            continue;

        res = FT_GetComPortNumber( ftHandle, &comport );
        FT_Close( ftHandle );

        if ( FT_OK != res || comport == -1 )
            continue;

        char name[24];
        sprintf_s( name, "com%ld", comport );

        if ( !_strcmpi( name, com_port_name ) ) {
            device = i;
            break;
        }
    }

    free( device_info_nodes );

    return device;
}
