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

#pragma once

#include "DMXStudio.h"
#include "Venue.h"
#include "AnimationSignal.h"
#include "OpenDMXDriver.h"
#include "MusicPlayer.h"
#include "Fixture.h"
#include "SoundSampler.h"
#include "BeatDetector.h"
#include "SoundDetector.h"
#include "SceneSequence.h"
#include "SceneSoundLevel.h"
#include "SceneChannelAnimator.h"
#include "ScenePatternDimmer.h"
#include "SceneColorFader.h"
#include "SceneMovementAnimator.h"
#include "SceneStrobeAnimator.h"
#include "ScenePixelAnimator.h"
#include "SceneChannelFilter.h"
#include "Form.h"

class DMXTextUI;

typedef bool (DMXTextUI::*AnimHandlerEditor)( Scene*, UID );

struct AnimationEditor
{
    CString			    m_name;
    CString			    m_classname;
    AnimHandlerEditor	m_handler;

    AnimationEditor( LPCSTR name, LPCSTR classname, AnimHandlerEditor handler ) :
        m_name( name ),
        m_classname( classname ),
        m_handler( handler )
    {}
};

typedef std::vector<AnimationEditor> AnimationEditorArray;

class DMXTextUI
{
    bool            m_running;
    TextIO	        m_text_io;
    
public:
    DMXTextUI(void);
    ~DMXTextUI(void);

    bool loadVenue( const char* venue_filename );

    void run(void);

    static AnimationEditorArray animEditors;

private:
    inline Venue* getVenue() {
        Venue* venue = studio.getVenue();
        STUDIO_ASSERT( venue, "Missing venue" );
        return venue;
    }

    bool isUserSure(void);

    // Maintainence commands
    void help(void);
    void quit(void);
    void loadVenue(void);
    void writeVenue(void);
    void configVenue(void);
    void masterVolume(void);
    void muteVolume(void);

    void blackout(void);
    void autoBlackout(void);
    void soundDebug(void);
    void soundBPM(void);
    void soundDetect(void);
    void soundBeat(void);
    void soundDb();
    void whiteout(void);
    void whiteout_options(void);
    void animationSpeed(void);

    // Fixture control
    void controlFixture(void);
    void describeFixture(void);
    void copyFixture(void);
    void resetDefaultFixtures(void);
    void masterDimmer(void);
    void createFixture(void);
    void deleteFixture(void);
    void updateFixture(void);

    // Scene control
    void createScene(void);
    void selectScene(void);
    void deleteScene(void);
    void updateScene(void);
    void describeScene(void);
    void deleteFixtureFromScene(void);
    void sceneAddAnimation(void);
    void sceneDeleteAllAnimations(void);
    void sceneDeleteAnimation(void);
    void sceneUpdateAnimation(void);
    void copyScene(void);
    void copyWorkspaceToScene(void);

    // Chase control
    void selectChase(void);
    void deleteChase(void);
    void createChase(void);
    void describeChase(void);
    void updateChase(void);
    void copyChaseSteps(void);
    void deleteChaseSteps(void);
    void addChaseSteps(void);
    void chaseTap(void);
    void chaseBeat(void);
    bool editChaseSteps( Chase* chase, bool append_steps, UINT step_num_offset );

    // Fixture group control
    void createFixtureGroup(void);
    void describeFixtureGroup(void);
    void deleteFixtureGroup(void);

    // Music control
    void listPlaylists(void);
    void listTracks(void);
    void playTrack(void);
    void queueTrack(void);
    void queuePlaylist(void);
    void playPlaylist(void);
    void pauseTrack(void);
    void stopTrack(void);
    void forwardTrack(void);
    void backTrack(void);
    void showQueuedTracks(void);
    void selectPlaylist( bool queue );
    void selectTrack( bool queue );
    void musicPlayerLogin(void);
    void musicSceneSelect(void);
    void musicMapTrack(void);
    void musicRemoveMapping(void);
    void musicMapShow(void);

    // Animation editors
    void initializeAnimationEditors();
    bool animSequencerEditor( Scene* scene, UID anim_uid );
    bool animSoundLevelEditor( Scene* scene, UID anim_uid );
    bool animPatternDimmerEditor( Scene* scene, UID anim_uid );
    bool animChannelEditor( Scene* scene, UID anim_uid );
    bool animColorFaderEditor( Scene* scene, UID anim_uid );
    bool animMovementEditor( Scene* scene, UID anim_uid );
    bool animStrobeEditor( Scene* scene, UID anim_uid );
    bool animPixelEditor( Scene* scene, UID anim_uid );
    bool animFilterEditor( Scene* scene, UID anim_uid );
};

typedef void (DMXTextUI::*HandlerFunc)();

struct HandlerInfo
{
    HandlerFunc		m_funcptr;
    bool			m_running;
    const char*		m_desc;

    HandlerInfo( HandlerFunc funcptr, bool running, const char* desc ) :
        m_funcptr( funcptr ),
        m_running( running ),
        m_desc( desc)
    {}

    HandlerInfo( ) {}
};

typedef std::map<CString, HandlerInfo> HandlerMap;

class ConfirmForm : public Form
{
    BooleanField m_sure_field;

public:
    ConfirmForm( TextIO* input_stream, LPCSTR title=NULL, bool auto_delete=false ) :
          Form( input_stream, title, auto_delete ),
          m_sure_field( "Are you sure", false )
    {}

    bool play( void ) {
        if ( size() == 0 || getField<Field>(size()-1) != & m_sure_field )
            add( m_sure_field );
        return Form::play() && m_sure_field.isSet();
    }
};


class AudioCaptureField : public SelectionField
{
public:
    AudioCaptureField( LPCSTR default_value, bool must_match = false );
};

class ChaseSelectField : public NumberedListField
{
    Venue*		m_venue;

public:
    ChaseSelectField( LPCSTR label, Venue* venue, ChaseNumber default_chase=0 );

    UID getChaseUID() const { 
        Chase* chase = m_venue->getChaseByNumber( (ChaseNumber)getListValue() );
        STUDIO_ASSERT( chase, "Invalid chase number" );
        return chase->getUID();
    }

    void isReady( ) {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no chases currently defined" );
    }
};

class SceneSelectField : public NumberedListField
{
    Venue*		m_venue;
    bool		m_include_default;

public:
    SceneSelectField( LPCSTR label, Venue* venue, SceneNumber default_scene=0, bool includeDefault=true );

    UID getSceneUID() const {
        Scene* scene = m_venue->getSceneByNumber( (SceneNumber)getListValue() );
        STUDIO_ASSERT( scene, "Invalid scene number" );
        return scene->getUID();
    }

    static bool SceneSelectField::CompareSceneIDs( Scene* s1, Scene* s2 ) {
        return s1->getSceneNumber() < s2->getSceneNumber();
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no scenes currently defined" );
    }
};

class FixtureGroupSelectField : public NumberedListField
{
    Venue* m_venue;

public:
    FixtureGroupSelectField( LPCSTR label, Venue* venue );

    UID getFixtureGroupUID( ) {
       return m_venue->getFixtureGroupByNumber( getListValue() )->getUID();
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no fixture groups currently defined" );
    }
};

class ChannelControllerField : public IntegerField
{
    Venue*				m_venue;
    SceneActor      	m_actor;
    channel_t			m_channel;
    CString             m_label;
    CString             m_labelValue;
    bool                m_live_update;

public:
    ChannelControllerField( Venue* venue, SceneActor actor, channel_t channel, bool live_update=true );

    bool setValue( LPCSTR value );
    LPCSTR getLabel();
    void getLabelValue( CString& labelValue );

private:
    void setFixtureValues( BYTE chnl_value );
};

class UniqueSceneNumberField : public IntegerField
{
    Venue*		m_venue;

public:
    UniqueSceneNumberField( LPCSTR label, Venue* venue );

    SceneNumber getSceneNumber() const {
        return (SceneNumber)getLongValue();
    }

    bool setValue( LPCSTR value );
};

class UniqueFixtureNumberField : public IntegerField
{
    Venue*		m_venue;

public:
    UniqueFixtureNumberField( LPCSTR label, Venue* venue );

    FixtureNumber getFixtureNumber() const {
        return (FixtureNumber)getLongValue();
    }

    bool setValue( LPCSTR value );
};

class UniqueChaseNumberField : public IntegerField
{
    Venue*		m_venue;

public:
    UniqueChaseNumberField( LPCSTR label, Venue* venue );

    ChaseNumber getChaseNumber() const {
        return (ChaseNumber)getLongValue();
    }

    bool setValue( LPCSTR value );
};

class UniqueGroupNumberField : public IntegerField
{
    Venue*		m_venue;

public:
    UniqueGroupNumberField( LPCSTR label, Venue* venue );

    GroupNumber getGroupNumber() const {
        return (GroupNumber)getLongValue();
    }

    bool setValue( LPCSTR value );
};

class DmxAddressField : public IntegerField
{
    Venue*				m_venue;
    channel_t			m_num_channels;
    UID					m_uid;
    bool                m_allow_address_overlap;
    universe_t          m_universe_id;

public:
    DmxAddressField( LPCSTR label, Venue* venue, Fixture* fixture=NULL, universe_t universe_id=0 );

    bool setValue( LPCSTR value );

    void setNumChannels( channel_t num_channels ) {
        m_num_channels = num_channels;
    }

    void setUniverseId( universe_t universe_id ) {
        m_universe_id = universe_id;
    }

    void setAllowAddressOverlap( bool allow) {
        m_allow_address_overlap = allow;
    }

    void setFixture( Fixture *fixture );
};

class DmxUniverseField : public NumberedListField
{
public:
    DmxUniverseField( LPCSTR label, Venue* venue, universe_t universe_id=1 );

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no universes available" );
    }

    universe_t getUniverseId() const {
        return (universe_t)(getListValue());
    }

    void setUniverse( universe_t universe_id ) {
        setDefaultListValue( universe_id );
    }
};

class FixtureDefSelectField : public NumberedListField
{
    std::vector<FUID>	m_fuid_list;

public:
    FixtureDefSelectField( LPCSTR label );

    FUID getFUID( ) {
        return m_fuid_list[ getListValue()-1 ]; 
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no fixture definitions" );
    }
};

class SceneAnimationSelectField : public NumberedListField
{
    AnimationPtrArray	m_animations;

public:
    SceneAnimationSelectField( LPCSTR label, Scene* scene=NULL ) :
        NumberedListField( label )
    {
        if ( scene )
            selectAnimations( scene );
    }

    UID getAnimationUID( ) {
        return m_animations[ getListValue()-1 ]->getUID(); 
    }

    void selectAnimations( Scene* scene );

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no animations in this scene" );
    }
};

class AnimationSelectField : public NumberedListField
{
public:
    AnimationSelectField( LPCSTR label );

    AnimationEditor* getAnimationEditor( ) {
        return &DMXTextUI::animEditors[ getListValue()-1 ]; 
    }
};

class FixtureChannelsField : public MultiNumberedListField
{
public:
    FixtureChannelsField( LPCSTR label, Fixture* fixture, ChannelList channels ) :
        MultiNumberedListField( label )
    {
        if ( fixture != NULL )
            setFixture( fixture );

        std::vector<UINT> selected;
        for ( channel_t channel : channels )
            selected.push_back( channel+1 );
        if ( selected.size() == 0 )
            selected.push_back( 1 );
        setDefaultListValue( selected );
    }

    ChannelList getChannels() const {
        ChannelList list;
        for ( UINT v : getIntSelections( ) )
            list.push_back( v-1 );
        return list;
    }

    void setFixture( Fixture *fixture );
};

class FixtureChannelField : public NumberedListField
{
    channel_t m_channel;

public:
    FixtureChannelField( LPCSTR label, Fixture* fixture, channel_t channel ) :
        NumberedListField( label ),
        m_channel( channel )
    {
        if ( fixture != NULL )
            setFixture( fixture );
    }

    channel_t getChannel() const {
        return m_channel;
    }

    void setFixture( Fixture *fixture );

    bool setValue( LPCSTR value ) {
        if ( !NumberedListField::setValue( value ) )
            return false;
        m_channel = (channel_t)getListValue()-1;
        return true;
    }
};

class ChannelValueListField : public InputField
{
    ChannelValueArray m_channel_values;

public:
    ChannelValueListField( LPCSTR label, ChannelValueArray channel_values );

    ChannelValueArray getValues() const {
        return m_channel_values;
    }

    bool setValue( LPCSTR value );
};

class CoordinatesField : public InputField
{
    CoordinateArray m_coordinates;

public:
    CoordinatesField( LPCSTR label, CoordinateArray coordinates );

    CoordinateArray getCoordinates() const {
        return m_coordinates;
    }

    bool setValue( LPCSTR value );
};

class FixtureManufacturerSelectField : public NumberedListField
{
public:
    FixtureManufacturerSelectField( LPCSTR label ) :
        NumberedListField( label )
    {
        LPCSTRArray manufacturers = FixtureDefinition::getUniqueManufacturers();
        UINT index = 1;
        for ( LPCSTRArray::iterator it=manufacturers.begin(); it != manufacturers.end(); it++, index++ )
            addKeyValue( index, (*it) );
        setDefaultListValue( 1 );
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no manufacturers available" );
    }
};

class FixtureModelSelectField : public NumberedListField
{
    CString     m_manufacturer;
public:
    FixtureModelSelectField( LPCSTR label, LPCSTR manufacturer=NULL) :
        NumberedListField( label )
    {
        setManufacturer( manufacturer );
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no models available" );
    }

    void setManufacturer( LPCSTR manufacturer ) {
        if ( !manufacturer || manufacturer == m_manufacturer )
            return;

        clear();

        m_manufacturer = manufacturer;
        LPCSTRArray models = FixtureDefinition::getUniqueModels( m_manufacturer );
        UINT index = 1;
        for ( LPCSTRArray::iterator it=models.begin(); it != models.end(); it++, index++ )
            addKeyValue( index, (*it) );
        setDefaultListValue( 1 );
    }
};

class FixturePersonalitySelectField : public NumberedListField
{
    CString                  m_manufacturer;
    CString                  m_model;
    FixturePersonalityToFUID m_personalities;

public:
    FixturePersonalitySelectField( LPCSTR label, LPCSTR manufacturer=NULL, LPCSTR model=NULL) :
        NumberedListField( label )
    {
        setManuAndModel( manufacturer, model );
    }

    FUID getFUID() {
        return m_personalities[(UINT)getListValue()];
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no fixture personalities available" );
    }

    void setManuAndModel( LPCSTR manufacturer, LPCSTR model ) {
        if ( !manufacturer || !model || (manufacturer == m_manufacturer && model == m_model) )
            return;

        clear();

        m_manufacturer = manufacturer;
        m_model = model;
        m_personalities = FixtureDefinition::getModelPersonalities( m_manufacturer, m_model );

        for ( FixturePersonalityToFUID::iterator it=m_personalities.begin(); it != m_personalities.end(); ++it ) {
            CString label;
            label.Format( "CHANNELS", it->first );
            addKeyValue( it->first, label );
        }
        
        setDefaultListValue( m_personalities.begin()->first );
    }
};

class ActorSelectField : public InputField
{
    struct comparitor {
        bool operator() (const CString& lhs, const CString& rhs) const {
            if ( lhs.GetAt(0) == 'G' && rhs.GetAt(0) != 'G' )
                return true;
            if ( lhs.GetAt(0) != 'G' && rhs.GetAt(0) == 'G' )
                return false;

            if ( lhs.GetLength() != rhs.GetLength() )
                return lhs.GetLength() < rhs.GetLength();

            return lhs < rhs;
        }
    };

    struct entry {
        CString     m_label;
        CString     m_id;
        SceneActor  m_actor;

        entry() {}

        entry( CString& id, CString& label, SceneActor& actor ) :
           m_id( id ),
           m_label( label ),
           m_actor( actor )
        {}
    };

    typedef std::map<CString, entry, comparitor> EntryMap;

    EntryMap        m_entries;
    Venue*			m_venue;
    bool            m_allow_multiple;
    ActorList		m_selected_actors;

    void init( ActorPtrArray& actors, UIDArray& selected_actor_uids );

public:
    ActorSelectField( LPCSTR field_label, Venue* venue, Scene* scene, UIDArray& selected_actor_uids );
    ActorSelectField( LPCSTR field_label, Venue* venue, bool include_groups );
    ActorSelectField( LPCSTR field_label, Venue* venue );

    void setDefaultListValue( CString selection ) {
        m_default_value = m_current_value = selection;
    }

    bool setValue( LPCSTR value );

    bool isHelp( CString& input ) const {
        return _stricmp( input, "?" ) == 0;
    }

    void helpText( CString& help_text);

    UIDArray getActorUIDs( ) {
        UIDArray selected_uids;

        for ( ActorList::iterator it=m_selected_actors.begin(); it != m_selected_actors.end(); ++it )
            selected_uids.push_back( (*it).getActorUID() );

        return selected_uids;
    }

    ActorList getActors() const {
        return m_selected_actors;
    }

    UID getActorUID( ) const {
        return getActor().getActorUID();
    }

    SceneActor getActor() const {
        STUDIO_ASSERT( m_selected_actors.size() == 1, "Expected actor array size of 1" );
        return m_selected_actors[0];
    }

    void isReady() {
        if ( m_entries.size() == 0 )
            throw FieldException( "There are no fixtures currently defined" );
    }

    void allowMultiple( bool allow_multiple ) {
        m_allow_multiple = allow_multiple;
    }

    bool selectScene( UID scene_uid );
};

class ColorSelectField : public InputField
{
    Venue*			m_venue;
    bool            m_allow_multiple;
    RGBWAArray      m_colors;

public:
    ColorSelectField( LPCSTR field_label, RGBWAArray& colors );
    ColorSelectField( LPCSTR field_label, RGBWA& colors );

    void setDefaultListValue( CString selection ) {
        m_default_value = m_current_value = selection;
    }

    bool setValue( LPCSTR value );

    bool isHelp( CString& input ) const {
        return _stricmp( input, "?" ) == 0;
    }

    void helpText( CString& help_text);

    RGBWAArray getColors( ) {
        return m_colors;
    }

    RGBWA getColor( ) {
        return ( m_colors.size() == 0 ) ? RGBWA::BLACK : m_colors[0];
    }

    void isReady() {
    }

    void allowMultiple( bool allow_multiple ) {
        m_allow_multiple = allow_multiple;
    }

private:
    void init();
};

class ActsField : public MultiNumberedListField 
{
public:
    ActsField( Acts currentActs = Acts() ) : MultiNumberedListField( "Assigned acts" )
    {
        CString act_desc;
        for ( int act=1; act <= NUM_ACTS; act++ ) {
            act_desc.Format( "Act #%u", act );
            addKeyValue( act, act_desc );
        }

        setActs( currentActs );
    }

    void setActs( Acts acts ) {
        std::vector<UINT> default_values( acts.begin(), acts.end() );
        setDefaultListValue( default_values );
    }

    Acts getActs( ) const {
        Acts acts;
        for ( CString item : getSelections() )
            acts.insert( (UINT)atol( item ) );
        return acts;
    }
};

class BPMRatingField : public NumberedListField 
{
public:
    BPMRatingField( BPMRating current_bpm_rating ) : NumberedListField( "BPM rating" )
    {
        for ( int index=0; index < BPM_END; index++ )
            addKeyValue( (BPMRating)index, getRatingName((BPMRating)index) );

        setRating( current_bpm_rating );
    }

    void setRating( BPMRating rating ) {
        setDefaultListValue( rating );
    }

    BPMRating getRating( ) const {
        return (BPMRating)getListValue();
    }
};

