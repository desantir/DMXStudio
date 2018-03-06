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

#include "stdafx.h"
#include "Form.h"
#include "Venue.h"
#include "Fixture.h"
#include "AnimationSignal.h"
#include "SoundSampler.h"
#include "SceneSequence.h"
#include "SceneSoundLevel.h"
#include "SceneChannelAnimator.h"
#include "ScenePatternDimmer.h"
#include "SceneColorFader.h"
#include "SceneMovementAnimator.h"
#include "SceneStrobeAnimator.h"
#include "ScenePixelAnimator.h"
#include "SceneCueAnimator.h"
#include "SceneChannelFilter.h"
#include "ScenePulse.h"
#include "SceneFixtureDimmer.h"

class DMXTextUI;

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
        UID chase_uid = m_venue->getChaseByNumber( (ChaseNumber)getListValue() );
        STUDIO_ASSERT( chase_uid != NOUID, "Invalid chase number" );
        return chase_uid;
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
        UID scene_uid = m_venue->getSceneByNumber( (SceneNumber)getListValue() );
        STUDIO_ASSERT( scene_uid != NOUID, "Invalid scene number" );
        return scene_uid;
    }

    static bool SceneSelectField::CompareSceneIDs( Scene* s1, Scene* s2 ) {
        return s1->getSceneNumber() < s2->getSceneNumber();
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no scenes currently defined" );
    }
};

class AnimationSelectField : public KeyListField
{
    Venue*		m_venue;

public:
    AnimationSelectField( LPCSTR label, Venue* venue );

    UID getAnimationUID() const {
        return (UID)getKeyValueContext();
    }
    void setAnimationUID( UID animation_uid ) {
        for ( std::pair<CString, KeyValue> pair : m_selections )
            if ( pair.second.context == animation_uid ) {
                setValue( pair.first );
                return;
            }

        STUDIO_ASSERT( "Invalid animation UID" );
    }

    static bool AnimationSelectField::CompareAnimationIDs( AnimationDefinition* a1, AnimationDefinition* a2 ) {
        return a1->getNumber() < a2->getNumber();
    }

    void reloadAnimations( Scene* scene=NULL );

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no animations currently defined" );
    }
};

class FixtureGroupSelectField : public NumberedListField
{
    Venue* m_venue;

public:
    FixtureGroupSelectField( LPCSTR label, Venue* venue );

    UID getFixtureGroupUID( ) {
        return m_venue->getFixtureGroupByNumber( getListValue() );
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
	channel_address		m_channel;
    CString             m_label;
    CString             m_labelValue;
    bool                m_live_update;

public:
    ChannelControllerField( Venue* venue, SceneActor actor, channel_address channel, bool live_update=true );

    bool setValue( LPCSTR value );
    LPCSTR getLabel();
    void getLabelValue( CString& labelValue );

private:
    void setFixtureValues( channel_value chnl_value );
};

template <class Number, class Object>
class UniqueNumberField : public IntegerField
{
    typedef UID (Venue::*Getter)( Number );

    Venue*		m_venue;
    Number      m_current;
    Getter      m_getter;

public:
    UniqueNumberField( LPCSTR label, Venue* venue, Getter getter, Number current=0 ) :
        IntegerField( label, current, 1, MAX_OBJECT_NUMBER ),
        m_venue( venue ),
        m_current( current ),
        m_getter( getter )
    {}

    Number getNumber() const {
        return (Number)getLongValue();
    }

    bool setNumber( Number value ) {
        m_current = value;

        return IntegerField::setValue( value );
    }

    virtual bool isValid( long value ) {
        if ( (Number)value != m_current && (m_venue->*m_getter)( (Number)value ) != NOUID )
            throw FieldException( "Number %lu is already in use", value );

        return IntegerField::isValid( value );
    }
};

class DmxAddressField : public IntegerField
{
    Venue*				m_venue;
    size_t				m_num_channels;
    UID					m_uid;
    bool                m_allow_address_overlap;
    universe_t          m_universe_id;

public:
    DmxAddressField( LPCSTR label, Venue* venue, Fixture* fixture=NULL, universe_t universe_id=0 );

    bool setValue( LPCSTR value );

    void setNumChannels( size_t num_channels ) {
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
    Venue*      m_venue;

public:
    SceneAnimationSelectField( LPCSTR label, Venue* venue, Scene* scene=NULL ) :
        NumberedListField( label ),
        m_venue( venue )
    {
        if ( scene )
            loadAnimations( scene );
    }

    size_t getAnimationIndex( ) {
        return getListValue()-1;
    }

    UID getAnimationUID() {
        return (UID)getListValueContext();
    }

    void loadAnimations( Scene* scene ) 
    {
        int index = 0;
        for ( AnimationReference& ref : scene->animations() ) {
            AnimationDefinition* animation = m_venue->getAnimation( ref.getUID() );
            if ( animation != NULL ) {
                CString label;
                if ( animation->isShared() )
                    label.Format( "Animation %d: %s", animation->getNumber(), animation->getName() );
                else
                    label.Format( "Private animation: %s", animation->getName() );

                addKeyValue( index+1, (LPCSTR)label, animation->getUID() );
            }

            index++;
        }
        
        setDefaultListValue( 1 );
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no animations in this scene" );
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
        for ( channel_address channel : channels )
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
    Venue*			m_venue;
	channel_address m_channel;

public:
    FixtureChannelField( LPCSTR label, Venue* venue, UID fixture_uid, channel_address channel ) :
        NumberedListField( label ),
        m_venue( venue ),
        m_channel( channel )
    {
        setFixture( fixture_uid );
    }

	channel_address getChannel() const {
        return m_channel;
    }

    void setFixture( UID fixture_uid );

    bool setValue( LPCSTR value ) {
        if ( !NumberedListField::setValue( value ) )
            return false;
        m_channel = (channel_address)getListValue()-1;
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

    bool nextValue() {
        if ( m_allow_multiple )
            return false;

        EntryMap::iterator it = m_entries.find( m_current_value );
        if ( it == m_entries.end() )
            return false;
        it++;
        if ( it == m_entries.end() )
            return false;
        return setValue( (*it).first );
    }

    bool previousValue() {
        if ( m_allow_multiple )
            return false;

        EntryMap::iterator it = m_entries.find( m_current_value );
        if ( it == m_entries.begin() )
            return false;
        it--;
        return setValue( (*it).first );
    }

    void setActor( UID actor_uid ) {
        m_selected_actors.clear();

        Fixture* fixture = m_venue->getFixture( actor_uid );
        if ( fixture != NULL ) {
            m_selected_actors.emplace_back( fixture );
            return;
        }

        FixtureGroup* group = m_venue->getFixtureGroup( actor_uid );
        if ( group != NULL ) {
            m_selected_actors.emplace_back( m_venue, group );
            return;        
        }
    }

    void isReady() {
        if ( m_entries.size() == 0 )
            throw FieldException( "There are no fixtures currently defined" );
    }

    void allowMultiple( bool allow_multiple ) {
        m_allow_multiple = allow_multiple;
    }

    bool selectScene( UID scene_uid ) {
        return selectScene( scene_uid, UIDArray() );
    }

    bool selectScene( UID scene_uid, UIDArray& selected_actors );

    virtual void getLabelValue( CString& labelValue ) {
        if ( m_allow_multiple ) {
            Field::getLabelValue( labelValue );
        }
        else {
            EntryMap::iterator it = m_entries.find( m_current_value );
            if ( it != m_entries.end() )
                labelValue.AppendFormat( "%s - %s", it->second.m_id, it->second.m_label );
            else
                Field::getLabelValue( labelValue );
        }
    }
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

class ChannelTypeSelectField : public NumberedListField
{
public:
    ChannelTypeSelectField( LPCSTR label, ChannelType defaultType=CHNLT_UNKNOWN );

    ChannelType getChannelType() const {
        return (ChannelType)getListValue();
    }
};

class ChannelSelectField : public NumberedListField
{
public:
    ChannelSelectField( LPCSTR label, FUID fuid, size_t defaultChannel=0 );

	channel_address getChannel() const {
        return (channel_address)getListValue();
    }
};

class PaletteSelectField : public NumberedListField
{
    Venue*		        m_venue;

public:
    PaletteSelectField( LPCSTR label, Venue* venue, bool addNone=false, UID defaultPalette=0L );

    static bool PaletteSelectField::ComparePaletteNames( DObject& p1, DObject& p2 ) {
        return _strcmpi( p1.getName(), p2.getName() ) == -1;
    }

    UID getPaletteUID() const {
        return getListValue();
    }

    DWORD getListValue() const {
        return getListValueContext();
    }

    void isReady() {
        if ( m_selections.size() == 0 )
            throw FieldException( "There are no palettes currently defined" );
    }
};


class MultiPaletteSelectField : public MultiNumberedListField
{
    Venue*		        m_venue;

public:
    MultiPaletteSelectField( LPCSTR label, Venue* venue, UIDArray palette_references );

    static bool MultiPaletteSelectField::ComparePaletteNames( Palette* p1, Palette* p2 ) {
        return _strcmpi( p1->getName(), p2->getName() ) == -1;
    }

    UIDArray getSelectedPalettes();
};