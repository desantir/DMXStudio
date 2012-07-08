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
IniFile::IniFile(void)
{
}

// ----------------------------------------------------------------------------
//
IniFile::~IniFile(void)
{
}

// ----------------------------------------------------------------------------
//
void IniFile::read( LPCSTR filename, DMXStudio* studio )
{
    TiXmlDocument doc;
    if ( !doc.LoadFile( filename ) ) {
        DMXStudio::log( "Unable to load DMXStudio configuration file '%s' (%s)", filename, doc.ErrorDesc() );
        return;
    }

    TiXmlElement* dmx_studio = doc.FirstChildElement( "dmx_studio" );

    studio->setVenueFileName( read_text_element( dmx_studio, "venue_filename" ) );

    TiXmlElement* mobile = dmx_studio->FirstChildElement( "mobile" );
    if ( mobile ) {
        studio->setHttpPort( read_unsigned_attribute( mobile, "http_port" ) );
        studio->setEnableMobile( read_bool_attribute( mobile, "enable", true ) );
    }

    TiXmlElement* whiteout_slow = dmx_studio->FirstChildElement( "whiteout_slow" );
    if ( whiteout_slow ) {
        UINT on_ms = read_unsigned_attribute( whiteout_slow, "on_ms" );
        UINT off_ms = read_unsigned_attribute( whiteout_slow, "of_ms" );
        studio->setWhiteoutStrobeSlow( StrobeTime( on_ms, off_ms ) );
    }

    TiXmlElement* whiteout_fast = dmx_studio->FirstChildElement( "whiteout_fast" );
    if ( whiteout_slow ) {
        UINT on_ms = read_unsigned_attribute( whiteout_fast, "on_ms" );
        UINT off_ms = read_unsigned_attribute( whiteout_fast, "of_ms" );
        studio->setWhiteoutStrobeFast( StrobeTime( on_ms, off_ms ) );
    }

    TiXmlElement* music_player = dmx_studio->FirstChildElement( "music_player" );
    if ( music_player ) {
        studio->createMusicPlayer( read_text_element( music_player, "username" ),
                                   read_text_element( music_player, "library" ) );
    }

    DMXStudio::log_status( "Settings loaded from '%s'", filename );
}

// ----------------------------------------------------------------------------
//
void IniFile::write( LPCSTR filename, DMXStudio* studio )
{
    TiXmlDocument doc;

    TiXmlElement dmx_studio( "dmx_studio" );
    add_text_element( dmx_studio, "venue_filename", studio->getVenueFileName() );

    TiXmlElement mobile( "mobile" );
    add_attribute( mobile, "enable", studio->getEnableMobile() );
    add_attribute( mobile, "http_port", studio->getHttpPort() );
    dmx_studio.InsertEndChild( mobile );

    TiXmlElement whiteout_slow( "whiteout_slow" );
    add_attribute( whiteout_slow, "on_ms", studio->getWhiteoutStrobeSlow().m_on_ms );
    add_attribute( whiteout_slow, "of_ms", studio->getWhiteoutStrobeSlow().m_off_ms );
    dmx_studio.InsertEndChild( whiteout_slow );

    TiXmlElement whiteout_fast( "whiteout_fast" );
    add_attribute( whiteout_fast, "on_ms", studio->getWhiteoutStrobeFast().m_on_ms );
    add_attribute( whiteout_fast, "of_ms", studio->getWhiteoutStrobeFast().m_off_ms );
    dmx_studio.InsertEndChild( whiteout_fast );

    doc.InsertEndChild( dmx_studio );

    doc.SaveFile( filename );
}