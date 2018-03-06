/* 
    Copyright (C) 2014-2016 Robert DeSantis
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

#include "ScenePixelAnimator.h"
#include "ColorFader.h"
#include "Bitmap.h"
#include "ScenePixelAnimatorTask.h"

const char* ScenePixelAnimator::className = "ScenePixelAnimator";
const char* ScenePixelAnimator::animationName = "Pixel animator";

// ----------------------------------------------------------------------------
//
ScenePixelAnimator::ScenePixelAnimator( UID animation_uid, bool shared, UID reference_fixture, 
                                        AnimationSignal signal,
                                        PixelEffect effect,
                                        RGBWAArray custom_colors,
                                        RGBWA empty_color,
                                        unsigned generations,
                                        unsigned num_pixels,
                                        bool color_fade,
                                        unsigned increment,
                                        bool combine_fixtures ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_effect ( effect ),
    m_custom_colors ( custom_colors ),
    m_empty_color ( empty_color ),
    m_generations ( generations ),
    m_num_pixels ( num_pixels ),
    m_color_fade ( color_fade ),
    m_increment ( increment ),
    m_combine_fixtures ( combine_fixtures )
{
}

// ----------------------------------------------------------------------------
//
ScenePixelAnimator::~ScenePixelAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* ScenePixelAnimator::clone( ) {
	return new ScenePixelAnimator( 0L, m_shared, m_reference_fixture, m_signal, m_effect, m_custom_colors, m_empty_color, m_generations, m_num_pixels, m_color_fade, m_increment, m_combine_fixtures );
}

// ----------------------------------------------------------------------------
//
AnimationTask* ScenePixelAnimator::createTask( AnimationEngine* engine, ActorList& actors, UID owner_uid ) {
    return new ScenePixelAnimatorTask( engine, m_uid, actors, owner_uid );
}

// ----------------------------------------------------------------------------
//
CString ScenePixelAnimator::getSynopsis(void) {
    CString synopsis;

    switch ( m_effect ) {
        case MOVING:
            synopsis.Format( "Scrolling( generations=%d pixels=%d increment=%d fade=%s ", 
                            m_generations, m_num_pixels, m_increment, (m_color_fade) ? "yes" : "no" );
            break;

        case STACKED:
            synopsis.Format( "Stacked( " );
            break;

        case STACKED_LEFT:
            synopsis.Format( "Stacked Left( " );
            break;
        
        case STACKED_RIGHT:
            synopsis.Format( "Stacked Right( " );
            break;
        
        case BEAM:
            synopsis.Format( "Beam( pixels=%d fade=%s ", m_num_pixels, ((m_color_fade) ? "yes" : "no") );
            break;

        case RANDOM:
            synopsis.Format( "Random( generations=%d pixels=%d ", m_generations, m_num_pixels );
            break;

        case CHASE:
            synopsis.Format( "Chase( generations=%d pixels=%d increment=%d ", m_generations, m_num_pixels, m_increment );
            break;

        case COLOR_CHASE:
            synopsis.Format( "Color Chase( pixels=%d ", m_num_pixels );
            break;
    }

    synopsis.AppendFormat( "pixel off color=%s combine=%s )\n", 
                m_empty_color.getColorName(), ((m_combine_fixtures) ? "yes" : "no") );

    if ( m_custom_colors.size() ) {
        synopsis.AppendFormat( "Custom Colors( " );
        for ( RGBWAArray::iterator it=m_custom_colors.begin(); it != m_custom_colors.end(); ++it )
            synopsis.AppendFormat( "%s ", (*it).getColorName() );
        synopsis.AppendFormat( ")\n" );
    }

    synopsis.AppendFormat( "%s", AnimationDefinition::getSynopsis() );

    return synopsis;
}
