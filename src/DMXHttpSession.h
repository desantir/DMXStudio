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

#include "DMXStudio.h"
#include "SoundSampler.h"
#include "BeatDetector.h"

class BeatBin
{
    unsigned    m_start_freq;
    unsigned    m_end_freq;
    CEvent      m_beat;

    BeatBin& operator=( BeatBin& rhs );

public:
    BeatBin( unsigned start_freq, unsigned end_freq ) :
        m_start_freq( start_freq ),
        m_end_freq( end_freq )
    { }

    BeatBin( BeatBin& other ) {
        m_start_freq = other.m_start_freq;
        m_end_freq = other.m_end_freq;
    }

    inline unsigned getStartFreq( ) const {
        return m_start_freq;
    }

    inline unsigned getEndFreq( ) const {
        return m_end_freq;
    }

    inline CEvent* getEvent() {
        return &m_beat;
    }

    bool inline isBeat( ) {
        return ( ::WaitForSingleObject( m_beat.m_hObject, 0 ) == WAIT_OBJECT_0 );
    }
};

typedef std::vector<BeatBin> BeatBinArray;

class DMXHttpSession
{
    CString                 m_session_id;
    DWORD                   m_expires;
    EventQueue              m_events;
    CCriticalSection        m_event_lock;

    SoundSampler            m_sound_sampler;
    BeatDetector            m_beat_sampler;
    BeatBinArray            m_beats;

public:
    DMXHttpSession() :
        m_sound_sampler( 2 )
    {
        m_session_id.Format( "DMXSTUDIO%lu", GetTickCount() );
        ping();
    }

    ~DMXHttpSession() {
        m_sound_sampler.detach();
        m_beat_sampler.detach();
    }

    void ping() {
        m_expires = GetTickCount() + (1000 * 60 * 15);
    }

    bool isExpired( DWORD now ) const {
        return now > m_expires;
    }

    LPCSTR getId() const {
        return m_session_id;
    }

    bool hasPendingEvents() const {
        return m_events.size() > 0;
    }

    bool nextEvent( Event& event ) {
        CSingleLock lock( &m_event_lock, TRUE );

        if ( !hasPendingEvents() )
            return false;

        event = m_events.front();
        m_events.pop();
        return true;
    }

    void queueEvent( const Event& event ) {
        CSingleLock lock( &m_event_lock, TRUE );

        m_events.push( event );
    }

    SoundSampler& getSoundSampler() {
        return m_sound_sampler;
    }

    BeatDetector& getBeatDetector() {
        return m_beat_sampler;
    }

    BeatBinArray& getBeats() {
        return m_beats;
    }
};

typedef std::map<CString, DMXHttpSession*> SessionMap;
