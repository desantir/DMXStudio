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

#define DMX_MAX_UNIVERSES          4
#define DMX_PACKET_SIZE            512
#define MULTI_UNIV_PACKET_SIZE     (DMX_PACKET_SIZE*DMX_MAX_UNIVERSES)

#define INVALID_CHANNEL             0xFFFF

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

typedef unsigned int channel_t;
typedef unsigned int universe_t;

typedef unsigned char BYTE;

typedef DWORD UID;
typedef std::set<UID> UIDSet;
typedef std::vector<UID> UIDArray;
typedef std::vector<LPCSTR> LPCSTRArray;

#define NOUID   0L

typedef std::vector<channel_t> ChannelList;

inline UIDSet UIDArrayToSet( UIDArray& uid_array ) {
    UIDSet uid_set;
    uid_set.insert( uid_array.begin(), uid_array.end() );
    return uid_set;
}

template <class T, class R>
R maxKeyValue( T& map ) {
    R maxValue = 0;

    for ( T::iterator it=map.begin(); it != map.end(); ++it ) {
        if ( it->first > maxValue )
            maxValue = it->first;
    }

    return maxValue;
}

class Venue;
class EventBus;
class MusicPlayer;

class DMXStudio : public EventBusListener, IPlayerEventCallback
{
    FILE*               m_hLog;
    bool                m_debug;
    CCriticalSection    m_venue_mutex;
    Venue*              m_venue;
    CString             m_venue_container;                      // Location on server where all venue files are kept
    CString             m_venue_filename;                       // Venue filename relative to venue_container
    UINT                m_http_port;
    StrobeTime          m_whiteout_strobe_slow;
    StrobeTime          m_whiteout_strobe_fast;
    bool                m_enable_http;
    MusicPlayer*        m_music_player;
    bool                m_dmx_required;
    EventBus            m_event_bus;                            // Studio events

public:
    DMXStudio();
    ~DMXStudio();

    void readIniFile();
    void writeIniFile();
        
    inline Venue* getVenue() const {
        return m_venue;
    }

    void createMusicPlayer( LPCSTR username, LPCSTR player_dll_path );

    inline bool hasMusicPlayer() const {
        return m_music_player != NULL;
    }
    inline MusicPlayer* getMusicPlayer() const {
        return m_music_player;
    }

    bool isDMXRequired() const {
        return m_dmx_required;
    }
    void setDMXRequired( bool required ) {
        m_dmx_required = required;
    }

    LPCSTR getVenueFileName() const {
        return m_venue_filename;
    }
    void setVenueFileName( LPCSTR filename ) {
        m_venue_filename = filename;
    }

    LPCSTR getVenueContainer() const {
        return m_venue_container;
    }
    void setVenueContainer( LPCSTR container ) {
        m_venue_container = container;
    }

    bool getEnableHttp() const {
        return m_enable_http;
    }

    StrobeTime getWhiteoutStrobeSlow() const {
        return m_whiteout_strobe_slow;
    }
    void setWhiteoutStrobeSlow( StrobeTime strobe_slow ) {
        m_whiteout_strobe_slow = strobe_slow;
    }
        
    StrobeTime getWhiteoutStrobeFast() const {
        return m_whiteout_strobe_fast;
    }
    void setWhiteoutStrobeFast( StrobeTime strobe_fast ) {
        m_whiteout_strobe_fast = strobe_fast;
    }

    bool isDebug() const {
        return m_debug;
    }
    void setDebug( bool debug ) {
        m_debug = debug;
    }

    UINT getHttpPort() const {
        return m_http_port;
    }
    void setHttpPort( UINT http_port ) {
        m_http_port = http_port;
    }

    void runStudio();

    static void log( std::exception& ex );
    static void log( StudioException& ex );
    static void log( const char *fmt, ... );
    static void log_status( const char *fmt, ... );
    static bool fireEvent( EventSource source, DWORD uid, EventAction action, LPCSTR text=NULL, DWORD val1=0L, DWORD val2=0L, DWORD val3=0L, DWORD val4=0L );
    static EventBus* getEventBus();

    void writeVenueToString( CString& output_string );

    bool loadVenueFromFile( LPCSTR venue_filename );
    bool saveVenueToFile( LPCSTR venue_filename );
    bool loadVenueFromString( LPCSTR venue_xml );
    bool newVenue( );

    bool handleEvent( const Event& event );
    HRESULT STDMETHODCALLTYPE notify( TrackEventData* pNotify );

private:
    Venue* readVenueFromFile( LPCSTR input_file );
    void writeVenueToFile( LPCSTR output_file );
    Venue* readVenueFromString( LPCSTR venue_xml );

    LPCSTR getDefaultVenueFilename( void );

    void openStudioLogFile( void);
    void closeStudioLogFile( void );
    void showIpAddress( void );
};

extern DMXStudio studio;