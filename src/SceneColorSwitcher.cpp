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


#include "SceneColorSwitcher.h"
#include "AnimationTask.h"

const char* SceneColorSwitcher::className = "SceneColorSwitcher";

#define NUM_COLORS 16

const BYTE color_table_rgb[NUM_COLORS][COLOR_CHANNELS] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00 },				         // Black
  { 0xFF, 0x00, 0x00, 0x00, 0x00 },                      // Red
  { 0xFF, 0x20, 0x00, 0x00, 0x00 },                      // Blood orange    
  { 0xFF, 0x40, 0x00, 0x00, 0x00 },                      // Orange  
  { 0xFF, 0x40, 0x20, 0x00, 0x00 },                      // Salmon    
  { 0xFF, 0xFF, 0x00, 0x00, 0x00 },                      // Yellow
  { 0x30, 0xFF, 0x05, 0x00, 0x00 },                      // Lime
  { 0x00, 0xFF, 0x00, 0x00, 0x00 },                      // Green
  { 0x00, 0xFF, 0x20, 0x00, 0x00 },                      // Teal
  { 0x10, 0xFF, 0x7F, 0x00, 0x00 },                      // Cyan
  { 0x00, 0x80, 0x80, 0x00, 0x00 },                      // Light blue
  { 0x00, 0x00, 0xFF, 0x00, 0x00 },                      // Blue
  { 0x40, 0x00, 0xFF, 0x00, 0x00 },                      // Violet  
  { 0xFF, 0x00, 0xFF, 0x00, 0x00 },                      // Magenta
  { 0xFF, 0x00, 0x20, 0x00, 0x00 },                      // Hot pink
  { 0xFF, 0xFF, 0xFF, 0xFF, 0x00 }                       // White  
};

ColorNames generate_color_names() {
    ColorNames map;
    map[0] = "Black";
    map[1] = "Red";
    map[2] = "Blood orange";
    map[3] = "Orange";  
    map[4] = "Salmon";    
    map[5] = "Yellow";
    map[6] = "Lime";
    map[7] = "Green";
    map[8] = "Teal";
    map[9] = "Cyan";
    map[10] = "Light blue";
    map[11] = "Blue";
    map[12] = "Violet";  
    map[13] = "Magenta";
    map[14] = "Hot pink";
    map[15] = "White"; 
    return map;
}

ColorNames SceneColorSwitcher::color_names = generate_color_names();

// ----------------------------------------------------------------------------
//
SceneColorSwitcher::SceneColorSwitcher( UID animation_uid, 
                                        AnimationSignal signal,
                                        UIDArray actors,
                                        unsigned strobe_neg_color,
                                        unsigned strobe_pos_ms,
                                        unsigned strobe_neg_ms,
                                        ColorProgression custom_colors ) :
    AbstractAnimation( animation_uid, signal ),
    m_strobe_neg_color( strobe_neg_color ),
    m_strobe_pos_ms( strobe_pos_ms ),
    m_strobe_neg_ms( strobe_neg_ms ),
    m_signal_processor( NULL ),
    m_custom_colors( custom_colors ),
    m_strobe_periods( 0 )
{
    m_actors = actors;
}

// ----------------------------------------------------------------------------
//
SceneColorSwitcher::~SceneColorSwitcher(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneColorSwitcher::clone() {
    return new SceneColorSwitcher( m_uid, m_signal, m_actors, m_strobe_neg_color, 
                                   m_strobe_pos_ms, m_strobe_neg_ms, m_custom_colors );
}

// ----------------------------------------------------------------------------
//
CString SceneColorSwitcher::getSynopsis(void) {
    CString synopsis;

    if ( m_custom_colors.size() ) {
        synopsis.AppendFormat( "Custom Colors( " );
        for ( ColorProgression::iterator it=m_custom_colors.begin(); it != m_custom_colors.end(); it++ )
            synopsis.AppendFormat( "%s ", color_names[*it] );
        synopsis.AppendFormat( ") " );
    }
    
    synopsis.AppendFormat( "Strobe( -color=%s +ms=%u -ms=%u ) %s", color_names[m_strobe_neg_color],
        m_strobe_pos_ms, m_strobe_neg_ms, 
        AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void SceneColorSwitcher::stopAnimation( )
{
    if ( m_signal_processor != NULL ) {
        delete m_signal_processor;
        m_signal_processor = NULL;
    }
}

// ----------------------------------------------------------------------------
//
void SceneColorSwitcher::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_color_effect = COLOR_EFFECT_CHANGE;
    m_color_index = m_custom_index = 0;
    m_strobe.setNegative( color_table_rgb[ m_strobe_neg_color < NUM_COLORS ? m_strobe_neg_color : 0 ] );

    UIDArray actors = populateActors( m_animation_task->getScene() );

    // For each fixture, find red, green, blue, white channels
    for ( UIDArray::iterator it=actors.begin(); it != actors.end(); it++ ) {
        Fixture* pf = m_animation_task->getFixture( (*it) );
        STUDIO_ASSERT( pf != NULL, "Missing fixture UID=%lu", (*it) );
        SceneActor* actor = task->getScene()->getActor(pf->getUID());
        STUDIO_ASSERT( actor != NULL, "Missing scene actor for fixture %lu", pf->getUID() );
                
        channel_t color_channels[ COLOR_CHANNELS ] = 
            { INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL, INVALID_CHANNEL };
        BYTE color_values[ COLOR_CHANNELS ] =
            { 0, 0, 0, 0, 0 };

        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            if ( cp->getType() == CHNLT_RED ) {
                color_channels[RED_INDEX] = channel;
                color_values[RED_INDEX] = actor->getChannelValue( channel );
            }
            else if ( cp->getType() == CHNLT_GREEN ) {
                color_channels[GREEN_INDEX] = channel;
                color_values[GREEN_INDEX] = actor->getChannelValue( channel );
            }
            else if ( cp->getType() == CHNLT_BLUE ) {
                color_channels[BLUE_INDEX] = channel;
                color_values[BLUE_INDEX] = actor->getChannelValue( channel );
            }
            else if ( cp->getType() == CHNLT_WHITE ) {
                color_channels[WHITE_INDEX] = channel;
                color_values[WHITE_INDEX] = actor->getChannelValue( channel );
            }
            else if ( cp->getType() == CHNLT_AMBER ) {
                color_channels[AMBER_INDEX] = channel;
                color_values[AMBER_INDEX] = actor->getChannelValue( channel );
            }
        }

        m_fixtures.push_back( SwitcherFixture( pf, color_channels, color_values ) );
    }

    m_signal_processor = new AnimationSignalProcessor( m_signal, task );
}

// ----------------------------------------------------------------------------
//
bool SceneColorSwitcher::sliceAnimation( DWORD time_ms, BYTE* dmx_packet )
{
    bool tick = m_signal_processor->tick( time_ms );
    bool changed = false;

    // Time to switch
    if ( tick ) {
        unsigned level = m_signal_processor->getLevel();
        int roll = rand() % 100;

        // Get next color
        if ( m_custom_colors.size() ) {
            m_color_index = m_custom_colors[m_custom_index];
            m_custom_index = (m_custom_index+1) % m_custom_colors.size() ;
        }
        else
            m_color_index = ((m_color_index) % (NUM_COLORS-1)) + 1;

        if ( m_color_effect == COLOR_EFFECT_STROBE && m_strobe_periods > 0 ) {
            m_strobe.setColor( color_table_rgb[m_color_index] );
            m_strobe_periods--;
        }
        else {
            if ( level < 10 )					// No or little sound then just blend
                m_color_effect = COLOR_EFFECT_BLEND;
            else if ( level > 90 ) {			// High sound energy change & strober
                m_color_effect = (roll > 75) ? COLOR_EFFECT_STROBE : COLOR_EFFECT_CHANGE;
            }
            else {
                if ( roll > 80 )
                    m_color_effect = COLOR_EFFECT_STROBE;
                else if ( roll > 50 )
                    m_color_effect = COLOR_EFFECT_CHANGE;
                else
                    m_color_effect = COLOR_EFFECT_BLEND;
            }

            if ( m_color_effect == COLOR_EFFECT_STROBE ) {
                m_strobe.setColor( color_table_rgb[m_color_index] );
                m_strobe.start( time_ms, m_strobe_pos_ms * (110-level) / 100, m_strobe_neg_ms );
                m_strobe_periods = rand() % 5;  // Keep strobe going for a bit to get full effect
            }
            else if ( m_color_effect == COLOR_EFFECT_CHANGE ) {
                for ( SwitcherFixtureArray::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
                    loadColorChannels( dmx_packet, (*it), color_table_rgb[m_color_index] );
                    changed = true;
                }
            }
            else if ( m_color_effect == COLOR_EFFECT_BLEND ) {
                WORD time = (WORD)(m_signal_processor->getNextSampleMS()-time_ms)/2;

                for ( SwitcherFixtureArray::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
                    SwitcherFixture& sfixture = (*it);
                    sfixture.m_fader.setTargets( color_table_rgb[m_color_index] );
                    sfixture.m_fader.start( time_ms, time );
                }
            }
        }
    }

    switch ( m_color_effect ) {
        case COLOR_EFFECT_STROBE:
            if ( m_strobe.strobe( time_ms ) ) {
                for ( SwitcherFixtureArray::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
                    SwitcherFixture& sfixture = (*it);
                    loadColorChannels( dmx_packet, sfixture, m_strobe.rgbwa() );
                }

                changed = true;
            }

            break;

        case COLOR_EFFECT_BLEND:
            for ( SwitcherFixtureArray::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
                SwitcherFixture& sfixture = (*it);
                
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
void SceneColorSwitcher::loadColorChannels( BYTE* dmx_packet, SwitcherFixture& sfixture, const BYTE* rgbwa )
{
    for ( int index=0; index < COLOR_CHANNELS; index++ )
        if ( sfixture.m_channels[index] != INVALID_CHANNEL )
            m_animation_task->loadChannel( dmx_packet, sfixture.m_pf, sfixture.m_channels[index], rgbwa[index] );

    sfixture.m_fader.setCurrent( rgbwa );
}
