/* 
    Copyright (C) 2012-14 Robert DeSantis
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
bool HttpRestServices::player_login( DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
   if ( !studio.getVenue() )
        return false;

    SimpleJsonParser parser;
    CString username;
    CString password;

    try {
        parser.parse( data );

        username = parser.get<CString>( "username" );
        password = parser.get<CString>( "password" );
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    JsonBuilder json( response );
    json.startObject();

    if ( studio.getMusicPlayer()->signon( username, password ) ) {
        json.add( "logged_in", true );
    }
    else {
        CString login_error = studio.getMusicPlayer()->getLastPlayerError();

        if ( login_error.IsEmpty() )
            login_error = "Login failed";

        json.add( "logged_in", false );
        json.add( "login_error", login_error );
    }

    json.endObject();
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_master_volume( DMXHttpSession* session, CString& response, LPCSTR data )
{
    UINT master_volume;
        
    if ( sscanf_s( data, "%u", &master_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolume( master_volume );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_mute_volume( DMXHttpSession* session, CString& response, LPCSTR data )
{
    UINT mute_volume;
        
    if ( sscanf_s( data, "%u", &mute_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolumeMute( mute_volume ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_queue_track( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    char        track_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", track_link, MAX_LINK_SIZE ) != 1 )
        return false;

    studio.getMusicPlayer()->queueTrack( track_link );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_play_track( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    char        track_link[MAX_LINK_SIZE];
    DWORD       seek_ms = 0L;

    CString scan_data = data;
    scan_data.Replace( "/", " " );

    if ( sscanf_s( scan_data, "%s %lu", track_link, MAX_LINK_SIZE, &seek_ms ) < 1 )
        return false;

    return studio.getMusicPlayer()->playTrack( track_link, seek_ms );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_track_analysis( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    char        track_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", track_link, MAX_LINK_SIZE ) != 1 )
        return false;

    AnalyzeInfo* info;

    if ( !studio.getMusicPlayer()->getTrackAnalysis( track_link, &info ) )
        return false;

    JsonBuilder json( response );

    json.startObject();
    json.add( "link", info->link );

    std::vector<uint16_t> amplitude_data( std::begin( info->data), &info->data[info->data_count] );

    json.startObject( "amplitude" );
    json.add( "duration_ms", info->duration_ms );
    json.add( "data_count", info->data_count );
    json.addArray< std::vector<uint16_t>>( "data", amplitude_data );
    json.endObject( "amplitude" );

    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_play_playlist( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    char        playlist_link[MAX_LINK_SIZE];
    unsigned    queue;

    CString scan_data = data;
    scan_data.Replace( "/", " " );

    if ( sscanf_s( scan_data, "%s %u", playlist_link, MAX_LINK_SIZE, &queue ) != 2 )
        return false;

    return studio.getMusicPlayer()->playAllTracks( playlist_link, queue ? true : false );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_playlists( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    JsonBuilder json( response );
    json.startObject();

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );

    json.startArray( "playlists" );
    for ( PlayerItems::iterator it=playlists.begin(); it != playlists.end(); it++ ) {
        json.startObject();
        json.add( "link", (*it) );
        json.add( "name", studio.getMusicPlayer()->getPlaylistName( (*it) ) );
        json.endObject();
    }
    json.endArray( "playlists" );
        
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool json_track_list( PlayerItems& tracks, CString& response )
{
    UINT track_number = 1;
    AudioInfo audioInfo;

    JsonBuilder json( response );
    json.startObject();

    json.startArray( "tracks" );
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ ) {
        CString track_name, artist_name, album_name, full_name;
        DWORD track_duration_ms;
        bool starred; 

        if ( !studio.getMusicPlayer()->getTrackInfo( (*it), &track_name, &artist_name, &album_name, &track_duration_ms, &starred ) )
            continue;

        json.startObject();
        json.add( "link", (*it) );
        json.add( "number", track_number );
        json.add ("full_name", studio.getMusicPlayer()->getTrackFullName( (*it) ) );    // Legacy for music match
        json.add( "track_name", track_name );
        json.add( "artist_name", artist_name );
        json.add( "album_name", album_name );
        json.add( "duration", track_duration_ms ); 
        json.add( "starred", starred );

        // Collect any audio info available in the cache and queue up the rest
        if ( studio.getMusicPlayer()->getTrackAudioInfo( (*it), &audioInfo, 0L ) == OK ) {
            json.startObject( "audio_info" );
            json.add( "key", audioInfo.key );
            json.add( "mode", audioInfo.mode );
            json.add( "bpm", audioInfo.tempo );
            json.endObject( "audio_info" );
        }

        json.endObject();
    }
    json.endArray( "tracks" );
        
    json.endObject();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_queued( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    PlayerItems tracks;
    studio.getMusicPlayer()->getQueuedTracks(tracks);

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_played( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    PlayerItems tracks;
    studio.getMusicPlayer()->getPlayedTracks(tracks);

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_playlist_tracks( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    char playlist_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", playlist_link, MAX_LINK_SIZE ) != 1 )
        return false;

    PlayerItems tracks;
    studio.getMusicPlayer()->getTracks( playlist_link, tracks );

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_back( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->backTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_forward( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->forwardTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_stop( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->stopTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_pause( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( true );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_music_track_play( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( false );
    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_matcher( DMXHttpSession* session, CString& response, LPCSTR data )
{
   if ( !studio.getVenue() )
        return false;

    MusicSceneSelectMap& mm = studio.getVenue()->music_scene_select_map();
    JsonBuilder json( response );

    json.startArray( );

    json.startObject();
    if ( mm.find( SILENCE_TRACK_LINK ) != mm.end() ) {
        MusicSceneSelector& silence = mm[SILENCE_TRACK_LINK];
        json.add( "track", silence.m_track_full_name );
        json.add( "id", silence.m_selection_uid );
        json.add( "type", silence.m_selection_type );
        json.add( "link", SILENCE_TRACK_LINK );
        json.add( "special", true );
    }
    else {
        json.add( "track", SILENCE_TRACK_NAME );
        json.add( "id", studio.getVenue()->getDefaultScene()->getUID() );
        json.add( "type", MST_SCENE );
        json.add( "link", SILENCE_TRACK_LINK );
        json.add( "special", true );
    }
    json.endObject();

    json.startObject();
    if ( mm.find( UNMAPPED_TRACK_LINK ) != mm.end() ) {
        MusicSceneSelector& unmapped = mm[UNMAPPED_TRACK_LINK];
        json.add( "track", unmapped.m_track_full_name );
        json.add( "id", unmapped.m_selection_uid );
        json.add( "type", unmapped.m_selection_type );
        json.add( "link", UNMAPPED_TRACK_LINK );
        json.add( "special", true );
    }
    else {
        json.add( "track", UNMAPPED_TRACK_NAME );
        json.add( "id", 0 );
        json.add( "type", MST_RANDOM_SCENE );
        json.add( "link", UNMAPPED_TRACK_LINK );
        json.add( "special", true );
    }
    json.endObject();

    for ( MusicSceneSelectMap::iterator it=mm.begin(); it != mm.end(); ++it ) {
        MusicSceneSelector& mss = it->second;

        if ( mss.m_track_link == SILENCE_TRACK_LINK || 
             mss.m_track_link == UNMAPPED_TRACK_LINK )
            continue;

        json.startObject();
        json.add( "track", mss.m_track_full_name );
        json.add( "link", mss.m_track_link );
        json.add( "id", mss.m_selection_uid );
        json.add( "type", mss.m_selection_type );
        json.endObject();
    }
    json.endArray( );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_matcher_search( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.getVenue() )
        return false;

    char playlist_link[MAX_LINK_SIZE];

    if ( sscanf_s( data, "%s", playlist_link, MAX_LINK_SIZE ) != 1 )
        return false;

    MusicSceneSelectMap& mm = studio.getVenue()->music_scene_select_map();
    JsonBuilder json( response );

    json.startArray( );

    MusicSceneSelector selector;

    if ( studio.getVenue()->findMusicMapping( playlist_link, selector ) ) {
        json.startObject();

        json.add( "track", selector.m_track_full_name );
        json.add( "id", selector.m_selection_uid );
        json.add( "type", selector.m_selection_type );
        json.add( "link", selector.m_track_link );
        json.add( "special", selector.isSpecialTrack() );
  
        json.endObject();
    }

    json.endArray( );

    return true;
}

// ----------------------------------------------------------------------------
//
static bool music_matcher_load( LPCSTR data, boolean clearFirst ) {
    if ( !studio.getVenue() )
        return false;

    SimpleJsonParser parser;

    std::vector<MusicSceneSelector> selections;

    try {
        parser.parse( data );

        PARSER_LIST selection_parsers = parser.get<PARSER_LIST>( "" );

        for ( PARSER_LIST::iterator it=selection_parsers.begin(); it != selection_parsers.end(); ++it ) {
            UID selection_id = (*it).get<UID>( "id" );
            CString selection_name = (*it).get<CString>( "track" );
            CString selection_link = (*it).get<CString>( "link" );
            MusicSelectorType selection_type = (MusicSelectorType)(*it).get<int>( "type" );

            selections.push_back( MusicSceneSelector( selection_name, selection_link, selection_type, selection_id ) );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    // Reload the selection map
    if ( clearFirst )
        studio.getVenue()->clearMusicMappings();

    studio.getVenue()->addMusicMappings( selections );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_music_matcher_load( DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    return music_matcher_load( data, true );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_music_matcher_update( DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    return music_matcher_load( data, false );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::musicPlayerToJson( JsonBuilder& json )
{
    // If we have a music player, return player status
    if ( studio.hasMusicPlayer() ) {
        json.startObject( "music_player" );
        json.add( "player_name", studio.getMusicPlayer()->getPlayerName() );
        json.add( "username", studio.getMusicPlayer()->getUsername() );

        if ( studio.getMusicPlayer()->isLoggedIn() ) {
            DWORD length, remaining;
            UINT queued, played;
            CString track_link;
            bool success = studio.getMusicPlayer()->getPlayingTrack( track_link, &length, &remaining, &queued, &played );

            json.add( "logged_in", true );
            json.add( "queued", queued );
            json.add( "played", played );

            if ( success ) {
                json.startObject( "playing" );
                json.add( "link", track_link );
                json.add( "name", studio.getMusicPlayer()->getTrackFullName( track_link ) );
                json.add( "length", length );
                json.add( "remaining", remaining );
                json.add( "paused", studio.getMusicPlayer()->isTrackPaused() );

                AudioInfo audio_info;
                if ( studio.getMusicPlayer()->getTrackAudioInfo( track_link, &audio_info, 0L ) == OK ) {
                    json.add( "bpm", audio_info.tempo );
                    json.add( "key", audio_info.key );
                }

                json.endObject( "playing" );
            }
            else
                json.addNull( "playing" );
        }
        else {
            json.add( "logged_in", false );
        }

        CString last_error = studio.getMusicPlayer()->getLastPlayerError( );
        if ( !last_error.IsEmpty() )
            json.add( "player_error", last_error );

        json.endObject( "music_player" );
    }
    else
        json.addNull( "music_player" );
}
