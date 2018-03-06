/* 
Copyright (C) 2011-2017 Robert DeSantis
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

#include "SceneStrobeAnimatorTask.h"

// ----------------------------------------------------------------------------
//
SceneStrobeAnimatorTask::SceneStrobeAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
	AnimationTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
SceneStrobeAnimatorTask::~SceneStrobeAnimatorTask(void)
{
}

// ----------------------------------------------------------------------------
//
void SceneStrobeAnimatorTask::setupAnimation( AnimationDefinition* definition, DWORD time_ms ) {

    SceneStrobeAnimator* config = dynamic_cast<SceneStrobeAnimator *>( definition );
    
    m_strobe_type = config->getStrobeType();
    m_strobe.setNegative( config->getStrobeNegColor() );
    m_strobe.setPositive( config->getStrobeColor() );
    m_strobe_time = config->getStrobeTime();

    if ( m_strobe_type == STROBE_FIXTURE ) {
        UINT strobe_percent = config->getStrobePercent();

        for ( SceneActor& actor : getActors() ) {
            for ( Fixture* pf : resolveActorFixtures( &actor ) ) {
                // Determine which channels will be participating
                for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                    Channel* cp = pf->getChannel( channel );

                    if ( cp->isDimmer() ) {
                        int fast = cp->getStrobeFast();
                        int slow = cp->getStrobeSlow();
                        int range = fast-slow;

                        loadChannel( pf, cp->getOffset(), slow + (range * strobe_percent) / 100 );
                    }
                }
            }
        }
    }
    else {
        setupFixtureFaders( m_fixtures );
    }

    restartAnimation( time_ms );
}

// ----------------------------------------------------------------------------
//
bool SceneStrobeAnimatorTask::sliceAnimation( DWORD time_ms )
{
    bool changed = false;

    if ( m_strobe_type == STROBE_SIMULATED ) {
        switch ( m_strobe.strobe( time_ms ) ) {
            case StrobeState::SE_OFF:
            case StrobeState::SE_ON:
            case StrobeState::SE_FADE_IN_START:
            case StrobeState::SE_FADE_OUT_START:
            case StrobeState::SE_FADE_OUT:
            case StrobeState::SE_FADE_IN:
                RGBWA color = m_strobe.rgbwa();
                for ( FaderFixture& f : m_fixtures )
                    loadFaderColorChannels( f, color );
                changed = true;
                break;
        }
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
bool SceneStrobeAnimatorTask::restartAnimation( DWORD time_ms ) {
    if ( m_strobe_type == STROBE_SIMULATED ) {
        m_strobe.start( time_ms, 
            StrobeTime( scaleAnimationSpeed( m_strobe_time.getOnMS() ),
					    scaleAnimationSpeed( m_strobe_time.getOffMS() ),
					    scaleAnimationSpeed( m_strobe_time.getFadeInMS() ),
					    scaleAnimationSpeed( m_strobe_time.getFadeOutMS() ),
					    m_strobe_time.getFlashes() ), m_strobe_time.getFadeInMS() != 0 );
    }

    return false;
}

// ----------------------------------------------------------------------------
//
void SceneStrobeAnimatorTask::stopAnimation()
{
}
