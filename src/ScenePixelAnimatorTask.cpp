#include "ScenePixelAnimatorTask.h"

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

#include "ScenePixelAnimatorTask.h"
#include "Bitmap.h"

// ----------------------------------------------------------------------------
//
ScenePixelAnimatorTask::ScenePixelAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
   ChannelAnimatorTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
ScenePixelAnimatorTask::~ScenePixelAnimatorTask(void)
{
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::generateProgram( AnimationDefinition* definition ) 
{
    ScenePixelAnimator* config = dynamic_cast<ScenePixelAnimator *>( definition );

    typedef std::vector<Participant> ParticipantArray;

    ParticipantArray participants;

    RGBWAArray&customColors = config->getCustomColors();

    // Setup the color progression for the animation
    if ( customColors.size() > 1 )
        m_colors.assign( customColors.begin(), customColors.end() );
    else if ( customColors.size() == 1 )
        RGBWA::resolveColor( customColors[0], m_colors );
    else
        RGBWA::getAllPredefinedColors( m_colors );

    // Determine which channels will be participating
	for ( SceneActor& actor : getActors() ) {
        Fixture* pf = getActorRepresentative( actor.getActorUID() );
        if ( pf != NULL && pf->getNumPixels() > 0 ) {
            participants.push_back( Participant( actor.getActorUID(), pf->getPixels() ) );
        }
    }

    if ( participants.size() > 0 ) {
        PixelEngine engine;

        if ( config->getCombineFixtures() ) {
            for ( Participant& p : participants )
                engine.loadPixels( p.m_actor_uid, p.m_pixels );

            generateEffect( engine, config );
        }
        else {
            for ( Participant& p : participants ) {
                engine.clear();
                engine.loadPixels( p.m_actor_uid, p.m_pixels );
                generateEffect( engine, config );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::generateEffect( PixelEngine& engine, ScenePixelAnimator* config ) {

    switch ( config->getEffect() ) {
        case MOVING:            genMovingDots( engine, config );                break;
        case STACKED:           genStackedDots( engine, config, NO_EXIT );      break;
        case STACKED_LEFT:      genStackedDots( engine, config, LEFT_EXIT);     break;
        case STACKED_RIGHT:     genStackedDots( engine, config, RIGHT_EXIT );   break;
        case BEAM:              genBeam( engine, config );                      break;
        case RANDOM:            genRandomDots( engine, config );                break;
        case CHASE:             genChase( engine, config );                     break;
        case COLOR_CHASE:       genColorChase( engine, config );                break;
    }

    add( engine.getProgram() );
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::genMovingDots( PixelEngine& engine, ScenePixelAnimator* config ) {
    ColorFader fader( true );

    const size_t num_pixels = engine.getNumPixels();

    for ( size_t color_index=0; color_index < m_colors.size(); color_index++ ) {
        fader.setCurrent( m_colors[color_index] );

        fader.start( 0, num_pixels / config->getIncrement(), 
            ( config->isFadeColors() ) ? m_colors[( color_index + 1 ) % m_colors.size()] : fader.rgbwa() );

        size_t target_pixel = 0;

        for ( unsigned gen=0; gen < config->getGenerations()*num_pixels / config->getIncrement(); gen++ ) {
            size_t target_range = target_pixel+config->getPixels()-1;
            for ( size_t pixel=0; pixel < num_pixels; pixel++ ) {
                bool fill = (pixel >= target_pixel && pixel <= target_range) ||
                    (target_range >= num_pixels && pixel <= (target_range%num_pixels));

                // XXXX if fading and wrap, need to use next color

                engine.addPixel( pixel, fill ? fader.rgbwa() : config->getEmptyColor() );
            }

            fader.tick();

            target_pixel = (target_pixel+config->getIncrement()) % num_pixels;
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::genStackedDots( PixelEngine& engine, ScenePixelAnimator* config, ExitDirection exit_direction ) {
    const size_t num_pixels = engine.getNumPixels();

    for ( size_t color_index=0; color_index < m_colors.size(); color_index++ ) {
        RGBWA& fill_color = m_colors[color_index];

        for ( size_t stack=num_pixels; stack--; ) {
            for ( size_t mover=0; mover <= stack; mover++ ) {
                for ( unsigned pixel=0; pixel < num_pixels; pixel++ ) {
                    bool fill = ( pixel > stack || mover == pixel );
                    engine.addPixel( pixel, fill ? fill_color : config->getEmptyColor() );
                }
            }
        }

        switch ( exit_direction ) {
        case LEFT_EXIT:
            for ( size_t stack=num_pixels; stack--; ) {
                for ( size_t mover=stack; mover < num_pixels; mover++ ) {
                    for ( unsigned pixel=0; pixel < num_pixels; pixel++ ) {
                        bool fill = ( pixel < stack || mover == pixel );
                        engine.addPixel( pixel, fill ? fill_color : config->getEmptyColor() );
                    }
                }
            }
            break;

        case RIGHT_EXIT:
            for ( size_t stack=0; stack++ < num_pixels; ) {
                for ( size_t mover=stack; mover--;  ) {
                    for ( unsigned pixel=0; pixel < num_pixels; pixel++ ) {
                        bool fill = ( pixel > stack || mover == pixel );
                        engine.addPixel( pixel, fill ? fill_color : config->getEmptyColor() );
                    }
                }
            }
            break;
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::genBeam( PixelEngine& engine, ScenePixelAnimator* config ) {
    ColorFader fader( true );

    const size_t num_pixels = engine.getNumPixels();

    for ( size_t color_index=0; color_index < m_colors.size(); color_index++ ) {
        fader.setCurrent( m_colors[color_index] );

        fader.start( 0, (num_pixels * 2 - 3), // # of ticks to get to next color (-3 = start, turn-around, end colors)
            ( config->isFadeColors() ) ? m_colors[(color_index+1) % m_colors.size()] : fader.rgbwa() );

        for ( size_t mover=0; mover < num_pixels; mover++ ) {
            for ( unsigned pixel=0; pixel < num_pixels; pixel++ )
                engine.addPixel( pixel, mover == pixel ? fader.rgbwa() : config->getEmptyColor() );
            fader.tick();
        }

        for ( size_t mover=num_pixels-2; mover > 0; mover-- ) {
            for ( unsigned pixel=0; pixel < num_pixels; pixel++ )
                engine.addPixel( pixel, mover == pixel ? fader.rgbwa() : config->getEmptyColor() );
            fader.tick();
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::genRandomDots( PixelEngine& engine, ScenePixelAnimator* config ) {
    Bitmap pixelMap;

    const size_t num_pixels = engine.getNumPixels();

    for ( unsigned gen=0; gen < config->getGenerations(); gen++ ) {
        pixelMap.clear();

        for ( unsigned dot=0; dot < config->getPixels() && dot < num_pixels; dot++ ) {
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
                engine.addPixel( pixel, config->getEmptyColor() );
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::genChase( PixelEngine& engine, ScenePixelAnimator* config ) {
    const size_t num_pixels = engine.getNumPixels();
    const size_t MAX_DOTS = 32;

    const unsigned num_chase_dots = std::max<unsigned>( config->getPixels(), MAX_DOTS );

    size_t dot_positions[MAX_DOTS];
    memset( dot_positions, 0, sizeof(dot_positions) );

    RGBWA dot_colors[MAX_DOTS];
    for ( size_t i=0; i < num_chase_dots; i++ )
        dot_colors[i] = m_colors[i % m_colors.size()];

    for ( unsigned gen=0; gen < config->getGenerations()*num_pixels; gen++ ) {
        for ( size_t pixel=0; pixel < num_pixels; pixel++ ) {
            RGBWA color = config->getEmptyColor();

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
                dot_positions[i] = (dot_positions[i] + (i*config->getIncrement())) % num_pixels;
        }
    }
}

// ----------------------------------------------------------------------------
//
void ScenePixelAnimatorTask::genColorChase( PixelEngine& engine, ScenePixelAnimator* config )
{
    const size_t num_pixels = engine.getNumPixels();

    int color_offset = 0;

    for ( unsigned gen=0; gen < config->getPixels(); gen++, color_offset = (color_offset+1) % m_colors.size() ) {
        int color_index = color_offset;

        for ( size_t pixel=0; pixel < num_pixels; pixel++ ) {
            engine.addPixel( pixel, m_colors[color_index] );
            color_index = (color_index+1) % m_colors.size();
        }
    }
}