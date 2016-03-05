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


#include "SceneColorFader.h"
#include "AnimationTask.h"

const char* SceneColorFader::className = "SceneColorSwitcher";
const char* SceneColorFader::animationName = "Color fader";

// ----------------------------------------------------------------------------
//
SceneColorFader::SceneColorFader( UID animation_uid, 
                                        AnimationSignal signal,
                                        UIDArray actors,
                                        RGBWA strobe_neg_color,
                                        unsigned strobe_pos_ms,
                                        unsigned strobe_neg_ms,
                                        UINT strobe_flashes,
                                        RGBWAArray custom_colors,
                                        FaderEffect fader_effect ) :
    AbstractAnimation( animation_uid, signal ),
    m_strobe_neg_color( strobe_neg_color ),
    m_strobe_pos_ms( strobe_pos_ms ),
    m_strobe_neg_ms( strobe_neg_ms ),
    m_strobe_flashes( strobe_flashes ),
    m_signal_processor( NULL ),
    m_custom_colors( custom_colors ),
    m_effect_periods( 0 ),
    m_fader_effect( fader_effect )
{
    m_actors = actors;

    // Setup the color progression for the animation
    if ( m_custom_colors.size() == 1 && m_custom_colors[0] == RGBWA(1,1,1) )
        RGBWA::getRainbowColors( m_colors );
    else if ( m_custom_colors.size() )
        m_colors.assign( m_custom_colors.begin(), m_custom_colors.end() );
    else
        RGBWA::getAllPredefinedColors( m_colors );
}

// ----------------------------------------------------------------------------
//
SceneColorFader::~SceneColorFader(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneColorFader::clone() {
    return new SceneColorFader( m_uid, m_signal, m_actors, m_strobe_neg_color, 
                                   m_strobe_pos_ms, m_strobe_neg_ms, m_strobe_flashes, m_custom_colors, m_fader_effect );
}

// ----------------------------------------------------------------------------
//
CString SceneColorFader::getSynopsis(void) {
    CString synopsis;

    switch ( m_fader_effect ) {
        case FADER_EFFECT_CHANGE:
            synopsis.Format( "Fade( Color Change )\n" );
            break; 

        case FADER_EFFECT_STROBE:
            synopsis.Format( "Fade( Strobe )\n" );
            break; 

        case FADER_EFFECT_BLEND:
            synopsis.Format( "Fade( Color Blend )\n" );
            break; 

        case FADER_EFFECT_ALL:
            synopsis.Format( "Fade( All )\n" );
            break; 
    }

    if ( m_custom_colors.size() ) {
        synopsis.AppendFormat( "Custom Colors( " );
        for ( RGBWAArray::iterator it=m_custom_colors.begin(); it != m_custom_colors.end(); ++it )
            synopsis.AppendFormat( "%s ", (*it).getColorName() );
        synopsis.AppendFormat( ")\n" );
    }
    
    synopsis.AppendFormat( "Strobe( -color=%s +ms=%u -ms=%u )\n%s", m_strobe_neg_color.getColorName(),
        m_strobe_pos_ms, m_strobe_neg_ms, 
        AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void SceneColorFader::stopAnimation( )
{
    if ( m_signal_processor != NULL ) {
        delete m_signal_processor;
        m_signal_processor = NULL;
    }
}

// ----------------------------------------------------------------------------
//
void SceneColorFader::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_current_effect = m_fader_effect;
    m_color_index = 0;
    m_effect_periods = 0;
    m_strobe.setNegative( m_strobe_neg_color );
    m_start_strobe = true;

    // For each fixture, find red, green, blue, white channels
    for ( UID actor_uid : populateActors() ) {
        SceneActor* actor = m_animation_task->getScene()->getActor( actor_uid );
        STUDIO_ASSERT( actor != NULL, "Missing scene actor for fixture %lu", actor_uid );

        // Setup state so that the slice animation does not need to gather additional info
        for ( Fixture *pf : m_animation_task->resolveActorFixtures( actor ) ) {
            if ( pf->getNumPixels() == 0 )
                continue;

            PixelArray* pixels = pf->getPixels();
            RGBWA initial_color;

            Pixel& pixel = pixels->at(0);

            if ( pixel.hasRed() )
                initial_color.red( actor->getChannelValue( pixel.red() ) );
            if ( pixel.hasGreen() )
                initial_color.green( actor->getChannelValue( pixel.green() ) );
            if ( pixel.hasBlue() )
                initial_color.blue( actor->getChannelValue( pixel.blue() ) );
            if ( pixel.hasWhite() )
                initial_color.white( actor->getChannelValue( pixel.white() ) );
            if ( pixel.hasAmber() )
                initial_color.amber( actor->getChannelValue( pixel.amber() ) );

            m_fixtures.push_back( FaderFixture( pf, pixels, initial_color ) );
        }
    }

    m_signal_processor = new AnimationSignalProcessor( m_signal, task );
}

// ----------------------------------------------------------------------------
//
bool SceneColorFader::sliceAnimation( DWORD time_ms, BYTE* dmx_packet )
{
   if ( m_fixtures.size() == 0 )
    return false;

    bool tick = m_signal_processor->tick( time_ms );
    bool changed = false;

    // Time to switch
    if ( tick ) {
        unsigned level = m_signal_processor->getLevel();

        // Choose next color
        RGBWA rgbwa = m_colors[m_color_index];
        m_color_index = (m_color_index + 1) % m_colors.size();

        if ( m_fader_effect == FADER_EFFECT_ALL ) {
            if ( m_effect_periods-- < 1 ) {                 // Choose new effect
                int roll = rand() % 100;

                if ( level < 10 )					        // No or little sound then just blend
                    m_current_effect = FADER_EFFECT_BLEND;
                else if ( level > 90 && m_signal.getInputType() != CAI_TIMER_ONLY ) { // High sound energy change & strober
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

            if ( m_start_strobe ) {
                m_strobe.start( time_ms, m_strobe_pos_ms * (110-level) / 100, m_strobe_neg_ms, m_strobe_flashes );
                m_start_strobe = false;
            }
        }
        else if ( m_current_effect == FADER_EFFECT_CHANGE ) {
            for ( FaderFixture& sfixture : m_fixtures ) {
                loadColorChannels( dmx_packet, sfixture, rgbwa );
                changed = true;
            }
            m_start_strobe = true;
        }
        else if ( m_current_effect == FADER_EFFECT_BLEND ) {
            WORD time = (WORD)(m_signal_processor->getNextSampleMS()-time_ms);
            if ( time > m_signal.getSampleRateMS() / 20 )   // Keep at target color a bit longer
                time -= (WORD)( m_signal.getSampleRateMS() / 20 );
            for ( FaderFixture& sfixture : m_fixtures )
                sfixture.m_fader.start( time_ms, time, rgbwa );
            m_start_strobe = true;
        }
    }

    switch ( m_current_effect ) {
        case FADER_EFFECT_STROBE:
            if ( m_strobe.strobe( time_ms ) ) {
                for ( FaderFixture& sfixture : m_fixtures )
                    loadColorChannels( dmx_packet, sfixture, m_strobe.rgbwa() );
                changed = true;
            }
            break;

        case FADER_EFFECT_BLEND:
            for (  FaderFixture& sfixture : m_fixtures ) {
                if ( sfixture.m_fader.fade( time_ms ) ) {
                    loadColorChannels( dmx_packet, sfixture, sfixture.m_fader.rgbwa() );
                    changed = true;
                }
            }
            break;
    }

    return changed;
}

// ----------------------------------------------------------------------------
//
void SceneColorFader::loadColorChannels( BYTE* dmx_packet, FaderFixture& sfixture, RGBWA& rgbwa )
{
    for ( Pixel& pixel : *sfixture.m_pixels ) {
        if ( pixel.hasRed() )
            m_animation_task->loadChannel( dmx_packet, sfixture.m_pf, pixel.red(), rgbwa.red() );
        if ( pixel.hasGreen() )
            m_animation_task->loadChannel( dmx_packet, sfixture.m_pf, pixel.green(), rgbwa.green() );
        if ( pixel.hasBlue() )
            m_animation_task->loadChannel( dmx_packet, sfixture.m_pf, pixel.blue(), rgbwa.blue() );
        if ( pixel.hasWhite() )
            m_animation_task->loadChannel( dmx_packet, sfixture.m_pf, pixel.white(), rgbwa.white() );
        if ( pixel.hasAmber() )
            m_animation_task->loadChannel( dmx_packet, sfixture.m_pf, pixel.amber(), rgbwa.amber() );
    }

    sfixture.m_fader.setCurrent( rgbwa );
}
