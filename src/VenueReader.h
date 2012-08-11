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

#include "Venue.h"
#include "ISerializable.h"

class VenueReader : public ISerializable
{
public:
    VenueReader(void);
    ~VenueReader(void);

    Venue* read( LPCSTR input_file );

    Venue* read( TiXmlElement* self, Venue* venue );
    Scene* read( TiXmlElement* self, Scene* scene );
    Fixture* read( TiXmlElement* self, Fixture* fixture );
    Chase* read( TiXmlElement* self, Chase* chase );
    FixtureGroup* read( TiXmlElement* self, FixtureGroup* fixture_group );
    SceneActor* read( TiXmlElement* self, SceneActor* actor );
    SceneStrobeAnimator* read( TiXmlElement* self, SceneStrobeAnimator* animation );
    ScenePatternDimmer* read( TiXmlElement* self, ScenePatternDimmer* animation );
    SceneMovementAnimator* read( TiXmlElement* self, SceneMovementAnimator* animation );
    SceneColorSwitcher* read( TiXmlElement* self, SceneColorSwitcher* animation );
    SceneChannelAnimator* read( TiXmlElement* self, SceneChannelAnimator* animation );
    SceneSequence* read( TiXmlElement* self, SceneSequence* animation );  
    SceneSoundLevel* read( TiXmlElement* self, SceneSoundLevel* animation );  
    AnimationSignal* read( TiXmlElement* self, AnimationSignal *signal );
    ChaseStep* read( TiXmlElement* self, ChaseStep* chase_step );
    MovementAnimation* read( TiXmlElement* self, MovementAnimation* movement );
    ChannelAnimation* read( TiXmlElement* self, ChannelAnimation* channel_animation );
    MusicSceneSelector* read( TiXmlElement* self, MusicSceneSelector* music_scene_selection );

    template <class T>
    std::vector<T *> read_xml_list( TiXmlElement *container, const char *element_name ) {
        std::vector<T *> list;

        if ( container ) {
            TiXmlElement* element = container->FirstChildElement( element_name );
            while ( element ) {
                T * instance = read( element, (T *)NULL );
                if ( instance ) {
                    list.push_back( instance );
                }
                element = element->NextSiblingElement();
            }
        }

        return list;
    }

private:
    void readDObject( TiXmlElement* self, DObject* dobject, LPCSTR number_name );
};

