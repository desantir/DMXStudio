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

// ----------------------------------------------------------------------------
//
AudioCaptureField::AudioCaptureField( LPCSTR default_value, bool must_match ) :
    SelectionField( "Audio capture device", default_value, must_match )
{
    for ( AudioCaptureDeviceArray::iterator it=AudioInputStream::audioCaptureDevices.begin();
            it != AudioInputStream::audioCaptureDevices.end(); ++it ) {
        m_selections.push_back( (*it).m_friendly_name );
    }
}

// ----------------------------------------------------------------------------
//
ChaseSelectField::ChaseSelectField( LPCSTR label, Venue* venue, ChaseNumber default_chase ) :
    NumberedListField( label ),
    m_venue( venue )
{
    if ( default_chase == 0 && venue->isChaseRunning() )
        default_chase = venue->getChase( venue->getRunningChase() )->getChaseNumber();

    ChasePtrArray list = m_venue->getChases();
    for ( ChasePtrArray::iterator it=list.begin(); it != list.end(); ++it )
        addKeyValue( (*it)->getChaseNumber(), (*it)->getName() );

    if ( default_chase == 0 && list.size() > 0 )
        default_chase = list[0]->getChaseNumber();

    setDefaultListValue( default_chase );
}

// ----------------------------------------------------------------------------
//
ChannelControllerField::ChannelControllerField( Venue* venue, SceneActor actor, channel_address channel, bool live_update ) :
        IntegerField( 0, 255 ),
        m_venue( venue ),
        m_actor( actor ),
        m_channel( channel ),
        m_live_update( live_update )
{
    BYTE value = m_venue->getBaseChannelValue( m_actor, m_channel );

    setInitialValue( value );
    setFixtureValues( value );
}

// ----------------------------------------------------------------------------
//
bool ChannelControllerField::setValue( LPCSTR value ) {
    if ( !IntegerField::setValue( value ) )
        return false;

    setFixtureValues( (BYTE)getIntValue() );

    return true;
}

// ----------------------------------------------------------------------------
// Set channel value on all fixtures
//
void ChannelControllerField::setFixtureValues( channel_value chnl_value ) {
    if ( m_live_update ) {
        SetChannelArray channel_sets;
        channel_sets.emplace_back( m_actor.getActorUID(), m_channel, chnl_value );
        m_venue->captureAndSetChannelValue( channel_sets );
    }

    m_labelValue.Format( "%u", chnl_value );
}

// ----------------------------------------------------------------------------
//
LPCSTR ChannelControllerField::getLabel() {
    CString name;
    CString addresses;
    CString range;

    // Figure out which channel text to show
    for ( Fixture *pf : m_venue->resolveActorFixtures( &m_actor ) ) {
        if ( pf->getNumChannels() > m_channel ) {
            Channel* ch = pf->getChannel(m_channel);

            if ( name.GetLength() == 0 )
                name = ch->getName();
            if ( addresses.GetLength() > 0 )
                addresses += ",";
            addresses.AppendFormat( "%u", pf->getChannelAddress(m_channel) );

            ChannelValueRange* rangep = ch->getRange( getIntValue() );
            if ( rangep && range.Find( rangep->getName() ) == - 1 ) {
                if ( range.GetLength() > 0 )
                    range += ",";
                range += rangep->getName();
            }
        }
    }

    if ( range.GetLength() > 0 )
        range = " \"" + range + "\" ";

    m_label.Format( "%d: %s (@%s)%s", m_channel+1, (LPCTSTR)name, (LPCTSTR)addresses, (LPCTSTR)range );

    return m_label;
}

// ----------------------------------------------------------------------------
//
void ChannelControllerField::getLabelValue( CString& labelValue ) {
    labelValue = m_labelValue;
}

// ----------------------------------------------------------------------------
//
FixtureGroupSelectField::FixtureGroupSelectField( LPCSTR label, Venue* venue ) :
    NumberedListField( label ),
    m_venue( venue )
{
    bool first = true;

    for ( FixtureGroup* group : m_venue->getFixtureGroups() ) {
        addKeyValue( group->getNumber(), group->getName() );

        if ( first ) {
            setDefaultListValue( group->getNumber() );
            first = false;
        }
    }
}

// ----------------------------------------------------------------------------
//
DmxUniverseField::DmxUniverseField(  LPCSTR label, Venue* venue, universe_t universe_id ) :
    NumberedListField( label )
{
    UniversePtrArray universes = venue->getUniverses();
    for ( UniversePtrArray::iterator it=universes.begin(); it != universes.end(); ++it ) {
        CString label;
        label.Format( "DMX Universe %d", (*it)->getId() );
        addKeyValue( (*it)->getId(), label );        
    }

    setDefaultListValue( universe_id );
}

// ----------------------------------------------------------------------------
//
DmxAddressField::DmxAddressField( LPCSTR label, Venue* venue, Fixture* fixture, universe_t universe ) :
    IntegerField( label, 1, 1, DMX_PACKET_SIZE ),
    m_venue( venue ),
    m_num_channels(1),
    m_uid(0),
    m_allow_address_overlap(false),
    m_universe_id( universe )
{
   setFixture( fixture );
}

void DmxAddressField::setFixture( Fixture *fixture ) {
    if ( fixture != NULL ) {
        m_uid = fixture->getUID();
        m_num_channels = fixture->getNumChannels();
        m_universe_id = fixture->getUniverseId();
        setInitialValue( fixture->getAddress() );
    }
}

// ----------------------------------------------------------------------------
//
bool DmxAddressField::setValue( LPCSTR value ) 
{
    long result;

    if ( !IntegerField::parse( value, result ) )
        throw FieldException( "'%s' is not a valid DMX address" );

	channel_address end_address = (channel_address)result+m_num_channels-1;

    if ( !m_allow_address_overlap ) {
        UID current = m_venue->whoIsAddressRange( m_universe_id, (channel_address)result, end_address );
        if ( current != 0 && current != m_uid )
            throw FieldException( "DMX address %d already in use", result );
        if ( end_address > DMX_PACKET_SIZE )
        throw FieldException( "Channel range past end of addressable DMX addresses (%d > %d)", end_address, DMX_PACKET_SIZE );
    }

    return IntegerField::setValue( value );
}

// ----------------------------------------------------------------------------
//
FixtureDefSelectField::FixtureDefSelectField( LPCSTR field_label ) :
    NumberedListField( field_label )
{
    UINT index = 1;

    for ( FixtureDefinitionMap::iterator it=FixtureDefinition::FixtureDefinitions.begin();
          it != FixtureDefinition::FixtureDefinitions.end(); it++, index++ ) {
        m_fuid_list.push_back( it->first );
        
        CString label;
        label.Format( "%s %s", it->second.getManufacturer(), it->second.getModel() );
        addKeyValue( index, (LPCSTR)label );
    }

    setDefaultListValue( 1 );
}

// ----------------------------------------------------------------------------
//
AnimationTypeSelectField::AnimationTypeSelectField( LPCSTR field_label ) :
        NumberedListField( field_label )
{
    for ( unsigned index=0; index < DMXTextUI::animEditors.size(); index++ ) {
        AnimationEditor& editor = DMXTextUI::animEditors[index];
        CString label;
        addKeyValue( index+1, editor.m_name );
    }

    setDefaultListValue( 1 );
}

// ----------------------------------------------------------------------------
//
void FixtureChannelField::setFixture( UID fixture_uid ) {

    Fixture* fixture = m_venue->getFixture( fixture_uid );
    if ( fixture == NULL )
        fixture = m_venue->getGroupRepresentative( fixture_uid );

    for ( size_t index=0; index < fixture->getNumChannels(); index++ )
        addKeyValue( index+1, fixture->getChannel(index)->getName() );

    if ( fixture->getChannel( m_channel ) == NULL )
        m_channel = 0;

    setDefaultListValue( m_channel+1 );
}

// ----------------------------------------------------------------------------
//
void FixtureChannelsField::setFixture( Fixture *fixture ) {
    for ( size_t index=0; index < fixture->getNumChannels(); index++ )
        addKeyValue( index+1, fixture->getChannel(index)->getName() );
}

// ----------------------------------------------------------------------------
//
ChannelValueListField::ChannelValueListField( LPCSTR label, ChannelValueArray channel_values ) :
        InputField( label, "" ),
        m_channel_values( channel_values )
{
    if ( m_channel_values.size() == 0 )
        m_channel_values.push_back( 0 );

    CString values;

    for ( ChannelValueArray::iterator it=m_channel_values.begin(); it != m_channel_values.end(); ++it ) {
        if ( values.GetLength() != 0 )
            values += ",";
        values.AppendFormat( "%d", (*it) );
    }

    setValue( values );
}

// ----------------------------------------------------------------------------
//
bool ChannelValueListField::setValue( LPCSTR value ) {
    CString key( value );
    ChannelValueArray channel_value_list;

    int curPos = 0;
    while ( true ) {
        CString resToken = key.Tokenize( _T(" ,"), curPos );
        if ( resToken.IsEmpty() )
            break;
            
        long channel_value = strtoul( resToken, NULL, 10 );
        if ( channel_value < 0 || channel_value > 255 ) 
            throw FieldException( "Value %lu is out of range (value values 0-255)", channel_value );

        channel_value_list.push_back( (BYTE)channel_value );
    }

    if ( channel_value_list.size() == 0 )
        throw FieldException( "Need at least one channel value" );

    m_channel_values = channel_value_list;

    return InputField::setValue( value );
}

// ----------------------------------------------------------------------------
//
CoordinatesField::CoordinatesField( LPCSTR label, CoordinateArray coordinates ) :
        InputField( label, "" ),
        m_coordinates( coordinates )
{
    CString values;

    for ( CoordinateArray::iterator it=m_coordinates.begin(); it != m_coordinates.end(); ++it ) {
        if ( values.GetLength() != 0 )
            values += ";";
        values.AppendFormat( "%u,%u", (*it).m_pan, (*it).m_tilt );
    }

    setValue( values );
}

// ----------------------------------------------------------------------------
//
bool CoordinatesField::setValue( LPCSTR value ) {
    CString key( value );
    CoordinateArray coordinates_list;

    int curPos = 0;
    while ( true ) {
        CString resToken = key.Tokenize( _T(";"), curPos );
        if ( resToken.IsEmpty() )
            break;

        UINT pan, tilt;

        resToken.Trim();

        if ( sscanf_s( resToken, "%u,%u", &pan, &tilt ) != 2 )
            throw FieldException( "Enter coordinates in the form: pan,tilt;pan,tilt;..." );

        coordinates_list.push_back( FixtureCoordinate(pan,tilt) );
    }

    m_coordinates = coordinates_list;

    return InputField::setValue( value );
}

// ----------------------------------------------------------------------------
//
ActorSelectField::ActorSelectField( LPCSTR field_label, Venue* venue ) :
    InputField( field_label, "" ),
    m_venue( venue ),
    m_allow_multiple( false )
{
}

// ----------------------------------------------------------------------------
//
ActorSelectField::ActorSelectField( LPCSTR field_label, Venue* venue, Scene* scene, UIDArray& selected_actor_uids ) :
        InputField( field_label, "" ),
        m_venue( venue ),
        m_allow_multiple( true )
{
    init( scene->getActors(), selected_actor_uids );
}

// ----------------------------------------------------------------------------
//
ActorSelectField::ActorSelectField( LPCSTR field_label, Venue* venue, bool include_groups ) :
        InputField( field_label, "" ),
        m_venue( venue ),
        m_allow_multiple( false )
{
    FixturePtrArray fixtures = venue->getFixtures();
    ActorList actors;

    for ( FixturePtrArray::iterator it=fixtures.begin(); it != fixtures.end(); ++it )
        actors.emplace_back( (*it) );

    if ( include_groups ) {
        FixtureGroupPtrArray fixture_groups = m_venue->getFixtureGroups();
        for ( FixtureGroupPtrArray:: iterator it=fixture_groups.begin(); it != fixture_groups.end(); ++it )
            actors.emplace_back( venue, (*it) );
    }

    ActorPtrArray actor_ptrs;

    for ( size_t i=0; i < actors.size(); i++)
        actor_ptrs.push_back( &actors[i] );

    init( actor_ptrs, UIDArray() );
}

// ----------------------------------------------------------------------------
//
bool ActorSelectField::selectScene( UID scene_uid, UIDArray& selected_actors )
{
    m_entries.clear();
    m_selected_actors.clear();

    Scene* scene = m_venue->getScene( scene_uid );
    if ( scene == NULL )
        return false;

    init( scene->getActors(), selected_actors );

    return true;
}

// ----------------------------------------------------------------------------
//
void ActorSelectField::init( ActorPtrArray& actors, UIDArray& selected_actor_uids )
{
    if ( actors.size() == 0 )
        return;

    CString default_value;
    CString values;

    std::map<UID,CString> actor_to_id;

    for ( SceneActor * actor : actors ) {
        CString label;
        CString id;

        if ( actor->isGroup() ) {
            FixtureGroup* group = m_venue->getFixtureGroup( actor->getActorUID() );
            label.Format( "%s", group->getName() );
            id.Format( "G%lu", group->getNumber() );
        }
        else {
            Fixture* pf = m_venue->getFixture( actor->getActorUID() );
            label.Format( "%s @ %d", pf->getFullName(), pf->getAddress() );
            id.Format( "%lu", pf->getFixtureNumber() );
        }

        m_entries[ id ] = ActorSelectField::entry( id, label, *actor );
        actor_to_id[ actor->getActorUID() ] = id;

        if ( default_value.GetLength() == 0 )
            default_value = id;
    }

    // Preseve selected actor order
    for ( UID selected_uid : selected_actor_uids ) {
        std::map<UID,CString>::iterator it = actor_to_id.find( selected_uid );
        if ( it != actor_to_id.end() ) {
            if ( values.GetLength() != 0 )
                values += ",";
            values.Append( (*it).second );
        }
    }

    if ( values.GetLength() == 0 )
        setValue( default_value );
    else
        setValue( values );
}

// ----------------------------------------------------------------------------
//
void ActorSelectField::helpText( CString& help_text) {
    for ( ActorSelectField::EntryMap::iterator it=m_entries.begin(); it != m_entries.end(); ++it )
        help_text.AppendFormat( " %6s - %s\n", (*it).second.m_id, (*it).second.m_label );
}

// ----------------------------------------------------------------------------
//
bool ActorSelectField::setValue( LPCSTR value ) {
    CString key( value );
    ActorList actors;

    int curPos = 0;
    while ( true ) {
        CString resToken = key.Tokenize( _T(" ,"), curPos );
        if ( resToken.IsEmpty() )
            break;

        resToken.MakeUpper();

        ActorSelectField::EntryMap::iterator it = m_entries.find( resToken );
        if ( it == m_entries.end() )
            throw FieldException( "Invalid fixture number %s", (LPCSTR)resToken );

        actors.push_back( (*it).second.m_actor );
    }

    if ( actors.size() == 0 )
        throw FieldException( "Select at least one fixture" );

    if ( actors.size() > 1 && !m_allow_multiple )
        throw FieldException( "Select a single fixture" );

    m_selected_actors = actors;

    return InputField::setValue( value );
}

// ----------------------------------------------------------------------------
//
SceneSelectField::SceneSelectField( LPCSTR label, Venue* venue, SceneNumber selected_scene, bool include_default ) :
    NumberedListField( label ),
    m_venue( venue ),
    m_include_default( include_default )
{
    ScenePtrArray scenes = m_venue->getScenes();
    sort( scenes.begin(), scenes.end(), SceneSelectField::CompareSceneIDs );

    if ( selected_scene == 0 )
        selected_scene = m_venue->getScene()->getSceneNumber();
    if ( !m_include_default && selected_scene == DEFAULT_SCENE_NUMBER )
        selected_scene = 0;

    for ( ScenePtrArray::iterator it=scenes.begin(); it != scenes.end(); ++it ) {
        Scene* scene = (*it);
        if ( m_include_default || scene->getSceneNumber() != DEFAULT_SCENE_NUMBER ) {
            addKeyValue( scene->getSceneNumber(), scene->getName() );
            if ( selected_scene == 0 )
                selected_scene = scene->getSceneNumber();
        }
    }

    setDefaultListValue( selected_scene );
}

// ----------------------------------------------------------------------------
//
AnimationSelectField::AnimationSelectField( LPCSTR label, Venue* venue ) :
    KeyListField( label ),
    m_venue( venue )
{
    reloadAnimations();
}

// ----------------------------------------------------------------------------
//
void AnimationSelectField::reloadAnimations( Scene* scene ) 
{
    AnimationPtrArray animations = m_venue->getAnimations();
    CString key;
    UINT private_animation_num = 1;
    AnimationReferenceArray animation_refs;

    if ( scene != NULL )
        animation_refs = scene->animations();

    sort( animations.begin(), animations.end(), AnimationSelectField::CompareAnimationIDs );

    bool defaultSelected = false;

    for ( AnimationDefinition* animation : animations ) {
        if ( !animation->isShared() ) {
            if ( animation_refs.size() == 0 )
                continue;

            AnimationReferenceArray::iterator it =
                std::find_if<AnimationReferenceArray::iterator>( animation_refs.begin(), animation_refs.end(),
                                [animation]( const AnimationReference& ref ) { return animation->getUID() == ref.getUID(); } );

            if ( it == animation_refs.end() )
                continue;

            key.Format( "P%u", private_animation_num++ );
        }
        else
            key.Format( "%lu", animation->getNumber() );
        
        addKeyValue( (LPCSTR)key, animation->getName(), animation->getUID() );

        if ( !defaultSelected ) {
            setValue( key );
            defaultSelected = true;
        }
    }
}

// ----------------------------------------------------------------------------
//
ColorSelectField::ColorSelectField( LPCSTR field_label, RGBWAArray& colors ) :
    InputField( field_label, "" ),
    m_allow_multiple( true ),
    m_colors( colors )
{
    init();
}

// ----------------------------------------------------------------------------
//
ColorSelectField::ColorSelectField( LPCSTR field_label, RGBWA& color ) :
    InputField( field_label, "" ),
    m_allow_multiple( false )
{
    m_colors.push_back( color );
    init();
}

// ----------------------------------------------------------------------------
//
void ColorSelectField::init() {
    CString values;

    for ( RGBWA& rgb : m_colors ) {
        if ( values.GetLength() > 0 )
            values += ", ";
        values += rgb.getColorName();
    }

    if ( values.GetLength() > 0 )
        setValue( values );
}

// ----------------------------------------------------------------------------
//
void ColorSelectField::helpText( CString& help_text) {
    help_text = "Enter one or more of the following colors (comma separated):\n";
    help_text += "    rgb(r g b)\n";
    help_text += "    #RRGGBB (where RR, GG, and BB are hex color values)\n";

    for ( CString& name : RGBWA::getAllColorNames() )
        help_text.AppendFormat( "    %s\n", (LPCSTR)name );
}

// ----------------------------------------------------------------------------
//
bool ColorSelectField::setValue( LPCSTR value ) {
    CString key( value );
    RGBWAArray colors;

    int curPos = 0;
    while ( true ) {
        CString resToken = key.Tokenize( _T(","), curPos );
        if ( resToken.IsEmpty() )
            break;

        resToken.Trim();
        if ( resToken.GetLength() == 0 )
            continue;

        bool found = false;
        RGBWA rgb;
        ULONG value;
        unsigned red, green, blue;

        if ( sscanf_s( resToken, "#%lx", &value ) == 1 ) {
            found = true;
            rgb = RGBWA( value );
        }
        else if ( sscanf_s( resToken, "rgb(%u %u %u)", &red, &green, &blue ) == 3 ) {
            found = true;
            rgb = RGBWA( red, green, blue );
        }
        else 
            found = RGBWA::getColorByName( (LPCSTR)resToken, rgb );

        if ( ! found )
            throw FieldException( "Unknown color value '%s'", (LPCSTR)resToken );

        colors.push_back( rgb );
    }

    if ( colors.size() > 1 && !m_allow_multiple )
        throw FieldException( "Select a single color" );

    m_colors = colors;

    return InputField::setValue( value );
}

// ----------------------------------------------------------------------------
//
ChannelTypeSelectField::ChannelTypeSelectField( LPCSTR label, ChannelType defaultType) :
    NumberedListField( label )
{
    for ( unsigned type=CHNLT_UNKNOWN; type < CHNLT_NUM_TYPES; type++ )
        addKeyValue(type, type == CHNLT_UNKNOWN ? "None" : Channel::getTypeName( (ChannelType)type ) );

    setDefaultListValue( defaultType );
}

// ----------------------------------------------------------------------------
//
ChannelSelectField::ChannelSelectField( LPCSTR label, FUID fuid, size_t defaultChannel ) :
    NumberedListField( label )
{
    FixtureDefinition* fd = FixtureDefinition::lookupFixture( fuid );

    addKeyValue( 0, "None" );

    for ( size_t index=0; index < fd->getNumChannels(); index++ ) {
        Channel* channel = fd->getChannel( index );
        addKeyValue( index+1, channel->getName() );    
    }

    setDefaultListValue( defaultChannel );
}

// ----------------------------------------------------------------------------
//
PaletteSelectField::PaletteSelectField( LPCSTR label, Venue* venue, bool addNone, UID defaultPalette ) :
    NumberedListField( label ),
    m_venue( venue )
{
    DObjectArray palettes = venue->getPaletteSummary();
    sort( palettes.begin(), palettes.end(), PaletteSelectField::ComparePaletteNames );

    if ( addNone )
        addKeyValue( 0, "None", 0 );

    DWORD defaultIndex = addNone ? 0 : 1;

    for ( DObject& palette : palettes ) {
        addKeyValue( palette.getNumber(), palette.getName(), palette.getUID() );
    
        if ( defaultPalette == palette.getUID() )
            defaultIndex = palette.getNumber();
    }

    setDefaultListValue( defaultIndex );
}

// ----------------------------------------------------------------------------
//
MultiPaletteSelectField::MultiPaletteSelectField( LPCSTR label, Venue* venue, UIDArray palette_references ) :
    MultiNumberedListField( label ),
    m_venue( venue )
{
    DObjectArray palettes = venue->getPaletteSummary();
    sort( palettes.begin(), palettes.end(), PaletteSelectField::ComparePaletteNames );

    for ( DObject& summary : palettes ) {
        addKeyValue( summary.getNumber(), summary.getName() );
    }

    std::vector<UINT> palette_numbers;

    for ( UID palette_id : palette_references ) {
        Palette palette;
        if ( m_venue->getPalette( palette_id, palette ) )
            palette_numbers.push_back( palette.getPaletteNumber() );
    }

    setDefaultListValue( palette_numbers );
}

// ----------------------------------------------------------------------------
//
UIDArray MultiPaletteSelectField::getSelectedPalettes() {
    UIDArray palettes;

    for ( UINT palette_number : getIntSelections( ) ) {
        UID uid = m_venue->getPaletteByNumber( palette_number );
        if ( uid != NOUID )
            palettes.push_back( uid );
    }
    
    return palettes;
}
