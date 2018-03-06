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

#include "SceneColorFader.h"
#include "AnimationEngine.h"
#include "ScenePulseTask.h"

// ----------------------------------------------------------------------------
//
ScenePulseTask::ScenePulseTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    AnimationTask( engine, animation_uid, actors, owner_uid ),
    m_fixture_index( 0 ),
    m_color_index( 0 )
{
}

// ----------------------------------------------------------------------------
//
ScenePulseTask::~ScenePulseTask(void)
{
}

// ----------------------------------------------------------------------------
//
void ScenePulseTask::setupAnimation( AnimationDefinition* definition, DWORD time_ms )
{
    ScenePulse* config = dynamic_cast<ScenePulse *>( definition );

    RGBWA::resolveColor( config->getPulseColor(), m_pulse_colors );

    m_pulse_ms = config->getPulseMS();
    m_select_random = config->isSelectRandom();
    m_pulse_fixture_count = config->getPulseFixtureCount(); 
    m_pulse_effect = config->getPulseEffect();

    // For each fixture, find red, green, blue, white channels
	for ( SceneActor& actor : getActors() ) {
        for ( Fixture* pf  : resolveActorFixtures( &actor ) ) {
            if ( pf->getNumPixels() == 0 )
                continue;

            // Setup state so that the slice animation does not need to gather additional info

            RGBWAArray initial_colors;
            PixelArray* pixels = pf->getPixels();

            for ( Pixel& pixel : *pf->getPixels() ) {
                RGBWA color;
                if ( pixel.hasRed() )
                    color.red( actor.getFinalChannelValue( pf->getUID(), pixel.red() ) );
                if ( pixel.hasGreen() )
                    color.green( actor.getFinalChannelValue( pf->getUID(), pixel.green() ) );
                if ( pixel.hasBlue() )
                    color.blue( actor.getFinalChannelValue( pf->getUID(), pixel.blue() ) );
                if ( pixel.hasWhite() )
                    color.white( actor.getFinalChannelValue( pf->getUID(), pixel.white() ) );
                if ( pixel.hasAmber() )
                    color.amber( actor.getFinalChannelValue( pf->getUID(), pixel.amber() ) );

                initial_colors.push_back( color );
            }

            m_fixtures.emplace_back( pf, pixels, initial_colors );
        }
    }

    m_pulse_fixture_count = std::min<unsigned>( m_pulse_fixture_count, m_fixtures.size() );
}

// ----------------------------------------------------------------------------
//
bool ScenePulseTask::sliceAnimation( DWORD time_ms )
{
    bool changed = false;

    for ( PulseFixture& pulse_fixture : m_fixtures ) {
        if ( pulse_fixture.m_pulse_end_ms == 0 || pulse_fixture.m_pulse_end_ms > time_ms  )
            continue;
                
        // Unpulse the fixture
        size_t index = 0;
        for ( Pixel& pixel : *pulse_fixture.m_pixels ) {
            loadColorChannels( pulse_fixture.m_fixture, pixel, pulse_fixture.m_colors[index] );
            index++;
        }

        pulse_fixture.m_pulse_end_ms = 0;   
		pulse_fixture.m_pulsed = true;
        changed = true;    
    }

    SignalState state = tick( time_ms );
    if ( state != SIGNAL_NEW_LEVEL )
        return changed;

    for ( size_t count=0; count < m_pulse_fixture_count; count++ ) {
        if ( m_select_random ) {
			m_fixture_index = rand() % m_fixtures.size();

            for ( int tries=m_fixtures.size(); tries--; ) {
                if ( !m_fixtures[m_fixture_index].m_pulse_end_ms && !m_fixtures[m_fixture_index].m_pulsed )
                    break;

				m_fixture_index = (m_fixture_index+1) % m_fixtures.size();
            }
        }

        if ( !m_fixtures[m_fixture_index].m_pulse_end_ms ) {
            PulseFixture& pulse_fixture = m_fixtures[m_fixture_index];

            switch ( m_pulse_effect ) {
                case PULSE_EFFECT_STROBE:
                    for ( Pixel& pixel : *pulse_fixture.m_pixels )
                        loadColorChannels( pulse_fixture.m_fixture, pixel, m_pulse_colors[m_color_index] );

                    break;

                case PULSE_EFFECT_BREATH:
                    break;
            }
            
            pulse_fixture.m_pulse_end_ms =  time_ms + scaleAnimationSpeed( m_pulse_ms );   
            m_color_index = (m_color_index+1) % m_pulse_colors.size();

            changed = true; 
        }

        m_fixture_index = (m_fixture_index+1) % m_fixtures.size();              
    }

	for ( PulseFixture& pulse_fixture : m_fixtures ) 
		pulse_fixture.m_pulsed = false;

    return changed;
}

// ----------------------------------------------------------------------------
//
void ScenePulseTask::loadColorChannels( Fixture* pf, Pixel& pixel, RGBWA& rgbwa )
{
    if ( pixel.hasRed() )
        loadChannel( pf, pixel.red(), rgbwa.red() );
    if ( pixel.hasGreen() )
        loadChannel( pf, pixel.green(), rgbwa.green() );
    if ( pixel.hasBlue() )
        loadChannel( pf, pixel.blue(), rgbwa.blue() );
    if ( pixel.hasWhite() )
        loadChannel( pf, pixel.white(), rgbwa.white() );
    if ( pixel.hasAmber() )
        loadChannel( pf, pixel.amber(), rgbwa.amber() );
}

// ----------------------------------------------------------------------------
//
bool ScenePulseTask::restartAnimation( DWORD time_ms ) { 
    bool changed = false;

    for ( PulseFixture& pulse_fixture : m_fixtures ) {
        if ( pulse_fixture.m_pulse_end_ms != 0 ) {
            // Unpulse the fixture
            size_t index = 0;
            for ( Pixel& pixel : *pulse_fixture.m_pixels ) {
                loadColorChannels( pulse_fixture.m_fixture, pixel, pulse_fixture.m_colors[index] );
                index++;
            }

            pulse_fixture.m_pulse_end_ms = 0;                
            changed = true;  
        }
    }

    return changed;
}


// ----------------------------------------------------------------------------
//
void ScenePulseTask::stopAnimation()
{
}