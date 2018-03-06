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

#pragma once

/**
    Venue class contains a persitable installation.  Multiple venues are allowed, each
    consisting of universe description, fixtures scenes, chases and any other
    artifacts specific to a particular installation.
*/

#include "DMXStudio.h"
#include "IVisitor.h"
#include "Universe.h"
#include "Scene.h"
#include "FixtureGroup.h"
#include "Chase.h"
#include "ChaseEngine.h"
#include "AudioInputStream.h"
#include "AudioVolumeController.h"
#include "BeatDetector.h"
#include "SoundDetector.h"
#include "ColorStrobe.h"
#include "MusicWatcher.h"
#include "LevelRecord.h"
#include "FixtureState.h"

class AnimationEngine;

#define DEFAULT_SCENE_NUMBER	    0

#define QUICK_COLOR_NUMBER	        1
#define QUICK_COLOR_NAME            "Quick Effect Color Animation"

#define QUICK_MOVEMENT_NUMBER	    2
#define QUICK_MOVEMENT_NAME         "Quick Effect Movement Animation"

#define QUICK_DIMMER_NUMBER	        3
#define QUICK_DIMMER_NAME           "Quick Effect Dimmer Animation"

enum WhiteoutMode {
    WHITEOUT_OFF = 0,
    WHITEOUT_STROBE_SLOW = 1,
    WHITEOUT_STROBE_FAST = 2,
    WHITEOUT_STROBE_MANUAL = 3,
    WHITEOUT_ON = 4,
    WHITEOUT_PULSE = 5
};

enum WhiteoutEffect {
    WHITEOUT_EFFECT_SINGLE_COLOR = 0,
    WHITEOUT_EFFECT_MULTI_COLOR = 1,            // Palettes only
    WHITEOUT_EFFECT_SMART_COLOR = 2             // Palettes only
};

enum QuickSceneEffect {
	QSE_NONE = 0,
	QSE_COLOR = 1,
	QSE_COLOR_CHASE = 2,
	QSE_COLOR_STROBE = 3,
	QSE_COLOR_PULSE = 4,
	QSE_COLOR_FADER = 5,
    QSE_COLOR_BEAT = 6,
    QSE_COLOR_BREATH = 7,
    QSE_COLOR_DIMMER = 8
};

enum QuickSceneMovement {
	QSM_NONE = 0,
	QSM_NOD = 1,
	QSM_WAVE = 2,
	QSM_ROTATE = 3,
	QSM_RANDOM = 4,
	QSM_NOD_STAGGERED = 5,
	QSM_WAVE_STAGGERED = 6,
	QSM_ROTATE_STAGGERED = 7,
    QSM_SHORT_NOD = 8,
    QSM_SHORT_NOD_STAGGERED = 9

};

typedef std::map<BPMRating,UIDArray> SceneRatingsMap;

typedef std::map<universe_t,Universe*> UniverseMap;
typedef std::vector<Universe*> UniversePtrArray;

struct SetChannel {
    UID					m_fixture_uid;
	channel_address		m_address;
    BYTE				m_value;
    Channel*			m_cp;

    SetChannel( UID fixture_uid, channel_address address, channel_value value, Channel* cp=NULL ) :
        m_fixture_uid( fixture_uid ),
        m_address ( address ),
        m_value( value ),
        m_cp( cp )
    {}
};

typedef std::vector<SetChannel> SetChannelArray;

struct WhiteoutFixture {
    UID                 m_uid;
    FixtureType         m_type;
    bool                m_hasColor;
    bool                m_hasMovement;
    SetChannelArray     m_channels;
    SetChannelArray     m_dimmers;

    WhiteoutFixture() :
        m_uid( NOUID ),
        m_type( FixtureType::FIXT_UNKNOWN ),
        m_hasColor( false ),
        m_hasMovement( false )
    {}

    WhiteoutFixture( UID uid, FixtureType type ) :
        m_uid( uid ),
        m_type( type ),
        m_hasColor( false ),
        m_hasMovement( false )
    {}
};

typedef std::vector<WhiteoutFixture> WhiteoutFixtureArray;

class Venue : public DObject, Threadable
{
    friend class ChaseEngine;
    friend class AnimationEngine;
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
    AnimationMap            m_animations;

    PaletteMap              m_palettes;                         // The palette

    BYTE				    m_master_dimmer;					// Master dimmer( 0 -> 100 )
    UID					    m_current_scene;					// Currently active scene
    UID                     m_default_scene_uid;                // The default scene
    UID                     m_test_animation_uid;               // Common UID used for test animation
    UINT                    m_animation_speed;                  // Animation speed percentage (50=50%  100=100%, etc.)

    ChaseEngine*		    m_chase_task;
    AnimationEngine*        m_animation_task;
    DWORD				    m_auto_backout_ms;					// Auto backout timer (TODO: FADE OUT)
    bool				    m_mute_blackout;					// Blackout only colors / dimmer
    bool                    m_hard_blackout;                    // User forced lighting blackout
    bool                    m_track_fixtures;                   // Enables fixture tracking events

    // Run-time state for whiteout and other effects
    WhiteoutFixtureArray    m_whiteout_fixtures;
    SetChannelArray         m_blackout_channels;
    SetChannelArray         m_dimmer_channels;
    SetChannelArray         m_home_channels;
    UIDArray                m_whiteout_pulse_fixture_ids;
    RGBWAArray              m_whiteout_color_list;
    size_t                  m_whiteout_positive_index;
    size_t                  m_whiteout_pulse_index;
    
    BYTE                    m_multi_universe_packet[ MULTI_UNIV_PACKET_SIZE ];
    bool                    m_packet_changed;
    FixtureStateMap         m_fixture_states;

    CString			        m_audio_capture_device;				// Audio capture device name
    float                   m_audio_boost;                      // Scales incoming audio signal
    float                   m_audio_boost_floor;                // Minimum signal sample value (used when scaling)
    UINT                    m_audio_sample_size;                // Audio sample size (1024 defaut)

    WhiteoutMode            m_whiteout;                         // Whiteout color channels
    ColorStrobe             m_whiteout_strobe;                  // Whiteout strobe control
    UINT                    m_whiteout_strobe_ms;               // Manual strobe time MS
    UINT                    m_whiteout_fade_ms;                 // Manual strobe fade time MS (used for fade in&out)
    RGBWA                   m_whiteout_color;                   // Whiteout "color"
    WhiteoutEffect          m_whiteout_effect;                  // Whiteout effect

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

protected:
    UINT run(void);

public:
    Venue(void);
    ~Venue(void);

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    bool open(void);
    bool close(void);

    inline bool isRunning() {
        return m_animation_task != NULL;
    }

    inline CCriticalSection* getVenueLock() {
        return &m_venue_mutex;
    }

    inline bool isTrackFixtures( ) const {
        return m_track_fixtures;
    }
    inline void setTrackFixtures( bool track_fixtures ) {
        m_track_fixtures = track_fixtures;
    }

    void getFixtureState( UID uid, RGBWA& color, bool& strobing );

    inline bool isFixtureTracked( UID uid ) const {
        return m_fixture_states.find( uid ) != m_fixture_states.end();
    }

    void clearAllUniverses();
    void addUniverse( Universe* universe );
    Universe* getUniverse( size_t universe_num );
    UniversePtrArray getUniverses();

    UID getAnimationByNumber( AnimationNumber animation_number );
    UID getChaseByNumber( ChaseNumber chase_number );
    UID getFixtureGroupByNumber( GroupNumber group_number );
    UID getSceneByNumber( SceneNumber scene_number );
    UID getPaletteByNumber( PaletteNumber palette_number );
    UID getFixtureByNumber( FixtureNumber fixture_number );

    bool isMusicSceneSelectEnabled() const {
        return m_music_scene_select_enabled;
    }
    void setMusicSceneSelectEnabled( bool music_scene_select_enabled ) {
        m_music_scene_select_enabled = music_scene_select_enabled;

        fireEvent( ES_MUSIC_MATCH, 0L, music_scene_select_enabled ? EA_START : EA_STOP );
    }

    MusicSceneSelectMap& music_scene_select_map() {
        return m_music_scene_select_map;
    }

    inline UINT getAnimationSpeed() const {
        return m_animation_speed;
    }
    void setAnimationSpeed( UINT speed );

    void mapMusicToScene( LPCSTR track_link, MusicSelectorType& type, UID& type_uid );
    void addMusicMapping( MusicSceneSelector& music_scene_selector );
    void deleteMusicMapping( LPCSTR track_full_name );
    void deleteMusicMappings( MusicSelectorType type, UID uid );
    void clearMusicMappings();
    void addMusicMappings( std::vector<MusicSceneSelector>& music_scene_selectors );
    bool findMusicMapping( LPCSTR track_link, MusicSceneSelector& result );

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
        if ( m_volume ) {
            m_volume->setMasterVolume( volume );
        }
    }

    bool isMasterVolumeMute(void) const {
        return ( m_volume ) ? m_volume->isMute() : false;
    }
    inline void setMasterVolumeMute( bool mute ) {
        if ( m_volume ) {
            m_volume->setMute( mute );
        }
    }

    void setHomePositions( LPBYTE dmx_packet );

    inline WhiteoutMode getWhiteout() const {
        return m_whiteout;
    }
    void setWhiteout( WhiteoutMode whiteout );

    void setAudioBoost( float scale ) {
        m_audio_boost = scale;
    }
    inline float getAudioBoost() const {
        return m_audio_boost;
    }

    void setWhiteoutStrobeMS( UINT strobe_ms, UINT fade_ms );

    UINT getWhiteoutStrobeMS() const {
        return m_whiteout_strobe_ms;
    }
    inline UINT getWhiteoutFadeMS() const {
        return m_whiteout_fade_ms;
    }

    inline RGBWA getWhiteoutColor() const {
        return m_whiteout_color;
    }
    void setWhiteoutColor( RGBWA& color );

    inline WhiteoutEffect getWhiteoutEffect() const {
        return m_whiteout_effect;
    }
    void setWhiteoutEffect( WhiteoutEffect effect );

    void advanceWhiteoutColor();

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

        fireEvent( ES_VENUE, 0L, EA_CHANGED );
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

    inline void getSoundLevels( SoundLevel& level ) {
        STUDIO_ASSERT( m_sound_detector != NULL, "No sound detector" );

        getSoundDetector()->getSoundData( level );
    }

    void writePacket( const channel_value* dmx_packet, bool apply_default_scene );
    void readPacket( channel_value* dmx_packet );
	void applyDefaultActors( channel_value* dmx_packet, ChannelDataList* returned_channel_data = NULL );

    UID whoIsAddressRange( universe_t universe, channel_address start_address, channel_address end_address );
	channel_address findFreeAddressRange( universe_t universe, UINT num_channels );

    void setMasterDimmer( BYTE dimmer ) {
        STUDIO_ASSERT( dimmer <= 100, "Master dimmer level must be between 0 and 100" );
        m_master_dimmer = dimmer;

        fireEvent( ES_MASTER_DIMMER, 0L, EA_CHANGED, NULL, dimmer );
    }
    BYTE getMasterDimmer( ) const {
        return m_master_dimmer;
    }

    inline void setMuteBlackout( bool blackout ) {
        m_mute_blackout = blackout;

        fireEvent( ES_MUTE_BLACKOUT, 0L, blackout ? EA_START : EA_STOP );
    }
    inline bool isMuteBlackout( ) const {
        return m_mute_blackout;
    }

    void setForceBlackout( bool hard_blackout );

    inline bool isForceBlackout() const {
        return m_hard_blackout;
    }

    void setAutoBlackoutMS( DWORD black_out ) {
        m_auto_backout_ms = black_out;
        if ( isRunning() ) {
            getSoundDetector()->setMuteMS( black_out );
        }
        fireEvent( ES_MUTE_BLACKOUT, 0L, EA_CHANGED, NULL, black_out );
    }
    DWORD getAutoBlackoutMS( ) const {
        return m_auto_backout_ms;
    }

    // Physical fixture methods
    Fixture* getFixture( UID pfuid );
    FixturePtrArray getFixtures();
    FixtureNumber nextAvailableFixtureNumber( void );
    UID addFixture( Fixture& pfixture );
    bool deleteFixture( UID pfuid );
    void fixtureUpdated( UID pfuid );
	channel_value getBaseChannelValue( Fixture* pfixture, channel_address channel );
	channel_value getBaseChannelValue( SceneActor& actor, channel_address channel );
	channel_value getBaseChannelValue( FixtureGroup* group, channel_address channel );
    Fixture* getFixtureByType( FUID fuid, universe_t universe, channel_address base_dmx_channel );

    // Default Scene methods
    void copySceneFixtureToDefault( UID scene_uid, UID fixture_uid );
    void moveDefaultFixturesToScene( UIDArray scene_uids, UIDArray actor_uids, boolean keep_groups, boolean clear_default, boolean copy_animations );
    void moveDefaultFixturesToScene( UID scene_uid, boolean keep_groups, boolean clear_default, boolean copy_animations );

    SceneActor* captureFixture( UID fixture_uid, std::vector<BYTE>* channel_values=NULL, UIDArray* palette_refs=NULL );
    void captureAndSetChannelValue( SetChannelArray channel_sets, DWORD change_id = 0L );
    void captureAndSetPalettes( UID actor_uid, UIDArray& palette_refs );

    SceneActor* getCapturedActor();
    void releaseActor( UID actor_id );
    void clearAllCapturedActors( );
    FixturePtrArray resolveActorFixtures( SceneActor* actor );
    Fixture* getGroupRepresentative( UID group_id );

    // Palette methods
    bool deletePalette( UID palette_id );
    UID addPalette( Palette& palette );
    void updatePalette( Palette& palette );
    PaletteNumber nextAvailablePaletteNumber( void );
    bool getSystemPalette( ULONG palette_number, RGBWAArray& palette_colors );
    void setVideoPalette( RGBWAArray& palette_colors, ColorWeights& palette_weights );
    DObjectArray getPaletteSummary();

    template <class T>
    inline bool getPalette( T identifier, Palette& palette ) {
        Palette* p = getPalette( identifier );
        if ( p == NULL )
            return false;
        palette = *p;
        return true;
    }

    // Scene methods
    Scene *getScene( UID scene_uid );
    ScenePtrArray getScenes();
    bool deleteScene( UID scene_uid );
    UID addScene( Scene& scene, bool isDefault=false );
    void playScene( UID scene_uid, SceneLoadMethod method );
    void stageActors( UID scene_uid );
    void clearAllAnimations();

    SceneNumber nextAvailableSceneNumber( void );
    void deleteAllScenes();
    UID getRandomScene();
    void populateSceneRatingsMap( SceneRatingsMap& map );
    void sceneUpdated( UID scene_uid );
    void fadeToNextScene( UID scene_uid, ULONG fade_time );
    void setSceneAnimationReferences( Scene* scene, AnimationReferenceArray& animation_refs, bool copyPrivateAnimations );

    inline void selectScene( UID scene_uid ) {
        // Stop any running chases and load the new object
        stopChase();

        playScene( scene_uid, SLM_LOAD );
    }

    inline UID getCurrentSceneUID() const {
        return m_current_scene;
    }

    inline Scene *getScene( ) {
        return getScene( m_current_scene );
    }

    inline Scene *getDefaultScene() {
        return getScene( m_default_scene_uid );
    }

    inline bool isDefaultScene() const {
        return m_current_scene == m_default_scene_uid;
    }

    inline size_t getNumScenes( ) const {
        return m_scenes.size();
    }

    // Fixture group methods
    UID addFixtureGroup( FixtureGroup& group );
    FixtureGroupPtrArray getFixtureGroups( );
    FixtureGroup* getFixtureGroup( UID group_id );
    bool deleteFixtureGroup( UID group_id );
    GroupNumber nextAvailableFixtureGroupNumber( void );
    void deleteAllFixtureGroups();

    // Scene chase methods
    Chase *getChase( UID chase_uid );
    UID addChase( Chase& chase );
    ChasePtrArray getChases( );
    bool deleteChase( UID chase_id );
    bool copyChaseSteps( UID source_chase_id, UID target_chase_id );
    ChaseNumber nextAvailableChaseNumber( void );
    void deleteAllChases();
    UID getRandomChase();
    void chaseUpdated( UID chase_uid );
    void chaseStep( int steps );

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

    // Animation methods
    AnimationDefinition* getAnimation( UID animation_uid );
    AnimationPtrArray getAnimations();
    bool deleteAnimation( UID animation_uid );
    UID addAnimation( AnimationDefinition* animation );
    void replaceAnimation( AnimationDefinition* animation );
    void testAnimation( AnimationDefinition* animation );
    AnimationNumber nextAvailableAnimationNumber( void );
    void deleteAllAnimations();
    void captureAnimation( UID animation_uid );
    void releaseAnimation( UID animation_uid );

    inline size_t getNumAnimations( ) const {
        return m_animations.size();
    }

    // Venue and events
    void configure( LPCSTR name, LPCSTR description, LPCSTR audio_capture_device, float audio_boost, 
                    float audio_boost_floor, int audio_sample_size, int auto_blackout, bool track_fixtures, 
                    std::vector<Universe>& universes );

    inline bool fireEvent( EventSource source, DWORD uid, EventAction action, LPCSTR text=NULL, DWORD val1=0L, DWORD val2=0L, DWORD val3=0L, DWORD val4=0L ) {
        if ( isRunning() )
            return DMXStudio::fireEvent( source, uid, action, text, val1, val2, val3, val4 );

        return false;
    }

    void computeActorFinalValues( Scene* scene );
    void computeActorFinalValues( SceneActor* actor );

    AnimationLevelMap loadAnimationLevelData( DWORD after_ms );

	void createQuickScene( UIDArray fixtures, QuickSceneEffect effect, RGBWA color, unsigned color_speed_ms, 
                           QuickSceneMovement movement, unsigned move_speed_ms, bool multi_color );
    void stopQuickScene();

private:
    inline UID allocUID() {
        return m_uid_pool++;
    }

    SceneActor * getDefaultActor( UID pfuid );
	void loadSceneChannels( channel_value *dmx_multi_universe_packet, ActorPtrArray& actors, ChannelDataList* returned_channel_data = NULL );
	channel_address loadChannel( channel_value *dmx_packet, Fixture* pf, channel_address channel, channel_value value );
    void whiteoutChannels( channel_value *dmx_packet );
    void smartWhiteoutChannels( channel_value *dmx_packet );
    void blackoutChannels( channel_value *dmx_packet );
    void dimmerChannels( channel_value *dmx_packet );
    void removeFixtureFromPalette( UID fixture_id );
    void scanFixtureChannels( void );
    void setupGlobalPaletteEntries();
    void addActorToDefaultAnimations( UID fixture_id );
    void addQuickEffectAnimation( AnimationDefinition* animation, AnimationNumber number, LPCSTR name, UIDArray& fixtures );
    void autoCreateUniverseFixtures();

    Palette* getPalette( UID palette_id );
    Palette* getPalette( LPCSTR palette_name );
};
