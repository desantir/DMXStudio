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

#include "DMXTextUI.h"

AnimationEditorArray DMXTextUI::animEditors;

void DMXTextUI::initializeAnimationEditors() {
    animEditors.push_back( AnimationEditor( SceneSequence::animationName,	        SceneSequence::className, &DMXTextUI::animSequencerEditor ) );
    animEditors.push_back( AnimationEditor( SceneSoundLevel::animationName,		    SceneSoundLevel::className, &DMXTextUI::animSoundLevelEditor) );
    animEditors.push_back( AnimationEditor( ScenePatternDimmer::animationName,	    ScenePatternDimmer::className, &DMXTextUI::animPatternDimmerEditor) );
    animEditors.push_back( AnimationEditor( SceneChannelAnimator::animationName,	SceneChannelAnimator::className, &DMXTextUI::animChannelEditor) );
    animEditors.push_back( AnimationEditor( SceneColorFader::animationName,		SceneColorFader::className, &DMXTextUI::animColorFaderEditor) );
    animEditors.push_back( AnimationEditor( SceneMovementAnimator::animationName,	SceneMovementAnimator::className, &DMXTextUI::animMovementEditor) );
    animEditors.push_back( AnimationEditor( SceneStrobeAnimator::animationName,	    SceneStrobeAnimator::className, &DMXTextUI::animStrobeEditor) );
    animEditors.push_back( AnimationEditor( ScenePixelAnimator::animationName,	    ScenePixelAnimator::className, &DMXTextUI::animPixelEditor) );
}

// ----------------------------------------------------------------------------
//
class SignalForm : public Form
{
    NumberedListField	m_input_type;
    IntegerField		m_sample_ms;
    IntegerField		m_freq_low;
    IntegerField		m_freq_high;
    IntegerField		m_random_low;
    IntegerField		m_random_high;
    IntegerField		m_decay_ms;
    NumberedListField	m_apply_to;
    IntegerField		m_scale_factor;
    IntegerField		m_max_threshold;

public:
    SignalForm( TextIO* input_stream, AnimationSignal& signal ) :
        Form( input_stream, "Animation Input Signal and Properties" ),
        m_input_type( "Signal input" ),
        m_sample_ms( "Level sample interval (ms)", signal.getSampleRateMS(), 0 ),
        m_freq_low( "Frequence low", signal.getInputLow(), 0, 44100/2 ),
        m_freq_high( "Frequence high", signal.getInputHigh(), 0, 44100/2 ),
        m_random_low( "Random low", signal.getInputLow(), 0, 100),
        m_random_high( "Random high", signal.getInputHigh(), 0, 100 ),
        m_decay_ms( "Sample decay (ms)", signal.getSampleDecayMS(), 0 ),
        m_apply_to( "Apply Input" ),
        m_scale_factor( "Level curve factor (1=linear)", signal.getScaleFactor(), 1, 100 ),
        m_max_threshold( "Maximum level threshold", signal.getMaxThreshold(), 0, AnimationSignal::MAXIMUM_LEVEL )
    {
        m_input_type.addKeyValue( 1, "Timer" );
        m_input_type.addKeyValue( 2, "Sound level" );
        m_input_type.addKeyValue( 3, "Average sound level" );
        m_input_type.addKeyValue( 4, "Frequency beat" );
        m_input_type.addKeyValue( 5, "Frequency level" );
        m_input_type.addKeyValue( 6, "Random value" );
        m_input_type.setDefaultListValue( signal.getInputType() );

        m_apply_to.addKeyValue( 1, "Channel Values" );
        m_apply_to.addKeyValue( 2, "Sample Time" );
        m_apply_to.addKeyValue( 3, "Channel Values & Time" );
        m_apply_to.addKeyValue( 4, "Inverse Sample Time" );
        m_apply_to.setDefaultListValue( signal.getApplyTo() );

        add( m_input_type );
        add( m_apply_to );
        add( m_sample_ms );
        add( m_freq_low );
        add( m_freq_high );
        add( m_random_low );
        add( m_random_high );
        add( m_decay_ms );
        add( m_scale_factor );
        add( m_max_threshold );
    }

    void updateAnimationSignal( AnimationSignal& signal ) {
        signal.setInputType( (AnimationSignalInput)(m_input_type.getListValue()) );
        signal.setSampleRateMS( m_sample_ms.getLongValue() );

        if ( signal.getInputType() != CAI_RANDOM ) {
            signal.setInputLow( m_freq_low.getLongValue() );
            signal.setInputHigh( m_freq_high.getLongValue() );
        }
        else {
            signal.setInputLow( m_random_low.getLongValue() );
            signal.setInputHigh( m_random_high.getLongValue() );
        }

        signal.setSampleDecayMS( m_decay_ms.getLongValue() );
        signal.setApplyTo( (ApplySignal)m_apply_to.getListValue() );
        signal.setScaleFactor( m_scale_factor.getLongValue() );
        signal.setMaxThreshold( m_max_threshold.getLongValue() );
    }

protected:
     void fieldLeaveNotify( size_t field_num ) {
         if ( getField<Field>(field_num) != &m_input_type )
             return;

        m_freq_low.setHidden( true );
        m_freq_high.setHidden( true );
        m_random_low.setHidden( true );
        m_random_high.setHidden( true );
        m_apply_to.setHidden( true );
        m_scale_factor.setHidden( true );
        m_max_threshold.setHidden( true );

        switch ( (AnimationSignalInput)m_input_type.getListValue() ) {
            case CAI_FREQ_LEVEL:
                m_scale_factor.setHidden( false );
                m_max_threshold.setHidden( false );
            case CAI_FREQ_BEAT:
                m_apply_to.setHidden( false );
                m_freq_low.setHidden( false );
                m_freq_high.setHidden( false );
                break;

            case CAI_RANDOM:
                m_apply_to.setHidden( false );
                m_random_low.setHidden( false );
                m_random_high.setHidden( false );
                break;

            case CAI_SOUND_LEVEL:
            case CAI_AVG_SOUND_LEVEL:
                m_max_threshold.setHidden( false );
                m_apply_to.setHidden( false );
                m_scale_factor.setHidden( false );
                break;

            default:
                break;
        }
     }
};

// ----------------------------------------------------------------------------
//
class ChannelAnimationForm : public Form
{
    Venue*					m_venue;

    ActorSelectField        m_actor_select;
    BooleanField			m_more_field;
    FixtureChannelField		m_channel_field;
    NumberedListField		m_value_anim_field;
    IntegerField			m_range_low_field;
    IntegerField			m_range_high_field;
    ChannelValueListField	m_value_list_field;

public:
    ChannelAnimationForm( TextIO* input_stream, Venue* venue, Scene* scene, ChannelAnimation& animation, 
                          bool multiple_fixtures, bool show_more ) :
        Form( input_stream, "Define/Update Channel Animation" ),
        m_venue( venue ),
        m_actor_select( "Fixtures (comma separated)", venue, scene, UIDArray( animation.getActorUID() ) ),
        m_more_field( "Add another channel", false ),
        m_channel_field( "Channel", NULL, animation.getChannel() ),
        m_value_anim_field( "Value animation" ),
        m_range_low_field( "Range low value (inclusive)", 0, 0, 255 ),
        m_range_high_field( "Range high value (inclusive)", 0, 0, 255 ),
        m_value_list_field( "Channel values (comma separated)", animation.getChannelValues() )
    {
        m_value_anim_field.addKeyValue( 1, "List" );
        m_value_anim_field.addKeyValue( 2, "Range" );
        m_value_anim_field.addKeyValue( 3, "Scale" );
        m_value_anim_field.setDefaultListValue( animation.getAnimationStyle() );

        add( m_actor_select );
        add( m_channel_field );
        add( m_value_anim_field );
        add( m_range_low_field );
        add( m_range_high_field );
        add( m_value_list_field );
        add( m_more_field );

        m_more_field.setHidden( !show_more );

        if ( !multiple_fixtures ) {
            m_actor_select.allowMultiple( false );
            m_actor_select.setLabel( "Fixture" );
        }

        ChannelValueArray values = animation.getChannelValues();
        if ( values.size() >= 2 ) {
            m_range_low_field.setInitialValue( values[0] );
            m_range_high_field.setInitialValue( values[1] );
        }
    }

    bool addAnotherChannel() const {
        return m_more_field.isSet();
    }

    void updateAnimation( ChannelAnimation& animation ) {
        animation.setActorUID( m_actor_select.getActorUIDs()[0] );
        animation.setChannel( m_channel_field.getChannel() );
        animation.setAnimationStyle( (ChannelAnimationStyle)m_value_anim_field.getListValue() );

        ChannelValueArray values;

        if ( animation.getAnimationStyle() == CAM_RANGE ) {
            values.push_back( m_range_low_field.getIntValue() );
            values.push_back( m_range_high_field.getIntValue() );
        }
        else if ( animation.getAnimationStyle() == CAM_LIST )
            values = m_value_list_field.getValues();

        animation.setChannelValues( values );
    }

    void appendAnimations( ChannelAnimationArray& animations ) {
        UIDArray fixtures = m_actor_select.getActorUIDs();
        for ( UIDArray::iterator it=fixtures.begin(); it != fixtures.end(); ++it ) {
            ChannelAnimation animation( 0, 0, CAM_SCALE, ChannelValueArray() );
            updateAnimation( animation );
            animation.setActorUID( (*it) );
            animations.push_back( animation );
        }
    }
    
protected:
     void fieldLeaveNotify( size_t field_num ) {
         if ( getField<Field>(field_num) == &m_actor_select ) {
            if ( m_actor_select.getActorUIDs().size() > 0 )
                m_channel_field.setFixture( m_venue->getFixture( m_actor_select.getActorUIDs()[0] ) );
         }
         else if ( getField<Field>(field_num) == &m_value_anim_field ) {
            m_range_low_field.setHidden( true );
            m_range_high_field.setHidden( true );
            m_value_list_field.setHidden( true );

            switch ( (ChannelAnimationStyle)m_value_anim_field.getListValue() ) {
                case CAM_LIST:
                    m_value_list_field.setHidden( false );
                    break;

                case CAM_RANGE:
                    m_range_low_field.setHidden( false );
                    m_range_high_field.setHidden( false );
                    break;

                case CAM_SCALE:
                    break;
            }
        }
     }
};

// ----------------------------------------------------------------------------
//
class AnimationMovementForm : public SignalForm
{
    ActorSelectField    m_actor_select;
    NumberedListField	m_movement_field;
    IntegerField		m_movement_speed_field;
    IntegerField		m_tilt_start_field;
    IntegerField		m_tilt_end_field;
    IntegerField		m_pan_start_field;
    IntegerField		m_pan_end_field;
    IntegerField		m_pan_inc_field;
    IntegerField		m_home_wait_field;
    IntegerField		m_dest_wait_field;
    BooleanField 		m_beam_return_field;
    IntegerField 		m_positions_field;
    IntegerField 		m_group_size_field;
    BooleanField 		m_alternate_groups_field;
    CoordinatesField    m_coordinates_field;
    BooleanField 		m_run_once_field;
    FloatField          m_home_x_field;
    FloatField          m_home_y_field;
    FloatField          m_height_field;
    FloatField          m_fixture_spacing_field;
    FloatField          m_radius_field;

public:
    AnimationMovementForm( TextIO* text_io, Venue* venue, Scene* scene, SceneMovementAnimator* anim ) :
        SignalForm( text_io, anim->signal() ),
        m_actor_select( "Fixtures (comma separated)", venue, scene, anim->getActors() ),
        m_movement_field( "Movement" ),
        m_movement_speed_field( "Movement speed (0=fastest)",  anim->movement().m_speed, 0, 255 ),
        m_tilt_start_field( "Start tilt angle (degrees)", anim->movement().m_tilt_start, 0, 360 ),
        m_tilt_end_field( "End tilt angle (degrees)", anim->movement().m_tilt_end, 0, 360 ),
        m_pan_start_field( "Start pan angle (degrees)", anim->movement().m_pan_start, 0, 540 ),
        m_pan_end_field( "End pan angle (degrees)", anim->movement().m_pan_end, 0, 540 ),
        m_pan_inc_field( "Pan increment (degrees)", anim->movement().m_pan_increment, 0, 540 ),
        m_home_wait_field( "Home wait (periods)", anim->movement().m_home_wait_periods, 1, MAX_WAIT_PERIODS ),
        m_dest_wait_field( "Destination wait (periods)", anim->movement().m_dest_wait_periods, 1, MAX_WAIT_PERIODS ),
        m_beam_return_field( "Blackout on return to home", anim->movement().m_backout_home_return ),
        m_positions_field( "Postions to generate", anim->movement().m_positions, 1, MAX_RANDOM_POSITIONS ),
        m_group_size_field( "Fixtures per group", anim->movement().m_group_size, 1 ),
        m_alternate_groups_field( "Reverse group home positions", anim->movement().m_alternate_groups ),
        m_coordinates_field( "Coordinates (pan1,tilt1;pan2,tilt2;...)", anim->movement().m_coordinates ),
        m_run_once_field( "Run movement animation once", anim->movement().m_run_once ),
        m_home_x_field( "Target X", anim->movement().m_home_x, "%.1f" ),
        m_home_y_field( "Target Y", anim->movement().m_home_y, "%.1f" ),
        m_height_field( "Height", anim->movement().m_height, "%.1f" ),
        m_radius_field( "Radius", anim->movement().m_radius, "%.1f" ),
        m_fixture_spacing_field( "Spacing", anim->movement().m_fixture_spacing, "%.1f" )
    {
        m_movement_field.addKeyValue( 1, "Random location" );
        m_movement_field.addKeyValue( 2, "Fan" );
        m_movement_field.addKeyValue( 3, "Rotate" );
        m_movement_field.addKeyValue( 4, "Nod (up/down)" );
        m_movement_field.addKeyValue( 5, "Criss-cross" );
        m_movement_field.addKeyValue( 6, "Absolute coordinates" );
        m_movement_field.addKeyValue( 7, "Moon flower" );
        m_movement_field.setDefaultListValue( anim->movement().m_movement_type );

        add( m_movement_field );
        add( m_movement_speed_field );
        add( m_home_x_field );
        add( m_home_y_field );
        add( m_height_field );
        add( m_fixture_spacing_field );
        add( m_radius_field );
        add( m_tilt_start_field );
        add( m_tilt_end_field );
        add( m_pan_start_field );
        add( m_pan_end_field );
        add( m_pan_inc_field );
        add( m_home_wait_field );
        add( m_dest_wait_field );
        add( m_beam_return_field );
        add( m_positions_field );
        add( m_group_size_field );
        add( m_alternate_groups_field );
        add( m_coordinates_field );
        add( m_run_once_field );
        add( m_actor_select );
    }

    void update( SceneMovementAnimator* anim ) {
        updateAnimationSignal( anim->signal() );

        anim->setActors( m_actor_select.getActorUIDs() );
        anim->movement().m_movement_type = (MovementAnimationType)m_movement_field.getListValue();
        anim->movement().m_speed = (BYTE)m_movement_speed_field.getIntValue();
        anim->movement().m_tilt_start = m_tilt_start_field.getIntValue();
        anim->movement().m_tilt_end = m_tilt_end_field.getIntValue();
        anim->movement().m_pan_start = m_pan_start_field.getIntValue();
        anim->movement().m_pan_end = m_pan_end_field.getIntValue();
        anim->movement().m_pan_increment = m_pan_inc_field.getIntValue();
        anim->movement().m_home_wait_periods = m_home_wait_field.getIntValue();
        anim->movement().m_dest_wait_periods = m_dest_wait_field.getIntValue();
        anim->movement().m_group_size = m_group_size_field.getIntValue();
        anim->movement().m_positions = m_positions_field.getIntValue();
        anim->movement().m_alternate_groups = m_alternate_groups_field.isSet();
        anim->movement().m_backout_home_return = m_beam_return_field.isSet();
        anim->movement().m_coordinates = m_coordinates_field.getCoordinates();
        anim->movement().m_run_once = m_run_once_field.isSet();
        anim->movement().m_home_x = m_home_x_field.getFloatValue();
        anim->movement().m_home_y = m_home_y_field.getFloatValue();
        anim->movement().m_height = m_height_field.getFloatValue();
        anim->movement().m_fixture_spacing = m_fixture_spacing_field.getFloatValue();
        anim->movement().m_radius = m_radius_field.getFloatValue();
    }

protected:
     void fieldLeaveNotify( size_t field_num ) {
         if ( getField<Field>(field_num) != &m_movement_field ) {
            SignalForm::fieldLeaveNotify( field_num );
            return;
        }

        m_pan_inc_field.setHidden( true );
        m_home_wait_field.setHidden( true );
        m_beam_return_field.setHidden( true );
        m_positions_field.setHidden( true );
        m_group_size_field.setHidden( true );
        m_alternate_groups_field.setHidden( true );
        m_coordinates_field.setHidden( true );
        m_home_x_field.setHidden( true );
        m_home_y_field.setHidden( true );
        m_height_field.setHidden( true );
        m_fixture_spacing_field.setHidden( true );
        m_radius_field.setHidden( true );
        m_tilt_end_field.setHidden( true );
        m_pan_start_field.setHidden( true );
        m_pan_end_field.setHidden( true );
        m_pan_inc_field.setHidden( true );
        m_dest_wait_field.setHidden( false );
        m_run_once_field.setHidden( false );
        m_movement_speed_field.setHidden( false );

        switch ( (MovementAnimationType)m_movement_field.getListValue() ) {
            case MOVEMENT_COORDINATES:
                m_coordinates_field.setHidden( false );
                m_beam_return_field.setHidden( false );
                break;

            case MOVEMENT_RANDOM:

                m_positions_field.setHidden( false );
                m_group_size_field.setHidden( false );
                m_tilt_end_field.setHidden( false );
                m_pan_start_field.setHidden( false );
                m_pan_end_field.setHidden( false );
                m_pan_inc_field.setHidden( false );
                break;

            case MOVEMENT_FAN:
                m_home_wait_field.setHidden( false );
                m_beam_return_field.setHidden( false );
                m_tilt_end_field.setHidden( false );
                m_pan_start_field.setHidden( false );
                m_pan_end_field.setHidden( false );
                m_pan_inc_field.setHidden( false );
                break;

            case MOVEMENT_MOONFLOWER:
                m_pan_inc_field.setHidden( false );
                m_positions_field.setHidden( false );
                m_home_wait_field.setHidden( false );
                m_home_x_field.setHidden( false );
                m_home_y_field.setHidden( false );
                m_height_field.setHidden( false );
                m_fixture_spacing_field.setHidden( false );
                m_radius_field.setHidden( false );
                break;

            case MOVEMENT_NOD:
                m_pan_inc_field.setHidden( false );
            case MOVEMENT_ROTATE:
            case MOVEMENT_XCROSS:
                m_home_wait_field.setHidden( false );
                m_beam_return_field.setHidden( false );
                m_group_size_field.setHidden( false );
                m_alternate_groups_field.setHidden( false );
                m_tilt_end_field.setHidden( false );
                m_pan_start_field.setHidden( false );
                m_pan_end_field.setHidden( false );
                m_pan_inc_field.setHidden( false );
                break;
        }
     }
};

// ----------------------------------------------------------------------------
//
class PixelAnimationForm : public SignalForm
{
    ActorSelectField    m_actor_select;
    NumberedListField	m_effect_field;
    IntegerField        m_generations_field;
    IntegerField        m_pixels_field;
    IntegerField        m_increment_field;
    BooleanField        m_combine_field;
    BooleanField        m_color_fade_field;
    ColorSelectField    m_color_progression_field;
    ColorSelectField    m_off_pixel_color_field;

public:
    PixelAnimationForm( TextIO* text_io, Venue* venue, Scene* scene, ScenePixelAnimator* anim ) :
        m_effect_field( "Pixel Effect" ),
        SignalForm( text_io, anim->signal() ),
        m_actor_select( "Fixtures (comma separated)", venue, scene, anim->getActors() ),
        m_generations_field( "Generations", anim->getGenerations(), 1, 1000 ),
        m_pixels_field( "Pixels", anim->getPixels(), 1, 64 ),
        m_increment_field( "Increment", anim->getIncrement(), 1, 64 ),
        m_color_fade_field( "Fade colors", anim->isFadeColors() ),
        m_color_progression_field( "Custom color progression (comma separated)", anim->getCustomColors() ),
        m_off_pixel_color_field( "Off pixel color", anim->getEmptyColor() ),
        m_combine_field( "Combine fixtures", anim->getCombineFixtures() )
    {
        m_effect_field.addKeyValue( 1, "Scrolling" );
        m_effect_field.addKeyValue( 2, "Stacked" );
        m_effect_field.addKeyValue( 3, "Stacked (Left)" );
        m_effect_field.addKeyValue( 4, "Stacked (Right)" );
        m_effect_field.addKeyValue( 5, "Beam" );
        m_effect_field.addKeyValue( 6, "Random" );
        m_effect_field.addKeyValue( 7, "Chase" );
        m_effect_field.setDefaultListValue( (DWORD)anim->getEffect() );

        add( m_effect_field );
        add( m_generations_field );
        add( m_pixels_field );
        add( m_increment_field );
        add( m_color_progression_field );
        add( m_off_pixel_color_field );
        add( m_color_fade_field );
        add( m_actor_select );
        add( m_combine_field );
    }

    void update( ScenePixelAnimator* anim ) {
        updateAnimationSignal( anim->signal() );

        anim->setEffect( (PixelEffect)m_effect_field.getListValue() );
        anim->setActors( m_actor_select.getActorUIDs() );
        anim->setGenerations( m_generations_field.getIntValue() );
        anim->setPixel( m_pixels_field.getIntValue() );
        anim->setIncrement( m_increment_field.getIntValue() );
        anim->setFadeColors( m_color_fade_field.isSet() );
        anim->setCombineFixtures( m_combine_field.isSet() );
        anim->setCustomColors( m_color_progression_field.getColors() );
        anim->setEmptyColor( m_off_pixel_color_field.getColor() );
    }

protected:
     void fieldLeaveNotify( size_t field_num ) {
         if ( getField<Field>(field_num) != &m_effect_field ) {
            SignalForm::fieldLeaveNotify( field_num );
            return;
        }

        switch ( (PixelEffect)m_effect_field.getListValue() ) {
            case MOVING:
                m_generations_field.setHidden( false );
                m_pixels_field.setHidden( false );
                m_increment_field.setHidden( false );
                m_color_fade_field.setHidden( false );
                break;

            case STACKED:
            case STACKED_LEFT:
            case STACKED_RIGHT:
                m_generations_field.setHidden( true );
                m_pixels_field.setHidden( true );
                m_increment_field.setHidden( true );
                m_color_fade_field.setHidden( true );
                break;
        
            case BEAM:
                m_generations_field.setHidden( true );
                m_pixels_field.setHidden( true );
                m_increment_field.setHidden( true );
                m_color_fade_field.setHidden( false );
                break;

            case RANDOM:
                m_generations_field.setHidden( false );
                m_pixels_field.setHidden( false );
                m_increment_field.setHidden( true );
                m_color_fade_field.setHidden( true );
                break;

            case CHASE:
                m_generations_field.setHidden( false );
                m_pixels_field.setHidden( false );
                m_increment_field.setHidden( false );
                m_color_fade_field.setHidden( true );
                break;
        }
     }
};

// ----------------------------------------------------------------------------
//
void DMXTextUI::sceneAddAnimation(void)
{
    SceneSelectField scene_field( "Scene", getVenue() );
    AnimationSelectField editor_field( "Animation" );

    Form form( &m_text_io, "Add Animation to Scene" );
    form.add( scene_field );
    form.add( editor_field );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );
    AnimationEditor* editor = editor_field.getAnimationEditor( );

    if ( !(this->*editor->m_handler)( scene, 0 ) )
        return;

    if ( scene->getUID() == getVenue()->getCurrentSceneUID() )
        getVenue()->loadScene();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::sceneUpdateAnimation(void)
{
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( field_num );
                SceneAnimationSelectField* anim_field = getField<SceneAnimationSelectField>( 1 );
                Scene* scene = m_venue->getScene( scene_field->getSceneUID() );
                anim_field->selectAnimations( scene );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) : 
            Form( input_stream, "Update Scene Animation" ),
            m_venue(venue) {}
    };

    SceneSelectField scene_field( "Scene", getVenue() );
    SceneAnimationSelectField animation_select_field( "Animation" );

    MyForm form( &m_text_io, getVenue() );
    form.add( scene_field );
    form.add( animation_select_field );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );
    AbstractAnimation* animation = scene->getAnimation( animation_select_field.getAnimationUID() );
    AnimationEditor* editor = NULL;

    for ( unsigned index=0; index < DMXTextUI::animEditors.size(); index++ ) {
        if ( strcmp( animEditors[index].m_classname, animation->getClassName() ) == 0 ) {
            editor = &DMXTextUI::animEditors[index];
            break;
        }
    }

    STUDIO_ASSERT( editor, "Missing animation editor for '%s'", animation->getClassName() );

    if ( !(this->*editor->m_handler)( scene, animation->getUID() ) )
        return;

    if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
        getVenue()->loadScene( );
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animSequencerEditor( Scene* scene, UID anim_uid )
{
    SceneSequence* anim;

    if ( anim_uid == 0 )
        anim = new SceneSequence( 0, AnimationSignal(), UIDArray() );
    else
        anim = reinterpret_cast<SceneSequence*>( scene->getAnimation( anim_uid ) );

    IntegerField sample_ms_field( "Sequence speed (ms)", anim->signal().getSampleRateMS(), 0 );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue(), scene, anim->getActors() );

    Form form( &m_text_io );
    form.add( sample_ms_field );
    form.add( actor_select );
    if ( !form.play() )
        return false;

    anim->signal().setSampleRateMS( sample_ms_field.getLongValue() );
    anim->setActors( actor_select.getActorUIDs() );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animSoundLevelEditor( Scene* scene, UID anim_uid )
{
    SceneSoundLevel* anim;

    if ( anim_uid == 0 )
        anim = new SceneSoundLevel( 0, AnimationSignal(), UIDArray(), FADE_ALL );
    else
        anim = reinterpret_cast<SceneSoundLevel*>( scene->getAnimation( anim_uid ) );

    NumberedListField fade_what_field( "Fade" );
    fade_what_field.addKeyValue( 1, "Colors" );
    fade_what_field.addKeyValue( 2, "Dimmers" );
    fade_what_field.addKeyValue( 3, "Colors & dimmers" );
    fade_what_field.setDefaultListValue( anim->getFadeWhat() );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue(), scene, anim->getActors() );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( fade_what_field );
    form.add( actor_select );
    if ( !form.play() )
        return false;

    anim->setFadeWhat( (WORD)fade_what_field.getListValue() );
    form.updateAnimationSignal( anim->signal() );
    anim->setActors( actor_select.getActorUIDs() );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animPatternDimmerEditor( Scene* scene, UID anim_uid )
{
    ScenePatternDimmer* anim;

    if ( anim_uid == 0 ) 
        anim = new ScenePatternDimmer( 0, AnimationSignal(), UIDArray(), DP_SEQUENCE );
    else
        anim = reinterpret_cast<ScenePatternDimmer*>( scene->getAnimation( anim_uid ) );

    NumberedListField fade_what_field( "Pattern" );
    fade_what_field.addKeyValue( 1, "Sequence" );
    fade_what_field.addKeyValue( 2, "Cylon" );
    fade_what_field.addKeyValue( 3, "Pairs" );
    fade_what_field.addKeyValue( 4, "To Center" );
    fade_what_field.addKeyValue( 5, "Alternate" );
    fade_what_field.addKeyValue( 6, "On/Off" );
    fade_what_field.setDefaultListValue( anim->getDimmerPattern() );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue(), scene, anim->getActors() );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( fade_what_field );
    form.add( actor_select );
    if ( !form.play() )
        return false;

    anim->setDimmerPattern( (DimmerPattern)(fade_what_field.getListValue()) );
    form.updateAnimationSignal( anim->signal() );
    anim->setActors( actor_select.getActorUIDs() );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animColorFaderEditor( Scene* scene, UID anim_uid )
{
    SceneColorFader* anim;

    if ( anim_uid == 0 )
        anim = new SceneColorFader( 0, AnimationSignal(), UIDArray(), RGBWA::BLACK, 200, 100, RGBWAArray(), FaderEffect::FADER_EFFECT_ALL );
    else
        anim = reinterpret_cast<SceneColorFader*>( scene->getAnimation( anim_uid ) );

    ColorSelectField color_progression_field( "Custom color progression (comma separated)", anim->getCustomColors() );
    ColorSelectField strobe_neg_color_field( "Negative strobe color", anim->getStrobeNegColor() );

    NumberedListField fader_mode_field( "Fader" );
    fader_mode_field.addKeyValue( 1, "Color change" );
    fader_mode_field.addKeyValue( 2, "Strobe" );
    fader_mode_field.addKeyValue( 3, "Color blend" );
    fader_mode_field.addKeyValue( 4, "All effects" );
    fader_mode_field.setDefaultListValue( anim->getFaderEffect() );

    IntegerField strobe_pos_ms_field( "Strobe on (ms)", anim->getStrobePosMS(),0, 32000 );
    IntegerField strobe_neg_ms_field( "Strobe off (ms)", anim->getStrobeNegMS(),0, 32000 );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue(), scene, anim->getActors() );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( fader_mode_field );
    form.add( color_progression_field );
    form.add( strobe_neg_color_field );
    form.add( strobe_pos_ms_field );
    form.add( strobe_neg_ms_field );
    form.add( actor_select );
    if ( !form.play() )
        return false;

    anim->setFaderEffect( (FaderEffect)fader_mode_field.getListValue() );
    anim->setCustomColors( color_progression_field.getColors() );
    anim->setStrobeNegColor( strobe_neg_color_field.getColor() );
    anim->setStrobePosMS( strobe_pos_ms_field.getLongValue() );
    anim->setStrobeNegMS( strobe_neg_ms_field.getLongValue() );
    form.updateAnimationSignal( anim->signal() );
    anim->setActors( actor_select.getActorUIDs() );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animStrobeEditor( Scene* scene, UID anim_uid )
{
    SceneColorFader* anim;

    if ( anim_uid == 0 )
        anim = new SceneStrobeAnimator( 0, AnimationSignal(), UIDArray(), RGBWA::BLACK, 200, 100 );
    else
        anim = reinterpret_cast<SceneStrobeAnimator*>( scene->getAnimation( anim_uid ) );

    ColorSelectField strobe_neg_color_field( "Negative strobe color", anim->getStrobeNegColor() );
    IntegerField strobe_pos_ms_field( "Strobe on (ms)", anim->getStrobePosMS(),0, 32000 );
    IntegerField strobe_neg_ms_field( "Strobe off (ms)", anim->getStrobeNegMS(),0, 32000 );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue(), scene, anim->getActors() );

    Form form( &m_text_io );
    form.add( strobe_neg_color_field );
    form.add( strobe_pos_ms_field );
    form.add( strobe_neg_ms_field );
    form.add( actor_select );
    if ( !form.play() )
        return false;

    anim->setStrobeNegColor( strobe_neg_color_field.getColor() );
    anim->setStrobePosMS( strobe_pos_ms_field.getLongValue() );
    anim->setStrobeNegMS( strobe_neg_ms_field.getLongValue() );
    anim->setActors( actor_select.getActorUIDs() );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animMovementEditor( Scene* scene, UID anim_uid )
{
    SceneMovementAnimator* anim;

    if ( anim_uid == 0 )
        anim = new SceneMovementAnimator( getVenue()->allocUID(), AnimationSignal(), UIDArray(), MovementAnimation() );
    else
        anim = reinterpret_cast<SceneMovementAnimator*>( scene->getAnimation( anim_uid ) );

    AnimationMovementForm form( &m_text_io, getVenue(), scene, anim );
    if ( !form.play() )
        return false;

    form.update( anim );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animChannelEditor( Scene* scene, UID anim_uid )
{
    SceneChannelAnimator* anim;

    if ( anim_uid == 0 )
        anim = new SceneChannelAnimator( getVenue()->allocUID(), AnimationSignal(), ChannelAnimationArray() );
    else
        anim = reinterpret_cast<SceneChannelAnimator*>( scene->getAnimation( anim_uid ) );

    SignalForm form( &m_text_io, anim->signal() );
    if ( !form.play() )
        return false;

    ChannelAnimationArray animations = anim->channelAnimations();
    bool appendAnimations = ( animations.size() == 0 );

    if ( !appendAnimations ) {					// Edit existing animations
        for ( size_t index=0; index < animations.size(); index++ ) {
            ChannelAnimationForm cform( &m_text_io, 
                                        getVenue(), 
                                        scene, 
                                        animations[index],
                                        false,
                                        index+1 == animations.size() );
            if ( !cform.play() )
                return false;

            cform.updateAnimation( animations[index] );

            if ( index+1 == animations.size() && cform.addAnotherChannel() )
                appendAnimations = true;
        }				
    }

    if ( appendAnimations ) {					// New animation list (allows mutiple fixtures)
        while ( true ) {
            ChannelAnimationForm cform( &m_text_io, 
                                        getVenue(), 
                                        scene, 
                                        ChannelAnimation( 0, 0, CAM_SCALE, ChannelValueArray() ),
                                        true,
                                        true );
            if ( !cform.play() )
                return false;

            cform.appendAnimations( animations );

            if ( !cform.addAnotherChannel() )
                break;
        }			
    }
    
    form.updateAnimationSignal( anim->signal() );
    anim->setChannelAnimations( animations );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::animPixelEditor( Scene* scene, UID anim_uid )
{
    ScenePixelAnimator* anim;

    if ( anim_uid == 0 ) 
        anim = new ScenePixelAnimator( 0, AnimationSignal(), UIDArray(), PixelEffect::BEAM, 
                                       RGBWAArray(), RGBWA::BLACK, 5, 1, true, 2, true );
    else
        anim = reinterpret_cast<ScenePixelAnimator*>( scene->getAnimation( anim_uid ) );

    PixelAnimationForm form( &m_text_io, getVenue(), scene, anim );
    if ( !form.play() )
        return false;

    form.update( anim );

    if ( anim_uid == 0 ) {
        anim->setUID( getVenue()->allocUID() );
        scene->addAnimation( anim );
    }

    return true;
}