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

#pragma once

#include "ChannelAnimatorTask.h"
#include "ScenePixelAnimator.h"

class PixelEngine
{
    enum { COLOR_CHANNELS = 4 };

    size_t              m_pixels;
    ChannelProgramArray m_animation_array;

public:
    PixelEngine( ) :
        m_pixels(0)
    {}

    ~PixelEngine() {
    }

    void clear() {
        m_pixels = 0;
        m_animation_array.clear();
    }

    void loadPixels( UID actor_uid,  PixelArray* pixels ) {
        for ( size_t index=0; index < pixels->size(); index++ ) {
            m_animation_array.emplace_back( actor_uid, pixels->at( index ).red(), CAM_LIST );
            m_animation_array.emplace_back( actor_uid, pixels->at( index ).green(), CAM_LIST );
            m_animation_array.emplace_back( actor_uid, pixels->at( index ).blue(), CAM_LIST );
            m_animation_array.emplace_back( actor_uid, pixels->at( index ).white(), CAM_LIST );

            m_pixels++;
        }
    }

    inline size_t getNumPixels( ) const {
        return m_pixels;
    }

    inline void addPixel( size_t pixel, RGBWA& color ) {
        size_t offset = pixel * COLOR_CHANNELS;

        m_animation_array[offset + 0].addChannelValue( color.red() );
        m_animation_array[offset + 1].addChannelValue( color.green() );
        m_animation_array[offset + 2].addChannelValue( color.blue() );
        m_animation_array[offset + 3].addChannelValue( color.white() );
    }

    inline ChannelProgramArray& getProgram() {
        return m_animation_array;
    }
};

class ScenePixelAnimatorTask : public ChannelAnimatorTask
{
    friend class VenueWriter;
    friend class VenueReader;

    enum ExitDirection { NO_EXIT=1, RIGHT_EXIT=2, LEFT_EXIT=3 };

    struct Participant {
        PixelArray*         m_pixels;
        UID                 m_actor_uid;

        Participant( UID actor_uid, PixelArray* pixels ) :
            m_actor_uid( actor_uid ),
            m_pixels( pixels )
        {}
    };

    RGBWAArray                  m_colors;

    void generateProgram( AnimationDefinition* definition );

public:
    ScenePixelAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid=NOUID );
    ~ScenePixelAnimatorTask();

private:
    void generateEffect( PixelEngine& engine, ScenePixelAnimator* config );
    void genMovingDots( PixelEngine& engine, ScenePixelAnimator* config );
    void genStackedDots( PixelEngine& engine, ScenePixelAnimator* config, ExitDirection exit_direction );
    void genBeam( PixelEngine& engine, ScenePixelAnimator* config );
    void genRandomDots( PixelEngine& engine, ScenePixelAnimator* config );
    void genChase( PixelEngine& engine, ScenePixelAnimator* config );
    void genColorChase( PixelEngine& engine, ScenePixelAnimator* config );
};

