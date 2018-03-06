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

class StudioException : public std::exception 
{
    CString	    m_message;
    CString	    m_file;
    long		m_lineno;

public:
    StudioException( const char *format, ... ) :
        m_lineno( 0 )
    {
        va_list args;
        va_start( args, format );
        m_message.FormatV( format, args );
        va_end( args );
    }

    StudioException( const char * file, long lineno, const char *format, ... ) :
        m_file( file ),
        m_lineno( lineno )
    {
        va_list args;
        va_start( args, format );
        m_message.FormatV( format, args );
        va_end( args );
    }

    virtual const char* what() const throw() {
        return (LPCSTR)m_message;
     }

    const char *getFile() { return m_file; }
    long getLine() { return m_lineno; }
};

#define STUDIO_ASSERT( cond, ... )		\
    if ( !(cond) ) { throw StudioException( __FILE__, __LINE__, __VA_ARGS__ ); }
