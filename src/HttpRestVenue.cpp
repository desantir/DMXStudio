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
#include "MimeDecoder.h"

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_scene_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID scene_id;

    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( scene_id == 0 )
        scene_id = venue->getDefaultScene() ->getUID();

    venue->selectScene( scene_id );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_scene_stage( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID scene_id;
    SceneLoadMethod method;

    if ( sscanf_s( data, "%lu/%d", &scene_id, &method ) != 2 )
        throw RestServiceException( "Invalid service arguments" );

    if ( scene_id == 0 )
        scene_id = venue->getDefaultScene()->getUID();

    venue->stopChase();

    venue->playScene( scene_id, method );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_chase_show( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID chase_id;
        
    if ( sscanf_s( data, "%lu", &chase_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( chase_id > 0 ) {
        Chase* chase = venue->getChase( chase_id );
        if ( !chase )
            throw RestServiceException( "Invalid chase UID" );

        venue->startChase( chase_id );
    }
    else
        venue->stopChase();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_venue_status( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    JsonBuilder json( response );
    json.startObject();
	json.add( "session_id", session->getId() );
    json.add( "blackout", venue->isForceBlackout() );
    json.add( "auto_blackout", venue->isMuteBlackout() );
    json.add( "dimmer", venue->getMasterDimmer() );
    json.add( "whiteout", venue->getWhiteout() );
    json.add( "whiteout_strobe_ms", venue->getWhiteoutStrobeMS() );
    json.add( "whiteout_fade_ms", venue->getWhiteoutFadeMS() );
    json.add( "whiteout_color", venue->getWhiteoutColor() );
    json.add( "whiteout_effect", venue->getWhiteoutEffect() );
    json.add( "current_scene", venue->getCurrentSceneUID() );
    json.add( "current_chase", venue->getRunningChase() );
    json.add( "master_volume", venue->getMasterVolume() );
    json.add( "mute", venue->isMasterVolumeMute() );
    json.add( "has_music_player", studio.hasMusicPlayer() );
    json.add( "music_match", venue->isMusicSceneSelectEnabled() );
    json.add( "venue_filename", studio.getVenueFileName() );
    json.add( "dmx_max_universes", DMX_MAX_UNIVERSES );
    json.add( "default_scene_uid", venue->getDefaultScene()->getUID() );
    json.add( "animation_speed", venue->getAnimationSpeed() );
    json.add( "track_fixtures", venue->isTrackFixtures() );

    json.addArray<UIDArray>( "captured_fixtures", venue->getDefaultScene()->getActorUIDs() );

    musicPlayerToJson( json );
    
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_strobe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UINT whiteout_strobe_ms, whiteout_fade_ms;

    if ( sscanf_s( data, "%u/%u", &whiteout_strobe_ms, &whiteout_fade_ms ) != 2 )
        throw RestServiceException( "Invalid service arguments" );
    if ( whiteout_strobe_ms < 25 || whiteout_strobe_ms > 10000 || whiteout_fade_ms > 10000 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setWhiteoutStrobeMS( whiteout_strobe_ms, whiteout_fade_ms );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_whiteout_color( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    ULONG rgbwa;
    if ( data[0] == '#' )
        data++;
    if ( sscanf_s( data, "%lx", &rgbwa ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setWhiteoutColor( RGBWA(rgbwa) );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_whiteout_effect( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    unsigned effect;
    if ( sscanf_s( data, "%u", &effect ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setWhiteoutEffect( (WhiteoutEffect)effect );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_whiteout_palette( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;

    try {
        parser.parse( data );

        RGBWAArray palette_colors = parser.getArrayAsVector<RGBWA>( "palette" );
        ColorWeights palette_weights;

        if ( parser.has_key( "weights" ) )
            palette_weights = parser.getArrayAsVector<double>( "weights" );

        UINT duration = parser.get<UINT>( "duration_ms" );

        venue->setVideoPalette( palette_colors, palette_weights );

        venue->setWhiteoutColor( SYSTEM_PALETTE_4 );
        venue->setWhiteoutStrobeMS( duration, 0 );
        venue->setWhiteout( WhiteoutMode::WHITEOUT_ON );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_blackout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    unsigned blackout;
        
    if ( sscanf_s( data, "%u", &blackout ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setForceBlackout( blackout ? true : false );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_music_match( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    unsigned music_match;
        
    if ( sscanf_s( data, "%u", &music_match ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setMusicSceneSelectEnabled( music_match ? true : false );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_animation_speed( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    unsigned animation_speed;

    if ( sscanf_s( data, "%u", &animation_speed ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( animation_speed < 1 || animation_speed > 10000 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setAnimationSpeed( animation_speed );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_whiteout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    int whiteout;

    if ( sscanf_s( data, "%u", &whiteout ) != 1 )
        throw RestServiceException( "Invalid service arguments" );
    if ( whiteout < 0 || whiteout > 5)
        throw RestServiceException( "Invalid service arguments" );

    venue->setWhiteout( (WhiteoutMode)whiteout );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_venue_masterdimmer( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    int dimmer;

    if ( sscanf_s( data, "%u", &dimmer ) != 1 )
        throw RestServiceException( "Invalid service arguments" );
    if ( dimmer < 0 || dimmer > 100 )
        throw RestServiceException( "Invalid service arguments" );

    venue->setMasterDimmer( dimmer );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_venue_layout( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    LPCSTR layout = venue->getVenueLayout();
    if ( layout == NULL )
        response.Empty();
	else
		response = layout; 
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_venue_layout_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    venue->setVenueLayout( data );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_venue_describe( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) {
    JsonBuilder json( response );

    json.startObject();
    json.add( "name", venue->getName() );
    json.add( "description", venue->getDescription() );
    json.add( "auto_blackout", venue->getAutoBlackoutMS() );
    json.add( "audio_capture_device", venue->getAudioCaptureDevice() );
    json.add( "audio_sample_size", venue->getAudioSampleSize() );
    json.add( "audio_boost", venue->getAudioBoost() );
    json.add( "audio_boost_floor", venue->getAudioBoostFloor() );
    json.add( "track_fixtures", venue->isTrackFixtures() );

    json.startArray( "ports" );
    for ( int i=1; i <= 12; i++ ) {
        CString com_port;
        com_port.Format( "com%d", i );
        json.add( com_port );
    }
    json.endArray( "ports" );

    json.startArray("driver_types");
    json.add( "Enttec USB Pro");
    json.add( "Open DMX");
    json.add( "Philips Hue");
    json.endArray("driver_types");

    json.startArray( "universes" );
    UniversePtrArray universes = venue->getUniverses();
    for ( UniversePtrArray::iterator it=universes.begin(); it != universes.end(); ++it ) {
        Universe* universe = (*it);

        json.startObject( );
        json.add( "id", universe->getId() );
        json.add( "type", universe->getType() );
        json.add( "dmx_config", universe->getDmxConfig() );
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
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_venue_update( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
        bool track_fixtures = parser.get<bool>( "track_fixtures" );
        JsonNodePtrArray universeParsers = parser.getObjects("universes");

        std::vector<Universe> universes;

        for (  JsonNode* univ : universeParsers ) {
            unsigned id = univ->get<unsigned>( "id" );
            UniverseType type = (UniverseType)univ->get<unsigned>( "type" );
            CString dmx_config = univ->get<CString>( "dmx_config" );
            unsigned dmx_packet_delay_ms = univ->get<unsigned>( "packet_delay_ms" );
            unsigned dmx_minimum_delay_ms = univ->get<unsigned>( "minimum_delay_ms" );

            universes.emplace_back( id, type, dmx_config, dmx_packet_delay_ms, dmx_minimum_delay_ms );
        }

        venue->configure( name, description, audio_capture_device, audio_boost, audio_boost_floor, audio_sample_size, 
                          auto_blackout, track_fixtures, universes );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_venue_save( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    CString venue_filename;

    try {
        parser.parse( data );
        venue_filename = parser.get<CString>( "venue_filename" );
        venue_filename.Replace( "\\\\", "\\" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( !studio.saveVenueToFile( (LPCSTR)venue_filename ) )
        throw RestServiceException( "Unable to save venue file '%s'", venue_filename );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_venue_load( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    CString venue_filename;

    try {
        parser.parse( data );
        venue_filename = parser.get<CString>( "venue_filename" );
        venue_filename.Replace( "\\\\", "\\" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( !studio.loadVenueFromFile( (LPCSTR)venue_filename ) ) 
        throw RestServiceException( "Unable to load venue file '%s'", venue_filename );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_venue_new( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    CString reset_what;

    try {
        parser.parse( data );
        reset_what = parser.get<CString>( "reset_what" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
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
		venue->deleteAllChases();
        venue->deleteAllScenes();
		venue->deleteAllAnimations();
    }
    else
        throw RestServiceException( "Invalid venue reset object" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::venue_upload( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
        throw RestServiceException( "Invalid service arguments" );

    xml.SetAt( index2, '\0' );

    if ( !studio.loadVenueFromString( &((LPCSTR)xml)[index1] ) )
        throw RestServiceException( "Unable to upload venue file" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::HttpRestServices::query_animation_levels( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    DWORD after_ms;

    if ( sscanf_s( data, "%lu", &after_ms ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    JsonBuilder json( response );

    json.startArray();

    for ( AnimationLevelMap::value_type entry : venue->loadAnimationLevelData( after_ms ) ) {
        json.startObject();
        json.add( "animation_id", entry.first );
        json.startArray( "data" );

        for ( LevelRecord data : entry.second ) {
            json.startObject();
            json.add( "ms", data.time_ms );
            json.add( "level", data.level );
            json.endObject();
        }

        json.endArray( "data" );
        json.endObject();
    }

    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::venue_create_quickscene(Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type)
{
	SimpleJsonParser parser;

	UIDArray fixtures;
	RGBWA color;
	QuickSceneEffect effect;
	unsigned color_speed_ms, move_speed_ms;
	QuickSceneMovement movement;
    bool multi_color;

	try {
		parser.parse( data );
		fixtures = parser.getArrayAsVector<UID>( "fixtures" );
		effect = (QuickSceneEffect)parser.get<unsigned>( "effect" );
		color_speed_ms = parser.get<unsigned>( "color_speed_ms" );
        move_speed_ms = parser.get<unsigned>( "move_speed_ms" );
		movement = (QuickSceneMovement)parser.get<unsigned>( "movement" );
		color = parser.get<RGBWA>( "color" );
        multi_color = parser.get<bool>( "multi_color" );
	}
	catch ( std::exception& e ) {
		throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
	}

	venue->createQuickScene( fixtures, effect, color, color_speed_ms, movement, move_speed_ms, multi_color );
}


// ----------------------------------------------------------------------------
//
void HttpRestServices::control_quickscene_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data ) 
{
    venue->stopQuickScene();
}