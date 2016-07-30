/* 
Copyright (C) 2011-2016 Robert DeSantis
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

#include "HttpRestServices.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_scene_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID scene_id;

    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        return false;

    if ( scene_id == 0 )
        scene_id = venue->getDefaultScene() ->getUID();

    venue->selectScene( scene_id );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_scene_stage( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID scene_id;
    SceneLoadMethod method;

    if ( sscanf_s( data, "%lu/%d", &scene_id, &method ) != 2 )
        return false;

    if ( scene_id == 0 )
        scene_id = venue->getDefaultScene()->getUID();

    venue->stopChase();

    venue->playScene( scene_id, method );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_chase_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID chase_id;
        
    if ( sscanf_s( data, "%lu", &chase_id ) != 1 )
        return false;

    if ( chase_id > 0 ) {
        Chase* chase = venue->getChase( chase_id );
        if ( !chase )
            return false;

        venue->startChase( chase_id );
    }
    else
        venue->stopChase();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_venue_status( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    JsonBuilder json( response );
    json.startObject();
    json.add( "blackout", venue->isForceBlackout() );
    json.add( "auto_blackout", venue->isMuteBlackout() );
    json.add( "dimmer", venue->getMasterDimmer() );
    json.add( "whiteout", venue->getWhiteout() );
    json.add( "whiteout_strobe", venue->getWhiteoutStrobeMS() );
    json.add( "whiteout_color", venue->getWhiteoutColor() );
    json.add( "animation_speed", venue->getAnimationSampleRate() );
    json.add( "current_scene", venue->getCurrentSceneUID() );
    json.add( "current_chase", venue->getRunningChase() );
    json.add( "master_volume", venue->getMasterVolume() );
    json.add( "mute", venue->isMasterVolumeMute() );
    json.add( "has_music_player", studio.hasMusicPlayer() );
    json.add( "music_match", venue->isMusicSceneSelectEnabled() );
    json.add( "venue_filename", studio.getVenueFileName() );
    json.add( "dmx_max_universes", DMX_MAX_UNIVERSES );
    json.add( "default_scene_uid", venue->getDefaultScene()->getUID() );

    json.addArray<UIDArray>( "captured_fixtures", venue->getDefaultScene()->getActorUIDs() );

    musicPlayerToJson( json );
    
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_strobe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UINT whiteout_strobe_ms;

    if ( sscanf_s( data, "%u", &whiteout_strobe_ms ) != 1 )
        return false;
    if ( whiteout_strobe_ms < 25 || whiteout_strobe_ms > 10000)
        return false;

    venue->setWhiteoutStrobeMS( whiteout_strobe_ms );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_whiteout_color( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    ULONG rgbwa;
    if ( data[0] == '#' )
        data++;
    if ( sscanf_s( data, "%lx", &rgbwa ) != 1 )
        return false;

    venue->setWhiteoutColor( RGBWA(rgbwa) );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_blackout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    unsigned blackout;
        
    if ( sscanf_s( data, "%u", &blackout ) != 1 )
        return false;

    venue->setForceBlackout( blackout ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_music_match( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    unsigned music_match;
        
    if ( sscanf_s( data, "%u", &music_match ) != 1 )
        return false;

    venue->setMusicSceneSelectEnabled( music_match ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_whiteout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    int whiteout;

    if ( sscanf_s( data, "%d", &whiteout ) != 1 )
        return false;
    if ( whiteout < 0 || whiteout > 4)
        return false;

    venue->setWhiteout( (WhiteoutMode)whiteout );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_masterdimmer( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    int dimmer;

    if ( sscanf_s( data, "%d", &dimmer ) != 1 )
        return false;
    if ( dimmer < 0 || dimmer > 100 )
        return false;

    venue->setMasterDimmer( dimmer );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_animation_speed( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    DWORD sample_rate_ms;
        
    if ( sscanf_s( data, "%lu", &sample_rate_ms ) != 1 )
        return false;

    venue->setAnimationSampleRate( sample_rate_ms );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_venue_layout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    LPCSTR layout = venue->getVenueLayout();
    if ( layout == NULL )
        return false;

    response = layout; 
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_venue_layout_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    venue->setVenueLayout( data );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_venue_describe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    JsonBuilder json( response );

    json.startObject();
    json.add( "name", venue->getName() );
    json.add( "description", venue->getDescription() );
    json.add( "auto_blackout", venue->getAutoBlackoutMS() );
    json.add( "audio_capture_device", venue->getAudioCaptureDevice() );
    json.add( "audio_sample_size", venue->getAudioSampleSize() );
    json.add( "audio_boost", venue->getAudioBoost() );
    json.add( "audio_boost_floor", venue->getAudioBoostFloor() );

    json.startArray( "ports" );
    for ( int i=1; i <= 12; i++ ) {
        CString com_port;
        com_port.Format( "com%d", i );
        json.add( com_port );
    }
    json.endArray( "ports" );

    json.startArray("driver_types");
    json.add( "Open DMX");
    json.add( "Enttec USB Pro");
    json.endArray("driver_types");

    json.startArray( "universes" );
    UniversePtrArray universes = venue->getUniverses();
    for ( UniversePtrArray::iterator it=universes.begin(); it != universes.end(); ++it ) {
        Universe* universe = (*it);

        json.startObject( );
        json.add( "id", universe->getId() );
        json.add( "type", universe->getType() );
        json.add( "dmx_port", universe->getDmxPort() );
        json.add( "packet_delay_ms", universe->getDmxPacketDelayMS() );
        json.add( "minimum_delay_ms", universe->getDmxMinimumDelayMS() );
        json.endObject( );
    }
    json.endArray( "universes" );

    json.startArray( "capture_devices" );
    for ( AudioCaptureDeviceArray::iterator it=AudioInputStream::audioCaptureDevices.begin();
            it != AudioInputStream::audioCaptureDevices.end(); ++it )
        json.add( (*it).m_friendly_name );

    json.endArray( "capture_devices" );

    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_venue_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;

    try {
        parser.parse( data );

        CString name = parser.get<CString>( "name" );
        CString description = parser.get<CString>( "description" );
        CString audio_capture_device = parser.get<CString>( "audio_capture_device" );
        float audio_boost = parser.get<float>( "audio_boost" );
        float audio_boost_floor = parser.get<float>( "audio_boost_floor" );
        int audio_sample_size = parser.get<int>( "audio_sample_size" );
        int auto_blackout = parser.get<int>( "auto_blackout" );
        JsonNodePtrArray universeParsers = parser.getObjects("universes");

        std::vector<Universe> universes;

        for (  JsonNode* univ : universeParsers ) {
            unsigned id = univ->get<unsigned>( "id" );
            UniverseType type = (UniverseType)univ->get<unsigned>( "type" );
            CString dmx_port = univ->get<CString>( "dmx_port" );
            unsigned dmx_packet_delay_ms = univ->get<unsigned>( "packet_delay_ms" );
            unsigned dmx_minimum_delay_ms = univ->get<unsigned>( "minimum_delay_ms" );

            universes.push_back( Universe( id, type, dmx_port, dmx_packet_delay_ms, dmx_minimum_delay_ms ) );
        }

        venue->configure( name, description, audio_capture_device, audio_boost, audio_boost_floor, audio_sample_size, auto_blackout, universes );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_venue_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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

    return studio.saveVenueToFile( (LPCSTR)venue_filename );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_venue_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
bool HttpRestServices::edit_venue_new( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
        venue->deleteAllChases();
    }
    else if ( reset_what == "groups" ) {
        venue->deleteAllFixtureGroups();
    }
    else if ( reset_what == "scenes" ) {
        venue->deleteAllScenes();
    }
    else
        return false;

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::venue_upload( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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