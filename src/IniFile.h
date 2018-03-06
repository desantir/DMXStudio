/* 
Copyright (C) 2011-2017 Robert DeSantis
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

class IniFile
{
public:
    CString         m_venue_filename;
    CString         m_venue_container;
    bool            m_http_enabled;
    UINT            m_http_port;
    StrobeTime      m_whiteout_strobe_slow;
    StrobeTime      m_whiteout_strobe_fast;
    StrobeTime      m_whiteout_strobe_manual;
    bool            m_music_player_enabled;
    CString         m_music_player_ddl;
    CString         m_music_player_username;
    bool            m_dmx_required;
    bool            m_debug;
    CString         m_google_api_key;
    size_t          m_videos_per_track;
    size_t          m_video_palette_size;

    // Hue specific initializers
    bool                m_hue_auto_setup;
    CString             m_hue_bridge_ip;
    std::set<UINT>      m_hue_ignored_lights;
    std::set<CString>   m_hue_allowed_groups;

    IniFile();
    ~IniFile(void);

    bool read( LPCSTR filename );
    void write( LPCSTR filename );

    inline void setVenueFilename( LPCSTR filename ) {
        m_venue_filename = filename;
    }
    inline LPCSTR getVenueFilename( void ) const {
        return m_venue_filename;
    }

    inline LPCSTR getVenueContainer() const {
        return m_venue_container;
    }
    inline void setVenueContainer( LPCSTR container ) {
        m_venue_container = container;
    }

    inline bool isDMXRequired() const {
        return m_dmx_required;
    }
    inline void setDMXRequired( bool required ) {
        m_dmx_required = required;
    }

    inline void setHttpEnabled( bool mobile ) {
        m_http_enabled = mobile;
    }
    inline bool isHttpEnabled( void ) const {
        return m_http_enabled;
    }

    inline void setHttpPort( UINT port ) {
        m_http_port = port;
    }
    inline UINT getHttpPort( void ) const {
        return m_http_port;
    }

    inline StrobeTime getWhiteoutStrobeSlow() const {
        return m_whiteout_strobe_slow;
    }
    inline void setWhiteoutStrobeSlow( StrobeTime strobe_slow ) {
        m_whiteout_strobe_slow = strobe_slow;
    }
        
    inline StrobeTime getWhiteoutStrobeFast() const {
        return m_whiteout_strobe_fast;
    }
    inline void setWhiteoutStrobeFast( StrobeTime strobe_fast ) {
        m_whiteout_strobe_fast = strobe_fast;
    }

    inline StrobeTime getWhiteoutStrobeManual() const {
        return m_whiteout_strobe_manual;
    }
    inline void setWhiteoutStrobeManual( StrobeTime strobe_manual ) {
        m_whiteout_strobe_manual = strobe_manual;
    }

    inline void setMusicPlayerEnabled( bool player_enabled ) {
        m_music_player_enabled = player_enabled;
    }
    inline bool isMusicPlayerEnabled( void ) const {
        return m_music_player_enabled;
    }

    inline void setMusicPlayer( LPCSTR player ) {
        m_music_player_ddl = player;
    }
    inline LPCSTR getMusicPlayer( void ) const {
        return m_music_player_ddl;
    }

    inline void setMusicUsername( LPCSTR username ) {
        m_music_player_username = username;
    }
    inline  LPCSTR getMusicUsername( void ) const {
        return m_music_player_username;
    }

    inline bool isDebug() const {
        return m_debug;
    }
    inline void setDebug( bool debug ) {
        m_debug = debug;
    }

    inline LPCSTR getHueBridgeIP() const {
        return m_hue_bridge_ip;
    }
    inline void setHueBridgeIP( LPCSTR hue_bridge_ip ) {
        m_hue_bridge_ip = hue_bridge_ip;
    }

    inline std::set<UINT> getHueIgnoredLights() const {
        return m_hue_ignored_lights;
    }
    inline void setHueIgnoredLights( std::set<UINT> hue_ignored_lights ) {
        m_hue_ignored_lights = hue_ignored_lights;
    }

    inline std::set<CString> getHueAllowedGroups() const {
        return m_hue_allowed_groups;
    }
    inline void setHueAllowedGroups( std::set<CString> hue_allowed_groups ) {
        m_hue_allowed_groups = hue_allowed_groups;
    }

    inline bool isHueAutoSetup() const {
        return m_hue_auto_setup;
    }
    void setHueAutoSetup( bool hue_auto_setup) {
        m_hue_auto_setup = hue_auto_setup;
    }

    inline LPCSTR getGoogleAPIKey() const {
        return m_google_api_key;
    }
    inline void setGoogleAPIKey( LPCSTR key ) {
        m_google_api_key = key;
    }

    inline size_t getVideosPerTrack() const {
        return m_videos_per_track;
    }
    inline void setVideosPerTrack( size_t videos_per_track ) {
        m_videos_per_track = videos_per_track;
    }

    inline size_t getVideosPaletteSize() const {
        return m_video_palette_size;
    }
    inline void setVideosPaletteSize( size_t video_palette_size ) {
        m_videos_per_track = video_palette_size;
    }
};

