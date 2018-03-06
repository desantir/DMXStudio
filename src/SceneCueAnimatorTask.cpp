/* 
Copyright (C) 2017 Robert DeSantis
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

#include "SceneCueAnimator.h"
#include "AnimationEngine.h"
#include "SceneCueAnimatorTask.h"

// ----------------------------------------------------------------------------
//
SceneCueAnimatorTask::SceneCueAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    AnimationTask( engine, animation_uid, actors, owner_uid ),
    m_cue_index( 0 ),
    m_fan_cues( false )
{
}

// ----------------------------------------------------------------------------
//
SceneCueAnimatorTask::~SceneCueAnimatorTask(void)
{
}

// ----------------------------------------------------------------------------
//
void SceneCueAnimatorTask::setupAnimation( AnimationDefinition* definition, DWORD time_ms )
{
    SceneCueAnimator* config = dynamic_cast<SceneCueAnimator *>( definition );

    m_tracking = config->isTracking();
    m_cues = config->getCues();
    m_fan_cues = config->getGroupSize() > 0;

    size_t group_index = 0;

    // Setup fixture groups
	for ( SceneActor& actor : getActors() ) {
        // Explode fixure groups (at least for version 0)
        for ( Fixture *pf : resolveActorFixtures( &actor ) ) {
            if ( group_index == 0 )                    // New group
                m_cue_groups.emplace_back();

            // Add channels to entry
            PaletteEntry entry;
            for ( channel_address channel=0; channel < pf->getNumChannels(); channel++ )
                entry.addValue( channel, actor.getFinalChannelValue( pf->getUID(), channel ) );

            // Update map at back of cue group list
            m_cue_groups.back()[pf->getUID()] = entry;

            if ( ++group_index == config->getGroupSize() )
                group_index = 0;
        }
    }
}

// ----------------------------------------------------------------------------
//
bool SceneCueAnimatorTask::sliceAnimation( DWORD time_ms )
{
    SignalState state = tick( time_ms );

    if ( state != SIGNAL_NEW_LEVEL )
        return false;

    if ( m_cues.size() == 0 )
        return false;

    size_t index = m_cue_index;

    for ( PaletteEntryMap& group : m_cue_groups ) {
        applyCue( m_cues[index], group, index == 0 );

        if ( m_fan_cues )
            if ( ++index >= m_cues.size() )
                index = 0;
    }

    // Increase cue index for next pass and wrap if needed
    if ( ++m_cue_index >= m_cues.size() )
        m_cue_index = 0;

    return true;
}

// ----------------------------------------------------------------------------
//
void SceneCueAnimatorTask::applyCue( UIDArray cues, PaletteEntryMap& group, bool reset_values ) {

    for ( PaletteEntryMap::value_type& fixture_entry : group ) {
        Fixture *pf = getFixture( fixture_entry.first );
        STUDIO_ASSERT( pf != NULL, "Unexpected null actor" );
        
        PaletteEntry& channel_values = m_current_cue_channels[ pf->getUID() ];

        // If not tracking, restore channel values changed on last cue
        if ( !m_tracking || reset_values ) {
            for ( PaletteValues::value_type& value : channel_values.getValues() ) {
				channel_address channel = value.first;
                loadChannel( pf, channel, fixture_entry.second.getValue( channel ) );
            }

            channel_values.clear();
        }
        else {
            // If we are tracking, then m_current_cue_channels contains the cumulative set of changed channels
        }

        // Apply all cue in this set (if any)
        for ( UID palette_id : cues ) {
            Palette* palette = getPalette( palette_id );
            if ( palette == NULL )
                continue;

            PaletteEntry* entry = palette->getEntry( pf );
            if ( entry == NULL )
                continue;

            PaletteValues& values = entry->getValues();

            if ( entry->getAddressing() == EntryAddressing::BY_CHANNEL ) {
                for ( PaletteValues::value_type value : values ) {
                    if ( value.first < pf->getNumChannels() )
                        channel_values.addValue( value.first, value.second );
                }
            }
            else {
                for ( size_t ch=0; ch < pf->getNumChannels(); ch++ ) {
                    Channel* channel = pf->getChannel( ch );
                    PaletteValues::iterator it = values.find( channel->getType() );
                    if ( it != values.end() )
                        channel_values.addValue( ch, it->second );
                }
            }
        }

        // Write new channel values to the DMX packet
        for ( PaletteValues::value_type& value : channel_values.getValues() )
            loadChannel( pf, value.first, value.second );
    }
}

// ----------------------------------------------------------------------------
//
bool SceneCueAnimatorTask::restartAnimation( DWORD time_ms ) {
    return false;
}

// ----------------------------------------------------------------------------
//
void SceneCueAnimatorTask::stopAnimation()
{
}
