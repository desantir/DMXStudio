/* 
Copyright (C) 2011-14 Robert DeSantis
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
#include "FindNextAvailable.h"

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
    m_whiteout_color( RGBWA::WHITE ),
    m_music_scene_select_enabled( false ),
    m_music_watcher( NULL ),
    m_audio_sample_size( 1024 )
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
            m_audio = AudioInputStream::createAudioStream( m_audio_capture_device, m_audio_sample_size, m_audio_boost, m_audio_boost_floor );
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
            if ( studio.isDMXRequired() ) {
                DMXStudio::log( StudioException( "Cannot start DMX universe [%s] (STATUS %d)", 
                                                 getDmxPort(), status ) );
                close();
                return false;
            }
            else {
                DMXStudio::log_status( "DMX UNIVERSE NOT STARTED [error on %s]", getDmxPort() );
            }
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
    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it ) {
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
bool Venue::deleteFixture( UID pfuid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureMap::iterator it_del = m_fixtures.find( pfuid );
    if ( it_del == m_fixtures.end() )
        return false;

    UID chase_id = getRunningChase();
    if ( chase_id != 0 )
        stopChase();

    clearAnimations();

    clearAllCapturedActors();

    selectScene( getDefaultScene()->getUID() );

    // Remove fixture from all dependant objects
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
        it->second.removeActor( pfuid );

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); ++it )
        it->second.removeFixture( pfuid );

    m_fixtures.erase( it_del );

    // Restart chase if one was running
    if ( chase_id )
        startChase( chase_id );

    return true;
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

    for ( it=m_fixtures.begin(); it != m_fixtures.end(); ++it )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
FixtureNumber Venue::nextAvailableFixtureNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FindNextAvailable<FixtureNumber> finder;
    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it )
        finder.insert( it->second.getFixtureNumber() );

    return (FixtureNumber)finder.nextAvailable();
}

// ----------------------------------------------------------------------------
//
Fixture* Venue::getFixtureByNumber( FixtureNumber fixture_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it )
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

    for ( it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); ++it )
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
bool  Venue::deleteFixtureGroup( UID group_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureGroupMap::iterator it = m_fixtureGroups.find( group_id );
    if ( it == m_fixtureGroups.end() )
        return false;

    // Remove fixture group from all dependant objects
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
        it->second.removeActor( group_id );

    m_fixtureGroups.erase( it );
    return true;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteAllFixtureGroups()
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    // Remove all fixture groups from all dependant objects
    for ( FixtureGroupMap::iterator fgit = m_fixtureGroups.begin(); fgit != m_fixtureGroups.end(); ++fgit ) {
        UID group_id = (*fgit).first;

        for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
            it->second.removeActor( group_id );
    }

    m_fixtureGroups.clear();
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

    FindNextAvailable<GroupNumber> finder;
    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); ++it )
        finder.insert( it->second.getGroupNumber() );

    return (GroupNumber)finder.nextAvailable();
}

// ----------------------------------------------------------------------------
//
FixtureGroup* Venue::getFixtureGroupByNumber( GroupNumber group_number )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); ++it )
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

    for ( it=m_chases.begin(); it != m_chases.end(); ++it )
        list.push_back( &it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
Chase* Venue::getChaseByNumber( ChaseNumber chase_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); ++it )
        if ( it->second.getChaseNumber() == chase_number )
            return &it->second;
    return NULL;
}

// ----------------------------------------------------------------------------
//
ChaseNumber Venue::nextAvailableChaseNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FindNextAvailable<ChaseNumber> finder;
    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); ++it )
        finder.insert( it->second.getChaseNumber() );

    return (ChaseNumber)finder.nextAvailable();
}

// ----------------------------------------------------------------------------
//
bool Venue::deleteChase( UID chase_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ChaseMap::iterator it = m_chases.find( chase_id );
    if ( it == m_chases.end() )
        return false;

    if ( getRunningChase() == chase_id )
        stopChase();

    // Remove all music mappings with this chase
    deleteMusicMappings( MST_CHASE, chase_id );

    m_chases.erase( it );
    return true;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteAllChases()
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    stopChase();

    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); ++it ) {
        // Remove all music mappings with this chase
        deleteMusicMappings( MST_CHASE, it->first );
    }

    m_chases.clear();
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

    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
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
bool Venue::deleteScene( UID scene_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( getDefaultScene()->getUID() == scene_uid )
        return false;

    SceneMap::iterator it = m_scenes.find( scene_uid );
    if ( it == m_scenes.end() )
        return false;

    // Stop running chase as it may refer to the doomed scene
    UID chase_id = getRunningChase();
    if ( chase_id != 0 )
        stopChase();
    if ( scene_uid == m_current_scene )
        selectScene( getDefaultScene()->getUID() );

    // Remove all chase steps with this scene
    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); ++it )
        it->second.removeScene( scene_uid );

    // Remove all music mappings with this scene
    deleteMusicMappings( MST_SCENE, scene_uid );

    m_scenes.erase( it );

    // Restart chase if one was running
    if ( chase_id )
        startChase( chase_id );

    return true;
}

// ----------------------------------------------------------------------------
//
void Venue:: deleteAllScenes()
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    deleteAllChases();

    selectScene( getDefaultScene()->getUID() );

    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ) {
        if ( it->second.getSceneNumber() != DEFAULT_SCENE_NUMBER ) {
            // Remove all music mappings with this scene
            deleteMusicMappings( MST_SCENE, it->first );

            it = m_scenes.erase( it );
        }
        else
            it++;
    }
}

// ----------------------------------------------------------------------------
//
SceneNumber Venue::nextAvailableSceneNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FindNextAvailable<SceneNumber> finder;
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
        finder.insert( it->second.getSceneNumber() );

    return (ChaseNumber)finder.nextAvailable();
}

// ----------------------------------------------------------------------------
//
void Venue::moveDefaultFixturesToScene( UID scene_uid, boolean keep_groups, boolean clear_default ) {
   moveDefaultFixturesToScene( scene_uid, getDefaultScene()->getActorUIDs(), keep_groups, clear_default );
}

// ----------------------------------------------------------------------------
//
void Venue::moveDefaultFixturesToScene( UID scene_uid, UIDArray actor_uids, boolean keep_groups, boolean clear_default ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Scene* scene = getScene( scene_uid );
    if ( scene == NULL )
        return;

    for ( UIDArray::iterator it = actor_uids.begin(); it != actor_uids.end(); ++it ) {
        SceneActor* actor = getDefaultScene()->getActor( *it );
        if ( actor == NULL )
            continue;

        if ( actor->isGroup() && !keep_groups ) {               // Explode groups - add individual fixtures
            BYTE channel_values[DMX_PACKET_SIZE];
            actor->getChannelValues( channel_values );

            for ( Fixture* fixture : resolveActorFixtures( actor ) ) {
                SceneActor new_actor( fixture );
                new_actor.setChannelValues( fixture->getNumChannels(), channel_values );
                scene->addActor( new_actor );
            }
        }
        else
            scene->addActor( *actor );

        if ( clear_default )
            getDefaultScene()->removeActor( actor->getActorUID() );
    }

    // Restart the scene if this is the active scene
    if ( getCurrentSceneUID() == scene_uid )
        loadScene();
}

// ----------------------------------------------------------------------------
//
void Venue::copySceneFixtureToDefault( UID scene_uid, UID actor_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor* actor = getScene( scene_uid )->getActor( actor_uid );
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
void Venue::releaseActor( UID actor_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    getDefaultScene()->removeActor( actor_id );
    if ( m_captured_actor == actor_id )
        m_captured_actor = 0;
}

// ----------------------------------------------------------------------------
//
ScenePtrArray Venue::getScenes() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ScenePtrArray list;

    SceneMap::iterator it;

    for ( it=m_scenes.begin(); it != m_scenes.end(); ++it )
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
SceneActor* Venue::captureFixture( UID fixture_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor * actor = NULL;

    if ( fixture_uid > 0 ) {
        Fixture* fixture = getFixture( fixture_uid );
        STUDIO_ASSERT( fixture, "Invalid fixture UID %ld", fixture_uid );

        actor = getDefaultActor( fixture->getUID() );
        if ( !actor ) {
            getDefaultScene()->addActor( SceneActor( fixture ) );
            actor = getDefaultScene()->getActor( fixture->getUID() );
            m_animation_task->updateChannels();     // Latch in the new default values
        }
    }

    m_captured_actor = fixture_uid;

    return actor;
}

// ----------------------------------------------------------------------------
//
SceneActor* Venue::captureFixtureGroup( UID group_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor * actor = NULL;

    if ( group_uid > 0 ) {
        FixtureGroup* group = getFixtureGroup( group_uid );
        STUDIO_ASSERT( group, "Invalid fixture group UID %ld", group_uid );

        actor = getDefaultActor( group->getUID() );
        if ( !actor ) {
            getDefaultScene()->addActor( SceneActor( this, group ) );
            actor = getDefaultScene()->getActor( group->getUID() );
            m_animation_task->updateChannels();     // Latch in the new default values
        }
    }

    m_captured_actor = group_uid;

    return actor;
}

// ----------------------------------------------------------------------------
//
SceneActor* Venue::getCapturedActor() {
    return getDefaultActor( m_captured_actor );
}

// ----------------------------------------------------------------------------
//
SceneActor * Venue::getDefaultActor( UID actor_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    return getDefaultScene()->getActor( actor_id );
}

// ----------------------------------------------------------------------------
//
void Venue::setHomePositions( LPBYTE dmx_packet )
{
    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it ) {
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

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it ) {
        Fixture* pf = &it->second;
        if ( !pf->canWhiteout() )
            continue;

        RGBWA&color = (m_whiteout == WHITEOUT_ON || m_whiteout_strobe.isOn()) ?
            getWhiteoutColor() : RGBWA::BLACK;

        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel(channel);
            if ( !cp->canWhiteout() )       // Do not modify this channel during whiteout
                continue;

            switch ( cp->getType() ) {
                case CHNLT_RED:
                    loadChannel( dmx_packet, pf, channel, color.red() );
                    break;

                case CHNLT_BLUE:
                    loadChannel( dmx_packet, pf, channel, color.blue() );
                    break;

                case CHNLT_GREEN:
                    loadChannel( dmx_packet, pf, channel, color.green() );
                    break;

                case CHNLT_WHITE:
                    loadChannel( dmx_packet, pf, channel, color.white() );
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
    for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); ++it ) {
        SceneActor* actor = (*it);

        for ( Fixture* pf : resolveActorFixtures( actor ) ) {
            for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
                BYTE value = actor->getChannelValue( channel );
                loadChannel( dmx_packet, pf, channel, value );
            }                
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
void Venue::captureAndSetChannelValue( SceneActor& target_actor, channel_t channel, BYTE value ) {
    if ( target_actor.isGroup() ) {
        SceneActor* actor = captureFixtureGroup( target_actor.getActorUID() );
        actor->setChannelValue( channel, value );

        FixtureGroup* group = getFixtureGroup( target_actor.getActorUID() );
        UIDSet fixtures = group->getFixtures();

        for ( UIDSet::iterator it2=fixtures.begin(); it2 != fixtures.end(); ++it2 ) {
            Fixture* pf = getFixture( *it2 );
            STUDIO_ASSERT( pf, "Invalid fixture %lu in group %lu", (*it2), target_actor.getActorUID() );

            if ( pf->getNumChannels() > channel ) {
                value = adjustChannelValue( pf, channel, value );
                getUniverse()->write( pf->getChannelAddress( channel ), value, true );
            }
        }
    }
    else {
        Fixture* pf = getFixture( target_actor.getActorUID() );
        STUDIO_ASSERT( channel < pf->getNumChannels(), "Channel %d out of range for fixture %ld", channel, pf->getUID() );

        SceneActor* actor = captureFixture( pf->getUID() );
        actor->setChannelValue( channel, value );

        value = adjustChannelValue( pf, channel, value );
        getUniverse()->write( pf->getChannelAddress( channel ), value, true );
    }
}

// ----------------------------------------------------------------------------
//
void Venue::writePacket( const BYTE* dmx_packet ) {
    BYTE packet[ DMX_PACKET_SIZE ];
    memcpy( packet, dmx_packet, DMX_PACKET_SIZE );

    // Overwrite any manually controlled fixtures
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

    SceneActor* actor = getDefaultActor( pfixture->getUID() );
    if ( actor )
        return actor->getChannelValue( channel );

    if ( !isDefaultScene() ) {
        actor =  getScene()->getActor( pfixture->getUID() );
        if ( actor )
            return actor->getChannelValue( channel );
    }

    return pfixture->getChannel( channel )->getDefaultValue();
}

// ----------------------------------------------------------------------------
//
BYTE Venue::getChannelValue( SceneActor& target_actor, channel_t channel ) {
    SceneActor* actor = getDefaultActor( target_actor.getActorUID() );
    if ( actor )
        return actor->getChannelValue( channel );

    if ( !isDefaultScene() ) {
        actor = getScene()->getActor( target_actor.getActorUID() );
        if ( actor )
            return actor->getChannelValue( channel );
    }

    if ( channel < target_actor.getNumChannels() )
        return target_actor.getChannelValue( channel );

    Fixture* pf = NULL;

    if ( target_actor.isGroup() )
        pf = getGroupRepresentative( target_actor.getActorUID() );
    else
        pf = getFixture( target_actor.getActorUID() );

    STUDIO_ASSERT( pf != NULL && pf->getNumChannels() > channel, 
                   "Attempt to load invalid channel %u from fixture %lu", 
                   channel, pf->getFUID() );

    return pf->getChannel( channel )->getDefaultValue();
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

// ----------------------------------------------------------------------------
//
void Venue::clearMusicMappings() 
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_music_scene_select_map.clear();
}

// ----------------------------------------------------------------------------
//    
void Venue::addMusicMappings( std::vector<MusicSceneSelector>& selectors ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( std::vector<MusicSceneSelector>::iterator it=selectors.begin(); it != selectors.end(); ++it )
        m_music_scene_select_map[ (*it).m_track_full_name ] = (*it);
}

// ----------------------------------------------------------------------------
//    
FixturePtrArray Venue::resolveActorFixtures( SceneActor* actor )
{
    FixturePtrArray fixtures;

    if ( actor->isGroup() ) {
        FixtureGroup* group = getFixtureGroup( actor->getActorUID() );
        STUDIO_ASSERT( group, "Group %ul in actor not found", actor->getActorUID() );

        for ( UID fixture_uid : group->getFixtures() )
            fixtures.push_back( getFixture( fixture_uid ) );
    }
    else {
        Fixture* pf = getFixture( actor->getActorUID() );
        STUDIO_ASSERT( pf, "Fixture %ul in actor not found", actor->getActorUID() );
        fixtures.push_back( pf );
    }

    return fixtures;
}

// ----------------------------------------------------------------------------
// Fixture groups are allows to contain any fixures - they do not need to be the same
// type nor have the same number of channels.  However, there are times when groups are
// treated as a set of similar fixtures.  This method provide a representative for the
// group to setup channels and get default values.

Fixture*  Venue::getGroupRepresentative( UID group_uid ) {
    FixtureGroup* group = getFixtureGroup( group_uid );
    STUDIO_ASSERT( group, "Invalid group id %ul", group_uid );

    Fixture* fixture = NULL;

    for ( UID fixture_uid : group->getFixtures() ) {
        Fixture* f = getFixture( fixture_uid );
        STUDIO_ASSERT( f != NULL, "Missing fixture for %lu", fixture_uid );

        if ( fixture == NULL || f->getNumChannels() > fixture->getNumChannels() )
            fixture = f;
    }

    return fixture;
}