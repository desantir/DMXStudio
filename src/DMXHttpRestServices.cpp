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

#include "DMXHttpRestServices.h"
#include "SimpleJsonBuilder.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::query_venue_status( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_venue_blackout( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_venue_music_match( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_venue_whiteout( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_venue_masterdimmer( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_scene_show( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_chase_show( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_venue_strobe( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_fixture_release( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_fixture_channel( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_master_volume( CString& response, LPCSTR data )
{
    UINT master_volume;
        
    if ( sscanf_s( data, "%u", &master_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolume( master_volume );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_mute_volume( CString& response, LPCSTR data )
{
    UINT mute_volume;
        
    if ( sscanf_s( data, "%u", &mute_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolumeMute( mute_volume ? true : false );

    return true;
}


// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_music_play_track( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_music_play_playlist( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::query_music_playlists( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::query_music_queued( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::query_music_played( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::query_music_playlist_tracks( CString& response, LPCSTR data )
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
bool DMXHttpRestServices::control_music_track_back( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->backTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_music_track_forward( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->forwardTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_music_track_stop( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->stopTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_music_track_pause( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( true );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_music_track_play( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( false );
    return true;
}

