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

#include "AnimationDefinition.h"

enum StrobeType {
	STROBE_SIMULATED = 1,
	STROBE_FIXTURE = 2
};

class SceneStrobeAnimator : public AnimationDefinition
{
    friend class VenueWriter;
    friend class VenueReader;

protected:
	// Configuration
	StrobeType			m_strobe_type;
	UINT				m_strobe_percent;
	RGBWA   			m_strobe_neg_color;
	StrobeTime			m_strobe_time;
	RGBWA				m_strobe_color;

public:
    static const char* className;
    static const char* animationName;

    SceneStrobeAnimator( UID animation_uid, bool shared, UID reference_fixture, 
                        AnimationSignal signal,
						StrobeType strobe_type,
						UINT strobe_percent,
						StrobeTime strobe_time,
						RGBWA strobe_color,	
                        RGBWA strobe_neg_color );

    SceneStrobeAnimator(void)
    {}

    virtual ~SceneStrobeAnimator(void);

    CString getSynopsis(void);

    const char* getPrettyName() { return SceneStrobeAnimator::animationName; }
    const char* getClassName() { return SceneStrobeAnimator::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    AnimationTask* createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid );

	AnimationDefinition* clone();

	StrobeType getStrobeType() const {
		return m_strobe_type;
	}
	void setStrobeType( StrobeType strobe_type ) {
		m_strobe_type = strobe_type;
	}

	UINT getStrobePercent() const {
		return m_strobe_percent;
	}
	void setStrobePercent( UINT strobe_percent ) {
		m_strobe_percent = strobe_percent;
	}

	StrobeTime getStrobeTime() const {
		return m_strobe_time;
	}
	void setStrobeTime( StrobeTime strobe_time ) {
		m_strobe_time = strobe_time;
	}

	RGBWA getStrobeColor() const {
		return m_strobe_color;
	}
	void setStrobeColor( RGBWA strobe_color ) {
		m_strobe_color = strobe_color;
	}

	RGBWA getStrobeNegColor() const {
		return m_strobe_neg_color;
	}
	void setStrobeNegColor( RGBWA strobe_neg_color ) {
		m_strobe_neg_color = strobe_neg_color;
	}
};

