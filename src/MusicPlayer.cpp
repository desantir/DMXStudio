/* 
Copyright (C) 2011,2012 Robert DeSantis
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
#include "MusicPlayer.h"

#define VERIFY_LIBRARY_LOADED \
    STUDIO_ASSERT( m_library, "Music player library not loaded" )

#define VERIFY_PLAYER_LOGGED_IN \
    STUDIO_ASSERT( m_logged_in, "User is not logged into the music controller" )

// ----------------------------------------------------------------------------
//
MusicPlayer::MusicPlayer( LPCSTR library_path, LPCSTR username ) :
        m_dll_path( library_path ),
        m_username( username ),
        m_library( NULL ),
        m_logged_in( false )
{
}

// ----------------------------------------------------------------------------
//
MusicPlayer::~MusicPlayer(void)
{
    if ( m_library )
        disconnect();
}

// ----------------------------------------------------------------------------
//
void MusicPlayer::initialize( ) 
{
    m_library = LoadLibrary( m_dll_path );
    STUDIO_ASSERT( m_library, "Unable to load music controller DLL '%s'", m_dll_path );

    GetPlayerApiVersion pGetPlayerApiVersion = getAddress<GetPlayerApiVersion>( "GetPlayerApiVersion" );
    STUDIO_ASSERT( (*pGetPlayerApiVersion)() == 1, "Music controller version is not compatible" );

    m_GetPlayerName = getAddress<GetPlayerName>( "GetPlayerName" );
    m_Connect = getAddress<Connect>( "Connect" );
    m_Disconnect = getAddress<Disconnect>( "Disconnect" );
    m_Signon = getAddress<Signon>( "Signon" );
    m_GetPlaylists = getAddress<GetPlaylists>( "GetPlaylists" );
    m_GetPlaylistName = getAddress<GetPlaylistName>( "GetPlaylistName" );
    m_GetTracks = getAddress<GetTracks>( "GetTracks" );
    m_PlayTrack = getAddress<PlayTrack>( "PlayTrack" );
    m_PlayAllTracks = getAddress<PlayAllTracks>( "PlayAllTracks" );
    m_ForwardTrack = getAddress<ForwardTrack>( "ForwardTrack" );
    m_BackTrack = getAddress<BackTrack>( "BackTrack" );
    m_StopTrack = getAddress<StopTrack>( "StopTrack" );
    m_PauseTrack = getAddress<PauseTrack>( "PauseTrack" );
    m_GetPlayingTrack = getAddress<GetPlayingTrack>( "GetPlayingTrack" );
    m_IsTrackPaused = getAddress<IsTrackPaused>( "IsTrackPaused" );
    m_IsLoggedIn = getAddress<IsLoggedIn>( "IsLoggedIn" );
    m_GetTrackInfo = getAddress<GetTrackInfo>( "GetTrackInfo" );
    m_GetQueuedTracks = getAddress<GetQueuedTracks>( "GetQueuedTracks" );
    m_GetPlayedTracks = getAddress<GetPlayedTracks>( "GetPlayedTracks" );
    m_GetLastPlayerError = getAddress<GetLastPlayerError>( "GetLastPlayerError" );
    m_WaitOnTrackEvent = getAddress<WaitOnTrackEvent>( "WaitOnTrackEvent" );

    DMXStudio::log_status( "Loaded music controller '%s'", getPlayerName() );
}

// ----------------------------------------------------------------------------
//
CString MusicPlayer::getPlayerName( ) 
{
    VERIFY_LIBRARY_LOADED;

    CString buffer;
    buffer.Empty();
    (*m_GetPlayerName)( buffer.GetBufferSetLength( 512 ), 512 );
    buffer.ReleaseBuffer();
    return buffer;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::connect( )
{
    VERIFY_LIBRARY_LOADED;
    
    m_logged_in = (*m_Connect)( );

    return m_logged_in;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::disconnect( )
{
    VERIFY_LIBRARY_LOADED;

    bool result = (*m_Disconnect)( );

    FreeLibrary( m_library );
    m_library = NULL;

    return result;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::signon( LPCSTR username, LPCSTR password )
{
    VERIFY_LIBRARY_LOADED;

    m_logged_in = (*m_Signon)( username, password );

    if ( m_logged_in )
        m_username = username;

    return m_logged_in;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlaylists( PlayerItems& playlists )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    DWORD playlist_ids[500];
    UINT num_lists = 0;

    playlists.clear();

    bool results = (*m_GetPlaylists)( &num_lists, playlist_ids, sizeof(playlist_ids) );
    std::copy( playlist_ids, playlist_ids+num_lists, std::inserter( playlists, playlists.begin() ) );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getTracks( DWORD playlist_id, PlayerItems& tracks )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    DWORD track_ids[2000];
    UINT num_tracks = 0;

    tracks.clear();

    bool results = (*m_GetTracks)( playlist_id, &num_tracks, track_ids, sizeof(track_ids) );
    std::copy( track_ids, track_ids+num_tracks, std::inserter( tracks, tracks.begin() ) );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getQueuedTracks( PlayerItems& queued_tracks )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    DWORD track_ids[5000];
    UINT num_tracks = 0;

    queued_tracks.clear();

    bool results = (*m_GetQueuedTracks)( &num_tracks, track_ids, sizeof(track_ids) );
    std::copy( track_ids, track_ids+num_tracks, std::inserter( queued_tracks, queued_tracks.begin() ) );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlayedTracks( PlayerItems& played_tracks )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    DWORD track_ids[5000];
    UINT num_tracks = 0;

    played_tracks.clear();

    bool results = (*m_GetPlayedTracks)( &num_tracks, track_ids, sizeof(track_ids) );
    std::reverse_copy( track_ids, track_ids+num_tracks, std::inserter( played_tracks, played_tracks.begin() ) );
    return results;
}

// ----------------------------------------------------------------------------
//
CString MusicPlayer::getPlaylistName( DWORD playlist_id )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    CString buffer;
    buffer.Empty();
    (*m_GetPlaylistName)( playlist_id, buffer.GetBufferSetLength( 512 ), 512 );
    buffer.ReleaseBuffer();
    return buffer;
}

// ----------------------------------------------------------------------------
//
CString MusicPlayer::getTrackFullName( DWORD track_id )
{
    CString title, artist;
    CString buffer;

    if ( getTrackInfo( track_id, &title, &artist ) )
        buffer.Format( "%s by %s", title, artist );

    return buffer;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getTrackInfo( DWORD track_id, CString* track_name, CString* artist_name, CString* album_name, DWORD* track_duration_ms, bool* starred )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    LPSTR track_name_ptr = track_name ? track_name->GetBufferSetLength(256) : NULL;
    LPSTR artist_name_ptr = artist_name ? artist_name->GetBufferSetLength(256) : NULL;
    LPSTR album_name_ptr = album_name ? album_name->GetBufferSetLength(256) : NULL;

    bool result = (*m_GetTrackInfo)( track_id, track_name_ptr, 256, artist_name_ptr, 256, album_name_ptr, 256, track_duration_ms, starred );

    if ( track_name_ptr )
        track_name->ReleaseBuffer();
    if ( artist_name_ptr )
        artist_name->ReleaseBuffer();
    if ( album_name_ptr )
        album_name->ReleaseBuffer();

    return result;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::playAllTracks( DWORD playlist_id, bool queue )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_PlayAllTracks)( playlist_id, queue );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::playTrack( DWORD track_id, bool queue )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_PlayTrack)( track_id, queue );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::forwardTrack( )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_ForwardTrack)( );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::backTrack( )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_BackTrack)( );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::stopTrack( )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_StopTrack)( );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::pauseTrack( bool pause )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_PauseTrack)( pause );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::isLoggedIn( )
{
    VERIFY_LIBRARY_LOADED;

    m_logged_in = (*m_IsLoggedIn)( );

    return m_logged_in;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::isTrackPaused( )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_IsTrackPaused)( );
}

// ----------------------------------------------------------------------------
//
DWORD MusicPlayer::getPlayingTrack( DWORD* track_length, DWORD* time_remaining, UINT* queued_tracks, UINT *played_tracks )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    DWORD track_id = 0;

    (*m_GetPlayingTrack)( &track_id, track_length, time_remaining, queued_tracks, played_tracks );

    return track_id;
}

// ----------------------------------------------------------------------------
//
CString MusicPlayer::getLastPlayerError( ) 
{
    VERIFY_LIBRARY_LOADED;

    CString buffer;
    buffer.Empty();
    (*m_GetLastPlayerError)( buffer.GetBufferSetLength( 512 ), 512 );
    buffer.ReleaseBuffer();
    return buffer;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::waitOnTrackEvent( DWORD wait_ms, DWORD* track_id, bool* paused )
{
    return (*m_WaitOnTrackEvent)( wait_ms, track_id, paused );
}
