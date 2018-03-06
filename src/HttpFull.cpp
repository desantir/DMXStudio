/* 
Copyright (C) 2011-2017 Robert DeSantis
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

#include "HttpFull.h"
#include "HttpUtils.h"
#include "Venue.h"

#define SPOTIFY_REDIRECT_URI "http://localhost/dmxstudio/full/spotify/"

// ----------------------------------------------------------------------------
//
HttpFull::HttpFull( UINT port ) :
    IRequestHandler( port )
{
}

// ----------------------------------------------------------------------------
//
HttpFull::~HttpFull(void)
{
}

// ----------------------------------------------------------------------------
//
DWORD HttpFull::processGetRequest( HttpWorkerThread* worker )
{
    try {
        CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
        int pos = path.Find( '?' );
        if ( pos != -1 )
			path = path.Left( pos );

        CString prefix( path );
        if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
            prefix += "/";

        if ( prefix == DMX_URL_ROOT_FULL ) {                  // Redirect to full index page
            return worker->sendRedirect( DMX_URL_FULL_HOME );
        }
		else if ( prefix == DMX_SPOTIFY_AUTHORIZE ) {		// Spotify authorize response
			CString response;
			bool success = spotify_authorize( worker, CW2A( worker->getRequest()->CookedUrl.pQueryString ), response );

			CString redirect_url;
			redirect_url.Format( "/dmxstudio/full/show-status.htm?status=%s", encodeString( response ) );
			worker->sendRedirect( (LPCSTR)redirect_url );

			studio.fireEvent( ES_STUDIO, NOUID, EA_MESSAGE, response, success ? 0 : 2 );

			if ( success )
				studio.fireEvent( ES_MUSIC_PLAYER, NOUID, EA_START );
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
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501();
    }
}

// ----------------------------------------------------------------------------
//
DWORD HttpFull::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    return worker->error_501();
}

// ----------------------------------------------------------------------------
// see https://developer.spotify.com/web-api/authorization-guide/

bool HttpFull::spotify_authorize( HttpWorkerThread* worker, LPCSTR query, CString& response ) {
	if ( !studio.getMusicPlayer()->isLoaded() || strcmp( "SPOTIFY", studio.getMusicPlayer()->getPlayerType() ) ) {
		response = "Spotify music player is not available";
		return false;
	}

	if ( query == NULL ) {
		response = "Spotify sign-on failed";
		return false;
	}

	BYTE *buffer = NULL;
	ULONG buffer_size = 0L;

	try {
		std::map<CString,CString> parameters;
		std::map<CString,CString>::iterator it;

		parseQuery( parameters, query );
	
		// Make sure this response copntains the expected state for this session	
		it = parameters.find( "state" );
		if ( it == parameters.end( ) || it->second != worker->getSession( )->getId( ) )
			throw std::exception( "invalid state" );

		// If there is an error field, then the sign-on failed
		it = parameters.find( "error" );
		if ( it != parameters.end( ) )
			throw std::exception( it->second );

		// Get the authorization code to get access and refresh tokens 	
		it = parameters.find( "code" );
		if ( it == parameters.end( ) )
			throw std::exception( "missing authorization code" );

		// Generate the body for our request
		CString body;
		body.Format( "grant_type=authorization_code&code=%s&redirect_uri=%s", (LPCSTR)it->second, (LPCSTR)encodeString( SPOTIFY_REDIRECT_URI ) );

		// Generate the authorization header
		char base64[2048];
		int base64len = sizeof(base64);
		encodeBase64( studio.getMusicPlayer()->getPlayerAuthorization(), base64, &base64len );

		CStringW http_headers;
		http_headers.Format( L"Authorization: Basic %s\r\nContent-Type: application/x-www-form-urlencoded\r\n", 
			(LPCWSTR)CA2W(base64) );

		httpPost( L"accounts.spotify.com", "/api/token", body, (LPCWSTR)http_headers, &buffer, &buffer_size );

		buffer = (BYTE *)realloc( buffer, buffer_size+1 );
		buffer[buffer_size] = '\0';

		if ( !studio.getMusicPlayer()->acceptAuthorization( buffer, buffer_size ) )
			throw std::exception( "authorization not accepted" );

		free( buffer );

		response = "Spotify sign-on successful!";
		return true;
	}
	catch ( std::exception& e ) {
		if ( buffer != NULL )
			free( buffer );

		studio.log( e );

		response.Format( "Spotity sign-on error (%s)", e.what() );
		return false;
	}
}

