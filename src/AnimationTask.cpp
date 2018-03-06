/* 
Copyright (C) 2016 Robert DeSantis
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

#include "AnimationTask.h"
#include "AnimationDefinition.h"
#include "AnimationEngine.h"

// ----------------------------------------------------------------------------
//
void AnimationTask::init( AnimationDefinition* animation, DWORD time_ms, channel_value* dmx_packet )
{
	m_channel_updates.clear();

    m_signal_processor = new AnimationSignalProcessor( animation->signal(), m_engine );

    setupAnimation( animation, time_ms );

	applyChannelUpdates( dmx_packet );
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::slice( DWORD time_ms, channel_value* dmx_packet )
{
	m_channel_updates.clear();

	bool changed = sliceAnimation( time_ms );

	applyChannelUpdates( dmx_packet );

	return changed;
}

// ----------------------------------------------------------------------------
//
bool AnimationTask::restart( DWORD time_ms, channel_value* dmx_packet)
{
	m_channel_updates.clear();

	bool changed = restartAnimation( time_ms );

	applyChannelUpdates( dmx_packet );

	return changed;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::applyChannelUpdates( channel_value* dmx_packet )
{
	for ( ChannelUpdate& u : m_channel_updates )
		dmx_packet[ u.m_pf->getMultiPacketAddress( u.m_channel ) ] = u.m_value;
}

// ----------------------------------------------------------------------------
//
void AnimationTask::setupFixtureFaders( FaderFixtureArray& faders )
{
    // For each fixture, find red, green, blue, white channels
    for ( SceneActor& actor : getActors() ) {
        // Setup state so that the slice animation does not need to gather additional info
        for ( Fixture *pf : resolveActorFixtures( &actor ) ) {
            if ( pf->getNumPixels() == 0 )
                continue;

            PixelArray* pixels = pf->getPixels();
            RGBWA initial_color;

            Pixel& pixel = pixels->at(0);

            if ( pixel.hasRed() )
                initial_color.red( actor.getFinalChannelValue( pf->getUID(), pixel.red() ) );
            if ( pixel.hasGreen() )
                initial_color.green( actor.getFinalChannelValue( pf->getUID(), pixel.green() ) );
            if ( pixel.hasBlue() )
                initial_color.blue( actor.getFinalChannelValue( pf->getUID(), pixel.blue() ) );
            if ( pixel.hasWhite() )
                initial_color.white( actor.getFinalChannelValue( pf->getUID(), pixel.white() ) );
            if ( pixel.hasAmber() )
                initial_color.amber( actor.getFinalChannelValue( pf->getUID(), pixel.amber() ) );
            if ( pixel.hasUV() )
                initial_color.uv( actor.getFinalChannelValue( pf->getUID(), pixel.uv() ) );

            faders.push_back( FaderFixture( pf, pixels, initial_color ) );
        }
    }
}

// ----------------------------------------------------------------------------
//
void AnimationTask::loadFaderColorChannels( FaderFixture& sfixture, RGBWA& rgbwa )
{
    for ( Pixel& pixel : *sfixture.m_pixels ) {
        if ( pixel.hasRed() )
            loadChannel( sfixture.m_pf, pixel.red(), rgbwa.red() );
        if ( pixel.hasGreen() )
            loadChannel( sfixture.m_pf, pixel.green(), rgbwa.green() );
        if ( pixel.hasBlue() )
            loadChannel( sfixture.m_pf, pixel.blue(), rgbwa.blue() );
        if ( pixel.hasWhite() )
            loadChannel( sfixture.m_pf, pixel.white(), rgbwa.white() );
        if ( pixel.hasAmber() )
            loadChannel( sfixture.m_pf, pixel.amber(), rgbwa.amber() );
        if ( pixel.hasUV() )
            loadChannel( sfixture.m_pf, pixel.uv(), rgbwa.uv() );
    }
}