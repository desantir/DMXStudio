/* 
Copyright (C) 2015-16 Robert DeSantis
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

#include "Universe.h"
#include "OpenDMXDriver.h"
#include "USBProDriver.h"

// ----------------------------------------------------------------------------
//
DMX_STATUS Universe::start() {
    switch ( m_type ) {
        case OPEN_DMX:
            m_driver = new OpenDMXDriver(m_id);
            break;

        case ENTTEC_USB_PRO:
            m_driver = new USBProDriver(m_id);
            break;
    }

    m_driver->setConnectionInfo(m_dmx_port );
    m_driver->setPacketDelayMS( m_dmx_packet_delay );
    m_driver->setMinimumDelayMS( m_dmx_packet_min_delay );

    return m_driver->start(  );
}

// ----------------------------------------------------------------------------
//
void  Universe::stop() {
    if ( m_driver ) {
        m_driver->stop();
        delete m_driver;
        m_driver = NULL;
    }
}