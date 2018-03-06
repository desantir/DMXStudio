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

#pragma once

#include "stdafx.h"
#include "IVisitor.h"
#include "Fixture.h"
#include "FixtureGroup.h"
#include "Palette.h"

typedef std::map<UID, ChannelValues> ActorChannelValues;

class SceneActor
{
    friend class VenueWriter;
    friend class VenueReader;

    UID			            m_uid;						    // Fixture/group UID
    bool                    m_group;                        // Actor is a group
    UIDArray                m_palette_references;           // Palettes to use for all actors
    ChannelValues           m_channel_values;               // Base channel values (before palettes) 
    ActorChannelValues      m_resolved_values;              // Resolved values for all actors (computed with palette values)

public:
    SceneActor(void) :
        m_uid(0),
        m_group(false)
    { 
    }

    SceneActor( Fixture * );
    SceneActor( Venue* venue, FixtureGroup * );
    ~SceneActor(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    inline UID getActorUID() const {
        return m_uid;
    }

    inline bool isGroup() const {
        return m_group;
    }

    inline size_t getNumChannels() const {
        return m_channel_values.getNumChannels();
    }

    // Return the base channel value
    inline channel_value getBaseChannelValue( channel_address channel ) const {
        if ( channel >= getNumChannels() ) {
            // DMXStudio::log_warning( "Channel out of range - base value [UID=%lu]", m_uid );
            return 0;
        }
        return m_channel_values[ channel ];
    }

    void setFinalChannelValues( UID fixture_id,  ChannelValues& values ) {
        m_resolved_values[ fixture_id ] = values;
    }

    // Return the palettized channel value
    inline channel_value getFinalChannelValue( UID fixture_id, channel_address channel ) {
        if ( channel >= getNumChannels() ) {
            // DMXStudio::log_warning( "Channel out of range - final value [UID=%lu]", m_uid );
            return 0;
        }

        ActorChannelValues::iterator it = m_resolved_values.find( fixture_id );
        STUDIO_ASSERT( it != m_resolved_values.end(), "No resolved value for channel" );
        return it->second[ channel ];
    }

    // Return the non-palette channel values
    inline ChannelValues getBaseChannelValues( ) const {
        return m_channel_values;
    }

    // Set the base non-palette channel values
    inline void setBaseChannelValues( size_t channels, channel_value* values ) {
        reset();
        m_channel_values.setAll( channels, values );
    }

    // Set the base non-palette channel values
    inline void setBaseChannelValues( ChannelValues& new_values ) {
        reset();
        m_channel_values = new_values;
    }

    // Set the base non-palette channel value
    inline void setBaseChannelValue( channel_address channel, channel_value value ) {
        m_channel_values.set( channel, value );
        m_resolved_values.clear();
    }

    inline UIDArray getPaletteReferences() {
        return m_palette_references;
    }

    inline void setPaletteReferences( UIDArray& references ) {
        m_palette_references = references;
    }

    inline bool hasPaletteReferences() const {
        return m_palette_references.size() > 0;
    }

    inline bool hasPaletteReference( UID palette_id ) const {
        for ( UID uid : m_palette_references )
            if ( uid == palette_id )
                return true;
        return false;
    }

    inline void removeAllPaletteReferences() {
        m_palette_references.clear();
    }

    inline void addPaletteReference( UID palette_uid ) {
        if ( !hasPaletteReference( palette_uid ) )
            m_palette_references.push_back( palette_uid );
    }

    inline bool removePaletteReference( UID palette_id ) {
        for ( UIDArray::iterator it=m_palette_references.begin(); it !=m_palette_references.end(); ++it )
            if ( (*it) == palette_id ) {
                m_palette_references.erase( it );
                return true;
            }

        return false;
    }

    void reset( void ) {
        m_channel_values.clearAllValues();
        m_resolved_values.clear();
    }
};

typedef std::map< UID, SceneActor > ActorMap;
typedef std::map< UID, SceneActor * > ActorPtrMap;
typedef std::vector< SceneActor > ActorList;		
typedef std::vector< SceneActor * > ActorPtrArray;	
