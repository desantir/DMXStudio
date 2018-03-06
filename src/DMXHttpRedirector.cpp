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

#include "DMXHttpRedirector.h"
#include "HttpMobile.h"
#include "HttpFull.h"

// ----------------------------------------------------------------------------
//
DMXHttpRedirector::DMXHttpRedirector( UINT port ) :
    IRequestHandler( port )
{
}

// ----------------------------------------------------------------------------
//
DMXHttpRedirector::~DMXHttpRedirector(void)
{
}

// ----------------------------------------------------------------------------
//
DWORD DMXHttpRedirector::processGetRequest( HttpWorkerThread* worker )
{
    CString prefix( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
        prefix += "/";

    if ( prefix != DMX_URL_ROOT )      
        return worker->error_404();

    // Redirect to full or mobile based on user agent
    
    // IE9 - Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; WOW64; Trident/5.0)
    // iPad - Mozilla/5.0 (iPad; CPU OS 6_0 like Mac OS X) AppleWebKit/536.26 (KHTML, like Gecko) Version/6.0 Mobile/10A403 Safari/8536.25
    // Android - Mozilla/5.0 (Linux; U; Android 2.3.5; en-us; Sprint APA9292KT Build/GRJ90) AppleWebKit/533.1 (KHTML, like Gecko) Version/4.0 Mobile Safari/533.1

    if ( worker->getRequest()->Headers.KnownHeaders[HttpHeaderUserAgent].RawValueLength > 0 ) {
        CString user_agent = worker->getRequest()->Headers.KnownHeaders[HttpHeaderUserAgent].pRawValue;
        user_agent.MakeLower();

        LPCSTR redirect;

        if ( user_agent.Find( "ipad" ) != -1 || user_agent.Find( "iphone" ) != -1 || user_agent.Find( "android" ) != -1 )
            redirect = DMX_URL_MOBILE_HOME;
        else
            redirect = DMX_URL_FULL_HOME;

        return worker->sendRedirect( redirect );
    }

    return worker->error_404();
}

// ----------------------------------------------------------------------------
//
DWORD DMXHttpRedirector::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    return worker->error_501();
}