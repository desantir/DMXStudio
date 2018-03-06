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

#include "DMXTextAnim.h"

static AnimationEditorArray animEditors;

AnimationEditorArray DMXTextUI::animEditors;

void DMXTextUI::initializeAnimationEditors() {

    animEditors.emplace_back( SceneFixtureDimmer::animationName,    SceneFixtureDimmer::className, &DMXTextUI::animFixtureDimmerEditor );
    animEditors.emplace_back( SceneSoundLevel::animationName,		SceneSoundLevel::className, &DMXTextUI::animSoundLevelEditor );
    animEditors.emplace_back( ScenePatternDimmer::animationName,	ScenePatternDimmer::className, &DMXTextUI::animPatternDimmerEditor );
    animEditors.emplace_back( SceneChannelAnimator::animationName,	SceneChannelAnimator::className, &DMXTextUI::animChannelEditor );
    animEditors.emplace_back( SceneColorFader::animationName,		SceneColorFader::className, &DMXTextUI::animColorFaderEditor );
    animEditors.emplace_back( SceneMovementAnimator::animationName,	SceneMovementAnimator::className, &DMXTextUI::animMovementEditor );
    animEditors.emplace_back( SceneStrobeAnimator::animationName,	SceneStrobeAnimator::className, &DMXTextUI::animStrobeEditor );
    animEditors.emplace_back( ScenePixelAnimator::animationName,	ScenePixelAnimator::className, &DMXTextUI::animPixelEditor );
    animEditors.emplace_back( SceneChannelFilter::animationName,	SceneChannelFilter::className, &DMXTextUI::animFilterEditor );
    animEditors.emplace_back( ScenePulse::animationName,	        ScenePulse::className, &DMXTextUI::animPulseEditor );
    animEditors.emplace_back( SceneCueAnimator::animationName,      SceneCueAnimator::className, &DMXTextUI::animCueEditor );
    animEditors.emplace_back( SceneSequence::animationName,	        SceneSequence::className, &DMXTextUI::animSequencerEditor );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describeAnimation(void) {
    AnimationSelectField animation_field( "Animation to describe", getVenue() );

    Form form( &m_text_io );
    form.add( animation_field );

    if ( form.play() ) {
        AnimationDefinition* animation = getVenue()->getAnimation( animation_field.getAnimationUID() );

        if ( animation->isShared() ) 
            m_text_io.printf( "Animation %d: %s (ID %d)\n", animation->getNumber(), animation->getName(), animation->getUID());
        else
            m_text_io.printf( "Private Animation: %s (ID %d)\n", animation->getName(), animation->getUID());

        CString animimation_synopsis = animation->getSynopsis();
        if ( animimation_synopsis.GetLength() > 0 ) {
            if ( animimation_synopsis[animimation_synopsis.GetLength()-1] == '\n' )
                animimation_synopsis = animimation_synopsis.Mid( 0, animimation_synopsis.GetLength()-1 );
            animimation_synopsis.Replace( "\n", "\n      " );
            m_text_io.printf( "      %s\n", (LPCTSTR)animimation_synopsis );
        }

        m_text_io.printf( "      Signal( %s )\n", (LPCTSTR)animation->signal().getSynopsis() );
        m_text_io.printf( "\n" );

        m_text_io.printf( "Acts:\n      " );
        if ( !animation->getActs().empty() ) {
            for ( ActNumber act : animation->getActs() )
                m_text_io.printf( "%u ", act );
        }
        else
            m_text_io.printf( "Default Act" );

        m_text_io.printf( "\n" );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteAnimation(void) {
    AnimationSelectField animation_field( "Animation to delete", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( animation_field );
    if ( form.play() )
        getVenue()->deleteAnimation( animation_field.getAnimationUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createAnimation(void) {
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 1 ) {
                getField<UniqueNumberField<AnimationNumber, AnimationDefinition>>( 2 )->setHidden( !getField<BooleanField>( 1 )->isSet() );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream, "Add Animation" ),
            m_venue(venue)
        {
            setAutoDelete( true );

            AnimationNumber nextUnique = m_venue->nextAvailableAnimationNumber();

            CString defaultName;
            defaultName.Format( "New Animation %lu", nextUnique );

            addAuto( new InputField( "New animation name", defaultName ) );
            addAuto( new BooleanField( "Shared", true ) );
            addAuto( new UniqueNumberField<AnimationNumber, AnimationDefinition>( "Animation number", m_venue, &Venue::getAnimationByNumber, nextUnique ) );
            addAuto( new AnimationTypeSelectField( "Type" ) );
            addAuto( new InputField( "Description", "" ) );
            addAuto( new ActorSelectField( "Reference fixture", m_venue, true ) );
            addAuto( new ActsField() );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    UID reference_uid = form.getField<ActorSelectField>( 5 )->getActorUID();

    AnimationEditor* editor = form.getField<AnimationTypeSelectField>( 3 )->getAnimationEditor( );

    AnimationDefinition* animation = (this->*editor->m_handler)( NULL, reference_uid );
    if ( animation == NULL )
        return;

    bool isShared = form.getField<BooleanField>( 1 )->isSet();

    animation->setName( form.getField<InputField>( 0 )->getValue() );
    animation->setShared( isShared );
    animation->setAnimationNumber( !isShared ? 0 : form.getField<UniqueNumberField<AnimationNumber, AnimationDefinition>>( 2 )->getNumber() );
    animation->setDescription( form.getField<InputField>( 4 )->getValue() );
    animation->setReferenceFixtureId( reference_uid );
    animation->setActs( form.getField<ActsField>( 6 )->getActs() );

    getVenue()->addAnimation( animation );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateAnimation(void) {
    AnimationSelectField animation_select_field( "Animation to update", getVenue() );

    Form select_animation_form( &m_text_io );
    select_animation_form.add( animation_select_field );
    if ( !select_animation_form.play() )
        return;

    AnimationDefinition* animation = getVenue()->getAnimation( animation_select_field.getAnimationUID() );

    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 1 ) {
                getField<UniqueNumberField<AnimationNumber, AnimationDefinition>>( 2 )->setHidden( !getField<BooleanField>( 1 )->isSet() );
            }
        }

    public:
        MyForm( AnimationDefinition* animation, TextIO* input_stream, Venue* venue ) :
            Form( input_stream, "Add Animation" ),
            m_venue(venue)
        {
            setAutoDelete( true );

            addAuto( new InputField( "Name", animation->getName() ) );
            addAuto( new BooleanField( "Shared animation", true ) );
            addAuto( new UniqueNumberField<AnimationNumber, AnimationDefinition>( "Animation number", m_venue, &Venue::getAnimationByNumber, animation->getNumber() ) );
            addAuto( new InputField( "Description", animation->getDescription() ) );
            addAuto( new ActorSelectField( "Reference fixture", m_venue, true ) );
            addAuto( new ActsField( animation->getActs() ) );

            getField<ActorSelectField>( 4 )->setActor( animation->getReferenceFixtureId() );
        }
    };

    MyForm form( animation, &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    AnimationEditor* editor = NULL;

    for ( unsigned index=0; index < DMXTextUI::animEditors.size(); index++ ) {
        if ( strcmp( animEditors[index].m_classname, animation->getClassName() ) == 0 ) {
            editor = &DMXTextUI::animEditors[index];
            break;
        }
    }

    STUDIO_ASSERT( editor, "Missing animation editor for '%s'", animation->getClassName() );

    UID reference_uid = form.getField<ActorSelectField>( 4 )->getActorUID();

    animation = (this->*editor->m_handler)( animation, reference_uid );
    if ( animation == NULL )
        return;

    bool isShared = form.getField<BooleanField>( 1 )->isSet();

    animation->setName( form.getField<InputField>( 0 )->getValue() );
    animation->setShared( isShared );
    animation->setAnimationNumber( isShared ? 0 : form.getField<UniqueNumberField<AnimationNumber, AnimationDefinition>>( 2 )->getNumber() );
    animation->setDescription( form.getField<InputField>( 3 )->getValue() );
    animation->setReferenceFixtureId( reference_uid );
    animation->setActs( form.getField<ActsField>( 5 )->getActs() );

    getVenue()->replaceAnimation( animation );
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animSequencerEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneSequence> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneSequence( NOUID, false, NOUID, AnimationSignal() ) );
    else
        anim.reset( new SceneSequence( *reinterpret_cast<SceneSequence*>( current_animation ) ) );

    SignalForm form( &m_text_io, anim->signal() );
    if ( !form.play() )
        return false;

    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animSoundLevelEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneSoundLevel> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneSoundLevel( NOUID, false, NOUID, AnimationSignal(), FADE_ALL ) );
    else
        anim.reset( new SceneSoundLevel( *reinterpret_cast<SceneSoundLevel*>( current_animation ) ) );

    NumberedListField fade_what_field( "Fade" );
    fade_what_field.addKeyValue( 1, "Colors" );
    fade_what_field.addKeyValue( 2, "Dimmers" );
    fade_what_field.addKeyValue( 3, "Colors & dimmers" );
    fade_what_field.setDefaultListValue( anim->getFadeWhat() );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( fade_what_field );
    if ( !form.play() )
        return false;

    anim->setFadeWhat( (WORD)fade_what_field.getListValue() );
    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animPatternDimmerEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<ScenePatternDimmer> anim;

    if ( current_animation == NULL )
        anim.reset( new ScenePatternDimmer( NOUID, false, NOUID, AnimationSignal(), DP_SEQUENCE ) );
    else
        anim.reset( new ScenePatternDimmer( *reinterpret_cast<ScenePatternDimmer*>( current_animation ) ) );

    NumberedListField fade_what_field( "Pattern" );
    fade_what_field.addKeyValue( 1, "Sequence" );
    fade_what_field.addKeyValue( 2, "Cylon" );
    fade_what_field.addKeyValue( 3, "Pairs" );
    fade_what_field.addKeyValue( 4, "To Center" );
    fade_what_field.addKeyValue( 5, "Alternate" );
    fade_what_field.addKeyValue( 6, "On/Off" );
    fade_what_field.addKeyValue( 7, "Random" );
    fade_what_field.addKeyValue( 8, "Ramp Up" );
    fade_what_field.addKeyValue( 9, "Ramp Up/Down" );
    fade_what_field.addKeyValue( 10, "Random Ramp Up" );
    fade_what_field.setDefaultListValue( anim->getDimmerPattern() );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( fade_what_field );
    if ( !form.play() )
        return false;

    anim->setDimmerPattern( (DimmerPattern)(fade_what_field.getListValue()) );
    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animFixtureDimmerEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneFixtureDimmer> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneFixtureDimmer( NOUID, false, NOUID, AnimationSignal(), DM_RAMP_UP, 0, 100 ) );
    else
        anim.reset( new SceneFixtureDimmer( *reinterpret_cast<SceneFixtureDimmer*>( current_animation ) ) );

    NumberedListField dimmer_mode_field( "Dimmer" );
    dimmer_mode_field.addKeyValue( 1, "Ramp Up" );
    dimmer_mode_field.addKeyValue( 2, "Ramp Down" );
    dimmer_mode_field.addKeyValue( 3, "Breath" );
    dimmer_mode_field.addKeyValue( 4, "Random" );
    dimmer_mode_field.setDefaultListValue( anim->getDimmerMode() );

    IntegerField min_percent_field( "Minimum dimmer percent", anim->getMinPercent(), 0, 100 );
    IntegerField max_percent_field( "Maximum dimmer percent", anim->getMaxPercent(), 0, 100 );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( dimmer_mode_field );
    form.add( min_percent_field );
    form.add( max_percent_field );
    if ( !form.play() )
        return false;

    anim->setDimmerMode( (DimmerMode)(dimmer_mode_field.getListValue()) );
    anim->setMinPercent( min_percent_field.getIntValue() );
    anim->setMaxPercent( max_percent_field.getIntValue() );

    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animColorFaderEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneColorFader> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneColorFader( NOUID, false, NOUID, AnimationSignal(), StrobeTime( 200, 100, 0, 0, 1 ), RGBWA::BLACK, RGBWAArray(), FaderEffect::FADER_EFFECT_ALL ) );
    else
        anim.reset( new SceneColorFader( *reinterpret_cast<SceneColorFader*>( current_animation ) ) );

    ColorSelectField color_progression_field( "Custom color progression (comma separated)", anim->getCustomColors() );
    ColorSelectField strobe_neg_color_field( "Strobe off color", anim->getStrobeNegColor() );

    NumberedListField fader_mode_field( "Fader" );
    fader_mode_field.addKeyValue( 1, "Color change (single)" );
    fader_mode_field.addKeyValue( 2, "Color change (multiple)" );
    fader_mode_field.addKeyValue( 3, "Strobe" );
    fader_mode_field.addKeyValue( 4, "Color blend (single)" );
	fader_mode_field.addKeyValue( 5, "Color blend (multiple)" );
    fader_mode_field.addKeyValue( 6, "All effects" );

    fader_mode_field.setDefaultListValue( anim->getFaderEffect() );

    IntegerField strobe_pos_ms_field( "Strobe on (ms)", anim->getStrobeTime().getOnMS(),0, 32000 );
    IntegerField strobe_fade_in_ms_field( "Strobe fade in (ms)", anim->getStrobeTime().getFadeInMS(), 0, 32000 );
    IntegerField strobe_neg_ms_field( "Strobe off (ms)", anim->getStrobeTime().getOffMS(),0, 32000 );
    IntegerField strobe_fade_out_ms_field( "Strobe fade out (ms)", anim->getStrobeTime().getFadeOutMS(), 0, 32000 );
    IntegerField strobe_flashes_field( "Strobe flashes", anim->getStrobeTime().getFlashes(), 1, 50 );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( fader_mode_field );
    form.add( color_progression_field );
    form.add( strobe_neg_color_field );
    form.add( strobe_pos_ms_field );
    form.add( strobe_fade_in_ms_field );
    form.add( strobe_neg_ms_field );
    form.add( strobe_fade_out_ms_field );
    form.add( strobe_flashes_field );
    if ( !form.play() )
        return false;

	StrobeTime strobe_time( strobe_pos_ms_field.getLongValue(), strobe_neg_ms_field.getLongValue(),
		strobe_fade_in_ms_field.getIntValue(), strobe_fade_out_ms_field.getIntValue(),
		strobe_flashes_field.getIntValue() );

    anim->setFaderEffect( (FaderEffect)fader_mode_field.getListValue() );
    anim->setCustomColors( color_progression_field.getColors() );
    anim->setStrobeNegColor( strobe_neg_color_field.getColor() );
    anim->setStrobeTime( strobe_time );
    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animStrobeEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneStrobeAnimator> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneStrobeAnimator( NOUID, false, NOUID, AnimationSignal(), STROBE_SIMULATED, 1, StrobeTime( 200, 100, 0, 0, 1 ), RGBWA::WHITE, RGBWA::BLACK ) );
    else
        anim.reset( new SceneStrobeAnimator( *reinterpret_cast<SceneStrobeAnimator*>( current_animation ) ) );

	NumberedListField strobe_type_field( "Strobe mode" );
	strobe_type_field.addKeyValue( STROBE_SIMULATED, "Simulated" );
	strobe_type_field.addKeyValue( STROBE_FIXTURE, "Fixture" );
	strobe_type_field.setDefaultListValue( anim->getStrobeType() );

	IntegerField strobe_percent_field( "Hardware strobe percent", anim->getStrobePercent(), 1, 100 );
	ColorSelectField strobe_color_field( "Stobe color", anim->getStrobeColor() );
    ColorSelectField strobe_neg_color_field( "Stobe off color", anim->getStrobeNegColor() );
    IntegerField strobe_pos_ms_field( "Strobe on (ms)", anim->getStrobeTime().getOnMS(), 0, 32000 );
    IntegerField strobe_fade_in_ms_field( "Strobe fade in (ms)", anim->getStrobeTime().getFadeInMS(), 0, 32000 );
    IntegerField strobe_neg_ms_field( "Strobe off (ms)", anim->getStrobeTime().getOffMS(),0, 32000 );
    IntegerField strobe_fade_out_ms_field( "Strobe fade out (ms)", anim->getStrobeTime().getFadeOutMS(), 0, 32000 );
    IntegerField strobe_flashes_field( "Strobe flashes", anim->getStrobeTime().getFlashes(), 1, 50 );

    Form form( &m_text_io );
	form.add( strobe_type_field );
	form.add( strobe_percent_field );
	form.add( strobe_color_field );
    form.add( strobe_neg_color_field );
    form.add( strobe_pos_ms_field );
    form.add( strobe_fade_in_ms_field );
    form.add( strobe_neg_ms_field );
    form.add( strobe_fade_out_ms_field );
    form.add( strobe_flashes_field );
    if ( !form.play() )
        return false;

	StrobeTime strobe_time( strobe_pos_ms_field.getLongValue(), strobe_neg_ms_field.getLongValue(),
		strobe_fade_in_ms_field.getIntValue(), strobe_fade_out_ms_field.getIntValue(),
		strobe_flashes_field.getIntValue() );

	anim->setStrobeType( (StrobeType)strobe_type_field.getListValue() );
	anim->setStrobePercent( strobe_percent_field.getIntValue() );
	anim->setStrobeColor( strobe_color_field.getColor() );
    anim->setStrobeNegColor( strobe_neg_color_field.getColor() );
    anim->setStrobeTime( strobe_time );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animMovementEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneMovementAnimator> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneMovementAnimator( NOUID, false, NOUID, AnimationSignal(), MovementAnimation() ) );
    else
        anim.reset( new SceneMovementAnimator( *reinterpret_cast<SceneMovementAnimator*>( current_animation ) ) );

    AnimationMovementForm form( &m_text_io, getVenue(), anim.get() );
    if ( !form.play() )
        return false;

    form.update( anim.get() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animChannelEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneChannelAnimator> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneChannelAnimator( NOUID, false, NOUID, AnimationSignal(), ChannelAnimationArray() ) );
    else
        anim.reset( new SceneChannelAnimator( *reinterpret_cast<SceneChannelAnimator*>( current_animation ) ) );

    SignalForm form( &m_text_io, anim->signal() );
    if ( !form.play() )
        return false;

    ChannelAnimationArray animations = anim->channelAnimations();
    bool appendAnimations = ( animations.size() == 0 );

    if ( !appendAnimations ) {					// Edit existing animations
        for ( size_t index=0; index < animations.size(); index++ ) {
            ChannelAnimationForm cform( &m_text_io, 
                                        getVenue(), 
                                        animations[index],
                                        index+1 == animations.size(),
                                        reference_fixture_uid);
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
                                        ChannelAnimation( 0, CAM_SCALE, ChannelValueArray() ),
                                        true,
                                       reference_fixture_uid );
            if ( !cform.play() )
                return false;

            cform.appendAnimations( animations );

            if ( !cform.addAnotherChannel() )
                break;
        }			
    }
    
    form.updateAnimationSignal( anim->signal() );
    anim->setChannelAnimations( animations );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animPixelEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<ScenePixelAnimator> anim;

    if ( current_animation == NULL )
        anim.reset( new ScenePixelAnimator( NOUID, false, NOUID, AnimationSignal(),PixelEffect::BEAM, 
                                       RGBWAArray(), RGBWA::BLACK, 5, 1, true, 2, true ) );
    else
        anim.reset( new ScenePixelAnimator( *reinterpret_cast<ScenePixelAnimator*>( current_animation ) ) );

    PixelAnimationForm form( &m_text_io, getVenue(), anim.get() );
    if ( !form.play() )
        return false;

    form.update( anim.get() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animFilterEditor( AnimationDefinition* current_animation, UID reference_fixture_uid ) 
{
    std::unique_ptr<SceneChannelFilter> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneChannelFilter( NOUID, false, NOUID, AnimationSignal(), ChannelFilter::CF_SINE_WAVE, ChannelList(), 1, 5, 0 ) );
    else
        anim.reset( new SceneChannelFilter( *reinterpret_cast<SceneChannelFilter*>( current_animation ) ) );

    FilterAnimationForm form( &m_text_io, getVenue(), anim.get() );
    if ( !form.play() )
        return false;

    form.update( anim.get() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animPulseEditor( AnimationDefinition* current_animation, UID reference_fixture_uid ) 
{
    std::unique_ptr<ScenePulse> anim;

    if ( current_animation == NULL )
        anim.reset( new ScenePulse( NOUID, false, NOUID, AnimationSignal(), RGBWA::WHITE, 250, 1, false, PulseEffect::PULSE_EFFECT_STROBE ) );
    else
        anim.reset( new ScenePulse( *reinterpret_cast<ScenePulse*>( current_animation ) ) );

    NumberedListField pulse_mode_field( "Pulse" );
    pulse_mode_field.addKeyValue( 1, "Strobe" );
    pulse_mode_field.setDefaultListValue( anim->getPulseEffect() );

    IntegerField pulse_ms_field( "Pulse time (ms)", anim->getPulseMS(), 0, 32000 );
    ColorSelectField strobe_color_field( "Strobe pulse color", anim->getPulseColor() );
    IntegerField fixture_count_field( "Fixtures per pulse", anim->getPulseFixtureCount(), 0, 20 );

    BooleanField random_fixtures_field( "Select random fixtures", anim->isSelectRandom() );

    SignalForm form( &m_text_io, anim->signal() );
    form.add( pulse_mode_field );
    form.add( pulse_ms_field );
    form.add( strobe_color_field );
    form.add( fixture_count_field );
    form.add( random_fixtures_field );

    if ( !form.play() )
        return false;

    anim->setPulseEffect( (PulseEffect)pulse_mode_field.getListValue() );
    anim->setPulseMS( pulse_ms_field.getLongValue() );
    anim->setPulseColor( strobe_color_field.getColor() );
    anim->setPulseFixtureCount( fixture_count_field.getIntValue() );
    anim->setSelectRandom( random_fixtures_field.isSet() );
    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* DMXTextUI::animCueEditor( AnimationDefinition* current_animation, UID reference_fixture_uid )
{
    std::unique_ptr<SceneCueAnimator> anim;

    if ( current_animation == NULL )
        anim.reset( new SceneCueAnimator( NOUID, false, NOUID, AnimationSignal(), false, 0, CueArray() ) );
    else
        anim.reset( new SceneCueAnimator( *reinterpret_cast<SceneCueAnimator*>( current_animation ) ) );

    class MyForm : public SignalForm {
        Venue*	m_venue;
        std::vector<MultiPaletteSelectField*> m_palette_fields;

        void fieldLeaveNotify( size_t field_num ) {
            SignalForm::fieldLeaveNotify( field_num );

            if ( field_num == size()-1 ) {
                MultiPaletteSelectField* field = getField<MultiPaletteSelectField>( field_num );
                if ( field->hasSelections() )
                    addPaletteField( UIDArray() );
            }
        }

        void addPaletteField( UIDArray& palette_references ) {
            CString label;
            label.Format( "Cue #%d palette", m_palette_fields.size()+1 );

            MultiPaletteSelectField* field = new MultiPaletteSelectField( (LPCSTR)label, m_venue, palette_references );

            add( *field );

            m_palette_fields.push_back( field );
        }

    public:
        MyForm( TextIO* input_stream, AnimationSignal& signal, Venue* venue ) :
            SignalForm( input_stream, signal ),
            m_venue(venue)
        {
        }

        void addPalettes( CueArray& cues ) {
            for ( UIDArray& palette_references :cues )
                addPaletteField( palette_references );

            addPaletteField( UIDArray() );
        }

        std::vector<MultiPaletteSelectField*>& getPalettes() {
            return m_palette_fields;
        }

        ~MyForm() {
            for ( MultiPaletteSelectField* field : m_palette_fields )
                delete field;
            m_palette_fields.clear();
        }
    };

    IntegerField group_size_field( "Group size", anim->getGroupSize(), 0, 128 );
    BooleanField tracking_field( "Tracking mode", anim->isTracking() );

    MyForm form( &m_text_io, anim->signal(), getVenue() );
    form.add( group_size_field );
    form.add( tracking_field );
    form.addPalettes(  anim->getCues() );
    if ( !form.play() )
        return false;

    CueArray cues;
    for ( MultiPaletteSelectField* field : form.getPalettes() )
        cues.push_back( field->getSelectedPalettes() );

    // Remove last entry as it was the empty end marker
    cues.pop_back();

    anim->setTracking( tracking_field.isSet() );
    anim->setGroupSize( group_size_field.getIntValue() );
    anim->setCues( cues );
    form.updateAnimationSignal( anim->signal() );

    return anim.release();
}