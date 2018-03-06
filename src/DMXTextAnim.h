/* 
Copyright (C) 2011-2016 Robert DeSantis
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

#pragma once

#include "DMXTextUI.h"

// ----------------------------------------------------------------------------
//
class SignalForm : public Form
{
    NumberedListField	m_trigger;
    NumberedListField	m_source;
    IntegerField		m_timer;
    IntegerField		m_off;
    IntegerField		m_level_floor;
    IntegerField		m_level_ceil;
    BooleanField        m_level_invert;
    IntegerField		m_level_scale;
    IntegerField        m_level_periods;
    IntegerField		m_freq_low;
    IntegerField		m_freq_high;
    NumberedListField	m_timer_speed;
    IntegerField		m_level_hold;
    IntegerField		m_sensitivity;

public:
    SignalForm( TextIO* input_stream, AnimationSignal& signal ) :
        Form( input_stream, "Animation Signal Trigger and Level" ),
        m_trigger( "Signal trigger" ),
        m_source( "Signal source" ),
        m_timer( "Timer MS", signal.getTimerMS(), 1, 60000 ),
        m_off( "Source level reset timer MS (0 = disabled)", signal.getOffMS(), 0, 60000 ),
        m_level_floor( "Level minimum", signal.getLevelFloor(), 0, MAXIMUM_LEVEL ),
        m_level_ceil( "Level maximum", signal.getLevelCeil(), 0, MAXIMUM_LEVEL ),
        m_level_invert( "Invert level", signal.isLevelInvert() ),
        m_level_scale( "Level scale (1=linear)", signal.getLevelScale(), 1, 10 ),
        m_level_periods( "Number of wave periods", signal.getLevelPeriods(), 1, 1000 ),
        m_level_hold( "Number of hold periods", signal.getLevelHold(), 0, 1000 ),
        m_freq_low( "Frequence low", signal.getFreqLowHz(), 0, 22000 ),
        m_freq_high( "Frequence high", signal.getFreqHighHz(), 0, 22000 ),                
        m_timer_speed( "Timer speed" ),
        m_sensitivity( "Sensitivity", signal.getSensitivity(), 0, 100 )
    {
        m_trigger.addKeyValue( 1, "Timer" );
        m_trigger.addKeyValue( 2, "Frequency Beat" );
        m_trigger.addKeyValue( 3, "Amplitude Beat" );
        m_trigger.addKeyValue( 4, "Random Timer" );
        m_trigger.setDefaultListValue( signal.getTrigger() );

        m_source.addKeyValue( SignalSource::SOURCE_HIGH, "Always High" );
        m_source.addKeyValue( SignalSource::SOURCE_AMPLITUDE, "Amplitude" );
        m_source.addKeyValue( SignalSource::SOURCE_AVG_AMPLITUDE, "Average amplitude" );
        m_source.addKeyValue( SignalSource::SOURCE_BEAT_INTENSITY, "Amplitude beat intensity" );
        m_source.addKeyValue( SignalSource::SOURCE_FREQ, "Frequency" );
        m_source.addKeyValue( SignalSource::SOURCE_VOLUME, "Master Volume" );
        m_source.addKeyValue( SignalSource::SOURCE_LOW, "Always Low" ),
        m_source.addKeyValue( SignalSource::SOURCE_SQUAREWAVE, "Squarewave" );
        m_source.addKeyValue( SignalSource::SOURCE_SAWTOOTH, "Sawtooth" );
        m_source.addKeyValue( SignalSource::SOURCE_SINEWAVE, "Sinewave" );
        m_source.addKeyValue( SignalSource::SOURCE_TRIANGLEWAVE, "Triangle wave" );
        m_source.addKeyValue( SignalSource::SOURCE_STEPWAVE, "Step Wave (half)" );
        m_source.addKeyValue( SignalSource::SOURCE_STEPWAVE_FULL, "Step Wave (full)" );
        m_source.addKeyValue( SignalSource::SOURCE_RANDOM, "Random value" );
        m_source.addKeyValue( SignalSource::SOURCE_SOUND, "Sound Detect" );

        m_source.setDefaultListValue( signal.getSource() );

        m_timer_speed.addKeyValue( 1, "Normal" );
        m_timer_speed.addKeyValue( 2, "Level Increases" );
        m_timer_speed.addKeyValue( 3, "Level Reduces" );
        m_timer_speed.setDefaultListValue( signal.getSpeedAdjust() );

        add( m_trigger );
        add( m_source );
        add( m_timer_speed );
        add( m_timer );
        add( m_off ),
        add( m_freq_low );
        add( m_freq_high ); 
        add( m_level_floor );
        add( m_level_ceil );
        add( m_level_invert );
        add( m_level_scale );
        add( m_level_periods );   
        add( m_level_hold ); 
        add( m_sensitivity );
    }

    void updateAnimationSignal( AnimationSignal& signal ) {
        signal.setTrigger( (SignalTrigger)m_trigger.getListValue() );
        signal.setSource( (SignalSource)m_source.getListValue() );
        signal.setSpeedAdjust( (SpeedAdjust)m_timer_speed.getListValue() );
        signal.setTimerMS( m_timer.getLongValue() );
        signal.setOffMS( m_off.getLongValue() );
        signal.setLevelFloor( m_level_floor.getIntValue() );
        signal.setLevelCeil( m_level_ceil.getIntValue() );
        signal.setLevelInvert( m_level_invert.isSet() );
        signal.setLevelScale( m_level_scale.getIntValue() );
        signal.setLevelPeriods( m_level_periods.getIntValue() );
        signal.setFreqLowHz( m_freq_low.getLongValue() );
        signal.setFreqHighHz( m_freq_high.getLongValue() );
        signal.setLevelHold( m_level_hold.getIntValue() );
        signal.setSensitivity( m_sensitivity.getIntValue() );
    }

protected:
    void fieldLeaveNotify( size_t field_num ) {
        if ( getField<Field>(field_num) != &m_timer_speed )
            return;

        m_timer.setHidden( true );
        m_freq_low.setHidden( true );
        m_freq_high.setHidden( true );
        m_level_periods.setHidden( true );
        m_level_hold.setHidden( true );
        m_sensitivity.setHidden( true );

        switch ( (SignalTrigger)m_trigger.getListValue() ) {
            case TRIGGER_TIMER:
            case TRIGGER_RANDOM_TIMER:
                m_timer.setHidden( false );
                break;

            case TRIGGER_FREQ_BEAT:
                m_freq_low.setHidden( false );
                m_freq_high.setHidden( false );
                break;

            case TRIGGER_AMPLITUDE_BEAT:
                m_sensitivity.setHidden( false );

            default:
                break;
        }

        switch ( (SignalSource)m_source.getListValue() ) {
            case SOURCE_SQUAREWAVE:
            case SOURCE_SAWTOOTH:
            case SOURCE_TRIANGLEWAVE:
            case SOURCE_SINEWAVE:
            case SOURCE_STEPWAVE:
            case SOURCE_STEPWAVE_FULL:
                m_level_periods.setHidden( false );
                m_level_hold.setHidden( false );
                break;

            case SOURCE_RANDOM:
                m_level_periods.setHidden( false );
                break;

            case SOURCE_AMPLITUDE:
            case SOURCE_AVG_AMPLITUDE:
            case SOURCE_BEAT_INTENSITY:
            case SOURCE_FREQ:
                m_level_hold.setHidden( false );
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

    BooleanField			m_more_field;
    FixtureChannelField		m_channel_field;
    NumberedListField		m_value_anim_field;
    IntegerField			m_range_low_field;
    IntegerField			m_range_high_field;
    ChannelValueListField	m_value_list_field;

public:
    ChannelAnimationForm( TextIO* input_stream, Venue* venue, ChannelAnimation& animation, 
            bool show_more, UID reference_fixture ) :
            Form( input_stream, "Define/Update Channel Animation" ),
        m_venue( venue ),
        m_more_field( "Add another channel", false ),
        m_channel_field( "Channel", venue, reference_fixture, animation.getChannel() ),
        m_value_anim_field( "Value animation" ),
        m_range_low_field( "Range low value (inclusive)", 0, 0, 255 ),
        m_range_high_field( "Range high value (inclusive)", 0, 0, 255 ),
        m_value_list_field( "Channel values (comma separated)", animation.getChannelValues() )
    {
        m_value_anim_field.addKeyValue( 1, "Value List" );
        m_value_anim_field.addKeyValue( 2, "Value Range" );
        m_value_anim_field.addKeyValue( 3, "Scale Scene Value" );
        m_value_anim_field.addKeyValue( 4, "Signal Level" );
        m_value_anim_field.setDefaultListValue( animation.getAnimationStyle() );

        add( m_channel_field );
        add( m_value_anim_field );
        add( m_range_low_field );
        add( m_range_high_field );
        add( m_value_list_field );
        add( m_more_field );

        m_more_field.setHidden( !show_more );

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
        ChannelAnimation animation( 0, CAM_SCALE, ChannelValueArray() );
        updateAnimation( animation );
        animations.push_back( animation );
    }

protected:
    void fieldLeaveNotify( size_t field_num ) {
        if ( getField<Field>(field_num) == &m_value_anim_field ) {
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
            case CAM_LEVEL:
                break;
            }
        }
    }
};

// ----------------------------------------------------------------------------
//
class AnimationMovementForm : public SignalForm
{
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
    IntegerField		m_head_number_field;

public:
    AnimationMovementForm( TextIO* text_io, Venue* venue, SceneMovementAnimator* anim ) :
        SignalForm( text_io, anim->signal() ),
        m_movement_field( "Movement" ),
        m_head_number_field( "Head number (0=all)", anim->movement().m_head_number, 0, 32 ),
        m_movement_speed_field( "Movement speed (0=fastest)",  anim->movement().m_speed, 0, 255 ),
        m_tilt_start_field( "Start tilt angle (degrees)", anim->movement().m_tilt_start, 0, 360 ),
        m_tilt_end_field( "End tilt angle (degrees)", anim->movement().m_tilt_end, 0, 360 ),
        m_pan_start_field( "Start pan angle (degrees)", anim->movement().m_pan_start, 0, 540 ),
        m_pan_end_field( "End pan angle (degrees)", anim->movement().m_pan_end, 0, 540 ),
        m_pan_inc_field( "Pan increment (degrees)", anim->movement().m_pan_increment, 0, 540 ),
        m_home_wait_field( "Home wait (periods)", anim->movement().m_home_wait_periods, 1, MAX_WAIT_PERIODS ),
        m_dest_wait_field( "Destination wait (periods)", anim->movement().m_dest_wait_periods, 1, MAX_WAIT_PERIODS ),
        m_beam_return_field( "Blackout on return to home", anim->movement().m_backout_home_return ),
        m_positions_field( "Postions to generate / Sinewave step", anim->movement().m_positions, 1, MAX_RANDOM_POSITIONS ),
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
        m_movement_field.addKeyValue( 8, "Sine wave" );
        m_movement_field.setDefaultListValue( anim->movement().m_movement_type );

        add( m_movement_field );
        add( m_head_number_field );
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
        add( m_positions_field );
        add( m_home_wait_field );
        add( m_dest_wait_field );
        add( m_beam_return_field );
        add( m_group_size_field );
        add( m_alternate_groups_field );
        add( m_coordinates_field );
        add( m_run_once_field );
    }

    void update( SceneMovementAnimator* anim ) {
        updateAnimationSignal( anim->signal() );

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
        anim->movement().m_head_number = m_head_number_field.getIntValue();
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
        m_head_number_field.setHidden( false );

        m_pan_inc_field.setLabel( "Pan increment (degrees)" );
        m_positions_field.setLabel( "Postions to generate" );
        m_beam_return_field.setLabel( "Blackout on return to home" );

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
            m_beam_return_field.setHidden( false );
            m_beam_return_field.setLabel( "Blackout movement step (adds 1 period)" );
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

        case MOVEMENT_SINEWAVE:
            m_positions_field.setHidden( false );
            m_group_size_field.setHidden( false );
            m_tilt_end_field.setHidden( false );
            m_pan_start_field.setHidden( false );
            m_pan_end_field.setHidden( false );
            m_pan_inc_field.setHidden( false );

            m_pan_inc_field.setLabel( "Sine wave offset" );
            m_positions_field.setLabel( "Sine wave step" );
            break;
        }
    }
};

// ----------------------------------------------------------------------------
//
class PixelAnimationForm : public SignalForm
{
    NumberedListField	m_effect_field;
    IntegerField        m_generations_field;
    IntegerField        m_pixels_field;
    IntegerField        m_increment_field;
    BooleanField        m_combine_field;
    BooleanField        m_color_fade_field;
    ColorSelectField    m_color_progression_field;
    ColorSelectField    m_off_pixel_color_field;

public:
    PixelAnimationForm( TextIO* text_io, Venue* venue,ScenePixelAnimator* anim ) :
        m_effect_field( "Pixel Effect" ),
        SignalForm( text_io, anim->signal() ),
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
        m_effect_field.addKeyValue( 8, "Coplor Chase" );
        m_effect_field.setDefaultListValue( (DWORD)anim->getEffect() );

        add( m_effect_field );
        add( m_generations_field );
        add( m_pixels_field );
        add( m_increment_field );
        add( m_color_progression_field );
        add( m_off_pixel_color_field );
        add( m_color_fade_field );
        add( m_combine_field );
    }

    void update( ScenePixelAnimator* anim ) {
        updateAnimationSignal( anim->signal() );

        anim->setEffect( (PixelEffect)m_effect_field.getListValue() );
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

        case COLOR_CHASE:
            m_generations_field.setHidden( true );
            m_pixels_field.setHidden( false );
            m_increment_field.setHidden( true );
            m_color_fade_field.setHidden( true );
            break;
        }
    }
};

// ----------------------------------------------------------------------------
//
class FilterAnimationForm : public SignalForm
{
    Venue*                  m_venue;
    NumberedListField	    m_filter_field;
    FixtureChannelsField    m_channel_field;
    IntegerField            m_step_field;
    IntegerField            m_amplitude_field;
    IntegerField            m_offset_field;

public:
    FilterAnimationForm( TextIO* text_io, Venue* venue, SceneChannelFilter* anim ) :
        m_venue( venue ),
        m_filter_field( "Channel Filter" ),
        SignalForm( text_io, anim->signal() ),
        m_channel_field( "Channels (comma separated)", NULL, anim->getChannels() ),
        m_step_field( "Step", anim->getStep(), 1, 255 ),
        m_amplitude_field( "Amplitude", anim->getAmplitude(), 1, 255 ),
        m_offset_field( "Offset", anim->getOffset(), 0, 360 )
    {
        m_filter_field.addKeyValue( CF_SINE_WAVE, "Sine Wave" );
        m_filter_field.addKeyValue( CF_RAMP_UP, "Ramp Up" );
        m_filter_field.addKeyValue( CF_RAMP_DOWN, "Ramp Down" );
        m_filter_field.addKeyValue( CF_STEP_WAVE, "Step Wave" );
        m_filter_field.addKeyValue( CF_RANDOM, "Random" );

        m_filter_field.setDefaultListValue( (DWORD)anim->getFilter() );

        add( m_filter_field );
        add( m_channel_field );
        add( m_step_field );
        add( m_amplitude_field );
        add( m_offset_field );
    }

    void update( SceneChannelFilter* anim ) {
        updateAnimationSignal( anim->signal() );

        anim->setFilter( (ChannelFilter)m_filter_field.getListValue() );
        anim->setChannels( m_channel_field.getChannels() );
        anim->setStep( m_step_field.getIntValue() );
        anim->setAmplitude( m_amplitude_field.getIntValue() );
        anim->setOffset( m_offset_field.getIntValue() );
    }

protected:
    void fieldLeaveNotify( size_t field_num ) {
        if ( getField<Field>(field_num) != &m_filter_field ) {
            SignalForm::fieldLeaveNotify( field_num );
            return;
        }

        switch ( (ChannelFilter)m_filter_field.getListValue() ) {
        case CF_SINE_WAVE:   			       // Sine wave (amplitude, angle step)
            m_step_field.setHidden( false );
            m_amplitude_field.setHidden( false );
            m_amplitude_field.setLabel( "Amplitude" );
            m_offset_field.setHidden( false );
            break;

        case CF_RAMP_UP:                       // Ramp up (step)
            m_step_field.setHidden( false );
            m_amplitude_field.setHidden( false );
            m_amplitude_field.setLabel( "Maximum" );
            m_offset_field.setHidden( false );
            break;

        case CF_RAMP_DOWN:                      // Ramp down (step)
            m_step_field.setHidden( false );
            m_amplitude_field.setHidden( false );
            m_amplitude_field.setLabel( "Minimum" );
            m_offset_field.setHidden( false );
            break;

        case CF_STEP_WAVE:                    // Step (step)
            m_step_field.setHidden( false );
            m_amplitude_field.setHidden( true );
            m_offset_field.setHidden( false );
            break;

        case CF_RANDOM:                       // Random (amplitude)
            m_step_field.setHidden( true );
            m_amplitude_field.setHidden( false );
            m_amplitude_field.setLabel( "Amplitude" );
            m_offset_field.setHidden( false );
            break;
        }
    }
};

