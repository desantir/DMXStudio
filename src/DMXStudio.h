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

#define DMX_PACKET_SIZE     512
#define INVALID_CHANNEL     0xFFFF

enum ColorChannels {
    RED_INDEX=0,
    GREEN_INDEX=1,
    BLUE_INDEX=2,
    WHITE_INDEX=3,
    AMBER_INDEX=4,
    COLOR_CHANNELS
};

struct StrobeTime {
    UINT m_on_ms;
    UINT m_off_ms;

    StrobeTime( UINT on_ms=0, UINT off_ms=0 ) :
        m_on_ms( on_ms ),
        m_off_ms( off_ms )
    {}
};

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

inline UIDSet UIDArrayToSet( UIDArray& uid_array ) {
    UIDSet uid_set;
    uid_set.insert( uid_array.begin(), uid_array.end() );
    return uid_set;
}

template <class T, class R>
R maxKeyValue( T& map ) {
    R maxValue = 0;

    for ( T::iterator it=map.begin(); it != map.end(); it++ ) {
        if ( it->first > maxValue )
            maxValue = it->first;
    }

    return maxValue;
}

class Venue;
class MusicPlayer;

class DMXStudio
{
    FILE*               m_hLog;
    bool                m_debug;
    CCriticalSection    m_venue_mutex;
    Venue*              m_venue;
    CString             m_venue_filename;
    UINT                m_http_port;
    StrobeTime          m_whiteout_strobe_slow;
    StrobeTime          m_whiteout_strobe_fast;
    bool                m_enable_mobile;
    MusicPlayer*        m_music_player;

public:
    DMXStudio();
    ~DMXStudio();

    void readIniFile();
    void writeIniFile();
        
    inline Venue* getVenue() {
        return m_venue;
    }

    void createMusicPlayer( LPCSTR username, LPCSTR player_dll_path );

    inline bool hasMusicPlayer() const {
        return m_music_player != NULL;
    }
    inline MusicPlayer* getMusicPlayer() const {
        return m_music_player;
    }

    LPCSTR getVenueFileName() const {
        return m_venue_filename;
    }
    void setVenueFileName( LPCSTR filename ) {
        m_venue_filename = filename;
    }

    bool getEnableMobile() const {
        return m_enable_mobile;
    }
    void setEnableMobile( bool m_enable_mobile ) {
        m_enable_mobile = m_enable_mobile;
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

    Venue* readVenue( const char *input_file );
    void writeVenue( const char *output_file );

    bool loadVenue( LPCSTR venue_filename );
    bool saveVenue( LPCSTR venue_filename );

private:
    CString getDefaultVenueFilename( void );

    void openStudioLogFile( void);
    void closeStudioLogFile( void );
    void showIpAddress( void );
};

extern DMXStudio studio;