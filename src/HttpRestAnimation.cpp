/* 
Copyright (C) 2016-2017 Robert DeSantis
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
#include "SceneChannelFilter.h"
#include "SceneChannelAnimator.h"
#include "ScenePulse.h"
#include "SceneCueAnimator.h"
#include "SceneFixtureDimmer.h"
#include "SceneColorFader.h"

typedef AnimationDefinition* (*ANIMATION_PARSER_FUNC)(JsonNode&, AnimationSignal& );
typedef std::map<CString, ANIMATION_PARSER_FUNC> PARSER_MAP;

AnimationDefinition* SceneSequenceParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SoundLevelParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* ScenePatternDimmerParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneChannelAnimatorParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneColorFaderParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneMovementAnimatorParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneStrobeAnimatorParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* ScenePixelAnimatorParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneChannelFilterParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* ScenePulseParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneCueAnimatorParser( JsonNode& parser, AnimationSignal& signal );
AnimationDefinition* SceneFixtureDimmerParser( JsonNode& parser, AnimationSignal& signal );

AnimationSignal parseAnimationSignal( JsonNode parser );
AnimationDefinition* parseAnimation( JsonNode parser );

static PARSER_MAP init_animation_parsers() {
    PARSER_MAP animation_parsers;
    animation_parsers[ SceneSequence::className ] = &SceneSequenceParser;
    animation_parsers[ SceneSoundLevel::className ] = &SoundLevelParser;
    animation_parsers[ ScenePatternDimmer::className ] = &ScenePatternDimmerParser;
    animation_parsers[ SceneChannelAnimator::className ] = &SceneChannelAnimatorParser;
    animation_parsers[ SceneColorFader::className] = &SceneColorFaderParser;
    animation_parsers[ SceneMovementAnimator::className ] = &SceneMovementAnimatorParser;
    animation_parsers[ SceneStrobeAnimator::className ] = &SceneStrobeAnimatorParser;
    animation_parsers[ ScenePixelAnimator::className ] = &ScenePixelAnimatorParser;
    animation_parsers[ SceneChannelFilter::className ] = &SceneChannelFilterParser;
    animation_parsers[ ScenePulse::className ] = &ScenePulseParser;
    animation_parsers[ SceneCueAnimator::className ] = &SceneCueAnimatorParser;
    animation_parsers[ SceneFixtureDimmer::className ] = &SceneFixtureDimmerParser;

    return animation_parsers;
}

static PARSER_MAP animation_parsers = init_animation_parsers();

void animationToJson( Venue* venue, JsonBuilder& json, AnimationDefinition* animation );

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_animation( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;
    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    AnimationDefinition* animation = venue->getAnimation( uid );
    if ( animation == NULL )
        throw RestServiceException( "Invalid animation UID" );

    JsonBuilder json( response );
    json.startArray();
    animationToJson( venue, json, animation );
    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_animations( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    AnimationPtrArray animations = venue->getAnimations();
    std::sort( animations.begin(), animations.end(), CompareObjectNumber );

    JsonBuilder json( response );

    json.startArray();
    for ( auto animation : animations )
        animationToJson( venue, json, animation );
    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_animation_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;

    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->releaseAnimation( uid );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_animation_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;

    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    venue->captureAnimation( uid );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::delete_animation( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;

    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !venue->deleteAnimation( uid ) )
        throw RestServiceException( "Invalid animation UID" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_animation( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode )
{
    AnimationDefinition* animation = NULL;

    try {
        SimpleJsonParser parser;

        parser.parse( data );

        CString class_name = parser.get<CString>( "class_name" );
        JsonNode& signalParser = parser.get<JsonNode>( "signal" );
        JsonNode& animationParser = parser.get<JsonNode>( class_name );

        AnimationSignal signal = parseAnimationSignal( signalParser );

        ANIMATION_PARSER_FUNC anim_parser_func = animation_parsers[ class_name ];
        if ( anim_parser_func == NULL )
            throw std::exception( "Unknown animation class name from client" );

        animation = anim_parser_func( animationParser, signal );

        animation->setUID( parser.get<ULONG>( "id" ) );
        animation->setName( parser.get<CString>( "name" ) );
        animation->setDescription( parser.get<CString>( "description" ) );
        animation->setAnimationNumber( parser.get<ULONG>( "number" ) );
        animation->setReferenceFixtureId( parser.get<UID>( "reference_fixture_id" ) );
        animation->setShared( parser.get<bool>( "is_shared" ) );
    }
    catch ( std::exception& e ) {
        if ( animation != NULL )
            delete animation;

        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( animation->getUID() != 0 && !venue->getAnimation( animation->getUID() ) )
        throw RestServiceException( "Invalid animation UID" );

    // Make sure number is unique
    UID other_uid = animation->getNumber() != 0 ? venue->getAnimationByNumber( animation->getNumber() ) : NOUID;

    UID animation_uid = 0L;

    switch ( mode ) {
        case NEW:
        case COPY:
            // Make sure number is unique
            if ( other_uid != NOUID ) 
                throw RestServiceException( "Animation number must be unique" );

            animation->setUID( 0L );
            animation_uid = venue->addAnimation( animation );
            break;

        case UPDATE: {
            // Make sure number is unique
            if ( other_uid != NOUID && other_uid != animation->getUID() ) 
                throw RestServiceException( "Animation number must be unique" );

            animation_uid = animation->getUID();
            venue->replaceAnimation( animation );
            break;
        }

        case TEST: {
            venue->testAnimation( animation );
            break;
        }
    }

    if ( animation_uid != 0L ) {
        JsonBuilder json( response );

        json.startObject();
        json.add( "id", animation_uid );
        json.endObject();
    }
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_animation_test_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    venue->testAnimation( NULL );
}

// ----------------------------------------------------------------------------
//
AnimationSignal parseAnimationSignal( JsonNode parser ) {
    return AnimationSignal(
                (SignalTrigger)parser.get<UINT>( "trigger" ), 
                (SignalSource)parser.get<UINT>( "source" ),
                parser.get<DWORD>( "timer_ms" ),
                parser.get<DWORD>( "off_ms" ),
                parser.get<UINT>( "level_floor" ),
                parser.get<UINT>( "level_ceil" ),
                parser.get<bool>( "level_invert" ),
                parser.get<UINT>( "level_scale" ),
                parser.get<UINT>( "level_periods" ),
                parser.get<UINT>( "freq_low_hz" ),
                parser.get<UINT>( "freq_high_hz" ),
                (SpeedAdjust)parser.get<UINT>( "speed_adjust" ),
                parser.get<UINT>( "level_hold" ),
                parser.get<UINT>( "sensitivity" ) );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneSequenceParser( JsonNode& parser, AnimationSignal& signal ) {
    return new SceneSequence( NOUID, false, NOUID, signal );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SoundLevelParser( JsonNode& parser, AnimationSignal& signal ) {
    return new SceneSoundLevel( NOUID, false, NOUID, signal, parser.get<WORD>( "fade_what" ) );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* ScenePatternDimmerParser( JsonNode& parser, AnimationSignal& signal ) {
    return new ScenePatternDimmer( NOUID, false, NOUID, signal, (DimmerPattern)parser.get<int>( "dimmer_pattern" ) );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneFixtureDimmerParser( JsonNode& parser, AnimationSignal& signal ) {
    return new SceneFixtureDimmer( NOUID, false, NOUID, signal, 
            (DimmerMode)parser.get<int>( "dimmer_mode" ),
            parser.get<UINT>( "min_percent" ),
            parser.get<UINT>( "max_percent" )
        );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneStrobeAnimatorParser( JsonNode& parser, AnimationSignal& signal ) {
    RGBWA strobe_neg_color = parser.get<RGBWA>( "strobe_neg_color" );
	RGBWA strobe_color = parser.get<RGBWA>( "strobe_color" );
	StrobeType strobe_type = (StrobeType)parser.get<UINT>( "strobe_type" );
	UINT strobe_percent = parser.get<UINT>( "strobe_percent" );
    UINT strobe_pos_ms = parser.get<UINT>( "strobe_pos_ms" );
	UINT strobe_neg_ms = parser.get<UINT>( "strobe_neg_ms" );
	UINT strobe_flashes = parser.get<UINT>( "strobe_flashes" );
	UINT strobe_fade_in_ms = parser.get<UINT>( "strobe_fade_in_ms" );
	UINT strobe_fade_out_ms = parser.get<UINT>( "strobe_fade_out_ms" );

    return new SceneStrobeAnimator( NOUID, false, NOUID, signal, strobe_type, strobe_percent, StrobeTime( strobe_pos_ms,
        strobe_neg_ms, strobe_fade_in_ms, strobe_fade_out_ms, strobe_flashes ), strobe_color, strobe_neg_color );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneColorFaderParser( JsonNode& parser, AnimationSignal& signal ) {
    FaderEffect fader_effect = (FaderEffect)parser.get<unsigned>( "fader_effect" );
    RGBWA strobe_neg_color = parser.get<RGBWA>( "strobe_neg_color" );
    unsigned long strobe_pos_ms = parser.get<unsigned>( "strobe_pos_ms" );
    unsigned long strobe_neg_ms = parser.get<unsigned>( "strobe_neg_ms" );
    unsigned long strobe_flashes = parser.get<UINT>( "strobe_flashes" );
    unsigned long strobe_fade_in_ms = parser.get<unsigned>( "strobe_fade_in_ms" );
    unsigned long strobe_fade_out_ms = parser.get<unsigned>( "strobe_fade_out_ms" );
    RGBWAArray color_progression = parser.getArrayAsVector<RGBWA>( "color_progression" );

    return new SceneColorFader( NOUID, false, NOUID, signal, StrobeTime( strobe_pos_ms, strobe_neg_ms, strobe_fade_in_ms, 
		strobe_fade_out_ms, strobe_flashes ), strobe_neg_color, color_progression, fader_effect );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* ScenePulseParser( JsonNode& parser, AnimationSignal& signal ) {
    PulseEffect pulse_effect = (PulseEffect)parser.get<unsigned>( "pulse_effect" );
    RGBWA pulse_color = parser.get<RGBWA>( "pulse_color" );
    unsigned long pulse_ms = parser.get<unsigned>( "pulse_ms" );
    unsigned long pulse_fixture_count = parser.get<unsigned>( "pulse_fixture_count" );
    bool select_random = parser.get<bool>( "select_random" );

    return new ScenePulse( NOUID, false, NOUID, signal, pulse_color, pulse_ms,
        pulse_fixture_count, select_random, pulse_effect );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneMovementAnimatorParser( JsonNode& parser, AnimationSignal& signal ) {
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

    JsonNodePtrArray coordinates = parser.getObjects("coordinates");

    for ( JsonNode* cparser : coordinates ) {
        unsigned pan = cparser->get<unsigned>( "pan" );
        unsigned tilt = cparser->get<unsigned>( "tilt" );
        movement.m_coordinates.emplace_back( pan, tilt );
    }

    return new SceneMovementAnimator( NOUID, false, NOUID, signal, movement );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneChannelAnimatorParser( JsonNode& parser, AnimationSignal& signal ) { 
    ChannelAnimationArray channel_animations;

    JsonNodePtrArray animations = parser.getObjects("channel_animations");

    for ( JsonNode* aparser : animations ) {
        unsigned channel = aparser->get<unsigned>( "channel" );
        ChannelAnimationStyle style = (ChannelAnimationStyle)aparser->get<unsigned>( "style" );
        ChannelValueArray value_list = aparser->get<ChannelValueArray>( "values" );

        channel_animations.emplace_back( channel, style, value_list );
    }

    return new SceneChannelAnimator( NOUID, false, NOUID, signal, channel_animations );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneChannelFilterParser( JsonNode& parser, AnimationSignal& signal ) {
    ChannelFilter filter = (ChannelFilter)parser.get<unsigned>( "filter" );
    unsigned step = parser.get<unsigned>( "step" );
    unsigned amplitude= parser.get<unsigned>( "amplitude" );
    int offset= parser.get<int>( "offset" );
    ChannelList channels = parser.get<ChannelList>( "channels" );

    return new SceneChannelFilter( NOUID, false, NOUID, signal, filter, channels, step, amplitude, offset );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* ScenePixelAnimatorParser( JsonNode& parser, AnimationSignal& signal ) {
    PixelEffect pixel_effect = (PixelEffect)parser.get<unsigned>( "pixel_effect" );
    RGBWA empty_color = parser.get<RGBWA>( "pixel_off_color" );
    bool combine = parser.get<bool>( "combine" );
    bool fade = parser.get<bool>( "fade" );
    unsigned generations = parser.get<unsigned>( "generations" );
    unsigned pixels = parser.get<unsigned>( "pixels" );
    unsigned increment= parser.get<unsigned>( "increment" );
    RGBWAArray color_progression = parser.getArrayAsVector<RGBWA>( "color_progression" );

    return new ScenePixelAnimator( NOUID, false, NOUID, signal, pixel_effect, 
        color_progression, empty_color, generations, pixels, fade, increment, combine );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneCueAnimatorParser( JsonNode& parser, AnimationSignal& signal ) {
    bool tracking = parser.get<bool>( "tracking" );
    unsigned group_size = parser.get<unsigned>( "group_size" );
    CueArray cues;
        
    JsonNodePtrArray cues_node = parser.getObjects( "cues" );
    for ( JsonNode* cparser : cues_node )
        if ( cparser->isArray() )
            cues.push_back( cparser->getArrayAsVector<UID>() );

    return new SceneCueAnimator( NOUID, false, NOUID, signal, tracking, group_size, cues );
}

// ----------------------------------------------------------------------------
//
void animationToJson( Venue* venue, JsonBuilder& json, AnimationDefinition* animation )
{
    // Check if animation is active
    bool is_active = venue->getDefaultScene()->getAnimationByUID( animation->getUID() ) != NULL;

    json.startObject();
    json.add( "id", animation->getUID() );
    json.add( "class_name", animation->getClassName() );
    json.add( "name", animation->getName() );
    json.add( "number", animation->getNumber() );
    json.add( "reference_fixture_id", animation->getReferenceFixtureId() );
    json.add( "is_active", is_active );
    json.add( "is_shared", animation->isShared() );
    json.addArray<Acts>( "acts", animation->getActs() );

    // Add common signal data
    AnimationSignal& signal = animation->signal();

    json.startObject( "signal" );
    json.add( "trigger", signal.getTrigger() );
    json.add( "source", signal.getSource() );
    json.add( "timer_ms", signal.getTimerMS() );
    json.add( "off_ms", signal.getOffMS() );
    json.add( "level_floor", signal.getLevelFloor() );
    json.add( "level_ceil", signal.getLevelCeil() );
    json.add( "level_invert", signal.isLevelInvert() );
    json.add( "level_scale", signal.getLevelScale() );
    json.add( "level_periods", signal.getLevelPeriods() );
    json.add( "freq_low_hz", signal.getFreqLowHz() );
    json.add( "freq_high_hz", signal.getFreqHighHz() );
    json.add( "speed_adjust", signal.getSpeedAdjust() );
    json.add( "level_hold", signal.getLevelHold() );
    json.add( "sensitivity", signal.getSensitivity() );

    json.endObject();

    // Add animation specific data
	STUDIO_ASSERT( strchr( animation->getClassName(), '"' ) == NULL, "Unexpected special character in animation name" );

    json.startObject( animation->getClassName() );

    if ( !strcmp( animation->getClassName(), SceneStrobeAnimator::className ) ) {
        SceneStrobeAnimator* ssa = (SceneStrobeAnimator*)animation;

		json.add( "strobe_type", ssa->getStrobeType() );
		json.add( "strobe_percent", ssa->getStrobePercent() );

		json.add( "strobe_color", ssa->getStrobeColor() );
        json.add( "strobe_neg_color", ssa->getStrobeNegColor() );
        json.add( "strobe_pos_ms", ssa->getStrobeTime().getOnMS() );
        json.add( "strobe_neg_ms", ssa->getStrobeTime().getOffMS() );
        json.add( "strobe_flashes", ssa->getStrobeTime().getFlashes() );
        json.add( "strobe_fade_in_ms", ssa->getStrobeTime().getFadeInMS() );
        json.add( "strobe_fade_out_ms", ssa->getStrobeTime().getFadeOutMS() );
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
        json.addArray<RGBWAArray>( "color_progression", scs->getCustomColors() );
        json.add( "strobe_neg_color", scs->getStrobeNegColor() );
        json.add( "strobe_pos_ms", scs->getStrobeTime().getOnMS() );
        json.add( "strobe_neg_ms", scs->getStrobeTime().getOffMS() );
        json.add( "strobe_flashes", scs->getStrobeTime().getFlashes() );
        json.add( "strobe_fade_in_ms", scs->getStrobeTime().getFadeInMS() );
        json.add( "strobe_fade_out_ms", scs->getStrobeTime().getFadeOutMS() );
    }
    else if ( !strcmp( animation->getClassName(), ScenePulse::className ) ) {
        ScenePulse* sp = (ScenePulse*)animation;
        json.add( "pulse_effect", sp->getPulseEffect() );
        json.add( "pulse_color", sp->getPulseColor() );
        json.add( "pulse_ms", sp->getPulseMS() );
        json.add( "pulse_fixture_count", sp->getPulseFixtureCount() );
        json.add( "select_random", sp->isSelectRandom() );
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
        json.addArray<RGBWAArray>( "color_progression", spa->getCustomColors() );
    }
    else if ( !strcmp( animation->getClassName(), SceneChannelFilter::className ) ) {
        SceneChannelFilter* scf = (SceneChannelFilter*)animation;
        json.add( "filter", scf->getFilter() );
        json.add( "step", scf->getStep() );
        json.add( "amplitude", scf->getAmplitude() );
        json.add( "offset", scf->getOffset() );
        json.addArray<ChannelList>( "channels", scf->getChannels() );
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
            json.add( "channel", (*it).getChannel() );
            json.add( "style", (*it).getAnimationStyle() );
            json.addArray<ChannelValueArray>( "values", (*it).getChannelValues() );
            json.endObject();
        }

        json.endArray( "channel_animations" );
    }
    else if ( !strcmp( animation->getClassName(), SceneCueAnimator::className ) ) {
        SceneCueAnimator* sca = (SceneCueAnimator*)animation;
        json.add( "tracking", sca->isTracking() );
        json.add( "group_size", sca->getGroupSize() );

        json.startArray( "cues" );
        for ( UIDArray& palette_refs : sca->getCues() ) {
            json.startArray();
            for ( UID palette_id : palette_refs )
                json.add( palette_id );
            json.endArray();
        }
        json.endArray( "cues" );
    }
    else if ( !strcmp( animation->getClassName(), SceneFixtureDimmer::className ) ) {
        SceneFixtureDimmer* sfd = (SceneFixtureDimmer*)animation;
        json.add( "dimmer_mode", sfd->getDimmerMode() );
        json.add( "min_percent", sfd->getMinPercent() );
        json.add( "max_percent", sfd->getMaxPercent() );
    }

    json.endObject( animation->getClassName() );
    json.endObject();
}

