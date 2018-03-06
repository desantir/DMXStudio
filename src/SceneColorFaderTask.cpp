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

#include "SceneColorFader.h"
#include "AnimationEngine.h"
#include "SceneColorFaderTask.h"

// ----------------------------------------------------------------------------
//
SceneColorFaderTask::SceneColorFaderTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    AnimationTask( engine, animation_uid, actors, owner_uid ),
    m_effect_periods( 0 )
{
    m_fixtures.reserve( 25 );
}

// ----------------------------------------------------------------------------
//
SceneColorFaderTask::~SceneColorFaderTask(void)
{
}

// ----------------------------------------------------------------------------
//
void SceneColorFaderTask::setupAnimation( AnimationDefinition* definition, DWORD time_ms )
{
    SceneColorFader* config = dynamic_cast<SceneColorFader *>( definition );

    RGBWAArray&customColors = config->getCustomColors();

    // Setup the color progression for the animation
    if ( customColors.size() > 1 )
        m_colors.assign( customColors.begin(), customColors.end() );
    else if ( customColors.size() == 1 )
        RGBWA::resolveColor( customColors[0], m_colors );
    else
        RGBWA::getAllPredefinedColors( m_colors );

    m_current_effect = config->getFaderEffect();
    m_color_index = 0;
    m_effect_periods = 0;
    m_strobe.setNegative( config->getStrobeNegColor() );
	m_restart_strobe = true;
    m_fader_effect = config->getFaderEffect();
	m_strobe_time = config->getStrobeTime();

	m_strobe_time.setFadeInMS( std::min<unsigned>( m_strobe_time.getFadeInMS(), m_strobe_time.getOnMS() ) );
    m_strobe_time.setFadeOutMS( std::min<unsigned>( m_strobe_time.getFadeOutMS(), m_strobe_time.getOffMS() ) );

    if ( !m_colors.empty() )
        setupFixtureFaders( m_fixtures );
}

// ----------------------------------------------------------------------------
//
bool SceneColorFaderTask::sliceAnimation( DWORD time_ms )
{
    if ( m_fixtures.size() == 0 )
        return false;

    SignalState state = tick( time_ms );

    if ( state == SIGNAL_LEVEL_OFF ) {
        for ( FaderFixture& sfixture : m_fixtures ) {
			loadFaderColorChannels( sfixture, sfixture.m_color );
			sfixture.m_fader.setCurrent( sfixture.m_color );
		}

        return true;
    }

	bool changed = false;

    if ( state == SIGNAL_NEW_LEVEL ) {
        unsigned level = getLevel();

        // Choose next color
        RGBWA rgbwa = nextColor();

        if ( m_fader_effect == FADER_EFFECT_ALL ) {
            if ( m_effect_periods-- < 1 ) {                 // Choose new effect
                int roll = rand() % 100;

                if ( level < PERCENT_OF_LEVEL(10) )			// No or little sound then just blend
                    m_current_effect = FADER_EFFECT_BLEND;
                else if ( level > PERCENT_OF_LEVEL(90) && getSignal().getTrigger() != TRIGGER_TIMER ) { // High sound energy change & strober
                    m_current_effect = (roll > 75) ? FADER_EFFECT_STROBE : FADER_EFFECT_CHANGE;
                }
                else {
                    if ( roll > 95 )
                        m_current_effect = FADER_EFFECT_STROBE;
                    else if ( roll > 60 )
                        m_current_effect = FADER_EFFECT_CHANGE;
                    else
                        m_current_effect = FADER_EFFECT_BLEND;
                }

                m_effect_periods = rand() % 20;
            }
        }
        else
            m_current_effect = m_fader_effect;

        if ( m_current_effect == FADER_EFFECT_STROBE ) {
            m_strobe.setPositive( rgbwa );

            if ( m_restart_strobe ) {
                UINT light_ms = std::max<UINT>( scaleAnimationSpeed( m_strobe_time.getOnMS() * (MAXIMUM_LEVEL-level) / MAXIMUM_LEVEL ), 25 );
                UINT dark_ms = std::max<UINT>( scaleAnimationSpeed( m_strobe_time.getOffMS() * (MAXIMUM_LEVEL-level) / MAXIMUM_LEVEL ), 25 );

                m_strobe.start( time_ms, StrobeTime( light_ms, dark_ms, 
                                scaleAnimationSpeed( m_strobe_time.getFadeInMS() ), 
                                scaleAnimationSpeed( m_strobe_time.getFadeOutMS() ), m_strobe_time.getFlashes() ), true );
				m_restart_strobe = false;
            }
        }
        else if ( m_current_effect == FADER_EFFECT_CHANGE || m_current_effect == FADER_EFFECT_CHANGE_MULTI ) {
			bool first = true;
			UINT index = m_color_index;

            for ( FaderFixture& sfixture : m_fixtures ) {
				if ( m_current_effect == FADER_EFFECT_CHANGE_MULTI && !first ) {
					index = (index + 1) % m_colors.size();
					rgbwa = m_colors[index];
				}
				else
					first = false;

				loadFaderColorChannels( sfixture, rgbwa );
				sfixture.m_fader.setCurrent( rgbwa );
            }

			changed = true;
			m_restart_strobe = true;
        }
        else if ( m_current_effect == FADER_EFFECT_BLEND || m_current_effect == FADER_EFFECT_BLEND_MULTI ) {
            DWORD fade_time = (DWORD)(getNextSampleMS()-time_ms);
            if ( fade_time > getSignal().getTimerMS() / 20 )   // Keep at target color a bit longer
				fade_time -= (DWORD)( getSignal().getTimerMS() / 20 );
			fade_time = scaleAnimationSpeed( fade_time );

			bool first = true;
			UINT index = m_color_index;

            for ( FaderFixture& sfixture : m_fixtures ) {
				if ( m_current_effect == FADER_EFFECT_BLEND_MULTI && !first ) {
					index = (index + 1) % m_colors.size();
					rgbwa = m_colors[index];
				}
				else
					first = false;

                if ( sfixture.m_pf->canDimmerStrobe() ) {
                    sfixture.m_fader.start( time_ms, fade_time, rgbwa );
                }
                else {
                    loadFaderColorChannels( sfixture, rgbwa );
                    sfixture.m_fader.setCurrent( rgbwa );
                }
			}

			m_restart_strobe = true;
        }
    }

	// Deal with software strobing and fading

    switch ( m_current_effect ) {
        case FADER_EFFECT_STROBE:
            if ( m_strobe.check_strobe( time_ms ) ) {
                for ( FaderFixture& sfixture : m_fixtures ) {
                    RGBWA& color = sfixture.m_pf->canDimmerStrobe() ? m_strobe.rgbwa() : m_strobe.getNegative();
					loadFaderColorChannels( sfixture, color );
					sfixture.m_fader.setCurrent( color );
				}
                changed = true;
            }
            break;

        case FADER_EFFECT_BLEND:
		case FADER_EFFECT_BLEND_MULTI:
            for ( FaderFixture& sfixture : m_fixtures ) {
                if ( sfixture.m_pf->canDimmerStrobe() && sfixture.m_fader.fade( time_ms ) ) {
					loadFaderColorChannels( sfixture, sfixture.m_fader.rgbwa() );
                    changed = true;
                }
            }
            break;
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
bool SceneColorFaderTask::restartAnimation( DWORD time_ms ) {
	m_restart_strobe = true;
    return false;
}

// ----------------------------------------------------------------------------
//
void SceneColorFaderTask::stopAnimation()
{
}

