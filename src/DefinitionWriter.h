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
#include "IDefinitionVisitor.h"
#include "ISerializable.h"

class DefinitionWriter : public IDefinitionVisitor, ISerializable
{
    std::vector<TiXmlElement*>      m_parents;

public:
    DefinitionWriter(void);
    ~DefinitionWriter(void);

    void DefinitionWriter::writeFixtureDefinitions( );

    void visit( FixtureDefinition* fixture_definition );
    void visit( Channel* channel );
    void visit( ChannelAngle* channel_angle );
    void visit( ChannelValueRange* range );
    
	template <class T>
	void visit_object( TiXmlElement& parent, T& object ) {
        push_parent( parent );
		object.accept( this );
        pop_parent( );
	}

	template <class T>
	void visit_map( TiXmlElement& parent, T& list ) {
        push_parent( parent );
		for ( T::iterator it=list.begin();
			  it != list.end(); it++ )
			it->second.accept( this );
        pop_parent( );
	}

	template <class T>
	void visit_ptr_array( TiXmlElement &container, T& list ) {
        push_parent( container );
		for ( T::iterator it=list.begin();
			  it != list.end(); it++ )
			(*it)->accept( this );
        pop_parent( );
	}

	template <class T>
	void visit_array( TiXmlElement &container, T& list ) {
        push_parent( container );
		for ( T::iterator it=list.begin();
			  it != list.end(); it++ )
			(*it).accept( this );
        pop_parent( );
	}

protected:
    void push_parent( TiXmlElement& parent ) {
        m_parents.push_back( &parent );
    }

    void pop_parent() {
        m_parents.pop_back();
    }

    TiXmlElement& getParent() {
        return *(*m_parents.rbegin());
    }
};
