/* 
Copyright (C) 2011-15 Robert DeSantis
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
#include "ColorStrobe.h"
#include "BPMRating.h"
#include "MusicPlayer.h"
#include "EventBus.h"
#include "VideoFinder.h"
#include "IniFile.h"

class Venue;
class MusicPlayer;

class DMXStudio : public EventBusListener, IPlayerEventCallback
{
    IniFile             m_config;

    FILE*               m_hLog;

    CCriticalSection    m_venue_mutex;
    Venue*              m_venue;
    MusicPlayer*        m_music_player;
    VideoFinder         m_video_finder;
    EventBus            m_event_bus;                            // Studio events

public:
    DMXStudio();
    ~DMXStudio();
        
    inline Venue* getVenue() const {
        return m_venue;
    }

    inline bool hasMusicPlayer() const {
        return m_music_player != NULL;
    }
    inline MusicPlayer* getMusicPlayer() const {
        return m_music_player;
    }

    inline VideoFinder* getVideoFinder() {
        return &m_video_finder;
    }

    inline LPCSTR getGoogleAPIKey() const {
        return m_config.m_google_api_key;
    }
    inline void setGoogleAPIKey( LPCSTR key ) {
        m_config.m_google_api_key = key;
    }

    inline bool isDMXRequired() const {
        return m_config.m_dmx_required;
    }
    inline void setDMXRequired( bool required ) {
        m_config.m_dmx_required = required;
    }

    inline LPCSTR getVenueFileName() const {
        return m_config.m_venue_filename;
    }
    inline void setVenueFileName( LPCSTR filename ) {
        m_config.m_venue_filename = filename;
    }

    inline LPCSTR getVenueContainer() const {
        return m_config.m_venue_container;
    }
    inline void setVenueContainer( LPCSTR container ) {
        m_config.m_venue_container = container;
    }

    inline bool isHttpEnabled() const {
        return m_config.m_http_enabled;
    }

    inline StrobeTime getWhiteoutStrobeSlow() const {
        return m_config.m_whiteout_strobe_slow;
    }
    inline void setWhiteoutStrobeSlow( StrobeTime strobe_slow ) {
        m_config.m_whiteout_strobe_slow = strobe_slow;
    }
        
    inline StrobeTime getWhiteoutStrobeFast() const {
        return m_config.m_whiteout_strobe_fast;
    }
    inline void setWhiteoutStrobeFast( StrobeTime strobe_fast ) {
        m_config.m_whiteout_strobe_fast = strobe_fast;
    }

    inline StrobeTime getWhiteoutStrobeManual() const {
        return m_config.m_whiteout_strobe_manual;
    }
    inline void setWhiteoutStrobeManual( StrobeTime strobe_manual ) {
        m_config.m_whiteout_strobe_manual = strobe_manual;
    }

    inline bool isDebug() const {
        return m_config.m_debug;
    }
    inline void setDebug( bool debug ) {
        m_config.m_debug = debug;
    }

    inline UINT getHttpPort() const {
        return m_config.m_http_port;
    }
    inline void setHttpPort( UINT http_port ) {
        m_config.m_http_port = http_port;
    }

    inline LPCSTR getHueBridgeIP() const {
        return m_config.m_hue_bridge_ip;
    }
    inline void setHueBridgeIP( LPCSTR hue_bridge_ip ) {
        m_config.m_hue_bridge_ip = hue_bridge_ip;
    }

    inline std::set<UINT> getHueIgnoredLights() const {
        return m_config.m_hue_ignored_lights;
    }
    inline void setHueIgnoredLights( std::set<UINT> hue_ignored_lights ) {
        m_config.m_hue_ignored_lights = hue_ignored_lights;
    }
    inline bool isHueLightIgnored( UINT light_number ) const {
        return m_config.m_hue_ignored_lights.find( light_number ) != m_config.m_hue_ignored_lights.end();
    }

    inline bool isHueAutoSetup() const {
        return m_config.m_hue_auto_setup;
    }
    void setHueAutoSetup( bool hue_auto_setup) {
        m_config.m_hue_auto_setup = hue_auto_setup;
    }

    inline std::set<CString>* getHueAllowedGroups() {
        return &m_config.m_hue_allowed_groups;
    }
    inline void setHueAllowedGroups( std::set<CString> hue_allowed_groups ) {
        m_config.m_hue_allowed_groups = hue_allowed_groups;
    }
    inline bool hasHueAllowedGroups() const {
        return m_config.m_hue_allowed_groups.size() > 0;
    }

    inline size_t getVideosPerTrack() const {
        return m_config.m_videos_per_track;
    }
    inline void setVideosPerTrack( size_t videos_per_track ) {
        m_config.m_videos_per_track = videos_per_track;
    }

    inline size_t getVideosPaletteSize() const {
        return m_config.m_video_palette_size;
    }
    inline void setVideosPaletteSize( size_t video_palette_size ) {
        m_config.m_videos_per_track = video_palette_size;
    }

    void runStudio();

    static CString getUserDocumentDirectory();
    static void log( std::exception& ex );
    static void log( StudioException& ex );
    static void log( const char *fmt, ... );
    static void log_status( const char *fmt, ... );
    static void log_warning( const char *fmt, ... );
    static bool fireEvent( EventSource source, DWORD uid, EventAction action, LPCSTR text=NULL, DWORD val1=0L, DWORD val2=0L, DWORD val3=0L, DWORD val4=0L );
    static EventBus* getEventBus();

    void writeVenueToString( CString& output_string );

    bool loadVenueFromFile( LPCSTR venue_filename );
    bool saveVenueToFile( LPCSTR venue_filename );
    bool loadVenueFromString( LPCSTR venue_xml );
    bool newVenue( );

    bool handleEvent( const Event& event );
    HRESULT STDMETHODCALLTYPE notify( PlayerEventData* pNotify );

private:
    Venue* readVenueFromFile( LPCSTR input_file );
    void writeVenueToFile( LPCSTR output_file );
    Venue* readVenueFromString( LPCSTR venue_xml );

    LPCSTR getDefaultVenueFilename( void );

    void openStudioLogFile( void);
    void closeStudioLogFile( void );
    void showIpAddress( void );

    void readIniFile();
    void writeIniFile();

    void createMusicPlayer( LPCSTR username, LPCSTR player_dll_path );
};

extern DMXStudio studio;

