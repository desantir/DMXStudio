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


#include "VenueReader.h"
#include "Venue.h"
#include "SceneSequence.h"
#include "SceneChannelAnimator.h"
#include "ScenePatternDimmer.h"
#include "SceneColorFader.h"
#include "SceneMovementAnimator.h"
#include "SceneStrobeAnimator.h"
#include "SceneSoundLevel.h"
#include "ScenePixelAnimator.h"

// ----------------------------------------------------------------------------
//
VenueReader::VenueReader(void)
{
}

// ----------------------------------------------------------------------------
//
VenueReader::~VenueReader(void)
{
}

// ----------------------------------------------------------------------------
//
Venue* VenueReader::readFromFile( LPCSTR input_file )
{
    TiXmlDocument doc;
    if ( !doc.LoadFile( input_file ) )
        throw StudioException( "Error reading venue '%s' (%s)", input_file, doc.ErrorDesc() );

    return read( doc.FirstChildElement(), (Venue *)NULL );
}

// ----------------------------------------------------------------------------
//
Venue* VenueReader::readFromString( LPCSTR xml_data )
{
    TiXmlDocument doc;
    if ( !doc.Parse( xml_data ) )
        throw StudioException( "Error parsing venue '%s' (%s)", xml_data, doc.ErrorDesc() );

    return read( doc.FirstChildElement(), (Venue *)NULL );
}

// ----------------------------------------------------------------------------
//
Venue * VenueReader::read( TiXmlElement* self, Venue* venue ) {
    venue = new Venue();

    venue->m_scenes.clear();			// Kill the auto generated default scene

    venue->m_uid_pool = read_dword_attribute( self, "next_uid" );
    venue->m_current_scene = (UID)read_dword_attribute( self, "current_scene" );

    venue->m_name = read_text_element( self, "name" );
    venue->m_description = read_text_element( self, "description" );

    TiXmlElement *universe_container = self->FirstChildElement( "dmx_universes" );
    if ( universe_container ) {
        TiXmlElement *universe = universe_container->FirstChildElement( "universe" );
        if ( universe ) {
            venue->m_dmx_port = read_text_element( universe, "comm_port" );
            venue->m_dmx_packet_delay = read_unsigned_attribute( universe, "packet_maximum_delay", DEFAULT_PACKET_DELAY_MS );
            venue->m_dmx_packet_min_delay = read_unsigned_attribute( universe, "packet_minimum_delay", DEFAULT_MINIMUM_DELAY_MS );
        }
    }

    TiXmlElement *dimmer = self->FirstChildElement( "dimmer" );
    if ( dimmer ) {
        venue->m_master_dimmer = (BYTE)read_int_attribute( dimmer, "master_dimmer" );
        venue->m_auto_backout_ms = read_dword_attribute( dimmer, "auto_blackout" );
        venue->m_whiteout_strobe_ms = read_unsigned_attribute( dimmer, "whiteout_strobe", venue->getWhiteoutStrobeMS() );
        }

    TiXmlElement *audio = self->FirstChildElement( "audio" );
    if ( audio ) {
        venue->m_audio_capture_device = read_text_element( audio, "capture_device" );
        venue->m_audio_boost = (float)read_double_attribute( audio, "scale" );
        venue->m_audio_boost_floor = (float)read_double_attribute( audio, "floor" );
        venue->m_audio_sample_size = (UINT)read_unsigned_attribute( audio, "sample_size", 1024 );
    }

    venue->m_venue_layout = read_text_element( self, "venue_layout" );

    // Add all fixtures
    std::vector<Fixture *> fixtures = 
        read_xml_list<Fixture>( self->FirstChildElement( "fixtures" ), "fixture" );

    for ( std::vector<Fixture *>::iterator it=fixtures.begin(); it != fixtures.end(); ++it ) {
        venue->addFixture( *(*it) );
        delete (*it);
    }

    // Add scenes
    std::vector<Scene *> scenes = 
        read_xml_list<Scene>( self->FirstChildElement( "scenes" ), "scene" );

    for ( std::vector<Scene *>::iterator it=scenes.begin(); it != scenes.end(); ++it ) {
        venue->addScene( *(*it) );
        delete (*it);
    }

    // Add fixture groups
    std::vector<FixtureGroup *> fixture_groups = 
        read_xml_list<FixtureGroup>( self->FirstChildElement( "fixture_groups" ), "fixture_group" );

    for ( std::vector<FixtureGroup *>::iterator it=fixture_groups.begin(); it != fixture_groups.end(); ++it ) {
        venue->addFixtureGroup( *(*it) );
        delete (*it);
    }

    // Add chases
    std::vector<Chase *> chases = 
        read_xml_list<Chase>( self->FirstChildElement( "chases" ), "chase" );

    for ( std::vector<Chase *>::iterator it=chases.begin(); it != chases.end(); ++it ) {
        venue->addChase( *(*it) );
        delete (*it);
    }

    // Add music mappings
    TiXmlElement *music_scenes_element = self->FirstChildElement( "music_scenes" );
    if ( music_scenes_element ) {
        venue->m_music_scene_select_enabled = read_bool_attribute( music_scenes_element, "enabled" );
        std::vector<MusicSceneSelector *> music_scenes = 
            read_xml_list<MusicSceneSelector>( self->FirstChildElement( "music_scenes" ), "music_mapping" );

        for ( std::vector<MusicSceneSelector *>::iterator it=music_scenes.begin(); it != music_scenes.end(); ++it ) {
            venue->addMusicMapping( *(*it) );
            delete (*it);
        }
    }

    return venue;
}

// ----------------------------------------------------------------------------
//
void VenueReader::readDObject( TiXmlElement* self, DObject* dobject, LPCSTR number_name )
{
    dobject->m_uid = (UID)read_dword_attribute( self, "uid" );
    dobject->m_number = (SceneNumber)read_word_attribute( self, number_name );
    dobject->m_name = read_text_element( self, "name" );
    dobject->m_description = read_text_element( self, "description" );
}

// ----------------------------------------------------------------------------
//
Scene* VenueReader::read( TiXmlElement* self, Scene* scene )
{
    scene = new Scene();

    readDObject( self, scene, "scene_number" );

    std::vector<SceneActor *> actors = 
        read_xml_list<SceneActor>( self->FirstChildElement( "actors" ), "actor" );

    for ( std::vector<SceneActor *>::iterator it=actors.begin(); it != actors.end(); ++it ) {
        scene->addActor( *(*it) );
        delete (*it);
    }

    TiXmlElement* acts = self->FirstChildElement( "acts" );
    if ( acts ) {
        TiXmlElement* element = acts->FirstChildElement( "act" );
        while ( element ) {
            scene->m_acts.insert( read_unsigned_attribute( element, "number" ) ) ;
            element = element->NextSiblingElement();
        }
    }

    TiXmlElement* container = self->FirstChildElement( "animations" );
    if ( container ) {
        TiXmlElement* element = container->FirstChildElement( "animation" );
        while ( element ) {
            const char* class_name = read_text_attribute( element, "class" );
            AbstractAnimation* animation = NULL;

            if ( !strcmp( class_name, SceneSequence::className ) )
                animation = read( element, (SceneSequence*)NULL );
            else if ( !strcmp( class_name, SceneSoundLevel::className ) )
                animation = read( element, (SceneSoundLevel*)NULL );
            else if ( !strcmp( class_name, SceneChannelAnimator::className ) )
                animation = read( element, (SceneChannelAnimator*)NULL );
            else if ( !strcmp( class_name, ScenePatternDimmer::className ) )
                animation = read( element, (ScenePatternDimmer*)NULL );
            else if ( !strcmp( class_name, SceneColorFader::className ) )
                animation = read( element, (SceneColorFader*)NULL );
            else if ( !strcmp( class_name, SceneMovementAnimator::className ) )
                animation = read( element, (SceneMovementAnimator*)NULL );
            else if ( !strcmp( class_name, SceneStrobeAnimator::className ) )
                animation = read( element, (SceneStrobeAnimator*)NULL );
            else if ( !strcmp( class_name, ScenePixelAnimator::className ) )
                animation = read( element, (ScenePixelAnimator*)NULL );
            else
                STUDIO_ASSERT( false, "Unknown animation class '%s'", class_name );

            scene->addAnimation( animation );

            element = element->NextSiblingElement();
        }
    }

    return scene;
}

// ----------------------------------------------------------------------------
//
Chase* VenueReader::read( TiXmlElement* self, Chase* chase )
{
    chase = new Chase;

    readDObject( self, chase, "chase_number" );

    chase->m_delay_ms = (ULONG)read_dword_attribute( self, "delay_ms" );
    chase->m_fade_ms = (ULONG)read_dword_attribute( self, "fade_ms" );

    TiXmlElement* acts = self->FirstChildElement( "acts" );
    if ( acts ) {
        TiXmlElement* element = acts->FirstChildElement( "act" );
        while ( element ) {
            chase->m_acts.insert( read_unsigned_attribute( element, "number" ) ) ;
            element = element->NextSiblingElement();
        }
    }

    // Add chase steps
    std::vector<ChaseStep *> steps = 
        read_xml_list<ChaseStep>( self->FirstChildElement( "chase_steps" ), "chase_step" );

    for ( std::vector<ChaseStep *>::iterator it=steps.begin(); it != steps.end(); ++it ) {
        chase->m_chase_steps.push_back( *(*it) );
        delete (*it);
    }

    return chase;
}

// ----------------------------------------------------------------------------
//
ChaseStep* VenueReader::read( TiXmlElement* self, ChaseStep* chase_step )
{
    chase_step = new ChaseStep;

    chase_step->m_scene_uid = read_dword_attribute( self, "scene_uid" );
    chase_step->m_delay_ms = read_dword_attribute( self, "delay_ms"  );

    return chase_step;
}

// ----------------------------------------------------------------------------
//
FixtureGroup* VenueReader::read( TiXmlElement* self, FixtureGroup* fixture_group )
{
    fixture_group = new FixtureGroup();

    readDObject( self, fixture_group, "group_number" );

    TiXmlElement * fixture_container = self->FirstChildElement( "pfuids" );
    if ( fixture_container ) {
        TiXmlElement* element = fixture_container->FirstChildElement( "uid" );
        while ( element ) {
            fixture_group->m_fixtures.insert( read_dword_attribute( element, "value" ) );
            element = element->NextSiblingElement();
        }
    }

    TiXmlElement * container = self->FirstChildElement( "channels" );
    channel_t max_channel = 0;

    if ( container ) {
        TiXmlElement* element = container->FirstChildElement( "channel" );
        while ( element ) {
            channel_t channel = read_int_attribute( element, "number" );
            BYTE value = (BYTE)read_int_attribute( element, "value" );

            fixture_group->m_channel_values[ channel ] = value;
            if ( channel+1 > max_channel )
                max_channel = channel+1;

            element = element->NextSiblingElement();
        }
    }

    fixture_group->m_channels = (size_t)max_channel;

    return fixture_group;
}

// ----------------------------------------------------------------------------
//
MusicSceneSelector* VenueReader::read( TiXmlElement* self, MusicSceneSelector* music_scene_selection )
{
    music_scene_selection = new MusicSceneSelector();

    music_scene_selection->m_selection_uid = (UID)read_dword_attribute( self, "uid" );
    music_scene_selection->m_selection_type = (MusicSelectorType)read_dword_attribute( self, "type" );
    music_scene_selection->m_track_full_name = read_text_element( self, "track_full_name" );

    return music_scene_selection;
}

// ----------------------------------------------------------------------------
//
SceneActor* VenueReader::read( TiXmlElement* self, SceneActor* actor )
{
    actor = new SceneActor();

    actor->m_uid = (UID)read_dword_attribute( self, "fixure_uid" );
    actor->m_group = read_bool_attribute( self, "group", false );

    TiXmlElement * container = self->FirstChildElement( "channels" );
    channel_t max_channel = 0;

    if ( container ) {
        TiXmlElement* element = container->FirstChildElement( "channel" );
        while ( element ) {
            channel_t channel = read_int_attribute( element, "number" );
            BYTE value = (BYTE)read_int_attribute( element, "value" );

            actor->m_channel_values[ channel ] = value;
            if ( channel+1 > max_channel )
                max_channel = channel+1;

            element = element->NextSiblingElement();
        }
    }

    actor->m_channels = (size_t)max_channel;

    return actor;
}

// ----------------------------------------------------------------------------
//
Fixture* VenueReader::read( TiXmlElement* self, Fixture* fixture )
{
    fixture = new Fixture();

    readDObject( self, fixture, "fixture_number" );

    fixture->m_fuid = read_dword_attribute( self, "fuid" );
    fixture->m_universe = read_int_attribute( self, "universe" );
    fixture->m_address = read_int_attribute( self, "dmx_address" );
    
    TiXmlElement * container = self->FirstChildElement( "channel_map" );
    if ( container ) {
        TiXmlElement* element = container->FirstChildElement( "map" );
        while ( element ) {
            channel_t logical = read_int_attribute( element, "logical" );
            channel_t physical = (BYTE)read_int_attribute( element, "physical" );

            fixture->m_channel_map[logical] = physical;

            element = element->NextSiblingElement();
        }
    }

    return fixture;
}

// ----------------------------------------------------------------------------
//
SceneStrobeAnimator* VenueReader::read( TiXmlElement* self, SceneStrobeAnimator* animation )
{
    animation = new SceneStrobeAnimator();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );
    animation->m_strobe_neg_color = read_rgbw_attribute( self, "strobe_neg_color" );
    animation->m_strobe_pos_ms = read_unsigned_attribute( self, "strobe_pos_ms" );
    animation->m_strobe_neg_ms = read_unsigned_attribute( self, "strobe_neg_ms"  );

    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    return animation;
}

// ----------------------------------------------------------------------------
//
ScenePatternDimmer* VenueReader::read( TiXmlElement* self, ScenePatternDimmer* animation )
{
    animation = new ScenePatternDimmer();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );
    animation->m_dimmer_pattern = (DimmerPattern)read_dword_attribute( self, "dimmer_pattern" );

    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneMovementAnimator* VenueReader::read( TiXmlElement* self, SceneMovementAnimator* animation )
{
    animation = new SceneMovementAnimator();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );

    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    TiXmlElement* movement_element = self->FirstChildElement( "movement" );
    if ( movement_element ) {
        MovementAnimation* movement = read( movement_element, (MovementAnimation*)NULL );
        animation->m_movement = *movement;
        delete movement;
    }

    return animation;
}

// ----------------------------------------------------------------------------
//
MovementAnimation* VenueReader::read( TiXmlElement* self, MovementAnimation* movement )
{
    movement = new MovementAnimation();

    movement->m_movement_type = (MovementAnimationType)read_unsigned_attribute( self, "type" );
    movement->m_tilt_start = read_unsigned_attribute( self, "tilt_start_angle" );
    movement->m_tilt_end = read_unsigned_attribute( self, "tilt_end_angle" );
    movement->m_pan_start = read_unsigned_attribute( self,  "pan_start_angle" );
    movement->m_pan_end = read_unsigned_attribute( self, "pan_end_angle" );
    movement->m_pan_increment = read_unsigned_attribute( self, "pan_increment" );
    movement->m_speed = (BYTE)read_unsigned_attribute( self, "speed" );
    movement->m_home_wait_periods = read_unsigned_attribute( self, "home_wait_periods" );
    movement->m_dest_wait_periods = read_unsigned_attribute( self, "dest_wait_periods" );
    movement->m_group_size = read_unsigned_attribute( self, "group_size" );
    movement->m_positions = read_unsigned_attribute( self, "positions" );
    movement->m_alternate_groups = read_bool_attribute( self, "alternate_groups" );
    movement->m_backout_home_return = read_bool_attribute( self, "blackout_return" );
    movement->m_run_once = read_bool_attribute( self, "run_once" );
    movement->m_home_x = read_float_attribute( self, "home_x" );
    movement->m_home_y = read_float_attribute( self, "home_y" );
    movement->m_height = read_float_attribute( self, "height" );
    movement->m_fixture_spacing = read_float_attribute( self, "fixture_spacing" );
    movement->m_radius = read_float_attribute( self, "radius" );
    movement->m_head_number = read_unsigned_attribute( self, "head_number" );

    TiXmlElement* coordinates_element = self->FirstChildElement( "coordinate_list" );
    if ( coordinates_element ) {
        TiXmlElement* element = coordinates_element->FirstChildElement( "coordinate" );
        while ( element ) {
            UINT pan = read_unsigned_attribute( element, "pan" );
            UINT tilt = read_unsigned_attribute( element, "tilt" );

            movement->m_coordinates.push_back( FixtureCoordinate( pan, tilt ) );

            element = element->NextSiblingElement();
        }
    }

    return movement;
}

// ----------------------------------------------------------------------------
//
SceneColorFader* VenueReader::read( TiXmlElement* self, SceneColorFader* animation )
{
    animation = new SceneColorFader();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );
    animation->m_fader_effect = (FaderEffect)read_unsigned_attribute( self, "fader_effect", FaderEffect::FADER_EFFECT_ALL );
    animation->m_strobe_neg_color = read_rgbw_attribute( self, "strobe_neg_color" );
    animation->m_strobe_pos_ms = read_unsigned_attribute( self, "strobe_pos_ms" );
    animation->m_strobe_neg_ms = read_unsigned_attribute( self, "strobe_neg_ms"  );

    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    read_colors( self, "custom_colors", animation->m_custom_colors );

    return animation;
}

// ----------------------------------------------------------------------------
//
ScenePixelAnimator* VenueReader::read( TiXmlElement* self, ScenePixelAnimator* animation )
{
    animation = new ScenePixelAnimator();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );

    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );
    animation->m_effect = (PixelEffect)read_unsigned_attribute( self, "pixel_effect", 1 );
    animation->m_generations = read_unsigned_attribute( self, "generations", 1 );
    animation->m_num_pixels = read_unsigned_attribute( self, "pixels", 1 );
    animation->m_increment = read_unsigned_attribute( self, "increment", 1 );
    animation->m_color_fade = read_bool_attribute( self, "fade", 1 );
    animation->m_combine_fixtures = read_bool_attribute( self, "combine", 1 );
    animation->m_empty_color = read_rgbw_attribute( self, "pixel_off_color" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    read_colors( self, "custom_colors", animation->m_custom_colors );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimator* VenueReader::read( TiXmlElement* self, SceneChannelAnimator* animation )
{
    animation = new SceneChannelAnimator();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );
    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    std::vector<ChannelAnimation *> channel_animation = 
        read_xml_list<ChannelAnimation>( self->FirstChildElement( "channel_animations" ), "channel_animation" );

    for ( std::vector<ChannelAnimation *>::iterator it=channel_animation.begin(); 
          it != channel_animation.end(); ++it ) {
        animation->m_channel_animations.push_back( *(*it) );
        delete (*it);
    }

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneSequence* VenueReader::read( TiXmlElement* self, SceneSequence* animation )
{
    animation = new SceneSequence();
    
    animation->m_uid = (UID)read_dword_attribute( self, "uid" );
    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneSoundLevel* VenueReader::read( TiXmlElement* self, SceneSoundLevel* animation )
{
    animation = new SceneSoundLevel();

    animation->m_uid = (UID)read_dword_attribute( self, "uid" );
    animation->m_fade_what = (WORD)read_int_attribute( self, "fade" );
    animation->m_name = read_text_element( self, "name" );
    animation->m_description = read_text_element( self, "description" );

    TiXmlElement* signal_element = self->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    read_uids( self, "pfuids", animation->m_actors );

    return animation;
}

// ----------------------------------------------------------------------------
//
AnimationSignal* VenueReader::read( TiXmlElement* self, AnimationSignal *signal ) {

    signal = new AnimationSignal();

    signal->m_sample_rate_ms = (DWORD)read_dword_attribute( self, "sample_rate_ms" );
    signal->m_sample_decay_ms = (DWORD)read_dword_attribute( self, "sample_decay_ms" );
    signal->m_input_type = (AnimationSignalInput)read_int_attribute( self, "input_type" );
    signal->m_input_low = read_unsigned_attribute( self, "input_low" );
    signal->m_input_high = read_unsigned_attribute( self, "input_high" );
    signal->m_scale_factor = read_unsigned_attribute( self, "scale_factor" );
    signal->m_max_threshold = read_unsigned_attribute( self, "max_threshold" );
    signal->m_apply_to = (ApplySignal)read_word_attribute( self, "apply_to" );

    return signal;
}

// ----------------------------------------------------------------------------
//
ChannelAnimation* VenueReader::read( TiXmlElement* self, ChannelAnimation* channel_animation )
{
    channel_animation = new ChannelAnimation();

    channel_animation->m_actor_uid = (UID)read_dword_attribute( self, "actor_uid" );
    channel_animation->m_channel = (channel_t)read_word_attribute( self, "channel"  );
    channel_animation->m_animation_style = (ChannelAnimationStyle)read_int_attribute( self, "style" );

    TiXmlElement * values = self->FirstChildElement( "channel_values" );
    if ( values ) {
        TiXmlElement* element = values->FirstChildElement( "channel_value" );
        while ( element ) {
            BYTE value = (BYTE)read_int_attribute( element, "value" );
            channel_animation->m_value_list.push_back( value );
            element = element->NextSiblingElement();
        }
    }

    return channel_animation;
}
