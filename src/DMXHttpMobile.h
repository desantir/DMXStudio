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
#include "DMXHttpRestServices.h"
#include "MusicPlayer.h"

#define DMX_URL_ROOT_MOBILE                 DMX_URL_ROOT "mobile/"
#define DMX_URL_MOBILE_HOME                 DMX_URL_ROOT_MOBILE "mobile.htm"

class DMXHttpMobile : public DMXHttpRestServices
{
    typedef bool (DMXHttpMobile::*RestHandlerFunc)( CString& response, LPCSTR data );
    typedef std::map<CString, RestHandlerFunc> RestHandlerMap;

    RestHandlerMap      m_rest_handlers;

public:
    DMXHttpMobile(void);
    ~DMXHttpMobile(void);

    LPCSTR getPrefix() {
        return DMX_URL_ROOT_MOBILE;
    }

    UINT getPort() {
        return studio.getHttpPort();
    }

    DWORD processGetRequest( HttpWorkerThread* worker );
    DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size );
    bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content );

private:
    CString getFixtureDivContent();

    // Mobile UI specific responses
    bool control_fixture_capture( CString& response, LPCSTR data );

    // Potentially reusable
    bool control_animation_speed( CString& response, LPCSTR data );
    bool query_sound( CString& response, LPCSTR data );
};

