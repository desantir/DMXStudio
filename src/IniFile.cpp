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

#include "IniFile.h"

// ----------------------------------------------------------------------------
//
IniFile::IniFile( LPCSTR filename ) :
    m_ini_filename( filename ),
    m_http_port( 80 ),
    m_whiteout_strobe_slow( 300, 200 ),
    m_whiteout_strobe_fast( 100, 50 ),
    m_http_enabled( false ),
    m_music_player_enabled( false ),
    m_dmx_required( true )
{
}

// ----------------------------------------------------------------------------
//
IniFile::~IniFile(void)
{
}

// ----------------------------------------------------------------------------
//
bool IniFile::read( )
{
    TiXmlDocument doc;
    if ( !doc.LoadFile( m_ini_filename ) ) {
        m_last_error = doc.ErrorDesc();
        return false;
    }

    TiXmlElement* dmx_studio = doc.FirstChildElement( "dmx_studio" );
    m_dmx_required = read_bool_attribute( dmx_studio, "dmx_required", true );

    m_venue_filename = read_text_element( dmx_studio, "venue_filename" );
    m_venue_container = read_text_element( dmx_studio, "venue_container" );

    TiXmlElement* remote = dmx_studio->FirstChildElement( "remote" );
    if ( remote ) {
        m_http_port = read_unsigned_attribute( remote, "http_port" );
        m_http_enabled = read_bool_attribute( remote, "enable", true );
    }

    TiXmlElement* whiteout_slow = dmx_studio->FirstChildElement( "whiteout_slow" );
    if ( whiteout_slow ) {
        m_whiteout_strobe_slow.m_on_ms = read_unsigned_attribute( whiteout_slow, "on_ms" );
        m_whiteout_strobe_slow.m_off_ms = read_unsigned_attribute( whiteout_slow, "of_ms" );
    }

    TiXmlElement* whiteout_fast = dmx_studio->FirstChildElement( "whiteout_fast" );
    if ( whiteout_slow ) {
        m_whiteout_strobe_fast.m_on_ms = read_unsigned_attribute( whiteout_fast, "on_ms" );
        m_whiteout_strobe_fast.m_off_ms = read_unsigned_attribute( whiteout_fast, "of_ms" );
    }

    TiXmlElement* music_player = dmx_studio->FirstChildElement( "music_player" );
    if ( music_player ) {
        m_music_player_enabled = true;
        m_music_player_ddl = read_text_element( music_player, "library" );
        m_music_player_username = read_text_element( music_player, "username" );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
void IniFile::write( )
{
    TiXmlDocument doc;

    TiXmlElement dmx_studio( "dmx_studio" );
    add_attribute( dmx_studio, "dmx_required", m_dmx_required );
    
    add_text_element( dmx_studio, "venue_filename", m_venue_filename );

    TiXmlElement mobile( "remote" );
    add_attribute( mobile, "enable", m_http_enabled );
    add_attribute( mobile, "http_port", m_http_port );
    dmx_studio.InsertEndChild( mobile );

    TiXmlElement whiteout_slow( "whiteout_slow" );
    add_attribute( whiteout_slow, "on_ms", m_whiteout_strobe_slow.m_on_ms );
    add_attribute( whiteout_slow, "of_ms", m_whiteout_strobe_slow.m_off_ms );
    dmx_studio.InsertEndChild( whiteout_slow );

    TiXmlElement whiteout_fast( "whiteout_fast" );
    add_attribute( whiteout_fast, "on_ms", m_whiteout_strobe_fast.m_on_ms );
    add_attribute( whiteout_fast, "of_ms", m_whiteout_strobe_fast.m_off_ms );
    dmx_studio.InsertEndChild( whiteout_fast );

    doc.InsertEndChild( dmx_studio );

    doc.SaveFile( m_ini_filename );
}