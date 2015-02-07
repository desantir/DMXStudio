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

#include "HttpRestServices.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_scene_show( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID scene_id;

    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        return false;

    if ( scene_id == 0 )
        scene_id = studio.getVenue()->getDefaultScene() ->getUID();

    Scene* scene = studio.getVenue()->getScene( scene_id );
    if ( !scene )
        return false;

    studio.getVenue()->stopChase();

    studio.getVenue()->selectScene( scene_id );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_chase_show( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID chase_id;
        
    if ( sscanf_s( data, "%lu", &chase_id ) != 1 )
        return false;

    if ( chase_id > 0 ) {
        Chase* chase = studio.getVenue()->getChase( chase_id );
        if ( !chase )
            return false;

        studio.getVenue()->startChase( chase_id );
    }
    else
        studio.getVenue()->stopChase();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_venue_status( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    JsonBuilder json( response );
    json.startObject();
    json.add( "blackout", studio.getVenue()->getUniverse()->isBlackout() );
    json.add( "auto_blackout", studio.getVenue()->isLightBlackout() );
    json.add( "dimmer", studio.getVenue()->getMasterDimmer() );
    json.add( "whiteout", studio.getVenue()->getWhiteout() );
    json.add( "whiteout_strobe", studio.getVenue()->getWhiteoutStrobeMS() );
    json.add( "whiteout_color", studio.getVenue()->getWhiteoutColor() );
    json.add( "animation_speed", studio.getVenue()->getAnimationSampleRate() );
    json.add( "current_scene", studio.getVenue()->getCurrentSceneUID() );
    json.add( "current_chase", studio.getVenue()->getRunningChase() );
    json.add( "master_volume", studio.getVenue()->getMasterVolume() );
    json.add( "mute", studio.getVenue()->isMasterVolumeMute() );
    json.add( "has_music_player", studio.hasMusicPlayer() );
    json.add( "music_match", studio.getVenue()->isMusicSceneSelectEnabled() );
    json.add( "venue_filename", studio.getVenueFileName() );
    json.addArray<UIDArray>( "captured_fixtures", studio.getVenue()->getDefaultScene()->getActorUIDs() );

    // If we have a music player, return player status
    if ( studio.hasMusicPlayer() ) {
        json.startObject( "music_player" );
        json.add( "player_name", studio.getMusicPlayer()->getPlayerName() );
        json.add( "username", studio.getMusicPlayer()->getUsername() );

        if ( studio.getMusicPlayer()->isLoggedIn() ) {
            DWORD length, remaining;
            UINT queued, played;
            DWORD track = studio.getMusicPlayer()->getPlayingTrack( &length, &remaining, &queued, &played );
            
            json.add( "logged_in", true );
            json.add( "mapping", studio.getVenue()->isMusicSceneSelectEnabled() );
            json.add( "queued", queued );
            json.add( "played", played );

            if ( track ) {
                json.startObject( "playing" );
                json.add( "track", track );
                json.add( "name", studio.getMusicPlayer()->getTrackFullName( track ) );
                json.add( "length", length );
                json.add( "remaining", remaining );
                json.add( "paused", studio.getMusicPlayer()->isTrackPaused() );
                json.endObject( "playing" );
            }
        }
        else {
            json.add( "logged_in", false );
        }

        CString last_error = studio.getMusicPlayer()->getLastPlayerError( );
        if ( !last_error.IsEmpty() )
            json.add( "player_error", last_error );

        json.endObject( "music_player" );
    }
    
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_strobe( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UINT whiteout_strobe_ms;

    if ( sscanf_s( data, "%u", &whiteout_strobe_ms ) != 1 )
        return false;
    if ( whiteout_strobe_ms < 25 || whiteout_strobe_ms > 10000)
        return false;

    studio.getVenue()->setWhiteoutStrobeMS( whiteout_strobe_ms );

    if ( studio.getVenue()->getWhiteout() != WHITEOUT_OFF )
        studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_whiteout_color( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    ULONG rgbwa;
    if ( data[0] == '#' )
        data++;
    if ( sscanf_s( data, "%lx", &rgbwa ) != 1 )
        return false;

    studio.getVenue()->setWhiteoutColor( RGBWA(rgbwa) );

    if ( studio.getVenue()->getWhiteout() != WHITEOUT_OFF )
        studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_blackout( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    unsigned blackout;
        
    if ( sscanf_s( data, "%u", &blackout ) != 1 )
        return false;

    studio.getVenue()->getUniverse()->setBlackout( blackout ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_music_match( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    unsigned music_match;
        
    if ( sscanf_s( data, "%u", &music_match ) != 1 )
        return false;

    studio.getVenue()->setMusicSceneSelectEnabled( music_match ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_whiteout( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    int whiteout;

    if ( sscanf_s( data, "%d", &whiteout ) != 1 )
        return false;
    if ( whiteout < 0 || whiteout > 4)
        return false;

    studio.getVenue()->setWhiteout( (WhiteoutMode)whiteout );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_venue_masterdimmer( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    int dimmer;

    if ( sscanf_s( data, "%d", &dimmer ) != 1 )
        return false;
    if ( dimmer < 0 || dimmer > 100 )
        return false;

    studio.getVenue()->setMasterDimmer( dimmer );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_animation_speed( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    DWORD sample_rate_ms;
        
    if ( sscanf_s( data, "%lu", &sample_rate_ms ) != 1 )
        return false;

    studio.getVenue()->setAnimationSampleRate( sample_rate_ms );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_venue_layout( CString& response, LPCSTR data ) {
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
bool HttpRestServices::edit_venue_layout_save( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {
    Venue* venue = studio.getVenue();
    if ( !venue )
        return false;

    venue->setVenueLayout( data );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_venue_describe( CString& response, LPCSTR data ) {
    Venue* venue = studio.getVenue();
    if ( !venue )
        return false;

    JsonBuilder json( response );

    json.startObject();
    json.add( "name", venue->getName() );
    json.add( "description", venue->getDescription() );
    json.add( "auto_blackout", venue->getAutoBlackout() );
    json.add( "dmx_port", venue->getDmxPort() );
    json.add( "dmx_packet_delay_ms", venue->getDmxPacketDelayMS() );
    json.add( "dmx_minimum_delay_ms", venue->getDmxMinimumDelayMS() );
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
bool HttpRestServices::edit_venue_update( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {

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
bool HttpRestServices::edit_venue_save( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
bool HttpRestServices::edit_venue_load( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
bool HttpRestServices::edit_venue_new( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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
bool HttpRestServices::venue_upload( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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