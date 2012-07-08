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
#include "ISerializable.h"
#include "FixtureDefinition.h"

class DefinitionReader : public ISerializable
{
public:
    DefinitionReader(void);
    ~DefinitionReader(void);

    void readFixtureDefinitions( );

    FixtureDefinition* read( TiXmlElement* self, FixtureDefinition* fixture_definition );
    Channel* read( TiXmlElement* self, Channel* channel );
    ChannelAngle* read( TiXmlElement* self, ChannelAngle* channel_angle );
    ChannelValueRange* read( TiXmlElement* self, ChannelValueRange* range );

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
    void readFixtureDefinitions( LPCSTR directory );
};

