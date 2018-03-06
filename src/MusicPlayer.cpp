/* 
Copyright (C) 2011-17 Robert DeSantis
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

#include "stdafx.h"
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
    STUDIO_ASSERT( (*pGetPlayerApiVersion)() == 2, "Music controller version is not compatible" );

    m_GetPlayerInfo = getAddress<GetPlayerInfo>( "GetPlayerInfo" );
    m_Connect = getAddress<Connect>( "Connect" );
    m_Disconnect = getAddress<Disconnect>( "Disconnect" );
    m_RegisterEventListener = getAddress<RegisterEventListener>( "RegisterEventListener" );
    m_UnregisterEventListener = getAddress<UnregisterEventListener>( "UnregisterEventListener" );
    m_Signon = getAddress<Signon>( "Signon" );
	m_AcceptAuthorization = getAddress<AcceptAuthorization>( "AcceptAuthorization" );
	m_IsLoggedIn = getAddress<IsLoggedIn>( "IsLoggedIn" );

    m_GetPlaylists = getAddress<GetPlaylists>( "GetPlaylists" );
    m_GetPlaylistInfo = getAddress<GetPlaylistInfo>( "GetPlaylistInfo" );
    m_GetTracks = getAddress<GetTracks>( "GetTracks" );
	m_SearchTracks = getAddress<SearchTracks>( "SearchTracks" );
    m_QueueTrack = getAddress<QueueTrack>( "QueueTrack" );
    m_PlayTrack = getAddress<PlayTrack>( "PlayTrack" );
    m_PlayAllTracks = getAddress<PlayAllTracks>( "PlayAllTracks" );
    m_ForwardTrack = getAddress<ForwardTrack>( "ForwardTrack" );
    m_BackTrack = getAddress<BackTrack>( "BackTrack" );
    m_StopTrack = getAddress<StopTrack>( "StopTrack" );
    m_PauseTrack = getAddress<PauseTrack>( "PauseTrack" );
    m_GetPlayingTrack = getAddress<GetPlayingTrack>( "GetPlayingTrack" );
    m_IsTrackPaused = getAddress<IsTrackPaused>( "IsTrackPaused" );
    m_GetTrackInfo = getAddress<GetTrackInfo>( "GetTrackInfo" );
    m_GetTrackAudioInfo = getAddress<GetTrackAudioInfo>( "GetTrackAudioInfo" );
    m_GetQueuedTracks = getAddress<GetQueuedTracks>( "GetQueuedTracks" );
    m_GetPlayedTracks = getAddress<GetPlayedTracks>( "GetPlayedTracks" );
    m_GetLastPlayerError = getAddress<GetLastPlayerError>( "GetLastPlayerError" );
    m_WaitOnTrackEvent = getAddress<WaitOnTrackEvent>( "WaitOnTrackEvent" );
    m_GetTrackAnalysis = getAddress<GetTrackAnalysis>( "GetTrackAnalysis" );

	getPlayerInfo( &m_player_info );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlayerInfo( PlayerInfo* player_info ) 
{
    VERIFY_LIBRARY_LOADED;

    return (*m_GetPlayerInfo)( player_info );
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
bool MusicPlayer::registerEventListener( IPlayerEventCallback* listener )
{
    VERIFY_LIBRARY_LOADED;

    return (*m_RegisterEventListener)( listener );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::unregisterEventListener( IPlayerEventCallback* listener )
{
    VERIFY_LIBRARY_LOADED;

    return (*m_UnregisterEventListener)( listener );
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
bool MusicPlayer::isLoggedIn( )
{
	VERIFY_LIBRARY_LOADED;

	m_logged_in = (*m_IsLoggedIn)( );

	return m_logged_in;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::acceptAuthorization(LPBYTE authorization_response, DWORD authorization_len )
{
	VERIFY_LIBRARY_LOADED;

	return (*m_AcceptAuthorization)( authorization_response, authorization_len );
}

// ----------------------------------------------------------------------------
//
static void copyBufferToItemList( LPSTR playlists, PlayerItems& list ) {
    list.clear();

    for ( LPSTR ptr=playlists; *ptr != '\0'; ptr=&ptr[strlen(ptr)+1] )
        list.push_back( ptr );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlaylists( PlayerItems& playlists )
{
    VERIFY_LIBRARY_LOADED;

    char playlist_links[LINK_BUFFER_LENGTH];
    UINT num_lists = 0;

    bool results = (*m_GetPlaylists)( &num_lists, playlist_links, LINK_BUFFER_LENGTH );
    copyBufferToItemList( playlist_links, playlists );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getTracks( LPCSTR playlist_link, PlayerItems& tracks )
{
    VERIFY_LIBRARY_LOADED;

    char track_links[LINK_BUFFER_LENGTH];
    UINT num_tracks = 0;

    bool results = (*m_GetTracks)( playlist_link, &num_tracks, track_links, LINK_BUFFER_LENGTH );
    copyBufferToItemList( track_links, tracks );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::searchTracks( LPCSTR search_query, PlayerItems& tracks )
{
	VERIFY_LIBRARY_LOADED;

	char track_links[LINK_BUFFER_LENGTH];
	UINT num_tracks = 0;

	bool results = (*m_SearchTracks)( search_query, &num_tracks, track_links, LINK_BUFFER_LENGTH );
	copyBufferToItemList( track_links, tracks );
	return results;
}


// ----------------------------------------------------------------------------
//
bool MusicPlayer::getQueuedTracks( PlayerItems& queued_tracks )
{
    VERIFY_LIBRARY_LOADED;

    char track_links[LINK_BUFFER_LENGTH];
    UINT num_tracks = 0;

    bool results = (*m_GetQueuedTracks)( &num_tracks, track_links, LINK_BUFFER_LENGTH );
    copyBufferToItemList( track_links, queued_tracks );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlayedTracks( PlayerItems& played_tracks )
{
    VERIFY_LIBRARY_LOADED;

    char track_links[LINK_BUFFER_LENGTH];
    UINT num_tracks = 0;

    bool results = (*m_GetPlayedTracks)( &num_tracks, track_links, LINK_BUFFER_LENGTH );
    copyBufferToItemList( track_links, played_tracks );
    return results;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlaylistInfo( LPCSTR playlist_link, PlaylistInfo* playlist_info )
{
    VERIFY_LIBRARY_LOADED;

    return (*m_GetPlaylistInfo)( playlist_link, playlist_info );
}

// ----------------------------------------------------------------------------
//
CString MusicPlayer::getTrackFullName( LPCSTR track_link )
{
    TrackInfo track_info;
    CString buffer;

    if ( getTrackInfo( track_link, &track_info ) )
        buffer.Format( "%s by %s", track_info.track_name, track_info.artist_name );

    return buffer;
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getTrackInfo( LPCSTR track_link, TrackInfo* track_info )
{
    VERIFY_LIBRARY_LOADED;

    return (*m_GetTrackInfo)( track_link, track_info );
}

// ----------------------------------------------------------------------------
//
AudioStatus MusicPlayer::getTrackAudioInfo( LPCSTR track_link, AudioInfo* audio_info, DWORD wait_ms ) 
{
    VERIFY_LIBRARY_LOADED;

    return (*m_GetTrackAudioInfo)( track_link, audio_info, wait_ms );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::playAllTracks( LPCSTR playlist_link, bool queue )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_PlayAllTracks)( playlist_link, queue );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::queueTrack( LPCSTR track_link )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_QueueTrack)( track_link );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::playTrack( LPCSTR track_link, DWORD seek_ms )
{
    VERIFY_LIBRARY_LOADED;
    VERIFY_PLAYER_LOGGED_IN;

    return (*m_PlayTrack)( track_link, seek_ms );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getTrackAnalysis( LPCSTR track_link, AnalyzeInfo** info )
{
    VERIFY_LIBRARY_LOADED;

    return (*m_GetTrackAnalysis)( track_link, info );
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
bool MusicPlayer::isTrackPaused( )
{
    VERIFY_LIBRARY_LOADED;

    return (*m_IsTrackPaused)( );
}

// ----------------------------------------------------------------------------
//
bool MusicPlayer::getPlayingTrack( PlayingInfo* playing_info )
{
    VERIFY_LIBRARY_LOADED;

    bool result = (*m_GetPlayingTrack)( playing_info );

    return result;
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
bool MusicPlayer::waitOnTrackEvent( DWORD wait_ms, CString& track_link, bool* paused )
{
    bool result = (*m_WaitOnTrackEvent)( wait_ms, track_link.GetBuffer(MAX_LINK_SIZE), paused );
    track_link.ReleaseBuffer();
    return result;
}
