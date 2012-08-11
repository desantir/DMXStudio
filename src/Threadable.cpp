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


#include "DMXStudio.h"
#include "Threadable.h"

// ----------------------------------------------------------------------------
//
Threadable::Threadable( LPCSTR name ) :
    m_running( false ),
    m_thread( NULL )
{
    m_name.Format( "DMXStudio!%s", name ? name : "Threadable" );
}

// ----------------------------------------------------------------------------
//
Threadable::~Threadable(void)
{
    if ( m_running )
        stopThread();
}

// ----------------------------------------------------------------------------
//
bool Threadable::startThread( ) {
    if ( m_running )
        return false;

    // Start thread
    m_thread = AfxBeginThread( _run, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED );
    if ( !m_thread ) {
        DMXStudio::log( "Thread failed to start" );
        m_running = false;
        return false;
    }

    m_thread->m_bAutoDelete = false;
    m_thread->ResumeThread();

    return true;
}

// ----------------------------------------------------------------------------
//
bool Threadable::stopThread() {
    if ( !m_running || !m_thread )
        return true;

    DWORD exit_code = 0;
    if ( GetExitCodeThread( m_thread->m_hThread, &exit_code ) && exit_code != STILL_ACTIVE ) {
        m_running = false;
        DMXStudio::log( "Thread premature exit (code %lx)", exit_code );
    }
    else {
        // Stop the thread
        m_running = false;

        // Wait for thread to stop
        DWORD status = ::WaitForSingleObject( m_thread->m_hThread, 5000 );

        if ( status == WAIT_FAILED )
            DMXStudio::log( "Thread failed to stop" );
        else
            delete m_thread;
    }

    m_thread = NULL;

    return true;
}

// ----------------------------------------------------------------------------
// Main thread loop
//
UINT __cdecl _run( LPVOID object )
{
    try {
        Threadable* t = reinterpret_cast<Threadable *>(object);
        t->setThreadName();
        t->m_running = true;
        return t->run();
    }
    catch ( std::exception& ex ) {
        DMXStudio::log( ex );
        return -1;
    }
}

// ----------------------------------------------------------------------------
// From http://msdn.microsoft.com/en-us/library/xcb2z8hs(v=vs.110).aspx
//

const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void Threadable::setThreadName()
{
    if ( m_name.GetLength() > 0 ) {
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = (LPCSTR)m_name;
        info.dwThreadID = -1;                // dwThreadID;
        info.dwFlags = 0;

        __try {
            RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
        }
    }
}