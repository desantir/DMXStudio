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

#include "FTDI_DMXDriver.h"

// ----------------------------------------------------------------------------
//
FTDI_DMXDriver::FTDI_DMXDriver( universe_t universe_id ) :
    AbstractDMXDriver( universe_id),
    m_fthandle(0)
{
    memset( &m_device_info, 0, sizeof(FT_DEVICE_LIST_INFO_NODE) );
}

// ----------------------------------------------------------------------------
//
FTDI_DMXDriver::~FTDI_DMXDriver()
{
}

// ----------------------------------------------------------------------------
// Open DMX port
//
DMX_STATUS FTDI_DMXDriver::dmx_open() {
    if ( getConnectionInfo() == NULL )
        return DMX_ERROR;

    int device = map_name_to_id( getConnectionInfo(), &m_device_info );
    if (device == -1)
        return DMX_NO_DEVICE;

    FT_STATUS res = FT_Open(device, &m_fthandle);
    if (FT_OK != res)
        return DMX_OPEN_FAILURE;

    res = FT_Purge( m_fthandle, FT_PURGE_RX | FT_PURGE_TX );
    if (FT_OK != res)
        return DMX_OPEN_FAILURE;

    // display_dmx_info();

    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Return true if DMX is running
//
boolean FTDI_DMXDriver::dmx_is_running() {
    return m_fthandle != 0;
}

// ----------------------------------------------------------------------------
// Close DMX port
//
DMX_STATUS FTDI_DMXDriver::dmx_close(void) {
    if (!m_fthandle)
        return DMX_NO_CONNECTION;

    FT_Close(m_fthandle);

    m_fthandle = 0;

    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Utility function for testing
//
DMX_STATUS FTDI_DMXDriver::display_dmx_info(void) {
    FT_STATUS res;
    DWORD device_count;

    res = FT_CreateDeviceInfoList(&device_count);
    if (FT_OK != res) {
        printf("Unable to get device count\n");
        return DMX_ERROR;
    }

    FT_DEVICE_LIST_INFO_NODE *device_info_nodes =
        (FT_DEVICE_LIST_INFO_NODE *)calloc(sizeof(FT_DEVICE_LIST_INFO_NODE), device_count);

    res = FT_GetDeviceInfoList(device_info_nodes, &device_count);
    if (FT_OK != res) {
        printf("Unable to get device information\n");
        free(device_info_nodes);
        return DMX_ERROR;
    }

    for (unsigned i = 0; i < device_count; i++) {
        printf("%d: Flags=%ld, Type=%ld, ID=%ld, Location ID=%ld, s/n=%s, description=%s, handle=%ld\n",
            i, device_info_nodes[i].Flags, device_info_nodes[i].Type,
            device_info_nodes[i].ID, device_info_nodes[i].LocId,
            device_info_nodes[i].SerialNumber, device_info_nodes[i].Description,
            (DWORD)device_info_nodes[i].ftHandle );
    }

    free(device_info_nodes);

    return DMX_OK;
}

// ----------------------------------------------------------------------------
//
int FTDI_DMXDriver::map_name_to_id( LPCSTR com_port_name, FT_DEVICE_LIST_INFO_NODE *device_info ) {
    FT_STATUS res;
    DWORD device_count;
    int device = -1;

    res = FT_CreateDeviceInfoList(&device_count);
    if (FT_OK != res)
        return -1;

    FT_DEVICE_LIST_INFO_NODE *device_info_nodes =
        (FT_DEVICE_LIST_INFO_NODE *)calloc(sizeof(FT_DEVICE_LIST_INFO_NODE), device_count);

    res = FT_GetDeviceInfoList(device_info_nodes, &device_count);
    if (FT_OK != res) {
        free(device_info_nodes);
        return -1;
    }

    for (unsigned i = 0; i < device_count; i++) {
        LONG comport = -1;
        FT_HANDLE ftHandle;

        res = FT_Open(i, &ftHandle);
        if (res != FT_OK)
            continue;

        res = FT_GetComPortNumber(ftHandle, &comport);
        FT_Close(ftHandle);

        if (FT_OK != res || comport == -1)
            continue;

        char name[24];
        sprintf_s(name, "com%ld", comport);

        if (!_strcmpi(name, com_port_name)) {
            device = i;

            if ( device_info != NULL )
                memcpy( device_info, &device_info_nodes[i], sizeof(FT_DEVICE_LIST_INFO_NODE) );

            break;
        }
    }

    free(device_info_nodes);

    return device;
}
