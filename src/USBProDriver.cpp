/*
Copyright (C) 2016 Robert DeSantis
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
#include "USBProDriver.h"

static BYTE end_code = DMX_END_CODE;        // Always the same, never changes

// ----------------------------------------------------------------------------
//
USBProDriver::USBProDriver(universe_t universe_id) :
    FTDI_DMXDriver(universe_id)
{
    // Write the Packet Header - it never changes
    m_dmx_header[0] = DMX_START_CODE;
    m_dmx_header[1] = SET_DMX_TX_MODE;
    m_dmx_header[2] = (DMX_PACKET_SIZE + 1) & 0xFF;
    m_dmx_header[3] = (DMX_PACKET_SIZE + 1) >> 8;
}

// ----------------------------------------------------------------------------
//
USBProDriver::~USBProDriver(void) 
{
}

// ----------------------------------------------------------------------------
//
CString USBProDriver::dmx_name() {
    LPCSTR deviceName = m_device_info.Description[0] != '\0' ? m_device_info.Description : "USB PRO";

    CString name;
    name.Format( "%s on %s", deviceName, getConnectionInfo() );
    return name;
}

// ----------------------------------------------------------------------------
// Open DMX port
//
DMX_STATUS USBProDriver::dmx_open() {

    DMX_STATUS status = FTDI_DMXDriver::dmx_open();
    if (status != DMX_OK)
        return status;

    FT_STATUS res = FT_SetBaudRate(m_fthandle, 115200);
    if (FT_OK != res) {
        dmx_close();
        return DMX_OPEN_FAILURE;
    }

    DMXStudio::log_status("DMX universe %d driver started [%s]", getId(), (LPCSTR)dmx_name() );

    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Close DMX port
//
DMX_STATUS USBProDriver::dmx_close(void) {
    DMX_STATUS status = FTDI_DMXDriver::dmx_close();

    DMXStudio::log_status("DMX universe %d driver stopped [%s]", getId(), (LPCSTR)dmx_name() );

    return status;
}

// ----------------------------------------------------------------------------
// Receive data
//
DMX_STATUS USBProDriver::receiveData( int label, BYTE *data, WORD expected_length )
{
    FT_STATUS res = 0;
    WORD length = 0;
    DWORD bytes_read = 0;
    BYTE byte = 0;
    char buffer[600];
    
    // Check for Start Code and matching Label
    while ( byte != label )
    {
        while (byte != DMX_START_CODE)
        {
            res = FT_Read( m_fthandle, &byte, 1 ,&bytes_read );
            if(bytes_read == 0) 
                return DMX_RECEIVE_ERROR;
        }

        res = FT_Read( m_fthandle, &byte, 1, &bytes_read );
        if (bytes_read == 0) 
            return DMX_RECEIVE_ERROR;
    }
    
    // Read Length
    res = FT_Read( m_fthandle, &buffer, 2, &bytes_read );
    if ( bytes_read != 2)
        return DMX_RECEIVE_ERROR;
    length = buffer[0] | ((WORD)buffer[1] << 8);

    // Check Length is not greater than allowed
    if ( length > DMX_PACKET_SIZE )
        return DMX_RECEIVE_ERROR;

    // Read the actual Response Data
    res = FT_Read( m_fthandle, buffer, length, &bytes_read );
    if (bytes_read != length) 
        return DMX_RECEIVE_ERROR;

    // Check The End Code
    res = FT_Read( m_fthandle, &byte, 1, &bytes_read );
    if (bytes_read == 0)
        return DMX_RECEIVE_ERROR;

    if ( byte != DMX_END_CODE )
        return DMX_RECEIVE_ERROR;

    // Copy The Data read to the buffer passed
    memcpy( data, buffer, expected_length );
    
    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Receive DMX data
//
DMX_STATUS USBProDriver::receiveDMX( int label, BYTE *data, WORD *receive_length )
{
    FT_STATUS res = 0;
    WORD length = 0;
    DWORD bytes_read = 0;
    BYTE byte = 0;
    char buffer[600];

    // Wait for Start Code
    while (byte != DMX_START_CODE)
    {
        res = FT_Read( m_fthandle, &byte, 1 ,&bytes_read );
        if(bytes_read == 0) 
            return DMX_RECEIVE_ERROR;
    }

    // Read Length
    res = FT_Read( m_fthandle, &buffer, DMX_HEADER_LENGTH-1, &bytes_read );
    if ( bytes_read != DMX_HEADER_LENGTH-1 )
        return DMX_RECEIVE_ERROR;
    length = buffer[0] | ((WORD)buffer[1] << 8);

    // Check Length is not greater than allowed
    if ( length > DMX_PACKET_SIZE )
        return DMX_RECEIVE_ERROR;

    // Read the actual Response Data
    res = FT_Read( m_fthandle, buffer, length, &bytes_read );
    if (bytes_read != length) 
        return DMX_RECEIVE_ERROR;

    // Check The End Code
    res = FT_Read( m_fthandle, &byte, 1, &bytes_read );
    if (bytes_read == 0)
        return DMX_RECEIVE_ERROR;

    if ( byte != DMX_END_CODE )
        return DMX_RECEIVE_ERROR;

    // Copy The Data read to the buffer passed
    memcpy( data, buffer, length );
    *receive_length = length;

    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Send data buffer
//
DMX_STATUS USBProDriver::sendData( BYTE label, BYTE* data, WORD data_size ) {
    DWORD written = 0;

    BYTE data_header[DMX_HEADER_LENGTH];
    data_header[0] = DMX_START_CODE;
    data_header[1] = label;
    data_header[2] = (data_size) & 0xFF;
    data_header[3] = (data_size) >> 8;

    FT_STATUS status = FT_Write(m_fthandle, data_header, DMX_HEADER_LENGTH, &written);
    if (status != FT_OK)
        return DMX_SEND_ERROR;
    if (written != DMX_HEADER_LENGTH)
        return DMX_SEND_ERROR;

    // Write The Data
    status = FT_Write(m_fthandle, data, data_size, &written);
    if (status != FT_OK)
        return DMX_SEND_ERROR;
    if ( written != data_size )
        return DMX_SEND_ERROR;

    // Write End Code
    status = FT_Write(m_fthandle, &end_code, 1, &written);
    if (status != FT_OK)
        return DMX_SEND_ERROR;
    if ( written != 1 )
        return DMX_SEND_ERROR;

    return DMX_OK;
}

// ----------------------------------------------------------------------------
//
DMX_STATUS USBProDriver::setParams() {
    int size = 0;
    DMXUSBPROParamsType params;

    DMX_STATUS status = sendData( GET_WIDGET_PARAMS, (BYTE *)&size, 2 );
    if (status != DMX_OK) {
        dmx_close();
        return status;
    }

    status = receiveData( GET_WIDGET_PARAMS_REPLY, (BYTE *)&params, sizeof(DMXUSBPROParamsType) );
    if (status != DMX_OK) {
        dmx_close();
        return status;
    }

    // Display All PRO Parametrs & Info avialable
    printf("FIRMWARE VERSION: %d.%d\n", params.FirmwareMSB, params.FirmwareLSB);
    printf("BREAK TIME: %d micro sec\n", (int) (params.BreakTime * 10.67) + 100 );
    printf("MAB TIME: %d micro sec\n", (int) (params.MaBTime * 10.67) );
    printf("SEND REFRESH RATE: %d packets/sec\n", params.RefreshRate);

    return DMX_OK;
}

