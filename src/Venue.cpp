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


#include "DMXStudio.h"
#include "OpenDMXDriver.h"
#include "Venue.h"
#include "AnimationTask.h"
#include "AbstractAnimation.h"

// ----------------------------------------------------------------------------
//
Venue::Venue(void) :
    m_uid_pool( 1L ),
    m_universe( NULL ),
    m_chase_task( NULL ),
    m_animation_task( NULL ),
    m_master_dimmer( 100 ),
    m_auto_backout_ms( 0 ),
    m_light_blackout( false ),
    m_audio( NULL ),
    m_volume( NULL ),
    m_sound_detector( NULL ),
    m_audio_boost( 0.0f ),
    m_audio_boost_floor( 0.0009f ),
    m_captured_actor( 0 ),
    m_whiteout( WHITEOUT_OFF ),
    m_whiteout_strobe_ms( 100 ),
    m_music_scene_select_enabled( false ),
    m_music_watcher( NULL )
{
    addScene( Scene( m_current_scene=allocUID(), DEFAULT_SCENE_NUMBER, "Workspace", "" ) );
}

// ----------------------------------------------------------------------------
//
Venue::~Venue(void)
{
    close();
}

// ----------------------------------------------------------------------------
//
bool Venue::open(void) {

    if ( !isRunning() ) {
        try {
            m_audio = AudioInputStream::createAudioStream( m_audio_capture_device, 1024, m_audio_boost, m_audio_boost_floor );
        }
        catch ( StudioException& ex ) {
            DMXStudio::log( ex );
            close();
            return false;
        }

        try {
            m_volume = AudioVolumeController::createVolumeController();
        }
        catch ( StudioException& ex ) {
            DMXStudio::log( ex );
            close();
            return false;
        }

        m_sound_detector = new SoundDetector( m_auto_backout_ms );
        m_sound_detector->attach( m_audio );

        m_universe = new OpenDMXDriver();
        m_universe->setPacketDelayMS( m_dmx_packet_delay );
        m_universe->setMinimumDelayMS( m_dmx_packet_min_delay );
        DMX_STATUS status = m_universe->start( m_dmx_port );

        if ( status != DMX_OK ) {
            DMXStudio::log( StudioException( "Cannot start DMX universe [%s] (STATUS %d)", 
                                             getDmxPort(), status ) );
            close();
            return false;
        }

        DMXStudio::log_status( "Venue started [%s]", m_name );

        m_animation_task = new AnimationTask( this );
        m_animation_task->start();

        loadScene();

        if ( studio.hasMusicPlayer() ) {
            m_music_watcher = new MusicWatcher( studio.getMusicPlayer(), this );
            m_music_watcher->start();
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool Venue::close(void) {
    if ( m_music_watcher ) {
        m_music_watcher->stop();
        delete m_music_watcher;
        m_music_watcher = NULL;
    }

    stopChase();

    if ( m_animation_task ) {
        m_animation_task->stop();
        delete m_animation_task;
        m_animation_task = NULL;
    }

    DMXStudio::log_status( "Venue stopped [%s]", m_name );

    if ( m_universe ) {
        m_universe->stop();
        delete m_universe;
        m_universe = NULL;
    }

    if ( m_sound_detector ) {
        m_sound_detector->detach();
        delete m_sound_detector;
        m_sound_detector = NULL;
    }

    if ( m_volume ) {
        AudioVolumeController::releaseVolumeController( m_volume );
        m_volume = NULL;
    }

    if ( m_audio ) {
        AudioInputStream::releaseAudioStream( m_audio );
        m_audio = NULL;
    }

    return true;
}

// ----------------------------------------------------------------------------
//
void Venue::setWhiteout( WhiteoutMode whiteout ) {
    if ( whiteout == WHITEOUT_STROBE_SLOW )
        m_whiteout_strobe.start( GetCurrentTime(), studio.getWhiteoutStrobeSlow() );
    else if ( whiteout == WHITEOUT_STROBE_FAST )
        m_whiteout_strobe.start( GetCurrentTime(), studio.getWhiteoutStrobeFast() );
    else if ( whiteout == WHITEOUT_STROBE_MANUAL )
        m_whiteout_strobe.start( GetCurrentTime(), m_whiteout_strobe_ms, m_whiteout_strobe_ms/2 );

    m_whiteout = whiteout;
}

// ----------------------------------------------------------------------------
//
UID Venue::whoIsAddressRange( universe_t universe, channel_t start_address, channel_t end_address ) {
    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
        channel_t fixture_start = it->second.getAddress();
        channel_t fixture_end = fixture_start + it->second.getNumChannels() - 1;

        if ( fixture_end >= start_address && fixture_start <= end_address )
            return it->second.getUID();
    }

    return 0;
}

// ----------------------------------------------------------------------------
//
channel_t Venue::findFreeAddressRange( UINT num_channels ) 
{
    // This is brute force ..
    for ( channel_t base=1; base <= DMX_PACKET_SIZE; ) {
        UID uid = whoIsAddressRange( 1, base, base+num_channels-1 );
        if ( !uid )
            return base;
        base += getFixture( uid )->getNumChannels();
    }

    return (channel_t)INVALID_CHANNEL;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteFixture( UID pfuid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureMap::iterator it_del = m_fixtures.find( pfuid );
    if ( it_del == m_fixtures.end() )
        return;

    if ( isChaseRunning() )
        stopChase();

    clearAnimations();

    clearAllCapturedActors();

    selectScene( getDefaultScene()->getUID() );

    // Remove fixture from all dependant objects
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); it++ )
        it->second.removeActor( pfuid );

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); it++ )
        it->second.removeFixture( pfuid );

    m_fixtures.erase( it_del );
}

// ----------------------------------------------------------------------------
//
void Venue::addFixture( Fixture& pfixture ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_fixtures[ pfixture.getUID() ] = pfixture;
}

// ----------------------------------------------------------------------------
//
Fixture* Venue::getFixture( UID pfuid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureMap::iterator it = m_fixtures.find( pfuid );
    if ( it == m_fixtures.end() )
        return NULL;

    return &it->second;
}

// ----------------------------------------------------------------------------
//
FixturePtrArray Venue::getFixtures() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixturePtrArray list;
    FixtureMap::iterator it;

    for ( it=m_fixtures.begin(); it != m_fixtures.end(); it++ )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
FixtureNumber Venue::nextAvailableFixtureNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureNumber fixture_number = 1;

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ )
        if ( it->second.getFixtureNumber() >= fixture_number )
            fixture_number = it->second.getFixtureNumber()+1;

    return fixture_number;
}

// ----------------------------------------------------------------------------
//
Fixture* Venue::getFixtureByNumber( FixtureNumber fixture_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ )
        if ( it->second.getFixtureNumber() == fixture_number )
            return &it->second;
    return NULL;
}

// ----------------------------------------------------------------------------
//
FixtureGroupPtrArray Venue::getFixtureGroups( ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureGroupPtrArray list;
    FixtureGroupMap::iterator it;

    for ( it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); it++ )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
void Venue::addFixtureGroup( FixtureGroup& group ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_fixtureGroups[ group.getUID() ]= group;
}

// ----------------------------------------------------------------------------
//
void  Venue::deleteFixtureGroup( UID group_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureGroupMap::iterator it = m_fixtureGroups.find( group_id );
    if ( it != m_fixtureGroups.end() )
        m_fixtureGroups.erase( it );
}

// ----------------------------------------------------------------------------
//
FixtureGroup* Venue::getFixtureGroup( UID group_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureGroupMap::iterator it = m_fixtureGroups.find( group_id );
    if ( it == m_fixtureGroups.end() )
        return NULL;
    return &(it->second);
}

// ----------------------------------------------------------------------------
//
GroupNumber Venue::nextAvailableFixtureGroupNumber( void )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    GroupNumber group_number = 1;

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); it++ )
        if ( it->second.getGroupNumber() >= group_number )
            group_number = it->second.getGroupNumber()+1;

    return group_number;
}

// ----------------------------------------------------------------------------
//
FixtureGroup* Venue::getFixtureGroupByNumber( GroupNumber group_number )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); it++ )
        if ( it->second.getGroupNumber() == group_number )
            return &it->second;
    return NULL;
}

// ----------------------------------------------------------------------------
//
void Venue::addChase( Chase& chase ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_chases[ chase.getUID() ] = chase;
}

// ----------------------------------------------------------------------------
//
Chase *Venue::getChase( UID chase_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ChaseMap::iterator it = m_chases.find( chase_uid );
    if ( it == m_chases.end() )
        return NULL;

    return &it->second;
}

// ----------------------------------------------------------------------------
//
ChasePtrArray Venue::getChases( ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ChasePtrArray list;
    ChaseMap::iterator it;

    for ( it=m_chases.begin(); it != m_chases.end(); it++ )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
Chase* Venue::getChaseByNumber( ChaseNumber chase_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); it++ )
        if ( it->second.getChaseNumber() == chase_number )
            return &it->second;
    return NULL;
}

// ----------------------------------------------------------------------------
//
ChaseNumber Venue::nextAvailableChaseNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ChaseNumber chase_number = 1;

    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); it++ )
        if ( it->second.getChaseNumber() >= chase_number )
            chase_number = it->second.getChaseNumber()+1;

    return chase_number;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteChase( UID chase_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( getRunningChase() == chase_id )
        stopChase();

    // Remove all music mappings with this chase
    deleteMusicMappings( MST_CHASE, chase_id );

    ChaseMap::iterator it = m_chases.find( chase_id );
    if ( it != m_chases.end() )
        m_chases.erase( it );
}

// ----------------------------------------------------------------------------
//
ChaseController Venue::startChase( UID chase_id, ChaseRunMode run_mode ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    stopChase();

    m_chase_task = new ChaseTask( this, getChase( chase_id ), run_mode );
    m_chase_task->start( );

    return ChaseController( this, m_chase_task );
}

// ----------------------------------------------------------------------------
//
bool Venue::stopChase() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( m_chase_task ) {
        m_chase_task->stop();
        delete m_chase_task;
        m_chase_task = NULL;
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool Venue::copyChaseSteps( UID source_chase_id, 
                            UID target_chase_id ) 
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    Chase* chase_source = getChase( source_chase_id );
    Chase* chase_target = getChase( target_chase_id );

    if ( !chase_source || !chase_target )
        return false;

    chase_target->appendStep( chase_source->getSteps() );
    return true;
}

// ----------------------------------------------------------------------------
//
Scene *Venue::getScene( UID scene_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneMap::iterator it = m_scenes.find( scene_uid );
    if ( it == m_scenes.end() )
        return NULL;

    return &it->second;
}

// ----------------------------------------------------------------------------
//
Scene *Venue::getSceneByNumber( SceneNumber scene_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); it++ )
        if ( it->second.getSceneNumber() == scene_number )
            return &it->second;
    
    return NULL;
}

// ----------------------------------------------------------------------------
//
void Venue::selectScene( UID scene_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Scene *scene = getScene( scene_uid );
    if ( scene != NULL ) {
        clearAnimations();
        m_current_scene = scene_uid;
        loadScene();
    }
}

// ----------------------------------------------------------------------------
//
void Venue::deleteScene( UID scene_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( getDefaultScene()->getUID() == scene_uid )
        return;
    if ( isChaseRunning() )
        stopChase();
    if ( scene_uid == m_current_scene )
        selectScene( getDefaultScene()->getUID() );

    // Remove all chase steps with this scene
    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); it++ )
        it->second.removeScene( scene_uid );

    // Remove all music mappings with this scene
    deleteMusicMappings( MST_SCENE, scene_uid );

    Scene *scene = getScene( scene_uid );
    if ( scene != NULL ) {
        m_scenes.erase( m_scenes.find( scene_uid ) );
    }
}

// ----------------------------------------------------------------------------
//
SceneNumber Venue::nextAvailableSceneNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneNumber scene_number = 1;

    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); it++ )
        if ( it->second.getSceneNumber() >= scene_number )
            scene_number = it->second.getSceneNumber()+1;

    return scene_number;
}

// ----------------------------------------------------------------------------
//
void Venue::copyDefaultFixturesToScene( UID scene_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Scene* scene = getScene( scene_uid );
    if ( scene == NULL )
        return;

    ActorPtrArray actors = getDefaultScene()->getActors();
    for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); it++ )
        scene->addActor( *(*it) );
}

// ----------------------------------------------------------------------------
//
void Venue::copySceneFixtureToDefault( UID scene_uid, UID fixture_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor* actor = getScene( scene_uid )->getActor( fixture_uid );
    if ( actor )
        getDefaultScene()->addActor( *actor );
}

// ----------------------------------------------------------------------------
//
void Venue::clearAllCapturedActors( ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    getDefaultScene()->removeAllActors();
    getDefaultScene()->clearAnimations();
    m_captured_actor = 0;
}

// ----------------------------------------------------------------------------
//
void Venue::releaseActor( UID fixture_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    getDefaultScene()->removeActor( fixture_id );
    if ( m_captured_actor == fixture_id )
        m_captured_actor = 0;
}

// ----------------------------------------------------------------------------
//
ScenePtrArray Venue::getScenes() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ScenePtrArray list;

    SceneMap::iterator it;

    for ( it=m_scenes.begin(); it != m_scenes.end(); it++ )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
void Venue::addScene( Scene& scene ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_scenes[ scene.getUID() ] = scene;
}

// ----------------------------------------------------------------------------
//
void Venue::loadScene() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Scene* scene = getScene();

    STUDIO_ASSERT( scene != NULL, "Missing scene" );

    if ( m_animation_task )
        m_animation_task->stageScene( scene );
}

// ----------------------------------------------------------------------------
//
void Venue::clearAnimations() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( m_animation_task )
        m_animation_task->clearAnimations();
}

// ----------------------------------------------------------------------------
//
DWORD Venue::getAnimationSampleRate() {
    if ( m_animation_task )
        return m_animation_task->getAnimationSampleRate();
    return 0L;
}

// ----------------------------------------------------------------------------
//
void Venue::setAnimationSampleRate( DWORD sample_rate_ms ) {
    if ( m_animation_task )
        return m_animation_task->setAnimationSampleSampleRate( sample_rate_ms );
}

// ----------------------------------------------------------------------------
//
SceneActor* Venue::captureActor( UID fixture_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor * actor = NULL;

    if ( fixture_id > 0 ) {
        Fixture* fixture = getFixture( fixture_id );
        STUDIO_ASSERT( fixture, "Invalid fixture UID %ld", fixture_id );
        actor = getDefaultActor( fixture->getUID() );
        if ( !actor ) {
            getDefaultScene()->addActor( SceneActor( fixture ) );
            actor = getDefaultScene()->getActor( fixture->getUID() );
        }
    }

    m_captured_actor = fixture_id;

    return actor;
}

// ----------------------------------------------------------------------------
//
SceneActor* Venue::getCapturedActor() {
    return getDefaultActor( m_captured_actor );
}

// ----------------------------------------------------------------------------
//
SceneActor * Venue::getDefaultActor( UID fixture_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    // If we already have a default actor for this fixture, use it
    SceneActor * actor = getDefaultScene()->getActor( fixture_id );

    // If current scene is not default, and actor exists, copy to default scene
    if ( !actor && !isDefaultScene() ) {
        actor = getScene()->getActor( fixture_id );
        if ( actor != NULL )
            getDefaultScene()->addActor( *actor );
    }

    return actor;
}

// ----------------------------------------------------------------------------
//
void Venue::setHomePositions( LPBYTE dmx_packet )
{
    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
        Fixture* pf = &it->second;
        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            BYTE home_value = pf->getChannel( channel )->getHomeValue();
            if ( home_value != 0 )
                dmx_packet[ pf->getChannelAddress( channel ) - 1 ] = home_value;
        }
    }
}

// ----------------------------------------------------------------------------
//
void Venue::whiteoutChannels( LPBYTE dmx_packet ) {
    // Full on - all fixture RGBW channels and dimmers on (fixture must have RGBW and dimmer channels)

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
        Fixture* pf = &it->second;
        if ( !pf->canWhiteout() )
            continue;

        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel(channel);
            if ( !cp->canWhiteout() )       // Do not modify this channel during whiteout
                continue;

            switch ( cp->getType() ) {
                case CHNLT_RED:
                case CHNLT_BLUE:
                case CHNLT_GREEN:
                case CHNLT_WHITE:
                    loadChannel( dmx_packet, pf, channel,
                        (m_whiteout == WHITEOUT_ON || m_whiteout_strobe.isOn()) ? 255 : 0 );
                    break;

                case CHNLT_AUTOPROG:
                case CHNLT_COLOR_MACRO:
                case CHNLT_COLOR_SPEED:
                case CHNLT_PROG_SPEED:
                case CHNLT_GOBO:
                    loadChannel( dmx_packet, pf, channel, 0 );
                    break;
            }

            if ( cp->isDimmer() )
                loadChannel( dmx_packet, pf, channel,
                    (m_whiteout == WHITEOUT_ON || m_whiteout_strobe.isOn()) ? 
                        cp->getDimmerHighestIntensity() : cp->getDimmerLowestIntensity()  );
        }
    }
}

// ----------------------------------------------------------------------------
//
void Venue::loadSceneChannels( BYTE *dmx_packet, Scene* scene ) {
    STUDIO_ASSERT( scene != NULL, "Invalid scene (NULL)" );

    ActorPtrArray actors = scene->getActors();
    for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); it++ ) {
        Fixture* pf = getFixture( (*it)->getPFUID() );
        STUDIO_ASSERT( pf != NULL, "Invalid fixture %ld in scene %ld", (*it)->getPFUID(), scene->getUID() );

        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            BYTE value = (*it)->getChannelValue( channel );
            loadChannel( dmx_packet, pf, channel, value );
        }
    }
}

// ----------------------------------------------------------------------------
// Single point to load a DMX channel - applies dimmer and soft backout effects
//
void Venue::loadChannel( BYTE* dmx_packet, Fixture* pf, channel_t channel, BYTE value ) {
    dmx_packet[ pf->getChannelAddress( channel ) - 1 ] = adjustChannelValue( pf, channel, value );
}

// ----------------------------------------------------------------------------
//
BYTE Venue::adjustChannelValue( Fixture* pf, channel_t channel, BYTE value )
{
    if ( m_light_blackout && pf->getChannel( channel )->canBlackout() )
        value = 0;
    else if ( m_master_dimmer < 100 && pf->getChannel( channel )->isDimmer() ) {
        Channel* chnl = pf->getChannel( channel );

        if ( chnl->getDimmerLowestIntensity() < chnl->getDimmerHighestIntensity() ) {
            if ( value >= chnl->getDimmerLowestIntensity() && value <= chnl->getDimmerHighestIntensity() ) {
                value -= chnl->getDimmerLowestIntensity();
                value = (value * m_master_dimmer)/100;
                value += chnl->getDimmerLowestIntensity();
            }
        }
        else {
            if ( value >= chnl->getDimmerHighestIntensity() && value <= chnl->getDimmerLowestIntensity() ) {
                unsigned max = chnl->getDimmerLowestIntensity()-chnl->getDimmerHighestIntensity();
                value -= chnl->getDimmerHighestIntensity();
                value = max - (((max-value) * m_master_dimmer)/100);
                value += chnl->getDimmerHighestIntensity();
            }
        }
    }

    return value;
}

// ----------------------------------------------------------------------------
//
void Venue::setChannelValue( Fixture* pf, channel_t channel, BYTE value ) {
    STUDIO_ASSERT( channel < pf->getNumChannels(), 
        "Channel %d out of range for fixture %ld", channel, pf->getUID() );

    SceneActor * actor = captureActor( pf->getUID() );
    actor->setChannelValue( channel, value );

    value = adjustChannelValue( pf, channel, value );;

    getUniverse()->write( pf->getChannelAddress( channel ), value, true );
}

// ----------------------------------------------------------------------------
//
void Venue::writePacket( const BYTE* dmx_packet ) {
    BYTE packet[ DMX_PACKET_SIZE ];
    memcpy( packet, dmx_packet, DMX_PACKET_SIZE );

    // Overwrite any manually controlled fixtures
    if ( !isDefaultScene() )
        loadSceneChannels( packet, getDefaultScene() );

    // Handle whiteout effect (all venue fixtures)
    if ( getWhiteout() != WHITEOUT_OFF )               
        whiteoutChannels( packet );

    getUniverse()->write_all( packet);
}

// ----------------------------------------------------------------------------
//
void Venue::readPacket( BYTE* dmx_packet ) {
    getUniverse()->read_all( dmx_packet );
}

// ----------------------------------------------------------------------------
//
BYTE Venue::getChannelValue( Fixture* pfixture, channel_t channel ) {
    STUDIO_ASSERT( channel < pfixture->getNumChannels(), 
        "Channel %d out of range for fixture %ld", channel, pfixture->getUID() );

    SceneActor * actor = getDefaultActor( pfixture->getUID() );
    if ( actor )
        return actor->getChannelValue( channel );

    return pfixture->getChannel( channel )->getDefaultValue();
}

// ----------------------------------------------------------------------------
//
void Venue::mapMusicToScene( LPCSTR track_name, MusicSelectorType& type, UID& type_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    type = MST_SCENE;
    type_uid = getDefaultScene()->getUID();

    MusicSceneSelectMap::iterator it = m_music_scene_select_map.find( track_name );
    if ( it == m_music_scene_select_map.end() )
        it = m_music_scene_select_map.find( UNMAPPED_TRACK_NAME );
    if ( it != m_music_scene_select_map.end() ) {
        type_uid = it->second.m_selection_uid;
        type = it->second.m_selection_type;
    }
}

// ----------------------------------------------------------------------------
//    
void Venue::addMusicMapping( MusicSceneSelector& music_scene_selector ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_music_scene_select_map[ music_scene_selector.m_track_full_name ] = music_scene_selector;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteMusicMapping( LPCSTR track_full_name ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    MusicSceneSelectMap::iterator it_del = m_music_scene_select_map.find( track_full_name );
    if ( it_del != m_music_scene_select_map.end() ) {
        m_music_scene_select_map.erase( it_del );
    }
}

// ----------------------------------------------------------------------------
//
void Venue::deleteMusicMappings( MusicSelectorType type, UID uid  )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( MusicSceneSelectMap::iterator it = m_music_scene_select_map.begin();
          it != m_music_scene_select_map.end(); ) {
        if ( it->second.m_selection_type == type && it->second.m_selection_uid == uid )
            it = m_music_scene_select_map.erase( it );
        else
            it++;
    }
}
