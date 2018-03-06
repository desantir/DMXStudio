/* 
Copyright (C) 2017 Robert DeSantis
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

#define NOUID                      0L
#define DMX_MAX_UNIVERSES          4
#define DMX_PACKET_SIZE            512
#define MULTI_UNIV_PACKET_SIZE     (DMX_PACKET_SIZE*DMX_MAX_UNIVERSES)

#define INVALID_CHANNEL             0xFFFF

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

typedef unsigned char BYTE;

typedef unsigned int universe_t;
typedef unsigned int channel_address;
typedef BYTE channel_value;

typedef DWORD UID;
typedef std::set<UID> UIDSet;
typedef std::vector<UID> UIDArray;
typedef std::vector<LPCSTR> LPCSTRArray;
typedef std::map<CString,CString> PropertyMap;

struct ChannelData {
	channel_address		m_address;
	channel_value		m_value;

	ChannelData( channel_address address, channel_value value ) :
		m_address( address ),
		m_value( value )
	{}

	ChannelData() 
	{}
	
	~ChannelData()
	{}
};

typedef std::vector<channel_address> ChannelList;
typedef std::vector<ChannelData> ChannelDataList;

inline UIDSet UIDArrayToSet( UIDArray& uid_array ) {
    UIDSet uid_set;
    uid_set.insert( uid_array.begin(), uid_array.end() );
    return uid_set;
}

template <class T, class R>
R maxKeyValue( T& map ) {
    R maxValue = 0;

    for ( T::iterator it=map.begin(); it != map.end(); ++it ) {
        if ( it->first > maxValue )
            maxValue = it->first;
    }

    return maxValue;
}

class StrobeTime {

	UINT					m_off_ms;
	UINT					m_on_ms;
    UINT                    m_flashes;
	UINT                    m_fade_in_ms;         // Fade into strobe time in MS (0=no fade)
	UINT                    m_fade_out_ms;        // Fade out of strobe time in MS (0=no fade)

public:
    StrobeTime( UINT on_ms=0, UINT off_ms=0, UINT fade_in_ms=0, UINT fade_out_ms=0, UINT flashes=1 ) :
        m_on_ms( on_ms ),
        m_off_ms( off_ms ),
        m_fade_in_ms( fade_in_ms ),
        m_fade_out_ms( fade_out_ms ),
		m_flashes( flashes )
    {}

	inline UINT getOffMS() const {
		return m_off_ms;
	}
	inline void setOffMS(UINT strobe_off_ms) {
		m_off_ms = strobe_off_ms;
	}

	inline UINT getOnMS() const {
		return m_on_ms;
	}
	inline void setOnMS(UINT strobe_on_ms) {
		m_on_ms = strobe_on_ms;
	}

	inline UINT getFadeOutMS() const {
		return m_fade_out_ms;
	}
	inline void setFadeOutMS(UINT fade_out_ms) {
		m_fade_out_ms = fade_out_ms;
	}

	inline UINT getFadeInMS() const {
		return m_fade_in_ms;
	}
	inline void setFadeInMS(UINT fade_in_ms) {
		m_fade_in_ms = fade_in_ms;
	}

	inline UINT getFlashes() const {
		return m_flashes;
	}
	inline void setFlashes(UINT strobe_flashes) {
		m_flashes = strobe_flashes;
	}
};
