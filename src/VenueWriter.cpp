/* 
Copyright (C) 2011-14 Robert DeSantis
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
    add_text_element( element, "name", dobject->m_name );
    add_text_element( element, "description", dobject->m_description );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Venue* venue ) {
    TiXmlElement venueElement( "venue" );

    add_attribute( venueElement, "version", "0.2" );
    add_attribute( venueElement, "next_uid", venue->m_uid_pool );
    add_attribute( venueElement, "current_scene", venue->m_current_scene );
    add_text_element( venueElement, "name", venue->m_name );
    add_text_element( venueElement, "description", venue->m_description );

    // Add DMX universes
    TiXmlElement dmx_universes( "dmx_universes" );
    TiXmlElement universe( "universe" );
    add_attribute( universe, "index", "0" );
    add_text_element( universe, "interface", "usb_dmx" );
    add_text_element( universe, "comm_port", venue->m_dmx_port );
    dmx_universes.InsertEndChild( universe );
    venueElement.InsertEndChild( dmx_universes );

    // Add dimmer/whiteout
    TiXmlElement dimmer( "dimmer" );
    add_attribute( dimmer, "master_dimmer", (int)venue->m_master_dimmer );
    add_attribute( dimmer, "auto_blackout", venue->m_auto_backout_ms );
    add_attribute( dimmer, "whiteout_strobe", venue->m_whiteout_strobe_ms );
    add_attribute( dimmer, "whiteout_color", venue->m_whiteout_color );
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

    // Add music mappings
    TiXmlElement music_scenes( "music_scenes" );
    add_attribute( music_scenes, "enabled", venue->m_music_scene_select_enabled );
    visit_map<MusicSceneSelectMap>( music_scenes, venue->m_music_scene_select_map );
    venueElement.InsertEndChild( music_scenes );

    getParent().InsertEndChild( venueElement );
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

    if ( fixture->m_channel_map.size() > 0 ) {
        TiXmlElement channel_map( "channel_map" );

        for ( channel_t channel=0; channel <  fixture->m_channel_map.size(); channel++ ) {
            TiXmlElement channelElement( "map" );
            add_attribute( channelElement, "logical", (int)channel );
            add_attribute( channelElement, "physical", (int)fixture->m_channel_map[channel] );

            channel_map.InsertEndChild( channelElement );
        }

        element.InsertEndChild( channel_map );
    }

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( Scene* scene )
{
    TiXmlElement element( "scene" );

    writeDObject( element, scene, "scene_number" );

    // Write all scene actors
    TiXmlElement actors( "actors" );
    visit_map<ActorMap>( actors, scene->m_actors );
    element.InsertEndChild( actors );

    TiXmlElement acts( "acts" );
    for ( ActNumber act_number : scene->m_acts ) {
        TiXmlElement uidElement( "act" );
        add_attribute( uidElement, "number", act_number );
        acts.InsertEndChild( uidElement );
    }
    element.InsertEndChild( acts );

    if ( scene->m_animations.size() > 0 ) {
        TiXmlElement animations( "animations" );
        visit_ptr_array<AnimationPtrArray>( animations, scene->m_animations );
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

    for ( channel_t channel=0; channel < actor->m_channels; channel++ ) {
        TiXmlElement channelElement( "channel" );

        add_attribute( channelElement, "number", (int)channel );
        add_attribute( channelElement, "value", (int)actor->m_channel_values[channel] );

        channels.InsertEndChild( channelElement );
    }

    element.InsertEndChild( channels );

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

    TiXmlElement steps( "chase_steps" );
    visit_array<ChaseStepArray>( steps, chase->m_chase_steps );
    chase_element.InsertEndChild( steps );

    TiXmlElement acts( "acts" );
    for ( ActNumber act_number : chase->m_acts ) {
        TiXmlElement uidElement( "act" );
        add_attribute( uidElement, "number", act_number );
        acts.InsertEndChild( uidElement );
    }
    chase_element.InsertEndChild( acts );

    getParent().InsertEndChild( chase_element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( FixtureGroup* fixture_group )
{
    TiXmlElement element( "fixture_group" );

    writeDObject( element, fixture_group, "group_number" );

    add_pfuids_element<UIDSet>( element, fixture_group->m_fixtures );

    TiXmlElement channels( "channels" );

    for ( channel_t channel=0; channel < fixture_group->m_channels; channel++ ) {
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
    add_text_element( element, "track_full_name", music_scene_selection->m_track_full_name );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneStrobeAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );
    add_attribute( element, "strobe_neg_color", animation->m_strobe_neg_color );
    add_attribute( element, "strobe_pos_ms", animation->m_strobe_pos_ms );
    add_attribute( element, "strobe_neg_ms", animation->m_strobe_neg_ms );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ScenePatternDimmer* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );
    add_attribute( element, "dimmer_pattern", (int)animation->m_dimmer_pattern );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneMovementAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    visit_object<MovementAnimation>( element, animation->m_movement );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneColorFader* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );
    add_attribute( element, "fader_effect", (unsigned)animation->m_fader_effect );
    add_attribute( element, "strobe_neg_color", animation->m_strobe_neg_color );
    add_attribute( element, "strobe_pos_ms", animation->m_strobe_pos_ms );
    add_attribute( element, "strobe_neg_ms", animation->m_strobe_neg_ms );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    if ( animation->m_custom_colors.size() )
        add_colors_element( element, "custom_colors", animation->m_custom_colors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ScenePixelAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );
    add_attribute( element, "pixel_effect", animation->m_effect );
    add_attribute( element, "generations", animation->m_generations );
    add_attribute( element, "pixels", animation->m_num_pixels );
    add_attribute( element, "increment", animation->m_increment );
    add_attribute( element, "fade", animation->m_color_fade );
    add_attribute( element, "combine", animation->m_combine_fixtures );
    add_attribute( element, "pixel_off_color", animation->m_empty_color );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    if ( animation->m_custom_colors.size() )
        add_colors_element( element, "custom_colors", animation->m_custom_colors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneChannelAnimator* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );

    visit_object<AnimationSignal>( element, animation->m_signal );

    TiXmlElement channel_animations( "channel_animations" );
    visit_array<ChannelAnimationArray>( channel_animations, animation->m_channel_animations );
    element.InsertEndChild( channel_animations );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneSequence* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( SceneSoundLevel* animation )
{
    TiXmlElement element( "animation" );

    writeDObject( element, animation, "number" );

    add_attribute( element, "class", animation->getClassName() );
    add_attribute( element, "fade", animation->m_fade_what );

    visit_object<AnimationSignal>( element, animation->m_signal );

    add_pfuids_element<UIDArray>( element, animation->m_actors );

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( AnimationSignal* signal )
{
    TiXmlElement element( "signal" );

    add_attribute( element, "sample_rate_ms", signal->m_sample_rate_ms );
    add_attribute( element, "sample_decay_ms", signal->m_sample_decay_ms );
    add_attribute( element, "input_type", signal->m_input_type );
    add_attribute( element, "scale_factor", signal->m_scale_factor );
    add_attribute( element, "max_threshold", signal->m_max_threshold );
    add_attribute( element, "apply_to", signal->m_apply_to );

    if ( signal->m_input_type != CAI_TIMER_ONLY &&
         signal->m_input_type != CAI_SOUND_LEVEL &&
         signal->m_input_type != CAI_AVG_SOUND_LEVEL ) {
        add_attribute( element, "input_low", signal->m_input_low );
        add_attribute( element, "input_high", signal->m_input_high );
    }

    getParent().InsertEndChild( element );
}

// ----------------------------------------------------------------------------
//
void VenueWriter::visit( ChaseStep* chase_step )
{
    TiXmlElement step( "chase_step" );
    add_attribute( step, "scene_uid", chase_step->m_scene_uid );
    add_attribute( step, "delay_ms", (DWORD)chase_step->m_delay_ms );
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

    add_attribute( element, "actor_uid", channel_animation->m_actor_uid );
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