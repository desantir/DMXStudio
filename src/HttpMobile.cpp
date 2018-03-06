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

#include "HttpMobile.h"
#include "SimpleJsonBuilder.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
HttpMobile::HttpMobile( UINT port ) :
    IRequestHandler( port )
{
}

// ----------------------------------------------------------------------------
//
HttpMobile::~HttpMobile(void)
{
}

// ----------------------------------------------------------------------------
//
DWORD HttpMobile::processGetRequest( HttpWorkerThread* worker )
{
    CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    int pos = path.Find( '?' );
    if ( pos != -1 )                                        // Remove query string
       path = path.Left( pos );

    CString prefix( path );
    if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
        prefix += "/";

    if ( prefix == DMX_URL_ROOT_MOBILE ) {                  // Redirect to mobile index page
        return worker->sendRedirect( DMX_URL_MOBILE_HOME );
    }

    // Invoke the approriate handler
    RestHandlerFunc func = NULL;
    UINT len = 0;
    for ( RestHandlerMap::iterator it=m_rest_handlers.begin(); it != m_rest_handlers.end(); ++it ) {
        if ( prefix.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
            func = it->second;
            len = strlen(it->first);
        }
    }

    if ( func != NULL ) {
        CString response;
        if ( (this->*func)( response, path.Mid( len ) ) )
            return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
        return worker->error_501();
    }

    // Perhaps this is a file request
    if ( path.Find( DMX_URL_ROOT, 0 ) == 0 ) {
        path.Replace( DMX_URL_ROOT, "" );
        return worker->sendFile( (LPCSTR)path, this ); 
    }

    return worker->error_404();
}

// ----------------------------------------------------------------------------
//
DWORD HttpMobile::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    return worker->error_501();
}



