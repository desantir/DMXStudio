/* 
Copyright (C) 2011-2016 Robert DeSantis
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
#include "SimpleJsonParser.h"
#include "SimpleJsonBuilder.h"

// ----------------------------------------------------------------------------
//
IniFile::IniFile( LPCSTR filename ) :
    m_ini_filename( filename ),
    m_http_port( 80 ),
    m_whiteout_strobe_slow( 300, 200 ),
    m_whiteout_strobe_fast( 100, 50 ),
    m_http_enabled( false ),
    m_music_player_enabled( false ),
    m_dmx_required( true ),
    m_debug( false )
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
    FILE* fp;

    errno_t err = fopen_s( &fp, m_ini_filename, "r" );
    if ( err || !fp )   // Assume it does not exist
        return false;
        
    SimpleJsonParser parser;

    try {
        parser.parse( fp );

        fclose( fp );
    }
    catch ( std::exception& e ) {
        fclose( fp );
        throw e;
    }
    
    m_dmx_required = parser.get<bool>( "dmx_required", true );
    m_debug = parser.get<bool>( "debug", false );
    m_venue_filename = parser.get<CString>( "venue_filename", "" );
    m_venue_container = parser.get<CString>( "venue_container", "" );

    if ( parser.has_key( "remote" ) ) {
        JsonNode remote = parser.get<JsonNode>( "remote" );
        m_http_port = parser.get<unsigned>( "http_port", 80 );
        m_http_enabled = parser.get<bool>( "enable", true );
    }

    if ( parser.has_key( "whiteout_slow" ) ) {
        JsonNode whiteout_slow = parser.get<JsonNode>( "whiteout_slow" );
        m_whiteout_strobe_slow.m_on_ms = whiteout_slow.get<unsigned>( "on_ms" );
        m_whiteout_strobe_slow.m_off_ms = whiteout_slow.get<unsigned>( "off_ms" );
    }

    if ( parser.has_key( "whiteout_fast" ) ) {
        JsonNode whiteout_fast = parser.get<JsonNode>( "whiteout_fast" );
        m_whiteout_strobe_fast.m_on_ms = whiteout_fast.get<unsigned>( "on_ms" );
        m_whiteout_strobe_fast.m_off_ms = whiteout_fast.get<unsigned>( "off_ms" );
    }

    if ( parser.has_key( "music_player" ) ) {
        JsonNode music_player = parser.get<JsonNode>( "music_player" );
        m_music_player_ddl = music_player.get<CString>( "library" );
        m_music_player_username = music_player.get<CString>( "username" );

        m_music_player_enabled = true;
    }

    return true;
}

// ----------------------------------------------------------------------------
//
void IniFile::write( )
{
    JsonFileWriter ini( m_ini_filename );
    JsonBuilder json( ini, true );

    json.startObject();

    json.add( "dmx_required", m_dmx_required );
    json.add( "debug", m_debug );
    json.add( "venue_filename", m_venue_filename );
    json.add( "venue_container", m_venue_container );

    json.startObject( "remote" );
    json.add( "enable", m_http_enabled );
    json.add( "http_port", m_http_port );
    json.endObject( "remote" );

    json.startObject( "whiteout_slow" );
    json.add( "on_ms", m_whiteout_strobe_slow.m_on_ms );
    json.add( "off_ms", m_whiteout_strobe_slow.m_off_ms );
    json.endObject( "whiteout_slow" );

    json.startObject( "whiteout_fast" );
    json.add( "on_ms", m_whiteout_strobe_fast.m_on_ms );
    json.add( "off_ms", m_whiteout_strobe_fast.m_off_ms );
    json.endObject( "whiteout_fast" );

    json.startObject( "music_player" );
    json.add( "library", m_music_player_ddl );
    json.add( "username", m_music_player_username );
    json.endObject( "music_player" );

    json.endObject();
}