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

#include <Winhttp.h>
#include <upnp.h>

#include "AbstractDMXDriver.h"
#include "SimpleJsonParser.h"

#define PACKET_TROTTLE_MS   125         // Throttle output of Hue packets

#define CHANNELS_PER_FIXTURE 8

/**
 DMX driver to control Philips Hue lighting
*/

struct CallbackContext {
    bool                m_success;
    bool                m_error;
    std::exception      m_exception;

    CString             m_body;
    HINTERNET           m_hRequest;
    CString             m_response;

    CallbackContext( HINTERNET hRequest, LPCSTR body ) :
        m_hRequest( hRequest ),
        m_success( false ),
        m_error( false ),
        m_body( body == NULL ? "" : body)
    {}

    inline void wait() {
        while ( true ) {
            if ( m_error ) 
                throw m_exception;
            if ( m_success )
                break;

            Sleep(10);
        }
    }

    inline bool isComplete() {
        return m_success || m_error;
    }
};

typedef CallbackContext * LPCALLBACKCONTEXT;

enum DeviceID {
    HUE_UNKNOWN = 0,
    HUE_DIMMABLE = 0x100,
    HUE_COLOR = 0x200,
    HUE_EXTENDED_COLOR = 0x210,
    HUE_COLOR_TEMPERATURE = 0x220
};

struct Group
{
    CString                 m_number;
    CString                 m_name;
    CString                 m_type;
    CString                 m_class;

    std::set<UINT>          m_lights;

    Group() {}

    bool containsLight( UINT light_number ) {
        return m_lights.find( light_number ) != m_lights.end();
    }
};

struct GroupCaseInsensitiveComparator
{
    bool operator() ( const CString& k1, const CString& k2 ) const {
        return _stricmp( (LPCSTR)k1, (LPCSTR)k2 ) < 0;
    }
};

typedef std::map<CString,Group,GroupCaseInsensitiveComparator> GroupMap;

struct Light
{
    UINT        m_number;

    CString     m_type;
    CString     m_name;
    CString     m_modelid;
    CString     m_manufacturer;
    CString     m_uniqueid;
    CString     m_swversion;

    char        m_color_gamut;              // A, B, C, or spec for null for non-color
    bool        m_on;
    UINT8       m_brightness;               // Brightness 1-254
    float       m_x;
    float       m_y;
    UINT16      m_ct;                       // Color temperature 153 (6500K) to 500 (2000K)
    DeviceID    m_device_id;                

    Light() :
        m_color_gamut( 0 ),
        m_on( false ),
        m_brightness( 0 ),
        m_x( 0.0f ),
        m_y( 0.0f ),
        m_ct( 153 ),
        m_device_id( HUE_UNKNOWN )
    {}

	channel_address dmx( ) const {
		return 1 + ((m_number-1) * CHANNELS_PER_FIXTURE);
	}

    inline bool m_hasWhiteChannel() const {
        return m_device_id == HUE_COLOR_TEMPERATURE || m_device_id == HUE_EXTENDED_COLOR;
    }
};

typedef std::map<int,Light> LightMap;

struct HuePacket {
    UINT        m_light;
    CString     m_packet;
    bool        m_on_off;
    bool        m_brightness;

    HuePacket( UINT light, LPCSTR packet, bool on_off, bool brightness ) :
        m_light( light ),
        m_packet( packet ),
        m_on_off( on_off ),
        m_brightness( brightness )
    {}
};

typedef std::list<HuePacket> HuePacketQueue;

class PhilipsHueDriver : public AbstractDMXDriver, Threadable
{
    HINTERNET   m_hSession;                             // WinHTTP session
    HINTERNET   m_hConnect;                             // WinHTTP Hue bridge connection
    DWORD       m_callback_context;                     // WinHTTP callback context

    CEvent m_wake;										// Wake up DMX transmitter
    std::atomic_bool m_latch;   						// Latch DMX packet

	channel_value m_packet[DMX_PACKET_SIZE + 1];					// Current DMX packet
	channel_value m_pending_packet[DMX_PACKET_SIZE + 1];			// Current staged DMX packet

    UINT run(void);

    CString     m_authorized_user;                      // Hue user name
    CString     m_ip;                                   // Hue bridge IP address
    CString     m_client_name;                          // Name of this client

    GroupMap    m_groups;                               // Bridge groups
    LightMap    m_lights;                               // Bridge lamps

    HuePacketQueue                m_packet_queue;       // Queue to trottle packet output
    std::queue<LPCALLBACKCONTEXT> m_asyncRequest;       // In proccess async requests

public:
    PhilipsHueDriver( universe_t universe_id );
    virtual ~PhilipsHueDriver(void);

protected:
    virtual CString dmx_name();
    virtual DMX_STATUS dmx_send( channel_value* packet );
    virtual DMX_STATUS dmx_open();
    virtual DMX_STATUS dmx_close(void);
    virtual boolean dmx_is_running();
    virtual DMX_STATUS dmx_discover( DriverFixtureArray& fixtures );

private:
    void uPNP_discover();
    bool createAuthorizedBridgeUser();
    bool discoverBridgeLights();
    bool discoverBridgeGroups();
    bool isLightAllowed( UINT light_number );

    LPCALLBACKCONTEXT jsonSend( LPCWSTR method, LPCSTR url, LPCSTR body );

    LPCALLBACKCONTEXT jsonPOST( LPCSTR url, LPCSTR body ) {
        return jsonSend( L"POST", url, body );
    }

    LPCALLBACKCONTEXT jsonPUT( LPCSTR url, LPCSTR body ) {
        return jsonSend( L"PUT", url, body );
    }

    LPCALLBACKCONTEXT jsonGET( LPCSTR url ) {
        return jsonSend( L"GET", url, NULL );
    }

    void checkHueError( SimpleJsonParser& parser );

    void updateLights();

    void compute_hsb( UINT8 rgb_red, UINT8 rgb_green, UINT8 rgb_blue, float& x, float &y, UINT8& bri, char color_gamut );

    bool readHueUser( CString& user, LPCSTR bridge_ip );
    bool writeHueUser( LPCSTR user, LPCSTR bridge_ip );

    void queuePacket( UINT light, LPCSTR packet, bool on_off, bool brightness );
};

