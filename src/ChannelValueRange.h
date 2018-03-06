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

#include "stdafx.h"
#include "IDefinitionVisitor.h"

class ChannelValueRange
{
    friend class DefinitionWriter;
    friend class DefinitionReader;

	channel_value	m_start;						// Start value (inclusive)
	channel_value	m_end;							// End value (inclusive)
    CString			m_name;							// Range name
    int				m_extra;                        // Extra informations such as pan/tilt angle

    ChannelValueRange() {}

public:
    ChannelValueRange( channel_value start, channel_value end, const char *name, int extra=0 );
    ~ChannelValueRange(void);

    void accept( IDefinitionVisitor* visitor) {
        visitor->visit(this);
    }

	channel_value getStart() const {
        return m_start;
    }

	channel_value getEnd() const {
        return m_end;
    }

    const char * getName() const {
        return m_name;
    }

    int getExtra() const {
        return m_extra;
    }
};

typedef std::vector<ChannelValueRange> ChannelValueRangeArray;
