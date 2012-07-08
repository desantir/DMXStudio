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
            it != AudioInputStream::audioCaptureDevices.end(); it++ ) {
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
    for ( ChasePtrArray::iterator it=list.begin(); it != list.end(); it++ )
        addKeyValue( (*it)->getChaseNumber(), (*it)->getName() );

    if ( default_chase == 0 && list.size() > 0 )
        default_chase = list[0]->getChaseNumber();

    setDefaultListValue( default_chase );
}

// ----------------------------------------------------------------------------
//
SceneSelectField::SceneSelectField( LPCSTR label, Venue* venue, SceneNumber default_scene, bool include_default ) :
    NumberedListField( label ),
    m_venue( venue ),
    m_include_default( include_default )
{
    ScenePtrArray scenes = m_venue->getScenes();
    sort( scenes.begin(), scenes.end(), SceneSelectField::CompareSceneIDs );

    if ( default_scene == 0 )
        default_scene = m_venue->getScene()->getSceneNumber();
    if ( !m_include_default && default_scene == DEFAULT_SCENE_NUMBER )
        default_scene = 0;

    for ( ScenePtrArray::iterator it=scenes.begin(); it != scenes.end(); it++ ) {
        Scene* scene = (*it);
        if ( m_include_default || scene->getSceneNumber() != DEFAULT_SCENE_NUMBER ) {
            addKeyValue( scene->getSceneNumber(), scene->getName() );
            if ( default_scene == 0 )
                default_scene = scene->getSceneNumber();
        }
    }

    setDefaultListValue( default_scene );
}

// ----------------------------------------------------------------------------
//
FixtureSelectField::FixtureSelectField( LPCSTR field_label, Venue* venue, bool include_groups ) :
    KeyListField( field_label ),
    m_venue( venue ),
    m_include_groups( include_groups )
{
    FixturePtrArray fixtures = m_venue->getFixtures();
    for ( FixturePtrArray::iterator it=fixtures.begin() ; it != fixtures.end(); it++ ) {
        Fixture* pf = (*it);
        CString key;
        key.Format( "%lu", pf->getFixtureNumber() );
        CString label;
        label.Format( "%s @ %d", pf->getFullName(), pf->getAddress() );
        addKeyValue( key, label );

        if ( it == fixtures.begin() )
            setDefaultListValue( key );
    }
        
    if ( m_include_groups ) {
        m_fixture_groups = m_venue->getFixtureGroups();
        UINT index = 1;

        for ( FixtureGroupPtrArray:: iterator it=m_fixture_groups.begin(); it != m_fixture_groups.end(); it++, index++ ) {
            CString key;
            key.Format( "G%u", index );
            CString label;
            label.Format( "Group: %s", (*it)->getName() );
            addKeyValue( key, label );
        }
    }
}

// ----------------------------------------------------------------------------
//
void FixtureSelectField::getFixtures( FixturePtrArray& targets ) {
    targets.clear();

    if ( m_include_groups && toupper(m_current_value[0]) == 'G' ) {
        UINT index = strtoul( &((LPCSTR)m_current_value)[1], NULL, 10 )-1;

        FixtureGroup* group = m_fixture_groups[index];
        UIDSet list = group->getFixtures();
        for ( UIDSet::iterator it=list.begin(); it != list.end(); it++ ) {
            Fixture* pf = m_venue->getFixture( *it );
            targets.push_back( pf );
        }
        std::sort( targets.begin(), targets.end(), FixtureSelectField::SortByBaseChannel );
    }
    else {
        UINT index = strtoul( (LPCSTR)m_current_value, NULL, 10 );
        Fixture* pf = m_venue->getFixtureByNumber( index );
        targets.push_back( pf );
    }
}

// ----------------------------------------------------------------------------
//
ChannelControllerField::ChannelControllerField( Venue* venue, FixturePtrArray* fixtures, channel_t channel ) :
        IntegerField( 0, 255 ),
        m_venue( venue ),
        m_fixtures( fixtures ),
        m_channel( channel )
{
    BYTE value = 0;

    for ( FixturePtrArray::iterator it=m_fixtures->begin(); it != m_fixtures->end(); it++ ) {
        Fixture* pf = (*it);

        if ( pf->getNumChannels() > m_channel )
            value = std::max<channel_t>( value, m_venue->getChannelValue( pf, m_channel ) );
    }

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
void ChannelControllerField::setFixtureValues( BYTE chnl_value ) {
    for ( FixturePtrArray::iterator it=m_fixtures->begin(); it != m_fixtures->end(); it++ ) {
        Fixture * pf = (*it);
        if ( pf->getNumChannels() > m_channel )
            m_venue->setChannelValue( pf, m_channel, chnl_value );
    }
}

// ----------------------------------------------------------------------------
//
LPCSTR ChannelControllerField::getLabel() {
    CString name;
    CString addresses;
    CString range;

    // Figure out which channel text to show
    for ( FixturePtrArray::iterator it=m_fixtures->begin(); it != m_fixtures->end(); it++ ) {
        Fixture * pf = (*it);

        if ( pf->getNumChannels() > m_channel ) {
            Channel* ch = pf->getChannel(m_channel);

            if ( name.GetLength() == 0 )
                name = ch->getName();
            if ( addresses.GetLength() > 0 )
                addresses += ",";
            addresses.AppendFormat( "%d", pf->getChannelAddress(m_channel) );
            channel_t value = m_venue->getChannelValue( pf, m_channel );

            ChannelValueRange* rangep = ch->getRange( value );
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
    for ( FixturePtrArray::iterator it=m_fixtures->begin(); it != m_fixtures->end(); it++ ) {
        Fixture * pf = (*it);
        if ( pf->getNumChannels() > m_channel ) {
            Channel* ch = pf->getChannel(m_channel);
            if ( labelValue.GetLength() > 0 )
                labelValue += ",";
            channel_t value = m_venue->getChannelValue( pf, m_channel );
            labelValue.AppendFormat( "%d", value );
        }
    }
}

// ----------------------------------------------------------------------------
//
UniqueSceneNumberField::UniqueSceneNumberField( LPCSTR label, Venue* venue ) :
    IntegerField( label, venue->nextAvailableSceneNumber(), 1, MAX_OBJECT_NUMBER ),
    m_venue( venue )
{
};

// ----------------------------------------------------------------------------
//
bool UniqueSceneNumberField::setValue( LPCSTR value ) {
    long result;

    if ( !IntegerField::parse( value, result ) )
        return false;

    if ( m_venue->getSceneByNumber( (SceneNumber)result ) != NULL )
        throw FieldException( "Scene number %lu is already in use", result );

    return IntegerField::setValue( value );
}

// ----------------------------------------------------------------------------
//
UniqueFixtureNumberField::UniqueFixtureNumberField( LPCSTR label, Venue* venue ) :
    IntegerField( label, venue->nextAvailableFixtureNumber(), 1, MAX_OBJECT_NUMBER ),
    m_venue( venue )
{
};

// ----------------------------------------------------------------------------
//
bool UniqueFixtureNumberField::setValue( LPCSTR value ) {
    long result;

    if ( !IntegerField::parse( value, result ) )
        return false;

    if ( m_venue->getFixtureByNumber( (FixtureNumber)result ) != NULL )
        throw FieldException( "Fixture number %lu is already in use", result );

    return IntegerField::setValue( value );
}

// ----------------------------------------------------------------------------
//
UniqueChaseNumberField::UniqueChaseNumberField( LPCSTR label, Venue* venue ) :
    IntegerField( label, venue->nextAvailableChaseNumber(), 1, MAX_OBJECT_NUMBER ),
    m_venue( venue )
{
};

// ----------------------------------------------------------------------------
//
bool UniqueChaseNumberField::setValue( LPCSTR value ) {
    long result;

    if ( !IntegerField::parse( value, result ) )
        return false;

    if ( m_venue->getChaseByNumber( (ChaseNumber)result ) != NULL )
        throw FieldException( "Chase number %lu is already in use", result );

    return IntegerField::setValue( value );
}

// ----------------------------------------------------------------------------
//
FixtureGroupSelectField::FixtureGroupSelectField( LPCSTR label, Venue* venue ) :
    NumberedListField( label ),
    m_venue( venue )
{
    m_fixture_groups = m_venue->getFixtureGroups();
    UINT index = 1;

    for ( FixtureGroupPtrArray:: iterator it=m_fixture_groups.begin(); it != m_fixture_groups.end(); it++, index++ )
        addKeyValue( index, (*it)->getName() );

    setDefaultListValue( 1 );
}

// ----------------------------------------------------------------------------
//
DmxAddressField::DmxAddressField( LPCSTR label, Venue* venue, Fixture* fixture ) :
    IntegerField( label, 1, 1, DMX_PACKET_SIZE ),
    m_venue( venue ),
    m_num_channels(1),
    m_uid(0)
{
    if ( fixture != NULL ) {
        m_uid = fixture->getUID();
        m_num_channels = fixture->getNumChannels();
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

    channel_t end_address = (channel_t)result+m_num_channels-1;

    UID current = m_venue->whoIsAddressRange( 1, (channel_t)result, end_address );
    if ( current != 0 && current != m_uid )
        throw FieldException( "DMX address %d already in use", result );
    if ( end_address > DMX_PACKET_SIZE )
        throw FieldException( "Channel range past end of addressable DMX addresses (%d > %d)", end_address, DMX_PACKET_SIZE );

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
bool SceneFixtureSelectField::selectScene( UID scene_uid, UID default_fixture )
{
    Scene* scene = m_venue->getScene( scene_uid );
    if ( scene == NULL )
        return false;

    m_scene_uid = scene_uid;

    ActorPtrArray actors = scene->getActors();

    FixtureNumber default_fixture_number = 0;

    for ( unsigned index=0; index < actors.size(); index++ ) {
        Fixture* pf = m_venue->getFixture( actors[index]->getPFUID() );

        CString label;
        label.Format( "%s @ %d", pf->getFullName(), pf->getAddress() );

        addKeyValue( pf->getFixtureNumber(), label );

        if ( default_fixture == pf->getUID() )
            default_fixture_number = pf->getFixtureNumber();
    }

    if ( default_fixture_number != 0 )
        setDefaultListValue( default_fixture_number );
    else if ( actors.size() > 0 )
        setDefaultListValue( actors[0]->getPFUID() );

    return true;
}

// ----------------------------------------------------------------------------
//
void SceneAnimationSelectField::selectAnimations( Scene* scene ) 
{
    m_animations = scene->animations();

    for ( unsigned index=0; index < m_animations.size(); index++ ) {
        AbstractAnimation* animation = m_animations[index];
        CString label;
        label.Format( "%s (ID=%lu)", animation->getName(), animation->getUID() );
        addKeyValue( index+1, (LPCSTR)label );
    }

    setDefaultListValue( 1 );
}

// ----------------------------------------------------------------------------
//
AnimationSelectField::AnimationSelectField( LPCSTR field_label ) :
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
FixtureSelect::FixtureSelect( LPCSTR field_label, Venue* venue, UIDArray& fixture_uids, UIDArray& selected_uids ) :
        InputField( field_label, "" ),
        m_venue( venue ),
        m_selected_uids( selected_uids )
{
    if ( fixture_uids.size() == 0 )
        return;

    std::set<UID> uids;
    CString default_value;

    for ( UIDArray::iterator it=fixture_uids.begin(); it != fixture_uids.end(); it++ ) {
        Fixture* pf = m_venue->getFixture((*it) );
        CString label;
        label.Format( "%s @ %d", pf->getFullName(), pf->getAddress() );
        m_fixtures[ pf->getFixtureNumber() ] = label;
        uids.insert( pf->getUID() );
        if ( default_value.GetLength() == 0 )
            default_value.Format( "%lu", pf->getFixtureNumber() );
    }

    // Add all fixture groups where all the fixtures are in the scene
    FixtureGroupPtrArray fixture_groups = m_venue->getFixtureGroups();
    UINT index = 1;

    for ( FixtureGroupPtrArray:: iterator it=fixture_groups.begin(); it != fixture_groups.end(); it++ ) {
        UIDSet list = (*it)->getFixtures();
        bool missing = false;
        for ( UIDSet::iterator it2=list.begin(); it2 != list.end(); it2++ ) {
            if ( uids.find( (*it2) ) == uids.end() ) {
                missing = true;
                break;
            }
        }

        if ( !missing ) {
            m_fixture_groups.push_back( *it );
        }
    }

    CString values;

    for ( UIDArray::iterator it=m_selected_uids.begin(); it != m_selected_uids.end(); it++ ) {
        if ( uids.find( (*it) ) != uids.end() ) {
            if ( values.GetLength() != 0 )
                values += ",";
            Fixture* pf = m_venue->getFixture((*it) );
            values.AppendFormat( "%lu", pf->getFixtureNumber() );
        }
    }

    if ( values.GetLength() == 0 )
        setValue( default_value );
    else
        setValue( values );
}

// ----------------------------------------------------------------------------
//
bool FixtureSelect::setValue( LPCSTR value ) {
    CString key( value );
    UIDArray uids;

    int curPos = 0;
    while ( true ) {
        CString resToken = key.Tokenize( _T(" ,"), curPos );
        if ( resToken.IsEmpty() )
            break;

        if ( toupper(resToken[0]) == 'G' ) {
            UINT index = strtoul( &((LPCSTR)resToken)[1], NULL, 10 )-1;
            if ( index >= m_fixture_groups.size() )
                throw FieldException( "Invalid group number G%d", index+1 );

            FixtureGroup* group = m_fixture_groups[index];
            UIDSet list = group->getFixtures();
            for ( UIDSet::iterator it=list.begin(); it != list.end(); it++ )
                uids.push_back( *it );
        }
        else {	
            FixtureNumber fixture_number = (FixtureNumber)strtoul( resToken, NULL, 10 );
            if ( m_fixtures.find( fixture_number ) == m_fixtures.end() )
                throw FieldException( "Invalid fixture %u", value );

            uids.push_back( m_venue->getFixtureByNumber( fixture_number )->getUID() );
        }
    }

    if ( uids.size() == 0 )
        throw FieldException( "Select at least one fixture" );

    m_selected_uids = uids;

    return InputField::setValue( value );
}

// ----------------------------------------------------------------------------
//
void FixtureChannelField::setFixture( Fixture *fixture ) {
    for ( channel_t index=0; index < fixture->getNumChannels(); index++ )
        addKeyValue( index+1, fixture->getChannel(index)->getName() );

    if ( fixture->getChannel( m_channel ) == NULL )
        m_channel = 0;

    setDefaultListValue( m_channel+1 );
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

    for ( ChannelValueArray::iterator it=m_channel_values.begin(); it != m_channel_values.end(); it++ ) {
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

    for ( CoordinateArray::iterator it=m_coordinates.begin(); it != m_coordinates.end(); it++ ) {
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
