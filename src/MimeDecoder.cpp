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

#include "MimeDecoder.h"

#define FORM_DATA_HEADER        "Content-Disposition: form-data;"
#define CONTENT_TYPE_HEADER     "Content-Type:"

// ----------------------------------------------------------------------------
//
MimeDecoder::MimeDecoder( LPBYTE buffer, DWORD buffer_size ) :
    m_buffer( buffer ),
    m_buffer_size( buffer_size )
{
    if ( !getNextLine( m_marker ) ) 
        throw StudioException( "Invalid multipart form data encountered" );

    m_end_marker.Format( "%s--", (LPCSTR)m_marker );    
}

// ----------------------------------------------------------------------------
//
LONG MimeDecoder::findString( LPCSTR search ) {
    LPBYTE fence = m_buffer;
    DWORD buffer_size = m_buffer_size;

    if ( *search == '\0' )
        return 0;

    UINT search_size = strlen( search );

    while ( buffer_size >= search_size ) {
        if ( !_strnicmp( (LPCSTR)fence, search, search_size ) )
            return (DWORD)(fence-m_buffer);

        fence++;
        buffer_size--;
    }

    return -1;
}

// ----------------------------------------------------------------------------
//
bool MimeDecoder::getNextLine( CString& line ) {
    DWORD pos = findString( "\r\n" );
    if ( pos == -1 )
        return false;

    LPSTR ptr = line.GetBufferSetLength( pos );
    memcpy( ptr, m_buffer, pos );
    line.ReleaseBufferSetLength( pos ); 

    m_buffer += pos+2;
    m_buffer_size -= (pos+2);

    return true;
}

// ----------------------------------------------------------------------------
//
bool MimeDecoder::nextPart( CString& field_name, CString& content_type ) {
    CString disposition;

    while ( true ) {
        if ( !getNextLine( disposition ) ) 
            throw StudioException( "Invalid multipart form - premature end" );
        
        if ( disposition == m_end_marker )
            return false;

        if ( disposition == m_marker )
            continue;

        size_t pos = disposition.Find( FORM_DATA_HEADER );
        if ( pos != 0 )
            throw StudioException( "Invalid multipart form - expected Content-Disposition header" );

        pos = disposition.Find( "name=\"" );
        if ( pos == -1 )
            throw StudioException( "Invalid multipart form data - missing field name" );

        pos += 6;

        size_t pos2 = disposition.Find( "\"", pos );
        if ( pos2 == -1 )
            throw StudioException( "Invalid multipart form data encountered" );

        field_name = disposition.Mid( pos, pos2-pos );

        if ( !getNextLine( content_type ) ) 
            throw StudioException( "Invalid multipart form data encountered" );

        if ( content_type.IsEmpty() )           // No content type
            return true;

        pos = content_type.Find( CONTENT_TYPE_HEADER );
        if ( pos != 0 )
            throw StudioException( "Invalid multipart form - expected Content-Type" );

        content_type = content_type.Mid( strlen(CONTENT_TYPE_HEADER)+1 );

        if ( !getNextLine( disposition ) )      // Blank line
            throw StudioException( "Invalid multipart form - expected empty line" );

        return true;
    }
}