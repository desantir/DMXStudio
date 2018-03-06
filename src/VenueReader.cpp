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
#include "SceneChannelFilter.h"
#include "ScenePulse.h"
#include "SceneCueAnimator.h"
#include "SceneFixtureDimmer.h"

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

    // FOLLOWING FIELDS FOR UPGRADE ONLY
    m_venue_temp = venue;
    m_anim_num_temp = 1;

    venue->m_scenes.clear();			// Kill the auto generated default scene

    venue->m_uid_pool = read_dword_attribute( self, "next_uid" );
    venue->m_current_scene = (UID)read_dword_attribute( self, "current_scene" );
    venue->m_track_fixtures = read_bool_attribute( self, "track_fixtures" );

    venue->m_name = read_text_element( self, "name" );
    venue->m_description = read_text_element( self, "description" );

    // Add all universes (up to the max, must be in correct order)
    std::vector<Universe *> universes = 
        read_xml_list<Universe>( self->FirstChildElement( "dmx_universes" ), "universe" );

    for ( std::vector<Universe *>::iterator it=universes.begin(); it != universes.end(); ++it ) {
        venue->addUniverse( (*it) );
    }

    TiXmlElement *dimmer = self->FirstChildElement( "dimmer" );
    if ( dimmer ) {
        venue->m_master_dimmer = (BYTE)read_int_attribute( dimmer, "master_dimmer" );
        venue->m_auto_backout_ms = read_dword_attribute( dimmer, "auto_blackout" );
        venue->m_whiteout_strobe_ms = read_unsigned_attribute( dimmer, "whiteout_strobe_ms", venue->getWhiteoutStrobeMS() );
        venue->m_whiteout_fade_ms = read_unsigned_attribute( dimmer, "whiteout_fade_ms", venue->getWhiteoutFadeMS() );
        venue->m_whiteout_color = read_rgbw_attribute( dimmer, "whiteout_color", RGBWA::WHITE );
        venue->m_whiteout_effect = (WhiteoutEffect)read_unsigned_attribute( dimmer, "whiteout_effect", WhiteoutEffect::WHITEOUT_EFFECT_SINGLE_COLOR );
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
        venue->m_fixtures[ (*it)->getUID() ] = *(*it);  // NOTE: Do not call venue addFixture
        delete (*it);
    }

    // Add scenes
    std::vector<Scene *> scenes = 
        read_xml_list<Scene>( self->FirstChildElement( "scenes" ), "scene" );

    for ( std::vector<Scene *>::iterator it=scenes.begin(); it != scenes.end(); ++it ) {
        venue->addScene( *(*it), (*it)->getNumber() == DEFAULT_SCENE_NUMBER);
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

    // Add animations
    TiXmlElement* container = self->FirstChildElement( "animations" );
    if ( container ) {
        venue->m_animation_speed = read_unsigned_attribute( dimmer, "animation_speed", 100 );

        TiXmlElement* element = container->FirstChildElement( "animation" );
        while ( element ) {
            venue->addAnimation( readAnimation( element ) );

            element = element->NextSiblingElement();
        }
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

    // Add palettes
    std::vector<Palette *> palettes = 
        read_xml_list<Palette>( self->FirstChildElement( "palettes" ), "palette" );

    for ( std::vector<Palette *>::iterator it=palettes.begin(); it != palettes.end(); ++it ) {
        if ( (*it)->getType() != PT_UNUSED )
            venue->m_palettes[ (*it)->getUID() ] = *(*it);
        delete (*it);
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
    dobject->m_created = read_uint64_attribute( self, "created" );

    TiXmlElement* acts = self->FirstChildElement( "acts" );
    if ( acts ) {
        TiXmlElement* act_element = acts->FirstChildElement( "act" );
        while ( act_element ) {
            dobject->m_acts.insert( read_unsigned_attribute( act_element, "number" ) ) ;
            act_element = act_element->NextSiblingElement();
        }
    }
}

// ----------------------------------------------------------------------------
//
Universe* VenueReader::read( TiXmlElement* self, Universe* universe )
{
    universe = new Universe();

    universe->m_id = (universe_t)read_unsigned_attribute( self, "id", 0 );
    universe->m_type = (UniverseType)read_unsigned_attribute( self, "type", OPEN_DMX );
    universe->m_dmx_packet_delay = read_unsigned_attribute( self, "packet_delay_ms", DEFAULT_PACKET_DELAY_MS );
    universe->m_dmx_packet_min_delay = read_unsigned_attribute( self, "minimum_delay_ms", DEFAULT_MINIMUM_DELAY_MS );
    universe->m_dmx_config = read_text_element( self, "dmx_config" );

    return universe;
}

// ----------------------------------------------------------------------------
//
Scene* VenueReader::read( TiXmlElement* self, Scene* scene )
{
    scene = new Scene();

    readDObject( self, scene, "scene_number" );

    scene->m_bpm_rating = (BPMRating)read_int_attribute( self, "bpm_rating", BPMRating::BPM_NO_RATING);

    std::vector<SceneActor *> actors = 
        read_xml_list<SceneActor>( self->FirstChildElement( "actors" ), "actor" );

    for ( std::vector<SceneActor *>::iterator it=actors.begin(); it != actors.end(); ++it ) {
        scene->addActor( *(*it) );
        delete (*it);
    }

    TiXmlElement* refs_container = self->FirstChildElement( "animation_refs" );
    if ( refs_container ) {
        TiXmlElement* element = refs_container->FirstChildElement( "animation" );
        while ( element ) {
            AnimationReference animation;
            animation.m_animation_uid = (UID)read_dword_attribute( element, "uid" );
            read_uids( element, "pfuids", animation.m_actors );
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
    chase->m_repeat = read_bool_attribute( self, "repeat", true );
    chase->m_step_trigger = (ChaseStepTrigger)read_unsigned_attribute( self, "step_trigger", ChaseStepTrigger::CT_TIMER );

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
    chase_step->m_method = (SceneLoadMethod)read_int_attribute( self, "load_method", SLM_LOAD );

    return chase_step;
}

// ----------------------------------------------------------------------------
//
FixtureGroup* VenueReader::read( TiXmlElement* self, FixtureGroup* fixture_group )
{
    fixture_group = new FixtureGroup();

    readDObject( self, fixture_group, "group_number" );
	fixture_group->m_grid_position.m_x = read_long_attribute( self, "grid_x" );
	fixture_group->m_grid_position.m_y = read_long_attribute( self, "grid_y" );

    TiXmlElement * fixture_container = self->FirstChildElement( "pfuids" );
    if ( fixture_container ) {
        TiXmlElement* element = fixture_container->FirstChildElement( "uid" );
        while ( element ) {
            fixture_group->m_fixtures.insert( read_dword_attribute( element, "value" ) );
            element = element->NextSiblingElement();
        }
    }

    TiXmlElement * container = self->FirstChildElement( "channels" );
	channel_address max_channel = 0;

    if ( container ) {
        TiXmlElement* element = container->FirstChildElement( "channel" );
        while ( element ) {
			channel_address channel = read_int_attribute( element, "number" );
            channel_value value = (BYTE)read_int_attribute( element, "value" );

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
    music_scene_selection->m_track_link  = read_text_element( self, "track_link" );
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

    if ( container ) {
        TiXmlElement* element = container->FirstChildElement( "channel" );

		channel_address max_channel = 0;
		channel_value channel_values[DMX_PACKET_SIZE];
        memset( channel_values, 0, sizeof(channel_values) );

        while ( element ) {
			channel_address channel = read_int_attribute( element, "number" );
			channel_value value = (channel_value)read_int_attribute( element, "value" );

            channel_values[ channel ] = value;
            if ( channel+1 > max_channel )
                max_channel = channel+1;

            element = element->NextSiblingElement();
        }

        actor->setBaseChannelValues( max_channel, channel_values );
    }

    read_uids( self, "palette_refs", actor->m_palette_references );

    return actor;
}

// ----------------------------------------------------------------------------
//
Fixture* VenueReader::read( TiXmlElement* self, Fixture* fixture )
{
    fixture = new Fixture();

    readDObject( self, fixture, "fixture_number" );

    fixture->m_fuid = read_dword_attribute( self, "fuid" );
    fixture->setUniverseId( read_int_attribute( self, "universe" ) );
    fixture->setAddress( read_int_attribute( self, "dmx_address" ) );		// MUST CALL
    fixture->m_allow_master_dimming = read_bool_attribute( self, "allow_dimming", true );
    fixture->m_allow_whiteout = read_bool_attribute( self, "allow_whiteout", true );
	fixture->m_grid_position.m_x = read_long_attribute( self, "grid_x" );
	fixture->m_grid_position.m_y = read_long_attribute( self, "grid_y" );
    
    return fixture;
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* VenueReader::readAnimation( TiXmlElement* element )
{
    const char* class_name = read_text_attribute( element, "class" );
    AnimationDefinition* animation = NULL;

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
    else if ( !strcmp( class_name, SceneChannelFilter::className ) )
        animation = read( element, (SceneChannelFilter*)NULL );
    else if ( !strcmp( class_name, ScenePulse::className ) )
        animation = read( element, (ScenePulse*)NULL );
    else if ( !strcmp( class_name, SceneCueAnimator::className ) )
        animation = read( element, (SceneCueAnimator*)NULL );
    else if ( !strcmp( class_name, SceneFixtureDimmer::className ) )
        animation = read( element, (SceneFixtureDimmer*)NULL );
    else
        STUDIO_ASSERT( false, "Unknown animation class '%s'", class_name );

    readDObject( element, animation, "animation_number" );

    animation->m_shared = read_bool_attribute( element, "shared", true );
    animation->m_reference_fixture = read_dword_attribute( element, "reference_fixture" );

    TiXmlElement* signal_element = element->FirstChildElement( "signal" );
    if ( signal_element ) {
        AnimationSignal* signal = read( signal_element, (AnimationSignal*)NULL );
        animation->m_signal = *signal;
        delete signal;
    }

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneStrobeAnimator* VenueReader::read( TiXmlElement* self, SceneStrobeAnimator* animation )
{
    animation = new SceneStrobeAnimator();

	animation->m_strobe_time = readStrobeTime( self );
    animation->m_strobe_neg_color = read_rgbw_attribute( self, "strobe_neg_color", RGBWA::BLACK );
	animation->m_strobe_color = read_rgbw_attribute( self, "strobe_color", RGBWA::WHITE );
	animation->m_strobe_percent = read_unsigned_attribute( self, "strobe_percent", 50 );
	animation->m_strobe_type = (StrobeType)read_unsigned_attribute( self, "strobe_type", STROBE_SIMULATED );

    return animation;
}

// ----------------------------------------------------------------------------
//
StrobeTime VenueReader::readStrobeTime( TiXmlElement* self )
{
	return StrobeTime( read_unsigned_attribute( self, "strobe_pos_ms" ), read_unsigned_attribute( self, "strobe_neg_ms"  ),
		read_unsigned_attribute( self, "strobe_fade_in_ms", 0 ), read_unsigned_attribute( self, "strobe_fade_out_ms", 0 ),
		read_unsigned_attribute(self, "strobe_flashes", 1 ) );
}

// ----------------------------------------------------------------------------
//
ScenePatternDimmer* VenueReader::read( TiXmlElement* self, ScenePatternDimmer* animation )
{
    animation = new ScenePatternDimmer();

    animation->m_dimmer_pattern = (DimmerPattern)read_dword_attribute( self, "dimmer_pattern" );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneFixtureDimmer* VenueReader::read( TiXmlElement* self, SceneFixtureDimmer* animation )
{
    animation = new SceneFixtureDimmer();

    animation->m_dimmer_mode = (DimmerMode)read_dword_attribute( self, "dimmer_mode" );
    animation->m_min_percent = read_unsigned_attribute( self, "min_percent" );
    animation->m_max_percent = read_unsigned_attribute( self, "max_percent" );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneMovementAnimator* VenueReader::read( TiXmlElement* self, SceneMovementAnimator* animation )
{
    animation = new SceneMovementAnimator();

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

            movement->m_coordinates.emplace_back( pan, tilt );

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

    animation->m_fader_effect = (FaderEffect)read_unsigned_attribute( self, "fader_effect", FaderEffect::FADER_EFFECT_ALL );
    animation->m_strobe_neg_color = read_rgbw_attribute( self, "strobe_neg_color" );
	animation->m_strobe_time = readStrobeTime( self );

    read_colors( self, "custom_colors", animation->m_custom_colors );

    return animation;
}

// ----------------------------------------------------------------------------
//
ScenePulse* VenueReader::read( TiXmlElement* self, ScenePulse* animation )
{
    animation = new ScenePulse();

    animation->m_pulse_effect = (PulseEffect)read_unsigned_attribute( self, "pulse_effect", PulseEffect::PULSE_EFFECT_STROBE );
    animation->m_pulse_color = read_rgbw_attribute( self, "pulse_color" );
    animation->m_pulse_ms = read_unsigned_attribute( self, "pulse_ms", 200 );
    animation->m_pulse_fixture_count = read_unsigned_attribute( self, "pulse_fixture_count", 1 );
    animation->m_select_random = read_bool_attribute( self, "select_random" );

    return animation;
}

// ----------------------------------------------------------------------------
//
ScenePixelAnimator* VenueReader::read( TiXmlElement* self, ScenePixelAnimator* animation )
{
    animation = new ScenePixelAnimator();

    animation->m_effect = (PixelEffect)read_unsigned_attribute( self, "pixel_effect", 1 );
    animation->m_generations = read_unsigned_attribute( self, "generations", 1 );
    animation->m_num_pixels = read_unsigned_attribute( self, "pixels", 1 );
    animation->m_increment = read_unsigned_attribute( self, "increment", 1 );
    animation->m_color_fade = read_bool_attribute( self, "fade", 1 );
    animation->m_combine_fixtures = read_bool_attribute( self, "combine", 1 );
    animation->m_empty_color = read_rgbw_attribute( self, "pixel_off_color" );

    read_colors( self, "custom_colors", animation->m_custom_colors );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimator* VenueReader::read( TiXmlElement* self, SceneChannelAnimator* animation )
{
    animation = new SceneChannelAnimator();

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
SceneChannelFilter* VenueReader::read( TiXmlElement* self, SceneChannelFilter* animation )
{
    animation = new SceneChannelFilter();

    animation->m_filter = (ChannelFilter)read_unsigned_attribute( self, "filter");
	channel_address temp_channel = (channel_address)read_unsigned_attribute( self, "channel" );
    animation->m_step = read_unsigned_attribute( self, "step" );
    animation->m_amplitude = read_unsigned_attribute( self, "amplitude" );
    animation->m_offset = read_int_attribute( self, "offset" );

    TiXmlElement* channels = self->FirstChildElement( "channels" );
    if ( channels )
        animation->m_channels = read_unsigned_value_list<ChannelList>( channels, "channel" );
    else
        animation->m_channels.push_back( temp_channel );

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneCueAnimator* VenueReader::read( TiXmlElement* self, SceneCueAnimator* animation )
{
    animation = new SceneCueAnimator();

    animation->m_tracking = read_bool_attribute( self, "tracking" );
    animation->m_group_size = read_unsigned_attribute( self, "group_size" );

    TiXmlElement* cues = self->FirstChildElement( "cues" );
    if ( cues ) {
        TiXmlElement* element = cues->FirstChildElement( "cue" );
        while ( element ) {
            UIDArray uids;
            read_uids( element, "palette_refs", uids );
            animation->getCues().push_back(uids);
            element = element->NextSiblingElement();
        }
    }

    return animation;
}

// ----------------------------------------------------------------------------
//
SceneSequence* VenueReader::read( TiXmlElement* self, SceneSequence* animation )
{
    animation = new SceneSequence();
    
    return animation;
}

// ----------------------------------------------------------------------------
//
SceneSoundLevel* VenueReader::read( TiXmlElement* self, SceneSoundLevel* animation )
{
    animation = new SceneSoundLevel();

    animation->m_fade_what = (WORD)read_int_attribute( self, "fade" );

    return animation;
}

// ----------------------------------------------------------------------------
//
AnimationSignal* VenueReader::read( TiXmlElement* self, AnimationSignal *signal ) {

    signal = new AnimationSignal();

    signal->m_trigger = (SignalTrigger)read_word_attribute( self, "trigger", SignalTrigger::TRIGGER_TIMER );
    signal->m_source = (SignalSource)read_word_attribute( self, "source", SignalSource::SOURCE_HIGH );
    signal->m_timer_ms = read_dword_attribute( self, "timer_ms", 500 );
    signal->m_off_ms = read_dword_attribute( self, "off_ms", 0 );
    signal->m_level_floor = read_unsigned_attribute( self, "level_floor", 0 );
    signal->m_level_ceil = read_unsigned_attribute( self, "level_ceil", MAXIMUM_LEVEL );
    signal->m_level_invert = read_bool_attribute( self, "level_invert", false );
    signal->m_level_scale = read_unsigned_attribute( self, "level_scale", 1 );
    signal->m_level_periods = read_unsigned_attribute( self, "level_periods", 10);
    signal->m_freq_low_hz = read_unsigned_attribute( self, "freq_low_hz", 0);
    signal->m_freq_high_hz = read_unsigned_attribute( self, "freq_high_hz", 0);
    signal->m_speed_adjust = (SpeedAdjust)read_word_attribute( self, "speed_adjust", SpeedAdjust::SPEED_NORMAL );
    signal->m_level_hold = read_unsigned_attribute( self, "level_hold", 0);
    signal->m_sensitivity = read_unsigned_attribute( self, "sensitivity", 0 );

    return signal;
}

// ----------------------------------------------------------------------------
//
ChannelAnimation* VenueReader::read( TiXmlElement* self, ChannelAnimation* channel_animation )
{
    channel_animation = new ChannelAnimation();

    channel_animation->m_channel = (channel_address)read_word_attribute( self, "channel"  );
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

// ----------------------------------------------------------------------------
//
PaletteEntry* VenueReader::read( TiXmlElement* self, PaletteEntry* palette_entry )
{
    palette_entry = new PaletteEntry();

    palette_entry->m_target_uid = read_dword_attribute( self, "target_uid" );
    palette_entry->m_addressing = (EntryAddressing)read_word_attribute( self, "addressing", EntryAddressing::BY_CHANNEL );

    TiXmlElement * values = self->FirstChildElement( "channel_values" );
    if ( values ) {
        TiXmlElement* element = values->FirstChildElement( "channel_value" );
        while ( element ) {
            unsigned id = read_unsigned_attribute( element, "id" );
            BYTE value = (BYTE)read_int_attribute( element, "value" );
            palette_entry->m_values[id] = value;
            element = element->NextSiblingElement();
        }
    }

    return palette_entry;
}

// ----------------------------------------------------------------------------
//
Palette* VenueReader::read( TiXmlElement* self, Palette* palette ) {
    palette = new Palette();

    readDObject( self, palette, "palette_number" );

    palette->m_type = (PaletteType)read_word_attribute( self, "type", PaletteType::PT_UNSPECIFIED );

    // Read color palette if available
    read_colors( self, "palette_colors", palette->m_palette_colors );

    // Read color weights if available
    TiXmlElement * color_weights_element = self->FirstChildElement( "color_weights" );
    if ( color_weights_element )
        palette->m_palette_weights = read_double_value_list<ColorWeights>( color_weights_element, "weight" );

    // Read fixture specific palettes
    std::vector<PaletteEntry *> fixture_entries = 
        read_xml_list<PaletteEntry>( self->FirstChildElement( "fixture_palettes" ), "palette_entry" );

    for ( std::vector<PaletteEntry *>::iterator it=fixture_entries.begin(); it != fixture_entries.end(); ++it ) {
        palette->m_fixture_map[ (*it)->getTargetUID() ] = *(*it);
        delete (*it);
    }

    // Read fixture definition specific palettes
    std::vector<PaletteEntry *> definition_entries = 
        read_xml_list<PaletteEntry>( self->FirstChildElement( "definition_palettes" ), "palette_entry" );

    for ( std::vector<PaletteEntry *>::iterator it=definition_entries.begin(); it != definition_entries.end(); ++it ) {
        palette->m_fixture_definition_map[ (*it)->getTargetUID() ] = *(*it);
        delete (*it);
    }

    TiXmlElement* default_entry = self->FirstChildElement( "default_palette" );
    if ( default_entry ) {
        TiXmlElement* palette_entry = default_entry->FirstChildElement( "palette_entry" );
        if ( palette_entry != NULL ) {
            PaletteEntry* entry = read( palette_entry, (PaletteEntry*)NULL );
            palette->m_global = *entry;
            delete entry;
        }
    }

    return palette;
}