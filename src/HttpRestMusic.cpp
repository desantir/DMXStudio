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
bool HttpRestServices::player_login( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
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

    UINT        track_number;
    INT         playlist_number;
    unsigned    queue;

    if ( sscanf_s( data, "%d/%u/%u", &playlist_number, &track_number, &queue ) != 3 )
        return false;

    PlayerItems tracks;

    if ( playlist_number == PLAYED_TRACKS_PLAYLIST )
        studio.getMusicPlayer()->getPlayedTracks(tracks);
    else if ( playlist_number == QUEUED_TRACKS_PLAYLIST )
        studio.getMusicPlayer()->getQueuedTracks(tracks);
    else {
        PlayerItems playlists;
        studio.getMusicPlayer()->getPlaylists( playlists );

        if ( playlist_number <= 0 || playlist_number > (int)playlists.size() )
            return false;

        DWORD playlist_id = playlists[ playlist_number-1 ];
        studio.getMusicPlayer()->getTracks( playlist_id, tracks );
    }

    if ( track_number == 0 || track_number > tracks.size() )
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

    INT         playlist_number;
    unsigned    queue;

    if ( sscanf_s( data, "%u/%u", &playlist_number, &queue ) != 2 )
        return false;

    if ( playlist_number == PLAYED_TRACKS_PLAYLIST || 
         playlist_number == QUEUED_TRACKS_PLAYLIST ) {
        PlayerItems tracks;

        if ( playlist_number == PLAYED_TRACKS_PLAYLIST )
            studio.getMusicPlayer()->getPlayedTracks(tracks);
        else if ( playlist_number == QUEUED_TRACKS_PLAYLIST )
            studio.getMusicPlayer()->getQueuedTracks(tracks);

        // TODO this is not exactly right - should clear out existing queued tracks
        for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++ ) {
            studio.getMusicPlayer()->playTrack( (*it), queue ? true : false );
            queue = 1;
        }
    }
    else {
        PlayerItems playlists;
        studio.getMusicPlayer()->getPlaylists( playlists );
        if ( playlist_number <= 0 || playlist_number > (int)playlists.size() )
            return false;

        DWORD playlist_id = playlists[ playlist_number-1 ];
        studio.getMusicPlayer()->playAllTracks( playlist_id, queue ? true : false );
    }

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
bool json_track_list( PlayerItems& tracks, CString& response )
{
    UINT track_number = 1;

    JsonBuilder json( response );
    json.startObject();

    json.startArray( "tracks" );
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ ) {
        CString track_name, artist_name, album_name, full_name, track_link;
        DWORD track_duration_ms;
        bool starred; 

        if ( !studio.getMusicPlayer()->getTrackInfo( (*it), &track_name, &artist_name, &album_name, &track_duration_ms, &starred, &track_link ) )
            continue;

        json.startObject();
        json.add( "number", track_number );
        json.add ("full_name", studio.getMusicPlayer()->getTrackFullName( (*it) ) );    // Legacy for music match
        json.add( "id", (*it) );
        json.add( "track_name", track_name );
        json.add( "artist_name", artist_name );
        json.add( "album_name", album_name );
        json.add( "duration", track_duration_ms ); 
        json.add( "starred", starred );
        json.add( "link", track_link );
        json.endObject();
    }
    json.endArray( "tracks" );
        
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

    return json_track_list( tracks, response );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_music_played( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    PlayerItems tracks;
    studio.getMusicPlayer()->getPlayedTracks(tracks);

    return json_track_list( tracks, response );
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

    return json_track_list( tracks, response );
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
bool HttpRestServices::query_music_matcher( CString& response, LPCSTR data )
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
bool HttpRestServices::edit_music_matcher( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
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
    studio.getVenue()->clearMusicMappings();
    studio.getVenue()->addMusicMappings( selections );

    return true;
}

