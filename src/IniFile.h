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

#include "DMXStudio.h"
#include "ISerializable.h"

class IniFile : public ISerializable
{
    CString         m_ini_filename;
    CString         m_last_error;

    CString         m_venue_filename;
    CString         m_venue_container;
    bool            m_http_enabled;
    UINT            m_http_port;
    StrobeTime      m_whiteout_strobe_slow;
    StrobeTime      m_whiteout_strobe_fast;
    bool            m_music_player_enabled;
    CString         m_music_player_ddl;
    CString         m_music_player_username;
    bool            m_dmx_required;
public:
    IniFile( LPCSTR filename );
    ~IniFile(void);

    bool read( void );
    void write( void );

    LPCSTR getLastError( void ) const {
        return m_last_error;
    }

    void setVenueFilename( LPCSTR filename ) {
        m_venue_filename = filename;
    }
    LPCSTR getVenueFilename( void ) const {
        return m_venue_filename;
    }

    LPCSTR getVenueContainer() const {
        return m_venue_container;
    }
    void setVenueContainer( LPCSTR container ) {
        m_venue_container = container;
    }

    bool isDMXRequired() const {
        return m_dmx_required;
    }
    void setDMXRequired( bool required ) {
        m_dmx_required = required;
    }

    void setHttpEnabled( bool mobile ) {
        m_http_enabled = mobile;
    }
    bool isHttpEnabled( void ) const {
        return m_http_enabled;
    }

    void setHttpPort( UINT port ) {
        m_http_port = port;
    }
    UINT getHttpPort( void ) const {
        return m_http_port;
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

    void setMusicPlayerEnabled( bool player_enabled ) {
        m_music_player_enabled = player_enabled;
    }
    bool isMusicPlayerEnabled( void ) const {
        return m_music_player_enabled;
    }

    void setMusicPlayer( LPCSTR player ) {
        m_music_player_ddl = player;
    }
    LPCSTR getMusicPlayer( void ) const {
        return m_music_player_ddl;
    }

    void setMusicUsername( LPCSTR username ) {
        m_music_player_username = username;
    }
    LPCSTR getMusicUsername( void ) const {
        return m_music_player_username;
    }
};

