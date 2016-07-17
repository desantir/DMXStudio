/* 
Copyright (C) 2011-2016 Robert DeSantis
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

#include "DMXHttpServer.h"
#include "HttpMobile.h"
#include "HttpFull.h"
#include "HttpRestServices.h"
#include "DMXHttpRedirector.h"

static const HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_1;

// ----------------------------------------------------------------------------
//
DMXHttpServer::DMXHttpServer(void) :
    Threadable( "DMXHttpServer" )
{
    registerHandler( new HttpMobile() );             // Textbook example of need for DI
    registerHandler( new HttpFull() );
    registerHandler( new HttpRestServices() );
    registerHandler( new DMXHttpRedirector() );

    // Some MIME code we recognize
    m_mime_map[ "jpg" ] = "image/jpeg";
    m_mime_map[ "png" ] = "image/png";
    m_mime_map[ "gif" ] = "image/gif";
    m_mime_map[ "css" ] = "text/css";
    m_mime_map[ "htm" ] = "text/html";
    m_mime_map[ "html" ] = "text/html";
    m_mime_map[ "js" ] = "text/html";
    m_mime_map[ "xml" ] = "text/xml";

    DMXStudio::getEventBus()->addListener( this );
}

// ----------------------------------------------------------------------------
//
DMXHttpServer::~DMXHttpServer(void)
{
    DMXStudio::getEventBus()->removeListener( this );

    freeWorkers();
    stopThread();

    // Release sessions
    for ( SessionMap::iterator it=m_sessions.begin(); it != m_sessions.end(); it++ ) {
        delete it->second;
    }
    m_sessions.clear();

    // Release all handlers
    for ( RequestHandlerPtrArray::iterator it=m_request_handlers.begin();
        it != m_request_handlers.end(); ++it ) {
        delete (*it);
    }
    m_request_handlers.clear();
}

// ----------------------------------------------------------------------------
//
void DMXHttpServer::createThreadPool( UINT thread_count, HANDLE hReqQueue )
{
    // Determine location of the docroot directory
    CString docroot;
    GetModuleFileName( NULL, docroot.GetBuffer( 1025 ), 1024 );
    docroot.ReleaseBuffer();

    int location = docroot.ReverseFind( '\\' );
    if ( location != -1 )
        docroot = docroot.Left( location+1 );
    else
        docroot.Append( "\\" );
    docroot.Append( "docroot\\" );

    for ( UINT i=0; i < thread_count; i++ ) {
        HttpWorkerThread* worker = new HttpWorkerThread( this, m_worker_threads.size()+1, hReqQueue, docroot );
        worker->start();
        m_worker_threads.push_back( worker );
    }
}

// ----------------------------------------------------------------------------
//
void DMXHttpServer::freeWorkers()
{
    for ( ThreadArray::iterator it=m_worker_threads.begin(); it != m_worker_threads.end(); ++it ) {
        (*it)->stop();
        delete (*it);
    }
    m_worker_threads.clear();
}

// ----------------------------------------------------------------------------
//
IRequestHandler* DMXHttpServer::getHandler( LPCSTR prefix, UINT port )
{
    CString path( prefix );
    if ( path.ReverseFind( '/' ) != path.GetLength()-1 )    // Must end in '/' to match handler names
        path += "/";

    IRequestHandler* func = NULL;
    UINT len = 0;

    for ( RequestHandlerPtrArray::iterator it=m_request_handlers.begin();
            it != m_request_handlers.end(); ++it ) {
        if ( (*it)->getPort() == port && path.Find( (*it)->getPrefix() ) == 0 && strlen((*it)->getPrefix()) > len ) {
            func = (*it);
            len = strlen((*it)->getPrefix());
        }
    }

    return func;
}

// ----------------------------------------------------------------------------
//
UINT DMXHttpServer::run()
{
    CString     listener_url;
    ULONG       retCode;
    HANDLE      hReqQueue = NULL;

    try {
        // Initialize HTTP Server APIs
        retCode = HttpInitialize( HttpApiVersion, HTTP_INITIALIZE_SERVER, NULL );
        if (retCode != NO_ERROR)
            throw StudioException( "HttpInitialize failed with %lu", retCode );

        // Create a Request Queue Handle
        retCode = HttpCreateHttpHandle( &hReqQueue, 0 );
        if ( retCode != NO_ERROR )
            throw StudioException( "HttpCreateHttpHandle failed with %lun", retCode );

        for ( RequestHandlerPtrArray::iterator it=m_request_handlers.begin();
                it != m_request_handlers.end(); ++it ) {
            listener_url.Format( "http://+:%d%s", (*it)->getPort(), (*it)->getPrefix() );

            retCode = HttpAddUrl( hReqQueue, CA2W(listener_url), NULL );
            if ( retCode == ERROR_ACCESS_DENIED ) 
                throw StudioException( "Access denied - add URL using 'netsh.exe http add urlacl url=%s user=[DOMAIN\\]<username>'", listener_url );
            if (retCode != NO_ERROR)
                throw StudioException( "HttpAddUrl failed with %lu", retCode );

            DMXStudio::log_status( "HTTP request listener started on '%s'", listener_url );
        }

        createThreadPool( 4, hReqQueue );

        CSingleLock lock( &m_session_lock, FALSE );
        DWORD session_purge = 0L;

        while ( isRunning() ) {
            DWORD now = GetTickCount();

            // Purge expired sessions
            if ( now > session_purge ) {
                lock.Lock();

                try {
                    for ( SessionMap::iterator it=m_sessions.begin(); it != m_sessions.end(); ) {
                        if ( it->second->isExpired( now ) ) {
                            DMXStudio::log_status( "Session expired %s", it->second->getId() );
                            delete it->second;
                            it = m_sessions.erase( it );
                        }
                        else
                            it++;
                    }
                }
                catch ( std::exception& e ) {
                    DMXStudio::log( e );
                }
                
                lock.Unlock();

                session_purge = now + (1000 * 60);
            }

            Sleep( 1000 );
        }

        freeWorkers();
    }
    catch ( ... ) {
        if ( hReqQueue ) 
            HttpRemoveUrl( hReqQueue, CA2W(listener_url) );

        HttpTerminate( HTTP_INITIALIZE_SERVER, 0 );
        throw;
    }

    HttpRemoveUrl( hReqQueue, CA2W(listener_url) );
    HttpTerminate( HTTP_INITIALIZE_SERVER, 0 );

    DMXStudio::log_status( "HTTP request listener stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
DMXHttpSession* DMXHttpServer::getSession( LPCSTR sessionId )
{
    CSingleLock lock( &m_session_lock, TRUE );

    auto it = m_sessions.find( sessionId );

    return ( it == m_sessions.end() ) ? NULL : it->second;
}

// ----------------------------------------------------------------------------
//
DMXHttpSession* DMXHttpServer::createSession()
{
    CSingleLock lock( &m_session_lock, TRUE );

    DMXHttpSession* session = new DMXHttpSession();
     
    m_sessions[ session->getId() ] = session;

    DMXStudio::log_status( "Created session %s", session->getId() );

    return session;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpServer::handleEvent( const Event& event )
{
    CSingleLock lock( &m_session_lock, TRUE );

    for ( SessionMap::iterator it=m_sessions.begin(); it != m_sessions.end(); it++ ) {
        // If the venus has stopped, release beat and sound detectors
        if ( event.m_source == ES_VENUE && event.m_action == EA_STOP ) {
            it->second->getBeatDetector().detach();
            it->second->getSoundSampler().detach();
        }

        it->second->queueEvent( event );
    }

    return false;
}
