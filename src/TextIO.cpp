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

#include "stdafx.h"
#include "TextIO.h"

// ----------------------------------------------------------------------------
//
TextIO::TextIO(void)
{
}

// ----------------------------------------------------------------------------
//
TextIO::~TextIO(void)
{
}

// ----------------------------------------------------------------------------
//
void TextIO::printf( const char *format, ... )
{
	va_list args;
	va_start( args, format );
	vprintf( format, args );
	va_end( args );
}

// ----------------------------------------------------------------------------
//
void TextIO::tokenize( CString& str ) {
	int curPos = 0;

	m_tokens.clear();

	do {
		CString resToken = str.Tokenize( _T(" \t"), curPos );
		if ( resToken.IsEmpty() )
			break;

		m_tokens.push_back( resToken );
	} 
	while ( true );
}

// ----------------------------------------------------------------------------
//
bool TextIO::nextToken( CString& str ) {
	if ( !haveTokens() )
		return false;

	str = m_tokens[0];
	m_tokens.erase( m_tokens.begin() );
	return true;
}

// ----------------------------------------------------------------------------
//
int TextIO::getString( CString& result, bool password ) {
	if  ( nextToken( result ) )
		return INPUT_SUCCESS;

	char buffer[80];
	int retcode = get_line( buffer, sizeof(buffer), password );
	if ( retcode != INPUT_SUCCESS && retcode != INPUT_SUCCESS_AND_EXIT )
		return retcode;

	result = buffer;

	return retcode;
}


// ----------------------------------------------------------------------------
//
int TextIO::get_line( char * buffer, int length, bool password ) {
	int index = 0;
	buffer[0] = '\0';

	while ( true ) {
		int ch = _getch();

		switch ( ch ){
			case 224:
				ch = _getch();
				//printf( "%d\n", ch );
				return ch;

			case 8:
				if ( index > 0 ) {
					buffer[--index] = '\0';
					printf( "\x08 \x08" );
				}
				break;

			case INPUT_EXIT:
			case INPUT_SUCCESS:
            case INPUT_SUCCESS_AND_EXIT:
				return ch;

			default:
				if ( index < length-1 ) {
					buffer[index++ ] = ch;
					buffer[index] = '\0';
					printf( "%c", password ? '*' : ch );
				}
				break;
		}
	}

	return -1;
}
