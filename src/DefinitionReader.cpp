/* 
Copyright (C) 2011-2015 Robert DeSantis
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


#include "DefinitionReader.h"
#include "DefinitionWriter.h"

// ----------------------------------------------------------------------------
//
DefinitionReader::DefinitionReader(void)
{
}

// ----------------------------------------------------------------------------
//
DefinitionReader::~DefinitionReader(void)
{
}

// ----------------------------------------------------------------------------
//
void DefinitionReader::readFixtureDefinitions( )
{
    TCHAR definitions_root[MAX_PATH]; 

    GetCurrentDirectory( MAX_PATH, definitions_root );
    strcat_s( definitions_root, "\\FixtureDefinitions" );

    DWORD attributes = GetFileAttributes( definitions_root );

    if ( attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) )
        throw StudioException( "Unable to find fixture definition directory '%s'", definitions_root );

    readFixtureDefinitions( definitions_root );

    size_t definitions = FixtureDefinition::FixtureDefinitions.size();

    if ( definitions == 0 )
        throw StudioException( "No fixture definitions found in '%s'", definitions_root );

    DMXStudio::log_status( "Found %d fixture personality definitions", definitions );
}

// ----------------------------------------------------------------------------
//
void DefinitionReader::readFixtureDefinitions( LPCSTR directory )
{
    CFileFind finder;
    
    CString directory_search( directory );
    directory_search.Append( "\\*" );

    BOOL working = finder.FindFile( directory_search );
    if ( !working ) 
        throw StudioException( "Unable to find fixture definition directory '%s'", directory );

    while ( working ) {
        working = finder.FindNextFile();
        if ( finder.IsDots() )
            continue;

        CString file_name = finder.GetFilePath();
        if ( finder.IsDirectory() )
            readFixtureDefinitions( file_name );

        CString test_name( file_name );
        if ( test_name.MakeLower().Find( ".xml" ) == -1 )
            continue;

        // Check for definition file
        TiXmlDocument doc;
        if ( !doc.LoadFile( file_name ) )
            throw StudioException( "Error reading fixture definition '%s'", file_name );

        try {
            TiXmlElement* root = doc.FirstChildElement( "fixture_definitions" );
            LPCSTR author = read_text_element( root, "author" );
            LPCSTR version = read_text_element( root, "version" );

            FixtureDefinitionPtrArray fixture_definitions = read_xml_list<FixtureDefinition>( root, "fixture" );

            //DefinitionWriter writer;
            //writer.writeFixtureDefinition( file_name, author, version, fixture_definitions );

            for ( FixtureDefinition* definition : fixture_definitions ) {
                definition->setSourceFile( file_name );
                definition->m_author = author;
                definition->m_version = version;

                FixtureDefinition::addFixtureDefinition( definition );
                delete definition;
            }
        }
        catch ( StudioException e ) {
            throw StudioException( "%s: %s", file_name, e.what() );
        }
    }
    
    finder.Close();
}

// ----------------------------------------------------------------------------
//
FixtureDefinition* DefinitionReader::read( TiXmlElement* self, FixtureDefinition* fixture )
{
    fixture = new FixtureDefinition();

    try {
        fixture->m_fuid = (FUID)read_dword_attribute( self, "fuid" );
        fixture->m_type = fixture->convertTextToFixtureType( read_text_attribute( self, "type" ) );
        fixture->m_manufacturer = read_text_element( self, "manufacturer" );
        fixture->m_model = read_text_element( self, "model" );

        // Add channel list(s)
        std::vector<Channel *> channels = 
            read_xml_list<Channel>( self->FirstChildElement( "channels" ), "channel" );

        for ( std::vector<Channel *>::iterator it=channels.begin(); it != channels.end(); ++it ) {
            fixture->m_channels.push_back( *(*it) );
            delete (*it);
        }

        if ( fixture->m_fuid == 0 ) {                           // Generate FUID with a (hopefully) unique hash
            fixture->m_fuid = fixture->generateFUID( );
        }

        fixture->chooseCapabilities();
    }
    catch ( ... ) {
        delete fixture;
        throw;
    }

    return fixture;
}

// ----------------------------------------------------------------------------
//
Channel* DefinitionReader::read( TiXmlElement* self, Channel* channel )
{
    channel = new Channel();

    try {
        channel->m_channel_offset = (channel_t)read_dword_attribute( self, "index" );
        channel->m_type = Channel::convertTextToChannelType( read_text_attribute( self, "type" ) );
        channel->m_name = read_text_element( self, "name" );
        channel->m_is_color = read_bool_attribute( self, "color" );
        channel->m_can_blackout = read_bool_attribute( self, "blackout" );
        channel->m_can_whiteout = read_bool_attribute( self, "whiteout", true );
        channel->m_default_value = (BYTE)read_int_attribute( self, "value" );
        channel->m_home_value = (BYTE)read_int_attribute( self, "home_value" );
        channel->m_pixel_index = (BYTE)read_int_attribute( self, "pixel" );
        channel->m_head_number = (BYTE)read_int_attribute( self, "head" );

        // If head number is not set on tilt or pan, default to 1
        if ( channel->m_head_number == 0 && 
             (channel->m_type == CHNLT_TILT
              || channel->m_type == CHNLT_PAN
              || channel->m_type == CHNLT_PAN_FINE
              || channel->m_type == CHNLT_TILT_FINE) )
            channel->m_head_number = 1;

        STUDIO_ASSERT( channel->m_channel_offset > 0, "Channel '%s' index < 1", channel->m_name );

        channel->m_channel_offset--;        // Adjust offset for internal zero based

        TiXmlElement *dimmer = self->FirstChildElement( "dimmer" );
        if ( dimmer ) {
            channel->m_is_dimmer = true;
            channel->m_lowest_intensity = (BYTE)read_int_attribute( dimmer, "lowest_intensity", 0 );
            channel->m_highest_intensity = (BYTE)read_int_attribute( dimmer, "highest_intensity", 255 );
            channel->m_off_intensity = (BYTE)read_int_attribute( dimmer, "off_intensity", channel->m_lowest_intensity );
        }
        else {
            channel->m_is_dimmer = ( channel->m_type == CHNLT_DIMMER );     // Implies this is the default 0-255 dimmer channel type
            channel->m_lowest_intensity = 0;
            channel->m_highest_intensity = 255;
            channel->m_off_intensity = 0;
        }

        // Add channel ranges
        std::vector<ChannelValueRange *> ranges = 
            read_xml_list<ChannelValueRange>( self->FirstChildElement( "ranges" ), "range" );

        for ( std::vector<ChannelValueRange *>::iterator it=ranges.begin(); it != ranges.end(); ++it ) {
            STUDIO_ASSERT( (*it)->getEnd() >= (*it)->getStart(), "Channel '%s' range %s invalid", channel->m_name, (*it)->getName() );
            STUDIO_ASSERT( channel->getRange( (*it)->getEnd() ) == NULL, "Channel '%s' range %s overlaps", channel->m_name, (*it)->getName() );
            STUDIO_ASSERT( channel->getRange( (*it)->getStart() ) == NULL, "Channel '%s' range %s overlaps", channel->m_name, (*it)->getName() );
            channel->m_ranges.push_back( *(*it) );
            delete (*it);
        }

        // Add angles
        std::vector<ChannelAngle *> angles = 
            read_xml_list<ChannelAngle>( self->FirstChildElement( "angles" ), "angle" );

        for ( std::vector<ChannelAngle *>::iterator it=angles.begin(); it != angles.end(); ++it ) {
            channel->m_angles[ (*it)->getAngle() ] = *(*it);
            delete (*it);
        }

        channel->generateAngleTable();
    }
    catch( ... ) {
        delete channel;
        throw;
    }

    return channel;
}

// ----------------------------------------------------------------------------
//
ChannelAngle* DefinitionReader::read( TiXmlElement* self, ChannelAngle* channel_angle )
{
    channel_angle = new ChannelAngle();

    channel_angle->m_angle = (int)read_int_attribute( self, "degrees" );
    channel_angle->m_value = (BYTE)read_int_attribute( self, "value" );

    return channel_angle;
}

// ----------------------------------------------------------------------------
//
ChannelValueRange* DefinitionReader::read( TiXmlElement* self, ChannelValueRange* range )
{
    range = new ChannelValueRange();

    range->m_start = (BYTE)read_dword_attribute( self, "start" );
    range->m_end = (BYTE)read_dword_attribute( self, "end" );
    range->m_name = read_text_element( self, "name" );

    return range;
}
