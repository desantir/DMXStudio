/* 
Copyright (C) 2011-2013 Robert DeSantis
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

#include "DMXHttpFull.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::query_venue_layout( CString& response, LPCSTR data ) {
    Venue* venue = studio.getVenue();
    if ( !venue )
        return false;

    LPCSTR layout = venue->getVenueLayout();
    if ( layout == NULL )
        return false;

    response = layout; 
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_venue_layout_save( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    Venue* venue = studio.getVenue();
    if ( !venue )
        return false;

    venue->setVenueLayout( data );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::query_venue_describe( CString& response, LPCSTR data ) {
    Venue* venue = studio.getVenue();
    if ( !venue )
        return false;

    response.Format( "{ \"name\":\"%s\", \"description\":\"%s\", \"auto_blackout\":%u, \"dmx_port\":\"%s\", \"dmx_packet_delay_ms\":%u, \"dmx_minimum_delay_ms\":%u, ",
                    encodeJsonString( venue->getName() ), 
                    encodeJsonString( venue->getDescription() ),
                    venue->getAutoBlackout(),
                    encodeJsonString( venue->getDmxPort() ),
                    venue->getDmxPacketDelayMS(),
                    venue->getDmxMinimumDelayMS() );

    response.AppendFormat( "\"audio_capture_device\":\"%s\", \"audio_sample_size\":%u, \"audio_boost\":%f, \"audio_boost_floor\":%f, ",
                    encodeJsonString( venue->getAudioCaptureDevice() ), 
                    venue->getAudioSampleSize(),
                    venue->getAudioBoost(),
                    venue->getAudioBoostFloor() );

    response.AppendFormat( "\"ports\":[\"com1\",\"com2\",\"com3\",\"com4\",\"com5\",\"com6\",\"com7\",\"com8\",\"com9\",\"com10\",\"com11\",\"com12\"], " );

    response.AppendFormat( "\"capture_devices\":[" );

    for ( AudioCaptureDeviceArray::iterator it=AudioInputStream::audioCaptureDevices.begin();
            it != AudioInputStream::audioCaptureDevices.end(); ++it ) {
        if ( it != AudioInputStream::audioCaptureDevices.begin() )
            response.AppendFormat( "," );
        response.AppendFormat( "\"%s\"", encodeJsonString ((*it).m_friendly_name) );
    }

    response.AppendFormat( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_venue_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {

    Venue* venue = studio.getVenue();
    if ( !venue )
        return false;

    SimpleJsonParser parser;

    try {
        parser.parse( data );

        CString name = parser.get<CString>( "name" );
        CString description = parser.get<CString>( "description" );
        CString dmx_port = parser.get<CString>( "dmx_port" );
        CString audio_capture_device = parser.get<CString>( "audio_capture_device" );
        int dmx_packet_delay_ms = parser.get<int>( "dmx_packet_delay_ms" );
        int dmx_minimum_delay_ms = parser.get<int>( "dmx_minimum_delay_ms" );
        int audio_sample_size = parser.get<int>( "audio_sample_size" );
        float audio_boost = parser.get<float>( "audio_boost" );
        float audio_boost_floor = parser.get<float>( "audio_boost_floor" );
        int auto_blackout = parser.get<int>( "auto_blackout" );

        // There may be a better solution for this, but we need to kill all attached sound devices before the reset
        m_sound_sampler.detach();

        venue->close();

        venue->setName( name );
        venue->setDescription( description );
        venue->setDmxPort( dmx_port );
        venue->setDmxPacketDelayMS( dmx_packet_delay_ms );
        venue->setDmxMinimumDelayMS( dmx_minimum_delay_ms );
        venue->setAudioCaptureDevice( audio_capture_device );
        venue->setAudioBoost( audio_boost );
        venue->setAudioBoostFloor( audio_boost_floor );
        venue->setAudioSampleSize( audio_sample_size );
        venue->setAutoBlackout( auto_blackout );

        venue->open();
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_venue_save( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
   if ( !studio.getVenue() )
        return false;

    SimpleJsonParser parser;
    CString venue_filename;

    try {
        parser.parse( data );
        venue_filename = parser.get<CString>( "venue_filename" );
        venue_filename.Replace( "\\\\", "\\" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    return studio.saveVenueToFile( (LPCSTR)venue_filename );
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_venue_load( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    CString venue_filename;

    try {
        parser.parse( data );
        venue_filename = parser.get<CString>( "venue_filename" );
        venue_filename.Replace( "\\\\", "\\" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    return studio.loadVenueFromFile( (LPCSTR)venue_filename );
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_venue_new( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    CString reset_what;

    try {
        parser.parse( data );
        reset_what = parser.get<CString>( "reset_what" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( reset_what == "new" ) {
        studio.newVenue();
    }
    else if ( reset_what == "chases" ) {
        studio.getVenue()->deleteAllChases();
    }
    else if ( reset_what == "groups" ) {
        studio.getVenue()->deleteAllFixtureGroups();
    }
    else if ( reset_what == "scenes" ) {
        studio.getVenue()->deleteAllScenes();
    }
    else
        return false;

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::venue_upload( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    /*
        -----------------------------7dd1a41420546
        Content-Disposition: form-data; name="lsd_upload_file"; filename="d:\Users\bobby\Desktop\a small venue.xml"
        Content-Type: text/xml
    */
    CString xml( data );

    int index1 = xml.Find( "\r\n" );
    CString marker = xml.Left( index1 );
    index1 += 2;
    int index2 = xml.Find( "\r\n", index1 );
    CString content_disposition = xml.Mid( index1, index2-index1 );
    index1 = index2+2;
    index2 = xml.Find( "\n", index1 );
    CString type = xml.Mid( index1, index2-index1 );
    index1 = index2+2;

    // Extract XML
    index2 = xml.Find( marker, index1 );
    if ( index2 == -1 )
        return false;
    xml.SetAt( index2, '\0' );

    return studio.loadVenueFromString( &((LPCSTR)xml)[index1] );
}