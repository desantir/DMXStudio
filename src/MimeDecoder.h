/* 
Copyright (C) 2018 Robert DeSantis
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

#include "DMXStudio.h"

class MimeDecoder
{
        LPBYTE      m_buffer;
        DWORD       m_buffer_size;
        CString     m_end_marker;
        CString     m_marker;
public:
    MimeDecoder( LPBYTE buffer, DWORD buffer_size );

    ~MimeDecoder()
    {}

    inline void advance( DWORD advance ) {
        if ( m_buffer_size < advance )
            throw StudioException( "Advance past end of message buffer" );

        m_buffer += advance;
        m_buffer_size -= advance;
    }

    inline LPCSTR getMarker() const {
        return (LPCSTR)m_marker;
    }

    inline LPBYTE getBuffer() {
        return m_buffer;
    }

    LONG findString( LPCSTR search );
    bool getNextLine( CString& line );

    bool nextPart( CString& field_name, CString& content_type );
};

