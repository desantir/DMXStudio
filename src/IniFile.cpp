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

#include "IniFile.h"
#include "SimpleJsonParser.h"
#include "SimpleJsonBuilder.h"

// ----------------------------------------------------------------------------
//
IniFile::IniFile() :
    m_http_port( 80 ),
    m_whiteout_strobe_slow( 50, 500 ),
    m_whiteout_strobe_fast( 50, 50 ),
    m_whiteout_strobe_manual( 50, 0 ),
    m_http_enabled( true ),
    m_music_player_enabled( false ),
    m_dmx_required( true ),
    m_debug( false ),
    m_videos_per_track( 25 ),
    m_video_palette_size( 10 ),
    m_hue_auto_setup( true )
{
}

// ----------------------------------------------------------------------------
//
IniFile::~IniFile(void)
{
}

// ----------------------------------------------------------------------------
//
bool IniFile::read( LPCSTR filename )
{
    FILE* fp;

    errno_t err = fopen_s( &fp, filename, "r" );
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
    m_google_api_key = parser.get<CString>( "google_api_key", "" );

    if ( parser.has_key( "remote" ) ) {
        JsonNode remote = parser.get<JsonNode>( "remote" );
        m_http_port = remote.get<unsigned>( "http_port", 80 );
        m_http_enabled = remote.get<bool>( "enable", true );
     }

    if ( parser.has_key( "videos" ) ) {
        JsonNode videos = parser.get<JsonNode>( "videos" );
        m_videos_per_track = videos.get<unsigned>( "videos_per_track", 12 );
        m_video_palette_size = videos.get<unsigned>( "video_palette_size", 12 );
    }

    if ( parser.has_key( "whiteout_slow" ) ) {
        JsonNode whiteout_slow = parser.get<JsonNode>( "whiteout_slow" );
        m_whiteout_strobe_slow.setOnMS( whiteout_slow.get<unsigned>( "on_ms" ) );
        m_whiteout_strobe_slow.setOffMS( whiteout_slow.get<unsigned>( "off_ms" ) );
        m_whiteout_strobe_slow.setFadeInMS( whiteout_slow.get<unsigned>( "fade_in_ms", 0 ) );
        m_whiteout_strobe_slow.setFadeOutMS( whiteout_slow.get<unsigned>( "fade_out_ms", 0 ) );
		m_whiteout_strobe_slow.setFlashes( whiteout_slow.get<unsigned>( "flashes", 1 ) );
    }

    if ( parser.has_key( "whiteout_fast" ) ) {
        JsonNode whiteout_fast = parser.get<JsonNode>( "whiteout_fast" );
        m_whiteout_strobe_fast.setOnMS( whiteout_fast.get<unsigned>( "on_ms" ) );
        m_whiteout_strobe_fast.setOffMS( whiteout_fast.get<unsigned>( "off_ms" ) );
        m_whiteout_strobe_fast.setFadeInMS( whiteout_fast.get<unsigned>( "fade_in_ms", 0 ) );
        m_whiteout_strobe_fast.setFadeOutMS( whiteout_fast.get<unsigned>( "fade_out_ms", 0 ) );
        m_whiteout_strobe_fast.setFlashes( whiteout_fast.get<unsigned>( "flashes", 1 ) );
    }

    if ( parser.has_key( "whiteout_manual" ) ) {
        JsonNode whiteout_manual = parser.get<JsonNode>( "whiteout_manual" );
        m_whiteout_strobe_manual.setOnMS( whiteout_manual.get<unsigned>( "on_ms" ) );
        m_whiteout_strobe_manual.setOffMS( 0 );
        m_whiteout_strobe_manual.setFadeInMS( whiteout_manual.get<unsigned>( "fade_in_ms", 0 ) );
        m_whiteout_strobe_manual.setFadeOutMS( whiteout_manual.get<unsigned>( "fade_out_ms", 0 ) );
        m_whiteout_strobe_manual.setFlashes( whiteout_manual.get<unsigned>( "flashes", 1 ) );
    }

    if ( parser.has_key( "music_player" ) ) {
        JsonNode music_player = parser.get<JsonNode>( "music_player" );
        m_music_player_ddl = music_player.get<CString>( "library" );
        m_music_player_username = music_player.get<CString>( "username" );

        m_music_player_enabled = true;
    }

    if ( parser.has_key( "hue" ) ) {
        JsonNode hue = parser.get<JsonNode>( "hue" );
        m_hue_bridge_ip = hue.get<CString>( "bridge_ip", "" );
        m_hue_auto_setup = hue.get<bool>( "auto_setup", true );

        if ( hue.has_key( "ignored_lights" ) ) {
            for ( UINT light_number : hue.getArrayAsVector<UINT>( "ignored_lights" ) ) {
                m_hue_ignored_lights.insert( light_number );
            }
        }

        if ( hue.has_key( "allowed_groups" ) ) {
            for ( CString& group_name : hue.getArrayAsVector<CString>( "allowed_groups" ) ) {
                m_hue_allowed_groups.insert( group_name );
            }
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
void IniFile::write( LPCSTR filename )
{
    JsonFileWriter ini( filename );
    JsonBuilder json( ini, true );

    json.startObject();

    json.add( "dmx_required", m_dmx_required );
    json.add( "debug", m_debug );
    json.add( "venue_filename", m_venue_filename );
    json.add( "venue_container", m_venue_container );
    json.add( "google_api_key", m_google_api_key );

    json.startObject( "videos" );
    json.add( "videos_per_track", m_videos_per_track );
    json.add( "video_palette_size", m_video_palette_size );
    json.endObject( "videos" );

    json.startObject( "remote" );
    json.add( "enable", m_http_enabled );
    json.add( "http_port", m_http_port );
    json.endObject( "remote" );

    json.startObject( "whiteout_slow" );
    json.add( "on_ms", m_whiteout_strobe_slow.getOnMS() );
    json.add( "off_ms", m_whiteout_strobe_slow.getOffMS() );
    json.add( "fade_in_ms", m_whiteout_strobe_slow.getFadeInMS() );
    json.add( "fade_out_ms", m_whiteout_strobe_slow.getFadeOutMS() );

    json.endObject( "whiteout_slow" );

    json.startObject( "whiteout_fast" );
    json.add( "on_ms", m_whiteout_strobe_fast.getOnMS() );
    json.add( "off_ms", m_whiteout_strobe_fast.getOffMS() );
    json.add( "fade_in_ms", m_whiteout_strobe_fast.getFadeInMS() );
    json.add( "fade_out_ms", m_whiteout_strobe_fast.getFadeOutMS() );
    json.endObject( "whiteout_fast" );

    json.startObject( "music_player" );
    json.add( "library", m_music_player_ddl );
    json.add( "username", m_music_player_username );
    json.endObject( "music_player" );

    json.startObject( "hue" );
    json.add( "bridge_ip", m_hue_bridge_ip );
    json.add( "auto_setup", m_hue_auto_setup );

    json.startArray( "ignored_lights" );
    for ( UINT id : m_hue_ignored_lights )
        json.add( id );
    json.endArray( "ignored_lights" );

    json.startArray( "allowed_groups" );
    for ( CString group : m_hue_allowed_groups )
        json.add( group );
    json.endArray( "allowed_groups" );

    json.endObject( "hue" );

    json.endObject();
}