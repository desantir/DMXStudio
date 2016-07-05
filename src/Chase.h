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

#include "IVisitor.h"
#include "ChaseStep.h"
#include "Act.h"

#define DEFAULT_CHASE_DELAY		5000

typedef ULONG ChaseNumber;

class Chase : public DObject
{
    friend class VenueWriter;
    friend class VenueReader;

    ULONG			m_delay_ms;						// Scene delay in milliseconds
    ULONG			m_fade_ms;						// Fade time - transition between scenes
    ChaseStepArray	m_chase_steps;
    Acts            m_acts;                         // List of acts this object belongs to
    bool            m_repeat;                       // Chase will repeat indefinately

public:
    Chase(void) : 
        m_delay_ms( DEFAULT_CHASE_DELAY ),
        m_fade_ms( 0 ),
        m_repeat( true ),
        DObject()
    {}

    Chase( UID uid, ChaseNumber chase_number, ULONG delay_ms, ULONG fade_ms, const char * name, const char *description, bool repeat );
    ~Chase(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    ChaseNumber getChaseNumber( ) const {
        return getNumber();
    }
    void setChaseNumber( ChaseNumber chase_number ) {
        setNumber( chase_number );
    }

    void setDelayMS( ULONG delay_ms ) {
        m_delay_ms = delay_ms;
    }
    ULONG getDelayMS() const {
        return m_delay_ms;
    }

    void setRepeat( bool repeat ) {
        m_repeat = repeat;
    }
    bool isRepeat() const {
        return m_repeat;
    }
    
    void setFadeMS( ULONG fade_ms ) {
        m_fade_ms = fade_ms;
    }
    ULONG getFadeMS() const {
        return m_fade_ms;
    }

    bool removeScene( UID scene_uid );

    size_t getNumSteps() const {
        return m_chase_steps.size();
    }

    void appendStep( ChaseStep& step ) {
        m_chase_steps.push_back( step );
    }

    void appendStep( ChaseStepArray& steps ) {
        m_chase_steps.insert<ChaseStepArray::iterator>(
            m_chase_steps.end(),
            steps.begin(), steps.end() );
    }

    void deleteStep( unsigned step_number ) {
        STUDIO_ASSERT( step_number < m_chase_steps.size(), "Chase delete step out of range" );
        m_chase_steps.erase( m_chase_steps.begin()+step_number );
    }

    void insertStep( unsigned step_number, ChaseStep& step ) {
        STUDIO_ASSERT( step_number <= m_chase_steps.size(), "Chase insert step out of range" );
        m_chase_steps.insert( m_chase_steps.begin()+step_number, step );
    }

    void insertStep( unsigned step_number, ChaseStepArray& steps ) {
        STUDIO_ASSERT( step_number <= m_chase_steps.size(), "Chase insert step out of range" );
        m_chase_steps.insert<ChaseStepArray::iterator>(
            m_chase_steps.begin()+step_number, 
            steps.begin(), steps.end() );
    }

    ChaseStep* getStep( unsigned step_number ) {
        STUDIO_ASSERT( step_number <= m_chase_steps.size(), "Chase insert step out of range" );
        return &m_chase_steps.at( step_number );
    }

    ChaseStepArray getSteps() const {
        return m_chase_steps;
    }
    void setSteps( ChaseStepArray& steps ) {
        m_chase_steps = steps;
    }

    inline Acts getActs() const {
        return m_acts;
    }
    inline void setActs( Acts& acts ) {
        m_acts = acts;
    }
};

typedef std::map<UID, Chase> ChaseMap;
typedef std::vector<Chase *> ChasePtrArray;

