/* 
Copyright (C) 2012 Robert DeSantis
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
#include "SimpleJsonBuilder.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
HttpRestServices::HttpRestServices(void) :
    m_sound_sampler( 2 )
{
    m_rest_get_handlers[ DMX_URL_QUERY_SCENES ] = &HttpRestServices::query_scenes;
    m_rest_get_handlers[ DMX_URL_QUERY_CHASES ] = &HttpRestServices::query_chases;
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURES ] = &HttpRestServices::query_fixtures;
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_STATUS ] = &HttpRestServices::query_venue_status;
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_DESCRIBE ] = &HttpRestServices::query_venue_describe;
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURE_DEFINITIONS ] = &HttpRestServices::query_fixture_definitions;
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_LAYOUT ] = &HttpRestServices::query_venue_layout;

    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_BLACKOUT ] = &HttpRestServices::control_venue_blackout;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_WHITEOUT ] = &HttpRestServices::control_venue_whiteout;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_MUSIC_MATCH ] = &HttpRestServices::control_venue_music_match;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_MASTERDIMMER ] = &HttpRestServices::control_venue_masterdimmer;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_STROBE ] = &HttpRestServices::control_venue_strobe;
    m_rest_get_handlers[ DMX_URL_CONTROL_SCENE_SHOW ] = &HttpRestServices::control_scene_show;
    m_rest_get_handlers[ DMX_URL_CONTROL_CHASE_SHOW ] = &HttpRestServices::control_chase_show;
    m_rest_get_handlers[ DMX_URL_CONTROL_FIXTURE_CAPTURE ] = &HttpRestServices::control_fixture_capture;
    m_rest_get_handlers[ DMX_URL_CONTROL_FIXTURE_RELEASE ] = &HttpRestServices::control_fixture_release;
    m_rest_get_handlers[ DMX_URL_CONTROL_FIXTURE_CHANNEL ] = &HttpRestServices::control_fixture_channel;

    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_VOLUME_MASTER ] = &HttpRestServices::control_master_volume;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_VOLUME_MUTE ] = &HttpRestServices::control_mute_volume;   
    m_rest_get_handlers[ DMX_URL_CONTROL_SOUNDSAMPLER_START ] = &HttpRestServices::control_soundsampler_start;
    m_rest_get_handlers[ DMX_URL_CONTROL_SOUNDSAMPLER_STOP ] = &HttpRestServices::control_soundsampler_stop;
    m_rest_get_handlers[ DMX_URL_QUERY_SOUNDSAMPLER ] = &HttpRestServices::query_soundsampler;
    m_rest_get_handlers[ DMX_URL_CONTROL_BEATSAMPLER_STOP ] = &HttpRestServices::control_beatsampler_stop;
    m_rest_get_handlers[ DMX_URL_QUERY_BEATSAMPLER ] = &HttpRestServices::query_beatsampler;
    m_rest_get_handlers[ DMX_URL_CONTROL_ANIMATION_SPEED ] = &HttpRestServices::control_animation_speed;

    m_rest_get_handlers[ DMX_URL_DELETE_SCENE ] = &HttpRestServices::delete_scene;
    m_rest_get_handlers[ DMX_URL_DELETE_CHASE ] = &HttpRestServices::delete_chase;
    m_rest_get_handlers[ DMX_URL_DELETE_FIXTURE ] = &HttpRestServices::delete_fixture;
    m_rest_get_handlers[ DMX_URL_DELETE_FIXTUREGROUP ] = &HttpRestServices::delete_fixturegroup;

    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_BACK ] = &HttpRestServices::control_music_track_back;
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_FORWARD ] = &HttpRestServices::control_music_track_forward;
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_STOP ] = &HttpRestServices::control_music_track_stop;
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_PAUSE ] = &HttpRestServices::control_music_track_pause;
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_PLAY ] = &HttpRestServices::control_music_track_play;
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_PLAYLISTS ] = &HttpRestServices::query_music_playlists;
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_PLAYLIST_TRACKS ] = &HttpRestServices::query_music_playlist_tracks;
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_QUEUED ] = &HttpRestServices::query_music_queued;
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_PLAYED ] = &HttpRestServices::query_music_played;
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_PLAY_TRACK ] = &HttpRestServices::control_music_play_track;
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_PLAY_PLAYLIST ] = &HttpRestServices::control_music_play_playlist;

    // POST request handlers
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTURE_CHANNELS ] = &HttpRestServices::control_fixture_channels;
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTURE ] = &HttpRestServices::control_fixture;
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTUREGROUP ] = &HttpRestServices::control_fixture_group;

    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_COPY_FIXTURES ] = &HttpRestServices::edit_scene_copy_fixtures;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_UPDATE ] = &HttpRestServices::edit_venue_update;
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_CREATE  ] = &HttpRestServices::edit_scene_create;
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_UPDATE ] = &HttpRestServices::edit_scene_update;
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_COPY ] = &HttpRestServices::edit_scene_copy;
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_CREATE  ] = &HttpRestServices::edit_chase_create;
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_UPDATE ] = &HttpRestServices::edit_chase_update;
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_COPY ] = &HttpRestServices::edit_chase_copy;
    m_rest_post_handlers[ DMX_URL_EDIT_FIXTURE_CREATE ] = &HttpRestServices::edit_fixture_create;
    m_rest_post_handlers[ DMX_URL_EDIT_FIXTURE_UPDATE ] = &HttpRestServices::edit_fixture_update;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_LAYOUT_SAVE ] = &HttpRestServices::edit_venue_layout_save;

    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_SAVE ] = &HttpRestServices::edit_venue_save;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_LOAD ] = &HttpRestServices::edit_venue_load;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_NEW ] = &HttpRestServices::edit_venue_new;

    m_rest_post_handlers[DMX_URL_EDIT_FIXTUREGROUP_CREATE ] = &HttpRestServices::edit_fixturegroup_create;
    m_rest_post_handlers[DMX_URL_EDIT_FIXTUREGROUP_UPDATE ] = &HttpRestServices::edit_fixturegroup_update;

    m_rest_post_handlers[ DMX_URL_VENUE_UPLOAD ] = &HttpRestServices::venue_upload;

    m_rest_post_handlers[ DMX_URL_CONTROL_BEATSAMPLER_START ] = &HttpRestServices::control_beatsampler_start;
}

// ----------------------------------------------------------------------------
//
HttpRestServices::~HttpRestServices(void)
{
    m_sound_sampler.detach();
}

// ----------------------------------------------------------------------------
//
DWORD HttpRestServices::processGetRequest( HttpWorkerThread* worker )
{
    try {
        CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
        int pos = path.Find( '?' );
        if ( pos != -1 )                                        // Remove query string
           path = path.Left( pos );

        CString prefix( path );
        if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
            prefix += "/";

        // Invoke the approriate handler
        RestGetHandlerFunc func = NULL;
        UINT len = 0;
        for ( RestGetHandlerMap::iterator it=m_rest_get_handlers.begin(); it != m_rest_get_handlers.end(); ++it ) {
            if ( prefix.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
                func = it->second;
                len = strlen(it->first);
            }
        }

        if ( func != NULL ) {
            CString response;
            if ( (this->*func)( response, path.Mid( len ) ) )
                return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
            return worker->error_501();
        }

        // Perhaps this is a file request
        if ( path.Find( DMX_URL_ROOT, 0 ) == 0 ) {
            path.Replace( DMX_URL_ROOT, "" );

            if ( path == "rest/venue/download/" ) {
                CString venue_xml, venue_xml_name;
                studio.writeVenueToString( venue_xml );
                venue_xml_name.Format( "%s.xml", studio.getVenue()->getName() == NULL ? "venue" : studio.getVenue()->getName() );
                return worker->sendAttachment( (LPBYTE)(LPCSTR)venue_xml, venue_xml.GetLength(), "text/xml", (LPCSTR)venue_xml_name ); 
            }

            return worker->sendFile( (LPCSTR)path, this ); 
        }

        return worker->error_404();
    }
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501();
    }
}

// ----------------------------------------------------------------------------
//
DWORD HttpRestServices::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    try {
        CString url_path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
        if ( url_path.GetLength() > 0 && url_path[url_path.GetLength()-1] != '/' )
            url_path += "/";

        CString contentType( worker->getContentType() );
        int semi = contentType.Find( ";" );
        if ( semi != -1 )
            contentType = contentType.Left( semi );

        if ( studio.isDebug() )
            printf( "url=%s\nContentType=%s\nrequest=%s\n", url_path, (LPCSTR)contentType, contents );

        // Only accept JSON and multipart content types
        if ( contentType != "application/json" && contentType != "multipart/form-data" )
            return worker->sendResponse( 415, "Unsupported content type", NULL );

        // Invoke the approriate handler
        RestPostHandlerFunc func = NULL;
        UINT len = 0;
        for ( RestPostHandlerMap::iterator it=m_rest_post_handlers.begin(); it != m_rest_post_handlers.end(); ++it ) {
            if ( url_path.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
                func = it->second;
                len = strlen(it->first);
            }
        }

        if ( func != NULL ) {
            CString response;
            if ( (this->*func)( response, (LPCSTR)contents, size, (LPCSTR)contentType ) )
                return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
        }

        return worker->error_501();
    }
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501();
    }
}

// ----------------------------------------------------------------------------
//
bool CompareObjectNumber( DObject* o1,  DObject* o2 ) {
    return o1->getNumber() < o2->getNumber();
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
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_release( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id != 0 )
        studio.getVenue()->releaseActor( fixture_id );
    else
        studio.getVenue()->clearAllCapturedActors();

    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_fixture_channel( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
    channel_t channel;
    unsigned channel_value;

    if ( sscanf_s( data, "%lu/%u/%u", &fixture_id, &channel, &channel_value ) != 3 )
        return false;

    Fixture* pf = studio.getVenue()->getFixture( fixture_id );
    if ( !pf )
        return false;

    studio.getVenue()->captureAndSetChannelValue( pf, channel, channel_value );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_master_volume( CString& response, LPCSTR data )
{
    UINT master_volume;
        
    if ( sscanf_s( data, "%u", &master_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolume( master_volume );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_mute_volume( CString& response, LPCSTR data )
{
    UINT mute_volume;
        
    if ( sscanf_s( data, "%u", &mute_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolumeMute( mute_volume ? true : false );

    return true;
}


// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_play_track( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT        playlist_number, track_number;
    unsigned    queue;

    if ( sscanf_s( data, "%lu/%lu/%u", &playlist_number, &track_number, &queue ) != 3 )
        return false;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );
    if ( playlist_number == 0 || playlist_number >playlists.size() )
        return false;

    DWORD playlist_id = playlists[ playlist_number-1 ];

    PlayerItems tracks;
    studio.getMusicPlayer()->getTracks( playlist_id, tracks );
    if ( track_number == 0 || track_number >tracks.size() )
        return false;

    DWORD track_id = tracks[ track_number-1 ];
    studio.getMusicPlayer()->playTrack( track_id, queue ? true : false );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_play_playlist( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT       playlist_number;
    unsigned    queue;

    if ( sscanf_s( data, "%lu/%u", &playlist_number, &queue ) != 2 )
        return false;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );
    if ( playlist_number == 0 || playlist_number >playlists.size() )
        return false;

    DWORD playlist_id = playlists[ playlist_number-1 ];
    studio.getMusicPlayer()->playAllTracks( playlist_id, queue ? true : false );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_playlists( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    JsonBuilder json( response );
    json.startObject();

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );

    UINT playlist_number = 1;

    json.startArray( "playlists" );
    for ( PlayerItems::iterator it=playlists.begin(); it != playlists.end(); it++, playlist_number++ ) {
        json.startObject();
        json.add( "id", playlist_number );
        json.add( "name", studio.getMusicPlayer()->getPlaylistName( (*it) ) );
        json.endObject();
    }
    json.endArray( "playlists" );
        
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_queued( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    PlayerItems tracks;
    studio.getMusicPlayer()->getQueuedTracks(tracks);

    UINT track_number = 1;

    JsonBuilder json( response );
    json.startObject();

    json.startArray( "tracks" );
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ ) {
        json.startObject();
        json.add( "id", track_number );
        json.add( "name", studio.getMusicPlayer()->getTrackFullName( (*it) ) );
        json.endObject();
    }
    json.endArray( "tracks" );
        
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_played( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    PlayerItems tracks;
    studio.getMusicPlayer()->getPlayedTracks(tracks);

    UINT track_number = 1;

    JsonBuilder json( response );
    json.startObject();

    json.startArray( "tracks" );
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ ) {
        json.startObject();
        json.add( "id", track_number );
        json.add( "name", studio.getMusicPlayer()->getTrackFullName( (*it) ) );
        json.endObject();
    }
    json.endArray( "tracks" );
        
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_playlist_tracks( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT playlist_number;

    if ( sscanf_s( data, "%u", &playlist_number ) != 1 )
        return false;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );
    if ( playlist_number == 0 || playlist_number >playlists.size() )
        return false;

    DWORD playlist_id = playlists[ playlist_number-1 ];

    PlayerItems tracks;
    studio.getMusicPlayer()->getTracks( playlist_id, tracks );

    UINT track_number = 1;

    JsonBuilder json( response );
    json.startObject();  
      
    json.startArray( "playlist" );
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ ) {
        json.startObject();
        json.add( "id", track_number );
        json.add( "name", studio.getMusicPlayer()->getTrackFullName( (*it) ) );
        json.endObject();
    }
    json.endArray( "playlist" );
        
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_back( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->backTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_forward( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->forwardTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_stop( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->stopTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_pause( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( true );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_play( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( false );
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
bool HttpRestServices::control_fixture_capture( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id > 0 ) {
        SceneActor* actor = studio.getVenue()->captureActor( fixture_id );

        // Return current channel values
        JsonBuilder json( response );
        json.startArray();
        for ( channel_t ch=0; ch < actor->getNumChannels(); ch++ )
            json.add( actor->getChannelValue( ch ) );
        json.endArray();
    }

    return true;
}


