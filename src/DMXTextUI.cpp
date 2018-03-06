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

#include "DMXTextUI.h"

static const char * LINE_CLEAR = "\r                                                                                                    \r";

static HandlerMap function_map;

static CString track_time( DWORD time );

static LPCSTR sceneLoadMethods[] = { NULL, "Load", "Add", "Remove" };

// ----------------------------------------------------------------------------
//
DMXTextUI::DMXTextUI(void)
{
    function_map[ "q" ] = HandlerInfo( &DMXTextUI::quit,                        false,  "Quit" );
    function_map[ "?" ] = HandlerInfo( &DMXTextUI::help,                        false,  "Help" );
    function_map[ "vo" ] = HandlerInfo( &DMXTextUI::loadVenue,                  false,  "Venue Open" );
    function_map[ "vs" ] = HandlerInfo( &DMXTextUI::writeVenue,                 false,  "Venue Save" );
    function_map[ "vc" ] = HandlerInfo( &DMXTextUI::configVenue,                false,  "Venue Configuration" );

    function_map[ "s" ] = HandlerInfo( &DMXTextUI::selectScene,                 true,   "Scene Load" );
    function_map[ "sc" ] = HandlerInfo( &DMXTextUI::createScene,                true,   "Scene Create" );
    function_map[ "scp" ] = HandlerInfo( &DMXTextUI::copyScene,                 true,   "Scene CoPy" );
    function_map[ "sd" ] = HandlerInfo( &DMXTextUI::deleteScene,                true,   "Scene Delete" );
    function_map[ "su" ] = HandlerInfo( &DMXTextUI::updateScene,                true,   "Scene Update" );
    function_map[ "sx" ] = HandlerInfo( &DMXTextUI::describeScene,              true,   "Scene Describe" );
    function_map[ "saa" ] = HandlerInfo( &DMXTextUI::sceneAddAnimation,         true,   "Scene Animation Add" );
    function_map[ "sau" ] = HandlerInfo( &DMXTextUI::sceneUpdateAnimation,      true,   "Scene Animation Update" );
    function_map[ "sac" ] = HandlerInfo( &DMXTextUI::sceneDeleteAllAnimations,  true,   "Scene Clear all Animations" );
    function_map[ "sad" ] = HandlerInfo( &DMXTextUI::sceneDeleteAnimation,      true,   "Scene Animation Delete" );	
    function_map[ "scf" ] = HandlerInfo( &DMXTextUI::copyFixture,               true,   "Scene Copy Fixture to workspace" );
    function_map[ "sw" ] = HandlerInfo( &DMXTextUI::copyWorkspaceToScene,		true,   "Scene Copy Workspace Fixture(s) to scene" );
    function_map[ "sdf" ] = HandlerInfo( &DMXTextUI::deleteFixtureFromScene,	true,   "Scene Delete Fixture" );

    function_map[ "gc" ] = HandlerInfo( &DMXTextUI::createFixtureGroup,         true,   "Group Create" );
    function_map[ "gx" ] = HandlerInfo( &DMXTextUI::describeFixtureGroup,       true,   "Group Describe" );
    function_map[ "gd" ] = HandlerInfo( &DMXTextUI::deleteFixtureGroup,         true,   "Group Delete" );

    function_map[ "c" ] = HandlerInfo( &DMXTextUI::selectChase,                 true,   "Chase start/stop" );
    function_map[ "cd" ] = HandlerInfo( &DMXTextUI::deleteChase,                true,   "Chase Delete" );
    function_map[ "cx" ] = HandlerInfo( &DMXTextUI::describeChase,              true,   "Chase Describe" );
    function_map[ "cc" ] = HandlerInfo( &DMXTextUI::createChase,                true,   "Chase Create" );
    function_map[ "cu" ] = HandlerInfo( &DMXTextUI::updateChase,                true,   "Chase Update" );
    function_map[ "cv" ] = HandlerInfo( &DMXTextUI::copyChaseSteps,             true,   "Chase copy steps" );
    function_map[ "csd" ] = HandlerInfo( &DMXTextUI::deleteChaseSteps,          true,   "Chase delete steps" );
    function_map[ "csa" ] = HandlerInfo( &DMXTextUI::addChaseSteps,             true,   "Chase add steps" );
    function_map[ "ca" ] = HandlerInfo( &DMXTextUI::advanceChase,               true,   "Chase advance" );

    function_map[ "r" ] = HandlerInfo( &DMXTextUI::resetDefaultFixtures,        true,   "Reset workspace fixtures" );
    function_map[ "f" ] = HandlerInfo( &DMXTextUI::controlFixture,              true,   "Fixture control (copy to workspace)" );
    function_map[ "fc" ] = HandlerInfo( &DMXTextUI::createFixture,              true,   "Fixture Create" );
    function_map[ "fd" ] = HandlerInfo( &DMXTextUI::deleteFixture,              true,   "Fixture Delete" );
    function_map[ "fu" ] = HandlerInfo( &DMXTextUI::updateFixture,              true,   "Fixture Update" );
    function_map[ "fx" ] = HandlerInfo( &DMXTextUI::describeFixture,            true,   "Fixture describe" );

    function_map[ "d" ] = HandlerInfo( &DMXTextUI::masterDimmer,                true,   "Master dimmer" );

    function_map[ "hz" ] = HandlerInfo( &DMXTextUI::soundDb,                    true,   "Sound sample tests" );
    function_map[ "hy" ] = HandlerInfo( &DMXTextUI::soundDetect,                true,   "Sound peak test" );
    function_map[ "hbpm" ] = HandlerInfo( &DMXTextUI::soundBPM,                 true,   "Sound energy beat test" );
    function_map[ "hb" ] = HandlerInfo( &DMXTextUI::soundDebug,                 true,   "Sound debug toggle" );
    function_map[ "hfb" ] = HandlerInfo( &DMXTextUI::soundBeat,                 true,   "Sound frequency beat test" );
            
    function_map[ "b" ] = HandlerInfo( &DMXTextUI::blackout,                    true,   "Blackout" );
    function_map[ "ba" ] = HandlerInfo( &DMXTextUI::autoBlackout,               true,   "Set venue auto blackout" );
    function_map[ "w" ] = HandlerInfo( &DMXTextUI::whiteout,                    true,   "Whiteout" );
    function_map[ "wo" ] = HandlerInfo( &DMXTextUI::whiteout_options,           true,   "Whiteout options" );

    function_map[ "as" ] = HandlerInfo( &DMXTextUI::animationSpeed,	            true,   "Animation Speed" );
    function_map[ "ac" ] = HandlerInfo( &DMXTextUI::createAnimation,            true,   "Create Animation" );
    function_map[ "ax" ] = HandlerInfo( &DMXTextUI::describeAnimation,          true,   "Describe Animation" );
    function_map[ "ad" ] = HandlerInfo( &DMXTextUI::deleteAnimation,            true,   "Delete Animation" );
    function_map[ "au" ] = HandlerInfo( &DMXTextUI::updateAnimation,            true,   "Update Animation" );

    function_map[ "v" ] = HandlerInfo( &DMXTextUI::masterVolume,                true,   "Master Volume" );
    function_map[ "vm" ] = HandlerInfo( &DMXTextUI::muteVolume,                 true,   "Mute Volume" );

    function_map[ "px" ] = HandlerInfo( &DMXTextUI::describePalette,            true,   "Palette Describe" ); 
    function_map[ "pc" ] = HandlerInfo( &DMXTextUI::createPalette,              true,   "Palette Create" ); 
    function_map[ "pd" ] = HandlerInfo( &DMXTextUI::deletePalette,              true,   "Palette Delete" ); 
    function_map[ "pu" ] = HandlerInfo( &DMXTextUI::updatePalette,              true,   "Palette Update" ); 

    function_map[ "pfu" ] = HandlerInfo( &DMXTextUI::updatePaletteFixture,      true,   "Palett Add/Update Fixture" ); 
    function_map[ "pfd" ] = HandlerInfo( &DMXTextUI::deletePaletteFixture,      true,   "Palette Delete Fixture" ); 
    function_map[ "pfdu" ] = HandlerInfo( &DMXTextUI::updatePaletteFixtureDef,  true,   "Palette Add/Update Fixture Definition" );
    function_map[ "pfdd" ] = HandlerInfo( &DMXTextUI::deletePaletteFixtureDef,  true,   "Palette Delete Fixture Definition" );

    function_map[ "psu" ] = HandlerInfo( &DMXTextUI::updateScenePalettes,	    true,   "Scene Actor And/Update Palettes" );

    if ( studio.hasMusicPlayer() ) {
        function_map[ "plx" ] = HandlerInfo( &DMXTextUI::listPlaylists,          false,  "List Playlists" );
        function_map[ "tx" ] = HandlerInfo( &DMXTextUI::listTracks,             false,  "List Tracks" );
        function_map[ "tp" ] = HandlerInfo( &DMXTextUI::playTrack,	    		false,  "Play Track" );
        function_map[ "tq" ] = HandlerInfo( &DMXTextUI::queueTrack,             false,  "Queue Track" );
        function_map[ "plq" ] = HandlerInfo( &DMXTextUI::queuePlaylist,          false,  "Queue Playlist" );
        function_map[ "plp" ] = HandlerInfo( &DMXTextUI::playPlaylist,           false,  "Play Playlist" );
        function_map[ "p" ] = HandlerInfo( &DMXTextUI::pauseTrack,              false,  "Track pause" );
        function_map[ "ts" ] = HandlerInfo( &DMXTextUI::stopTrack,              false,  "Stop track" );
        function_map[ "tf" ] = HandlerInfo( &DMXTextUI::forwardTrack,           false,  "Forward track" );
        function_map[ "tb" ] = HandlerInfo( &DMXTextUI::backTrack,              false,  "Back track" );
        function_map[ "tqx" ] = HandlerInfo( &DMXTextUI::showQueuedTracks,      false,  "Show track queue" );
        function_map[ "mpl" ] = HandlerInfo( &DMXTextUI::musicPlayerLogin,      false,  "Log into music player" );

        function_map[ "me" ] = HandlerInfo( &DMXTextUI::musicSceneSelect,       false,  "Music Scene Select" );
        function_map[ "ma" ] = HandlerInfo( &DMXTextUI::musicMapTrack,          false,  "Map music to scene or chase" );
        function_map[ "mr" ] = HandlerInfo( &DMXTextUI::musicRemoveMapping,     false,  "Remove music mapping" );
        function_map[ "mx" ] = HandlerInfo( &DMXTextUI::musicMapShow,           false,  "Show music mappings" );
        function_map[ "mpl" ] = HandlerInfo( &DMXTextUI::musicPlayerLogin,      false,  "Show player login" );
    }

    // function_map[ "test" ] = HandlerInfo( &DMXTextUI::test,                     false,  "Test" );

    initializeAnimationEditors();
}

// ----------------------------------------------------------------------------
//
DMXTextUI::~DMXTextUI(void)
{
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::run()
{
    m_running = true;

    m_text_io.printf( "\n\nDMXStudio text UI - type ? for command list\n" );

    while ( m_running ) {
        CString light_status( "LIGHT | " );
        CString music_status( "SOUND | " );
                
        if ( getVenue()->isRunning() ) {
            light_status.AppendFormat( "Dimmer %d%% | ", getVenue()->getMasterDimmer() );
            if ( getVenue()->isForceBlackout() )
                light_status.AppendFormat( "Blackout ON | " );
            else if ( getVenue()->getWhiteout() != WHITEOUT_OFF ) {
                light_status.Append( "Whiteout " );

                if ( getVenue()->getWhiteout() == WHITEOUT_ON )
                    light_status.Append( "ON " );
                else if ( getVenue()->getWhiteout() == WHITEOUT_STROBE_SLOW )
                    light_status.Append( "STROBE SLOW " );
                else if ( getVenue()->getWhiteout() == WHITEOUT_STROBE_FAST )
                    light_status.Append( "STROBE FAST " );
                else if ( getVenue()->getWhiteout() == WHITEOUT_STROBE_MANUAL )
                    light_status.AppendFormat( "STROBE %dms ", getVenue()->getWhiteoutStrobeMS() );
                else if ( getVenue()->getWhiteout() == WHITEOUT_PULSE )
                    light_status.Append( "PULSE " );

                if ( getVenue()->getWhiteoutColor() != RGBWA::WHITE )
                    light_status.AppendFormat( "COLOR %s ", getVenue()->getWhiteoutColor().getColorName().MakeUpper() ); 

                light_status.Append( "| " );
            }

            if ( getVenue()->getAutoBlackoutMS() != 0 )
                light_status.AppendFormat( "Blackout %lums | ", getVenue()->getAutoBlackoutMS() );
            if ( getVenue()->isChaseRunning() ) {
                Chase* chase = getVenue()->getChase( getVenue()->getRunningChase() );
                light_status.AppendFormat( "Chase #%lu %s | ", chase->getChaseNumber(), chase->getName() );
            }
            light_status.AppendFormat( "Scene #%d %s | ", getVenue()->getScene()->getSceneNumber(), getVenue()->getScene()->getName() );

            Scene* workspace = getVenue()->getDefaultScene();
            if ( workspace->getNumActors() > 0 || workspace->getNumAnimations() > 0 ) {
                light_status.AppendFormat( "Workspace: " );
                for ( SceneActor* actor : workspace->getActors() ) {
                    if ( actor->isGroup() ) {
                        FixtureGroup* group = getVenue()->getFixtureGroup( actor->getActorUID() );
                        if ( group != NULL )
                            light_status.AppendFormat( "G%u ", group->getNumber() );
                    }
                    else {
                        Fixture* pf = getVenue()->getFixture( actor->getActorUID() );
                        if ( pf != NULL )
                            light_status.AppendFormat( "F%u ", pf->getFixtureNumber() );
                    }
                }

                for (AnimationReference& ref : workspace->animations() ) {
                    AnimationDefinition* animation = getVenue()->getAnimation( ref.getUID() );
                    if ( animation != NULL ) {
                        light_status.AppendFormat( "A%u ", animation->getNumber() );
                    }
                }

                light_status.AppendFormat( "| " );
            }

            light_status.AppendFormat( "Anim Speed: %d%% | ", getVenue()->getAnimationSpeed() );
        }
        else {
            light_status.AppendFormat( "Venue STOPPED | " );
        }

        if ( getVenue()->isMasterVolumeMute() )
            music_status.Append( "Volume MUTE" );
        else
            music_status.AppendFormat( "Volume %d%%", getVenue()->getMasterVolume( ) );

        // If we have a music player and it has status, display that
        if ( studio.hasMusicPlayer() && studio.getMusicPlayer()->isLoggedIn() ) {
            PlayingInfo playing_info;

            bool success = studio.getMusicPlayer()->getPlayingTrack( &playing_info );
            
            if ( getVenue()->isMusicSceneSelectEnabled() )
                music_status.AppendFormat( " | Music Match ON" );
            else
                music_status.AppendFormat( " | Music Match OFF" );

            if ( success ) {
                music_status.AppendFormat( " | %s", studio.getMusicPlayer()->getTrackFullName( playing_info.track_link ) );

                if ( playing_info.track_length ) {
                    music_status.AppendFormat( " | length %s", track_time(playing_info.track_length) );

                    if ( playing_info.time_remaining )
                        music_status.AppendFormat( " | remaining %s", track_time(playing_info.time_remaining) );
                }

                if ( studio.getMusicPlayer()->isTrackPaused() )
                    music_status.Append( " | PAUSED" );
            }
        }

        m_text_io.printf( "\n%s\n%s\n\n> ", (LPCSTR)light_status, (LPCSTR)music_status );

        m_text_io.clear();

        CString cmd;
        int retcode = m_text_io.getString( cmd );
        m_text_io.printf( "\n" );
        
        if ( retcode != INPUT_SUCCESS )
            continue;
        m_text_io.tokenize( cmd );
        if ( !m_text_io.nextToken( cmd ) )
            continue;

        cmd.MakeLower();

        m_text_io.printf( "\n" );

        HandlerMap::iterator it = function_map.find( cmd );
        if ( it == function_map.end() ) {
            m_text_io.printf( "Unrecognized command '%s' - Type ? for list of commands\n", (LPCTSTR)cmd );
        }
        else if ( !getVenue()->isRunning() && (*it).second.m_running ) {
            m_text_io.printf( "Venue must be running to use '%s'\n", (*it).second.m_desc );
        }
        else {
            try {
                (this->*(*it).second.m_funcptr)();
            }
            catch ( StudioException& ex ) {
                m_text_io.printf( "\n" );
                DMXStudio::log( ex );
            }
            catch ( std::exception& ex ) {
                m_text_io.printf( "\n" );
                DMXStudio::log( ex );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
static CString track_time( DWORD time )
{
    CString time_str;

    if ( time > 1000*60*60 ) {  // Hours
        time_str.Format( "%d:", time / (1000*60*60) );
        time = time % (1000*60*60);
    }

    time_str.AppendFormat( "%d:", time / (1000*60) );
    time = time % (1000*60);

    time_str.AppendFormat( "%02d", time / (1000) );

    return time_str;
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::muteVolume(void)
{
    BooleanField mute_field( "Mute volume", getVenue()->isMasterVolumeMute() );

    Form form( &m_text_io );
    form.add( mute_field );

    if ( form.play() ) {
        getVenue()->setMasterVolumeMute( mute_field.isSet() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::masterVolume()
{
    class VolumeField : public IntegerField
    {
        Venue*	m_venue;
    public:
        VolumeField( LPCSTR label, Venue* venue, long default_value ) :
            IntegerField( label, default_value, 0, 100 ),
            m_venue( venue )
        {}

        bool setValue( LPCSTR value ) {
            bool result = IntegerField::setValue( value );
            if ( result )
                m_venue->setMasterVolume( getIntValue() );
            return result;
        }
    };

    VolumeField volume_field( "Master volume", getVenue(), getVenue()->getMasterVolume( ) );

    Form form( &m_text_io );
    form.add( volume_field );
    form.play();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::blackout()
{
    getVenue()->setForceBlackout( !getVenue()->isForceBlackout() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::whiteout(void)
{
    NumberedListField whiteout_field( "Whiteout" );

    whiteout_field.addKeyValue( 0, "Off" );
    whiteout_field.addKeyValue( 1, "Strobe Slow" );
    whiteout_field.addKeyValue( 2, "Strobe Fast" );
    whiteout_field.addKeyValue( 3, "Strobe Manual" );
    whiteout_field.addKeyValue( 4, "On" );
    whiteout_field.setDefaultListValue( getVenue()->getWhiteout() );

    Form form( &m_text_io );
    form.add( whiteout_field );

    if ( form.play() && form.isChanged() ) {
        getVenue()->setWhiteout( (WhiteoutMode)(whiteout_field.getListValue()) );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::whiteout_options()
{
    NumberedListField whiteout_field( "Whiteout" );
    IntegerField whiteout_strobe_ms_field( "Strobe MS", getVenue()->getWhiteoutStrobeMS(), 25, 10000 );
    IntegerField whiteout_fade_ms_field( "Fade MS", getVenue()->getWhiteoutFadeMS(), 25, 10000 );
    ColorSelectField whiteout_color_field( "Whiteout Color", getVenue()->getWhiteoutColor() );
    NumberedListField whiteout_effect( "Whiteout Palette Effect" );

    whiteout_field.addKeyValue( 0, "Off" );
    whiteout_field.addKeyValue( 1, "Strobe Slow" );
    whiteout_field.addKeyValue( 2, "Strobe Fast" );
    whiteout_field.addKeyValue( 3, "Strobe Manual" );
    whiteout_field.addKeyValue( 4, "On" );
    whiteout_field.addKeyValue( 5, "Pulse" );
    whiteout_field.setDefaultListValue( getVenue()->getWhiteout() );

    whiteout_effect.addKeyValue( WhiteoutEffect::WHITEOUT_EFFECT_SINGLE_COLOR, "Single Color" );
    whiteout_effect.addKeyValue( WhiteoutEffect::WHITEOUT_EFFECT_MULTI_COLOR, "Multi-Color (palettes only)" );
    whiteout_effect.addKeyValue( WhiteoutEffect::WHITEOUT_EFFECT_SMART_COLOR, "Smart Color (palettes only)" );

    whiteout_effect.setDefaultListValue( getVenue()->getWhiteoutEffect() );

    Form form( &m_text_io );
    form.add( whiteout_field );
    form.add( whiteout_strobe_ms_field );
    form.add( whiteout_fade_ms_field );
    form.add( whiteout_color_field );
    form.add( whiteout_effect );

    if ( form.play() && form.isChanged() ) {
        getVenue()->setWhiteout( (WhiteoutMode)(whiteout_field.getListValue()) );
        getVenue()->setWhiteoutStrobeMS( whiteout_strobe_ms_field.getIntValue(), whiteout_fade_ms_field.getIntValue() );
        getVenue()->setWhiteoutColor( whiteout_color_field.getColor() );
        getVenue()->setWhiteoutEffect( (WhiteoutEffect)whiteout_effect.getListValue() );
    }
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::isUserSure() {
    ConfirmForm form( &m_text_io );
    return form.play();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::autoBlackout()
{
    IntegerField blackout_field( "Venue auto blackout MS", getVenue()->getAutoBlackoutMS(), 0, 65000 );
    Form form( &m_text_io );

    form.add( blackout_field );

    if ( form.play() && form.isChanged() )
        getVenue()->setAutoBlackoutMS( blackout_field.getIntValue() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::resetDefaultFixtures() {
    getVenue()->clearAllCapturedActors();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::soundDebug()
{
    studio.setDebug( !studio.isDebug() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::loadVenue()
{
    InputField filename_field( "Name of venue file to open", studio.getVenueFileName() );
    Form form( &m_text_io );

    form.add( filename_field );

    if ( form.play() ) {
        LPCSTR venue_filename = filename_field.getValue();
        if ( !studio.loadVenueFromFile( venue_filename ) )
            m_text_io.printf( "Cannot open venue file '%s'\n", venue_filename );
        else if ( studio.hasMusicPlayer() && !studio.getMusicPlayer()->isLoggedIn() )
            musicPlayerLogin( );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::writeVenue()
{
    InputField filename_field( "Name of venue file to save", studio.getVenueFileName() );
    Form form( &m_text_io );

    form.add( filename_field );

    if ( form.play() ) {
        LPCSTR venue_filename = filename_field.getValue();
        studio.saveVenueToFile( venue_filename );
        m_text_io.printf( "Venue saved to '%s'\n", venue_filename );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::configVenue()
{
    SelectionList port_selections;
    for ( UINT port_num=1; port_num <= 12; port_num++ ) {
        CString port;
        port.Format( "com%d", port_num );
        port_selections.push_back( port );
    }

    InputField name_field( "Venue name", getVenue()->getName() );
    InputField desc_field( "Venue description", getVenue()->getDescription()  );
    AudioCaptureField audio_capture_field( getVenue()->getAudioCaptureDevice() );
    FloatField audio_scale_field( "Audio boost", getVenue()->getAudioBoost(), "%3.1f" );
    FloatField audio_floor_field( "Audio boost sample floor", getVenue()->getAudioBoostFloor(), "%5.3f" );
    IntegerField audio_sample_size_field( "Audio sample size (multiple of 1024)", getVenue()->getAudioSampleSize(), 1024 );
    BooleanField enable_track_fixtures_field( "Fixture color live tracking (experimental)", getVenue()->isTrackFixtures() );

    SelectionField port_fields[DMX_MAX_UNIVERSES] = {
        SelectionField( "DMX Universe 1: Port", "com1", port_selections ),
        SelectionField( "DMX Universe 2: Port", "com1", port_selections ),
        SelectionField( "DMX Universe 3: Port", "com1", port_selections ),
        SelectionField( "DMX Universe 4: Port", "com1", port_selections )
    };

    NumberedListField driver_fields[DMX_MAX_UNIVERSES] = {
        NumberedListField("DMX Universe 1: Driver" ),
        NumberedListField("DMX Universe 2: Driver" ),
        NumberedListField("DMX Universe 3: Driver" ),
        NumberedListField("DMX Universe 4: Driver" )
    };

    IntegerField dmx_packet_delay_fields[DMX_MAX_UNIVERSES] = {
        IntegerField ( "DMX Universe 1: Packet delay MS", DEFAULT_PACKET_DELAY_MS ),
        IntegerField ( "DMX Universe 2: Packet delay MS", DEFAULT_PACKET_DELAY_MS ),
        IntegerField ( "DMX Universe 3: Packet delay MS", DEFAULT_PACKET_DELAY_MS ),
        IntegerField ( "DMX Universe 4: Packet delay MS", DEFAULT_PACKET_DELAY_MS )
    };

    IntegerField dmx_min_delay_fields[DMX_MAX_UNIVERSES] = {
        IntegerField( "DMX Universe 1: Packet minimum delay MS", DEFAULT_MINIMUM_DELAY_MS ),
        IntegerField( "DMX Universe 2: Packet minimum delay MS", DEFAULT_MINIMUM_DELAY_MS ),
        IntegerField( "DMX Universe 3: Packet minimum delay MS", DEFAULT_MINIMUM_DELAY_MS ),
        IntegerField( "DMX Universe 4: Packet minimum delay MS", DEFAULT_MINIMUM_DELAY_MS )
    };

    Form form( &m_text_io );

    form.add( name_field );
    form.add( desc_field );
    form.add( audio_capture_field );
    form.add( audio_scale_field );
    form.add( audio_floor_field );
    form.add( audio_sample_size_field );
    form.add( enable_track_fixtures_field );


    for (  unsigned id=0; id < DMX_MAX_UNIVERSES; ++id ) {
        driver_fields[id].addKeyValue( 0, "UNUSED" );
        driver_fields[id].addKeyValue( 1, "USB PRO" );
        driver_fields[id].addKeyValue( 2, "OPEN DMX" );
        driver_fields[id].addKeyValue( 3, "PHILIPS HUE" );
        driver_fields[id].setDefaultListValue( 0 );
    }

    UniversePtrArray universes = getVenue()->getUniverses();
    for ( UniversePtrArray::iterator it=universes.begin(); it != universes.end(); ++it ) {
        Universe* universe = (*it);
        port_fields[universe->getId()-1].setValue( universe->getDmxConfig() );
        driver_fields[universe->getId()-1].setDefaultListValue( universe->getType() );
        dmx_packet_delay_fields[universe->getId()-1].setValue( universe->getDmxPacketDelayMS() );
        dmx_min_delay_fields[universe->getId()-1].setValue( universe->getDmxMinimumDelayMS() ); 
    }

    for (  unsigned id=0; id < DMX_MAX_UNIVERSES; ++id ) {
        form.add( driver_fields[id] );
        form.add( port_fields[id] );
        form.add( dmx_packet_delay_fields[id] );
        form.add( dmx_min_delay_fields[id] );
    }

    if ( !form.play() || !form.isChanged() )
        return;

    LPCSTR name = name_field.getValue();
    LPCSTR description = desc_field.getValue();
    LPCSTR audio_capture_device = audio_capture_field.getValue();
    float audio_boost = audio_scale_field.getFloatValue();
    float audio_boost_floor = audio_floor_field.getFloatValue();
    int audio_sample_size = audio_sample_size_field.getIntValue();
    int auto_blackout = getVenue()->getAutoBlackoutMS();
    bool track_fixtures = enable_track_fixtures_field.isSet();

    std::vector<Universe> new_universes;

    for ( universe_t univ_num=0; univ_num < DMX_MAX_UNIVERSES; ++univ_num ) {
        UniverseType type = (UniverseType)(driver_fields[univ_num].getListValue());

        if ( type  == UniverseType::NONE )
            continue;

        new_universes.emplace_back( univ_num+1, 
                type,
                port_fields[univ_num].getValue(), 
                dmx_packet_delay_fields[univ_num].getIntValue(), 
                dmx_min_delay_fields[univ_num].getIntValue() );
    }

    getVenue()->configure( name, description, audio_capture_device, audio_boost, audio_boost_floor, 
                           audio_sample_size, auto_blackout, track_fixtures, new_universes );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::help() {
    for ( HandlerMap::iterator it=function_map.begin(); it != function_map.end(); ++it ) {
        if ( getVenue()->isRunning() || !(*it).second.m_running )
            m_text_io.printf( "%-4s - %s\n", (LPCTSTR)(*it).first, (*it).second.m_desc );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::quit() {
    m_running = !isUserSure();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::soundDetect( )
{
    static const char* level_meter = "====================";

    SoundLevel levels;

    while ( true ) {
        if ( _kbhit() && _getch() == 27 )
            break;

        getVenue()->getSoundLevels( levels );

        unsigned amp = levels.amplitude;
        const char *meter = &level_meter[ 20 - amp/5 ];

        m_text_io.printf( "%4d %s %s                       \r", 
            levels.amplitude, meter, getVenue()->isMute() ? "MUTE" : "     " );

        Sleep( 10 );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::soundBPM( )
{
    BeatDetector detector;
    detector.attach( getVenue()->getAudio() );

    CEvent beat_event;

    detector.addFrequencyEvent( &beat_event );

    while ( true ) {
        if ( _kbhit() && _getch() == 27 )
            break;

        bool beat = ( ::WaitForSingleObject( beat_event.m_hObject, 50 ) == WAIT_OBJECT_0 );

        m_text_io.printf( "%s     \r", (beat) ? "*" : " " );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::soundBeat( )
{
    static IntegerField start_freq__field( "Starting frequency", 150, 0, 128000 );
    static IntegerField end_freq_field( "Ending frequency", 500, 0, 128000 );

    Form form( &m_text_io );

    form.add( start_freq__field );
    form.add( end_freq_field );

    if ( !form.play() )
        return;

    BeatDetector detector( 256 );
    detector.attach( getVenue()->getAudio() );

    CEvent beat_event;

    detector.addFrequencyEvent( &beat_event, start_freq__field.getIntValue(), end_freq_field.getIntValue() );

    while ( true ) {
        if ( _kbhit() && _getch() == 27 )
            break;

        bool beat = ( ::WaitForSingleObject( beat_event.m_hObject, 50 ) == WAIT_OBJECT_0 );

        m_text_io.printf( "%s        \r", (beat) ? " *" : " " );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::soundDb(void) {
    static IntegerField sample_multiplier_field( "Sample Multiplier", 1, 1, 1024 );
    static IntegerField channels_field( "Channels", 1, 1, 2 );
    static AudioCaptureField audio_capture_field( getVenue()->getAudioCaptureDevice() );

    Form form( &m_text_io );
    form.add( sample_multiplier_field );
    form.add( channels_field );
    form.add( audio_capture_field );

    if ( !form.play() )
        return;

    AudioInputStream* audio = AudioInputStream::createAudioStream( audio_capture_field.getValue(), 
                                                            1024 * sample_multiplier_field.getIntValue() );
    try {
        WAVEFORMATEXTENSIBLE pwfx = audio->getFormat();

        m_text_io.printf( "waveformatex.wFormatTag = %x\n", pwfx.Format.wFormatTag );
        m_text_io.printf( "waveformatex.nChannels = %d\n", pwfx.Format.nChannels );
        m_text_io.printf( "waveformatex.nSamplesPerSec =  %d\n", pwfx.Format.nSamplesPerSec );
        m_text_io.printf( "waveformatex.wBitsPerSample =  %d\n", pwfx.Format.wBitsPerSample );
        m_text_io.printf( "waveformatex.nBlockAlign =  %d\n", pwfx.Format.nBlockAlign );
        m_text_io.printf( "waveformatex.nAvgBytesPerSec =  %d\n", pwfx.Format.nAvgBytesPerSec );
        m_text_io.printf( "wValidBitsPerSample=%d\n", pwfx.Samples.wValidBitsPerSample );
        m_text_io.printf( "dwChannelMask=%x\n", pwfx.dwChannelMask );
        m_text_io.printf( "SubFormat=%08lx %04x %04x ", pwfx.SubFormat.Data1, pwfx.SubFormat.Data2, pwfx.SubFormat.Data3 );
        for ( int i=0; i < 8; i++ ) {
            if ( i == 2 )
                m_text_io.printf( " " );
            m_text_io.printf( "%02x", pwfx.SubFormat.Data4[i] );  
        }
        m_text_io.printf( "\n" );

        CEvent event_handler;


        SoundSampler sampler( channels_field.getIntValue(), &event_handler );
        sampler.attach( audio );

        for ( ULONG index=0; true; index++ ) {
            if ( ::WaitForSingleObject( event_handler.m_hObject, 2*60*1000L ) != WAIT_OBJECT_0 )
                break;

            ULONG sample_number;
            SampleSet samples = sampler.getSampleSet( sample_number );

            unsigned amplitude = sampler.getLevel( 32, 100 );

            if ( index % 50 == 0 ) {
                CString label1;
                CString label2;

                for ( int i=0; i < channels_field.getIntValue(); i++ ) {
                    for ( SampleSet::iterator it=samples.begin(); it != samples.end(); ++it ) {
                        label1.AppendFormat( "%7d", (*it).getChannel() );
                        label2.AppendFormat( "%7.0f",(*it).getFrequency() );
                    }
                }
                m_text_io.printf( "\nChannel:  %s\n", (LPCTSTR)label1 );
                m_text_io.printf( "Hz:       %s\n\n",  (LPCTSTR)label2 );
            }

            m_text_io.printf( "%06lu dB:", sample_number );
            for ( SampleSet::iterator it=samples.begin(); it != samples.end(); ++it ) {
                m_text_io.printf( "% 7d", (*it).getDB() );
            }

            m_text_io.printf( " %5d\n", amplitude );

            if ( _kbhit() && _getch() == 27 )
                break;
        }

        sampler.detach();
    }
    catch ( ... ) {
        AudioInputStream::releaseAudioStream( audio );
        throw;
    }

    AudioInputStream::releaseAudioStream( audio );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createChase(void) {
    UniqueNumberField<ChaseNumber, Chase> 
        chase_number_field( "Create chase number", getVenue(), &Venue::getChaseByNumber, getVenue()->nextAvailableChaseNumber() );

    InputField name_field( "Chase name", "New chase" );
    InputField description_field( "Chase description", "" );
    ActsField acts_field;
    IntegerField chase_delay( "Chase delay (ms)", 0, 0 );
    IntegerField chase_fade( "Chase fade (ms)", 0, 0 );
    BooleanField repeat_field( "Repeat scene steps", true );
    NumberedListField trigger_field( "Chase advance trigger" );

    trigger_field.addKeyValue( ChaseStepTrigger::CT_MANUAL, "Manual (user controlled)" );
    trigger_field.addKeyValue( ChaseStepTrigger::CT_TIMER, "Timer" );

    Form form( &m_text_io );
    form.add( chase_number_field );
    form.add( name_field );
    form.add( description_field );
    form.add( acts_field );
    form.add( chase_delay );
    form.add( chase_fade );
    form.add( repeat_field );
    form.add( trigger_field );

    if ( !form.play() )
        return;

    Chase chase( NOUID, 
                 chase_number_field.getNumber(),
                 chase_delay.getLongValue(),
                 chase_fade.getLongValue(),
                 name_field.getValue(),
                 description_field.getValue(),
                 repeat_field.isSet(),
                 (ChaseStepTrigger)trigger_field.getListValue() );

    chase.setActs( acts_field.getActs() );

    m_text_io.printf( "\nAdd chase scene steps (select scene 0 to finish)\n\n" );

    if ( editChaseSteps( &chase, true, 0 ) )
        getVenue()->addChase( chase );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateChase(void) {
    class MyForm : public Form {
        Venue*				m_venue;
        UniqueNumberField<ChaseNumber, Chase> m_number_field;
        ChaseSelectField	m_chase_field;
        InputField			m_name_field;
        InputField			m_description_field;
        IntegerField		m_chase_delay;
        IntegerField		m_chase_fade;
        ActsField           m_acts_field;
        BooleanField        m_repeat_field;
        NumberedListField   m_trigger_field;
                
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                Chase* chase = getChase();

                m_number_field.setNumber( chase->getChaseNumber() );
                m_name_field.setValue( chase->getName() );
                m_description_field.setValue( chase->getDescription() );
                m_acts_field.setActs( chase->getActs() );
                m_chase_delay.setValue( chase->getDelayMS() );
                m_chase_fade.setValue( chase->getFadeMS() );
                m_repeat_field.setValue( chase->isRepeat() );
                m_trigger_field.setDefaultListValue( chase->getStepTrigger() );
            }
            else if ( field_num == 3 ) {
                getChase()->setDelayMS( m_chase_delay.getLongValue() );
            }
            else if ( field_num == 4 ) {
                getChase()->setFadeMS(  m_chase_delay.getLongValue() );
            }
        }

        void formCompleteNotify() {
            Chase* chase = getChase();
            chase->setChaseNumber( m_number_field.getNumber() );
            chase->setName( m_name_field.getValue() );
            chase->setDescription( m_description_field.getValue() );
            chase->setActs( m_acts_field.getActs() );
            chase->setDelayMS( m_chase_delay.getLongValue() );
            chase->setFadeMS( m_chase_fade.getLongValue() );
            chase->setRepeat( m_repeat_field.isSet() );
            chase->setStepTrigger( (ChaseStepTrigger)m_trigger_field.getListValue() );
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream ),
            m_venue( venue ),
            m_number_field( "Chase number", m_venue, &Venue::getChaseByNumber ),
            m_chase_field( "Select chase to update", m_venue ),
            m_name_field( "Chase name", "" ),
            m_description_field( "Chase description", "" ),
            m_chase_delay( "Chase default duration (ms)", 0, 0 ),
            m_chase_fade( "Chase fade (ms)", 0, 0  ),
            m_repeat_field( "Repeat scene steps", true ),
            m_trigger_field( "Chase advance trigger" )
        {
            m_trigger_field.addKeyValue( ChaseStepTrigger::CT_MANUAL, "Manual (user controlled)" );
            m_trigger_field.addKeyValue( ChaseStepTrigger::CT_TIMER, "Timer" );

            add( m_chase_field );
            add( m_name_field );
            add( m_description_field );
            add( m_acts_field );
            add( m_chase_delay );
            add( m_chase_fade );
            add( m_repeat_field );
            add( m_trigger_field );
        }

        Chase* getChase() const {
            return m_venue->getChase( m_chase_field.getChaseUID() );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;
    
    m_text_io.printf( "\nEdit chase scene steps\n\n" );

    editChaseSteps( form.getChase(), false, 0 );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::addChaseSteps(void) {
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                ChaseSelectField* chase_field = getField<ChaseSelectField>( field_num );
                IntegerField* position_field = getField<IntegerField>( 1 );
                Chase* chase = m_venue->getChase( chase_field->getChaseUID() );
                position_field->setValue( chase->getNumSteps()+1 );
                position_field->setHigh( chase->getNumSteps()+1 );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) : 
            Form( input_stream ),
            m_venue(venue) {}
    };

    ChaseSelectField chase_field( "Add steps to chase", getVenue() );
    IntegerField position_field( "Insert at step", 1, 1, 1 );

    MyForm form( &m_text_io, getVenue() );
    form.add( chase_field );
    form.add( position_field );
    if ( !form.play() )
        return;

    m_text_io.printf( "\nAdd chase scene steps (select scene 0 to finish)\n\n" );

    Chase dummy( 0, 0, 0, 0, "", "", false, ChaseStepTrigger::CT_TIMER );
    UINT position = position_field.getIntValue()-1;

    if ( editChaseSteps( &dummy, true, position ) ) {
        Chase* chase = getVenue()->getChase( chase_field.getChaseUID() );

        UID running_chase_id = getVenue()->getRunningChase();
        if ( running_chase_id == chase->getUID() )
            studio.getVenue()->stopChase();

        for ( size_t i=0; i < dummy.getNumSteps(); i++ )
            chase->insertStep( position+i, *dummy.getStep(i) );

        getVenue()->chaseUpdated( chase->getUID() );

        if ( running_chase_id == chase->getUID() )
            getVenue()->startChase( chase->getUID() );
    }
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::editChaseSteps( Chase* chase, bool append_steps, UINT step_num_offset ) {
    static const UINT CHASE_FIELDS = 3;              // Fields per chase step


    class MyForm : public Form {
        Venue*		m_venue;
        Chase*		m_chase;
        bool		m_append_steps;
        UINT		m_step_num_offset;
        
        void fieldLeaveNotify( size_t field_num ) {
            if ( m_append_steps ) {
                if ( field_num == size()-1 ) {
                    SceneSelectField* scene_field = getField<SceneSelectField>( size()-CHASE_FIELDS );
                    if ( scene_field->getSceneUID() != 0 )
                        addStep( size()/CHASE_FIELDS+1, &ChaseStep( 0, 0, SLM_LOAD ) );
                }
                else if ( field_num == size()-CHASE_FIELDS ) {
                    SceneSelectField* scene_field = getField<SceneSelectField>( size()-CHASE_FIELDS );
                    if ( scene_field->getListValue() == 0 )
                        stop();
                }
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue, Chase* chase, bool append_steps, UINT step_num_offset ) :
            Form( input_stream ),
            m_venue( venue ),
            m_chase( chase ),
            m_append_steps( append_steps ),
            m_step_num_offset( step_num_offset )
        {
            setAutoDelete( true );

            if ( m_append_steps )
                addStep( size()/CHASE_FIELDS+1, &ChaseStep( 0, 0, SLM_LOAD ) );
        }

        void addStep( UINT step_number, ChaseStep* step ) {
            CString label;
            
            label.Format( "Step %d scene", step_number+m_step_num_offset );
            SceneNumber scene_number = step->getSceneUID() != 0 ? m_venue->getScene( step->getSceneUID() )->getSceneNumber() : 0;
            addAuto( new SceneSelectField( label, m_venue, scene_number, m_append_steps ) );
            
            label.Format( "Step %d duration (ms)", step_number+m_step_num_offset );
            addAuto( new IntegerField( label, step->getDelayMS(), 0 ) );
            
            label.Format( "Step %d method", step_number+m_step_num_offset );
            NumberedListField* method_field = new NumberedListField( label );

            method_field->addKeyValue( SLM_LOAD, sceneLoadMethods[SLM_LOAD] );

            if ( step_number > 1 ) {
                method_field->addKeyValue( SLM_ADD, sceneLoadMethods[SLM_ADD] );
                method_field->addKeyValue( SLM_MINUS, sceneLoadMethods[SLM_MINUS] );
            }

            method_field->setDefaultListValue( step_number == 1 ? SLM_LOAD : step->getMethod() );

            addAuto( method_field );
        }

        size_t getNumSteps( ) const {
            size_t steps = size()/3;
            if ( m_append_steps )			// If appending always have one extra at the end
                steps--;
            return steps;
        }

    };

    MyForm form( &m_text_io, getVenue(), chase, append_steps, step_num_offset );

    for ( unsigned index=0; index < chase->getNumSteps(); index++ )
        form.addStep( index+1, chase->getStep( index ) );

    if ( !form.play() )
        return false;

    size_t chase_steps = form.getNumSteps();

    for ( UINT step_num=0; step_num < chase_steps; step_num++ ) {
        SceneSelectField* scene_field = form.getField<SceneSelectField>( step_num*CHASE_FIELDS );
        IntegerField* delay_field = form.getField<IntegerField>( step_num*CHASE_FIELDS+1 );
        NumberedListField* method_field = form.getField<NumberedListField>( step_num*CHASE_FIELDS+2 );

        if ( step_num < chase->getNumSteps() ) {
            ChaseStep* step = chase->getStep( step_num );
            step->setSceneUID( scene_field->getSceneUID() );
            step->setDelayMS( delay_field->getLongValue() );
            step->setMethod( (SceneLoadMethod)method_field->getListValue() );
        }
        else
            chase->appendStep( ChaseStep( scene_field->getSceneUID(), delay_field->getLongValue(), (SceneLoadMethod)method_field->getListValue() ) );
    }

    getVenue()->chaseUpdated( chase->getUID() );

    return true;
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::selectChase(void) {
    ChaseSelectField chase_field( "Chase to run or stop", getVenue() );

    Form form( &m_text_io );
    form.add( chase_field );
    if ( !form.play() )
        return;

    if ( getVenue()->getRunningChase() == chase_field.getChaseUID() )
        getVenue()->stopChase();
    else
        getVenue()->startChase( chase_field.getChaseUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteChase(void) {
    ChaseSelectField chase_field( "Chase to delete", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( chase_field );
    if ( !form.play() )
        return;

    getVenue()->deleteChase( chase_field.getChaseUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::advanceChase() {
    IntegerField steps_field( "Chase steps", 1, -1, 1 );

    Form form( &m_text_io );
    form.add( steps_field );
    if ( !form.play() )
        return;

    getVenue()->chaseStep( steps_field.getIntValue() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describeChase(void) {
    ChaseSelectField chase_field( "Chase to describe", getVenue() );

    Form form( &m_text_io );
    form.add( chase_field );
    if ( !form.play() )
        return;

    Chase *chase = getVenue()->getChase( chase_field.getChaseUID() );
    m_text_io.printf( "\nChase #%d: %s Fade=%lums %s", 
        chase->getChaseNumber(), chase->getName(), chase->getFadeMS(),
        chase->isRepeat() ? "Repeating" : "Non-repeating" );

    switch ( chase->getStepTrigger() ) {
        case CT_TIMER:
            m_text_io.printf( " Step Duration=%lums" , chase->getDelayMS() );
            break;

        case CT_MANUAL:
            m_text_io.printf( " Manual Advance" );
            break;
    }

    m_text_io.printf( "\n\n  Acts: " );
    if ( !chase->getActs().empty() ) {
        for ( ActNumber act : chase->getActs() )
            m_text_io.printf( "%u ", act );
    }
    else
        m_text_io.printf( "Default Act" );
    m_text_io.printf( "\n\n" );


    for ( unsigned index=0; index < chase->getNumSteps(); index++ ) {
        ChaseStep* step = chase->getStep( index );
        m_text_io.printf( "  Step %2d: %s scene %lu %s", index+1,
                sceneLoadMethods[ (int)step->getMethod() ],
                getVenue()->getScene( step->getSceneUID() )->getSceneNumber(),
                getVenue()->getScene( step->getSceneUID() )->getName() );

        if ( chase->getStepTrigger() == CT_TIMER && step->getDelayMS() > 0 )
            m_text_io.printf( "  Duration %lums", index+1 );

        m_text_io.printf( "\n" );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::copyChaseSteps(void) {
    ChaseSelectField chase_source_field( "Copy steps from chase", getVenue() );
    ChaseSelectField chase_target_field( "Copy steps to chase", getVenue() );

    Form form( &m_text_io );
    form.add( chase_source_field );
    form.add( chase_target_field );
    if ( !form.play() )
        return;

    if ( getVenue()->getRunningChase() == chase_target_field.getChaseUID() ) {
        m_text_io.printf( "Stop chase before editing.\n" );
        return;
    }

    // We specifically allow steps to be copied from the same chase
    getVenue()->copyChaseSteps( chase_source_field.getChaseUID(), 
                             chase_target_field.getChaseUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteChaseSteps(void) {
    ChaseSelectField chase_field( "Delete steps from chase", getVenue() );

    Form form( &m_text_io );
    form.add( chase_field );
    if ( !form.play() )
        return;

    if ( getVenue()->getRunningChase() == chase_field.getChaseUID() ) {
        m_text_io.printf( "Stop chase before editing.\n" );
        return;
    }

    Chase* chase = getVenue()->getChase( chase_field.getChaseUID() );

    form.setTitle( "Choose steps to delete" );
    form.clear();
    form.setAutoDelete( true );

    for (unsigned step_number=0; step_number < chase->getNumSteps(); step_number++ ) {
        ChaseStep* step = chase->getStep( step_number );

        CString label;
        label.Format( "Remove step %2d: Scene %s (delay %lu)", step_number+1, 
                    getVenue()->getScene( step->getSceneUID() )->getName(), step->getDelayMS() );

        form.addAuto( new BooleanField( (LPCSTR)label, false ) );
    }

    if ( !form.play() )
        return;

    for ( unsigned step_number=chase->getNumSteps(); step_number-- > 0; )
        if ( form.getField<BooleanField>( step_number )->isSet() )
            chase->deleteStep( step_number );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::masterDimmer(void) {
    IntegerField master_dimmer_field( "Master dimmer %", getVenue()->getMasterDimmer(), 0, 100 );

    Form form( &m_text_io );
    form.add( master_dimmer_field );
    if ( !form.play() )
        return;

    if ( form.isChanged() ) {
        getVenue()->setMasterDimmer( (BYTE)master_dimmer_field.getIntValue() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::animationSpeed(void) {
    IntegerField animation_speed_field( "Animation Speed %", getVenue()->getAnimationSpeed(), 25, 2000 );

    Form form( &m_text_io );
    form.add( animation_speed_field );
    if ( !form.play() )
        return;

    if ( form.isChanged() ) {
        getVenue()->setAnimationSpeed( animation_speed_field.getIntValue() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describeFixture(void) {
    ActorSelectField fixture_field( "Fixture to describe", getVenue(), false );

    Form form( &m_text_io );
    form.add( fixture_field );
    if ( !form.play() )
        return;

    SceneActor actor = fixture_field.getActor( );
    if ( !actor.isGroup() ) { 
        Fixture* pf = getVenue()->getFixture( actor.getActorUID() );
        m_text_io.printf( "%2d: %s @ %d - %d", pf->getFixtureNumber(), pf->getFullName(), pf->getUniverseId(), pf->getAddress() );

        if ( pf->canWhiteout() && pf->getAllowWhiteout() )
            m_text_io.printf( ", WHITEOUT" );

        if ( pf->hasDimmer() && pf->getAllowMasterDimming() )
            m_text_io.printf( ", DIM" );

        if ( pf->hasDimmer() && pf->getAllowMasterDimming() )
            m_text_io.printf( ", ID=%u", pf->getUID() );

        m_text_io.printf( "\n" );
        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            m_text_io.printf( "   %d->%d @ %d - %s\n", channel+1, pf->getChannel(channel)->getOffset()+1, pf->getChannelAddress( channel ), cp->getName() );

            ChannelValueRangeArray ranges = cp->getRanges();
            for ( ChannelValueRangeArray::iterator rit=ranges.begin(); rit != ranges.end(); rit++ ) {
                m_text_io.printf( "      %03d-%03d %s\n", (*rit).getStart(), (*rit).getEnd(), (*rit).getName() );
            }
        }
    }
    else
        throw FieldException( "Unexpected group in fixture list" );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::controlFixture( ) {
    ActorSelectField fixture_field( "Fixture to control", getVenue(), true );

    Form form( &m_text_io );
    form.add( fixture_field );
    if ( !form.play() )
        return;

	channel_address channel = 0;
    size_t num_channels = 0;

    SceneActor actor = fixture_field.getActor();
    CString title;
    
    for ( Fixture* pf : getVenue()->resolveActorFixtures( &actor ) ) {
        if ( title.GetLength() != 0 )
            title.Append( "\n" );
        title.AppendFormat( "Channels: %s @ %d", pf->getFullName(), pf->getAddress() );
        num_channels = std::max<size_t>( num_channels, pf->getNumChannels() );
    }

    form.clear();
    form.setTitle( (LPCSTR)title );
    form.setAutoDelete( true );
    form.setMoveAfterChange( false );

    // Generate channel control fields
    for ( channel=0; channel < num_channels; channel++ )
        form.addAuto( new ChannelControllerField( getVenue(), actor, channel ) );

    form.play();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::selectScene( ) {
    SceneSelectField scene_field( "Scene to load", getVenue() );

    Form form( &m_text_io );
    form.add( scene_field );
    if ( form.play() )
        getVenue()->selectScene( scene_field.getSceneUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteScene( ) {
    SceneSelectField scene_field( "Scene to delete", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( scene_field );
    if ( form.play() )
        getVenue()->deleteScene( scene_field.getSceneUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describeScene( ) {
    SceneSelectField scene_field( "Scene to describe", getVenue() );

    Form form( &m_text_io );
    form.add( scene_field );

    if ( form.play() ) {
        Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );

        m_text_io.printf( "\nScene #%lu - %s [%s]\n", 
            scene->getSceneNumber(), scene->getName(), getRatingName(scene->getBPMRating()) );

        if ( scene->getNumActors() == 0 )
            m_text_io.printf( "\n   NO FIXTURES\n" );

        for ( SceneActor* actor : scene->getActors() ) {
            Fixture* fixture = NULL;

            if ( actor->isGroup() ) {
                FixtureGroup * group = getVenue()->getFixtureGroup( actor->getActorUID() );

                m_text_io.printf( " Group %d %s UID=%lu\n", group->getNumber(), group->getName(), group->getUID() );
                fixture = getVenue()->getGroupRepresentative( group->getUID() );
            }
            else {
                fixture = getVenue()->getFixture( actor->getActorUID() );

                m_text_io.printf( " %3d %s @ %u UID=%lu\n", fixture->getFixtureNumber(), fixture->getFullName(), 
                                                           fixture->getChannelAddress(0), fixture->getUID() );
            }

            if ( fixture != NULL ) {
                if ( actor->hasPaletteReferences() ) {
                    for ( UID palette_id : actor->getPaletteReferences() ) {
                        Palette palette;
                        if ( getVenue()->getPalette( palette_id, palette ) )
                            m_text_io.printf( "      Palette: %s\n", palette.getName() );
                    }
                }

                for ( size_t channel=0; channel < fixture->getNumChannels(); channel++ ) {
                    m_text_io.printf( "      %2u - %s [%d]\n", 
                        channel+1,
                        fixture->getChannel( channel )->getName(),
                        actor->getBaseChannelValue( channel ) );
                }
            }
        }

        m_text_io.printf( "  Acts:\n      " );
        if ( !scene->getActs().empty() ) {
            for ( ActNumber act : scene->getActs() )
                m_text_io.printf( "%u ", act );
        }
        else
            m_text_io.printf( "Default Act" );
        m_text_io.printf( "\n" );

        for ( AnimationReference& animation_ref : scene->animations() ) {
            AnimationDefinition* animation = getVenue()->getAnimation( animation_ref.getUID() );
            STUDIO_ASSERT( animation, "Unable to find animation UID %lu in scene %s", animation_ref.getUID(), scene->getName() );

            if ( animation->isShared() )
                m_text_io.printf( "  Animation %d: %s (ID %d)\n", animation->getNumber(), animation->getName(), animation->getUID());
            else
                m_text_io.printf( "  Private Animation: %s (ID %d)\n", animation->getName(), animation->getUID());

            CString animimation_synopsis = animation->getSynopsis();
            if ( animimation_synopsis.GetLength() > 0 ) {
                if ( animimation_synopsis[animimation_synopsis.GetLength()-1] == '\n' )
                    animimation_synopsis = animimation_synopsis.Mid( 0, animimation_synopsis.GetLength()-1 );
                animimation_synopsis.Replace( "\n", "\n      " );
                m_text_io.printf( "      %s\n", (LPCTSTR)animimation_synopsis );
            }

            m_text_io.printf( "      Signal( %s )\n", (LPCTSTR)animation->signal().getSynopsis() );

            CString fixture_list;

            for ( UID actor : animation_ref.getActors() ) {
                Fixture *pf = studio.getVenue()->getFixture( actor );
                if ( pf != NULL )
                    fixture_list.AppendFormat( "F%lu ", pf->getNumber() );
                else {
                    FixtureGroup* group = studio.getVenue()->getFixtureGroup( actor );
                    if ( group != NULL )
                        fixture_list.AppendFormat( "G%lu ", group->getNumber() );
                }
            }

            m_text_io.printf( "      Fixtures( %s )\n", (LPCTSTR)fixture_list );
        }
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateScene( ) {
    SceneSelectField scene_field( "Scene to update", getVenue() );

    Form form( &m_text_io );
    form.add( scene_field );

    if ( form.play() ) {
        Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );

        if ( scene->getSceneNumber() != DEFAULT_SCENE_NUMBER ) {
            UniqueNumberField<SceneNumber, Scene> 
                scene_number_field( "Scene number", getVenue(), &Venue::getSceneByNumber, scene->getSceneNumber() );
            InputField name_field( "Scene name", scene->getName() );
            InputField description_field( "Scene description", scene->getDescription() );
            ActsField acts_field( scene->getActs() );
            BPMRatingField bpm_field( scene->getBPMRating() );

            form.clear();
            form.add( scene_number_field );
            form.add( name_field );
            form.add( description_field );
            form.add( acts_field );
            form.add( bpm_field );

            if ( form.play() ) {
            scene->setSceneNumber( scene_number_field.getNumber() );
                scene->setName( name_field.getValue() );
                scene->setDescription( description_field.getValue() );
                scene->setActs( acts_field.getActs() );
                scene->setBPMRating( bpm_field.getRating() );
                getVenue()->sceneUpdated( scene->getUID() );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::copyWorkspaceToScene()
{
    if ( getVenue()->isChaseRunning() ) {
        m_text_io.printf( "Stop all chases before updating scene\n" );
        return;
    }

    SceneSelectField scene_field( "Copy workspace fixtures to scene", getVenue() );
    BooleanField remove_from_ws_field( "Remove from workspace", true );
    BooleanField keep_groups( "Preserve fixture groups", true );
    BooleanField keep_animations( "Include current animations", true );

    Form form( &m_text_io );
    form.add( scene_field );
    form.add( keep_groups );
    form.add( keep_animations );
    form.add( remove_from_ws_field );

    if ( form.play() ) {
        getVenue()->moveDefaultFixturesToScene( scene_field.getSceneUID(), 
                                                keep_groups.isSet(),
                                                remove_from_ws_field.isSet(),
                                                keep_animations.isSet() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createScene( ) {
    UniqueNumberField<SceneNumber, Scene> 
        scene_number_field( "Create scene number", getVenue(), &Venue::getSceneByNumber, getVenue()->nextAvailableSceneNumber() );

    InputField name_field( "Scene name", "New scene" );
    InputField description_field( "Scene description", "" );
    BooleanField copy_field( "Copy current fixtures", true );
    BooleanField keep_animations( "Include current animations", true );
    ActsField acts_field;
    BooleanField keep_groups( "Preserve fixture groups", true );
    BPMRatingField bpm_field( BPM_MEDIUM );

    Form form( &m_text_io );
    form.add( scene_number_field );
    form.add( name_field );
    form.add( description_field );
    form.add( copy_field );
    form.add( acts_field );
    form.add( bpm_field );
    form.add( keep_groups );
    form.add( keep_animations );

    if ( !form.play() )
        return;

    Scene scene( NOUID, scene_number_field.getNumber(),
                 name_field.getValue(), description_field.getValue() );
    scene.setActs( acts_field.getActs() );
    scene.setBPMRating( bpm_field.getRating() );

    getVenue()->addScene( scene );

    if ( copy_field.isSet() )
        getVenue()->moveDefaultFixturesToScene( scene.getUID(), keep_groups.isSet(), true, keep_animations.isSet() );

    getVenue()->clearAllCapturedActors();
    getVenue()->selectScene( scene.getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::copyScene(void) {
    SceneSelectField scene_field( "Scene to copy", getVenue() );

    UniqueNumberField<SceneNumber, Scene> 
        scene_number_field( "New scene number", getVenue(), &Venue::getSceneByNumber, getVenue()->nextAvailableSceneNumber() );

    InputField name_field( "New scene name", "New scene" );
    InputField description_field( "New scene description", "" );

    Form form( &m_text_io );
    form.add( scene_field );
    form.add( scene_number_field );
    form.add( name_field );
    form.add( description_field );

    if ( !form.play() )
        return;

    Scene new_scene( *getVenue()->getScene( scene_field.getSceneUID() ) );
        
    new_scene.setSceneNumber( scene_number_field.getNumber() );
    new_scene.setName( name_field.getValue() );
    new_scene.setDescription( description_field.getValue() );
    new_scene.setUID( NOUID );

    AnimationReferenceArray animation_refs = new_scene.animations();

    // Re-add animation references to the scene and handle private animations
    getVenue()->setSceneAnimationReferences( &new_scene, animation_refs, true );

    UID new_scene_uid = getVenue()->addScene( new_scene );

    // Compute final channel values for all actors
    getVenue()->computeActorFinalValues( getVenue()->getScene( new_scene_uid ) );

    getVenue()->selectScene( new_scene.getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::copyFixture(void) {
    class MyForm : public Form
    {
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( field_num );
                ActorSelectField* scene_fixture_field = getField<ActorSelectField>( 1 );
                scene_fixture_field->selectScene( scene_field->getSceneUID() );
            }
        }

    public:
        MyForm( TextIO* input_stream) : Form( input_stream ) {}
    };

    SceneSelectField scene_select_field( "Scene to copy from", getVenue(), 0, true );
    ActorSelectField scene_fixture_field( "Scene fixture to copy", getVenue() );
    scene_fixture_field.allowMultiple( true );

    MyForm form( &m_text_io );
    form.add( scene_select_field );
    form.add( scene_fixture_field );
    if ( !form.play() )
        return;

    getVenue()->copySceneFixtureToDefault( scene_select_field.getSceneUID(),
                                        scene_fixture_field.getActorUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteFixtureFromScene(void) {

    class MyForm : public Form
    {
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( field_num );
                ActorSelectField* scene_fixture_field = getField<ActorSelectField>( 1 );
                scene_fixture_field->selectScene( scene_field->getSceneUID() );
            }
        }

    public:
        MyForm( TextIO* input_stream) : Form( input_stream ) {}
    };

    SceneSelectField scene_select_field( "Delete fixture from scene", getVenue(), 0, true );
    ActorSelectField scene_fixture_field( "Delete scene fixture", getVenue() );

    MyForm form( &m_text_io );
    form.add( scene_select_field );
    form.add( scene_fixture_field );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_select_field.getSceneUID() );
    scene->removeActor( scene_fixture_field.getActorUID() );

    getVenue()->sceneUpdated( scene->getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describeFixtureGroup( ) {
    FixtureGroupSelectField group_select_field( "Fixture group to describe", getVenue() );

    Form form( &m_text_io );
    form.add( group_select_field );

    if ( !form.play() )
        return;

    FixtureGroup *group = getVenue()->getFixtureGroup( group_select_field.getFixtureGroupUID() );
    m_text_io.printf( "\nFixture Group #%u: %s (ID=%lu)\n\n", group->getGroupNumber(), group->getName(), group->getUID() );

    UIDSet uids = group->getFixtures( );
    for ( UIDSet::iterator it=uids.begin(); it != uids.end(); ++it ) {
        Fixture* pf = getVenue()->getFixture( (*it) );

        m_text_io.printf( "%s @ %d\n", pf->getFullName(), pf->getAddress() );
    }

    if ( group->getNumChannelValues() > 0 ) {
        Fixture* pf = getVenue()->getGroupRepresentative( group->getUID() );
        if ( pf ) {
            m_text_io.printf( "\nDefault channel values:\n" );

            for ( size_t i=0; i < pf->getNumChannels(); i++ ) {
                Channel* ch = pf->getChannel( i );
                BYTE value = group->getChannelValue( i );
                ChannelValueRange* range = ch->getRange( value );

                m_text_io.printf( "%3u - %s [%u] %s\n", i, ch->getName(), value, (range != NULL) ? range->getName() : "" );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteFixtureGroup( ) {
    FixtureGroupSelectField group_select_field( "Fixture group to delete", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( group_select_field );
    if ( form.play() )
        getVenue()->deleteFixtureGroup( group_select_field.getFixtureGroupUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteFixture(void) {
    ActorSelectField fixture_select_field( "Fixture to delete", getVenue(), false );

    ConfirmForm form( &m_text_io );
    form.add( fixture_select_field );
    if ( form.play() ) {
            getVenue()->deleteFixture( fixture_select_field.getActorUID() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateFixture(void) {
    class MyForm : public Form
    {
    public:
        ActorSelectField fixture_select_field;
        UniqueNumberField<FixtureNumber, Fixture> fixture_number_field;
        InputField name_field;
        InputField description_field;
        BooleanField address_overlap_field;
        DmxAddressField dmx_address_field;
        DmxUniverseField dmx_universe;
        BooleanField allow_whiteout_field;
        BooleanField allow_dimming_field;
        
        Venue* m_venue;
        Fixture* m_fixture;

        void fieldLeaveNotify( size_t field_num ) {
           if ( getField<Field>( field_num ) == &fixture_select_field ) {
                m_fixture = m_venue->getFixture( fixture_select_field.getActorUID() );

                dmx_address_field.setFixture( m_fixture );
                dmx_universe.setUniverse( m_fixture->getUniverseId() );

                name_field.setValue( m_fixture->getName() );
                fixture_number_field.setNumber( m_fixture->getFixtureNumber() );
                description_field.setValue( m_fixture->getDescription() );
                allow_whiteout_field.setValue( m_fixture->getAllowWhiteout() );
                allow_dimming_field.setValue( m_fixture->getAllowMasterDimming() );

                allow_whiteout_field.setHidden( !m_fixture->canWhiteout() );
                allow_dimming_field.setHidden( !m_fixture->hasDimmer() );
           }
           else if ( getField<Field>( field_num ) == &dmx_universe ) {
                if ( dmx_universe.getUniverseId() != m_fixture->getUniverseId() ) {
					channel_address base_address = m_venue->findFreeAddressRange( dmx_universe.getUniverseId(), m_fixture->getNumChannels() ); 
                    if ( base_address == INVALID_CHANNEL )
                        throw FieldException( "There are no empty address ranges available for %d channels", m_fixture->getNumChannels() );

                    dmx_address_field.setUniverseId( dmx_universe.getUniverseId() );
                    dmx_address_field.setInitialValue( base_address ); 
                }
            }
            else if ( getField<Field>( field_num ) == &address_overlap_field ) {
                dmx_address_field.setAllowAddressOverlap( address_overlap_field.isSet() );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream, "Update Fixture" ),
            m_venue(venue),
            fixture_number_field( "Fixture number", venue, &Venue::getFixtureByNumber ),
            fixture_select_field( "Fixture to update", venue, false ),
            name_field( "Fixture location", "Somewhere" ),
            description_field( "Fixture description", "" ),
            address_overlap_field( "Allow DMX addresses to overlap", false ),  
            dmx_address_field( "DMX base address", venue, NULL ),
            dmx_universe( "DMX universe", venue, 0 ),
            allow_whiteout_field( "Whiteout affects this fixture", true ),
            allow_dimming_field( "Master dimmer affects this fixture", true )
        {
            add( fixture_select_field ),
            add( fixture_number_field ),
            add( name_field );
            add( description_field );
            add( dmx_universe );
            add( address_overlap_field );
            add( dmx_address_field );
            add( allow_whiteout_field );
            add( allow_dimming_field );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    form.m_fixture->setName( form.name_field.getValue() );
    form.m_fixture->setDescription( form.description_field.getValue() );
    form.m_fixture->setAddress( form.dmx_address_field.getIntValue() );
    form.m_fixture->setUniverseId( form.dmx_universe.getUniverseId() );
    form.m_fixture->setAllowMasterDimming( form.allow_dimming_field.isSet() );
    form.m_fixture->setAllowWhiteout( form.allow_whiteout_field.isSet() );

    getVenue()->fixtureUpdated( form.m_fixture->getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createFixture(void)
{
    class MyForm : public Form
    {
    public:
        FixtureManufacturerSelectField manufacturer_field;
        FixtureModelSelectField model_field;
        FixturePersonalitySelectField personality_field;
        UniqueNumberField<FixtureNumber, Fixture> fixture_number_field;
        InputField name_field;
        InputField description_field;
        BooleanField address_overlap_field;
        DmxAddressField dmx_address_field;
        DmxUniverseField dmx_universe;
        BooleanField allow_whiteout_field;
        BooleanField allow_dimming_field;

        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( getField<Field>( field_num ) == &manufacturer_field ) {
                model_field.setManufacturer( manufacturer_field.getKeyValue() );
            }
            else if ( getField<Field>( field_num ) == &model_field  ) {
                personality_field.setManuAndModel( manufacturer_field.getKeyValue(), model_field.getKeyValue() );
            }
            else if ( getField<Field>( field_num ) == &dmx_universe ) {
                FixtureDefinition* fixdef = 
                    &FixtureDefinition::FixtureDefinitions[personality_field.getFUID()];

				channel_address base_address = m_venue->findFreeAddressRange( dmx_universe.getUniverseId(), fixdef->getNumChannels() ); 
                if ( base_address == INVALID_CHANNEL )
                    throw FieldException( "There are no empty address ranges available for %d channels", fixdef->getNumChannels() );

                dmx_address_field.setNumChannels( fixdef->getNumChannels() );
                dmx_address_field.setUniverseId( dmx_universe.getUniverseId() );
                dmx_address_field.setInitialValue( base_address ); 

                allow_whiteout_field.setHidden( !fixdef->canWhiteout() );
                allow_dimming_field.setHidden( !fixdef->hasDimmer() );
            }
            else if ( getField<Field>( field_num ) == &address_overlap_field ) {
                dmx_address_field.setAllowAddressOverlap( address_overlap_field.isSet() );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream, "Create Fixture" ),
            m_venue(venue),
            manufacturer_field( "Manufacturer" ),
            model_field( "Model" ),
            personality_field( "Personality" ),
            fixture_number_field( "Fixture number", venue, &Venue::getFixtureByNumber, venue->nextAvailableFixtureNumber() ),
            name_field( "Fixture location", "Somewhere" ),
            description_field( "Fixture description", "" ),
            address_overlap_field( "Allow DMX addresses to overlap", false ),  
            dmx_address_field( "DMX base address", venue, NULL ),
            dmx_universe( "DMX universe", venue, 0 ),
            allow_whiteout_field( "Whiteout affects this fixture", true ),
            allow_dimming_field( "Master dimmer affects this fixture", true )
        {
            add( manufacturer_field );
            add( model_field );
            add( personality_field ); 
            add( dmx_universe );
            add( address_overlap_field );
            add( dmx_address_field );
            add( fixture_number_field );
            add( name_field );
            add( description_field );
            add( allow_whiteout_field );
            add( allow_dimming_field );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Fixture fixture( 0L,
                     form.fixture_number_field.getNumber(), 
                     form.dmx_universe.getUniverseId(),  
                     form.dmx_address_field.getIntValue(),
                     form.personality_field.getFUID(),
                     form.name_field.getValue(), 
                     form.description_field.getValue(),
                     form.allow_dimming_field.isSet(),
                     form.allow_whiteout_field.isSet() );

    getVenue()->addFixture( fixture );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createFixtureGroup( ) {
    class MyForm : public Form
    {
        enum { BASE_FIELD_COUNT = 5 };

    public:
        std::vector<ChannelControllerField> m_channel_fields;

        UniqueNumberField<GroupNumber, FixtureGroup> group_number_field;
        InputField name_field;
        InputField description_field;
        ActorSelectField actor_select;
        BooleanField load_channel_defaults;

        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( getField<Field>(field_num) == &load_channel_defaults ) {
                if ( load_channel_defaults.isSet() ) {
                    if ( size() == BASE_FIELD_COUNT ) {
                        Fixture* pf = getGroupRepresentative();
                        if ( pf == NULL )
                            return;

                        SceneActor actor( pf );

                        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ )
                            m_channel_fields.emplace_back( m_venue, actor, channel, false );

                        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ )
                            add( m_channel_fields[channel] );
                    }
                }
                else
                    resetChannels();
            }
        }

        void fieldChangeNotify( size_t field_num ) {
            if ( getField<Field>(field_num) == &actor_select )
                resetChannels();
        }

        void resetChannels() {
            m_channel_fields.clear();
            setFieldCount( BASE_FIELD_COUNT );
        }

        size_t getChannelValues( BYTE * channel_values ) {
            size_t num_channels = 0;
            for ( ChannelControllerField& chan : m_channel_fields )
                channel_values[num_channels++] = chan.getIntValue();
            return num_channels;
        }

        Fixture*  getGroupRepresentative( ) {
            Fixture* fixture = NULL;

            for ( UID fixture_uid : actor_select.getActorUIDs() ) {
                Fixture* f = m_venue->getFixture( fixture_uid );
                STUDIO_ASSERT( f != NULL, "Missing fixture for %lu", fixture_uid );

                if ( fixture == NULL || f->getNumChannels() > fixture->getNumChannels() )
                    fixture = f;
            }

            return fixture;
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream, "Create Fixture Group" ),
            m_venue(venue),
            group_number_field( "Create group number", venue, &Venue::getFixtureGroupByNumber, venue->nextAvailableFixtureGroupNumber() ),
            name_field( "Group name", "New group" ),
            description_field( "Group description", "" ),
            actor_select( "Fixtures (comma separated)", venue, false ),
            load_channel_defaults( "Set default channel values", false )

        {
            actor_select.allowMultiple( true );

            add( group_number_field );
            add( name_field );
            add( description_field );
            add( actor_select );
            add( load_channel_defaults );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    FixtureGroup group( 0L, 
                        form.group_number_field.getNumber(),
                        form.name_field.getValue(), 
                        form.description_field.getValue() );

    group.setFixtures( UIDArrayToSet( form.actor_select.getActorUIDs() ) );

    channel_value channel_values[DMX_PACKET_SIZE];
    size_t num_channels = form.getChannelValues( channel_values );
    if ( num_channels > 0 )
        group.setChannelValues( num_channels, channel_values );

    getVenue()->addFixtureGroup( group );
}


// ----------------------------------------------------------------------------
//
void DMXTextUI::sceneDeleteAllAnimations(void)
{
    SceneSelectField scene_field( "Clear all animations from scene", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( scene_field );
    if ( form.play() ) {
        Scene * scene = getVenue()->getScene( scene_field.getSceneUID() );

        if ( scene->getUID() == getVenue()->getCurrentSceneUID() )
            getVenue()->clearAllAnimations();

        // Reset scene references to empty and release private animations
        getVenue()->setSceneAnimationReferences( scene, AnimationReferenceArray(), false );

        getVenue()->sceneUpdated( scene->getUID() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::sceneDeleteAnimation(void)
{
    class MyForm : public ConfirmForm {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( field_num );
                SceneAnimationSelectField* anim_field = getField<SceneAnimationSelectField>( 1 );
                Scene* scene = m_venue->getScene( scene_field->getSceneUID() );
                anim_field->loadAnimations( scene );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            ConfirmForm( input_stream ), 
            m_venue(venue) {}
    };

    SceneSelectField scene_field( "Delete animation from scene", getVenue() );
    SceneAnimationSelectField animation_select_field( "Scene animation", getVenue()  );

    MyForm form( &m_text_io, getVenue() );
    form.add( scene_field );
    form.add( animation_select_field );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );

    if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
        getVenue()->clearAllAnimations();

    AnimationReferenceArray animation_refs = scene->animations();
    animation_refs.erase( animation_refs.begin() + animation_select_field.getAnimationIndex() );

    // Reset scene references to empty and release private animations
    getVenue()->setSceneAnimationReferences( scene, animation_refs, false );

    getVenue()->sceneUpdated( scene->getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::sceneAddAnimation(void)
{
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( field_num );
                ActorSelectField* actor_field = getField<ActorSelectField>( 2 );

                Scene* scene = m_venue->getScene( scene_field->getSceneUID() );
                actor_field->selectScene( scene_field->getSceneUID(), scene->getActorUIDs() );
            }
        }

    public:
        MyForm( TextIO* input_stream, LPCSTR title, Venue* venue ) :
            Form( input_stream, title ),
            m_venue(venue)
        {}
    };

    SceneSelectField scene_field( "Add animation to scene", getVenue() );
    AnimationSelectField anim_field( "Animation", getVenue() );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue()  );
    actor_select.allowMultiple( true );

    MyForm form( &m_text_io, "Add Animation to Scene", getVenue() );
    form.add( scene_field );
    form.add( anim_field );
    form.add( actor_select );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );
    
    if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
        getVenue()->clearAllAnimations();
    
    scene->addAnimation( AnimationReference( anim_field.getAnimationUID(), actor_select.getActorUIDs() ) );

    getVenue()->sceneUpdated( scene->getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::sceneUpdateAnimation(void)
{
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( 0 );
                Scene* scene = m_venue->getScene( scene_field->getSceneUID() );

                SceneAnimationSelectField* scene_animation_select_field = getField<SceneAnimationSelectField>( 1 );
                scene_animation_select_field->loadAnimations( scene );
            }
            else if ( field_num == 1 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( 0 );
                SceneAnimationSelectField* scene_animation_select_field = getField<SceneAnimationSelectField>( 1 );
                AnimationSelectField* anim_field = getField<AnimationSelectField>( 2 );
                ActorSelectField* actor_select = getField<ActorSelectField>( 3 );

                Scene* scene = m_venue->getScene( scene_field->getSceneUID() );
                AnimationReference* ref = scene->getAnimationByUID( scene_animation_select_field->getAnimationUID() );

                anim_field->reloadAnimations( scene );


                anim_field->setAnimationUID( ref->getUID() );


                actor_select->selectScene( scene_field->getSceneUID(), ref != NULL ? ref->getActors() : UIDArray() );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) : 
            Form( input_stream, "Update Scene Animation" ),
            m_venue(venue) {}
    };

    SceneSelectField scene_field( "Scene", getVenue() );
    SceneAnimationSelectField scene_animation_select_field( "Scene animation", getVenue() );
    AnimationSelectField anim_field( "Animation", getVenue() );
    ActorSelectField actor_select( "Fixtures (comma separated)",  getVenue()  );
    actor_select.allowMultiple( true );

    MyForm form( &m_text_io, getVenue() );
    form.add( scene_field );
    form.add( scene_animation_select_field );
    form.add( anim_field );
    form.add( actor_select );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );

    if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
        getVenue()->clearAllAnimations();

    scene->replaceAnimation( scene_animation_select_field.getAnimationIndex(), 
                AnimationReference( anim_field.getAnimationUID(), actor_select.getActorUIDs() ) );

    getVenue()->sceneUpdated( scene->getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deletePalette(void)
{
    PaletteSelectField palette_field( "Palette to delete", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( palette_field );

    if ( form.play() )
        getVenue()->deletePalette( palette_field.getPaletteUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describePalette(void)
{
    PaletteSelectField palette_field( "Palette to describe", getVenue() );

    Form form( &m_text_io );
    form.add( palette_field );

    if ( form.play() ) {
        Palette palette;
            
        getVenue()->getPalette( palette_field.getPaletteUID(), palette );

        m_text_io.printf( "Palette #%d '%s':\n", palette.getNumber(), palette.getName() );

        if ( palette.getType( ) == PT_COLOR_PALETTE ) {
            size_t color_number = 1;

            for ( auto color : palette.getPaletteColors() )
                m_text_io.printf( "   Color % 3u: %s\n", color_number++, color.getColorName() );
        }

        if ( palette.getFixtureMap().size() > 0 ) {
            for ( PaletteEntryMap::value_type& pair : palette.getFixtureMap() ) {
                Fixture* fixture = getVenue()->getFixture( pair.first );
                if ( fixture != NULL ) {
                    m_text_io.printf( "   Fixture: %s\n", fixture->getFullName() );
                    describePaletteEntry( fixture->getFixtureDefinition(), pair.second );
                }
            }
        }
              
        if ( palette.getFixtureDefinitionMap().size() > 0 ) {
            for ( PaletteEntryMap::value_type& pair : palette.getFixtureDefinitionMap() ) {
                FixtureDefinition* definition = FixtureDefinition::lookupFixture( pair.first );

                if ( definition != NULL ) {
                    m_text_io.printf( "   %s %s %d channel\n", definition->getManufacturer(), definition->getModel(), definition->getNumChannels() );
                    describePaletteEntry( definition, pair.second );
                }
            }
        }

        if ( palette.getGlobalEntry()->hasValues() ) {
            m_text_io.printf( "   Global:\n" );        
        
            describePaletteEntry( NULL, *palette.getGlobalEntry() );
        }
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describePaletteEntry( FixtureDefinition* def, PaletteEntry& entry )
{
    for ( PaletteValues::value_type& pair : entry.getValues() ) {
        unsigned channel = pair.first;
        BYTE value = pair.second;
        
        if ( entry.getAddressing() == EntryAddressing::BY_TYPE ) {
            m_text_io.printf( "      %s [%d]\n", Channel::getTypeName( (ChannelType)channel ), value );   
        }
        else {
            m_text_io.printf( "      %2u - %s [%d]\n", channel+1, def->getChannel( channel )->getName(), value );
        }    
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createPalette(void)
{
    Palette new_palette;

    new_palette.setPaletteNumber( getVenue()->nextAvailablePaletteNumber() );

    editPalette( new_palette, true );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updatePalette(void)
{
    PaletteSelectField palette_field( "Palette to update", getVenue() );

    Form form( &m_text_io );
    form.add( palette_field );

    if ( form.play() ) {
        Palette palette;
        getVenue()->getPalette( palette_field.getPaletteUID(), palette );
        editPalette( palette, false );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::editPalette( Palette& palette, bool create )
{
    class MyForm : public Form {
        Venue*	m_venue;
        Palette& m_palette;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                Palette palette;
                bool success = m_venue->getPalette( getField<InputField>( field_num )->getValue(), palette );
                if ( success && m_palette.getUID() != palette.getUID() )
                    throw FieldException( "Palette name already exists" );
            }
            if ( field_num == 1 ) {
                InputField* name_field = getField<InputField>( field_num );
                CString name( name_field->getValue() );
                if ( name.Trim().GetLength() == 0 )
                    throw FieldException( "Palette name is required" );
            }
            else if ( field_num == 3 && size() == 5 ) {
                NumberedListField* palette_type_field = getField<NumberedListField>( field_num );
                ChannelTypeSelectField* type_field = getField<ChannelTypeSelectField>( field_num+1 );

                switch ( (PaletteType)palette_type_field->getListValue() ) {
                    case PT_LOCATION:
                        type_field->setDefaultListValue( CHNLT_PAN );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_PAN_FINE ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_TILT ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_TILT_FINE ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type" ) );
                        break;

                    case PT_DIMMER:
                        type_field->setDefaultListValue( CHNLT_DIMMER );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type" ) );
                        break;

                    case PT_STROBE:
                        type_field->setDefaultListValue( CHNLT_STROBE );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type" ) );
                        break;

                    case PT_COLOR:
                        type_field->setDefaultListValue( CHNLT_RED );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_GREEN ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_BLUE ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_WHITE ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type", CHNLT_AMBER ) );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type" ) );
                        break;

                    case PT_GOBO:
                        type_field->setDefaultListValue( CHNLT_GOBO );
                        addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                        addAuto( new ChannelTypeSelectField( "Channel type" ) );
                        break;

                    default:
                        break;
                }
            }
            else if ( field_num == size()-1 ) {
                ChannelTypeSelectField* type_field = getField<ChannelTypeSelectField>( field_num );
                if ( type_field->getChannelType() != CHNLT_UNKNOWN ) {
                    addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                    addAuto( new ChannelTypeSelectField( "Channel type" ) );
                }
            }
        }

        public:
            MyForm( TextIO* input_stream, Venue* venue, Palette& palette, bool create ) :
                Form( input_stream, (create) ? "Create Palette" : "Update Palette" ),
                m_venue(venue),
                m_palette(palette)
            {
                setAutoDelete( true );

                InputField* name_field = new InputField( "Palette name", palette.getName() );
                InputField* description_field = new InputField( "Palette description", palette.getDescription() );
                NumberedListField* type_field = new NumberedListField( "Palette type" ) ;
                UniqueNumberField<PaletteNumber, Palette>* palette_number_field =
                     new UniqueNumberField<PaletteNumber, Palette>( "Palette number", m_venue, &Venue::getPaletteByNumber, palette.getPaletteNumber() );

                type_field->addKeyValue( PaletteType::PT_UNSPECIFIED, "Generic" );
                type_field->addKeyValue( PaletteType::PT_LOCATION, "Location (pan/tilt)" );
                type_field->addKeyValue( PaletteType::PT_COLOR, "Color" );
                type_field->addKeyValue( PaletteType::PT_DIMMER, "Dimmer" );
                type_field->addKeyValue( PaletteType::PT_STROBE, "Strobe" );
                type_field->addKeyValue( PaletteType::PT_GOBO, "Gobo" ); 
				type_field->addKeyValue( PaletteType::PT_FIXTURE_PRESET, "Fixture preset" );
				type_field->addKeyValue( PaletteType::PT_COLOR_PALETTE, "Color Palette" );

                type_field->setDefaultListValue( palette.getType() );

                addAuto( palette_number_field );
                addAuto( name_field );
                addAuto( description_field );
                addAuto( type_field );

                for ( PaletteValues::value_type& pair : palette.getGlobalEntry()->getValues() ) {
                    addAuto( new ChannelTypeSelectField( "Channel type", (ChannelType)pair.first ) );
                    addAuto( new IntegerField( "Channel value", (BYTE)pair.second, 0, 255 ) );
                }

                addAuto( new ChannelTypeSelectField( "Channel type" ) );
            }
    };

    MyForm form( &m_text_io, getVenue(), palette, create );
    if ( !form.play() )
        return;

    palette.setPaletteNumber( (PaletteNumber)form.getField<UniqueNumberField<PaletteNumber, Palette>>( 0 )->getNumber() );
    palette.setName( form.getField<InputField>( 1 )->getValue() );
    palette.setDescription( form.getField<InputField>( 2 )->getValue() );
    palette.setType( (PaletteType)form.getField<NumberedListField>( 3 )->getListValue() );

    PaletteEntry global( 0L, BY_TYPE );

    for ( size_t field_num=4; field_num < form.size()-1; field_num += 2 ) {
        ChannelTypeSelectField* type_field = form.getField<ChannelTypeSelectField>( field_num );
        if ( type_field->getChannelType() == CHNLT_UNKNOWN )
            continue;

        IntegerField* value_field = form.getField<IntegerField>( field_num+1 );    
        global.addValue( type_field->getChannelType(), (BYTE)value_field->getIntValue() );
    }

    palette.setGlobalEntry( global );

    if ( create ) 
        getVenue()->addPalette( palette );
    else {
        getVenue()->updatePalette( palette );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deletePaletteFixture(void)
{
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                Palette palette;
                STUDIO_ASSERT( m_venue->getPalette( getField<PaletteSelectField>( 0 )->getPaletteUID(), palette ), "Expecting a valid palette" );

                NumberedListField* fixture_field = getField<NumberedListField>( 1 );
                fixture_field->clear();

                bool selected = false;
                for ( PaletteEntryMap::value_type& pair : palette.getFixtureMap() ) {
                    Fixture* fixture = m_venue->getFixture( pair.first );
                    if ( fixture != NULL ) {
                        fixture_field->addKeyValue( fixture->getNumber(), fixture->getFullName(), fixture->getUID() );
                        if ( !selected ) {
                            fixture_field->setDefaultListValue( fixture->getNumber() );
                            selected = false;
                        }
                    }
                }
            }
        }

        public:
            MyForm( TextIO* input_stream, Venue* venue ) :
                Form( input_stream ),
                m_venue(venue)
            {
                setAutoDelete( true );

                addAuto( new PaletteSelectField( "Palette to add/update fixture", m_venue ) );
                addAuto( new NumberedListField( "Fixture to remove" ) );
            }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Palette palette;
    STUDIO_ASSERT( getVenue()->getPalette( form.getField<PaletteSelectField>( 0 )->getPaletteUID(), palette ), "Expecting a valid palette" );
    
    UID uid = form.getField<NumberedListField>( 1 )->getListValueContext();

    palette.getFixtureMap().erase( uid );
    getVenue()->updatePalette( palette );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deletePaletteFixtureDef(void)
{
    class MyForm : public Form {
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                Palette palette;
                STUDIO_ASSERT( m_venue->getPalette( getField<PaletteSelectField>( 0 )->getPaletteUID(), palette ), "Expecting a valid palette" );

                NumberedListField* fixture_field = getField<NumberedListField>( 1 );
                fixture_field->clear();

                size_t index = 1;
                CString name;
                bool selected = false;

                for ( PaletteEntryMap::value_type& pair : palette.getFixtureDefinitionMap() ) {
                    FixtureDefinition* fd = FixtureDefinition::lookupFixture( pair.first );
                    if ( fd != NULL ) {
                        name.Format( "%s %s", fd->getManufacturer(), fd->getModel() );
                        fixture_field->addKeyValue( index, name, fd->getFUID() );
                        if ( !selected ) {
                            fixture_field->setDefaultListValue( index );
                            selected = false;
                        }
                        index++;
                    }
                }
            }
        }

        public:
            MyForm( TextIO* input_stream, Venue* venue ) :
                Form( input_stream ),
                m_venue(venue)
            {
                setAutoDelete( true );

                addAuto( new PaletteSelectField( "Palette to add/update fixture definition", m_venue ) );
                addAuto( new NumberedListField( "Fixture definition to remove" ) );
            }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Palette palette;
    STUDIO_ASSERT( getVenue()->getPalette( form.getField<PaletteSelectField>( 0 )->getPaletteUID(), palette ), "Expecting a valid palette" );
    
    FUID fuid = form.getField<NumberedListField>( 1 )->getListValueContext();
    palette.getFixtureDefinitionMap().erase( fuid );

    getVenue()->updatePalette( palette );
}

// ----------------------------------------------------------------------------
//
void addFieldIfExists( Form* form, FixtureDefinition *fd,  ChannelType type )
{
    for ( channel_address channel : fd->getChannelsByType( type ) ) {
        form->addAuto( new ChannelSelectField( "Channel", fd->getFUID(), channel+1 ) );
        form->addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
    }
}

// ----------------------------------------------------------------------------
//
void autoAddPaletteFields( Form* form, FixtureDefinition *fd, PaletteType type )
{
    switch ( type ) {
        case PT_LOCATION:
            addFieldIfExists( form, fd, CHNLT_PAN );
            addFieldIfExists( form, fd, CHNLT_PAN_FINE );
            addFieldIfExists( form, fd, CHNLT_TILT );
            addFieldIfExists( form, fd, CHNLT_TILT_FINE );
            break;

        case PT_DIMMER:
            addFieldIfExists( form, fd, CHNLT_DIMMER );
            break;

        case PT_STROBE:
            addFieldIfExists( form, fd, CHNLT_STROBE );
            break;

        case PT_COLOR:
            addFieldIfExists( form, fd, CHNLT_RED );
            addFieldIfExists( form, fd, CHNLT_GREEN );
            addFieldIfExists( form, fd, CHNLT_BLUE );
            addFieldIfExists( form, fd, CHNLT_WHITE );
            addFieldIfExists( form, fd, CHNLT_AMBER );
            break;        
        
        case PT_GOBO:
            addFieldIfExists( form, fd, CHNLT_GOBO );
            break;

        default:
            break;
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updatePaletteFixtureDef(void)
{
    class MyForm : public Form {
        Venue*	m_venue;
        FUID m_fuid;
        Palette m_palette;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 1 ) {
                STUDIO_ASSERT( m_venue->getPalette( getField<PaletteSelectField>( 0 )->getPaletteUID(), m_palette ), "Expecting a valid palette" );

                FixtureDefinition* fd = FixtureDefinition::lookupFixture( getField<FixtureDefSelectField>( 1 )->getFUID() );
                STUDIO_ASSERT( fd != NULL, "Expecting a valid fixture definition" );

                if ( fd->getFUID() != m_fuid ) {                // Remove all extra fields
                    resize( 2 );
                    m_fuid = fd->getFUID();
                }   
                
                if ( size() == 2 ) {                   
                    PaletteEntryMap::iterator it = m_palette.getFixtureDefinitionMap().find( m_fuid );
                    if ( it != m_palette.getFixtureDefinitionMap().end() ) {
                        for ( PaletteValues::value_type& pair : it->second.getValues() ) {
                            addAuto( new ChannelSelectField( "Channel", m_fuid, pair.first+1 ) );
                            addAuto( new IntegerField( "Channel value", (BYTE)pair.second, 0, 255 ) );
                        }
                    }
                    else {
                        Palette palette;
                        m_venue->getPalette( getField<PaletteSelectField>( 0 )->getPaletteUID(), palette );                    
                        autoAddPaletteFields( this, fd, palette.getType() );
                    }

                    addAuto( new ChannelSelectField( "Channel", fd->getFUID() ) );
                }
            }
            else if ( field_num == size()-1 ) {
                ChannelSelectField* channel_field = getField<ChannelSelectField>( field_num );
                if ( channel_field->getChannel() != -0 ) {
                    addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                    addAuto( new ChannelSelectField( "Channel", m_fuid ) );
                }
            }
        }

        public:
            MyForm( TextIO* input_stream, Venue* venue ) :
                Form( input_stream ),
                m_venue(venue),
                m_fuid( 0L )
            {
                setAutoDelete( true );

                addAuto( new PaletteSelectField( "Palette to add/update fixture definition", m_venue ) );
                addAuto( new FixtureDefSelectField( "Fixture definition to add/update" ) );
            }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Palette palette;
    STUDIO_ASSERT( getVenue()->getPalette( form.getField<PaletteSelectField>( 0 )->getPaletteUID(), palette ), "Expecting a valid palette" );

    FUID fuid = form.getField<FixtureDefSelectField>( 1 )->getFUID();

    PaletteEntry entry( fuid, BY_CHANNEL );
    
    for ( size_t field_num=2; field_num < form.size()-1; field_num += 2 ) {
        ChannelSelectField* channel_field = form.getField<ChannelSelectField>( field_num );
        if ( channel_field->getChannel() == 0 )
            continue;

        IntegerField* value_field = form.getField<IntegerField>( field_num+1 ); 
        entry.addValue( channel_field->getChannel()-1, (BYTE)value_field->getIntValue() );
    }

    if ( entry.hasValues() )
        palette.getFixtureDefinitionMap()[fuid] = entry;
    else
        palette.getFixtureDefinitionMap().erase( fuid );

    getVenue()->updatePalette( palette );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updatePaletteFixture(void)
{
    class MyForm : public Form {
        Venue*	m_venue;
        UID m_fixture_id;
        FUID m_fuid;
        Palette m_palette;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 1 ) {
                STUDIO_ASSERT( m_venue->getPalette( getField<PaletteSelectField>( 0 )->getPaletteUID(), m_palette ), "Expecting a valid palette" );

                Fixture* pf = m_venue->getFixture( getField<ActorSelectField>( 1 )->getActorUID() );
                STUDIO_ASSERT( pf != NULL, "Expecting a valid fixture" );

                if ( pf->getUID() != m_fixture_id ) {                // Remove all extra fields
                    resize( 2 );
                    m_fixture_id = pf->getUID();
                    m_fuid = pf->getFixtureDefinition()->getFUID(); 
                }      

                if ( size() == 2 ) {
                    PaletteEntryMap::iterator it = m_palette.getFixtureMap().find( pf->getUID() );
                    if ( it != m_palette.getFixtureMap().end() ) {
                        for ( PaletteValues::value_type& pair : it->second.getValues() ) {
                            addAuto( new ChannelSelectField( "Channel", m_fuid, pair.first+1 ) );
                            addAuto( new IntegerField( "Channel value", (BYTE)pair.second, 0, 255 ) );
                        }
                    }
                    else {
                        Palette palette;
                        m_venue->getPalette( getField<PaletteSelectField>( 0 )->getPaletteUID(), palette );                    
                        FixtureDefinition* fd = FixtureDefinition::lookupFixture( pf->getFUID() );
                        STUDIO_ASSERT( fd != NULL, "Expecting a valid fixture definition" );
                        autoAddPaletteFields( this, fd, palette.getType() );
                    }
                    
                    addAuto( new ChannelSelectField( "Channel", m_fuid ) );
                }
            }
            else if ( field_num == size()-1 ) {
                ChannelSelectField* channel_field = getField<ChannelSelectField>( field_num );
                if ( channel_field->getChannel() != 0 ) {
                    addAuto( new IntegerField( "Channel value", 0, 0, 255 ) );
                    addAuto( new ChannelSelectField( "Channel", m_fuid ) );
                }
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream ),
            m_venue(venue),
            m_fixture_id( 0L )
        {
            setAutoDelete( true );

            addAuto( new PaletteSelectField( "Palette to add/update fixture", m_venue ) );
            addAuto( new ActorSelectField( "Fixture to add/update", m_venue, false ) );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Palette palette;
    STUDIO_ASSERT( getVenue()->getPalette( form.getField<PaletteSelectField>( 0 )->getPaletteUID(), palette ), "Expecting a valid palette" );

    UID fixture_id = form.getField<ActorSelectField>( 1 )->getActorUID();

    PaletteEntry entry( fixture_id, BY_CHANNEL );

    for ( size_t field_num=2; field_num < form.size()-1; field_num += 2 ) {
        ChannelSelectField* channel_field = form.getField<ChannelSelectField>( field_num );
        if ( channel_field->getChannel() == 0 )
            continue;

        IntegerField* value_field = form.getField<IntegerField>( field_num+1 ); 
        entry.addValue( channel_field->getChannel()-1, (BYTE)value_field->getIntValue() );
    }

    if ( entry.hasValues() )
        palette.getFixtureMap()[fixture_id] = entry;
    else
        palette.getFixtureMap().erase( fixture_id );

    getVenue()->updatePalette( palette );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateScenePalettes(void)
{
    class MyForm : public Form {
        Venue*	m_venue;
        UID m_scene_uid;
        UID m_actor_uid;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                m_scene_uid = getField<SceneSelectField>( 0 )->getSceneUID();
                resize(2);

                ActorSelectField* actor_field = getField<ActorSelectField>( 1 );
                actor_field->selectScene( m_scene_uid );

                m_actor_uid = 0;
            }
            else if ( field_num == 1 ) {
                m_scene_uid = getField<SceneSelectField>( 0 )->getSceneUID();
                m_actor_uid = getField<ActorSelectField>( 1 )->getActorUID();

                Scene* scene = m_venue->getScene( m_scene_uid );
                STUDIO_ASSERT( scene != NULL, "Expecting a valid scene" );
                SceneActor* actor = scene->getActor( m_actor_uid );
                STUDIO_ASSERT( actor != NULL, "Expecting a valid scene actor" );

                resize(2);

               for ( UID palette_uid : actor->getPaletteReferences() )
                    addAuto( new PaletteSelectField( "Palette", m_venue, true, palette_uid ) );

               addAuto( new PaletteSelectField( "Palette", m_venue, true ) );
            }
            else if ( size() > 2 && field_num == size()-1 ) {
                PaletteSelectField* palette_field = getField<PaletteSelectField>( size()-1 );
                if ( palette_field->getPaletteUID() != 0 )
                    addAuto( new PaletteSelectField( "Palette", m_venue, true ) );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream ),
            m_venue(venue),
            m_scene_uid(0),
            m_actor_uid(0)
        {
            setAutoDelete( true );

            addAuto( new SceneSelectField( "Scene to set palettes", m_venue ) );
            addAuto( new ActorSelectField( "Select actor", m_venue, false ) );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( form.getField<SceneSelectField>( 0 )->getSceneUID() );
    STUDIO_ASSERT( scene != NULL, "Expecting a valid scene" );
    SceneActor* actor = scene->getActor( form.getField<ActorSelectField>( 1 )->getActorUID() );
    STUDIO_ASSERT( actor != NULL, "Expecting a valid scene actor" );

    actor->removeAllPaletteReferences();

    for ( size_t index=2; index < form.size(); index++ ) {
        PaletteSelectField* palette_field = form.getField<PaletteSelectField>( index );
        actor->addPaletteReference( palette_field->getPaletteUID() );
    }

    getVenue()->sceneUpdated( scene->getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::test(void) {
}