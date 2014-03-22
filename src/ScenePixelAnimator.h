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

#pragma once

#include "IVisitor.h"
#include "SceneChannelAnimator.h"

class PixelEngine
{
    enum { COLOR_CHANNELS = 4 };

    size_t                          m_pixels;
    ChannelAnimationArray           m_animation_array;

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
            m_animation_array.push_back( ChannelAnimation( actor_uid, pixels->at( index ).red(), CAM_LIST ) );
            m_animation_array.push_back( ChannelAnimation( actor_uid, pixels->at( index ).green(), CAM_LIST ) );
            m_animation_array.push_back( ChannelAnimation( actor_uid, pixels->at( index ).blue(), CAM_LIST ) );
            m_animation_array.push_back( ChannelAnimation( actor_uid, pixels->at( index ).white(), CAM_LIST ) );

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

    inline void loadChannelAnimations( ChannelAnimationArray& target ) {
        target.insert( target.end(), m_animation_array.begin(), m_animation_array.end() );
    }
};

enum PixelEffect { MOVING=1, STACKED=2, STACKED_LEFT=3, STACKED_RIGHT=4, BEAM=5, RANDOM=6, CHASE=7 };

class ScenePixelAnimator : public SceneChannelAnimator
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

    // Configuration
    PixelEffect                 m_effect;
    RGBWAArray                  m_custom_colors;
    RGBWA   					m_empty_color;
    unsigned                    m_generations;
    unsigned                    m_num_pixels;
    bool                        m_color_fade;
    unsigned                    m_increment;
    bool                        m_combine_fixtures;

public:
    static const char* className;
    static const char* animationName;

    ScenePixelAnimator() {}

    ScenePixelAnimator( UID animation_uid, 
                        AnimationSignal signal,
                        UIDArray actors,
                        PixelEffect effect,
                        RGBWAArray custom_colors,
                        RGBWA empty_color,
                        unsigned generations,
                        unsigned num_pixels,
                        bool color_fade,
                        unsigned increment,
                        bool combine_fixtures );

    ~ScenePixelAnimator(void);

    AbstractAnimation* clone();

    const char* getName() { return ScenePixelAnimator::animationName; }
    const char* getClassName() { return ScenePixelAnimator::className; }

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );

    virtual CString getSynopsis(void);

    inline void setEffect( PixelEffect effect ) {
        m_effect = effect;
    }
    inline PixelEffect getEffect() const {
        return m_effect;
    }

    inline void setGenerations( unsigned generations ) {
        m_generations = generations;
    }
    inline unsigned getGenerations() const {
        return m_generations;
    }

    inline void setPixel( unsigned pixels ) {
        m_num_pixels = pixels;
    }
    inline unsigned getPixels() const {
        return m_num_pixels;
    }

    inline void setIncrement( unsigned increment ) {
        m_increment = increment;
    }
    inline unsigned getIncrement() const {
        return m_increment;
    }

    inline bool isFadeColors() const {
        return m_color_fade;
    }
    inline void setFadeColors( bool fade ) {
        m_color_fade = fade;
    }

    inline bool getCombineFixtures() const {
        return m_combine_fixtures;
    }
    inline void setCombineFixtures( bool combine ) {
        m_combine_fixtures = combine;
    }

    inline RGBWA getEmptyColor( ) const {
        return m_empty_color;
    }
    inline void setEmptyColor( RGBWA rgb ) {
        m_empty_color = rgb;
    }

    RGBWAArray getCustomColors( ) const {
        return m_custom_colors;
    }
    void setCustomColors( RGBWAArray colors ) {
        m_custom_colors = colors;
    }

private:
    void generateEffect( PixelEngine& engine );
    void genMovingDots( PixelEngine& engine );
    void genStackedDots( PixelEngine& engine, ExitDirection exit_direction );
    void genBeam( PixelEngine& engine );
    void genRandomDots( PixelEngine& engine );
    void genChase( PixelEngine& engine );
};

