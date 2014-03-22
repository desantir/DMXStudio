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

#include "DMXStudio.h"

class Venue;
class Fixture;
class Scene;
class SceneActor;
class Chase;
class FixtureGroup;
class SceneStrobeAnimator;
class ScenePatternDimmer;
class SceneMovementAnimator;
class SceneColorFader;
class SceneChannelAnimator;
class SceneSequence;
class SceneSoundLevel;
class AnimationSignal;
class ChaseStep;
class MovementAnimation;
class ChannelAnimation;
struct MusicSceneSelector;
class ScenePixelAnimator;

class IVisitor
{
public:
    virtual void visit( Venue* venue ) = 0;
    virtual void visit( Fixture* fixture ) = 0;
    virtual void visit( Scene* scene ) = 0;
    virtual void visit( SceneActor* actor ) = 0;
    virtual void visit( Chase* chase ) = 0;
    virtual void visit( FixtureGroup* fixture_group ) = 0;

    virtual void visit( SceneStrobeAnimator* animation ) = 0;
    virtual void visit( ScenePatternDimmer* animation ) = 0;
    virtual void visit( SceneMovementAnimator* animation ) = 0;
    virtual void visit( SceneColorFader* animation ) = 0;
    virtual void visit( SceneChannelAnimator* animation ) = 0;
    virtual void visit( SceneSequence* animation ) = 0;    
    virtual void visit( SceneSoundLevel* animation ) = 0;   
    virtual void visit( ScenePixelAnimator* animation ) = 0;   

    virtual void visit( AnimationSignal* signal ) = 0;
    virtual void visit( ChaseStep* chase_step ) = 0;
    virtual void visit( MovementAnimation* movement ) = 0;
    virtual void visit( ChannelAnimation* channel_animation ) = 0;

    virtual void visit( MusicSceneSelector* music_scene_selection ) = 0;

protected:
    virtual ~IVisitor() {}
};