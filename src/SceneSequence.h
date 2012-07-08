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

#include "AnimationTask.h"
#include "AbstractAnimation.h"

class SceneSequence : public AbstractAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

	unsigned			m_current_actor;
	DWORD				m_next_actor_ms;
	UIDArray			m_running_actors;

	SceneSequence(SceneSequence& other) {}
	SceneSequence& operator=(SceneSequence& rhs) { return *this; }

public:
	static const char* className;

	SceneSequence( void ) {};
	SceneSequence( UID animation_uid, AnimationSignal signal, UIDArray actors );
	virtual ~SceneSequence(void);

	AbstractAnimation* clone();

	const char* getName() { return "Fixture Sequencer"; }
	const char* getClassName() { return SceneSequence::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

	void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );
	bool sliceAnimation( DWORD time_ms, BYTE* dmx_packet );
	void stopAnimation( void );

private:
	void unselectActor( unsigned actor, BYTE* dmx_packet );
	void selectActor( unsigned actor, BYTE* dmx_packet );
};
