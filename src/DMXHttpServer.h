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

#pragma once

#include "DMXStudio.h"
#include "Threadable.h"
#include "DMXHttpSession.h"
#include "DMXHttpWorker.h"

#define DMX_URL_ROOT                        "/dmxstudio/"

#include <http.h>

typedef std::vector<std::unique_ptr<HttpWorkerThread>> ThreadArray;
typedef std::vector<std::unique_ptr<IRequestHandler>> RequestHandlerPtrArray;
typedef std::map<CString, CString> MimeMap;
typedef std::map<CString, std::unique_ptr<DMXHttpSession>> SessionMap;

class DMXHttpServer : public Threadable, EventBusListener
{
    ThreadArray             m_worker_threads;
    RequestHandlerPtrArray  m_request_handlers;
    MimeMap                 m_mime_map;
    SessionMap              m_sessions;
    CCriticalSection        m_session_lock;

    UINT run(void);

    bool handleEvent( const Event& event );

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
        m_request_handlers.push_back( std::unique_ptr<IRequestHandler>( handler ) );
    }

    IRequestHandler* getHandler( LPCSTR prefix, UINT port );

    LPCSTR lookupMimeForExtension( LPCSTR extension ) {
        MimeMap::iterator it = m_mime_map.find( extension );
        if ( it == m_mime_map.end() )
            return NULL;
        return it->second;
    }

    DMXHttpSession* getSession( LPCSTR sessionId );
    DMXHttpSession* createSession();

protected:
    void createThreadPool( UINT thread_count, HANDLE hReqQueue );
    void freeWorkers(void);
};
