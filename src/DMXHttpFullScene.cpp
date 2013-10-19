/* 
Copyright (C) 2011-2013 Robert DeSantis
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

#include "DMXHttpFull.h"

#include "Venue.h"
#include "ScenePatternDimmer.h"
#include "SceneStrobeAnimator.h"
#include "SceneSoundLevel.h"
#include "SceneMovementAnimator.h"
#include "SceneSequence.h"

typedef AbstractAnimation* (*ANIMATION_PARSER_FUNC)(SimpleJsonParser, AnimationSignal, UIDArray );
typedef std::map<CString, ANIMATION_PARSER_FUNC> PARSER_MAP;

AbstractAnimation* SceneSequenceParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SoundLevelParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* ScenePatternDimmerParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneChannelAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneColorSwitcherParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneMovementAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );
AbstractAnimation* SceneStrobeAnimatorParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors );

AnimationSignal parseAnimationSignal( SimpleJsonParser parser );
AbstractAnimation* parseAnimation( SimpleJsonParser parser );

static PARSER_MAP init_animation_parsers() {
    PARSER_MAP animation_parsers;
    animation_parsers[ "FixtureSequencer" ] = &SceneSequenceParser;
    animation_parsers[ "SoundLevel" ] = &SoundLevelParser;
    animation_parsers[ "ScenePatternDimmer" ] = &ScenePatternDimmerParser;
    animation_parsers[ "SceneChannelAnimator" ] = &SceneChannelAnimatorParser;
    animation_parsers[ "SceneColorSwitcher" ] = &SceneColorSwitcherParser;
    animation_parsers[ "SceneMovementAnimator" ] = &SceneMovementAnimatorParser;
    animation_parsers[ "SceneStrobeAnimator" ] = &SceneStrobeAnimatorParser;

    return animation_parsers;
}

static PARSER_MAP animation_parsers = init_animation_parsers();

// ----------------------------------------------------------------------------
//
template <class T>
CString makeColorArray( T& array )
{
    CString response( "[" );

    for ( T::iterator it=array.begin(); it != array.end(); ++it ) {
        if ( it != array.begin() )
            response.Append( "," );
        response.AppendFormat( "\"%06lX\"", (ULONG)(*it) );
    }

    response.Append( "]" );

    return response;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::query_scenes( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    response = "[";
    
    ScenePtrArray scenes = studio.getVenue()->getScenes();
    bool first = true;

    std::sort( scenes.begin(), scenes.end(), CompareObjectNumber );

    Scene* default_scene = studio.getVenue()->getDefaultScene();

    UID active_uid = studio.getVenue()->getCurrentSceneUID();

    for ( ScenePtrArray::iterator it=scenes.begin(); it != scenes.end(); it++, first=false) {
        if ( !first )
            response.Append( "," );

        Scene* scene = (*it);

        response.AppendFormat( "{ \"id\":%lu, \"number\":%lu, \"name\":\"%s\", \"description\":\"%s\", \"is_default\":%s, \"is_private\":%s, \"is_running\":%s,", 
            scene->getUID(), scene->getSceneNumber(), encodeJsonString( scene->getName() ), encodeJsonString( scene->getDescription() ),
            (scene == default_scene) ? "true" : "false",
            (scene->isPrivate() ) ? "true" : "false",
            (active_uid == scene->getUID() ) ? "true" : "false",
            makeUIDArray<UIDArray>( scene->getActorUIDs() ) );


        response.Append( "\"actors\": [ " );

        ActorPtrArray actors = scene->getActors();

        for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); ++it ) {
            if ( it != actors.begin() )
                response.AppendFormat( "," );

            Fixture* fixture = studio.getVenue()->getFixture( (*it)->getFUID() );

            response.AppendFormat( "{ \"id\":%lu, \"address\":%u, \"channels\": [", 
                    fixture->getUID(), fixture->getAddress() );

            for ( channel_t channel=0; channel < fixture->getNumChannels(); channel++ ) {
                if ( channel > 0 )
                    response.AppendFormat( "," );

                Channel* ch = fixture->getChannel( channel );
                BYTE value = (*it)->getChannelValue( fixture->mapChannel( channel ) );
                ChannelValueRange* range = ch->getRange( value );
                LPCSTR range_name = range ? range->getName() : "";

                 response.AppendFormat( "{ \"channel\":%u, \"name\":\"%s\", \"value\": %u, \"range_name\":\"%s\" }", 
                    channel, encodeJsonString( ch->getName() ), value, encodeJsonString( range_name ) );
            }

            response.AppendFormat( "]}" );   // End of channels
        }

        response.Append( "], \"animations\":[" );

        for ( size_t a=0; a < scene->getNumAnimations(); a++ ) {
            if ( a > 0 ) 
                response.Append( "," );

            AbstractAnimation* animation = scene->getAnimation( a );

            response.AppendFormat( "{ \"class_name\":\"%s\", \"name\":\"%s\", \"number\":%u, \"actors\":%s, ",
                encodeJsonString( animation->getClassName() ),
                encodeJsonString( animation->getName() ),
                animation->getNumber(),
                makeUIDArray<UIDArray>( animation->getActors() ) );

            // Add common signal data
            AnimationSignal& signal = animation->signal();
            response.AppendFormat( "\"signal\":{ \"sample_rate_ms\":%lu, \"input_type\":%u, \"input_low\":%u, \"input_high\":%u, \"sample_decay_ms\":%lu, \"scale_factor\":%u, \"max_threshold\":%u, \"apply_to\":%u }, ",
                signal.getSampleRateMS(),
                signal.getInputType(),
                signal.getInputLow(),
                signal.getInputHigh(),
                signal.getSampleDecayMS(),
                signal.getScaleFactor(),
                signal.getMaxThreshold(),
                signal.getApplyTo() );

            // Add animation specific data
            response.AppendFormat( "\"%s\":", encodeJsonString( animation->getClassName() ) );

            if ( !strcmp( animation->getClassName(), SceneStrobeAnimator::className ) ) {
                SceneStrobeAnimator* ssa = (SceneStrobeAnimator*)animation;
                response.AppendFormat( "{ \"strobe_neg_color\":\"%06lX\",\"strobe_pos_ms\":%lu,\"strobe_neg_ms\":%lu }",
                    ssa->getStrobeNegColor(),
                    ssa->getStrobePosMS(),
                    ssa->getStrobeNegMS() );
            }
            else if ( !strcmp( animation->getClassName(), ScenePatternDimmer::className ) ) {
                ScenePatternDimmer* spd = (ScenePatternDimmer*)animation;
                response.AppendFormat( "{ \"dimmer_pattern\":%u }",
                    spd->getDimmerPattern() );
            }
            else if ( !strcmp( animation->getClassName(), SceneSoundLevel::className ) ) {
                SceneSoundLevel* ssl = (SceneSoundLevel*)animation;
                response.AppendFormat( "{ \"fade_what\":%u }",
                    ssl->getFadeWhat() );
            }
           else if ( !strcmp( animation->getClassName(), SceneColorSwitcher::className ) ) {
                SceneColorSwitcher* scs = (SceneColorSwitcher*)animation;
                response.AppendFormat( "{ \"strobe_neg_color\":\"%06lX\",\"strobe_pos_ms\":%lu,\"strobe_neg_ms\":%lu, \"color_progression\":%s }",
                    scs->getStrobeNegColor(),
                    scs->getStrobePosMS(),
                    scs->getStrobeNegMS(),
                    makeColorArray<ColorProgression>( scs->getCustomColors() ) );
            }
           else if ( !strcmp( animation->getClassName(), SceneMovementAnimator::className ) ) {
                SceneMovementAnimator* sma = (SceneMovementAnimator*)animation;
                MovementAnimation& movement = sma->movement();

                response.AppendFormat( "{ \"movement_type\":%u, \"tilt_start_angle\":%u, \"tilt_end_angle\":%u, \"pan_start_angle\":%u, \"pan_end_angle\":%u, \"pan_increment\":%u,",
                    movement.m_movement_type, movement.m_tilt_start, movement.m_tilt_end, movement.m_pan_start, movement.m_pan_end, movement.m_pan_increment );

                response.AppendFormat( "\"speed\":%u, \"home_wait_periods\":%u, \"dest_wait_periods\":%u, \"group_size\":%u, \"positions\":%u, ",
                    movement.m_speed, movement.m_home_wait_periods, movement.m_dest_wait_periods, movement.m_group_size, movement.m_positions );

                response.AppendFormat( "\"alternate_groups\":%s, \"blackout_return\":%s, \"run_once\":%s, \"home_x\":%f, \"home_y\":%f, \"height\":%f, \"fixture_spacing\":%f, \"radius\":%f, ",
                    movement.m_alternate_groups ? "true" : "false",
                    movement.m_backout_home_return ? "true" : "false",
                    movement.m_run_once ? "true" : "false",
                    movement.m_home_x, movement.m_home_y, movement.m_height, movement.m_fixture_spacing, movement.m_radius );

                response.Append( "\"coordinates\":[" );

                for ( size_t index=0; index <  movement.m_coordinates.size(); index++ ) {
                    if ( index > 0 )
                        response.Append( "," );

                    response.AppendFormat( "{ \"pan\":%u, \"tilt\":%u }", movement.m_coordinates[index].m_pan, movement.m_coordinates[index].m_tilt );
                }

                response.Append( "]}" );
            }
            else if ( !strcmp( animation->getClassName(), SceneChannelAnimator::className ) ) {
                SceneChannelAnimator* sca = (SceneChannelAnimator*)animation;
                ChannelAnimationArray& chan_anims = sca->channelAnimations();

                response.AppendFormat( "{ \"channel_animations\":[ " );

                for ( ChannelAnimationArray::iterator it=chan_anims.begin(); it != chan_anims.end(); ++it ) {
                    if ( it != chan_anims.begin() )
                        response.Append( "," );

                    response.AppendFormat( "{ \"actor_uid\":%lu, \"channel\":%u, \"style\":%u, \"values\":%s } ",
                        (*it).getActor(),
                        (*it).getChannel(),
                        (*it).getAnimationStyle(),
                        makeUIDArray<ChannelValueArray>( (*it).getChannelValues() ) );
                }

                response.Append( "]}" );
            }
            else {
                response.Append( "{}" );
            }

            response.Append( "} ");
        }

        response.Append( "]}" );
    }

    response.Append( "]" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::delete_scene( CString& response, LPCSTR data ) {
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID scene_id;
        
    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        return false;

    return studio.getVenue()->deleteScene( scene_id );
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_scene_copy_fixtures( CString& response, LPCSTR data, DWORD size, LPCSTR content_type ) {

    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    bool clear;
    ULONG scene_id;
    std::vector<ULONG> fixtures;
    UIDSet uids;

    try {
        parser.parse( data );

        clear = parser.get<bool>( "clear" );
        scene_id = parser.get<unsigned long>( "scene_id" );
        fixtures = parser.get<std::vector<ULONG>>( "fixture_ids" );
    }
    catch ( std::exception& e ) {
        DMXStudio::log( StudioException( "JSON parser error (%s) data (%s)", e.what(), data ) );
        return false;
    }

    Scene* scene = studio.getVenue()->getScene( scene_id );
    if ( !scene )
        return false;

    Scene* default_scene = studio.getVenue()->getDefaultScene();

    for ( std::vector<ULONG>::iterator it=fixtures.begin(); it != fixtures.end(); ++it ) {
        Fixture* fixture = studio.getVenue()->getFixture( (UID)(*it) );
        if ( fixture ) {
            uids.insert( fixture->getUID() );
            continue;
        }

        FixtureGroup* group = studio.getVenue()->getFixtureGroup( (UID)(*it) );
        if ( group ) {
            UIDSet group_fixtures = group->getFixtures();
            uids.insert( group_fixtures.begin(), group_fixtures.end() );
            continue;
        }

        // Bad ID
        return false;
    }  

    // Add the fixtures to the scene
    for ( UIDSet::iterator it=uids.begin(); it != uids.end(); ++it ) {
        SceneActor *actor = default_scene->getActor( (*it) );
        if ( !actor )
            return false;

        scene->addActor( *actor );
        if ( clear )
            default_scene->removeActor( (*it) );
    }

    // Restart the scene if this is the active scene
    if ( studio.getVenue()->getCurrentSceneUID() == scene_id )
        studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::edit_scene( CString& response, LPCSTR data, EditMode mode ) {
   if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    UID scene_id;
    CString name, description;
    SceneNumber number;
    bool is_private;
    Scene* scene = NULL;
    UIDArray actors;
    PARSER_LIST animationParsers;
    std::vector<AbstractAnimation*> animations;
     
    try {
        parser.parse( data );

        scene_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        is_private = parser.get<bool>( "is_private" );
        actors = parser.get<UIDArray>( "actors" );
        animationParsers = parser.get<PARSER_LIST>("animations");

        for ( PARSER_LIST::iterator it=animationParsers.begin(); it != animationParsers.end(); ++it ) {
            CString class_name = (*it).get<CString>( "class_name" );
            UIDArray animation_actors = (*it).get<UIDArray>( "actors" );
            SimpleJsonParser signalParser = (*it).get<SimpleJsonParser>( "signal" );
            SimpleJsonParser animationParser = (*it).get<SimpleJsonParser>( class_name );
            AnimationSignal signal = parseAnimationSignal( signalParser );

            ANIMATION_PARSER_FUNC anim_parser_func = animation_parsers[ class_name ];
            if ( anim_parser_func == NULL )
                throw new std::exception( "Unknown animation class name from client" );

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

            for ( std::vector<ULONG>::iterator it = actors.begin(); it != actors.end(); ++it )
                studio.getVenue()->moveDefaultFixtureToScene( scene_id, (*it) );

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
    scene->setPrivate( is_private );

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
    unsigned long strobe_neg_color = parser.getHex<unsigned long>( "strobe_neg_color" );
    unsigned long strobe_pos_ms = parser.get<unsigned long>( "strobe_pos_ms" );
    unsigned long strobe_neg_ms = parser.get<unsigned long>( "strobe_neg_ms" );

    return new SceneStrobeAnimator( studio.getVenue()->allocUID(), signal, actors, strobe_neg_color, strobe_pos_ms, strobe_neg_ms );
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneColorSwitcherParser( SimpleJsonParser parser, AnimationSignal signal, UIDArray actors ) {
    unsigned long strobe_neg_color = parser.getHex<unsigned long>( "strobe_neg_color" );
    unsigned long strobe_pos_ms = parser.get<unsigned long>( "strobe_pos_ms" );
    unsigned long strobe_neg_ms = parser.get<unsigned long>( "strobe_neg_ms" );
    ColorProgression color_progression = parser.getHex<ColorProgression>( "color_progression" );

    return new SceneColorSwitcher( studio.getVenue()->allocUID(), signal, actors, strobe_neg_color, strobe_pos_ms, strobe_neg_ms, color_progression );
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


