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


#pragma once

#include "stdafx.h"
#include "StudioException.h"

typedef DWORD (__cdecl *GetPlayerApiVersion)( void );
typedef void (__cdecl *GetPlayerName)( LPSTR buffer, size_t buffer_length );
typedef bool (__cdecl *Connect)( void );
typedef bool (__cdecl *Disconnect)( void );
typedef bool (__cdecl *Signon)( LPCSTR username, LPCSTR password );
typedef bool (__cdecl *GetPlaylists)( UINT* num_lists, DWORD* playlist_ids, size_t playlist_ids_capacity );
typedef bool (__cdecl *GetPlaylistName)( DWORD playlist_id, LPSTR buffer, size_t buffer_length );
typedef bool (__cdecl *GetTracks)( DWORD playlist_id, UINT* num_tracks, DWORD* track_ids, size_t track_ids_capacity );
typedef bool (__cdecl *PlayTrack)( DWORD track_id, bool queue );
typedef bool (__cdecl *PlayAllTracks)( DWORD playlist_id, bool queue );
typedef bool (__cdecl *ForwardTrack)( void );
typedef bool (__cdecl *BackTrack)( void );
typedef bool (__cdecl *StopTrack)( void );
typedef bool (__cdecl *PauseTrack)( bool pause );
typedef bool (__cdecl *GetPlayingTrack)( DWORD* track_id, DWORD* track_length, DWORD* time_remaining, UINT* queued_tracks );
typedef bool (__cdecl *IsTrackPaused)( void );
typedef bool (__cdecl *IsLoggedIn)( void );
typedef bool (__cdecl *GetQueuedTracks)( UINT* num_tracks, DWORD* track_ids, size_t track_ids_capacity );
typedef bool (__cdecl *GetPlayedTracks)( UINT* num_tracks, DWORD* track_ids, size_t track_ids_capacity );
typedef bool (__cdecl *GetLastPlayerError)( LPSTR buffer, size_t buffer_length );
typedef bool (__cdecl *WaitOnTrackEvent)( DWORD wait_ms, DWORD* track_id, bool* paused );
typedef bool (__cdecl *GetTrackInfo)( DWORD track_id,  LPSTR track_name, size_t track_name_size, LPSTR artist_name, size_t artist_name_size, 
                                      LPSTR album_name, size_t album_name_size, DWORD* track_duration_ms, bool* starred );

typedef std::vector<DWORD> PlayerItems;

class MusicPlayer
{
    CString             m_dll_path;
    CString             m_username;

    HMODULE             m_library;
    bool                m_logged_in;

    // DLL function pointers
    GetPlayerName       m_GetPlayerName;
    Connect             m_Connect;
    Disconnect          m_Disconnect;
    Signon              m_Signon;
    GetPlaylists        m_GetPlaylists;
    GetPlaylistName     m_GetPlaylistName;
    GetTracks           m_GetTracks;
    PlayTrack           m_PlayTrack;
    PlayAllTracks       m_PlayAllTracks;
    ForwardTrack        m_ForwardTrack;
    BackTrack           m_BackTrack;
    StopTrack           m_StopTrack;
    PauseTrack          m_PauseTrack;
    GetPlayingTrack     m_GetPlayingTrack;
    IsTrackPaused       m_IsTrackPaused;
    GetTrackInfo        m_GetTrackInfo;
    IsLoggedIn          m_IsLoggedIn;
    GetQueuedTracks     m_GetQueuedTracks;
    GetPlayedTracks     m_GetPlayedTracks;
    GetLastPlayerError  m_GetLastPlayerError;
    WaitOnTrackEvent    m_WaitOnTrackEvent;

public:
    MusicPlayer( LPCSTR library_path, LPCSTR username );
    ~MusicPlayer(void);

    void initialize( );

    template <class T>
    T getAddress( LPCSTR function_name ) {
        T funcptr = (T) GetProcAddress( m_library, function_name );
        STUDIO_ASSERT( funcptr, "Music controller '%s' missing '%s()'", function_name, m_dll_path );
        return funcptr;
    }

    LPCSTR getUsername() const {
        return m_username;
    }

    bool isLoaded() const {
        return m_library != NULL;
    }

    CString getPlayerName( void );
    bool connect( void );
    bool disconnect( void );
    bool signon( LPCSTR username, LPCSTR password );
    bool getPlaylists( PlayerItems& playlists );
    bool getTracks( DWORD playlist_id, PlayerItems& tracks );
    bool getQueuedTracks( PlayerItems& queued_tracks );
    bool getPlayedTracks( PlayerItems& queued_tracks );
    CString getPlaylistName( DWORD playlist_id );
    CString getTrackFullName( DWORD track_id );
    bool playAllTracks( DWORD playlist_id, bool queue );
    bool playTrack( DWORD track_id, bool queue );
    bool forwardTrack( void );
    bool backTrack( void );
    bool stopTrack( void );
    bool pauseTrack( bool pause );
    bool isLoggedIn( void );
    bool isTrackPaused( void );
    bool getTrackInfo( DWORD track_id, DWORD* track_length, bool* track_starred=NULL );
    DWORD getPlayingTrack( DWORD* track_length=NULL, DWORD* time_remaining=NULL, UINT* queued_tracks=NULL );
    CString getLastPlayerError( void );
    bool waitOnTrackEvent( DWORD wait_ms, DWORD* track_id, bool* paused );
    bool getTrackInfo( DWORD track_id, CString* track_name=NULL, CString* artist_name=NULL, CString* album_name=NULL, DWORD* track_duration_ms=NULL, bool* starred=NULL );
};
