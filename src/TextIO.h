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

#define INPUT_SUCCESS		    13
#define INPUT_SUCCESS_AND_EXIT	10
#define	INPUT_ERROR			    0
#define INPUT_EXIT			    27
#define INPUT_UP			    72
#define INPUT_DOWN			    80
#define INPUT_RIGHT			    77
#define INPUT_LEFT			    75
#define INPUT_HOME			    71
#define INPUT_INSERT		    82
#define INPUT_END			    79
#define INPUT_PGUP			    73
#define INPUT_PGDN			    81
#define INPUT_CTL_RIGHT		    116
#define INPUT_CTL_LEFT		    115

typedef std::vector<CString> TokenArray;

class TextIO
{
	TokenArray		m_tokens;				// List of available command tokens

	int get_line( char * buffer, int length, bool password=false );

public:
	TextIO(void);
	~TextIO(void);

	void tokenize( CString& str );
	bool nextToken( CString& str );

	bool haveTokens( ) {
		return m_tokens.size() > 0;
	}

	void clear() {
		m_tokens.clear();
	}

	int getString( CString& string, bool password=false );
	void printf( const char *format, ... );
};

