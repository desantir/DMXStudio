/* 
Copyright (C) 2011-15 Robert DeSantis
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

/**
    Venue class contains a persitable installation.  Multiple venues are allowed, each
    consisting of universe description, fixtures scenes, chases and any other
    artifacts specific to a particular installation.
*/

#include "IVisitor.h"
#include "Universe.h"
#include "Scene.h"
#include "FixtureGroup.h"
#include "Chase.h"
#include "ChaseTask.h"
#include "AudioInputStream.h"
#include "AudioVolumeController.h"
#include "BeatDetector.h"
#include "SoundDetector.h"
#include "ColorStrobe.h"
#include "MusicWatcher.h"

class AnimationTask;

#define DEFAULT_SCENE_NUMBER	0

typedef enum {
    WHITEOUT_OFF = 0,
    WHITEOUT_STROBE_SLOW = 1,
    WHITEOUT_STROBE_FAST = 2,
    WHITEOUT_STROBE_MANUAL = 3,
    WHITEOUT_ON = 4
} WhiteoutMode;

typedef std::map<BPMRating,UIDArray> SceneRatingsMap;

typedef std::map<universe_t,Universe*> UniverseMap;
typedef std::vector<Universe*> UniversePtrArray;

class Venue : public DObject
{
    friend class ChaseTask;
    friend class AnimationTask;
    friend class VenueWriter;
    friend class VenueReader;
    friend class Scene;

    CCriticalSection        m_venue_mutex;						// Protect venue objects

    UID					    m_uid_pool;

    UniverseMap             m_universes;                        // DMX universes

    FixtureMap			    m_fixtures;							// List of fixture instances
    SceneMap			    m_scenes;
    FixtureGroupMap		    m_fixtureGroups;
    ChaseMap			    m_chases;

    BYTE				    m_master_dimmer;					// Master dimmer( 0 -> 100 )
    UID					    m_current_scene;					// Currently active scene
    ChaseTask*			    m_chase_task;
    AnimationTask*		    m_animation_task;
    DWORD				    m_auto_backout_ms;					// Auto backout timer (TODO: FADE OUT)
    bool				    m_mute_blackout;					// Blackout only colors / dimmer
    bool                    m_hard_blackout;                    // User forced lighting blackout

    CString			        m_audio_capture_device;				// Audio capture device name
    float                   m_audio_boost;                      // Scales incoming audio signal
    float                   m_audio_boost_floor;                // Minimum signal sample value (used when scaling)
    UINT                    m_audio_sample_size;                // Audio sample size (1024 defaut)

    StrobeTime              m_whiteout_strobe_slow;
    StrobeTime              m_whiteout_strobe_fast;
    WhiteoutMode            m_whiteout;                         // Whiteout color channels
    ColorStrobe             m_whiteout_strobe;                  // Whiteout strobe control
    UINT                    m_whiteout_strobe_ms;               // Manual strobe time MS
    RGBWA                   m_whiteout_color;                   // Whiteout "color"

    AudioInputStream*	    m_audio;
    SoundDetector*		    m_sound_detector;
    AudioVolumeController*  m_volume;
    MusicWatcher*           m_music_watcher;
    bool                    m_music_scene_select_enabled;
    MusicSceneSelectMap     m_music_scene_select_map;

    UID                     m_captured_actor;                   // Last actor captured to default (can be single or group)

    CString                 m_venue_layout;                     // Client venue layout (JSON)

    Venue(Venue& other) {}
    Venue& operator=(Venue& rhs) { return *this; }

public:
    Venue(void);
    ~Venue(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    bool open(void);
    bool close(void);
    bool isRunning();

    void clearAllUniverses();
    void addUniverse( Universe* universe );
    Universe* getUniverse( size_t universe_num );
    UniversePtrArray getUniverses();

    bool isMusicSceneSelectEnabled() const {
        return m_music_scene_select_enabled;
    }
    void setMusicSceneSelectEnabled( bool music_scene_select_enabled ) {
        m_music_scene_select_enabled = music_scene_select_enabled;
    }

    MusicSceneSelectMap& music_scene_select_map() {
        return m_music_scene_select_map;
    }

    void mapMusicToScene( LPCSTR track_link, MusicSelectorType& type, UID& type_uid );
    void addMusicMapping( MusicSceneSelector& music_scene_selector );
    void deleteMusicMapping( LPCSTR track_full_name );
    void deleteMusicMappings( MusicSelectorType type, UID uid );
    void clearMusicMappings();
    void addMusicMappings( std::vector<MusicSceneSelector>& music_scene_selectors );

    inline UINT getAudioSampleSize() const {
        return m_audio_sample_size;
    }
    inline void setAudioSampleSize( UINT audio_sample_size) {
        m_audio_sample_size = audio_sample_size;
    }

    inline UINT getMasterVolume(void) const {
        return ( m_volume ) ? m_volume->getMasterVolume() : 0;
    }
    inline void setMasterVolume( UINT volume ) {
        if ( m_volume )
            m_volume->setMasterVolume( volume );
    }

    bool isMasterVolumeMute(void) const {
        return ( m_volume ) ? m_volume->isMute() : false;
    }
    inline void setMasterVolumeMute( bool mute ) {
        if ( m_volume )
            m_volume->setMute( mute );
    }

    void setHomePositions( LPBYTE dmx_packet );

    WhiteoutMode getWhiteout() const {
        return m_whiteout;
    }
    void setWhiteout( WhiteoutMode whiteout );

    void setAudioBoost( float scale ) {
        m_audio_boost = scale;
    }
    float getAudioBoost() const {
        return m_audio_boost;
    }

    DWORD getAnimationSampleRate();
    void setAnimationSampleRate( DWORD sample_rate_ms );

    void setWhiteoutStrobeMS( UINT strobe_ms ) {
        m_whiteout_strobe_ms = strobe_ms;
        if ( m_whiteout == WHITEOUT_STROBE_MANUAL )     // Need to reset strobe timer
            setWhiteout( WHITEOUT_STROBE_MANUAL );
    }
    UINT getWhiteoutStrobeMS() const {
        return m_whiteout_strobe_ms;
    }

    RGBWA getWhiteoutColor() const {
        return m_whiteout_color;
    }
    void setWhiteoutColor( RGBWA& color ) {
        m_whiteout_color = color;
        if ( color.red() == 0xFF && color.blue() == 0xFF && color.green() == 0xFF )
            m_whiteout_color.white( 0xFF );             // For fixtures with white channel
    }

    void setAudioBoostFloor( float boost_floor ) {
        m_audio_boost_floor = boost_floor;
    }
    float getAudioBoostFloor() const {
        return m_audio_boost_floor;
    }

    const char *getVenueLayout() const {
        if ( m_venue_layout.GetLength() == 0 )
            return NULL;

        return (LPCSTR)m_venue_layout;
    }
    void setVenueLayout( const char * layout ) {
        if ( layout != NULL )
            m_venue_layout = layout;
        else
            m_venue_layout.Empty();
    }

    const char* getAudioCaptureDevice( ) const {
        return m_audio_capture_device;
    }
    void setAudioCaptureDevice( const char* audio_capture_device ) {
        m_audio_capture_device = audio_capture_device;
    }

    AudioInputStream* getAudio() {
        STUDIO_ASSERT( m_audio != NULL, "No audio stream" );
        return m_audio;
    }

    inline SoundDetector* getSoundDetector() const {
        STUDIO_ASSERT( m_sound_detector != NULL, "No sound detector" );
        return m_sound_detector;
    }

    bool isMute() const {
        return getSoundDetector()->isMute();
    }

    unsigned getSoundLevel( ) const {
        return getSoundDetector()->getAmplitude();
    }

    unsigned getAvgAmplitude( ) const {
        return getSoundDetector()->getAvgAmplitude();
    }

    void writePacket( const BYTE* dmx_packet );
    void readPacket( BYTE* dmx_packet );

    UID whoIsAddressRange( universe_t universe, channel_t start_address, channel_t end_address );
    channel_t findFreeAddressRange( universe_t universe, UINT num_channels );

    void setMasterDimmer( BYTE dimmer ) {
        STUDIO_ASSERT( dimmer <= 100, "Master dimmer level must be between 0 and 100" );
        m_master_dimmer = dimmer;
    }
    BYTE getMasterDimmer( ) const {
        return m_master_dimmer;
    }

    void setMuteBlackout( bool blackout ) {
        m_mute_blackout = blackout;
    }
    bool isMuteBlackout( ) const {
        return m_mute_blackout;
    }

    void setBlackout( bool hard_blackout ) {
        m_hard_blackout = hard_blackout;

        for ( UniverseMap::iterator it=m_universes.begin(); it != m_universes.end(); ++it )
            (*it).second->setBlackout( m_hard_blackout );
    }
    inline bool isBlackout() const {
        return m_hard_blackout;
    }

    void setAutoBlackout( DWORD black_out ) {
        m_auto_backout_ms = black_out;
        if ( isRunning() ) {
            getSoundDetector()->setMuteMS( black_out );
            if ( black_out == 0 )
                setMuteBlackout( false );
            loadScene();
        }
    }
    DWORD getAutoBlackout( ) const {
        return m_auto_backout_ms;
    }

    // Physical fixture methods
    Fixture* getFixture( UID pfuid );
    FixturePtrArray getFixtures();
    Fixture *getFixtureByNumber( FixtureNumber fixture_number );
    FixtureNumber nextAvailableFixtureNumber( void );
    UID addFixture( Fixture& pfixture );
    bool deleteFixture( UID pfuid );
    BYTE getChannelValue( Fixture* pfixture, channel_t channel );
    BYTE getChannelValue( SceneActor& actor, channel_t channel );

    // Default Scene methods
    void copySceneFixtureToDefault( UID scene_uid, UID fixture_uid );
    void moveDefaultFixturesToScene( UID scene_uid, UIDArray actor_uid, boolean keep_groups, boolean clear_default );
    void moveDefaultFixturesToScene( UID scene_uid, boolean keep_groups, boolean clear_default );

    SceneActor* captureFixture( UID fixture_uid );
    SceneActor* captureFixtureGroup( UID group_uid );
    SceneActor* getCapturedActor();
    void releaseActor( UID actor_id );
    void clearAllCapturedActors( );
    FixturePtrArray resolveActorFixtures( SceneActor* actor );
    Fixture* getGroupRepresentative( UID group_id );

    void captureAndSetChannelValue( SceneActor& actor, channel_t channel, BYTE value );

    // Scene methods
    Scene *getScene( UID scene_uid );
    ScenePtrArray getScenes();
    void selectScene( UID scene_uid );
    bool deleteScene( UID scene_uid );
    void updateCurrentScene( );
    UID addScene( Scene& scene );
    void loadScene();
    void clearAnimations();
    Scene *getSceneByNumber( SceneNumber scene_number );
    SceneNumber nextAvailableSceneNumber( void );
    void deleteAllScenes();
    UID getRandomScene();
    void populateSceneRatingsMap( SceneRatingsMap& map );

    UID getCurrentSceneUID() const {
        return m_current_scene;
    }

    Scene *getScene( ) {
        return getScene( m_current_scene );
    }

    Scene *getDefaultScene() {
        return getSceneByNumber( DEFAULT_SCENE_NUMBER );
    }

    bool isDefaultScene() {
        return (getScene()->getSceneNumber() == DEFAULT_SCENE_NUMBER );
    }

    size_t getNumScenes( ) const {
        return m_scenes.size();
    }

    // Fixture group methods
    UID addFixtureGroup( FixtureGroup& group );
    FixtureGroupPtrArray getFixtureGroups( );
    FixtureGroup* getFixtureGroup( UID group_id );
    bool deleteFixtureGroup( UID group_id );
    GroupNumber nextAvailableFixtureGroupNumber( void );
    FixtureGroup* getFixtureGroupByNumber( GroupNumber group_number );
    void deleteAllFixtureGroups();

    // Scene chase methods
    Chase *getChase( UID chase_uid );
    UID addChase( Chase& chase );
    ChasePtrArray getChases( );
    Chase* getChaseByNumber( ChaseNumber chase_number );
    bool deleteChase( UID chase_id );
    bool copyChaseSteps( UID source_chase_id, UID target_chase_id );
    ChaseNumber nextAvailableChaseNumber( void );
    void deleteAllChases();
    UID getRandomChase();

    inline size_t getNumChases() const {
        return m_chases.size();
    }

    bool startChase( UID chase_id );
    bool stopChase();

    bool isChaseRunning() const {
        return m_chase_task != NULL && m_chase_task->isChaseRunning();
    }

    bool isChaseFading() const {
        return isChaseRunning() && m_chase_task->isFading();
    }

    UID getRunningChase() const {
        if ( !isChaseRunning() )
            return 0;
        return m_chase_task->getChase()->getUID();
    }

private:
    inline UID allocUID() {
        return m_uid_pool++;
    }

    SceneActor * getDefaultActor( UID pfuid );
    void loadSceneChannels( BYTE *dmx_packet, Scene * scene );
    void loadChannel( BYTE *dmx_packet, Fixture* pf, channel_t channel, BYTE value );
    void whiteoutChannels( LPBYTE dmx_packet );
    BYTE adjustChannelValue( Fixture* pf, channel_t channel, BYTE value );
};
