/* 
Copyright (C) 2011-2014 Robert DeSantis
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

#include "HttpRestServices.h"

#include "Venue.h"
#include "ScenePatternDimmer.h"
#include "SceneStrobeAnimator.h"
#include "SceneSoundLevel.h"
#include "SceneMovementAnimator.h"
#include "SceneSequence.h"
#include "ScenePixelAnimator.h"

typedef AbstractAnimation* (*ANIMATION_PARSER_FUNC)(SimpleJsonParser, AnimationSignal, UIDArray );
typedef std::map<CString, ANIMATION_PARSER_FUNC> PARSER_MAP;

AbstractAnimation* SceneSequenceParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SoundLevelParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* ScenePatternDimmerParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneChannelAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneColorFaderParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneMovementAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneStrobeAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* ScenePixelAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );

AnimationSignal parseAnimationSignal( SimpleJsonParser parser );
AbstractAnimation* parseAnimation( SimpleJsonParser parser );

static PARSER_MAP init_animation_parsers() {
    PARSER_MAP animation_parsers;
    animation_parsers[ "FixtureSequencer" ] = &SceneSequenceParser;
    animation_parsers[ "SoundLevel" ] = &SoundLevelParser;
    animation_parsers[ "ScenePatternDimmer" ] = &ScenePatternDimmerParser;
    animation_parsers[ "SceneChannelAnimator" ] = &SceneChannelAnimatorParser;
    animation_parsers[ "SceneColorSwitcher" ] = &SceneColorFaderParser;
    animation_parsers[ "SceneMovementAnimator" ] = &SceneMovementAnimatorParser;
    animation_parsers[ "SceneStrobeAnimator" ] = &SceneStrobeAnimatorParser;
    animation_parsers[ "ScenePixelAnimator" ] = &ScenePixelAnimatorParser;

    return animation_parsers;
}

static PARSER_MAP animation_parsers = init_animation_parsers();

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_scenes( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    JsonBuilder json( response );
    json.startArray();

    ScenePtrArray scenes = studio.getVenue()->getScenes();

    std::sort( scenes.begin(), scenes.end(), CompareObjectNumber );

    Scene* default_scene = studio.getVenue()->getDefaultScene();

    UID active_uid = studio.getVenue()->getCurrentSceneUID();

    for ( ScenePtrArray::iterator it=scenes.begin(); it != scenes.end(); it++ ) {
        Scene* scene = (*it);

        json.startObject();
        json.add( "id", scene->getUID() );
        json.add( "number", scene->getSceneNumber() );
        json.add( "name", scene->getName() );
        json.add( "description", scene->getDescription() );
        json.add( "is_default", (scene == default_scene) );
        json.add( "is_running", (active_uid == scene->getUID()) );
        json.addArray<Acts>( "acts", scene->getActs() );

        json.startArray( "actors" );

        ActorPtrArray actors = scene->getActors();

        for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); ++it ) {
            Fixture* fixture = NULL;

            json.startObject();
            json.add( "id", (*it)->getActorUID() );
            json.add( "is_group", (*it)->isGroup() );

            if ( (*it)->isGroup() ) {
                fixture = studio.getVenue()->getGroupRepresentative( (*it)->getActorUID() );
            }
            else {
                fixture = studio.getVenue()->getFixture( (*it)->getActorUID() );
                json.add( "address", (int)fixture->getAddress() );
            }

            json.startArray( "channels" );
            if ( fixture != NULL ) {
                for ( channel_t channel=0; channel < fixture->getNumChannels(); channel++ ) {
                    Channel* ch = fixture->getChannel( channel );
                    BYTE value = (*it)->getChannelValue( fixture->mapChannel( channel ) );
                    ChannelValueRange* range = ch->getRange( value );
                    LPCSTR range_name = range ? range->getName() : "";

                    json.startObject();
                    json.add( "channel", (int)channel );
                    json.add( "name", ch->getName() );
                    json.add( "value", value );
                    json.add( "range_name", range_name );
                    json.endObject();
                }
            }
            json.endArray( "channels" );

            json.endObject();
        }

        json.endArray( "actors" );

        json.startArray( "animations" );

        for ( size_t a=0; a < scene->getNumAnimations(); a++ ) {
            AbstractAnimation* animation = scene->getAnimation( a );

            json.startObject();
            json.add( "class_name", animation->getClassName() );
            json.add( "name", animation->getName() );
            json.add( "number", animation->getNumber() );
            json.addArray<UIDArray>( "actors", animation->getActors() );

            // Add common signal data
            AnimationSignal& signal = animation->signal();
            json.startObject( "signal" );
            json.add( "sample_rate_ms", signal.getSampleRateMS() );
            json.add( "input_type", signal.getInputType() );
            json.add( "input_low", signal.getInputLow() );
            json.add( "input_high", signal.getInputHigh() );
            json.add( "sample_decay_ms", signal.getSampleDecayMS() );
            json.add( "scale_factor", signal.getScaleFactor() );
            json.add( "max_threshold", signal.getMaxThreshold() );
            json.add( "apply_to", signal.getApplyTo() );
            json.endObject();

            // Add animation specific data
            CString json_anim_name = JsonObject::encodeJsonString( animation->getClassName() );
            json.startObject( json_anim_name );

            if ( !strcmp( animation->getClassName(), SceneStrobeAnimator::className ) ) {
                SceneStrobeAnimator* ssa = (SceneStrobeAnimator*)animation;
                json.add( "strobe_neg_color", ssa->getStrobeNegColor() );
                json.add( "strobe_pos_ms", ssa->getStrobePosMS() );
                json.add( "strobe_neg_ms", ssa->getStrobeNegMS() );
            }
            else if ( !strcmp( animation->getClassName(), ScenePatternDimmer::className ) ) {
                ScenePatternDimmer* spd = (ScenePatternDimmer*)animation;
                json.add( "dimmer_pattern", spd->getDimmerPattern() );
            }
            else if ( !strcmp( animation->getClassName(), SceneSoundLevel::className ) ) {
                SceneSoundLevel* ssl = (SceneSoundLevel*)animation;
                json.add( "fade_what", ssl->getFadeWhat() );
            }
            else if ( !strcmp( animation->getClassName(), SceneColorFader::className ) ) {
                SceneColorFader* scs = (SceneColorFader*)animation;
                json.add( "fader_effect", scs->getFaderEffect() );
                json.add( "strobe_neg_color", scs->getStrobeNegColor() );
                json.add( "strobe_pos_ms", scs->getStrobePosMS() );
                json.add( "strobe_neg_ms", scs->getStrobeNegMS() );
                json.addColorArray<RGBWAArray>( "color_progression", scs->getCustomColors() );
            }
            else if ( !strcmp( animation->getClassName(), ScenePixelAnimator::className ) ) {
                ScenePixelAnimator* spa = (ScenePixelAnimator*)animation;
                json.add( "pixel_effect", spa->getEffect() );
                json.add( "generations", spa->getGenerations() );
                json.add( "pixels", spa->getPixels() );
                json.add( "increment", spa->getIncrement() );
                json.add( "fade", spa->isFadeColors() );
                json.add( "combine", spa->getCombineFixtures() );
                json.add( "pixel_off_color", spa->getEmptyColor() );
                json.addColorArray<RGBWAArray>( "color_progression", spa->getCustomColors() );
            }
            else if ( !strcmp( animation->getClassName(), SceneMovementAnimator::className ) ) {
                SceneMovementAnimator* sma = (SceneMovementAnimator*)animation;
                MovementAnimation& movement = sma->movement();

                json.add( "movement_type", movement.m_movement_type );
                json.add( "tilt_start_angle", movement.m_tilt_start );
                json.add( "tilt_end_angle", movement.m_tilt_end );
                json.add( "pan_start_angle", movement.m_pan_start );
                json.add( "pan_end_angle", movement.m_pan_end );
                json.add( "pan_increment", movement.m_pan_increment );
                json.add( "speed", movement.m_speed );
                json.add( "home_wait_periods", movement.m_home_wait_periods );
                json.add( "dest_wait_periods", movement.m_dest_wait_periods );
                json.add( "group_size", movement.m_group_size );
                json.add( "positions", movement.m_positions );
                json.add( "alternate_groups", movement.m_alternate_groups );
                json.add( "blackout_return", movement.m_backout_home_return );
                json.add( "run_once", movement.m_run_once );
                json.add( "home_x", movement.m_home_x );
                json.add( "home_y", movement.m_home_y );
                json.add( "height", movement.m_height );
                json.add( "fixture_spacing", movement.m_fixture_spacing );
                json.add( "radius", movement.m_radius );
                json.add( "head_number", movement.m_head_number );

                json.startArray( "coordinates" );
                for ( size_t index=0; index <  movement.m_coordinates.size(); index++ ) {
                    json.startObject();
                    json.add( "pan", movement.m_coordinates[index].m_pan );
                    json.add( "tilt", movement.m_coordinates[index].m_tilt );
                    json.endObject();
                }
                json.endArray( "coordinates" );
            }
            else if ( !strcmp( animation->getClassName(), SceneChannelAnimator::className ) ) {
                SceneChannelAnimator* sca = (SceneChannelAnimator*)animation;
                ChannelAnimationArray& chan_anims = sca->channelAnimations();

                json.startArray( "channel_animations" );

                for ( ChannelAnimationArray::iterator it=chan_anims.begin(); it != chan_anims.end(); ++it ) {
                    json.startObject();
                    json.add( "actor_uid", (*it).getActorUID() );
                    json.add( "channel", (*it).getChannel() );
                    json.add( "style", (*it).getAnimationStyle() );
                    json.addArray<ChannelValueArray>( "values", (*it).getChannelValues() );
                    json.endObject();
                }

                json.endArray( "channel_animations" );
            }

            json.endObject( json_anim_name );
            json.endObject();
        }

        json.endArray( "animations" );
        json.endObject();
    }

    json.endArray();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::delete_scene( CString& response, LPCSTR data ) {
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID scene_id;
        
    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        return false;

    return studio.getVenue()->deleteScene( scene_id );
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_scene_copy_fixtures( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {

    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    bool clear;
    bool keep_groups;
    ULONG scene_id;
    UIDArray actors;

    try {
        parser.parse( data );

        clear = parser.get<bool>( "clear" );
        scene_id = parser.get<unsigned long>( "scene_id" );
        keep_groups = parser.get<bool>( "keep_groups" );
        actors = parser.get<UIDArray>( "fixture_ids" );
    }
    catch ( std::exception& e ) {
        DMXStudio::log( StudioException( "JSON parser error (%s) data (%s)", e.what(), data ) );
        return false;
    }

    studio.getVenue()->moveDefaultFixturesToScene(scene_id, actors, keep_groups, clear );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::edit_scene( CString& response, LPCSTR data, EditMode mode ) {
   if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    UID scene_id;
    CString name, description;
    SceneNumber number;
    bool keep_groups;
    Scene* scene = NULL;
    UIDArray actors;
    PARSER_LIST animationParsers;
    std::vector<AbstractAnimation*> animations;
    Acts acts;
     
    try {
        parser.parse( data );

        scene_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        keep_groups = parser.get<bool>( "keep_groups" );
        actors = parser.get<UIDArray>( "actors" );
        animationParsers = parser.get<PARSER_LIST>("animations");
        acts = parser.get<Acts>( "acts" );

        for ( PARSER_LIST::iterator it=animationParsers.begin(); it != animationParsers.end(); ++it ) {
            CString class_name = (*it).get<CString>( "class_name" );
            UIDArray animation_actors = (*it).get<UIDArray>( "actors" );
            SimpleJsonParser signalParser = (*it).get<SimpleJsonParser>( "signal" );
            SimpleJsonParser animationParser = (*it).get<SimpleJsonParser>( class_name );
            AnimationSignal signal = parseAnimationSignal( signalParser );

            ANIMATION_PARSER_FUNC anim_parser_func = animation_parsers[ class_name ];
            if ( anim_parser_func == NULL )
                throw std::exception( "Unknown animation class name from client" );

            animations.push_back( anim_parser_func( animationParser, signal, animation_actors ) );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( scene_id != 0 ) {
        scene = studio.getVenue()->getScene( scene_id );
        if ( !scene )
            return false;
    }

    // Make sure number is unique
    if ( mode != UPDATE || (scene && number != scene->getNumber()) ) {
        if ( studio.getVenue()->getSceneByNumber( number ) != NULL ) 
            return false;
    }

    switch ( mode ) {
        case NEW: {
            scene_id = studio.getVenue()->allocUID();
            Scene new_scene;
            new_scene.setUID( scene_id );
            studio.getVenue()->addScene( new_scene );
            scene = studio.getVenue()->getScene( scene_id );
            studio.getVenue()->moveDefaultFixturesToScene( scene_id, actors, keep_groups, true );
            break;
        }

        case COPY: {
            scene_id = studio.getVenue()->allocUID();
            Scene new_scene( *scene );
            new_scene.setUID( scene_id );
            studio.getVenue()->addScene( new_scene );
            scene = studio.getVenue()->getScene( scene_id );
        }
        
        // Fall through 

        case UPDATE:
            // Verify actors - remove any that have been deselected
            UIDArray active_actors = scene->getActorUIDs();
            for ( UIDArray::iterator it=active_actors.begin(); it != active_actors.end(); ++it ) {
                 if ( std::find( actors.begin(), actors.end(), *it ) == actors.end() )
                    scene->removeActor( *it );
            }
            break;
    }

    scene->setName( name );
    scene->setSceneNumber( number );
    scene->setDescription( description );
    scene->setActs( acts );

    scene->clearAnimations();
    for ( std::vector<AbstractAnimation*>::iterator it=animations.begin(); it != animations.end(); ++it )
        scene->addAnimation( *it );

    if ( mode != UPDATE )            // Switch to new scene
        studio.getVenue()->selectScene( scene->getUID() );
    else if ( studio.getVenue()->getCurrentSceneUID() == scene->getUID() )
        studio.getVenue()->loadScene( );

    return true;
}

// ----------------------------------------------------------------------------
//
AnimationSignal parseAnimationSignal( SimpleJsonParser parser ) {
    return AnimationSignal(
        parser.get<DWORD>( "sample_rate_ms" ),
        (AnimationSignalInput)parser.get<int>( "input_type" ),
        parser.get<DWORD>( "sample_decay_ms" ),
        parser.get<unsigned>( "input_low" ),
        parser.get<unsigned>( "input_high" ),
        parser.get<unsigned>( "scale_factor" ),
        parser.get<unsigned>( "max_threshold" ),
        (ApplySignal)parser.get<int>( "apply_to" ) 
    );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneSequenceParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    return new SceneSequence( studio.getVenue()->allocUID(), signal, actors );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SoundLevelParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    return new SceneSoundLevel( studio.getVenue()->allocUID(), signal, actors, parser.get<WORD>( "fade_what" ) );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* ScenePatternDimmerParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    return new ScenePatternDimmer( studio.getVenue()->allocUID(), signal, actors, (DimmerPattern)parser.get<int>( "dimmer_pattern" ) );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneStrobeAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    RGBWA strobe_neg_color = parser.getHex<RGBWA>( "strobe_neg_color" );
    unsigned long strobe_pos_ms = parser.get<unsigned long>( "strobe_pos_ms" );
    unsigned long strobe_neg_ms = parser.get<unsigned long>( "strobe_neg_ms" );

    return new SceneStrobeAnimator( studio.getVenue()->allocUID(), signal, actors, strobe_neg_color, strobe_pos_ms, strobe_neg_ms );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneColorFaderParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    FaderEffect fader_effect = (FaderEffect)parser.get<unsigned>( "fader_effect" );
    RGBWA strobe_neg_color = parser.getHex<RGBWA>( "strobe_neg_color" );
    unsigned long strobe_pos_ms = parser.get<unsigned long>( "strobe_pos_ms" );
    unsigned long strobe_neg_ms = parser.get<unsigned long>( "strobe_neg_ms" );

    RGBWAArray color_progression = parser.getHex<RGBWAArray>( "color_progression" );

    return new SceneColorFader( studio.getVenue()->allocUID(), signal, actors, strobe_neg_color, 
                                strobe_pos_ms, strobe_neg_ms, color_progression, fader_effect );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneMovementAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    MovementAnimation movement;

    movement.m_movement_type = (MovementAnimationType)parser.get<unsigned>( "movement_type" );
    movement.m_speed = parser.get<unsigned>( "speed" );
    movement.m_dest_wait_periods = parser.get<unsigned>( "dest_wait_periods" );
    movement.m_home_wait_periods = parser.get<unsigned>( "home_wait_periods" );
    movement.m_group_size = parser.get<unsigned>( "group_size" );
    movement.m_positions = parser.get<unsigned>( "positions" );
    movement.m_run_once = parser.get<bool>( "run_once" );
    movement.m_backout_home_return = parser.get<bool>( "blackout_return" );
    movement.m_alternate_groups = parser.get<bool>( "alternate_groups" );
    movement.m_pan_start = parser.get<unsigned>( "pan_start_angle" );
    movement.m_pan_end = parser.get<unsigned>( "pan_end_angle" );
    movement.m_pan_increment = parser.get<unsigned>( "pan_increment" );
    movement.m_tilt_start = parser.get<unsigned>( "tilt_start_angle" );
    movement.m_tilt_end = parser.get<unsigned>( "tilt_end_angle" );
    movement.m_home_x = parser.get<float>( "home_x" );
    movement.m_home_y = parser.get<float>( "home_y" );
    movement.m_height = parser.get<float>( "height" );
    movement.m_fixture_spacing = parser.get<float>( "fixture_spacing" );
    movement.m_radius = parser.get<float>( "radius" );
    movement.m_head_number = parser.get<unsigned>( "head_number" );

    PARSER_LIST coordinates = parser.get<PARSER_LIST>("coordinates");

    for (PARSER_LIST::iterator it = coordinates.begin(); it != coordinates.end(); ++it ) {
        SimpleJsonParser& cparser =  (*it);
        unsigned pan = cparser.get<unsigned>( "pan" );
        unsigned tilt = cparser.get<unsigned>( "tilt" );
        movement.m_coordinates.push_back( FixtureCoordinate( pan, tilt ) );
    }

    return new SceneMovementAnimator( studio.getVenue()->allocUID(), signal, actors, movement );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneChannelAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) { 
    ChannelAnimationArray channel_animations;

    PARSER_LIST animations = parser.get<PARSER_LIST>("channel_animations");

    for (PARSER_LIST::iterator it = animations.begin(); it != animations.end(); ++it ) {
        SimpleJsonParser& aparser = (*it);

        UID actor = aparser.get<unsigned long>( "actor_uid" );
        unsigned channel = aparser.get<unsigned>( "channel" );
        ChannelAnimationStyle style = (ChannelAnimationStyle)aparser.get<unsigned>( "style" );
        ChannelValueArray value_list = aparser.get<ChannelValueArray>( "values" );
    
        channel_animations.push_back( ChannelAnimation( actor, channel, style, value_list ) );
    }

    return new SceneChannelAnimator( studio.getVenue()->allocUID(), signal, channel_animations );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* ScenePixelAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    PixelEffect pixel_effect = (PixelEffect)parser.get<unsigned>( "pixel_effect" );
    RGBWA empty_color = parser.getHex<RGBWA>( "pixel_off_color" );
    bool combine = parser.get<bool>( "combine" );
    bool fade = parser.get<bool>( "fade" );
    unsigned generations = parser.get<unsigned>( "generations" );
    unsigned pixels = parser.get<unsigned>( "pixels" );
    unsigned increment= parser.get<unsigned>( "increment" );
    RGBWAArray color_progression = parser.getHex<RGBWAArray>( "color_progression" );

    return new ScenePixelAnimator( studio.getVenue()->allocUID(), signal, actors, pixel_effect, 
                    color_progression, empty_color, generations, pixels, fade, increment, combine );
}