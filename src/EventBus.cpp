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

#include "DMXStudio.h"

static std::map<EventAction,CString> actions;
static std::map<EventSource,CString> sources;

static int populateEventNames() {
    actions[ EA_START ] = "START";
    actions[ EA_STOP ] = "STOP";
    actions[ EA_DELETED ] = "DELETED";
    actions[ EA_PAUSE ] = "PAUSE";
    actions[ EA_NEW ] = "NEW";
    actions[ EA_CHANGED ] = "CHANGED";
    actions[ EA_ERROR ] = "ERROR";
    actions[ EA_RESUME ] = "RESUME";
    actions[ EA_TIME ] = "TIME";

    sources[ ES_STUDIO ] = "STUDIO";
    sources[ ES_VENUE ] = "VENUE";
    sources[ ES_SCENE ] = "SCENE";
    sources[ ES_CHASE ] = "CHASE";
    sources[ ES_FIXTURE ] = "FIXTURE";
    sources[ ES_FIXTURE_GROUP ] = "FIXTURE_GROUP";
    sources[ ES_CHANNEL ] = "CHANNEL";
    sources[ ES_WHITEOUT ] = "WHITEOUT";
    sources[ ES_WHITEOUT_STROBE ] = "WHITEOUT_STROBE";
    sources[ ES_WHITEOUT_COLOR ] = "WHITEOUT_COLOR";
    sources[ ES_BLACKOUT ] = "BLACKOUT";
    sources[ ES_MUTE_BLACKOUT ] = "MUTE_BLACKOUT";
    sources[ ES_TRACK ] = "TRACK";
    sources[ ES_PLAYLIST ] = "PLAYLIST";
    sources[ ES_VOLUME ] = "VOLUME";
    sources[ ES_VOLUME_MUTE ] = "VOLUME_MUTE";
    sources[ ES_MUSIC_MATCH ] = "MUSIC_MATCH";
    sources[ ES_ANIMATION_SPEED ] = "ANIMATION_SPEED";
    sources[ ES_MUSIC_PLAYER ] = "MUSIC_PLAYER";
    sources[ ES_MASTER_DIMMER ] = "MASTER_DIMMER";
    sources[ ES_TRACK_QUEUES ] = "TRACK_QUEUES";

    return 0;
}

static int static_kludge = populateEventNames();

// ----------------------------------------------------------------------------
//
EventBus::EventBus() :
    Threadable( "EventBus" )
{
}

// ----------------------------------------------------------------------------
//
EventBus::~EventBus() {
}

// ----------------------------------------------------------------------------
//
UINT EventBus::run() {
    DMXStudio::log_status( "Studio event bus started" );

    CSingleLock lock( &m_bus_lock, FALSE );

    while ( isRunning() ) {
        if ( ::WaitForSingleObject( m_event_fired, 10000 ) != WAIT_OBJECT_0 )
            continue;

        while ( m_event_queue.size() > 0 ) {
            lock.Lock();
            Event event = m_event_queue.front();
            m_event_queue.pop();
            lock.Unlock();

            dispatchEvent( event );
        }
    }

    DMXStudio::log_status( "Studio event bus stopped" );

    return 0;
}

// ----------------------------------------------------------------------------
//
void EventBus::clearAll() {
    CSingleLock lock( &m_bus_lock, TRUE );

    m_event_queue.empty();
}

// ----------------------------------------------------------------------------
//
void EventBus::dispatchEvent( const Event& event ) {
    CSingleLock lock( &m_listener_lock, TRUE );

    for ( auto listener : m_listeners ) {
        if ( listener->handleEvent( event ) )
            break;
    }
}

// ----------------------------------------------------------------------------
//
void EventBus::addListener( EventBusListener* listener ) {
    CSingleLock lock( &m_listener_lock, TRUE );

    STUDIO_ASSERT( (findListener( listener ) == m_listeners.end()), "Listener added multiple times" );

    m_listeners.push_back( listener );
}

// ----------------------------------------------------------------------------
//
void EventBus::addListenerFirst( EventBusListener* listener ) {
    CSingleLock lock( &m_listener_lock, TRUE );

    STUDIO_ASSERT( (findListener( listener ) == m_listeners.end()), "Listener added multiple times" );

    m_listeners.push_front( listener );
}

// ----------------------------------------------------------------------------
//
bool EventBus::removeListener( EventBusListener* listener ) {
    CSingleLock lock( &m_listener_lock, TRUE );

    auto it = findListener( listener );
    if ( it == m_listeners.end() )
        return false;
        
    m_listeners.erase( it );
    return true;
}

// ----------------------------------------------------------------------------
//
EventListeners::iterator EventBus::findListener( EventBusListener* listener ) {
    auto it=m_listeners.begin();

    while ( it != m_listeners.end() ) {
        if ( (*it) == listener )
            break;
        it++;
    }

    return it;
}

// ----------------------------------------------------------------------------
//
bool EventBus::fireEvent( EventSource source, DWORD uid, EventAction action, LPCSTR text, DWORD val1, DWORD val2, DWORD val3, DWORD val4 ) {
    CSingleLock lock( &m_bus_lock, TRUE );

    STUDIO_ASSERT( m_event_queue.size() < MAX_EVENT_QUEUE_SIZE, "Event bus queue size exceeds capacity" );

    m_event_queue.push( Event( source, uid, action, text, val1, val2, val3, val4 ) );

    lock.Unlock();

    m_event_fired.SetEvent();

    return true;
}

// ----------------------------------------------------------------------------
//
CString EventBus::eventAsString( const Event& event ) {
    CString& actionName = actions[ event.m_action ];
    CString& sourceName = sources[ event.m_source ];

    if ( actionName.GetLength() == 0 )
        actionName = "UNKNOWN";
    if ( sourceName.GetLength() == 0 )
        sourceName = "UNKNOWN";

    CString text;
    text.Format( "%s %s UID %ld [%lu, %lu, %lu, %lu] %s", 
        actionName, sourceName, event.m_uid, event.m_val1, event.m_val2, event.m_val3, event.m_val4, (LPCSTR)event.m_text );

    return text;
}