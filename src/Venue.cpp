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

#include "DMXStudio.h"
#include "OpenDMXDriver.h"
#include "Venue.h"
#include "AnimationEngine.h"
#include "AnimationDefinition.h"
#include "FindNextAvailable.h"
#include "MusicWatcher.h"
#include "SceneSequence.h"
#include "SceneStrobeAnimator.h"
#include "ScenePulse.h"
#include "SceneMovementAnimator.h"
#include "SceneFixtureDimmer.h"
#include "ScenePatternDimmer.h"
#include "SceneColorFader.h"

// ----------------------------------------------------------------------------
//
Venue::Venue(void) :
    Threadable( "Venue" ),
    m_uid_pool( 1L ),
    m_chase_task( NULL ),
    m_animation_task( NULL ),
    m_master_dimmer( 100 ),
    m_auto_backout_ms( 0 ),
    m_mute_blackout( false ),
    m_hard_blackout( false ),
    m_track_fixtures( false ),
    m_audio( NULL ),
    m_volume( NULL ),
    m_sound_detector( NULL ),
    m_audio_boost( 0.0f ),
    m_audio_boost_floor( 0.0009f ),
    m_captured_actor( 0 ),
    m_whiteout( WHITEOUT_OFF ),
    m_whiteout_strobe_ms( 100 ),
    m_whiteout_fade_ms( 0 ),
    m_whiteout_color( RGBWA::WHITE ),
    m_whiteout_effect( WHITEOUT_EFFECT_SINGLE_COLOR ),
    m_music_scene_select_enabled( false ),
    m_whiteout_positive_index( 0 ),
    m_whiteout_pulse_index( 0 ),
    m_music_watcher( NULL ),
    m_audio_sample_size( 1024 ),
    m_default_scene_uid(0L),
    m_test_animation_uid( std::numeric_limits<DWORD>::max() ),
    m_animation_speed( 100 ),
    m_packet_changed( false )
{
    m_whiteout_strobe.setNegative( RGBWA::BLACK );

    memset( m_multi_universe_packet, 0, MULTI_UNIV_PACKET_SIZE );

    m_current_scene = addScene( Scene( NOUID, DEFAULT_SCENE_NUMBER, "Workspace", "" ), true );

    AnimationDefinition* colorAnim = 
        new SceneSequence( NOUID, TRUE, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, 1000 ) );
    colorAnim->setAnimationNumber( QUICK_COLOR_NUMBER );
    colorAnim->setName( QUICK_COLOR_NAME );
    addAnimation( colorAnim );

    AnimationDefinition* movementAnim = 
        new SceneMovementAnimator( NOUID, TRUE, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, 1000 ), MovementAnimation() );
    movementAnim->setAnimationNumber( QUICK_MOVEMENT_NUMBER );
    movementAnim->setName( QUICK_MOVEMENT_NAME );
    addAnimation( movementAnim );

    AnimationDefinition* dimmerAnim = 
        new SceneMovementAnimator( NOUID, TRUE, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, 1000 ), MovementAnimation() );
    dimmerAnim->setAnimationNumber( QUICK_DIMMER_NUMBER );
    dimmerAnim->setName( QUICK_DIMMER_NAME );
    addAnimation( dimmerAnim );
}

// ----------------------------------------------------------------------------
//
Venue::~Venue(void)
{
    close();

    // Release universes
    for ( UniverseMap::iterator it=m_universes.begin(); it != m_universes.end(); ++it ) {
        delete (*it).second;
    }
}

// ----------------------------------------------------------------------------
//
template <typename  T, typename  S>
static S nextAvailable( T& map ) {
    S defaultNumber = 1L;
    UINT64 max_timestamp = 0L;

    FindNextAvailable<S> finder;
    for ( auto it=map.begin(); it != map.end(); it++ ) {
        finder.insert( it->second.getNumber() );

        if ( it->second.getCreated() > max_timestamp ) {
            defaultNumber = it->second.getNumber()+1;
            max_timestamp = it->second.getCreated();
        }
    }

    return (S)finder.nextAvailable( defaultNumber );
}

// ----------------------------------------------------------------------------
//
bool Venue::open(void)
{
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

        for ( auto it : m_universes ) {
            Universe* universe = it.second;
            DMX_STATUS status = universe->start();

            if ( status != DMX_OK ) {
                if ( studio.isDMXRequired() ) {
                    DMXStudio::log( StudioException( "Cannot start DMX universe %d (STATUS %d)", 
                                                     universe->getId(), status ) );
                    close();
                    return false;
                }
                else {
                    DMXStudio::log_status( "DMX UNIVERSE %d NOT STARTED (STATUS %d)", universe->getId(), status );
                }
            }
        }

        // Validate that all fixures have definitions
        for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); it++ ) {
            try {
                it->second.getFixtureDefinition();
            }
            catch ( StudioException& exception ) {
                DMXStudio::log( exception );

                // Attempt to replace with a "custom" fixture"
                FixtureDefinition* fd = FixtureDefinition::lookupFixture( "Custom", "Generic", 1 );
                if ( fd == NULL )
                    throw StudioException( "Unable to replace fixure with generic definition" );

                it->second.setFUID( fd->getFUID() );
            }
        }

        // Automatically create Hue fixtures if needed and allowed
        autoCreateUniverseFixtures();

        // Setup global palette entries if missing
        setupGlobalPaletteEntries();

        // Preset utilty channels for all current fixtures
        scanFixtureChannels();

        // Compute final actor values
        for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); it++ ) {
            computeActorFinalValues( &it->second );
        }

        // Seek and destroy any orphan privae animations
        for ( AnimationMap::iterator it=m_animations.begin(); it != m_animations.end();  ) {
            AnimationDefinition* animation = it->second.get();

            if ( !animation->isShared() ) {
                bool referenced = false;

                for ( SceneMap::value_type& scene_pair : m_scenes ) {
                    if ( scene_pair.second.getAnimationByUID( animation->getUID() ) ) {
                        referenced = true;
                        break;
                    }
                }

                if ( !referenced ) {
                    studio.log_warning( "Removing unreferenced private animation %lu - '%s'", animation->getUID(), animation->getName() );
                    it = m_animations.erase( it );
                    continue;
                } 
            }

            ++it;
        }

        m_animation_task = new AnimationEngine( this );
        m_animation_task->start();

        m_chase_task = new ChaseEngine( this );
        m_chase_task->start();

        try {
            playScene( m_current_scene, SLM_LOAD );
        }
        catch ( std::exception& ex ) {
            studio.log( ex );
        }

        if ( studio.hasMusicPlayer() ) {
            m_music_watcher = new MusicWatcher( studio.getMusicPlayer(), this );
            m_music_watcher->start();
        }

		// Start any default scene animations
		for ( AnimationReference& a : getDefaultScene()->animations() )
			m_animation_task->queue( PlayTransientAnimation, a.getUID(), m_default_scene_uid );

        if ( isTrackFixtures() )
            startThread();

        DMXStudio::fireEvent( ES_VENUE, 0L, EA_START, m_name );
    }

    return true;
}

// ----------------------------------------------------------------------------
//
bool Venue::close(void) {
    stopThread();

    if ( m_music_watcher && m_music_watcher->stop() ) {
        delete m_music_watcher;
        m_music_watcher = NULL;
    }

    if ( m_chase_task && m_chase_task->stop() ) {
        delete m_chase_task;
        m_chase_task = NULL;
    }

    if ( m_animation_task && m_animation_task->stop() ) {
        delete m_animation_task;
        m_animation_task = NULL;
    }

    // Stop all universes
    for ( UniverseMap::iterator it=m_universes.begin(); it != m_universes.end(); ++it ) {
        (*it).second->stop();
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

    DMXStudio::fireEvent( ES_VENUE, 0L, EA_STOP, m_name );

    return true;
}

// ----------------------------------------------------------------------------
//
UID Venue::whoIsAddressRange( universe_t universe_id, channel_address start_address, channel_address end_address ) {
    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it ) {
        if ( it->second.getUniverseId() != universe_id )
            continue;

		channel_address fixture_start = it->second.getAddress();
		channel_address fixture_end = fixture_start + it->second.getNumChannels() - 1;

        if ( fixture_end >= start_address && fixture_start <= end_address )
            return it->second.getUID();
    }

    return 0;
}

// ----------------------------------------------------------------------------
//
channel_address Venue::findFreeAddressRange( universe_t universe_id, UINT num_channels ) 
{
    // This is brute force ..
    for ( channel_address base=1; base <= DMX_PACKET_SIZE; ) {
        UID uid = whoIsAddressRange( universe_id, base, base+num_channels-1 );
        if ( !uid )
            return base;
        base += getFixture( uid )->getNumChannels();
    }

    return (channel_address)INVALID_CHANNEL;
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

    clearAllAnimations();

    clearAllCapturedActors();

    selectScene( getDefaultScene()->getUID() );

    // Remove fixture from all dependant objects
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it ) {
        bool removed = it->second.removeActor( pfuid );

        if ( removed )
            fireEvent( ES_SCENE, it->first, EA_CHANGED );
    }

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); ++it ) {
        bool removed =  it->second.removeFixture( pfuid );

        if ( removed )
            fireEvent( ES_FIXTURE_GROUP, it->first, EA_CHANGED );
    }

    m_fixtures.erase( it_del );

    removeFixtureFromPalette( pfuid );

    // Preset utilty channels for all current fixtures
    scanFixtureChannels();

    fireEvent( ES_FIXTURE, pfuid, EA_DELETED );

    // Restart chase if one was running
    if ( chase_id )
        startChase( chase_id );

    return true;
}

// ----------------------------------------------------------------------------
//
UID Venue::addFixture( Fixture& pfixture ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( pfixture.getUID() == 0L )
        pfixture.setUID( allocUID() );

    m_fixtures[ pfixture.getUID() ] = pfixture;

    // Preset utilty channels for all current fixtures
    scanFixtureChannels();

    fireEvent( ES_FIXTURE, pfixture.getUID(), EA_NEW );

    return pfixture.getUID();
}

// ----------------------------------------------------------------------------
//
void Venue::fixtureUpdated( UID pfuid )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    // Preset utilty channels for all current fixtures
    scanFixtureChannels();

    fireEvent( ES_FIXTURE, pfuid, EA_CHANGED );
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

    return nextAvailable<FixtureMap, FixtureNumber>( m_fixtures );
}

// ----------------------------------------------------------------------------
//
UID Venue::getFixtureByNumber( FixtureNumber fixture_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it )
        if ( it->second.getFixtureNumber() == fixture_number )
            return it->first;
    return NOUID;
}

// ----------------------------------------------------------------------------
//
Fixture* Venue::getFixtureByType( FUID fuid, universe_t universe, channel_address base_dmx_channel ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureMap::iterator it=m_fixtures.begin(); it != m_fixtures.end(); ++it )
        if ( it->second.getFUID() == fuid && it->second.getUniverseId() == universe && it->second.getAddress() == base_dmx_channel )
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
UID Venue::addFixtureGroup( FixtureGroup& group ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( group.getUID() == 0L )
        group.setUID( allocUID() );

    m_fixtureGroups[ group.getUID() ]= group;

    fireEvent( ES_FIXTURE_GROUP, group.getUID(), EA_NEW );

    return group.getUID();
}

// ----------------------------------------------------------------------------
//
bool  Venue::deleteFixtureGroup( UID group_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureGroupMap::iterator it = m_fixtureGroups.find( group_id );
    if ( it == m_fixtureGroups.end() )
        return false;

    // Remove fixture group from all dependant objects
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it ) {
        bool removed = it->second.removeActor( group_id );

        if ( removed )
            fireEvent( ES_SCENE, it->first, EA_CHANGED );
    }

    m_fixtureGroups.erase( it );

    fireEvent( ES_FIXTURE_GROUP, group_id, EA_DELETED );

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

        for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it ) {
            bool removed = it->second.removeActor( group_id );

            if ( removed )
                fireEvent( ES_SCENE, it->first, EA_CHANGED );
        }
    }

    m_fixtureGroups.clear();

    fireEvent( ES_FIXTURE_GROUP, 0, EA_DELETED );
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

    return nextAvailable<FixtureGroupMap, GroupNumber>( m_fixtureGroups );
}

// ----------------------------------------------------------------------------
//
UID Venue::getFixtureGroupByNumber( GroupNumber group_number )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( FixtureGroupMap::iterator it=m_fixtureGroups.begin(); it != m_fixtureGroups.end(); ++it )
        if ( it->second.getGroupNumber() == group_number )
            return it->first;

    return NOUID;
}

// ----------------------------------------------------------------------------
//
UID Venue::addChase( Chase& chase ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( chase.getUID() == 0L )
        chase.setUID( allocUID() );

    m_chases[ chase.getUID() ] = chase;

    fireEvent( ES_CHASE, chase.getUID(), EA_NEW );

    return chase.getUID();
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
UID Venue::getChaseByNumber( ChaseNumber chase_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); ++it )
        if ( it->second.getChaseNumber() == chase_number )
            return it->first;
    return NOUID;
}

// ----------------------------------------------------------------------------
//
ChaseNumber Venue::nextAvailableChaseNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    return nextAvailable<ChaseMap, ChaseNumber>( m_chases );
}

// ----------------------------------------------------------------------------
//
bool Venue::deleteChase( UID chase_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    ChaseMap::iterator it = m_chases.find( chase_uid );
    if ( it == m_chases.end() )
        return false;

    if ( getRunningChase() == chase_uid )
        stopChase();

    // Remove all music mappings with this chase
    deleteMusicMappings( MST_CHASE, chase_uid );

    m_chases.erase( it );

    fireEvent( ES_CHASE, chase_uid, EA_DELETED );

    return true;
}

// ----------------------------------------------------------------------------
//
void Venue::chaseUpdated( UID chase_uid ) {
    fireEvent( ES_CHASE, chase_uid, EA_CHANGED );
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

    fireEvent( ES_CHASE, 0L, EA_DELETED );
}

// ----------------------------------------------------------------------------
//
bool Venue::startChase( UID chase_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( !m_chase_task )
        return false;

    return m_chase_task->startChase( getChase( chase_id ) );
}

// ----------------------------------------------------------------------------
//
bool Venue::stopChase() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( !m_chase_task )
        return false;

    return m_chase_task->stopChase();
}

// ----------------------------------------------------------------------------
//
void Venue::chaseStep( int steps ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( m_chase_task )
        m_chase_task->chaseStep( steps );
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

    fireEvent( ES_CHASE, target_chase_id, EA_CHANGED );

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
UID Venue::getSceneByNumber( SceneNumber scene_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
        if ( it->second.getSceneNumber() == scene_number )
            return it->first;
    
    return NOUID;
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
    for ( ChaseMap::value_type& pair : m_chases ) {
        bool removed = pair.second.removeScene( scene_uid );
        if ( removed )
            fireEvent( ES_CHASE, pair.first, EA_CHANGED );
    }

    // Remove all music mappings with this scene
    deleteMusicMappings( MST_SCENE, scene_uid );

    // Delete all private animations belonging to this scene
    AnimationReferenceArray animations = it->second.animations();       // Copy as it may change when anims are deleted

    // Delete the scene before possibly deleting private animations to avoid phantom update messages
    m_scenes.erase( it );

    for ( AnimationReference& animation_ref : animations ) {
        AnimationDefinition* animation = getAnimation( animation_ref.getUID() );
        if ( !animation->isShared() )
            deleteAnimation( animation->getUID() );
    }

    fireEvent( ES_SCENE, scene_uid, EA_DELETED );

    // Restart chase if one was running
    if ( chase_id )
        startChase( chase_id );

    return true;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteAllScenes()
{
    CSingleLock lock( &m_venue_mutex, TRUE );

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

    fireEvent( ES_SCENE, 0L, EA_DELETED );
}

// ----------------------------------------------------------------------------
//
SceneNumber Venue::nextAvailableSceneNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    return nextAvailable<SceneMap, SceneNumber>( m_scenes );
}

// ----------------------------------------------------------------------------
//
void Venue::moveDefaultFixturesToScene( UID scene_uid, boolean keep_groups, boolean clear_default, boolean copy_animations ) {
    UIDArray scene_uids;
    scene_uids.push_back( scene_uid );

   moveDefaultFixturesToScene( scene_uids, getDefaultScene()->getActorUIDs(), keep_groups, clear_default, copy_animations );
}

// ----------------------------------------------------------------------------
// Important to have copy of actor_uids
void Venue::moveDefaultFixturesToScene( UIDArray scene_uids, UIDArray actor_uids, boolean keep_groups, boolean clear_default, boolean copy_animations ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Scene* defaultScene = getDefaultScene();
    STUDIO_ASSERT( defaultScene != NULL, "Default scene not found" );

    for ( UID scene_uid : scene_uids ) {
        Scene* scene = getScene( scene_uid );
        if ( scene == NULL )
            continue;

        bool changed = false;

        for ( UIDArray::iterator it = actor_uids.begin(); it != actor_uids.end(); ++it ) {
            SceneActor* actor = defaultScene->getActor( *it );
            if ( actor == NULL )
                continue;

            if ( actor->isGroup() && !keep_groups ) {               // Explode groups - add individual fixtures
                ChannelValues channel_values = actor->getBaseChannelValues();

                for ( Fixture* fixture : resolveActorFixtures( actor ) ) {
                    SceneActor new_actor( fixture );
                    new_actor.setBaseChannelValues( channel_values );
                    new_actor.setPaletteReferences( actor->getPaletteReferences() );
                    scene->addActor( new_actor );
                    computeActorFinalValues( scene );
                }
            }
            else
                scene->addActor( *actor );

            changed = true;
        }

        if ( copy_animations ) {
            for ( AnimationReference& animation_ref : defaultScene->animations() ) {
				AnimationReference ref = animation_ref;

				AnimationDefinition* animation = getAnimation( ref.getUID() );

				// If this is the default animation, then we need to make a copy
				if ( !animation->isShared() || animation->getNumber() <= 2 ) {
					CString clone_name;
					clone_name.Format( "%s (copy)", animation->getName() );

					AnimationDefinition* clone = animation->clone();
					clone->setAnimationNumber( 0 );
					clone->setName( clone_name );
					clone->setUID( 0 );
                    clone->setShared( false );

					ref.setUID( addAnimation( clone ) );
				}

                if ( !keep_groups ) {
                    UIDArray newActors;

                    for ( UID actor_uid : ref.getActors() ) {
                        FixtureGroup* group = getFixtureGroup( actor_uid );
                        if ( group != NULL ) {
                            UIDSet fixtures = group->getFixtures();
                            newActors.insert( newActors.end(), fixtures.begin(), fixtures.end() );
                        }
                        else 
                            newActors.push_back( actor_uid );
                    }

                    ref.setActors( newActors );
                }
                
				scene->addAnimation( ref );

                changed = true;
            }
        }

        // Restart the scene if this is the active scene
        if ( changed )
            studio.getVenue()->sceneUpdated( scene_uid );
    }

    // Clear actors and animations from default if needed
    if ( clear_default ) {
        for ( UIDArray::iterator it = actor_uids.begin(); it != actor_uids.end(); ++it )
            defaultScene->removeActor( (*it) );

        if ( copy_animations ) {
            AnimationReferenceArray free_list( defaultScene->animations() );

            for ( AnimationReference& animation_ref : free_list )
                releaseAnimation( animation_ref.getUID() );
        }

        studio.getVenue()->sceneUpdated( m_default_scene_uid );
    }
}

// ----------------------------------------------------------------------------
//
void Venue::copySceneFixtureToDefault( UID scene_uid, UID actor_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor* actor = getScene( scene_uid )->getActor( actor_uid );
    if ( actor ) {
        getDefaultScene()->addActor( *actor );

        addActorToDefaultAnimations( actor->getActorUID() );
    }

    sceneUpdated( m_default_scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::setSceneAnimationReferences( Scene* scene, AnimationReferenceArray& animation_refs, bool copyPrivateAnimations ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    AnimationReferenceArray previous_references = scene->animations();

    scene->clearAnimations();

    for ( AnimationReference& animation_ref : animation_refs ) {
        AnimationReference ref = animation_ref;

        AnimationDefinition* animation = getAnimation( ref.getUID() );

        STUDIO_ASSERT( animation != NULL, "Scene %lu references invalid animation %lu", scene->getUID(), ref.getUID() );

        // If this is a private animation then we may need to make a copy
        if ( copyPrivateAnimations && (!animation->isShared() || animation->getNumber() <= 2) ) {
            CString clone_name;
            clone_name.Format( "%s (copy)", animation->getName() );

            AnimationDefinition* clone = animation->clone();
            clone->setAnimationNumber( 0 );
            clone->setName( clone_name );
            clone->setUID( 0 );
            clone->setShared( false );
            
            ref.setUID( addAnimation( clone ) );
        }

        scene->addAnimation( ref );

        // Removed used references from the old list
        for ( AnimationReferenceArray::iterator it=previous_references.begin(); it != previous_references.end(); ++it ) {
            if ( (*it).getUID() == ref.getUID() ) {
                previous_references.erase( it );
                break;
            }
        }
    }

    // Delete any orphan non-shared animations
    for ( AnimationReference& ref : previous_references ) {
        AnimationDefinition* animation = getAnimation( ref.getUID() );
        if ( animation != NULL && !animation->isShared() )
            deleteAnimation( animation->getUID() );
    }
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
UID Venue::addScene( Scene& scene, bool isDefault ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( scene.getUID() == 0L )
        scene.setUID( allocUID() );

    m_scenes[ scene.getUID() ] = scene;

    if ( isDefault )
        m_default_scene_uid = scene.getUID();

    fireEvent( ES_SCENE, scene.getUID(), EA_NEW );

    return scene.getUID();
}

// ----------------------------------------------------------------------------
//
void Venue::sceneUpdated( UID scene_uid ) {
    if ( scene_uid == m_current_scene )
        playScene( m_current_scene, SLM_LOAD );
    else if ( scene_uid == m_default_scene_uid && isRunning() ) {	// Reload channel values to pick up and default act0r changes
		if ( getDefaultScene()->animations().size() > 0 ) 			// Restart captured animations
			for ( AnimationReference& a : getDefaultScene()->animations() )
				m_animation_task->queue( PlayTransientAnimation, a.getUID(), m_default_scene_uid );
		else
			m_animation_task->updateChannels();
	}

    fireEvent( ES_SCENE, scene_uid, EA_CHANGED );
}

// ----------------------------------------------------------------------------
//
void Venue::playScene( UID scene_uid, SceneLoadMethod method ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Scene* scene = getScene( scene_uid );

    STUDIO_ASSERT( scene != NULL, "Missing scene %lu", scene_uid );

    if ( method == SLM_LOAD ) {
        // When changing from default scene, clear all playing animations (sigh)
        if ( m_current_scene == m_default_scene_uid 
             && scene_uid != m_default_scene_uid 
             && getDefaultScene()->getNumAnimations() > 0 ) {
            for ( auto anim : getDefaultScene()->animations() )
                fireEvent( ES_ANIMATION, anim.getUID(), EA_STOP );   

            // Reset scene references to empty and release private animations
            setSceneAnimationReferences( getDefaultScene(), AnimationReferenceArray(), false );
            
            fireEvent( ES_SCENE, m_default_scene_uid, EA_CHANGED );
        }

        m_current_scene = scene_uid;
        fireEvent( ES_SCENE, scene_uid, EA_START );
    }

    if ( isRunning() )
        m_animation_task->queue( EngineAction::PlayScene, scene_uid, method );
}

// ----------------------------------------------------------------------------
//
void Venue::stageActors( UID scene_uid ) {
    if ( isRunning() )
        m_animation_task->queue( EngineAction::StageActors, scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::clearAllAnimations() {
    if ( isRunning() )
        m_animation_task->queue( EngineAction::ClearAllAnimations );
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
void Venue::scanFixtureChannels()
{
    m_whiteout_fixtures.clear();
    m_blackout_channels.clear();
    m_dimmer_channels.clear();
    m_home_channels.clear();
    m_whiteout_pulse_fixture_ids.clear();
    m_fixture_states.clear();

    for ( FixtureMap::value_type& fixture : m_fixtures ) {
        Fixture* pf = &fixture.second;

        // If fixture has some color property (may be single color), add to the tracked fixture array
        if ( m_track_fixtures && pf->canWhiteout() && pf->getNumPixels() > 0 )
            m_fixture_states.emplace( pf->getUID(), pf );

        // Fixtures that can pulse
        if ( pf->canWhiteout() && pf->getAllowWhiteout() && pf->canDimmerStrobe() )
            m_whiteout_pulse_fixture_ids.push_back( pf->getUID() );

        WhiteoutFixture fixtureInfo( pf->getUID(), pf->getType() );

        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

			channel_address real_address = 
                ((pf->getUniverseId()-1) * DMX_PACKET_SIZE) + pf->getChannelAddress( channel ) - 1;

            // Store channel home values
            if ( cp->getHomeValue() != 0 )
                m_home_channels.emplace_back( pf->getUID(), real_address, cp->getHomeValue(), cp );

            // Store whiteout affected channels
            if ( pf->canWhiteout() && pf->getAllowWhiteout() ) {
                if ( cp->canWhiteout() ) {
                    switch ( cp->getType() ) {
                        case CHNLT_PAN:
                        case CHNLT_TILT:
                            fixtureInfo.m_hasMovement = true;
                            break;

                        case CHNLT_RED:
                        case CHNLT_BLUE:
                        case CHNLT_GREEN:
                        case CHNLT_WHITE:
                        case CHNLT_AMBER:
                        case CHNLT_UV:
                            fixtureInfo.m_hasColor = true;

                        case CHNLT_AUTOPROG:
                        case CHNLT_COLOR_MACRO:
                        case CHNLT_COLOR_SPEED:
                        case CHNLT_PROG_SPEED:
                        case CHNLT_GOBO:
                            fixtureInfo.m_channels.emplace_back( pf->getUID(), real_address, cp->getDefaultValue(), cp );
                            break;
                    }
                }

                // track dimmer channels even if no color channels sive fixture claims it can whiteout
			    if ( cp->isDimmer() )
                    fixtureInfo.m_dimmers.emplace_back( pf->getUID(), real_address, cp->getDefaultValue(), cp );
            }

            // Store blackout channels
            if ( cp->canBlackout() )
                m_blackout_channels.emplace_back( pf->getUID(), real_address, 0, cp );

            // Store master dimmer channels
            if ( cp->isDimmer() && pf->getAllowMasterDimming() )
                m_dimmer_channels.emplace_back( pf->getUID(), real_address, 0, cp );
        }

        if ( pf->canWhiteout() && pf->getAllowWhiteout() ) {
            // Add moving and non-color fixtures to the end and everyone else to the start for smart mode
            if ( !fixtureInfo.m_hasMovement || !fixtureInfo.m_hasColor )
                m_whiteout_fixtures.insert( m_whiteout_fixtures.begin(), fixtureInfo );
            else
                m_whiteout_fixtures.push_back( fixtureInfo );
        }
    }
}

// ----------------------------------------------------------------------------
//
void Venue::setHomePositions( channel_value* dmx_packet )
{
    for ( SetChannel& set : m_home_channels )
        dmx_packet[ set.m_address ] = set.m_value;
}

// ----------------------------------------------------------------------------
//
void Venue::setWhiteout( WhiteoutMode whiteout ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( whiteout == WHITEOUT_STROBE_SLOW ) {
        m_whiteout_strobe.setPositive( getWhiteoutColor() );
        m_whiteout_strobe.start( GetCurrentTime(), studio.getWhiteoutStrobeSlow(), false );
    }
    else if ( whiteout == WHITEOUT_STROBE_FAST ) {
        m_whiteout_strobe.setPositive( getWhiteoutColor() );
        m_whiteout_strobe.start( GetCurrentTime(), studio.getWhiteoutStrobeFast(), false );
    }
    else if ( whiteout == WHITEOUT_STROBE_MANUAL || whiteout == WHITEOUT_PULSE ) {
        StrobeTime strobe_time = studio.getWhiteoutStrobeManual();
        strobe_time.setOffMS( m_whiteout_strobe_ms );
        studio.setWhiteoutStrobeManual( strobe_time );

        m_whiteout_strobe.setPositive( getWhiteoutColor() );
        m_whiteout_strobe.start( GetCurrentTime(), studio.getWhiteoutStrobeManual(), false );
    }

    m_whiteout = whiteout;
    m_whiteout_positive_index = 0;

    RGBWA::resolveColor( m_whiteout_color, m_whiteout_color_list );

    m_animation_task->updateWhiteout();

    fireEvent( ES_WHITEOUT, 0L, EA_CHANGED, NULL, whiteout );
}

// ----------------------------------------------------------------------------
//
void Venue::setWhiteoutStrobeMS( UINT strobe_ms, UINT fade_ms ) {
    m_whiteout_strobe_ms = strobe_ms;
    m_whiteout_fade_ms = std::min<UINT>( strobe_ms, fade_ms );

    if ( m_whiteout == WHITEOUT_STROBE_MANUAL || m_whiteout == WHITEOUT_PULSE )     // Need to reset strobe timer
        setWhiteout( m_whiteout );

    fireEvent( ES_WHITEOUT_STROBE, 0L, EA_CHANGED, NULL, m_whiteout_strobe_ms, m_whiteout_fade_ms );
}

// ----------------------------------------------------------------------------
//
void Venue::setWhiteoutColor( RGBWA& color ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_whiteout_color = color;

    if ( color.red() == 0xFF && color.blue() == 0xFF && color.green() == 0xFF )
        m_whiteout_color.white( 0xFF );                 // For fixtures with white channel

    m_whiteout_strobe.setPositive( m_whiteout_color );  // In case it is already running

    m_whiteout_positive_index = 0;
    m_whiteout_pulse_index = 0;

    RGBWA::resolveColor( m_whiteout_color, m_whiteout_color_list );

    m_animation_task->updateWhiteout();

    CString hex_color;
    hex_color.AppendFormat( "%06lX", (ULONG)color );

    fireEvent( ES_WHITEOUT_COLOR, 0L, EA_CHANGED, (LPCSTR)hex_color, (ULONG)color );
}

// ----------------------------------------------------------------------------
//
void Venue::setWhiteoutEffect( WhiteoutEffect effect ) {
    m_whiteout_effect = effect;

    m_animation_task->updateWhiteout();

    fireEvent( ES_WHITEOUT_EFFECT, 0L, EA_CHANGED, NULL, effect );
}

// ----------------------------------------------------------------------------
//
void Venue::advanceWhiteoutColor() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( m_whiteout_color_list.size() > 1 )
        m_whiteout_positive_index = (m_whiteout_positive_index+1) % m_whiteout_color_list.size();
}

// ----------------------------------------------------------------------------
//
void Venue::whiteoutChannels( channel_value* dmx_packet ) {
    // Full on - all fixture RGBW channels and dimmers on (fixture must have RGBW and dimmer channels)

    FUID fixture_id = 0;

    if ( m_whiteout == WHITEOUT_PULSE ) {
        if ( !m_whiteout_strobe.isOn() || m_whiteout_pulse_fixture_ids.size() == 0 )
            return;

        if ( m_whiteout_fade_ms == 0 )
            m_whiteout_pulse_index = ( rand() ) % m_whiteout_pulse_fixture_ids.size();
        else
            m_whiteout_pulse_index = ( m_whiteout_pulse_index+1 ) % m_whiteout_pulse_fixture_ids.size();

        fixture_id = m_whiteout_pulse_fixture_ids[m_whiteout_pulse_index];
    }

    RGBWA color;
    size_t index = m_whiteout_positive_index;

    if ( m_whiteout == WHITEOUT_ON && index < m_whiteout_color_list.size() )
        color = m_whiteout_color_list[ index ];
    else
        color = m_whiteout_strobe.rgbwa();

	bool switch_color_per_fixture = m_whiteout_effect != WHITEOUT_EFFECT_SINGLE_COLOR && 
									m_whiteout_color_list.size() > 1 && 
									m_whiteout == WHITEOUT_ON;

    for ( WhiteoutFixture& fixtureInfo : m_whiteout_fixtures ) {
        if ( m_whiteout == WHITEOUT_PULSE && fixtureInfo.m_uid != fixture_id )
            continue; 

        if ( switch_color_per_fixture ) {
            if ( fixture_id == 0 )
                fixture_id = fixtureInfo.m_uid;
            else {
                index = (index+1) % m_whiteout_color_list.size();
                color = m_whiteout_color_list[ index ];
                fixture_id = fixtureInfo.m_uid;
            }
        }

        // Color and other whiteout-able channels
        for ( SetChannel& set : fixtureInfo.m_channels ) {
            switch ( set.m_cp->getType() ) {
                case CHNLT_RED:
                    dmx_packet[ set.m_address ] = color.red();
                    break;

                case CHNLT_BLUE:
                    dmx_packet[ set.m_address ] = color.blue();
                    break;

                case CHNLT_GREEN:
                    dmx_packet[ set.m_address ] = color.green();
                    break;

                case CHNLT_WHITE:
                    dmx_packet[ set.m_address ] = color.white();
                    break;

                case CHNLT_AMBER:
                    dmx_packet[ set.m_address ] = color.amber();
                    break;

                case CHNLT_UV:
                    dmx_packet[ set.m_address ] = color.uv();
                    break;

                case CHNLT_AUTOPROG:
                case CHNLT_COLOR_MACRO:
                case CHNLT_COLOR_SPEED:
                case CHNLT_PROG_SPEED:
                case CHNLT_GOBO:
                    dmx_packet[ set.m_address ] = set.m_value;
                    break;
            }
        }

        // Dimmer channel(s)
        for ( SetChannel& set : fixtureInfo.m_dimmers ) {
            // If a fixture can't strobe (like Hue) then it is shut off when strobing
            channel_value value = (m_whiteout == WHITEOUT_ON || (m_whiteout_strobe.isOn() && set.m_cp->canDimmerStrobe()) ) ? 
                set.m_cp->getDimmerHighestIntensity() : set.m_cp->getDimmerLowestIntensity();

            dmx_packet[ set.m_address ] = value;
        }
    }
}

// ---------------------------------------------------------------------------- EXPERIMENTAL !!!!!!!!!!!!!!!!!!
// Experimental smart whiteout color distribution
void Venue::smartWhiteoutChannels( channel_value *dmx_packet ) {
    if ( m_whiteout_color != SYSTEM_PALETTE_4 ) {
        whiteoutChannels( dmx_packet );
        return;
    }

    Palette* palette = getPalette( getPaletteByNumber( SYSTEM_PALETTE_VIDEO_COLORS ) );
    if ( palette == NULL || palette->getType() != PT_COLOR_PALETTE )
        return;

    WeightedColorArray wcolors = palette->getWeightedColors();      // Colors can change in the middle of this crappy algorithm
   
    size_t fixture_index = 0;
    size_t color_index = 0;
    size_t population;

    // TODO assign colors based on fixture type - > population on static fixtures - less color on moving if possible
    // TODO assign colors based on position?

    while ( fixture_index < m_whiteout_fixtures.size() ) {
        if ( wcolors[color_index].getWeight() < .10 )
            population = 1;
        else
            population = (size_t)((double)m_whiteout_fixtures.size() * wcolors[color_index].getWeight() );

        if ( population == 0 )
            population = 1;

        RGBWA color = wcolors[color_index].getColor();

        while ( population != 0 && fixture_index < m_whiteout_fixtures.size() ) {
            WhiteoutFixture& fixtureInfo = m_whiteout_fixtures[fixture_index];

            for ( SetChannel& set : fixtureInfo.m_channels ) {
                switch ( set.m_cp->getType() ) {
                    case CHNLT_RED:
                        dmx_packet[ set.m_address ] = color.red();
                        break;

                    case CHNLT_BLUE:
                        dmx_packet[ set.m_address ] = color.blue();
                        break;

                    case CHNLT_GREEN:
                        dmx_packet[ set.m_address ] = color.green();
                        break;

                    case CHNLT_WHITE:
                        dmx_packet[ set.m_address ] = color.white();
                        break;

                    case CHNLT_AMBER:
                        dmx_packet[ set.m_address ] = color.amber();
                        break;

                    case CHNLT_UV:
                        dmx_packet[ set.m_address ] = color.uv();
                        break;

                    case CHNLT_AUTOPROG:
                    case CHNLT_COLOR_MACRO:
                    case CHNLT_COLOR_SPEED:
                    case CHNLT_PROG_SPEED:
                    case CHNLT_GOBO:
                        dmx_packet[ set.m_address ] = set.m_value;
                        break;
                }
            }

            // Dimmer channel(s)
            for ( SetChannel& set : fixtureInfo.m_dimmers )
                dmx_packet[ set.m_address ] = set.m_cp->getDimmerHighestIntensity();

            fixture_index++;
            population--;
        }

        color_index = (color_index+1) % wcolors.size();
    }
}
// ---------------------------------------------------------------------------- EXPERIMENTAL !!!!!!!!!!!!!!!!!!

// ----------------------------------------------------------------------------
//
void Venue::blackoutChannels( channel_value* dmx_packet ) {
    for ( SetChannel& set : m_blackout_channels )
        dmx_packet[ set.m_address ] = 0;        // Do not modify this channel during blackout
}

// ----------------------------------------------------------------------------
//
void Venue::dimmerChannels( channel_value* dmx_packet ) {
    for ( SetChannel& set : m_dimmer_channels ) {
		channel_value value = dmx_packet[ set.m_address ];
        if ( value == set.m_cp->getDimmerOffIntensity() )
            continue;

        if ( m_master_dimmer == 0 ) {
            value = set.m_cp->getDimmerOffIntensity();
        }
        else if ( set.m_cp->getDimmerLowestIntensity() < set.m_cp->getDimmerHighestIntensity() ) {
            if ( value >= set.m_cp->getDimmerLowestIntensity() && value <= set.m_cp->getDimmerHighestIntensity() ) {
                value -= set.m_cp->getDimmerLowestIntensity();
                value = (value * m_master_dimmer)/100;
                value += set.m_cp->getDimmerLowestIntensity();
            }
        }
        else if ( value >= set.m_cp->getDimmerHighestIntensity() && value <= set.m_cp->getDimmerLowestIntensity() ) {
            unsigned max = set.m_cp->getDimmerLowestIntensity()-set.m_cp->getDimmerHighestIntensity();
            value -= set.m_cp->getDimmerHighestIntensity();
            value = max - (((max-value) * m_master_dimmer)/100);
            value += set.m_cp->getDimmerHighestIntensity();
        }

        dmx_packet[ set.m_address ] = value;
    }
}

// ----------------------------------------------------------------------------
//
void Venue::loadSceneChannels( channel_value *dmx_multi_universe_packet, ActorPtrArray& actors, ChannelDataList* returned_channel_data ) {
	if ( returned_channel_data != NULL )
		returned_channel_data->clear();

    for ( SceneActor* actor : actors ) {
        for ( Fixture* pf : resolveActorFixtures( actor ) ) {
            for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
				channel_value value = actor->getFinalChannelValue( pf->getUID(), channel );
				channel_address real_channel = loadChannel( dmx_multi_universe_packet, pf, channel, value );

				if ( returned_channel_data != NULL )
					returned_channel_data->emplace_back( real_channel, value );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
void Venue::removeFixtureFromPalette( UID fixture_id ) {
    bool changed = false;

    for ( PaletteMap::iterator it=m_palettes.begin(); it != m_palettes.end(); it++ )
        changed |= it->second.removeFixture( fixture_id );

    if ( changed )
        fireEvent( ES_PALETTE, 0, EA_CHANGED );
}

// ----------------------------------------------------------------------------
// Single point to load a DMX channel
//
channel_address Venue::loadChannel( channel_value* dmx_multi_universe_packet, Fixture* pf, channel_address channel, channel_value value ) {
	channel_address real_address = pf->getMultiPacketAddress( channel );

    dmx_multi_universe_packet[ real_address ] = value;

	return real_address;
}

// ----------------------------------------------------------------------------
//
SceneActor* Venue::captureFixture( UID fixture_uid, std::vector<channel_value>* channel_values, UIDArray* palette_refs ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    SceneActor * actor = NULL;

    if ( fixture_uid > 0 ) {
        actor = getDefaultActor( fixture_uid );

        bool changed = false;

        if ( !actor ) {
            FixtureGroup* group = getFixtureGroup( fixture_uid );
            if ( group != NULL ) {
                getDefaultScene()->addActor( SceneActor( this, group ) );
            }
            else {
                Fixture* fixture = getFixture( fixture_uid );
                if ( fixture == NULL )
                    STUDIO_ASSERT( fixture, "Invalid fixture/group UID %ld", fixture_uid );

                getDefaultScene()->addActor( SceneActor( fixture ) );
            }

            addActorToDefaultAnimations( fixture_uid );
            actor = getDefaultScene()->getActor( fixture_uid );
            changed = true;
        }

        if ( channel_values != NULL && channel_values->size() > 0 ) {
            for ( channel_address channel=0; channel < channel_values->size() && channel < actor->getNumChannels(); channel++ )
                actor->setBaseChannelValue( channel, channel_values->at( channel ) );
            changed = true;
        }

        if ( palette_refs != NULL ) {
            actor->setPaletteReferences( *palette_refs );
            changed = true;        
        }

        if ( changed ) {
            computeActorFinalValues( getDefaultScene() );
            sceneUpdated( m_default_scene_uid );
        }
    }

    m_captured_actor = fixture_uid;

    return actor;
}

// ----------------------------------------------------------------------------
//
void Venue::addActorToDefaultAnimations( UID fixture_id ) {
    for ( AnimationReference& ref : getDefaultScene()->animations() ) {
        ref.addActor( fixture_id );
    }
}

// ----------------------------------------------------------------------------
//
void Venue::captureAndSetChannelValue( SetChannelArray channel_sets, DWORD change_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( SetChannel set : channel_sets ) {
        SceneActor* actor = captureFixture( set.m_fixture_uid );
        STUDIO_ASSERT( actor, "Unable to find actor UID %ld", set.m_fixture_uid );

        actor->setBaseChannelValue( set.m_address, set.m_value );

        computeActorFinalValues( actor );

		DMXStudio::fireEvent( ES_CHANNEL, actor->getActorUID(), EA_CHANGED, NULL, set.m_address, set.m_value, change_id );

		if ( !isRunning() )
			continue;

		if ( m_current_scene == m_default_scene_uid ) {
			for ( Fixture* pf : resolveActorFixtures( actor ) ) {
				channel_value final_value = actor->getFinalChannelValue( pf->getUID(), set.m_address );
				channel_address real_address = pf->getMultiPacketAddress( set.m_address );

				m_animation_task->queue( UpdateChannelValue, NOUID, real_address << 8 | final_value );
			}
		}
		else
			m_animation_task->updateChannels();
    }
}

// ----------------------------------------------------------------------------
//
void Venue::captureAndSetPalettes( UID actor_uid, UIDArray& palette_refs ) {
    SceneActor* actor = captureFixture( actor_uid );
    STUDIO_ASSERT( actor, "Unable to find actor UID %ld", actor_uid );

    actor->setPaletteReferences( palette_refs );

    computeActorFinalValues( actor );
    sceneUpdated( m_default_scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::clearAllCapturedActors( ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

	m_animation_task->queue( EngineAction::ClearAllTransientAnimations );

	for ( AnimationReference& a : getDefaultScene()->animations() )
		fireEvent( ES_ANIMATION, a.getUID(), EA_STOP );

	getDefaultScene()->removeAllActors();

    // Reset scene references to empty and release private animations
    setSceneAnimationReferences( getDefaultScene(), AnimationReferenceArray(), false );

    m_captured_actor = 0;

    sceneUpdated( m_default_scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::releaseActor( UID actor_id ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    getDefaultScene()->removeActor( actor_id );
    if ( m_captured_actor == actor_id )
        m_captured_actor = 0;

	lock.Unlock();

    sceneUpdated( m_default_scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::applyDefaultActors( channel_value* dmx_packet, ChannelDataList* returned_channel_data ) {
	CSingleLock lock( &m_venue_mutex, TRUE );
	
	loadSceneChannels( dmx_packet, getDefaultScene()->getActors(), returned_channel_data );
}

// ----------------------------------------------------------------------------
//
void Venue::writePacket( const channel_value* dmx_multi_universe_packet, bool apply_default_scene ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    memcpy( m_multi_universe_packet, dmx_multi_universe_packet, MULTI_UNIV_PACKET_SIZE );

    // Overwrite any manually controlled fixtures
    if ( apply_default_scene && m_current_scene != m_default_scene_uid )
		applyDefaultActors( m_multi_universe_packet );

    if ( isMuteBlackout() || isForceBlackout() )                // Handle blackout of all venue fixtures
        blackoutChannels( m_multi_universe_packet );
    else if ( getWhiteout() == WHITEOUT_ON && getWhiteoutEffect() == WhiteoutEffect::WHITEOUT_EFFECT_SMART_COLOR )
        smartWhiteoutChannels( m_multi_universe_packet );
    else if ( getWhiteout() != WHITEOUT_OFF )                   // Handle whiteout effect (all venue fixtures)              
        whiteoutChannels( m_multi_universe_packet );

    if ( m_master_dimmer != 100 )                               // Handle master dimmer
        dimmerChannels(m_multi_universe_packet);

    for ( UniverseMap::iterator it=m_universes.begin(); it != m_universes.end(); ++it ) {
        Universe* universe = (*it).second;
        universe->write_all( &m_multi_universe_packet[DMX_PACKET_SIZE * (universe->getId()-1)] );
    }
    
    m_packet_changed = true;
}

// ----------------------------------------------------------------------------
//
void Venue::readPacket( channel_value* dmx_multi_universe_packet ) {
    memset( dmx_multi_universe_packet, 0, MULTI_UNIV_PACKET_SIZE );

    for ( UniverseMap::iterator it=m_universes.begin(); it != m_universes.end(); ++it ) {
        Universe* universe = (*it).second;
        universe->read_all( &dmx_multi_universe_packet[DMX_PACKET_SIZE * (universe->getId()-1)] );
    }
}

// ----------------------------------------------------------------------------
//
channel_value Venue::getBaseChannelValue( Fixture* pfixture, channel_address channel ) {
    STUDIO_ASSERT( channel < pfixture->getNumChannels(), 
        "Channel %d out of range for fixture %ld", channel, pfixture->getUID() );

    SceneActor* actor = getDefaultActor( pfixture->getUID() );
    if ( actor )
        return actor->getBaseChannelValue( channel );

    if ( !isDefaultScene() ) {
        actor =  getScene()->getActor( pfixture->getUID() );
        if ( actor )
            return actor->getBaseChannelValue( channel );
    }

    return pfixture->getChannel( channel )->getDefaultValue();
}

// ----------------------------------------------------------------------------
//
channel_value Venue::getBaseChannelValue( FixtureGroup* group, channel_address channel ) {
    SceneActor* actor = getDefaultActor( group->getUID() );
    if ( actor )
        return actor->getBaseChannelValue( channel );

    if ( !isDefaultScene() ) {
        actor =  getScene()->getActor( group->getUID() );
        if ( actor )
            return actor->getBaseChannelValue( channel );
    }

    return ( channel < group->getNumChannelValues() ) ? group->getChannelValue( channel ) : 0;
}

// ----------------------------------------------------------------------------
//
void Venue::fadeToNextScene( UID scene_uid, ULONG fade_time ) {
    if ( isRunning() )
        m_animation_task->queue( EngineAction::FadeToScene, scene_uid, fade_time );
}

// ----------------------------------------------------------------------------
//
channel_value Venue::getBaseChannelValue( SceneActor& target_actor, channel_address channel ) {
    SceneActor* actor = getDefaultActor( target_actor.getActorUID() );
    if ( actor )
        return actor->getBaseChannelValue( channel );

    if ( !isDefaultScene() ) {
        actor = getScene()->getActor( target_actor.getActorUID() );
        if ( actor )
            return actor->getBaseChannelValue( channel );
    }

    if ( channel < target_actor.getNumChannels() )
        return target_actor.getBaseChannelValue( channel );

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
void Venue::mapMusicToScene( LPCSTR track_link, MusicSelectorType& type, UID& type_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    type = MST_SCENE;
    type_uid = getDefaultScene()->getUID();

    MusicSceneSelectMap::iterator it = m_music_scene_select_map.find( track_link );
    if ( it == m_music_scene_select_map.end() )
        it = m_music_scene_select_map.find( UNMAPPED_TRACK_LINK );
    if ( it != m_music_scene_select_map.end() ) {
        type_uid = it->second.m_selection_uid;
        type = it->second.m_selection_type;
    }
}

// ----------------------------------------------------------------------------
//    
void Venue::addMusicMapping( MusicSceneSelector& music_scene_selector ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_music_scene_select_map[ music_scene_selector.m_track_link ] = music_scene_selector;

    fireEvent( ES_MUSIC_MATCH, 0L, EA_CHANGED );
}

// ----------------------------------------------------------------------------
//
void Venue::deleteMusicMapping( LPCSTR track_full_name ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    MusicSceneSelectMap::iterator it_del = m_music_scene_select_map.find( track_full_name );
    if ( it_del != m_music_scene_select_map.end() ) {
        m_music_scene_select_map.erase( it_del );

        fireEvent( ES_MUSIC_MATCH, 0L, EA_CHANGED );
    }
}

// ----------------------------------------------------------------------------
//
 bool Venue::findMusicMapping( LPCSTR track_full_name, MusicSceneSelector& result ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    MusicSceneSelectMap::iterator it = m_music_scene_select_map.find( track_full_name );
    if ( it == m_music_scene_select_map.end() )
        return false;

    result = it->second;

    return true;
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

    fireEvent( ES_MUSIC_MATCH, 0L, EA_CHANGED );
}

// ----------------------------------------------------------------------------
//
void Venue::clearMusicMappings() 
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_music_scene_select_map.clear();

    fireEvent( ES_MUSIC_MATCH, 0L, EA_CHANGED );
}

// ----------------------------------------------------------------------------
//    
void Venue::addMusicMappings( std::vector<MusicSceneSelector>& selectors ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( std::vector<MusicSceneSelector>::iterator it=selectors.begin(); it != selectors.end(); ++it )
        m_music_scene_select_map[ (*it).m_track_link] = (*it);

    fireEvent( ES_MUSIC_MATCH, 0L, EA_CHANGED );
}

// ----------------------------------------------------------------------------
//    
FixturePtrArray Venue::resolveActorFixtures( SceneActor* actor )
{
    FixturePtrArray fixtures;

    if ( actor->isGroup() ) {
        FixtureGroup* group = getFixtureGroup( actor->getActorUID() );
        STUDIO_ASSERT( group, "Group %lu in actor not found", actor->getActorUID() );

        for ( UID fixture_uid : group->getFixtures() )
            fixtures.push_back( getFixture( fixture_uid ) );
    }
    else {
        Fixture* pf = getFixture( actor->getActorUID() );
        STUDIO_ASSERT( pf, "Fixture %lu in actor not found", actor->getActorUID() );
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
    STUDIO_ASSERT( group, "Invalid group id %lu", group_uid );

    Fixture* fixture = NULL;

    for ( UID fixture_uid : group->getFixtures() ) {
        Fixture* f = getFixture( fixture_uid );
        STUDIO_ASSERT( f != NULL, "Missing fixture for %lu", fixture_uid );

        if ( fixture == NULL || f->getNumChannels() > fixture->getNumChannels() )
            fixture = f;
    }

    return fixture;
}

// ----------------------------------------------------------------------------
//
UID Venue::getRandomChase() {
    UINT count = (rand() % getNumChases());
    for ( ChaseMap::iterator it=m_chases.begin(); it != m_chases.end(); ++it )
        if ( !count-- )
            return it->first;
    return 0;
}

// ----------------------------------------------------------------------------
//
UID Venue::getRandomScene() {
    UINT count = (rand() % (getNumScenes()-1))+1;
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it )
        if ( !count-- )
            return it->first;
    return 0;
}

// ----------------------------------------------------------------------------
//
void Venue::populateSceneRatingsMap( SceneRatingsMap& ratings_map ) {
    for ( SceneMap::iterator it=m_scenes.begin(); it != m_scenes.end(); ++it ) {
        BPMRating rating = it->second.getBPMRating();
        ratings_map[rating].push_back( it->first );
    }
}

// ----------------------------------------------------------------------------
//
void Venue::clearAllUniverses() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( UniverseMap::iterator it=m_universes.begin(); it != m_universes.end(); ++it ) {
        (*it).second->stop();
        delete (*it).second;
    }

    m_universes.clear();
}

// ----------------------------------------------------------------------------
//
void Venue::addUniverse( Universe* universe ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    STUDIO_ASSERT( universe->getId() > 0, "Attempt to add invalid universe ID %d", universe->getId() );
    STUDIO_ASSERT( m_universes.find(universe->getId()) == m_universes.end(), "Universe %d already defined", universe->getId() );

    m_universes[universe->getId()] = universe;
}

// ----------------------------------------------------------------------------
//
Universe* Venue::getUniverse( size_t universe_num ) {
    UniverseMap::iterator it = m_universes.find(universe_num);
    STUDIO_ASSERT( it != m_universes.end(), "Invalid universe number %d", universe_num );
    return (*it).second;
}

// ----------------------------------------------------------------------------
//
UniversePtrArray Venue::getUniverses() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    UniversePtrArray list;
    UniverseMap::iterator it;

    for ( it=m_universes.begin(); it != m_universes.end(); ++it )
        list.push_back( it->second );

    return list;
}

// ----------------------------------------------------------------------------
//
void Venue::setForceBlackout( bool hard_blackout ) {
    m_hard_blackout = hard_blackout;

    if ( isRunning() )
        m_animation_task->updateChannels();

    fireEvent( ES_BLACKOUT, 0L, hard_blackout ? EA_START : EA_STOP );
}

// ----------------------------------------------------------------------------
//
void Venue::configure( LPCSTR name, LPCSTR description, LPCSTR audio_capture_device, float audio_boost, 
                       float audio_boost_floor, int audio_sample_size, int auto_blackout, bool track_fixtures,
                       std::vector<Universe>& universes )
{
    close();

    setName( name );
    setDescription( description );
    setAudioCaptureDevice( audio_capture_device );
    setAudioBoost( audio_boost );
    setAudioBoostFloor( audio_boost_floor );
    setAudioSampleSize( audio_sample_size );
    setAutoBlackoutMS( auto_blackout );
    setTrackFixtures( track_fixtures );

    clearAllUniverses();

    for ( Universe& univ : universes )
        addUniverse( new Universe( univ ) );

    fireEvent( ES_VENUE, 0L, EA_CHANGED );

    open();
}

// Animation methods

// ----------------------------------------------------------------------------
//
AnimationDefinition* Venue::getAnimation( UID animation_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    AnimationMap::iterator it = m_animations.find( animation_uid );
    if ( it == m_animations.end() )
        return NULL;

    return it->second.get();
}

// ----------------------------------------------------------------------------
//
UID Venue::getAnimationByNumber( AnimationNumber animation_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( AnimationMap::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
        if ( it->second->getNumber() == animation_number )
            return it->first;

    return NOUID;
}

// ----------------------------------------------------------------------------
//
AnimationPtrArray Venue::getAnimations() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    AnimationPtrArray list;

    for ( AnimationMap::iterator it=m_animations.begin(); it != m_animations.end(); ++it )
        list.push_back( it->second.get() );

    return list;
}

// ----------------------------------------------------------------------------
//
AnimationNumber Venue::nextAvailableAnimationNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    AnimationNumber defaultNumber = 1L;
    UINT64 max_timestamp = 0L;

    FindNextAvailable<AnimationNumber> finder;
    for ( auto it=m_animations.begin(); it != m_animations.end(); it++ ) {
        finder.insert( it->second->getNumber() );

        if ( it->second->getCreated() > max_timestamp ) {
            defaultNumber = it->second->getNumber()+1;
            max_timestamp = it->second->getCreated();
        }
    }

    return (AnimationNumber)finder.nextAvailable( defaultNumber );
}

// ----------------------------------------------------------------------------
//
bool Venue::deleteAnimation( UID animation_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    AnimationMap::iterator it = m_animations.find( animation_uid );
    if ( it == m_animations.end() )
        return false;

    // Remove animations from all scenes
    for ( SceneMap::value_type& pair : m_scenes ) {
        bool removed = pair.second.removeAnimationByUID( animation_uid );
        if ( removed )
            sceneUpdated( pair.first );
    }

    m_animations.erase( it );

    fireEvent( ES_ANIMATION, animation_uid, EA_DELETED );

    return true;
}

// ----------------------------------------------------------------------------
//
void Venue::deleteAllAnimations() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( SceneMap::value_type& pair : m_scenes ) {
        bool removed = pair.second.clearAnimations();
        if ( removed )
            fireEvent( ES_SCENE, pair.first, EA_CHANGED );
    }

    m_animations.clear();

    fireEvent( ES_ANIMATION, 0L, EA_DELETED );
}

// ----------------------------------------------------------------------------
//
UID Venue::addAnimation( AnimationDefinition* animation ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( animation->getUID() == 0L )
        animation->setUID( allocUID() );
    if ( !animation-> isShared() )
        animation->setAnimationNumber( 0 );

    m_animations[ animation->getUID() ] = std::unique_ptr<AnimationDefinition>( animation );

    fireEvent( ES_ANIMATION, animation->getUID(), EA_NEW );

    return animation->getUID();
}

// ----------------------------------------------------------------------------
//
void Venue::replaceAnimation( AnimationDefinition* animation ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    AnimationMap::iterator it = m_animations.find( animation->getUID() );
    if ( it == m_animations.end() ) {
        delete animation;
        return; // Unexpected ...
    }

    if ( !animation-> isShared() )
        animation->setAnimationNumber( 0 );

    it->second.reset( animation );

    fireEvent( ES_ANIMATION, animation->getUID(), EA_CHANGED );

    if ( isRunning() )
        m_animation_task->queue( EngineAction::UpdateAnimation, animation->getUID() );
}

// ----------------------------------------------------------------------------
//
void Venue::testAnimation( AnimationDefinition* animation ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( !animation ) {
        if ( isRunning() )
            m_animation_task->queue( EngineAction::RemoveTransientAnimation, m_test_animation_uid, NOUID );

        return;
    }

    animation->setUID( m_test_animation_uid );

    if ( isRunning() ) {
        m_venue_mutex.Unlock();     // Make sure we don't have the venue lock
        m_animation_task->playTransientAnimation( animation, NOUID );
    }

    delete animation;
}

// ----------------------------------------------------------------------------
//
void Venue::releaseAnimation( UID animation_uid ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

	AnimationDefinition* animation = getAnimation( animation_uid );
	if ( animation == NULL )
		return;

	Scene* scene = getDefaultScene();

	if ( scene->getAnimationByUID( animation_uid ) != NULL ) {
		m_animation_task->queue( EngineAction::RemoveTransientAnimation, animation_uid, m_default_scene_uid );

		fireEvent( ES_ANIMATION, animation_uid, EA_STOP );

		scene->removeAnimationByUID( animation_uid );

		lock.Unlock();

		sceneUpdated( scene->getUID() );
	}

    if ( !animation->isShared() )
        deleteAnimation( animation_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::captureAnimation( UID animation_uid ) {
	CSingleLock lock( &m_venue_mutex, TRUE );

	Scene* scene = getDefaultScene();

	if ( scene->getAnimationByUID( animation_uid ) == NULL ) {
		scene->addAnimation( AnimationReference( animation_uid, scene->getActorUIDs() ) );

		m_animation_task->queue( PlayTransientAnimation, animation_uid, m_default_scene_uid );

		lock.Unlock();

		sceneUpdated( m_default_scene_uid );

		fireEvent( ES_ANIMATION, animation_uid, EA_START );
	}
}

// ----------------------------------------------------------------------------
//
void Venue::setAnimationSpeed( UINT speed ) {
    m_animation_speed = speed;

    fireEvent( ES_ANIMATION_SPEED, 0L, EA_CHANGED, NULL, speed );
}

// ----------------------------------------------------------------------------
//
void Venue::computeActorFinalValues( Scene* scene ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( SceneActor* actor : scene->getActors() )
        computeActorFinalValues( actor );
}

// ----------------------------------------------------------------------------
//
void Venue::computeActorFinalValues( SceneActor* actor ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( Fixture* fixture : resolveActorFixtures( actor ) ) {
        ChannelValues channel_values = actor->getBaseChannelValues();

        for ( UID palette_id : actor->getPaletteReferences() ) {
            PaletteMap::iterator it = m_palettes.find( palette_id );
            if ( it == m_palettes.end() ) 
                continue;

            Palette& palette = it->second;

            PaletteEntry* entry = palette.getEntry( fixture );
            if ( entry == NULL )
                continue;

            PaletteValues& values = entry->getValues();

            if ( entry->getAddressing() == EntryAddressing::BY_CHANNEL ) {
                for ( PaletteValues::value_type value : values ) {
                    if ( value.first < fixture->getNumChannels() )
                        channel_values.set( value.first, value.second );
                }
            }
            else {
                for ( size_t ch=0; ch < fixture->getNumChannels(); ch++ ) {
                    Channel* channel = fixture->getChannel( ch );
                    PaletteValues::iterator it = values.find( channel->getType() );
                    if ( it != values.end() )
                        channel_values.setWithGrow( ch, it->second );
                }
            }
        }

        actor->setFinalChannelValues( fixture->getUID(), channel_values );
    }
}

// ----------------------------------------------------------------------------
//
Palette* Venue::getPalette( UID palette_id )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    PaletteMap::iterator it = m_palettes.find( palette_id );
    if ( it == m_palettes.end() )
        return NULL;

    return &it->second;
}

// ----------------------------------------------------------------------------
//
Palette* Venue::getPalette( LPCSTR palette_name )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( PaletteMap::value_type& pair : m_palettes )
        if ( _strcmpi( pair.second.getName(), palette_name ) == 0 )
            return &pair.second;

    return NULL;
}

// ----------------------------------------------------------------------------
//
UID Venue::getPaletteByNumber( PaletteNumber palette_number ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    for ( PaletteMap::iterator it=m_palettes.begin(); it != m_palettes.end(); ++it )
        if ( it->second.getPaletteNumber() == palette_number )
            return it->first;

    return NOUID;
}

// ----------------------------------------------------------------------------
//
void Venue::updatePalette( Palette& palette )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    m_palettes[ palette.getUID() ] = palette;

    UIDArray updated_scenes;

    for ( SceneMap::value_type& pair : m_scenes ) {
        for ( SceneActor* actor : pair.second.getActors() ) {
            if ( actor->hasPaletteReference( palette.getUID() ) ) {
                computeActorFinalValues( actor );
                updated_scenes.push_back( pair.first );
            }
        }
    }

    fireEvent( ES_PALETTE, palette.getUID(), EA_CHANGED );

    for ( UID uid : updated_scenes ) {
        if ( uid == m_current_scene )
            playScene( m_current_scene, SLM_LOAD );
        else if ( uid == m_default_scene_uid )    // Reload channel values to pick up and default act0r changes
            m_animation_task->updateChannels();
    }
}

// ----------------------------------------------------------------------------
//
bool Venue::deletePalette( UID palette_id )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    // Remove palette entry
    PaletteMap::iterator it = m_palettes.find( palette_id );
    if ( it == m_palettes.end() ) 
        return false;
        
    m_palettes.erase( it );

    // Remove reference from all actors
    for ( SceneMap::value_type& pair : m_scenes ) {
        bool updated = false;

        for ( SceneActor* actor : pair.second.getActors() ) {
            if ( actor->removePaletteReference( palette_id ) ) {
                computeActorFinalValues( actor );
                updated = true;
            }
        }

        if ( updated ) {
            sceneUpdated( pair.first );
        }
    }

    // Update animation definitions but leave any running animations alone
    for ( AnimationMap::value_type& pair : m_animations ) {
        if ( pair.second->removePaletteReference( palette_id ) )
            fireEvent( ES_ANIMATION, pair.second->getUID(), EA_CHANGED );
    }

    fireEvent( ES_PALETTE, palette_id, EA_DELETED );

    return true;
}

// ----------------------------------------------------------------------------
//
UID Venue::addPalette( Palette& palette )
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( palette.getUID() == 0L )
        palette.setUID( allocUID() );

    m_palettes[ palette.getUID() ] = palette;

    fireEvent( ES_PALETTE, palette.getUID(), EA_NEW );

    return palette.getUID();
}

// ----------------------------------------------------------------------------
//
DObjectArray Venue::getPaletteSummary()
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    DObjectArray list;

    for ( PaletteMap::iterator it=m_palettes.begin(); it != m_palettes.end(); ++it )
        list.push_back( static_cast<DObject>( it->second ) );

    return list;
}

// ----------------------------------------------------------------------------
//
PaletteNumber Venue::nextAvailablePaletteNumber( void ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    return nextAvailable<PaletteMap, PaletteNumber>( m_palettes );
}

// ----------------------------------------------------------------------------
//
void Venue::setVideoPalette( RGBWAArray& palette_colors, ColorWeights& palette_weights ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Palette* palette = getPalette( getPaletteByNumber( SYSTEM_PALETTE_VIDEO_COLORS ) );
    if ( palette == NULL ) {
        addPalette( Palette( 0L, SYSTEM_PALETTE_VIDEO_COLORS, "System Video Palette", palette_colors, palette_weights ) );
        return;
    }

    palette->setType( PT_COLOR_PALETTE );
    palette->setPaletteColors( palette_colors );
    palette->setPaletteWeights( palette_weights );

    updatePalette( *palette );
}

// ----------------------------------------------------------------------------
//
bool Venue::getSystemPalette( ULONG palette_number, RGBWAArray& palette_colors ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    Palette* palette = getPalette( getPaletteByNumber( palette_number ) );
    if ( palette == NULL || palette->getType() != PT_COLOR_PALETTE )
        return false;

    palette_colors = palette->getPaletteColors();
    return true;
}

// ----------------------------------------------------------------------------
//
AnimationLevelMap Venue::loadAnimationLevelData( DWORD after_ms ) {
    if ( m_animation_task != NULL )
        return m_animation_task->loadAnimationLevelData( after_ms );

    return AnimationLevelMap();
}

// ----------------------------------------------------------------------------
//
void Venue::setupGlobalPaletteEntries() {
    CSingleLock lock( &m_venue_mutex, TRUE );

    if ( getPaletteByNumber( SYSTEM_PALETTE_GLOBAL_COLORS ) == NOUID ) {
        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_GLOBAL_COLORS, "System Global Color Chips",
            RGBWA::toRGBWAArray( GLOBAL_PALETTE_DEFAULT_COLORS, 16 ) );
    }

    if ( getPaletteByNumber( SYSTEM_PALETTE_RAINBOW_COLORS ) == NOUID ) {
        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_RAINBOW_COLORS, "System Rainbow Sequence", 
            RGBWA::toRGBWAArray( RAINBOW_PALETTE_DEFAULT_COLORS, 252 ) );
    }

    if ( getPaletteByNumber( SYSTEM_PALETTE_PREDEFINED_COLORS ) == NOUID ) {
        RGBWAArray colors;
        RGBWA::getAllPredefinedColors( colors );

        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_PREDEFINED_COLORS, "System Predefined Colors", colors );
    }

    if ( getPaletteByNumber( SYSTEM_PALETTE_USER_1 ) == NOUID ) {
        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_USER_1, "User System Palette 1", RGBWAArray() );
    }

    if ( getPaletteByNumber( SYSTEM_PALETTE_USER_2 ) == NOUID ) {
        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_USER_2, "User System Palette 2", RGBWAArray() );
    }

    if ( getPaletteByNumber( SYSTEM_PALETTE_USER_3 ) == NOUID ) {
        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_USER_3, "User System Palette 3", RGBWAArray() );
    }

    if ( getPaletteByNumber( SYSTEM_PALETTE_USER_4 ) == NOUID ) {
        UID uid = allocUID();
        m_palettes[uid] = Palette( uid, SYSTEM_PALETTE_USER_4, "User System Palette 4", RGBWAArray() );
    }
}

// ----------------------------------------------------------------------------
//
void Venue::stopQuickScene() 
{
    CSingleLock lock( &m_venue_mutex, TRUE );

    EngineAction action = m_default_scene_uid == m_current_scene ? RemoveAnimation : RemoveTransientAnimation;

    for ( UINT animation_number=1; animation_number <= 3; animation_number++ ) {
        UID animation_uid = getAnimationByNumber( animation_number );
        if ( animation_uid != NOUID ) {
            m_animation_task->queue( action, animation_uid, m_default_scene_uid );

            fireEvent( ES_ANIMATION, animation_uid, EA_STOP );
        }
    }

    // Reset scene references to empty and discard private animations
    setSceneAnimationReferences( getDefaultScene(), AnimationReferenceArray(), false );

    lock.Unlock();

    sceneUpdated( m_default_scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::createQuickScene( UIDArray fixtures, QuickSceneEffect effect, RGBWA color, unsigned color_speed_ms, 
                              QuickSceneMovement motion, unsigned move_speed_ms, bool multi_color ) 
{
    CSingleLock lock( &m_venue_mutex, TRUE );

	multi_color = multi_color && color.isPalette();

	m_animation_task->queue( EngineAction::ClearAllTransientAnimations );

	for ( AnimationReference& a : getDefaultScene()->animations() )  {
        m_animation_task->queue( EngineAction::RemoveAnimation, a.getUID(), m_default_scene_uid );         
        fireEvent( ES_ANIMATION, a.getUID(), EA_STOP );
    }

    // Reset scene references to empty and discard private animations
    setSceneAnimationReferences( getDefaultScene(), AnimationReferenceArray(), false );

    AnimationDefinition* color_animation = NULL;
    AnimationDefinition* movement_animation = NULL;
	AnimationDefinition* dimmer_animation = NULL;

	// ADD COLOR FADER (COLOR CAN BE A PALETTE)

	switch ( effect ) {
		case QSE_COLOR_STROBE: {
            UINT on_ms = studio.getWhiteoutStrobeFast().getOnMS();
            color_animation = new ScenePulse( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms ), color, 100, 50, false, 
                PULSE_EFFECT_STROBE );
			break;
        }

		case QSE_COLOR_PULSE:
			color_animation = new ScenePulse( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms ), color, color_speed_ms, 1, true, 
											  PULSE_EFFECT_STROBE );
			break;

		case QSE_COLOR_FADER: {
			RGBWAArray custom_colors;
			custom_colors.push_back( color );

			if ( !color.isPalette() ) {
				custom_colors.push_back( RGBWA::BLACK );	
			}

			color_animation = new SceneColorFader( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms ), 
				StrobeTime(color_speed_ms, 100, 0, 0, 1), RGBWA::BLACK, custom_colors, 
				multi_color ? FaderEffect::FADER_EFFECT_BLEND_MULTI : FaderEffect::FADER_EFFECT_BLEND );
			break;
		}

        case QSE_COLOR_BEAT: {
            RGBWAArray custom_colors;
            custom_colors.push_back( color );

            AnimationSignal signal( TRIGGER_AMPLITUDE_BEAT, SOURCE_HIGH, 0 );
            signal.setSensitivity( 10 );
            signal.setOffMS( 75 );

            color_animation = new SceneColorFader( NOUID, true, NOUID, signal, 
                StrobeTime(color_speed_ms, 100, 0, 0, 1), RGBWA::BLACK, custom_colors, 
                multi_color ? FaderEffect::FADER_EFFECT_CHANGE_MULTI : FaderEffect::FADER_EFFECT_CHANGE );
            break;
        }

        case QSE_COLOR_CHASE:
		case QSE_COLOR_DIMMER:
		case QSE_COLOR_BREATH:
		case QSE_COLOR: {
			RGBWAArray custom_colors;
			custom_colors.push_back( color );

			color_animation = new SceneColorFader( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms ), 
				StrobeTime(color_speed_ms, 100, 0, 0, 1), RGBWA::BLACK, custom_colors, 
                multi_color && color.isPalette() ? FaderEffect::FADER_EFFECT_CHANGE_MULTI :FaderEffect::FADER_EFFECT_CHANGE );

			if ( effect == QSE_COLOR_BREATH ) {
				dimmer_animation = new SceneFixtureDimmer( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms/10 ), 
					DM_BREATH, 20, 100 );
			}
			else if ( effect == QSE_COLOR_DIMMER ) {
				dimmer_animation = new SceneFixtureDimmer( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms ), 
					DM_RANDOM, 10, 100 );
			}
            else if ( effect == QSE_COLOR_CHASE ) {
                dimmer_animation = new ScenePatternDimmer( NOUID, true, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, color_speed_ms ), 
                    DP_SEQUENCE );
            }

			break;
		}

		case QSE_NONE:
		default:
			break;
	}

    if ( motion != QSM_NONE ) {
        MovementAnimation movement;
        DWORD timer = move_speed_ms;

        switch ( motion ) {
            case QSM_NOD:
			case QSM_NOD_STAGGERED:
                movement.m_movement_type = MOVEMENT_NOD;
                movement.m_alternate_groups = motion == QSM_NOD_STAGGERED;
                movement.m_group_size = (motion == QSM_NOD_STAGGERED) ? 1 : 1000;
                movement.m_tilt_start = 45;
                movement.m_tilt_end = 360;
                movement.m_backout_home_return = false;
                movement.m_pan_increment = 0;
                movement.m_pan_start = 0;
                movement.m_pan_end = 0;
                break;

            case QSM_SHORT_NOD:
            case QSM_SHORT_NOD_STAGGERED:
                movement.m_movement_type = MOVEMENT_NOD;
                movement.m_tilt_start = 90;
                movement.m_tilt_end = 165;
                movement.m_backout_home_return = false;
                movement.m_group_size = (motion == QSM_NOD) ? 1000 : 1;
                movement.m_pan_increment = 0;
                movement.m_pan_start = 0;
                movement.m_pan_end = 0;
                break;

            case QSM_RANDOM:
                movement.m_movement_type = MOVEMENT_RANDOM;
                movement.m_positions = 25;
                movement.m_backout_home_return = false;
                movement.m_tilt_start = 0;
                movement.m_tilt_end = 270;
                movement.m_pan_start = 0;
                movement.m_pan_end = 360;
                break;

            case QSM_WAVE:
			case QSM_WAVE_STAGGERED:
                movement.m_movement_type = MOVEMENT_SINEWAVE;
                movement.m_tilt_start = 45;
                movement.m_tilt_end = 135;
                movement.m_positions = 9;
				movement.m_pan_increment = (motion == QSM_WAVE) ? 0 : 45;
                movement.m_backout_home_return = false;
                timer = 350;
                break;

			case QSM_ROTATE:
			case QSM_ROTATE_STAGGERED:
                movement.m_movement_type = MOVEMENT_ROTATE;
                movement.m_tilt_start = 100;
                movement.m_tilt_end = 100;
                movement.m_pan_start = 0;
                movement.m_pan_end = 360;
                movement.m_backout_home_return = false;
				movement.m_alternate_groups = (motion == QSM_ROTATE) ? false : true;
                break;
        }

        movement_animation = new SceneMovementAnimator( NOUID, TRUE, NOUID, AnimationSignal( TRIGGER_TIMER, SOURCE_HIGH, timer ), movement );
    }

	if ( color_animation != NULL )
        addQuickEffectAnimation( color_animation, QUICK_COLOR_NUMBER, QUICK_COLOR_NAME, fixtures );

	if ( dimmer_animation != NULL )
		addQuickEffectAnimation( dimmer_animation, QUICK_DIMMER_NUMBER, QUICK_DIMMER_NAME, fixtures );

    if ( movement_animation != NULL ) {
        UIDArray moving_fixtures;

        for ( UID uid : fixtures ) {
            SceneActor* actor = captureFixture( uid );

            Fixture* pf = ( actor->isGroup() ) ? getGroupRepresentative( uid ) : getFixture( uid );
            STUDIO_ASSERT( pf != NULL, "Invalid default scene actor %lu", uid );

            if ( pf->canMove() )
                moving_fixtures.push_back( uid );
        }

        if ( !moving_fixtures.empty() )
            addQuickEffectAnimation( movement_animation, QUICK_MOVEMENT_NUMBER, QUICK_MOVEMENT_NAME, moving_fixtures );
    }

	lock.Unlock();

	sceneUpdated( m_default_scene_uid );
}

// ----------------------------------------------------------------------------
//
void Venue::addQuickEffectAnimation( AnimationDefinition* animation, AnimationNumber number, LPCSTR name, UIDArray& fixtures ) {
    UID existing_uid = number != 0 ? getAnimationByNumber( number ) : NOUID;
    UID animation_uid = ( existing_uid != NOUID ) ? existing_uid :  NOUID;

    animation->setUID( animation_uid );
    animation->setAnimationNumber( number );
    animation->setName( name );
    animation->setShared( number != 0 );

    if ( animation_uid == NOUID )
        animation_uid = addAnimation( animation );

    getDefaultScene()->addAnimation( AnimationReference( animation_uid, fixtures ) );

    if ( existing_uid != NOUID )
        replaceAnimation( animation );

    fireEvent( ES_ANIMATION, animation_uid, EA_START );
}

// ----------------------------------------------------------------------------
//
UINT Venue::run(void) {
	DMXStudio::log_status( "Fixture tracker running" );

	CSingleLock lock( &m_venue_mutex, FALSE );

    RGBWAArray pixel_colors;
    RGBWA color;

	while ( Threadable::isRunning() ) {
		try { 
			Sleep( 50 );

            if ( !m_packet_changed )
                continue;

			lock.Lock();

			// Track changes to color channel and generate events on every color change
			for ( FixtureStateMap::value_type& entry : m_fixture_states ) {
                FixtureState& fixture_state = entry.second;

				UINT dimmerPercent = 100;
                bool strobing = false;

                if ( fixture_state.getDimmerChannel() != NULL ) {
                    Channel* cp = fixture_state.getDimmerChannel();

					channel_value dimmer_value = m_multi_universe_packet[ fixture_state.getPacketAddress( cp->getOffset() )];
                    if ( dimmer_value == cp->getDimmerOffIntensity() )
                        dimmerPercent = 0;
                    else {
					    channel_value low = cp->getDimmerLowestIntensity();
					    channel_value high = cp->getDimmerHighestIntensity();

					    if ( low < high ) {
						    if ( dimmer_value >= low && dimmer_value <= high ) {
							    UINT range = high-low;
                                dimmer_value -= low;
							    dimmerPercent = (100 * dimmer_value) / range;
						    }
					    }
					    else {
						    if ( dimmer_value >= high && dimmer_value <= low ) {
							    UINT range = low-high;
                                dimmer_value -= high;
							    dimmerPercent = 100 - ((100 * dimmer_value) / range);
						    }
					    }
                    }
				}

                if ( fixture_state.getStrobeChannel() != NULL ) {
                    Channel* cp = fixture_state.getStrobeChannel();

                    channel_value strobe_value = m_multi_universe_packet[ fixture_state.getPacketAddress( cp->getOffset() )];
                    channel_value slow = cp->getStrobeSlow();
                    channel_value fast = cp->getStrobeFast();

                    if ( slow < fast && strobe_value >= slow && strobe_value <= fast )
                        strobing = true;
                    else if ( fast > slow && strobe_value >= fast && strobe_value <= slow )
                        strobing = true;
                }

                pixel_colors.clear();

				for ( Pixel pixel : *fixture_state.getPixels() ) {
                    // TODO - How to deal with other color mixes (white, amber, UV), currently, ignoring UV and
                    // prioritizing WHITE > AMBER > RGB

                    if ( pixel.hasWhite() && m_multi_universe_packet[fixture_state.getPacketAddress(pixel.white())] != 0 ) {
                        BYTE white = m_multi_universe_packet[fixture_state.getPacketAddress(pixel.white())] * dimmerPercent / 100;
                        color.red( white );
                        color.blue( white );
                        color.green( white );
                    }
                    else if ( pixel.hasAmber() && m_multi_universe_packet[fixture_state.getPacketAddress(pixel.amber())] != 0 ) {
                        BYTE amber = m_multi_universe_packet[fixture_state.getPacketAddress(pixel.amber())] * dimmerPercent / 100;
                        color.red( 250 * amber/255 );
                        color.green( color.red() / 2 );
                        color.blue( 0 );
                    }
                    else {
					    color.red( pixel.hasRed() ? m_multi_universe_packet[fixture_state.getPacketAddress(pixel.red())] * dimmerPercent / 100 : 0 );
                        color.green( pixel.hasGreen() ? m_multi_universe_packet[fixture_state.getPacketAddress(pixel.green())] * dimmerPercent / 100: 0 );
                        color.blue( pixel.hasBlue() ? m_multi_universe_packet[fixture_state.getPacketAddress(pixel.blue())] * dimmerPercent / 100 : 0 );
                    }

                    // Increase value for viewing in UI
                    HSV hsv = color.toHSV();

                    if ( hsv.v > .10 )
                        hsv.v = std::min( 1.00, hsv.v + 0.20 );

					pixel_colors.push_back( RGBWA( hsv ) );
				}

                // If no change, ignore
				if ( strobing == fixture_state.isStrobing() && pixel_colors.size() == fixture_state.colors().size() ) 
                    if ( std::equal<RGBWAArray::iterator,RGBWAArray::iterator>(  pixel_colors.begin(), pixel_colors.end(), fixture_state.colors().begin() ) )
						continue;
 
                fixture_state.colors( pixel_colors ); 
                fixture_state.setStrobing( strobing );

				fireEvent( ES_FIXTURE_STATUS, fixture_state.getFixtureUID(), EA_CHANGED, NULL, 1, (long)pixel_colors[0], strobing );
			}

            m_packet_changed = false;

			lock.Unlock();
		}
		catch ( std::exception& ex ) {
			if ( lock.IsLocked() )
				lock.Unlock();

			DMXStudio::log( ex );
		}
	}

	DMXStudio::log_status( "Fixture tracker stopped" );

	return 0;
}

// ----------------------------------------------------------------------------
//
FixtureState::FixtureState( Fixture* pf ) :
    m_fixture_uid( pf->getUID() ),
    m_channel_base( pf->getMultiPacketAddress( 0 ) ),
    m_dimmer_channel( NULL ),
    m_strobe_channel( NULL ),
    m_strobing( false )
{ 
    FixtureDefinition* fd = pf->getFixtureDefinition();

    m_pixel_array = fd->getPixels();

    // This ignores the fact that there may be multiple dimmer and strobe channels
    for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
        Channel* cp = pf->getChannel( channel );
        if ( m_dimmer_channel == NULL && cp->isDimmer() )
            m_dimmer_channel = cp;
        if ( m_strobe_channel == NULL && cp->isStrobe() )
            m_strobe_channel = cp;
    }
}

// ----------------------------------------------------------------------------
//
void Venue::getFixtureState( UID uid, RGBWA& color, bool& strobing ) {
    CSingleLock lock( &m_venue_mutex, TRUE );

    FixtureStateMap::iterator it = m_fixture_states.find( uid );
    if ( it != m_fixture_states.end() ) {
        color = ( it->second.colors().size() > 0 ) ? it->second.colors()[0] : RGBWA::BLACK;
        strobing = it->second.isStrobing();
    }
    else {
        color = RGBWA::BLACK;
        strobing = false;
    }
}

// ----------------------------------------------------------------------------
//
void Venue::autoCreateUniverseFixtures() {
    if ( studio.isHueAutoSetup() ) {
        FixtureNumber fixture_number = 1000;
        
        while ( getFixtureByNumber( fixture_number ) != NOUID )
            fixture_number += 100;

        for ( auto it : m_universes ) {
            Universe* universe = it.second;
            if ( universe->getType() != UniverseType::PHILIPS_HUE )
                continue;

            DriverFixtureArray driver_fixtures;
            if ( universe->discoverFixtures( driver_fixtures ) != DMX_OK )
                continue;

            for ( DriverFixture f : driver_fixtures ) {
                FixtureDefinition* fd = FixtureDefinition::lookupFixture( f.getManufacturer(), f.getModel(), f.getChannels() );
                if ( fd == NULL ) {
                    DMXStudio::log_status( "Universe ID %u discovered unrecognized %u channel fixture %s %s", 
                        universe->getId(), f.getChannels(), f.getManufacturer(), f.getModel() );
                    continue;
                }

                Fixture* ph = getFixtureByType( fd->getFUID(), universe->getId(), f.getDMXAddress() );
                if ( ph != NULL )
                    continue;

                addFixture( Fixture( NOUID, fixture_number, universe->getId(), f.getDMXAddress(), fd->getFUID(), f.getName() ) );

                studio.log_status( "Added %s %s fixture #%u [Universe %u DMX %u]", f.getManufacturer(), f.getModel(), fixture_number, universe->getId(), f.getDMXAddress() );

                fixture_number = nextAvailableFixtureNumber();
            }
        }
    }
}