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

const char* SceneColorFader::className = "SceneColorSwitcher";
const char* SceneColorFader::animationName = "Color fader";

// ----------------------------------------------------------------------------
//
SceneColorFader::SceneColorFader( UID animation_uid, bool shared, UID reference_fixture, 
                                        AnimationSignal signal,
										StrobeTime strobe_time,
                                        RGBWA strobe_neg_color,
                                        RGBWAArray custom_colors,
                                        FaderEffect fader_effect ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_strobe_neg_color( strobe_neg_color ),
    m_strobe_time( strobe_time ),
    m_custom_colors( custom_colors ),
    m_fader_effect( fader_effect )
{
}

// ----------------------------------------------------------------------------
//
SceneColorFader::~SceneColorFader(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneColorFader::clone( ) {
	return new SceneColorFader( 0L, m_shared, m_reference_fixture, m_signal, m_strobe_time, m_strobe_neg_color, m_custom_colors, m_fader_effect );
}

// ----------------------------------------------------------------------------
//
AnimationTask* SceneColorFader::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new SceneColorFaderTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString SceneColorFader::getSynopsis(void) {
    CString synopsis;

    switch ( m_fader_effect ) {
        case FADER_EFFECT_CHANGE:
            synopsis.Format( "Fade( Single Color Change )\n" );
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

        case FADER_EFFECT_CHANGE_MULTI:
            synopsis.Format( "Fade( Milti-color Change )\n" );
            break; 
    }

    if ( m_custom_colors.size() ) {
        synopsis.AppendFormat( "Custom Colors( " );
        for ( RGBWAArray::iterator it=m_custom_colors.begin(); it != m_custom_colors.end(); ++it )
            synopsis.AppendFormat( "%s ", (*it).getColorName() );
        synopsis.AppendFormat( ")\n" );
    }
    
    synopsis.AppendFormat( "Strobe( -color=%s +ms=%u +fade=%u -ms=%u -fade=%u flash=%u )\n%s", m_strobe_neg_color.getColorName(),
		m_strobe_time.getOnMS(), m_strobe_time.getFadeInMS(), m_strobe_time.getOffMS(), m_strobe_time.getFadeOutMS(), m_strobe_time.getFlashes(),
        AnimationDefinition::getSynopsis() );

    return synopsis;
}
