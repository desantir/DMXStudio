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

#include "DMXHttpServer.h"

#define DMX_URL_ROOT_FULL                   DMX_URL_ROOT "full/"
#define DMX_URL_FULL_HOME                   DMX_URL_ROOT_FULL "full.htm"
#define DMX_SPOTIFY_AUTHORIZE				DMX_URL_ROOT "full/spotify/"

class HttpFull : public IRequestHandler
{
    typedef bool (HttpFull::*RestHandlerFunc)( CString& response, LPCSTR data );
    typedef std::map<CString, RestHandlerFunc> RestHandlerMap;

    RestHandlerMap      m_rest_handlers;

public:
    HttpFull( UINT port );
    ~HttpFull(void);

    LPCSTR getPrefix() {
        return DMX_URL_ROOT_FULL;
    }

    DWORD processGetRequest( HttpWorkerThread* worker );
    DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size );
    bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content ) { return false; }

private:
	bool spotify_authorize( HttpWorkerThread* worker, LPCSTR wquery, CString& response );
};




