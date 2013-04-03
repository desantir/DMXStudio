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

#include "DMXStudio.h"
#include "Threadable.h"

#define DMX_URL_ROOT                        "/dmxstudio/"

#include <http.h>

extern CString encodeHtmlString( LPCSTR string );
extern CString encodeJsonString( LPCSTR string );

#define ALLOC_MEM(cb) HeapAlloc(GetProcessHeap(), 0, (cb))
#define FREE_MEM(ptr) HeapFree(GetProcessHeap(), 0, (ptr))
#define REALLOC_MEM(mem,cb) HeapReAlloc(GetProcessHeap(), 0, mem, (cb))

#define INITIALIZE_HTTP_RESPONSE( resp, status, reason )    \
    do                                                      \
    {                                                       \
        RtlZeroMemory( (resp), sizeof(*(resp)) );           \
        (resp)->StatusCode = (status);                      \
        (resp)->pReason = (reason);                         \
        (resp)->ReasonLength = (USHORT) strlen(reason);     \
    } while (FALSE)

#define ADD_KNOWN_HEADER(Response, HeaderId, RawValue)               \
    do                                                               \
    {                                                                \
        (Response).Headers.KnownHeaders[(HeaderId)].pRawValue =      \
                                                          (RawValue);\
        (Response).Headers.KnownHeaders[(HeaderId)].RawValueLength = \
            (USHORT) strlen(RawValue);                               \
    } while(FALSE)

class HttpWorkerThread;

class IRequestHandler
{
public:
    virtual ~IRequestHandler() {}
    
    virtual LPCSTR getPrefix() = 0;
    virtual UINT getPort() = 0;
    virtual DWORD processGetRequest( HttpWorkerThread* worker ) = 0;
    virtual DWORD processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) = 0;
    virtual bool substitute( LPCSTR marker, LPCSTR data, CString& marker_content ) = 0;
};

typedef std::vector<HttpWorkerThread *> ThreadArray;
typedef std::vector<IRequestHandler *> RequestHandlerPtrArray;
typedef std::map<CString, CString> MimeMap;

class DMXHttpServer : public Threadable
{
    ThreadArray             m_worker_threads;
    RequestHandlerPtrArray  m_request_handlers;
    MimeMap                 m_mime_map;

    UINT run(void);

public:
    DMXHttpServer(void);
    ~DMXHttpServer(void);

    void start( void ) {
        startThread();
    }

    void stop( void ) {
        freeWorkers();
        stopThread();
    }

    void registerHandler( IRequestHandler* handler ) {
        m_request_handlers.push_back( handler );
    }

    IRequestHandler* getHandler( LPCSTR prefix, UINT port );

    LPCSTR lookupMimeForExtension( LPCSTR extension ) {
        MimeMap::iterator it = m_mime_map.find( extension );
        if ( it == m_mime_map.end() )
            return NULL;
        return it->second;
    }

protected:
    void createThreadPool( UINT thread_count, HANDLE hReqQueue );
    void freeWorkers(void);
};

class HttpWorkerThread : public Threadable
{
    UINT                m_worker_id;
    HANDLE              m_hReqQueue;
    PHTTP_REQUEST       m_pRequest;
    ULONG               m_RequestBufferLength;
    DMXHttpServer*      m_httpServer;
    CString             m_docroot;

    UINT run(void);

public:
    HttpWorkerThread( DMXHttpServer* httpServer, UINT worker_id, HANDLE hReqQueue, LPCSTR docroot );
    ~HttpWorkerThread(void);

    void start( void ) {
        startThread();
    }

    void stop( void ) {
        stopThread();
    }

    DWORD sendFile( LPCSTR file_name, IRequestHandler* handler=NULL );
    DWORD sendRedirect( LPCSTR location );
    DWORD sendAttachment( LPBYTE contents, DWORD size,  LPCSTR mime, LPCSTR attachment_name );

    PHTTP_REQUEST getRequest() const {
        return m_pRequest;
    }

    inline PCSTR getContentType() const {
        return m_pRequest->Headers.KnownHeaders[ HttpHeaderContentType ].pRawValue;
    }

    DWORD error_404( void ) {
        return sendResponse( 404, "NOT FOUND", "404 - Page not found!" );
    }

    DWORD error_500( void ) {
        return sendResponse( 500, "INTERNAL SERVER ERROR", "500 - Server error" );
    }

    DWORD error_501( void ) {
        return sendResponse( 501, "Not Implemented", NULL );
    }

    DWORD sendResponse( 
        IN USHORT        StatusCode,
        IN LPCSTR        pReason,
        IN LPCSTR        pEntityString );

    DWORD sendResponse( 
        IN USHORT        StatusCode,
        IN LPCSTR        pReason,
        IN LPBYTE        pEntity,
        IN ULONG         entityLength,
        IN BOOL          bNoCache,
        IN LPCSTR        mime,
        IN LPCSTR        disposition = NULL );

protected:
    void resizeRequestBuffer( DWORD new_size );

    DWORD processGetRequest( void );
    DWORD processPostRequest( void );

    UINT getRequestPort( ) const {
        UINT port = 80;
        PCWSTR colon = wcschr( m_pRequest->CookedUrl.pHost, ':' );
        if ( colon && colon < m_pRequest->CookedUrl.pAbsPath )
            port = _wtoi( &colon[1] ); 
        return port;
    }
};
