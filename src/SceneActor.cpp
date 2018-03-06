/* 
Copyright (C) 2011,2012 Robert DeSantis
 
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

#include "DMXStudio.h"
#include "SceneActor.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
SceneActor::SceneActor( Fixture *pf ) :
    m_uid( pf->getUID() ),
    m_group( false)
{
    m_channel_values.setNumChannels( pf->getNumChannels() );

    // Copy channel defaults
    for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
        m_channel_values.set( channel, pf->getChannel( channel )->getDefaultValue() );
    }
}

// ----------------------------------------------------------------------------
//
SceneActor::SceneActor( Venue* venue, FixtureGroup *fg ) :
    m_uid( fg->getUID() ),
    m_group( true )
{
    reset();

    if ( fg->getNumChannelValues() > 0 ) {
		channel_value channel_values[DMX_PACKET_SIZE];

        fg->getChannelValues( channel_values );
        m_channel_values.setAll( fg->getNumChannelValues(), channel_values );
    }
    else {
        Fixture* pf = venue->getGroupRepresentative( fg->getUID() );
        if ( pf != NULL ) {         // Make sure this is not an empty group
            m_channel_values.setNumChannels( pf->getNumChannels() );

            // Copy channel defaults
            for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                m_channel_values.set( channel, pf->getChannel( channel )->getDefaultValue() );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
SceneActor::~SceneActor(void)
{
}
