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
}

// ----------------------------------------------------------------------------
//
DMXHttpServer::~DMXHttpServer(void)
{
    freeWorkers();
    stopThread();

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
                throw StudioException( "Access denied - add URL using 'netsh.exe http add urlacl url=%s user=DOMAIN\user'", listener_url );
            if (retCode != NO_ERROR)
                throw StudioException( "HttpAddUrl failed with %lu", retCode );

            DMXStudio::log_status( "HTTP request listener started on '%s'", listener_url );
        }

        createThreadPool( 2, hReqQueue );

        while ( isRunning() ) {
            // Hopefully we can do something more useful here in the future
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
HttpWorkerThread::HttpWorkerThread( DMXHttpServer* httpServer, UINT worker_id, 
                                    HANDLE hReqQueue, LPCSTR docroot ) :
    Threadable( "HttpWorkerThread" ),
    m_httpServer( httpServer ),
    m_worker_id( worker_id ),
    m_hReqQueue( hReqQueue ),
    m_docroot( docroot ),
    m_RequestBufferLength( 0 ),
    m_pRequest( NULL )
{    
    resizeRequestBuffer( sizeof(HTTP_REQUEST) + 2048 );
}

// ----------------------------------------------------------------------------
//
HttpWorkerThread::~HttpWorkerThread(void) {
    stop();

    if ( m_pRequest )
        FREE_MEM( m_pRequest );
}

// ----------------------------------------------------------------------------
//
void HttpWorkerThread::resizeRequestBuffer( DWORD new_size ) {
    m_RequestBufferLength = new_size;

    if ( m_pRequest )
        FREE_MEM( m_pRequest );

    m_pRequest = (PHTTP_REQUEST)ALLOC_MEM( m_RequestBufferLength );
    if ( m_pRequest == NULL )
        throw StudioException( "Out of memory" );
}

// ----------------------------------------------------------------------------
//
CString encodeHtmlString( LPCSTR string )
{
    CString result( string );
    result.Replace( "<", "&lt;" );
    result.Replace( ">", "&gt;" );
    result.Replace( "\"", "&quot;" );
    return result;
}

// ----------------------------------------------------------------------------
//
UINT HttpWorkerThread::run() 
{
    HTTP_REQUEST_ID     requestId;
    OVERLAPPED          overlapped;

    DMXStudio::log_status( "HTTP worker thread %d started", m_worker_id );

    CEvent event;

    BOOL new_request = true;

    while ( isRunning() ) {
        if ( new_request ) {
            memset( &overlapped, 0, sizeof(OVERLAPPED) );
            overlapped.hEvent = event.m_hObject;

            memset( m_pRequest, 0, m_RequestBufferLength);
            HTTP_SET_NULL_ID( &requestId );
            new_request = false;
        }

        try {
            DWORD bytes_transferred = 0;

            ULONG result = HttpReceiveHttpRequest(
                        m_hReqQueue,                                // Req Queue
                        requestId,                                  // Req ID
                        0,                                          // Flags
                        m_pRequest,                                 // HTTP request buffer
                        m_RequestBufferLength,                      // req buffer length
                        NULL,                                       // bytes received (NULL for overlapped)
                        &overlapped );                              // LPOVERLAPPED

            // Overlapped will return ERROR_IO_PENDING
            if ( ERROR_IO_PENDING == result ) {
                for ( bool pending=true; pending; ) {
                    BOOL success = GetOverlappedResult( m_hReqQueue, &overlapped, &bytes_transferred, false );
                    if ( success ) {
                        result = NO_ERROR;
                        break;
                    }

                    result = GetLastError();

                    switch ( result ) { 
                        case ERROR_IO_INCOMPLETE:
                            if ( !isRunning() )
                                return 0;
                            Sleep(10);
                            break;

                        case ERROR_MORE_DATA:
                        case ERROR_HANDLE_EOF: 
                        default:
                            pending = false;
                            break;
                    } 
                }
            }

            switch ( result ) {
                case NO_ERROR:
                    switch ( m_pRequest->Verb ) {
                        case HttpVerbGET:
                            result = processGetRequest( );
                            break;

                        case HttpVerbPOST:
                            result = processPostRequest( );
                            break;

                        default:
                            result = sendResponse( 503, "Not Implemented", NULL );
                            break;
                    }

                    new_request = true;
                    break;
  
                case ERROR_MORE_DATA:
                    requestId = m_pRequest->RequestId;
                    resizeRequestBuffer( bytes_transferred );
                    break;

                case ERROR_CONNECTION_INVALID:
                default:
                    // The TCP connection was corrupted by the peer when
                    // attempting to handle a request with more buffer. 
                    new_request = true;
                    break;
            }
        }
        catch( std::exception& ex ) {
            DMXStudio::log( ex );
            new_request = true;
        }
    }

    return 0;
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::sendAttachment( LPBYTE contents, DWORD size, LPCSTR mime, LPCSTR attachment_name )
{
    CString content_disposition;
    content_disposition.Format( "attachment; filename=%s", attachment_name );

    DWORD result = sendResponse( 200, "OK", (LPBYTE)contents, size, true, "text/xml", (LPCSTR)content_disposition );
    return result;
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::sendFile( LPCSTR file_name, IRequestHandler* handler )
{
    CString full_file_path( m_docroot );
    full_file_path += file_name;
    full_file_path.Replace( "/", "\\" );

    FILE* pFile = NULL;
    fopen_s( &pFile, full_file_path, "rb" );
    if ( pFile == NULL ) {
        DMXStudio::log( "%d: HTTP file not found (404) '%s'", m_worker_id, (LPCSTR)full_file_path );
        return error_404( );
    }

    long size;
    LPBYTE contents;
    LPCSTR mime = NULL;
    bool no_cache = false;

    // Get MIME type
    CString extention;
    int position = full_file_path.ReverseFind( '.' );
    if ( position != -1 ) {
        mime = m_httpServer->lookupMimeForExtension( full_file_path.Mid( position+1 ) );
    }
    if ( mime == NULL )
        mime = "text/html";

    // Obtain file size:
    fseek( pFile, 0 , SEEK_END );
    size = ftell( pFile );
    rewind( pFile );

    // copy the file into a buffer
    contents = (LPBYTE)malloc( size+1 );
    STUDIO_ASSERT( contents != NULL, "Out of memory" );
    size = fread( contents, 1, size, pFile );
    contents[size] = '\0';

    if ( strncmp( mime, "text/", 5 ) == 0 ) {
        if ( handler && strstr( (LPCSTR)contents, "[$[" ) != NULL ) {   // TODO ONLY DO THIS FOR TEXT TYPES
            CString file_contents( contents );

            int start = 0;
            bool changed = false;

            while ( start < file_contents.GetLength() ) {
                int marker_begin = file_contents.Find( "[$[", start );
                if ( marker_begin == -1 )
                    break;
                int marker_end = file_contents.Find( "]$]", marker_begin );
                if ( marker_end == -1 )
                    break;

                CString marker = file_contents.Mid( marker_begin+3, marker_end-marker_begin-3 );
                CString data;

                int pos = marker.Find( ':' );
                if ( pos > -1 ) {
                    data = marker.Mid( pos+1 );
                    marker = marker.Left( pos );
                }

                CString marker_content;
                
                if ( handler->substitute( marker, data, marker_content ) ) {
                    file_contents = file_contents.Left( marker_begin ) + marker_content + file_contents.Mid( marker_end+3 );
                    start = marker_begin + marker_content.GetLength();
                    changed = true;
                }
                else {
                    start = marker_begin+1;
                }
            }

            if ( changed ) {
                size = file_contents.GetLength();
                contents = (LPBYTE)realloc( contents, size+1 );
                STUDIO_ASSERT( contents != NULL, "Out of memory" );
                memcpy( contents, file_contents, size );
            }
        }
    }

    fclose( pFile );
    
    try {
        DWORD result = sendResponse( 200, "OK", contents, size, no_cache, mime );
        free( contents );
        return result;
    }
    catch ( ... ) {
        free( contents );
        throw;
    }
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::sendRedirect( LPCSTR location )
{
    HTTP_RESPONSE   response;
    DWORD           result;

    INITIALIZE_HTTP_RESPONSE( &response, 302, "Moved Temporarily" );

    CString host( m_pRequest->CookedUrl.pHost );
    int pos = host.Find( "/" );
    if ( pos != -1 )
        host = host.Left( pos );

    if ( location[0] != '/' )
        host += "/";

    CString full_location( "http://" );
    full_location += host;
    full_location += location;

    // Add headers
    ADD_KNOWN_HEADER(response, HttpHeaderLocation, (LPCSTR)full_location );
   
    DMXStudio::log( "WORKER %d: HTTP response %d '%s'", m_worker_id, response.StatusCode, response.pReason );

    result = HttpSendHttpResponse(
                    m_hReqQueue,           // ReqQueueHandle
                    m_pRequest->RequestId, // Request ID
                    0,                   // Flags
                    &response,           // HTTP response
                    NULL,                // pReserved1
                    NULL,                // bytes sent  (OPTIONAL)
                    NULL,                // pReserved2  (must be NULL)
                    0,                   // Reserved3   (must be 0)
                    NULL,                // LPOVERLAPPED(OPTIONAL)
                    NULL                 // pReserved4  (must be NULL)
                    ); 

    if ( result != NO_ERROR )
        throw StudioException( "%d: HttpSendHttpResponse 302 failed with %lu \n", m_worker_id, result);

    return result;
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::sendResponse( 
    IN USHORT        StatusCode,
    IN LPCSTR        pReason,
    IN LPCSTR        pEntityString )
{
    ULONG length = 0;

    if ( pEntityString )
        length = strlen( pEntityString );

    return sendResponse( StatusCode, pReason, (LPBYTE)pEntityString, length, true, "text/html" );
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::sendResponse(
    IN USHORT        StatusCode,
    IN LPCSTR        pReason,
    IN LPBYTE        pEntity,
    IN ULONG         entityLength,
    IN BOOL          bNoCache,
    IN LPCSTR        mime,
    IN LPCSTR        disposition
    )
{
    HTTP_RESPONSE           response;
    HTTP_DATA_CHUNK         dataChunk;
    DWORD                   result;
    DWORD                   bytesSent;
    HTTP_UNKNOWN_HEADER     unknown_header;

    // Initialize the HTTP response structure.
    INITIALIZE_HTTP_RESPONSE( &response, StatusCode, pReason );

    // Add headers
    ADD_KNOWN_HEADER( response, HttpHeaderContentType, mime );

    if ( bNoCache ) {
        ADD_KNOWN_HEADER( response, HttpHeaderPragma, "no-cache" );
        ADD_KNOWN_HEADER( response, HttpHeaderCacheControl, "no-cache" );
    }
    else {
        ADD_KNOWN_HEADER( response, HttpHeaderExpires, "Thu, 27 Dec 2012 16:00:00 GMT" );       // TODO - Compute this
    }

    if ( disposition != NULL ) {
        unknown_header.pName = "Content-Disposition";
        unknown_header.NameLength = strlen(unknown_header.pName);
        unknown_header.pRawValue = disposition;
        unknown_header.RawValueLength = strlen( unknown_header.pRawValue );
        response.Headers.UnknownHeaderCount = 1;
        response.Headers.pUnknownHeaders = &unknown_header;
    }

    DMXStudio::log( "WORKER %d: HTTP response %d '%s'", m_worker_id, response.StatusCode, response.pReason );

    if ( entityLength ) {
        // Add an entity chunk.
        dataChunk.DataChunkType           = HttpDataChunkFromMemory;
        dataChunk.FromMemory.pBuffer      = (LPVOID)pEntity;
        dataChunk.FromMemory.BufferLength = entityLength;

        response.EntityChunkCount         = 1;
        response.pEntityChunks            = &dataChunk;
    }

    // Because the entity body is sent in one call, it is not
    // required to specify the Content-Length.
    result = HttpSendHttpResponse(
                    m_hReqQueue,         // ReqQueueHandle
                    m_pRequest->RequestId, // Request ID
                    0,                   // Flags
                    &response,           // HTTP response
                    NULL,                // pReserved1
                    &bytesSent,          // bytes sent  (OPTIONAL)
                    NULL,                // pReserved2  (must be NULL)
                    0,                   // Reserved3   (must be 0)
                    NULL,                // LPOVERLAPPED(OPTIONAL)
                    NULL                 // pReserved4  (must be NULL)
                    ); 

    if ( result != NO_ERROR )
        throw StudioException( "%d: HttpSendHttpResponse %d failed with %lu \n",  m_worker_id, StatusCode, result);

    return result;
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::processGetRequest()
{
    CString path( CW2A( m_pRequest->CookedUrl.pAbsPath ) );

    DMXStudio::log( "WORKER %d: HTTP GET request '%s'", m_worker_id, (LPCSTR)path );

    IRequestHandler* handler = m_httpServer->getHandler( path, getRequestPort() );
    if ( handler != NULL )
        return handler->processGetRequest( this );

    return error_404( );
}

// ----------------------------------------------------------------------------
//
DWORD HttpWorkerThread::processPostRequest()
{
    LPBYTE          pEntityBuffer;
    ULONG           EntityBufferLength = 4096;
    ULONG           TotalBytesRead = 0;
    ULONG           BufferOffset = 0;
    DWORD           result;
    
    CString path( CW2A( m_pRequest->CookedUrl.pAbsPath ) );
    DMXStudio::log( "WORKER %d: HTTP POST request '%s'", m_worker_id, (LPCSTR)path );

    pEntityBuffer = (LPBYTE)ALLOC_MEM( EntityBufferLength+1 );  // +1 in case we read _exactly_ EntityBufferLength (need room for null)
    if (pEntityBuffer == NULL) 
        throw StudioException( "Unable to allocate memory for POST request buffer" );
    *pEntityBuffer = '\0';

    try {
        if ( m_pRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS ) {
            bool done = false;

            // The entity body is sent over multiple calls.
            while ( !done ) {
                ULONG BytesRead = 0; 

                // Read the entity chunk from the request.
                result = HttpReceiveRequestEntityBody(
                            m_hReqQueue,
                            m_pRequest->RequestId,
                            0,
                            &pEntityBuffer[BufferOffset],
                            EntityBufferLength-BufferOffset,
                            &BytesRead,
                            NULL );

                switch ( result ) {
                    case NO_ERROR:
                        if ( BytesRead != 0 ) {
                            BYTE* pNewBuffer = NULL;
                            BufferOffset += BytesRead;
                            EntityBufferLength *= 2;

                            pNewBuffer = (LPBYTE) REALLOC_MEM( pEntityBuffer, EntityBufferLength+1 );
                            if ( pNewBuffer == NULL ) 
                                throw StudioException( "Unable to allocate memory for POST request buffer" );

                            pEntityBuffer = pNewBuffer;
                            TotalBytesRead += BytesRead;
                        }
                        break;

                    case ERROR_HANDLE_EOF:
                       pEntityBuffer[TotalBytesRead] = '\0';    // Add a complementary null
                       done = true;
                       break;

                    default:
                      throw StudioException( "HttpReceiveRequestEntityBody failed with %lu",  result);
                }
            }
        }
    }
    catch ( ... ) {
          FREE_MEM( pEntityBuffer );
          throw;
    }

    // Send post results to the appropriate handler
    try {
        IRequestHandler* handler = m_httpServer->getHandler( path, getRequestPort() );
        if ( handler != NULL )
            result = handler->processPostRequest( this, pEntityBuffer, TotalBytesRead );
        else
            result =  error_501( );

        FREE_MEM( pEntityBuffer );
    }
    catch ( ... ) {
        FREE_MEM( pEntityBuffer );
        throw;
    }

    return result;
}
