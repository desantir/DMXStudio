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
#include "PhilipsHueDriver.h"

#define UPNP_ASSERT( hr, ... ) \
    if ( !SUCCEEDED(hr) ) { \
        CString message( "Audio input stream " ); \
        message.AppendFormat( __VA_ARGS__ ); \
        message.AppendFormat( " (0x%lx)", hr ); \
        throw StudioException( __FILE__, __LINE__, (LPCSTR)message ); \
    }

#define HUE_NAME_PREFIX     "Philips hue ("

static LPCWSTR accept_types[] = { L"*/*", NULL };
static LPCWSTR gAgentName = L"DMXStudio";
static LPCWSTR gHeaders = L"Content-Type: application/json\r\n";

void FreeContext( LPCALLBACKCONTEXT lpContext );

static std::map<CString, char> model_to_gamut;

// ----------------------------------------------------------------------------
//
PhilipsHueDriver::PhilipsHueDriver( universe_t universe_id ) :
    AbstractDMXDriver( universe_id ),
    m_hSession( NULL ),
    m_hConnect( NULL),
    m_callback_context( (DWORD)this ),
    Threadable("PhilipsHueDriver")
{
    m_latch.store(false);
    memset(m_packet, 0, sizeof(m_packet));

    char machine_name[ 128 ];
    DWORD name_size = sizeof(machine_name);

    if ( GetComputerName( machine_name, &name_size ) == 0 )
        strcpy_s( machine_name, "unknown" );

    m_client_name.Format( "DMXSTUDIO#%s", machine_name );

    model_to_gamut["LCT001"] = 'B';
    model_to_gamut["LCT007"] = 'B';
    model_to_gamut["LCT010"] = 'C';
    model_to_gamut["LCT014"] = 'C';
    model_to_gamut["LCT002"] = 'B';
    model_to_gamut["LCT003"] = 'B';
    model_to_gamut["LCT011"] = 'C';
    model_to_gamut["LST001"] = 'A';
    model_to_gamut["LLC010"] = 'A';
    model_to_gamut["LLC011"] = 'A';
    model_to_gamut["LLC012"] = 'A';
    model_to_gamut["LLC006"] = 'A';
    model_to_gamut["LLC007"] = 'A';
    model_to_gamut["LLC013"] = 'A';
    model_to_gamut["LLM001"] = 'B';
    model_to_gamut["LLC020"] = 'C';
    model_to_gamut["LST002"] = 'C';
}

// ----------------------------------------------------------------------------
//
PhilipsHueDriver::~PhilipsHueDriver(void)
{
    dmx_close();
}

// ----------------------------------------------------------------------------
//
CString PhilipsHueDriver::dmx_name() {
    CString name( "Philips Hue" );

    if ( m_ip.GetLength() > 0 )
        name.AppendFormat( " @ %s", m_ip );

    return name;
}

// ----------------------------------------------------------------------------
// Return true if DMX is running
//
boolean PhilipsHueDriver::dmx_is_running() {
    return isRunning();
}

// ----------------------------------------------------------------------------
// Open DMX port
//
DMX_STATUS PhilipsHueDriver::dmx_open() {
    try {
        if ( studio.getHueBridgeIP() == NULL || strlen(studio.getHueBridgeIP()) == 0 ) {
            uPNP_discover();
            studio.setHueBridgeIP( m_ip );
        }
        else
            m_ip = studio.getHueBridgeIP();

        CStringW host( m_ip );

        m_hSession = WinHttpOpen( gAgentName, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC );
        if ( !m_hSession )
            throw StudioException( "Unable to open internet session [%ld]", GetLastError() );

        m_hConnect = WinHttpConnect( m_hSession, host, INTERNET_DEFAULT_HTTP_PORT, 0);
        if ( !m_hConnect )
            throw StudioException( "Unable to connect session [%ld]", GetLastError() );

        // Authorize if we don't have a user defined or the user cannot access the bridge
        if ( !readHueUser( m_authorized_user, (LPCSTR)m_ip ) ) {
            if ( !createAuthorizedBridgeUser() )
                throw StudioException( "Unable to create Hue bridge user" );        
        }

        if ( !discoverBridgeGroups() )
           throw StudioException( "Error discovering available Hue bridge groups" );
        if ( !discoverBridgeLights() )
            throw StudioException( "Error discovering available Hue bridge lights" );

        if ( m_lights.size() == 0 )
            throw StudioException( "No Hue light fixtures discovered" );

        // Initialize bridge lights
        CString body;
        body.Format( "{ \"on\": false, \"alert\": \"none\", \"effect\": \"none\", \"transitiontime\": 5, \"bri\": 1 }" );

        for ( LightMap::value_type& light : m_lights ) {
			if ( light.second.dmx() + CHANNELS_PER_FIXTURE <= 512 ) 
				 DMXStudio::log_status( "Light #%d %s %s [DMX %d]", light.second.m_number, (LPCSTR)light.second.m_name, 
					(LPCSTR)light.second.m_modelid, light.second.dmx() );
			else
				DMXStudio::log_status( "Light #%d %s %s [DMX OUT OF RANGE]", light.second.m_number, (LPCSTR)light.second.m_name, 
					(LPCSTR)light.second.m_modelid, light.second.dmx() );

            CString url;
            url.Format( "/api/%s/lights/%d/state", m_authorized_user, light.second.m_number );

            LPCALLBACKCONTEXT lpContext = jsonPUT( url, body );
            lpContext->wait();
            FreeContext( lpContext );
        }
    }
    catch ( std::exception& e ) {
        DMXStudio::log( e );

        if ( m_hConnect )
            WinHttpCloseHandle( m_hConnect );

        if ( m_hSession )
            WinHttpCloseHandle( m_hSession );

        m_hConnect = m_hSession = NULL;

        return DMX_ERROR;
    }

    return startThread() ? DMX_OK : DMX_ERROR;
}

// ----------------------------------------------------------------------------
// Send current buffer
//
DMX_STATUS PhilipsHueDriver::dmx_send( channel_value* packet ) {
    if (!isRunning())                              // If not running, assume we are in "disconnected" mode
        return DMX_OK;

    memcpy( m_pending_packet, packet, sizeof(m_packet) );

    m_latch.store(true);

    m_wake.SetEvent();

    return DMX_OK;
}

// ----------------------------------------------------------------------------
// Close DMX port
//
DMX_STATUS PhilipsHueDriver::dmx_close(void) {
    DMX_STATUS status = ( !stopThread() ) ? DMX_ERROR : DMX_OK;

    if ( m_hConnect )
        WinHttpCloseHandle( m_hConnect );

    if ( m_hSession )
        WinHttpCloseHandle( m_hSession );

    m_hConnect = m_hSession = NULL;

    return status;
}

// ----------------------------------------------------------------------------
//
UINT PhilipsHueDriver::run(void) {
    DMXStudio::log_status("DMX universe %d driver started [%s]", getId(), (LPCSTR)dmx_name() );

    const long sleep_ms = 5;
    DWORD next_send_ms = 0L;
    CString url;

    while (isRunning()) {
        while ( m_asyncRequest.size() > 0 ) {
            LPCALLBACKCONTEXT lpContext = m_asyncRequest.front();
            if ( !lpContext->isComplete() )
                break;

            m_asyncRequest.pop();

            if ( lpContext->m_error )
                DMXStudio::log( lpContext->m_exception );

            FreeContext( lpContext );
        }

        if (::WaitForSingleObject(m_wake.m_hObject, sleep_ms) == WAIT_OBJECT_0) {
            if ( m_latch.load() ) {
                memcpy(m_packet, m_pending_packet, sizeof(m_packet));
                m_latch.store(false);
                updateLights();
            }
        }

        DWORD time_ms = GetTickCount();

        if ( next_send_ms < time_ms && m_packet_queue.size() > 0 ) {
            HuePacket& packet = m_packet_queue.front();

            url.Format( "/api/%s/lights/%d/state", m_authorized_user, packet.m_light );

            m_asyncRequest.push( jsonPUT( url, packet.m_packet ) );

            //printf( "[%d] To: %u: %s%s%s (async size=%u)\n", m_packet_queue.size(), packet.m_light, (LPCSTR)packet.m_packet, 
            //        packet.m_on_off ? " [ON/OFF]" : "", packet.m_brightness ? " [BRI]" : "", m_asyncRequest.size() );

            m_packet_queue.pop_front();

            next_send_ms = time_ms + PACKET_TROTTLE_MS;
        }
    }

    DMXStudio::log_status("DMX universe %d driver stopped [%s]", getId(), (LPCSTR)dmx_name() );

    return DMX_OK;
}

// ----------------------------------------------------------------------------
//
void PhilipsHueDriver::queuePacket( UINT light, LPCSTR packet, bool on_off, bool brightness ) {

    for ( std::reverse_iterator<HuePacketQueue::iterator> rit=m_packet_queue.rbegin(); rit != m_packet_queue.rend(); rit++ ) {
        if ( rit->m_light != light )
            continue;

        // See if we replace a already queued packet for this light
        if ( (!rit->m_on_off || on_off) && (!rit->m_brightness || brightness) ) {
            rit->m_on_off = on_off;
            rit->m_brightness = brightness;
            rit->m_packet = packet;
            return;
        }
    }

    // Add a new packet
    m_packet_queue.emplace_back( light, packet, on_off, brightness );
}

// ----------------------------------------------------------------------------
//
void PhilipsHueDriver::updateLights() {
    CString body;

    for ( LightMap::value_type& light_entry : m_lights ) {
        Light& light = light_entry.second;

        const channel_address base = light.dmx();
        const channel_address dimmer = base + 0;
        const channel_address transition = base+1;
        const channel_address white = base + 2;
        const channel_address red = base+3;
        const channel_address green = base+4;
        const channel_address blue = base+5;

        UINT transition_time = m_packet[transition] / 10;

        UINT color_sum = m_packet[white];
        if ( light.m_color_gamut != '\0' )
            color_sum += m_packet[red] + m_packet[green] + m_packet[blue];

        try {
            // Optimize for "off" light
            if ( m_packet[dimmer] == 0 || color_sum == 0 ) {
                if ( light.m_on ) {
                    // Need to reset colors on off else fixtures will show last color when turned on with a transition time
                    body.Format( "{ \"on\": false, \"transitiontime\": %d, \"bri\":1}", transition_time ); 
                    queuePacket( light.m_number, body, true, true );

                    // body.Format( "{ \"xy\": [0.15, 0.15] }" ); 
                    // queuePacket( light.m_number, body, false, false );

                    light.m_on = false;
                    light.m_brightness = 1;

                    // These will need to be resent
                    light.m_x = 0;
                    light.m_y = 0;
                    light.m_ct = 0;
                }

                continue;
            }
            
            // Only send on when needed - improves performance
            LPCSTR on_json = ( light.m_on ) ? "" : "\"on\": true, ";  
            bool on_off = !light.m_on;

            if ( light.m_device_id == HUE_DIMMABLE ) {
                UINT8 bri = std::min<UINT8>( m_packet[dimmer], 254 );

                // Optimize for unchanged light
                if ( light.m_on && light.m_brightness == bri )
                    continue;       

                body.Format( "{ %s \"bri\": %d }", on_json, bri );

                queuePacket( light.m_number, body, on_off, true );

                light.m_on = true;
                light.m_brightness = bri;

                continue;
            }

            if ( light.m_hasWhiteChannel() && m_packet[white] > 0 ) {       // Color temp trumps all others
                UINT16 ct = 255 - m_packet[white];                          // Invert DMX value

                // Map 1-255 -> 153-500
                ct = 153 + (UINT16)(((double)ct / 254.0) * (500.0 - 153.0));

                UINT8 bri = std::min<UINT8>( m_packet[dimmer], 254 );

                // Optimize for unchanged light
                if ( light.m_on && light.m_ct == ct && light.m_brightness == bri )
                    continue;       

                CString bri_json;
                if ( bri != light.m_brightness )
                    bri_json.Format( "\"bri\": %d,", bri );

                body.Format( "{ %s%s\"transitiontime\": %d, \"ct\": %d }", on_json, bri_json, transition_time, ct );

                queuePacket( light.m_number, body, on_off, bri != light.m_brightness );

                light.m_on = true;
                light.m_ct = ct;
                light.m_brightness = bri;

                // These will need to be resent
                light.m_x = 0;
                light.m_y = 0;

                continue;
            }

            float x, y;
            UINT8 bri;

            compute_hsb( m_packet[red], m_packet[green], m_packet[blue], x, y, bri, light.m_color_gamut );

            // Use dimmer channel as a ceil for color
            bri = std::min<UINT8>( m_packet[dimmer], bri );

            // Optimize for unchanged light
            if ( light.m_on && light.m_x == x  && light.m_y == y && light.m_on && light.m_brightness == bri )
                continue;       

            CString bri_json;
            if ( bri != light.m_brightness )
                bri_json.Format( "\"bri\": %d,", bri );

            body.Format( "{ %s%s\"transitiontime\": %d, \"xy\": [%0.4f, %0.4f] }", on_json, bri_json, transition_time, x, y );

            queuePacket( light.m_number, body, on_off, bri != light.m_brightness );

            light.m_on = true;
            light.m_x = x;
            light.m_y = y;
            light.m_brightness = bri;

            // This will need to be resent
            light.m_ct = 0;
        }
        catch ( std::exception& e ) {
            DMXStudio::log( e );
        }
    }
}

// ----------------------------------------------------------------------------
// Convert RGB to XY (see http://www.developers.meethue.com/documentation/color-conversions-rgb-xy)
//
void PhilipsHueDriver::compute_hsb( UINT8 rgb_red, UINT8 rgb_green, UINT8 rgb_blue, float& x, float &y, UINT8& bri, char color_gamut ) {

    // Convert RGB to floating point equiv
    float red = rgb_red / 255.0f;
    float green = rgb_green / 255.0f;
    float blue = rgb_blue / 255.0f;

    // Apply a gamma correction 
    red = (red > 0.04045f) ? pow((red + 0.055f) / (1.0f + 0.055f), 2.4f) : (red / 12.92f);
    green = (green > 0.04045f) ? pow((green + 0.055f) / (1.0f + 0.055f), 2.4f) : (green / 12.92f);
    blue = (blue > 0.04045f) ? pow((blue + 0.055f) / (1.0f + 0.055f), 2.4f) : (blue / 12.92f); 

    // Convert the RGB values to XYZ using the Wide RGB D65 conversion formula
    float X = red * 0.664511f + green * 0.154324f + blue * 0.162028f;
    float Y = red * 0.283881f + green * 0.668433f + blue * 0.047685f;
    float Z = red * 0.000088f + green * 0.072310f + blue * 0.986039f;

    // Calculate the xy values from the XYZ values
    x = X / (X + Y + Z);
    y = Y / (X + Y + Z);
    // bri = 254.0 * Y;

    // TODO Calculate the closest point on the color gamut triangle and use that as xy value

    // Using Y as the brightness is not effective - red is bright and blue is dim
    // Hack to get max RGB value and use that to drive brightness
    UINT8 max_brightness = std::max<UINT8>( rgb_red, std::max<UINT8>( rgb_blue, rgb_green ) );

    bri = std::min<UINT8>( 255 * max_brightness / 255, 254 );
}

// ----------------------------------------------------------------------------
//
void PhilipsHueDriver::uPNP_discover() {

    IUPnPDeviceFinder* pDeviceFinder = NULL;
    IUPnPDevices* pDevices = NULL;
    IUnknown* punkEnum = NULL;
    IEnumUnknown *pEU = NULL; 
    IUnknown* punkDevice = NULL;
    IUPnPDevice* pDevice = NULL;
    long lCount; 
    HRESULT hr;

    std::vector<CString> hue_ips;

    DMXStudio::log_status( "Discovering HUE bridges..." );

    try {
        // Instantiate the device finder object
        hr = CoCreateInstance( CLSID_UPnPDeviceFinder, NULL, CLSCTX_SERVER, IID_IUPnPDeviceFinder, (LPVOID *)&pDeviceFinder);
        UPNP_ASSERT( hr, "Unable to initialize uPNP interface" );

        BSTR searchType = SysAllocString( L"upnp:rootdevice" );
        hr = pDeviceFinder->FindByType( searchType, 0, &pDevices );
        SysFreeString( searchType );
        UPNP_ASSERT( hr, "Unable to find uPNP by type" );

        hr = pDevices->get_Count (&lCount );
        UPNP_ASSERT( hr, "Unable to get uPNP device count" );

        STUDIO_ASSERT( lCount > 0, "Found no uPNP devices" );

        hr = pDevices->get__NewEnum( &punkEnum );
        UPNP_ASSERT( hr, "Unable to get IEnumUnknown enumerator interface" );

        hr = punkEnum->QueryInterface(IID_IEnumUnknown, (VOID **) &pEU);
        UPNP_ASSERT( hr, "Unable to get uPNP device query interface" );

        // Go through devices and fins Hue!
        for ( long lIndex = 0; lIndex<lCount; lIndex++ ) {
            hr = pEU->Next( 1, &punkDevice, NULL );
            UPNP_ASSERT( hr, "Unable to get next uPNP device" );

            // Get a IUPnPDevice pointer to the device just got
            hr = punkDevice->QueryInterface( IID_IUPnPDevice, (VOID **)&pDevice );
            UPNP_ASSERT( hr, "Unable to get device pointer" );

            BSTR bstrFriendlyName = NULL;
            hr = pDevice->get_FriendlyName( &bstrFriendlyName );
            UPNP_ASSERT( hr, "Unable to get device name" );

            CString friendly_name( bstrFriendlyName );
            SysFreeString(bstrFriendlyName);

            if ( strncmp( (LPCSTR)friendly_name, HUE_NAME_PREFIX, strlen(HUE_NAME_PREFIX) ) == 0 ) {
                DMXStudio::log_status( "Found %s", (LPCSTR)friendly_name );

                int index = friendly_name.Find( ')', strlen(HUE_NAME_PREFIX) );

                if ( index != -1 )
                    hue_ips.push_back( friendly_name.Mid( strlen(HUE_NAME_PREFIX), index - strlen(HUE_NAME_PREFIX) ) );
            }

            SAFE_RELEASE( punkDevice );
            SAFE_RELEASE( pDevice );

            punkDevice = NULL;
            pDevice = NULL;
        }
    }
    catch ( std::exception& e ) {
        SAFE_RELEASE( punkDevice );
        SAFE_RELEASE( pDevice );
        SAFE_RELEASE( pDevices );
        SAFE_RELEASE( punkEnum );
        SAFE_RELEASE( pEU ); 
        SAFE_RELEASE( pDeviceFinder );

        throw e;
    }

    SAFE_RELEASE( punkDevice );
    SAFE_RELEASE( pDevice );
    SAFE_RELEASE( pDevices );
    SAFE_RELEASE( punkEnum );
    SAFE_RELEASE( pEU ); 
    SAFE_RELEASE( pDeviceFinder );

    STUDIO_ASSERT( hue_ips.size() > 0, "No PNP HUE bridges found" );
    STUDIO_ASSERT( hue_ips.size() == 1, "Multiple HUE bridges found" );

    m_ip = hue_ips.back();
}

// ----------------------------------------------------------------------------
//
bool PhilipsHueDriver::discoverBridgeGroups() {
    CString hue_url;

    hue_url.Format( "/api/%s/groups", m_authorized_user );

    LPCALLBACKCONTEXT lpContext = NULL;

    SimpleJsonParser parser;

    try {
        lpContext = jsonGET( hue_url );

        lpContext->wait();

        try {
            parser.parse( lpContext->m_response );
        }
        catch ( std::exception& e ) {
            throw StudioException( "JSON parser error (%s) data (%s)", e.what(), lpContext->m_response );
        }

        checkHueError( parser );

        for ( CString key : parser.keys() ) {
            JsonNode& node = parser.get<JsonNode>( key );

            Group group;
            group.m_name = node.get<CString>( "name" );
            group.m_type = node.get<CString>( "type" );
            group.m_class = node.get<CString>( "class", "" );
            group.m_lights = node.getArrayAsSet<UINT>( "lights" );

            m_groups[group.m_name] = group;
        }
    }
    catch ( std::exception& ex ) {
        FreeContext( lpContext );

        DMXStudio::log( ex );
        return false;
    }

    FreeContext( lpContext );
    return true;
}

// ----------------------------------------------------------------------------
//
bool PhilipsHueDriver::discoverBridgeLights() {
    CString hue_url;

    hue_url.Format( "/api/%s/lights", m_authorized_user );

    LPCALLBACKCONTEXT lpContext = NULL;

    SimpleJsonParser parser;

    try {
        lpContext = jsonGET( hue_url );

        lpContext->wait();

        try {
            parser.parse( lpContext->m_response );
        }
        catch ( std::exception& e ) {
            throw StudioException( "JSON parser error (%s) data (%s)", e.what(), lpContext->m_response );
        }

        checkHueError( parser );

        for ( CString key : parser.keys() ) {
            JsonNode& node = parser.get<JsonNode>( key );

            UINT light_number = atoi( (LPCSTR)key );

            if ( !isLightAllowed( light_number ) )
                continue;

            Light light;

            light.m_number = light_number;
            light.m_type = node.get<CString>( "type" );
            light.m_name = node.get<CString>( "name" );
            light.m_modelid = node.get<CString>( "modelid" );
            light.m_manufacturer = node.get<CString>( "manufacturername" );
            light.m_uniqueid = node.get<CString>( "uniqueid" );
            light.m_swversion = node.get<CString>( "swversion" );
            light.m_color_gamut = model_to_gamut[light.m_modelid];

            if ( !light.m_type.CompareNoCase( "Extended Color Light" ) )
                light.m_device_id = HUE_EXTENDED_COLOR;
            else if ( !light.m_type.CompareNoCase( "Color Light" ) )
                light.m_device_id = HUE_COLOR;
            else if ( !light.m_type.CompareNoCase( "Color Temperature Light" ) )
                light.m_device_id = HUE_COLOR_TEMPERATURE;
            else // Dimmable Light
                light.m_device_id = HUE_DIMMABLE;

            m_lights[light.m_number] = light;
        }
    }
    catch ( std::exception& ex ) {
        FreeContext( lpContext );

        DMXStudio::log( ex );
        return false;
    }

    FreeContext( lpContext );
    return true;
}

bool PhilipsHueDriver::isLightAllowed( UINT light_number ) {
    if ( studio.hasHueAllowedGroups() ) {
        bool found_in_group = false;

        for ( LPCSTR group_name : *studio.getHueAllowedGroups() ) {
            GroupMap::iterator it = m_groups.find( group_name );
            if ( it == m_groups.end() )
                continue;

            if ( it->second.containsLight( light_number ) ) {
                found_in_group = true;
                break;
            }
        }

        if ( !found_in_group )
            return false;
    }

    return !studio.isHueLightIgnored( light_number );
}

// ----------------------------------------------------------------------------
//
bool PhilipsHueDriver::createAuthorizedBridgeUser() {
    CString request;

    request.Format( "{\"devicetype\":\"%s\"}", m_client_name );

    // Give the user time to press the bridge button
    printf( "\n>> PRESS THE HUE BRIDGE BUTTON AND THEN PRESS THE ENTER KEY: " );
    while ( _getch() != 13 )
        ;
    printf( "\n\n" );

    LPCALLBACKCONTEXT lpContext = NULL;

    SimpleJsonParser parser;

    try {
        lpContext = jsonPOST( "/api" , request );

        lpContext->wait();

        parser.parse( lpContext->m_response );

        checkHueError( parser );

        JsonNodePtrArray array = parser.getObjects();
        if ( array.size() != 1 )
            throw StudioException( "Unexpected response: %s", (LPCSTR)lpContext->m_response );   

        JsonNode* node = array.front();
        JsonNode& successNode = node->get<JsonNode>( "success" );

        m_authorized_user = successNode.get<CString>( "username" );

        if ( !writeHueUser( m_authorized_user, m_ip ) )
            DMXStudio::log_status( "Unable to save Hue autheorized user" );

        DMXStudio::log_status( "Hue bridge authorization successful" );
    }
    catch ( std::exception& ex ) {
        FreeContext( lpContext );

        DMXStudio::log( ex );
        return false;
    }

    FreeContext( lpContext );

    return true;
}

// ----------------------------------------------------------------------------
//
void PhilipsHueDriver::checkHueError( SimpleJsonParser& parser ) {
    if ( !parser.isArray() )
        return;

    JsonNodePtrArray array = parser.getObjects();
    if ( array.size() == 1 ) {
        JsonNode* node = array.front();

        if ( node->has_key( "error" ) ) {
            JsonNode& errorNode = node->get<JsonNode>( "error" );
            CString desc = errorNode.get<CString>( "description" );
            throw StudioException( "Hue bridge returned error: %s", (LPCSTR)desc );      
        }
    }
}

// ----------------------------------------------------------------------------
//
void FreeContext( LPCALLBACKCONTEXT lpContext ) 
{
    if ( lpContext != NULL ) {
        WinHttpSetStatusCallback( lpContext->m_hRequest, NULL, NULL, NULL );

        WinHttpCloseHandle( lpContext->m_hRequest );

        delete lpContext;
    }
}

// ----------------------------------------------------------------------------
//
VOID CALLBACK MyCallback( HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, WORD dwStatusInformationLength )
{
    // printf( "this=%lx status=%x\n", dwContext, dwInternetStatus );

    LPCALLBACKCONTEXT lpContext = (LPCALLBACKCONTEXT)dwContext;
    if ( lpContext == NULL )
        return;

    try {
        switch ( dwInternetStatus ) {
            case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
                // printf( "WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE (%d)\n", dwStatusInformationLength);

                if ( !WinHttpReceiveResponse( lpContext->m_hRequest, NULL ) )
                    throw StudioException( "Error waiting for HTTP response [%ld]", GetLastError() );

                break;

            case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: {
                // printf( "WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE (%d)\n", dwStatusInformationLength);

                DWORD dwStatusCode = 0;
                DWORD dwSize = sizeof(dwStatusCode);

                WinHttpQueryHeaders( lpContext->m_hRequest, 
                    WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
                    WINHTTP_HEADER_NAME_BY_INDEX, 
                    &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX );

                if ( dwStatusCode != HTTP_STATUS_OK )
                    throw StudioException( "Received HTTP status %ld", dwStatusCode );

                if ( !WinHttpQueryDataAvailable( lpContext->m_hRequest, NULL ) )
                    throw StudioException( "Error encountered [%ld]", GetLastError() );

                break;
            }

            case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE: {
                // printf( "WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE (%d)\n", dwStatusInformationLength);

                DWORD dwSize = *((LPDWORD)lpvStatusInformation);

                if ( dwSize == 0 ) {
                    lpContext->m_success = true;
                    break;
                }

                char* buffer = new char[dwSize+1];
                DWORD read;

				// CAUTION: If we don't read the entire buffer, we will get an async call with the remainder BEFORE the read returns
                if ( !WinHttpReadData( lpContext->m_hRequest, (LPVOID)buffer, dwSize, &read ) )
                    throw StudioException( "Error reading reponse [%ld]", GetLastError() );

                buffer[read] = '\0';

                lpContext->m_response.Append( buffer );

				delete[] buffer;

                break;
            }

            case WINHTTP_CALLBACK_STATUS_READ_COMPLETE: {
                // printf( "WINHTTP_CALLBACK_STATUS_READ_COMPLETE (%d)\n", dwStatusInformationLength);

                // Copy the data and delete the buffers.

                if ( dwStatusInformationLength != 0 ) {
                    if ( !WinHttpQueryDataAvailable( lpContext->m_hRequest, NULL ) )
                        throw StudioException( "Error encountered [%ld]", GetLastError() );
                }

                break;
            }

            case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
                // printf( "WINHTTP_CALLBACK_STATUS_REQUEST_ERROR \n" );

                throw StudioException( "Request error" );
        }
    }
    catch ( std::exception& ex ) {
        lpContext->m_exception = std::exception( ex.what() );
        lpContext->m_error = true;
    }
}

// ----------------------------------------------------------------------------
//
LPCALLBACKCONTEXT PhilipsHueDriver::jsonSend( LPCWSTR method, LPCSTR url, LPCSTR body )
{
    HINTERNET hRequest = NULL;

    CStringW wide_url( url );

    LPCALLBACKCONTEXT lpContext = NULL;

    try {
        hRequest = WinHttpOpenRequest( m_hConnect, method, wide_url, NULL, WINHTTP_NO_REFERER, accept_types, 0L );
        if ( !hRequest )
            throw StudioException( "Unable to open request [%ld]", GetLastError() );

        if ( WinHttpSetStatusCallback( hRequest, (WINHTTP_STATUS_CALLBACK)MyCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, NULL ) != NULL )
            throw StudioException( "Unexpected callback returned" );

        lpContext = new CallbackContext( hRequest, body );

        DWORD requestLength = lpContext->m_body.GetLength();
        LPVOID requestBuffer = (requestLength > 0) ? (LPVOID)(LPCSTR)(lpContext->m_body) : NULL;

        if ( !WinHttpSendRequest( hRequest, gHeaders, -1L, requestBuffer, requestLength, requestLength, (DWORD_PTR)lpContext ) )
            throw StudioException( "Unable to send HTTP request [%ld]", GetLastError() );
    }
    catch ( std::exception& ex ) {
        FreeContext( lpContext );

        if ( hRequest )
            WinHttpCloseHandle( hRequest );

        throw ex;
    }

    return lpContext;
}

// ----------------------------------------------------------------------------
//
bool PhilipsHueDriver::readHueUser( CString& user, LPCSTR bridge_ip )
{
    CString filename;
    filename.Format( "%s\\DMXStudio\\%s.hue", DMXStudio::getUserDocumentDirectory(), bridge_ip );

    user.Empty();

    if ( !PathFileExists( filename ) )
        return false;

    FILE* hHue = _fsopen( filename, "rt", _SH_DENYWR );
    if ( !hHue )
        return false;

    fgets( user.GetBufferSetLength( 1024 ), 1024, hHue );
    fclose( hHue );
    user.ReleaseBuffer();

    return true;
}

// ----------------------------------------------------------------------------
//
bool PhilipsHueDriver::writeHueUser( LPCSTR user, LPCSTR bridge_ip )
{
    CString filename;
    filename.Format( "%s\\DMXStudio\\%s.hue", DMXStudio::getUserDocumentDirectory(), bridge_ip );

    FILE* hHue = _fsopen( filename, "wt", _SH_DENYWR );
    if ( !hHue )
        return false;

    fputs( user, hHue );
    fclose( hHue );

    return true;
}

// ----------------------------------------------------------------------------
//
DMX_STATUS PhilipsHueDriver::dmx_discover( DriverFixtureArray& fixtures ) {
    for ( auto it : m_lights ) {
        Light& light = it.second;

        switch ( light.m_device_id ) {
            case HUE_DIMMABLE:
                fixtures.emplace_back( "Philips", "Hue Dimmable", 1, light.m_name, light.dmx() );
                break;

            case HUE_COLOR:
            case HUE_EXTENDED_COLOR:
                fixtures.emplace_back( "Philips", "Hue Color", 6, light.m_name, light.dmx() );
                break;

            case HUE_COLOR_TEMPERATURE:
                fixtures.emplace_back( "Philips", "Hue Color Temperature", 3, light.m_name, light.dmx() );
                break;
        }
    }

    return DMX_OK;
}