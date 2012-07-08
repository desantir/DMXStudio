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

#include "scene.h"

class ChaseStep
{
    friend class VenueWriter;
    friend class VenueReader;

	UID					m_scene_uid;
	ULONG				m_delay_ms;					// Step delay in milliseconds (0 indicates use default)

public:
	ChaseStep(void) : 
	  m_scene_uid(0), 
	  m_delay_ms(0) {}

	ChaseStep( UID scene_uid, ULONG delay );
	~ChaseStep(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

	void setSceneUID( UID scene_uid ) {
		m_scene_uid = scene_uid;
	}
	UID getSceneUID() const {
		return m_scene_uid;
	}

	void setDelayMS( ULONG delay_ms ) {
		m_delay_ms = delay_ms;
	}
	ULONG getDelayMS() const {
		return m_delay_ms;
	}
};

typedef std::vector<ChaseStep> ChaseStepArray;

