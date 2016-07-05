/* 
    Copyright (C) 2014 Robert DeSantis
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

const char* ScenePixelAnimator::className = "ScenePixelAnimator";
const char* ScenePixelAnimator::animationName = "Pixel animator";

// ----------------------------------------------------------------------------
//
ScenePixelAnimator::ScenePixelAnimator( UID animation_uid, 
                                        AnimationSignal signal,
                                        UIDArray actors,
                                        PixelEffect effect,
                                        RGBWAArray custom_colors,
                                        RGBWA empty_color,
                                        unsigned generations,
                                        unsigned num_pixels,
                                        bool color_fade,
                                        unsigned increment,
                                        bool combine_fixtures ) :
    SceneChannelAnimator( animation_uid, signal ),
    m_effect ( effect ),
    m_custom_colors ( custom_colors ),
    m_empty_color ( empty_color ),
    m_generations ( generations ),
    m_num_pixels ( num_pixels ),
    m_color_fade ( color_fade ),
    m_increment ( increment ),
    m_combine_fixtures ( combine_fixtures )
{
    m_actors = actors;

    // Setup the color progression for the animation
    if ( m_custom_colors.size() == 1 && m_custom_colors[1] == RGBWA(1,1,1) )
        RGBWA::getRainbowColors( m_colors );
    else if ( m_custom_colors.size() )
        m_colors.assign( m_custom_colors.begin(), m_custom_colors.end() );
    else
        RGBWA::getAllPredefinedColors( m_colors );
}

// ----------------------------------------------------------------------------
//
ScenePixelAnimator::~ScenePixelAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* ScenePixelAnimator::clone() {
    return new ScenePixelAnimator( m_uid, m_signal, m_actors, m_effect, m_custom_colors, 
                                   m_empty_color, m_generations, m_num_pixels, m_color_fade,
                                   m_increment, m_combine_fixtures  );
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
    }

    synopsis.AppendFormat( "pixel off color=%s combine=%s )\n", 
                m_empty_color.getColorName(), ((m_combine_fixtures) ? "yes" : "no") );

    if ( m_custom_colors.size() ) {
        synopsis.AppendFormat( "Custom Colors( " );
        for ( RGBWAArray::iterator it=m_custom_colors.begin(); it != m_custom_colors.end(); ++it )
            synopsis.AppendFormat( "%s ", (*it).getColorName() );
        synopsis.AppendFormat( ")\n" );
    }

    synopsis.AppendFormat( "%s", AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_channel_animations.clear();

    typedef std::vector<Participant> ParticipantArray;

    ParticipantArray participants;

    // Determine which channels will be participating
    for ( UID actor_uid : populateActors() ) {
        Fixture* pf = m_animation_task->getActorRepresentative( actor_uid );
        if ( pf != NULL && pf->getNumPixels() > 0 ) {
            participants.push_back( Participant( actor_uid, pf->getPixels() ) );
        }
    }

    if ( participants.size() > 0 ) {
        PixelEngine engine;

        if ( m_combine_fixtures ) {
            for ( Participant& p : participants )
                engine.loadPixels( p.m_actor_uid, p.m_pixels );

            generateEffect( engine );
        }
        else {
            for ( Participant& p : participants ) {
                engine.clear();
                engine.loadPixels( p.m_actor_uid, p.m_pixels );
                generateEffect( engine );
            }
        }
    }

    SceneChannelAnimator::initAnimation( task, time_ms, dmx_packet );
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::generateEffect( PixelEngine& engine ) {

    switch ( m_effect ) {
        case MOVING:            genMovingDots( engine );                break;
        case STACKED:           genStackedDots( engine, NO_EXIT );      break;
        case STACKED_LEFT:      genStackedDots( engine, LEFT_EXIT);     break;
        case STACKED_RIGHT:     genStackedDots( engine, RIGHT_EXIT );   break;
        case BEAM:              genBeam( engine );                      break;
        case RANDOM:            genRandomDots( engine );                break;
        case CHASE:             genChase( engine );                     break;
    }

    engine.loadChannelAnimations( m_channel_animations );
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::genMovingDots( PixelEngine& engine ) {
    ColorFader fader( true );

    const size_t num_pixels = engine.getNumPixels();

    for ( size_t color_index=0; color_index < m_colors.size(); color_index++ ) {
        fader.setCurrent( m_colors[color_index] );

        fader.start( 0, num_pixels / m_increment, 
            ( m_color_fade ) ? m_colors[( color_index + 1 ) % m_colors.size()] : fader.rgbwa() );

        size_t target_pixel = 0;

        for ( unsigned gen=0; gen < m_generations*num_pixels / m_increment; gen++ ) {
            size_t target_range = target_pixel+m_num_pixels-1;
            for ( size_t pixel=0; pixel < num_pixels; pixel++ ) {
                bool fill = (pixel >= target_pixel && pixel <= target_range) ||
                            (target_range >= num_pixels && pixel <= (target_range%num_pixels));

                // XXXX if fading and wrap, need to use next color

                engine.addPixel( pixel, fill ? fader.rgbwa() : m_empty_color );
            }

            fader.tick();

            target_pixel = (target_pixel+m_increment) % num_pixels;
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::genStackedDots( PixelEngine& engine, ExitDirection exit_direction ) {
    const size_t num_pixels = engine.getNumPixels();

    for ( size_t color_index=0; color_index < m_colors.size(); color_index++ ) {
        RGBWA& fill_color = m_colors[color_index];
             
        for ( size_t stack=num_pixels; stack--; ) {
            for ( size_t mover=0; mover <= stack; mover++ ) {
                for ( unsigned pixel=0; pixel < num_pixels; pixel++ ) {
                    bool fill = ( pixel > stack || mover == pixel );
                    engine.addPixel( pixel, fill ? fill_color : m_empty_color );
                }
            }
        }

        switch ( exit_direction ) {
            case LEFT_EXIT:
                for ( size_t stack=num_pixels; stack--; ) {
                    for ( size_t mover=stack; mover < num_pixels; mover++ ) {
                        for ( unsigned pixel=0; pixel < num_pixels; pixel++ ) {
                            bool fill = ( pixel < stack || mover == pixel );
                            engine.addPixel( pixel, fill ? fill_color : m_empty_color );
                        }
                    }
                }
                break;

            case RIGHT_EXIT:
                for ( size_t stack=0; stack++ < num_pixels; ) {
                    for ( size_t mover=stack; mover--;  ) {
                        for ( unsigned pixel=0; pixel < num_pixels; pixel++ ) {
                            bool fill = ( pixel > stack || mover == pixel );
                            engine.addPixel( pixel, fill ? fill_color : m_empty_color );
                        }
                    }
                }
                break;
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::genBeam( PixelEngine& engine ) {
    ColorFader fader( true );

    const size_t num_pixels = engine.getNumPixels();

    for ( size_t color_index=0; color_index < m_colors.size(); color_index++ ) {
        fader.setCurrent( m_colors[color_index] );

        fader.start( 0, (num_pixels * 2 - 3), // # of ticks to get to next color (-3 = start, turn-around, end colors)
            ( m_color_fade ) ? m_colors[(color_index+1) % m_colors.size()] : fader.rgbwa() );
             
        for ( size_t mover=0; mover < num_pixels; mover++ ) {
            for ( unsigned pixel=0; pixel < num_pixels; pixel++ )
                engine.addPixel( pixel, mover == pixel ? fader.rgbwa() : m_empty_color );
            fader.tick();
        }

        for ( size_t mover=num_pixels-2; mover > 0; mover-- ) {
            for ( unsigned pixel=0; pixel < num_pixels; pixel++ )
                engine.addPixel( pixel, mover == pixel ? fader.rgbwa() : m_empty_color );
            fader.tick();
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::genRandomDots( PixelEngine& engine ) {
    Bitmap pixelMap;

    const size_t num_pixels = engine.getNumPixels();

    for ( unsigned gen=0; gen < m_generations; gen++ ) {
        pixelMap.clear();

        for ( unsigned dot=0; dot < m_num_pixels && dot < num_pixels; dot++ ) {
            while ( true ) {
                int pixel = rand() % num_pixels;
                if ( pixelMap.isSet( pixel ) )
                    continue;
                pixelMap.set( pixel );
                break;
            }
        }

        for ( size_t pixel=0; pixel < num_pixels; pixel++ ) {
            if ( pixelMap.isSet( pixel ) )
                engine.addPixel( pixel, m_colors[rand() % m_colors.size()] );
            else
                engine.addPixel( pixel, m_empty_color );
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimator::genChase( PixelEngine& engine ) {
    const size_t num_pixels = engine.getNumPixels();
    const size_t MAX_DOTS = 32;

    const unsigned num_chase_dots = m_num_pixels > MAX_DOTS ? MAX_DOTS : m_num_pixels;

    size_t dot_positions[MAX_DOTS];
    memset( dot_positions, 0, sizeof(dot_positions) );

    RGBWA dot_colors[MAX_DOTS];
    for ( size_t i=0; i < num_chase_dots; i++ )
        dot_colors[i] = m_colors[i % m_colors.size()];

    for ( unsigned gen=0; gen < m_generations*num_pixels; gen++ ) {
        for ( size_t pixel=0; pixel < num_pixels; pixel++ ) {
            RGBWA color = m_empty_color;

            for ( int i=num_chase_dots; i-- > 0; )
                if ( dot_positions[i] == pixel ) {
                    color = dot_colors[i];
                    break;
                }
                
             engine.addPixel( pixel, color );
        }

        for ( size_t i=0; i < num_chase_dots; i++ ) {
            if ( i == 0 )
                dot_positions[i] = (dot_positions[i] + 1) % num_pixels;
            else
                dot_positions[i] = (dot_positions[i] + (i*m_increment)) % num_pixels;
        }
    }
}