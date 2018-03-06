/* 
Copyright (C) 2011-2017 Robert DeSantis
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

#include "VenueWriter.h"

// ----------------------------------------------------------------------------
//
VenueWriter::VenueWriter(void)
{
}

// ----------------------------------------------------------------------------
//
VenueWriter::~VenueWriter(void)
{
}

// ----------------------------------------------------------------------------
//
void VenueWriter::writeToFile( Venue* venue, LPCSTR output_file )
{
    TiXmlDocument doc;
    doc.InsertEndChild( TiXmlComment( "DMX Studio Venue definition" ) );
    
    push_parent( (TiXmlElement&)doc );
    venue->accept( this );
    pop_parent();

    // doc.Print();

    if ( !doc.SaveFile( output_file ) )
        throw StudioException( "Error writing to '%s'\n", output_file );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::writeToString( Venue* venue, CString& xml_output )
{
    TiXmlDocument doc;
    doc.InsertEndChild( TiXmlComment( "DMX Studio Venue definition" ) );
    
    push_parent( (TiXmlElement&)doc );
    venue->accept( this );
    pop_parent();

    TiXmlPrinter printer;
    doc.Accept( &printer );
    xml_output = printer.CStr();
}

// ----------------------------------------------------------------------------
//
void VenueWriter::writeDObject( TiXmlElement& element, DObject* dobject, LPCSTR number_name )
{
    add_attribute( element, "uid", dobject->m_uid );
    add_attribute( element, number_name, dobject->m_number );
    add_attribute( element, "created", dobject->m_created );
    add_text_element( element, "name", dobject->m_name );
    add_text_element( element, "description", dobject->m_description );

    if ( dobject->m_acts.size() > 0 ) {
        TiXmlElement acts( "acts" );
        for ( ActNumber act_number : dobject->m_acts ) {
            TiXmlElement uidElement( "act" );
            add_attribute( uidElement, "number", act_number );
            acts.InsertEndChild( uidElement );
        }
        element.InsertEndChild( acts );
    }
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Venue* venue ) {
    TiXmlElement venueElement( "venue" );

    add_attribute( venueElement, "version", "0.2" );
    add_attribute( venueElement, "next_uid", venue->m_uid_pool );
    add_attribute( venueElement, "current_scene", venue->m_current_scene );
    add_attribute( venueElement, "track_fixtures", venue->m_track_fixtures );

    add_text_element( venueElement, "name", venue->m_name );
    add_text_element( venueElement, "description", venue->m_description );

    // Add DMX universes
    TiXmlElement dmx_universes( "dmx_universes" );
    visit_ptr_array<UniversePtrArray>( dmx_universes, venue->getUniverses() );
    venueElement.InsertEndChild( dmx_universes );

    // Add dimmer/whiteout
    TiXmlElement dimmer( "dimmer" );
    add_attribute( dimmer, "master_dimmer", (int)venue->m_master_dimmer );
    add_attribute( dimmer, "auto_blackout", venue->m_auto_backout_ms );
    add_attribute( dimmer, "whiteout_strobe_ms", venue->m_whiteout_strobe_ms );
    add_attribute( dimmer, "whiteout_fade_ms", venue->m_whiteout_fade_ms );
    add_attribute( dimmer, "whiteout_color", venue->m_whiteout_color );
    add_attribute( dimmer, "whiteout_effect", venue->m_whiteout_effect );
    venueElement.InsertEndChild( dimmer );
    
    // Add audio
    TiXmlElement audio( "audio" );
    add_attribute( audio, "scale", venue->m_audio_boost );
    add_attribute( audio, "floor", venue->m_audio_boost_floor );
    add_attribute( audio, "sample_size", venue->m_audio_sample_size );
    add_text_element( audio, "capture_device",venue-> m_audio_capture_device );
    venueElement.InsertEndChild( audio );

    // Add venue layout
    if ( venue->m_venue_layout.GetLength() > 0 )
        add_cdata_element( venueElement, "venue_layout", venue-> m_venue_layout );

    // Add fixtures
    TiXmlElement fixtures( "fixtures" );
    visit_map<FixtureMap>( fixtures, venue->m_fixtures );
    venueElement.InsertEndChild( fixtures );

    // Add scenes
    TiXmlElement scenes( "scenes" );
    visit_map<SceneMap>( scenes, venue->m_scenes );
    venueElement.InsertEndChild( scenes );

    // Add fixture groups
    TiXmlElement groups( "fixture_groups" );
    visit_map<FixtureGroupMap>( groups, venue->m_fixtureGroups );
    venueElement.InsertEndChild( groups );

    // Add chases
    TiXmlElement chases( "chases" );
    visit_map<ChaseMap>( chases, venue->m_chases );
    venueElement.InsertEndChild( chases );

    // Add animations
    TiXmlElement animations( "animations" );
    add_attribute( animations, "animation_speed", venue->m_animation_speed );
    visit_ptr_map<AnimationMap>( animations, venue->m_animations );
    venueElement.InsertEndChild( animations );

    // Add music mappings
    TiXmlElement music_scenes( "music_scenes" );
    add_attribute( music_scenes, "enabled", venue->m_music_scene_select_enabled );
    visit_map<MusicSceneSelectMap>( music_scenes, venue->m_music_scene_select_map );
    venueElement.InsertEndChild( music_scenes );

    // Add palettes
    TiXmlElement palettes( "palettes" );
    visit_map<PaletteMap>( palettes, venue->m_palettes );
    venueElement.InsertEndChild( palettes );

    getParent().InsertEndChild( venueElement );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Universe* universe ) 
{
    TiXmlElement universe_element( "universe" );
    add_attribute( universe_element, "id", universe->getId() );
    add_attribute(universe_element, "type", universe->getType() );
    add_attribute( universe_element, "minimum_delay_ms", universe->getDmxMinimumDelayMS() );
    add_attribute( universe_element, "packet_delay_ms", universe->getDmxPacketDelayMS() );
    add_text_element( universe_element, "interface", universe->getInterface() );
    add_text_element( universe_element, "dmx_config", universe->getDmxConfig() );

    getParent().InsertEndChild( universe_element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Fixture* fixture ) 
{
    TiXmlElement element( "fixture" );

    writeDObject( element, fixture, "fixture_number" );

    add_attribute( element, "fuid", fixture->m_fuid );
    add_attribute( element, "universe", (int)fixture->m_universe );
    add_attribute( element, "dmx_address", (int)fixture->m_address );
    add_attribute( element, "allow_dimming", fixture->m_allow_master_dimming );
    add_attribute( element, "allow_whiteout", fixture->m_allow_whiteout );
	add_attribute( element, "grid_x", fixture->m_grid_position.m_x );
	add_attribute( element, "grid_y", fixture->m_grid_position.m_y );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Scene* scene )
{
    TiXmlElement element( "scene" );

    writeDObject( element, scene, "scene_number" );

    add_attribute( element, "bpm_rating", scene->m_bpm_rating );

    // Write all scene actors
    TiXmlElement actors( "actors" );
    visit_map<ActorMap>( actors, scene->m_actors );
    element.InsertEndChild( actors );

    if ( scene->m_animations.size() > 0 ) {
        TiXmlElement animations( "animation_refs" );
        visit_array<AnimationReferenceArray>( animations, scene->m_animations );
        element.InsertEndChild( animations );
    }

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneActor* actor )
{
    TiXmlElement element( "actor" );

    add_attribute( element, "fixure_uid", actor->m_uid );
    add_attribute( element, "group", actor->m_group );

    TiXmlElement channels( "channels" );
    for ( size_t channel=0; channel < actor->getNumChannels(); channel++ ) {
        TiXmlElement channelElement( "channel" );

        add_attribute( channelElement, "number", (int)channel );
        add_attribute( channelElement, "value", (int)actor->m_channel_values[channel] );

        channels.InsertEndChild( channelElement );
    }
    element.InsertEndChild( channels );

    if ( actor->m_palette_references.size() > 0 ) {
        add_uid_element<UIDArray>( element, "palette_refs", actor->m_palette_references );
    }

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Chase* chase )
{
    TiXmlElement chase_element( "chase" );

    writeDObject( chase_element, chase, "chase_number" );

    add_attribute( chase_element, "delay_ms", chase->m_delay_ms );
    add_attribute( chase_element, "fade_ms", chase->m_fade_ms );
    add_attribute( chase_element, "repeat", chase->m_repeat );
    add_attribute( chase_element, "step_trigger", chase->m_step_trigger );

    TiXmlElement steps( "chase_steps" );
    visit_array<ChaseStepArray>( steps, chase->m_chase_steps );
    chase_element.InsertEndChild( steps );

    getParent().InsertEndChild( chase_element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( FixtureGroup* fixture_group )
{
    TiXmlElement element( "fixture_group" );

    writeDObject( element, fixture_group, "group_number" );

    add_uid_element<UIDSet>( element, "pfuids", fixture_group->m_fixtures );
	add_attribute(element, "grid_x", fixture_group->m_grid_position.m_x );
	add_attribute(element, "grid_y", fixture_group->m_grid_position.m_y );

    TiXmlElement channels( "channels" );

    for ( channel_address channel=0; channel < fixture_group->m_channels; channel++ ) {
        TiXmlElement channelElement( "channel" );

        add_attribute( channelElement, "number", (int)channel );
        add_attribute( channelElement, "value", (int)fixture_group->m_channel_values[channel] );

        channels.InsertEndChild( channelElement );
    }

    element.InsertEndChild( channels );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( MusicSceneSelector* music_scene_selection )
{
    TiXmlElement element( "music_mapping" );

    add_attribute( element, "uid", music_scene_selection->m_selection_uid );
    add_attribute( element, "type", music_scene_selection->m_selection_type );
    add_text_element( element, "track_link", music_scene_selection->m_track_link );
    add_text_element( element, "track_full_name", music_scene_selection->m_track_full_name );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( AnimationReference* animation ) 
{
    TiXmlElement element( "animation" );

    add_attribute( element, "uid", animation->m_animation_uid );
    add_uid_element<UIDArray>( element, "pfuids", animation->m_actors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::writeAbstractAnimation( TiXmlElement& element, AnimationDefinition* animation )
{
    writeDObject( element, animation, "animation_number" );

    add_attribute( element, "shared", animation->isShared() );
    add_attribute( element, "class", animation->getClassName() );
    add_attribute( element, "reference_fixture", animation->m_reference_fixture );

    visit_object<AnimationSignal>( element, animation->m_signal );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneStrobeAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );
	writeStrobeTime( element, animation->getStrobeTime() );
    add_attribute( element, "strobe_neg_color", animation->m_strobe_neg_color );
	add_attribute( element, "strobe_color", animation->m_strobe_color );
	add_attribute( element, "strobe_percent", animation->m_strobe_percent );
	add_attribute( element, "strobe_type", animation->m_strobe_type );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::writeStrobeTime( TiXmlElement& element, StrobeTime& strobe_time )
{
	add_attribute( element, "strobe_pos_ms", strobe_time.getOnMS() );
	add_attribute( element, "strobe_neg_ms", strobe_time.getOffMS() );
	add_attribute( element, "strobe_flashes", strobe_time.getFlashes() );
	add_attribute( element, "strobe_fade_in_ms", strobe_time.getFadeInMS() );
	add_attribute( element, "strobe_fade_out_ms", strobe_time.getFadeOutMS() );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ScenePatternDimmer* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "dimmer_pattern", (int)animation->m_dimmer_pattern );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneFixtureDimmer* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "dimmer_mode", (int)animation->m_dimmer_mode );
    add_attribute( element, "min_percent", (int)animation->m_min_percent );
    add_attribute( element, "max_percent", (int)animation->m_max_percent );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneMovementAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    visit_object<MovementAnimation>( element, animation->m_movement );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneColorFader* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );
	writeStrobeTime( element, animation->getStrobeTime() );
    add_attribute( element, "fader_effect", (unsigned)animation->m_fader_effect );
    add_attribute( element, "strobe_neg_color", animation->m_strobe_neg_color );

    if ( animation->m_custom_colors.size() )
        add_colors_element( element, "custom_colors", animation->m_custom_colors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneCueAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "tracking", animation->m_tracking );
    add_attribute( element, "group_size", animation->m_group_size );

    TiXmlElement cues( "cues" );
    for ( UIDArray& palette_references : animation->m_cues ) {
        TiXmlElement cueElement( "cue" );
        add_uid_element<UIDArray>( cueElement, "palette_refs", palette_references );
        cues.InsertEndChild( cueElement );
    }
    element.InsertEndChild( cues );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ScenePulse* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "pulse_effect", (unsigned)animation->m_pulse_effect );
    add_attribute( element, "pulse_color", animation->m_pulse_color );
    add_attribute( element, "pulse_ms", animation->m_pulse_ms );
    add_attribute( element, "pulse_fixture_count", animation->m_pulse_fixture_count );
    add_attribute( element, "select_random", animation->m_select_random );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ScenePixelAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "pixel_effect", animation->m_effect );
    add_attribute( element, "generations", animation->m_generations );
    add_attribute( element, "pixels", animation->m_num_pixels );
    add_attribute( element, "increment", animation->m_increment );
    add_attribute( element, "fade", animation->m_color_fade );
    add_attribute( element, "combine", animation->m_combine_fixtures );
    add_attribute( element, "pixel_off_color", animation->m_empty_color );

    if ( animation->m_custom_colors.size() )
        add_colors_element( element, "custom_colors", animation->m_custom_colors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneChannelAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    TiXmlElement channel_animations( "channel_animations" );
    visit_array<ChannelAnimationArray>( channel_animations, animation->m_channel_animations );
    element.InsertEndChild( channel_animations );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneChannelFilter* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "filter", animation->getFilter() );
    add_attribute( element, "step", animation->getStep() );
    add_attribute( element, "amplitude", animation->getAmplitude() );
    add_attribute( element, "offset", animation->m_offset );

    TiXmlElement channels( "channels" );
    write_value_list<ChannelList>( channels, "channel", animation->getChannels() );
    element.InsertEndChild( channels );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneSequence* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneSoundLevel* animation )
{
    TiXmlElement element( "animation" );

    writeAbstractAnimation( element, animation );

    add_attribute( element, "fade", animation->m_fade_what );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( AnimationSignal* signal )
{
    TiXmlElement element( "signal" );

    add_attribute( element, "trigger", signal->m_trigger );
    add_attribute( element, "source", signal->m_source );
    add_attribute( element, "timer_ms", signal->m_timer_ms );
    add_attribute( element, "off_ms", signal->m_off_ms );
    add_attribute( element, "level_floor", signal->m_level_floor );
    add_attribute( element, "level_ceil", signal->m_level_ceil );
    add_attribute( element, "level_invert", signal->m_level_invert );
    add_attribute( element, "level_scale", signal->m_level_scale  );
    add_attribute( element, "level_periods", signal->m_level_periods );
    add_attribute( element, "freq_low_hz", signal->m_freq_low_hz );
    add_attribute( element, "freq_high_hz", signal->m_freq_high_hz );
    add_attribute( element, "speed_adjust", signal->m_speed_adjust );
    add_attribute( element, "level_hold", signal->m_level_hold );
    add_attribute( element, "sensitivity", signal->m_sensitivity );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ChaseStep* chase_step )
{
    TiXmlElement step( "chase_step" );
    add_attribute( step, "scene_uid", chase_step->m_scene_uid );
    add_attribute( step, "delay_ms", (DWORD)chase_step->m_delay_ms );
    add_attribute( step, "load_method", (int)chase_step->m_method );

    getParent().InsertEndChild( step );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( MovementAnimation* movement )
{
    TiXmlElement element( "movement" );

    add_attribute( element, "type", movement->m_movement_type );
    add_attribute( element, "tilt_start_angle", movement->m_tilt_start );
    add_attribute( element, "tilt_end_angle", movement->m_tilt_end );
    add_attribute( element, "pan_start_angle", movement->m_pan_start );
    add_attribute( element, "pan_end_angle", movement->m_pan_end );
    add_attribute( element, "pan_increment", movement->m_pan_increment );
    add_attribute( element, "speed",movement-> m_speed );
    add_attribute( element, "home_wait_periods", movement->m_home_wait_periods );
    add_attribute( element, "dest_wait_periods", movement->m_dest_wait_periods );
    add_attribute( element, "group_size", movement->m_group_size );
    add_attribute( element, "positions", movement->m_positions );
    add_attribute( element, "alternate_groups", movement->m_alternate_groups );
    add_attribute( element, "blackout_return", movement->m_backout_home_return );
    add_attribute( element, "run_once", movement->m_run_once );
    add_attribute( element, "home_x", movement->m_home_x );
    add_attribute( element, "home_y", movement->m_home_y );
    add_attribute( element, "height", movement->m_height );
    add_attribute( element, "fixture_spacing", movement->m_fixture_spacing );
    add_attribute( element, "radius", movement->m_radius );
    add_attribute( element, "head_number",  movement->m_head_number );

    if ( movement->m_coordinates.size() > 0 ) {
        TiXmlElement coordinates_element( "coordinate_list" );

        for ( size_t index=0; index <  movement->m_coordinates.size(); index++ ) {
            TiXmlElement coordElement( "coordinate" );
            add_attribute( coordElement, "pan", movement->m_coordinates[index].m_pan );
            add_attribute( coordElement, "tilt", movement->m_coordinates[index].m_tilt );

            coordinates_element.InsertEndChild( coordElement );
        }

        element.InsertEndChild( coordinates_element );
    }
    
    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ChannelAnimation* channel_animation )
{
    TiXmlElement element( "channel_animation" );

    add_attribute( element, "channel", (int)channel_animation->m_channel );
    add_attribute( element, "style", (int)channel_animation->m_animation_style );

    if ( channel_animation->m_value_list.size() > 0 ) {
        TiXmlElement values( "channel_values" );
        int number = 0;
        for ( ChannelValueArray::iterator it=channel_animation->m_value_list.begin(); 
              it != channel_animation->m_value_list.end(); it++, number++ ) {
            TiXmlElement valueElement( "channel_value" );
            add_attribute( valueElement, "number", number );
            add_attribute( valueElement, "value", (int) (*it) );
            values.InsertEndChild( valueElement );
        }
        element.InsertEndChild( values );
    }

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( PaletteEntry* palette_entry ) {

    TiXmlElement element( "palette_entry" );

    add_attribute( element, "target_uid", palette_entry->getTargetUID() );
    add_attribute( element, "addressing", palette_entry->getAddressing() );

    if ( palette_entry->m_values.size() > 0 ) {
        TiXmlElement values( "channel_values" );

        for ( PaletteValues::iterator it=palette_entry->m_values.begin(); it != palette_entry->m_values.end(); it++ ) {
            TiXmlElement valueElement( "channel_value" );
            add_attribute( valueElement, "id", it->first );
            add_attribute( valueElement, "value", (unsigned)it->second );
            values.InsertEndChild( valueElement );
        }
        element.InsertEndChild( values );
    }

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Palette* palette ) {
    TiXmlElement element( "palette" );

    writeDObject( element, palette, "palette_number" );

    add_attribute( element, "type", palette->getType() );

    // Write color palette
    if ( palette->m_palette_colors.size() )
        add_colors_element( element, "palette_colors", palette->m_palette_colors );

    if ( palette->m_palette_weights.size() ) {
        TiXmlElement color_weights_element( "color_weights" );
        write_value_list( color_weights_element, "weight", palette->m_palette_weights );
        element.InsertEndChild( color_weights_element );
    }

    if ( palette->m_fixture_map.size() > 0 ) {
        TiXmlElement entries( "fixture_palettes" );
        visit_map<PaletteEntryMap>( entries, palette->m_fixture_map );
        element.InsertEndChild( entries );
    }

    if ( palette->m_fixture_definition_map.size() > 0 ) {
        TiXmlElement entries( "definition_palettes" );
        visit_map<PaletteEntryMap>( entries, palette->m_fixture_definition_map );
        element.InsertEndChild( entries );
    }

    if ( palette->m_global.hasValues() ) {
        TiXmlElement global( "default_palette" );
        visit_object<PaletteEntry>( global, palette->m_global );
        element.InsertEndChild( global );
    }

    getParent().InsertEndChild( element );
}