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

struct CachedTrack {
    ULONG    data_size;
    UINT     channels;
    UINT     bit_rate;
    UINT     length_ms;
    UINT     frames;
    bool     ready;
    BYTE     data[1];

    inline UINT getFrameSize() const {
        return channels * sizeof(int16_t);
    }
};

struct AudioInfo {
    char        id[512];
    char        song_type[512];                 // Comman separated list of 'christmas', 'live', 'studio', 'acoustic' and 'electric'
    int         key;                            // Key of song C,C#,D,D#,E,F,F#,G,B#,B,B#,B (0-11)
    int         mode;                           // 0=minor, 1=major
    int         time_signature;                 // beats per measure
    double      energy;                         // 0.0 < energy < 1.0
    double      liveness;                       // 0.0 < liveness < 1.0
    double      tempo;                          // 0.0 < tempo < 500.0 (BPM)
    double      speechiness;                    // 0.0 < speechiness < 1.0
    double      acousticness;                   // 0.0 < acousticness < 1.0
    double      instrumentalness;
    double      duration;                       // Duration of the track
    double      loudness;                       // -100.0 < loudness < 100.0 (dB)
    double      valence;                        // Emotion 0=negative to 1=positive
    double      danceability;                   // 0.0 < danceability < 1.0
};

typedef DWORD (__cdecl *GetPlayerApiVersion)( void );
typedef void (__cdecl *GetPlayerName)( LPSTR buffer, size_t buffer_length );
typedef bool (__cdecl *Connect)( void );
typedef bool (__cdecl *Disconnect)( void );
typedef bool (__cdecl *Signon)( LPCSTR username, LPCSTR password );
typedef bool (__cdecl *GetPlaylists)( UINT* num_lists, DWORD* playlist_ids, size_t playlist_ids_capacity );
typedef bool (__cdecl *GetPlaylistName)( DWORD playlist_id, LPSTR buffer, size_t buffer_length );
typedef bool (__cdecl *GetTracks)( DWORD playlist_id, UINT* num_tracks, DWORD* track_ids, size_t track_ids_capacity );
typedef bool (__cdecl *PlayTrack)( DWORD track_id, bool queue );
typedef bool (__cdecl *CacheTrack)( DWORD track_id );
typedef bool (__cdecl *GetCachedTrack)( CachedTrack** cached_track );
typedef bool (__cdecl *PlayAllTracks)( DWORD playlist_id, bool queue );
typedef bool (__cdecl *ForwardTrack)( void );
typedef bool (__cdecl *BackTrack)( void );
typedef bool (__cdecl *StopTrack)( void );
typedef bool (__cdecl *PauseTrack)( bool pause );
typedef bool (__cdecl *GetPlayingTrack)( DWORD* track_id, DWORD* track_length, DWORD* time_remaining, UINT* queued_tracks, UINT* previous_tracks );
typedef bool (__cdecl *IsTrackPaused)( void );
typedef bool (__cdecl *IsLoggedIn)( void );
typedef bool (__cdecl *GetQueuedTracks)( UINT* num_tracks, DWORD* track_ids, size_t track_ids_capacity );
typedef bool (__cdecl *GetPlayedTracks)( UINT* num_tracks, DWORD* track_ids, size_t track_ids_capacity );
typedef bool (__cdecl *GetLastPlayerError)( LPSTR buffer, size_t buffer_length );
typedef bool (__cdecl *WaitOnTrackEvent)( DWORD wait_ms, DWORD* track_id, bool* paused );
typedef bool (__cdecl *GetTrackInfo)( DWORD track_id,  LPSTR track_name, size_t track_name_size, LPSTR artist_name, size_t artist_name_size, 
                                      LPSTR album_name, size_t album_name_size, DWORD* track_duration_ms, bool* starred, LPSTR track_link, size_t track_link_size );
typedef bool (__cdecl *GetTrackAudioInfo)( DWORD track_id, AudioInfo* audio_info );
 
typedef std::vector<DWORD> PlayerItems;

class MusicPlayer
{
    CString             m_dll_path;
    CString             m_username;

    HMODULE             m_library;
    bool                m_logged_in;

    // DLL function pointers
    GetPlayerName           m_GetPlayerName;
    Connect                 m_Connect;
    Disconnect              m_Disconnect;
    Signon                  m_Signon;
    GetPlaylists            m_GetPlaylists;
    GetPlaylistName         m_GetPlaylistName;
    GetTracks               m_GetTracks;
    PlayTrack               m_PlayTrack;
    PlayAllTracks           m_PlayAllTracks;
    CacheTrack              m_CacheTrack;
    GetCachedTrack          m_GetCachedTrack;
    ForwardTrack            m_ForwardTrack;
    BackTrack               m_BackTrack;
    StopTrack               m_StopTrack;
    PauseTrack              m_PauseTrack;
    GetPlayingTrack         m_GetPlayingTrack;
    IsTrackPaused           m_IsTrackPaused;
    GetTrackInfo            m_GetTrackInfo;
    GetTrackAudioInfo       m_GetTrackAudioInfo;
    IsLoggedIn              m_IsLoggedIn;
    GetQueuedTracks         m_GetQueuedTracks;
    GetPlayedTracks         m_GetPlayedTracks;
    GetLastPlayerError      m_GetLastPlayerError;
    WaitOnTrackEvent        m_WaitOnTrackEvent;

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
    bool cacheTrack( DWORD track_id );
    bool getCachedTrack( CachedTrack** cached_track );
    bool forwardTrack( void );
    bool backTrack( void );
    bool stopTrack( void );
    bool pauseTrack( bool pause );
    bool isLoggedIn( void );
    bool isTrackPaused( void );
    bool getTrackAudioInfo( DWORD track_id, AudioInfo* audio_info );
    DWORD getPlayingTrack( DWORD* track_length=NULL, DWORD* time_remaining=NULL, UINT* queued_tracks=NULL, UINT* previous_tracks=NULL );
    CString getLastPlayerError( void );
    bool waitOnTrackEvent( DWORD wait_ms, DWORD* track_id, bool* paused );
    bool getTrackInfo( DWORD track_id, CString* track_name=NULL, CString* artist_name=NULL, CString* album_name=NULL, DWORD* track_duration_ms=NULL, 
                       bool* starred=NULL, CString* track_link=NULL );
};
