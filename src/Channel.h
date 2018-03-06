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

enum ChannelType {
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
    CHNLT_LASER = 21,
    CHNLT_UV = 22,
    CHNLT_NUM_TYPES
} ;

typedef std::map<int,channel_value> AngleTable;

class Channel
{
    friend class DefinitionWriter;
    friend class DefinitionReader;

	channel_address			m_channel_offset;		// Channel offset
    CString				    m_name;					// Channel name
    ChannelType				m_type;					// Channel type
    BYTE                    m_pixel_index;          // Pixel index (1-255)

    bool					m_is_color;				// Is a color channel
    bool					m_can_blackout;			// Should be blacked out
    bool                    m_can_whiteout;         // Modify value on whiteout
	channel_value           m_default_value;        // Default channel value (when added to scene)
    bool                    m_dimmer_can_strobe;    // Fixture dimmer can be used to strobe
	channel_value           m_home_value;           // Default home value for ALL scenes (use to park robot position)

    bool                    m_is_dimmer;            // Channel supports dimming
	channel_value           m_lowest_intensity;     // Dimmer value for lowest intensity
	channel_value           m_highest_intensity;    // Dimmer value for highest intensity
	channel_value           m_off_intensity;        // Dimmer value for off (usually same as lowest intensity)

	bool					m_is_strobe;			// Channel supports strobing
	channel_value           m_strobe_slow;			// Stobe slowest
	channel_value           m_strobe_fast;			// Stobe fastest
	channel_value           m_strobe_off;			// Strobe off

    ChannelValueRangeArray	m_ranges;				// Describes channel value ranges
    ChannelAngleArray		m_angles;				// Angles for pan/tilt
    int                     m_min_angle;            // Pan/tilt minimum angle
    int                     m_max_angle;            // Pan/tilt maximum angle

    UINT                    m_head_number;          // For multiple pan/tilt heads
    AngleTable				m_angle_table;			// Table of angle -> DMX value

    void generateAngleTable(void);

public:
    Channel( channel_address offset=0, ChannelType type=CHNLT_UNKNOWN, const char *name = NULL );
    ~Channel(void);

    void accept( IDefinitionVisitor* visitor) {
        visitor->visit(this);
    }

    inline channel_address getOffset( ) const {
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

    inline bool canDimmerStrobe() const {
        return m_dimmer_can_strobe;
    }

    inline bool isColor() const {
        return m_is_color;
    }

    inline bool isDimmer() const { 
        return m_is_dimmer;
    }

    inline channel_value getDimmerLowestIntensity() const {
        return m_lowest_intensity;
    }

    inline channel_value getDimmerHighestIntensity() const {
        return m_highest_intensity;
    }

    inline channel_value getDimmerOffIntensity() const {
        return m_off_intensity;
    }

	inline bool isStrobe( ) const {
		return m_is_strobe;
	}

	inline channel_value getStrobeSlow() const {
		return m_strobe_slow;
	}

	inline channel_value getStrobeFast() const {
		return m_strobe_fast;
	}

	inline channel_value getStrobeOff() const {
		return m_strobe_off;
	}

    static const char *getTypeName( ChannelType type );

	channel_value convertAngleToValue( int angle );

    inline int getMinAngle() const {
        return m_min_angle;
    }
    inline int getMaxAngle() const {
        return m_max_angle;
    }

    inline channel_value getDefaultValue() const { 
        return m_default_value;
    }

    inline channel_value getHomeValue() const {
        return m_home_value;
    }

    inline BYTE getPixelIndex() const {
        return m_pixel_index;
    }

    inline UINT getHeadNumber() const {
        return m_head_number;
    }

    ChannelValueRangeArray getRanges() const {
        return m_ranges;
    }

    ChannelValueRange* getRange( channel_value value ) {
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

typedef std::vector<Channel*> ChannelPtrArray;

class ChannelValues {
    size_t			m_num_channels;
	channel_value   m_channel_values[64];      // Not worth a container for this

public:
    ChannelValues() :
        m_num_channels( 0 ) 
    {
        clearAllValues();
    }

    ChannelValues( size_t num_channels, channel_value *values ) :
        m_num_channels( num_channels )
    {
        STUDIO_ASSERT( m_num_channels < sizeof(m_channel_values), "Channel size out of bounds" );

        clearAllValues();

        memcpy( m_channel_values, values, m_num_channels );
    }

    inline size_t getNumChannels() const {
        return m_num_channels;
    }

    inline void setNumChannels( size_t num_channels ) {
        STUDIO_ASSERT( m_num_channels < sizeof(m_channel_values), "Channel size out of bounds" );
        m_num_channels = num_channels;
        clearAllValues();
    }

    inline const BYTE operator[]( size_t index ) const {
        STUDIO_ASSERT( index < m_num_channels, "Channel request out of bounds" );
        return m_channel_values[index];
    }

    inline void set( size_t index, channel_value value ) {
        STUDIO_ASSERT( index < m_num_channels, "Channel request out of bounds" );
        m_channel_values[index] = value;
    }

    inline void setWithGrow( size_t index, channel_value value ) {
        if ( index >= m_num_channels )
            m_num_channels = index + 1;
		STUDIO_ASSERT( m_num_channels < sizeof(m_channel_values), "Channel size out of bounds" );
        m_channel_values[index] = value;
    }

    inline void setAll( size_t num_channels, channel_value* values ) {
        STUDIO_ASSERT( num_channels < sizeof(m_channel_values), "Channel size out of bounds" );
        m_num_channels = num_channels;
        memcpy( m_channel_values, values, m_num_channels );
    }

    inline void clearAllValues() {
        memset( m_channel_values, 0, sizeof(m_channel_values) );
    }
};
