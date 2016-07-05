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

#pragma once

/**
    Enttec USB Pro compatible DMX driver class utilizing FTDI USB to Serial port
*/

#include "FTDI_DMXDriver.h"

// Enttec Pro definitions
#define GET_WIDGET_PARAMS       3
#define GET_WIDGET_SN           10
#define GET_WIDGET_PARAMS_REPLY 3
#define SET_WIDGET_PARAMS       4
#define SET_DMX_RX_MODE         5
#define SET_DMX_TX_MODE         6
#define SEND_DMX_RDM_TX         7
#define RECEIVE_DMX_ON_CHANGE   8
#define RECEIVED_DMX_COS_TYPE   9

#define DMX_START_CODE          0x7E 
#define DMX_END_CODE            0xE7 
#define DMX_HEADER_LENGTH       4

class USBProDriver : public FTDI_DMXDriver
{
    BYTE m_dmx_header[DMX_HEADER_LENGTH];

    DMX_STATUS receiveData( int label, BYTE *data, WORD expected_length );
    DMX_STATUS sendData( BYTE label, BYTE* data, WORD data_size );
    DMX_STATUS receiveDMX( int label, BYTE *data, WORD *receive_length );
    DMX_STATUS setParams();

public:
    USBProDriver(universe_t universe_id);
    virtual ~USBProDriver(void);

protected:
    inline DMX_STATUS dmx_send( BYTE* packet ) {
        return sendData( SET_DMX_TX_MODE, packet, DMX_PACKET_SIZE + 1 );
    }

    virtual CString dmx_name();
    virtual DMX_STATUS dmx_open();
    virtual DMX_STATUS dmx_close( void );
};

#pragma pack(1)
struct DMXUSBPROParamsType {
    unsigned char FirmwareLSB;
    unsigned char FirmwareMSB;
    unsigned char BreakTime;
    unsigned char MaBTime;
    unsigned char RefreshRate;
} ;

struct DMXUSBPROSetParamsType {
    unsigned char UserSizeLSB;
    unsigned char UserSizeMSB;
    unsigned char BreakTime;
    unsigned char MaBTime;
    unsigned char RefreshRate;
} ;
#pragma pack()

struct ReceivedDmxCosStruct
{
    unsigned char start_changed_byte_number;
    unsigned char changed_byte_array[5];
    unsigned char changed_byte_data[40];
};

