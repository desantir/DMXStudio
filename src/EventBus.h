/* 
Copyright (C) 2016 Robert DeSantis
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

#include "Threadable.h"
#include "Event.h"
#include "EventBusListener.h"

#define MAX_EVENT_QUEUE_SIZE    1000

typedef std::list<EventBusListener *> EventListeners;

class EventBus : public Threadable
{
    CEvent              m_event_fired;
    CCriticalSection    m_bus_lock;
    CCriticalSection    m_listener_lock;
    EventListeners      m_listeners;
    EventQueue          m_event_queue;

    UINT run();
    void dispatchEvent( const Event& event );
    EventListeners::iterator findListener( EventBusListener* listener );

public:
    EventBus();
    ~EventBus();

    bool fireEvent( EventSource source, DWORD uid, EventAction action, LPCSTR text=NULL, DWORD val1=0L, DWORD val2=0L, DWORD val3=0L, DWORD val4=0L );
    void addListener( EventBusListener* listener );
    void addListenerFirst( EventBusListener* listener );
    bool removeListener( EventBusListener* listener ); 
    void clearAll();
    
    static CString eventAsString( const Event& event );
};

