/* 
Copyright (C) 2011-14 Robert DeSantis
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


#include "DMXStudio.h"
#include "Channel.h"

typedef std::map<ChannelType,CString> ChannelTypeToNameMap;

static ChannelTypeToNameMap channelTypeToNameMap;

static int populateChannelTypes() {
    channelTypeToNameMap[CHNLT_UNKNOWN] = "Unknown";
    channelTypeToNameMap[CHNLT_RED] = "Red";
    channelTypeToNameMap[CHNLT_GREEN] = "Green";
    channelTypeToNameMap[CHNLT_BLUE] = "Blue";
    channelTypeToNameMap[CHNLT_AMBER] = "Amber";
    channelTypeToNameMap[CHNLT_WHITE] = "White";
    channelTypeToNameMap[CHNLT_DIMMER] = "Dimmer";
    channelTypeToNameMap[CHNLT_STROBE] = "Strobe";
    channelTypeToNameMap[CHNLT_GOBO] = "Gobo";
    channelTypeToNameMap[CHNLT_PAN] = "Pan";
    channelTypeToNameMap[CHNLT_TILT] = "Tilt";
    channelTypeToNameMap[CHNLT_AUTOPROG] = "Auto Program";
    channelTypeToNameMap[CHNLT_COLOR_MACRO] = "Color Macro";
    channelTypeToNameMap[CHNLT_PAN_FINE] = "Pan Fine";
    channelTypeToNameMap[CHNLT_TILT_FINE] = "Tilt Fine";
    channelTypeToNameMap[CHNLT_MOVEMENT_SPEED] = "Movement Speed";
    channelTypeToNameMap[CHNLT_COLOR_SPEED] = "Color Speed";
    channelTypeToNameMap[CHNLT_MOVEMENT_MACRO] = "Movement Macro";
    channelTypeToNameMap[CHNLT_CONTROL] = "Control";
    channelTypeToNameMap[CHNLT_DIMMER_AND_STROBE] = "Dimmer/Strobe";
    channelTypeToNameMap[CHNLT_PROG_SPEED] = "Program Speed";
    channelTypeToNameMap[CHNLT_LASER] = "Laser";

    return 0;
}

static int static_kludge = populateChannelTypes();

// ----------------------------------------------------------------------------
//
Channel::Channel( channel_t offset, ChannelType type, const char *name ) :
    m_channel_offset( offset ),
    m_type( type ),
    m_default_value( 0 ),
    m_home_value( 0 ),
    m_is_dimmer( false ),
    m_lowest_intensity( 0 ),
    m_highest_intensity( 255 ),
    m_pixel_index( 0 ),
    m_head_number( 1 )
{
    if ( name == NULL )
        m_name = getTypeName( m_type );
    else
        m_name = name;
}

// ----------------------------------------------------------------------------
//
Channel::~Channel(void)
{
}

// ----------------------------------------------------------------------------
//
BYTE Channel::convertAngleToValue( int angle ) 
{
    AngleTable::iterator it = m_angle_table.find( angle );
    if ( it == m_angle_table.end() )
        return 0;
    return it->second;
}

// ----------------------------------------------------------------------------
//
void Channel::generateAngleTable(void) {
    m_angle_table.clear();

    if ( m_angles.size() == 0 )
        return;

    // I hope this is not overkill, but there does not appear to be simple, consistent
    // conversion from value to angle and vise versa for pan & tilt channels

    int angle_low = -1;
    int angle_high = 0;
    int value_low = 0;
    int value_high = 0;

    for ( ChannelAngleMap::iterator it=m_angles.begin(); it != m_angles.end(); ++it ) {
        if ( angle_low == -1 || it->first < angle_low ) {
            angle_low = it->first;
            value_low = it->second.getValue();
        }
    }

    int range_angle = -1;
    BYTE range_low = 0;
    BYTE range_high = 0;

    struct range_def {
        int range_angle;
        BYTE range_low;
        BYTE range_high;

        range_def( int angle, int low, int high ) :
            range_angle( angle ), range_low( low ), range_high( high ) {}
    };

    std::vector<range_def> ranges;

    while ( true ) {
        m_angle_table[ angle_low ] = value_low;

        if ( range_angle == -1 ) {
            range_low = value_low;
        }
        else {
            if ( value_low > range_high ) {
                ranges.push_back( range_def( range_angle, range_low, range_high ) );
                range_low = range_high + 1;		// Make sure we don't skip any values
            }
        }

        range_angle = angle_low;		// This is an absolute
        range_high = value_low;

        angle_high = -1;

        // Find next highest angle in the angle map

        for ( ChannelAngleMap::iterator it=m_angles.begin(); it != m_angles.end(); ++it ) {
            if ( it->first > angle_low && (angle_high == -1 || it->first < angle_high) ) {
                angle_high = it->first;
                value_high = it->second.getValue();
            }
        }

        if ( angle_high == -1 )
            break;

        float step = (float)(value_high-value_low) / (float)(angle_high-angle_low);

        for ( int angle=angle_low+1; angle < angle_high; angle++ ) {
            BYTE value = value_low + (BYTE)((angle-angle_low)*step+0.5);
            m_angle_table[ angle ] = value;

            if ( value > range_high ) {
                ranges.push_back( range_def( range_angle, range_low, range_high ) );
                range_angle = angle;
                range_low = range_high + 1;		// Make sure we don't skip any values
            }

            range_high = value;
        }

        angle_low = angle_high;
        value_low = value_high;
    }

    ranges.push_back( range_def( range_angle, range_low, range_high ) );

    if ( m_ranges.size() == 0 ) {
        CString range_name;
        for ( size_t i=0; i < ranges.size(); i++ ) {
            range_name.Format( "%d degrees", ranges[i].range_angle);
            m_ranges.push_back( ChannelValueRange( ranges[i].range_low, ranges[i].range_high, range_name, ranges[i].range_angle ) );
        }
    }
}

// ----------------------------------------------------------------------------
//
const char *Channel::getTypeName( ChannelType type ) {
    ChannelTypeToNameMap::iterator it=channelTypeToNameMap.find( type );
    STUDIO_ASSERT( it != channelTypeToNameMap.end(), "Unknown channel type %d", type );
    return it->second;
}

// ----------------------------------------------------------------------------
//
ChannelType Channel::convertTextToChannelType( LPCSTR text_type )
{
    // Try to convert text representation
    for ( ChannelTypeToNameMap::iterator it=channelTypeToNameMap.begin();
          it != channelTypeToNameMap.end();
          it++ )
    {
        if ( _stricmp( text_type, it->second ) == 0 )
            return it->first;
    }

    // Maybe a numeric representation
    ChannelType type = CHNLT_UNKNOWN;
    if ( sscanf_s( text_type, "%d", &type ) != 1 )
        type = CHNLT_UNKNOWN;

    STUDIO_ASSERT( type >= CHNLT_UNKNOWN && type < CHNLT_NUM_TYPES, "Invalid channel type %d", type );

    return type;
}

// ----------------------------------------------------------------------------
//
CString Channel::convertChannelTypeToText( ChannelType type )
{
    CString result = getTypeName( type );
    result.MakeLower();
    return result; 
}

