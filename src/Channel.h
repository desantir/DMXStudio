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

#include "ChannelValueRange.h"
#include "ChannelAngle.h"
#include "IDefinitionVisitor.h"

typedef enum channel_type {
    CHNLT_UNKNOWN = 0,
    CHNLT_RED = 1,
    CHNLT_GREEN = 2,
    CHNLT_BLUE = 3,
    CHNLT_AMBER = 4,
    CHNLT_WHITE = 5,
    CHNLT_DIMMER = 6,
    CHNLT_STROBE = 7,
    CHNLT_GOBO = 8,
    CHNLT_PAN = 9,
    CHNLT_TILT = 10,
    CHNLT_AUTOPROG = 11,
    CHNLT_COLOR_MACRO = 12,
    CHNLT_PAN_FINE = 13,
    CHNLT_TILT_FINE = 14,
    CHNLT_MOVEMENT_SPEED = 15,
    CHNLT_COLOR_SPEED = 16,
    CHNLT_MOVEMENT_MACRO = 17,
    CHNLT_CONTROL = 18,
    CHNLT_DIMMER_AND_STROBE = 19,
    CHNLT_PROG_SPEED = 20,
    CHNLT_NUM_TYPES

} ChannelType;

typedef std::map<int,BYTE> AngleTable;

class Channel
{
    friend class DefinitionWriter;
    friend class DefinitionReader;

    channel_t				m_channel_offset;		// Channel offset
    CString				    m_name;					// Channel name
    ChannelType				m_type;					// Channel type
    bool					m_is_color;				// Is a color channel
    bool					m_can_blackout;			// Should be blacked out
    bool                    m_can_whiteout;         // Modify value on whiteout
    BYTE                    m_default_value;        // Default channel value (when added to scenee)
    BYTE                    m_home_value;           // Default home value for ALL scenes (use to park robot position)

    bool                    m_is_dimmer;            // Channel supports dimming
    BYTE                    m_lowest_intensity;     // Dimmer value for lowest intensity
    BYTE                    m_highest_intensity;    // Dimmer value for highest intensity

    ChannelValueRangeArray	m_ranges;				// Describes channel value ranges
    ChannelAngleMap			m_angles;				// Angles for pan/tilt

    AngleTable				m_angle_table;			// Table of angle -> DMX value

    void generateAngleTable(void);

public:
    Channel( channel_t offset=0, ChannelType type=CHNLT_UNKNOWN, const char *name = NULL );
    ~Channel(void);

    void accept( IDefinitionVisitor* visitor) {
        visitor->visit(this);
    }

    inline channel_t getOffset( ) const {
        return m_channel_offset;
    }

    inline const char* getName( ) const {
        return m_name;
    }

    inline ChannelType getType() const {
        return m_type;
    }

    inline bool canBlackout() const {
        return m_can_blackout;
    }

    inline bool canWhiteout() const {
        return m_can_whiteout;
    }

    inline bool isColor() const {
        return m_is_color;
    }

    inline bool isDimmer() const { 
        return m_is_dimmer;
    }

    inline BYTE getDimmerLowestIntensity() const {
        return m_lowest_intensity;
    }

    inline BYTE getDimmerHighestIntensity() const {
        return m_highest_intensity;
    }

    static const char *getTypeName( ChannelType type );

    BYTE convertAngleToValue( int angle );

    inline BYTE getDefaultValue() const { 
        return m_default_value;
    }

    inline BYTE getHomeValue() const {
        return m_home_value;
    }

    ChannelValueRangeArray getRanges() const {
        return m_ranges;
    }

    ChannelValueRange* getRange( BYTE value ) {
        for ( ChannelValueRangeArray::iterator rit=m_ranges.begin(); rit != m_ranges.end(); rit++ ) {
            if ( value >= (*rit).getStart() && value <= (*rit).getEnd() ) {
                return &(*rit);
            }
        }
        return NULL;
    }

private:
    static ChannelType convertTextToChannelType( LPCSTR text_type );
    static CString convertChannelTypeToText( ChannelType type );
};

