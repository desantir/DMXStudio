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
#include "TextIO.h"
#include "AudioVolumeController.h"

static const char * LINE_CLEAR = "\r                                                                                                    \r";

static HandlerMap function_map;

static CString track_time( DWORD time );

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
    function_map[ "sx" ] = HandlerInfo( &DMXTextUI::describeScene,              true,   "Scene describe" );
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
    function_map[ "ct" ] = HandlerInfo( &DMXTextUI::chaseTap,                   true,   "Chase tap" );
    function_map[ "cb" ] = HandlerInfo( &DMXTextUI::chaseBeat,                  true,   "Chase beat" );
    function_map[ "cd" ] = HandlerInfo( &DMXTextUI::deleteChase,                true,   "Chase Delete" );
    function_map[ "cx" ] = HandlerInfo( &DMXTextUI::describeChase,              true,   "Chase Describe" );
    function_map[ "cc" ] = HandlerInfo( &DMXTextUI::createChase,                true,   "Chase Create" );
    function_map[ "cu" ] = HandlerInfo( &DMXTextUI::updateChase,                true,   "Chase Update" );
    function_map[ "cv" ] = HandlerInfo( &DMXTextUI::copyChaseSteps,             true,   "Chase copy steps" );
    function_map[ "csd" ] = HandlerInfo( &DMXTextUI::deleteChaseSteps,          true,   "Chase delete steps" );
    function_map[ "csa" ] = HandlerInfo( &DMXTextUI::addChaseSteps,             true,   "Chase add steps" );

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
    function_map[ "ws" ] = HandlerInfo( &DMXTextUI::whiteoutStrobe,             true,   "Whiteout Strobe " );

    function_map[ "a" ] = HandlerInfo( &DMXTextUI::animationSpeed,              true,   "Animation Speed" );

    function_map[ "v" ] = HandlerInfo( &DMXTextUI::masterVolume,                true,   "Master Volume" );
    function_map[ "vm" ] = HandlerInfo( &DMXTextUI::muteVolume,                 true,   "Mute Volume" );

    if ( studio.hasMusicPlayer() ) {
        function_map[ "px" ] = HandlerInfo( &DMXTextUI::listPlaylists,          false,  "List Playlists" );
        function_map[ "tx" ] = HandlerInfo( &DMXTextUI::listTracks,             false,  "List Tracks" );
        function_map[ "tp" ] = HandlerInfo( &DMXTextUI::playTrack,	    		false,  "Play Track" );
        function_map[ "tq" ] = HandlerInfo( &DMXTextUI::queueTrack,             false,  "Queue Track" );
        function_map[ "pq" ] = HandlerInfo( &DMXTextUI::queuePlaylist,          false,  "Queue Playlist" );
        function_map[ "pp" ] = HandlerInfo( &DMXTextUI::playPlaylist,           false,  "Play Playlist" );
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
    }

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

    // If we have a music player - see if we need to login
    if ( studio.hasMusicPlayer() && !studio.getMusicPlayer()->isLoggedIn() )
        musicPlayerLogin( );

    while ( m_running ) {
        CString light_status( "LIGHT | " );
        CString music_status( "SOUND | " );
                
        if ( getVenue()->isRunning() ) {
            light_status.AppendFormat( "Dimmer %d%% | ", getVenue()->getMasterDimmer() );
            if ( getVenue()->getUniverse()->isBlackout() )
                light_status.AppendFormat( "Blackout ON | " );
            else if ( getVenue()->getWhiteout() == WHITEOUT_ON )
                light_status.AppendFormat( "Whiteout ON | " );
            else if ( getVenue()->getWhiteout() == WHITEOUT_STROBE_SLOW )
                light_status.AppendFormat( "Whiteout STROBE SLOW | " );
            else if ( getVenue()->getWhiteout() == WHITEOUT_STROBE_FAST )
                light_status.AppendFormat( "Whiteout STROBE FAST | " );
            else if ( getVenue()->getWhiteout() == WHITEOUT_STROBE_MANUAL )
                light_status.AppendFormat( "Whiteout STROBE %dms | ", getVenue()->getWhiteoutStrobeMS() );
            if ( getVenue()->getAutoBlackout() != 0 )
                light_status.AppendFormat( "Blackout %lums | ", getVenue()->getAutoBlackout() );
            if ( getVenue()->isChaseRunning() ) {
                Chase* chase = getVenue()->getChase( getVenue()->getRunningChase() );
                light_status.AppendFormat( "Chase #%lu %s | ", chase->getChaseNumber(), chase->getName() );
            }
            light_status.AppendFormat( "Scene #%d %s | ", getVenue()->getScene()->getSceneNumber(), getVenue()->getScene()->getName() );

            Scene* workspace = getVenue()->getDefaultScene();
            if ( workspace->getNumActors() > 0 ) {
                light_status.AppendFormat( "Workspace Fixtures: " );
                ActorPtrArray actors = workspace->getActors();
                for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); ++it ) {
                    Fixture* pf = getVenue()->getFixture( (*it)->getFUID() );
                    light_status.AppendFormat( "%u ", pf->getFixtureNumber() );
                }
                light_status.AppendFormat( "| " );
            }
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
            DWORD length, remaining;
            DWORD track = studio.getMusicPlayer()->getPlayingTrack( &length, &remaining );
            
            if ( getVenue()->isMusicSceneSelectEnabled() )
                music_status.AppendFormat( " | Scene Select ON" );
            else
                music_status.AppendFormat( " | Scene Select OFF" );

            if ( track ) {
                music_status.AppendFormat( " | %s", studio.getMusicPlayer()->getTrackFullName( track ) );

                if ( length ) {
                    music_status.AppendFormat( " | length %s", track_time(length) );

                    if ( remaining )
                        music_status.AppendFormat( " | remaining %s", track_time(remaining) );
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
            catch ( FieldException& ex ) {
                DMXStudio::log( ex );
            }
            catch ( StudioException& ex ) {
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
    getVenue()->getUniverse()->setBlackout( !getVenue()->getUniverse()->isBlackout() );
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
        getVenue()->loadScene();
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::whiteoutStrobe(void)
{
    IntegerField whiteout_ms_field( "Strobe MS", getVenue()->getWhiteoutStrobeMS(), 25, 10000 );

    Form form( &m_text_io );
    form.add( whiteout_ms_field );

    if ( form.play() ) {
        getVenue()->setWhiteout( WHITEOUT_STROBE_MANUAL );
        getVenue()->setWhiteoutStrobeMS( whiteout_ms_field.getIntValue() );
        getVenue()->loadScene();
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::animationSpeed(void)
{
    IntegerField speed_field( "Animation speed", getVenue()->getAnimationSampleRate(), 1, 25000 );

    Form form( &m_text_io );
    form.add( speed_field );

    if ( form.play() ) {
        getVenue()->setAnimationSampleRate( speed_field.getIntValue() );
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
    IntegerField blackout_field( "Venue auto blackout MS", getVenue()->getAutoBlackout(), 0, 65000 );
    Form form( &m_text_io );

    form.add( blackout_field );

    if ( form.play() && form.isChanged() )
        getVenue()->setAutoBlackout( blackout_field.getIntValue() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::resetDefaultFixtures() {
    getVenue()->clearAllCapturedActors();
    getVenue()->loadScene();
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
    SelectionField port_field( "DMX port name", getVenue()->getDmxPort(), port_selections );
    IntegerField dmx_packet_delay_field( "DMX packet delay MS", getVenue()->getDmxPacketDelayMS() );
    IntegerField dmx_min_delay_field( "DMX packet minimum delay MS", getVenue()->getDmxMinimumDelayMS() );
    AudioCaptureField audio_capture_field( getVenue()->getAudioCaptureDevice() );
    FloatField audio_scale_field( "Audio boost", getVenue()->getAudioBoost(), "%3.1f" );
    FloatField audio_floor_field( "Audio boost sample floor", getVenue()->getAudioBoostFloor(), "%5.3f" );
    IntegerField audio_sample_size_field( "Audio sample size (multiple of 1024)", getVenue()->getAudioSampleSize(), 1024 );

    Form form( &m_text_io );

    form.add( name_field );
    form.add( desc_field );
    form.add( port_field );
    form.add( dmx_packet_delay_field );
    form.add( dmx_min_delay_field );
    form.add( audio_capture_field );
    form.add( audio_scale_field );
    form.add( audio_floor_field );
    form.add( audio_sample_size_field );

    if ( !form.play() || !form.isChanged() )
        return;

    getVenue()->close();

    getVenue()->setName( name_field.getValue() );
    getVenue()->setDescription( desc_field.getValue() );
    getVenue()->setDmxPort( port_field.getValue() );
    getVenue()->setDmxPacketDelayMS( dmx_packet_delay_field.getIntValue() );
    getVenue()->setDmxMinimumDelayMS( dmx_min_delay_field.getIntValue() );
    getVenue()->setAudioCaptureDevice( audio_capture_field.getValue() );
    getVenue()->setAudioBoost( audio_scale_field.getFloatValue() );
    getVenue()->setAudioBoostFloor( audio_floor_field.getFloatValue() );
    getVenue()->setAudioSampleSize( audio_sample_size_field.getIntValue() );

    getVenue()->open();
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
void DMXTextUI::chaseBeat()
{
    static IntegerField start_freq__field( "Starting frequency", 150, 0, 128000 );
    static IntegerField end_freq_field( "Ending frequency", 500, 0, 128000 );

    ChaseSelectField chase_field( "Chase for auto beat", getVenue() );
    Form form( &m_text_io );

    form.add( chase_field );
    form.add( start_freq__field );
    form.add( end_freq_field );

    if ( !form.play() )
        return;

    if ( getVenue()->isChaseRunning() )
        getVenue()->stopChase();

    ChaseController cc = getVenue()->startChase( chase_field.getChaseUID(), CHASE_MANUAL );
    cc.followBeat( start_freq__field.getIntValue(), end_freq_field.getIntValue() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::soundDetect( )
{
    static const char* level_meter = "====================";

    SoundDetector* detector = getVenue()->getSoundDetector();

    while ( true ) {
        if ( _kbhit() && _getch() == 27 )
            break;

        unsigned amp = getVenue()->getSoundDetector()->getAmplitude();
        const char *meter = &level_meter[ 20 - amp/50 ];

        m_text_io.printf( "%4d %s %s                       \r", 
            detector->getAmplitude(), meter, detector->isMute() ? "MUTE" : "     " );

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

    UniqueChaseNumberField chase_number_field( "Create chase number", getVenue() );
    InputField name_field( "Chase name", "New chase" );
    InputField description_field( "Chase description", "" );
    BooleanField private_field( "Private chase", false );
    IntegerField chase_delay( "Chase delay (ms)", 0, 0 );
    IntegerField chase_fade( "Chase fade (ms)", 0, 0 );

    Form form( &m_text_io );
    form.add( chase_number_field );
    form.add( name_field );
    form.add( description_field );
    form.add( private_field );
    form.add( chase_delay );
    form.add( chase_fade );

    if ( !form.play() )
        return;

    Chase chase( getVenue()->allocUID(), 
                 chase_number_field.getChaseNumber(),
                 chase_delay.getLongValue(),
                 chase_fade.getLongValue(),
                 name_field.getValue(),
                 description_field.getValue() );
    chase.setPrivate( private_field.isSet() );

    m_text_io.printf( "\nAdd chase scene steps (select scene 0 to finish)\n\n" );

    if ( editChaseSteps( &chase, true, 0 ) )
        getVenue()->addChase( chase );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateChase(void) {
    class MyForm : public Form {
        Venue*				m_venue;
        ChaseSelectField	m_chase_field;
        InputField			m_name_field;
        InputField			m_description_field;
        IntegerField		m_chase_delay;
        IntegerField		m_chase_fade;
        BooleanField        m_private_field;
        
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                Chase* chase = getChase();
                m_name_field.setValue( chase->getName() );
                m_description_field.setValue( chase->getDescription() );
                m_private_field.setInitialValue( chase->isPrivate() );
                m_chase_delay.setValue( chase->getDelayMS() );
                m_chase_fade.setValue( chase->getFadeMS() );

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
            chase->setName( m_name_field.getValue() );
            chase->setDescription( m_description_field.getValue() );
            chase->setPrivate( m_private_field.isSet() );
            chase->setDelayMS( m_chase_delay.getLongValue() );
            chase->setFadeMS( m_chase_fade.getLongValue() );
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream ),
            m_venue( venue ),
            m_chase_field( "Select chase to update", m_venue ),
            m_name_field( "Chase name", "" ),
            m_description_field( "Chase description", "" ),
            m_private_field( "Private chase", false ),
            m_chase_delay( "Chase delay (ms)", 0, 0 ),
            m_chase_fade( "Chase fade (ms)", 0, 0  )
        {
            add( m_chase_field );
            add( m_name_field );
            add( m_description_field );
            add( m_private_field );
            add( m_chase_delay );
            add( m_chase_fade );
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

    Chase dummy( 0, 0, 0, 0, "", "" );
    UINT position = position_field.getIntValue()-1;

    if ( editChaseSteps( &dummy, true, position ) ) {
        Chase* chase = getVenue()->getChase( chase_field.getChaseUID() );
        if ( getVenue()->getRunningChase() == chase->getUID() )
            getVenue()->stopChase();

        for ( size_t i=0; i < dummy.getNumSteps(); i++ )
            chase->insertStep( position+i, *dummy.getStep(i) );
    }
}

// ----------------------------------------------------------------------------
//
bool DMXTextUI::editChaseSteps( Chase* chase, bool append_steps, UINT step_num_offset ) {
    class MyForm : public Form {
        Venue*		m_venue;
        Chase*		m_chase;
        bool		m_append_steps;
        UINT		m_step_num_offset;
        
        void fieldLeaveNotify( size_t field_num ) {
            if ( m_append_steps ) {
                if ( field_num == size()-1 ) {
                    SceneSelectField* scene_field = getField<SceneSelectField>( size()-2 );
                    if ( scene_field->getSceneUID() != 0 )
                        addStep( size()/2+1, &ChaseStep( 0, 0 ) );
                }
                else if ( field_num == size()-2 ) {
                    SceneSelectField* scene_field = getField<SceneSelectField>( size()-2 );
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
                addStep( size()/2+1, &ChaseStep( 0, 0 ) );
        }

        void addStep( UINT step_number, ChaseStep* step ) {
            CString label;
            label.Format( "Step %d scene", step_number+m_step_num_offset );
            SceneNumber scene_number = step->getSceneUID() != 0 ? m_venue->getScene( step->getSceneUID() )->getSceneNumber() : 0;
            addAuto( new SceneSelectField( label, m_venue, scene_number, m_append_steps ) );
            label.Format( "Step %d delay (ms)", step_number+m_step_num_offset );
            addAuto( new IntegerField( label, step->getDelayMS(), 0 ) );
        }

        size_t getNumSteps( ) const {
            size_t steps = size()/2;
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
        SceneSelectField* scene_field = form.getField<SceneSelectField>( step_num*2 );
        IntegerField* delay_field = form.getField<IntegerField>( step_num*2+1 );

        if ( step_num < chase->getNumSteps() ) {
            ChaseStep* step = chase->getStep( step_num );
            step->setSceneUID( scene_field->getSceneUID() );
            step->setDelayMS( delay_field->getLongValue() );
        }
        else
            chase->appendStep( ChaseStep( scene_field->getSceneUID(), delay_field->getLongValue() ) );
    }

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
void DMXTextUI::describeChase(void) {
    ChaseSelectField chase_field( "Chase to describe", getVenue() );

    Form form( &m_text_io );
    form.add( chase_field );
    if ( !form.play() )
        return;

    Chase *chase = getVenue()->getChase( chase_field.getChaseUID() );
    m_text_io.printf( "\nChase #%d: %s delay %lums fade=%lums %s\n\n", 
        chase->getChaseNumber(), chase->getName(), chase->getDelayMS(), chase->getFadeMS(),
        chase->isPrivate() ? " [private]" : "" );

    for ( unsigned index=0; index < chase->getNumSteps(); index++ ) {
        ChaseStep* step = chase->getStep( index );
        m_text_io.printf( "  Step %2d: Scene %lu %s (delay %lums)\n", index+1, 
                getVenue()->getScene( step->getSceneUID() )->getSceneNumber(),
                getVenue()->getScene( step->getSceneUID() )->getName(), step->getDelayMS() );
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
void DMXTextUI::chaseTap(void) {
    ChaseSelectField chase_field( "Chase to tap", getVenue() );

    Form form( &m_text_io );
    form.add( chase_field );
    if ( !form.play() )
        return;

    if ( getVenue()->isChaseRunning() )
        getVenue()->stopChase();

    ChaseController cc = getVenue()->startChase( chase_field.getChaseUID(), CHASE_RECORD );

    m_text_io.printf( "\nPress ENTER to tap, ESCAPE to stop\n\n" );

    DWORD ms = 0;

    while ( true ) {
        int ch = _getch();

        switch ( ch ){
            case 13: {
                DWORD now = GetTickCount();
                cc.tap();

                if ( ms == 0 )
                    m_text_io.printf( "| " );
                else
                    m_text_io.printf( "%lums | ", now-ms );

                ms = now;
                break;
            }
            case 27:
                cc.loopTap();
                m_text_io.printf( "\n" );
                return;
        }
    }
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
        getVenue()->loadScene();
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::describeFixture(void) {
    FixtureSelectField fixture_field( "Fixture to describe", getVenue() );

    Form form( &m_text_io );
    form.add( fixture_field );
    if ( !form.play() )
        return;

    FixturePtrArray targets;
    fixture_field.getFixtures( targets );

    for ( FixturePtrArray::iterator it = targets.begin(); it != targets.end(); ++it ) {
        Fixture * pf = (*it);
        m_text_io.printf( "%2d: %s @ %d\n", pf->getFixtureNumber(), pf->getFullName(), pf->getAddress() );

        for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            m_text_io.printf( "   %d->%d @ %d - %s\n", channel+1, pf->getChannel(channel)->getOffset()+1, pf->getChannelAddress( channel ), cp->getName() );

            ChannelValueRangeArray ranges = cp->getRanges();
            for ( ChannelValueRangeArray::iterator rit=ranges.begin(); rit != ranges.end(); rit++ ) {
                m_text_io.printf( "      %03d-%03d %s\n", (*rit).getStart(), (*rit).getEnd(), (*rit).getName() );
            }
        }
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::controlFixture( ) {
    FixtureSelectField fixture_field( "Fixture to control", getVenue(), true );

    Form form( &m_text_io );
    form.add( fixture_field );
    if ( !form.play() )
        return;

    FixturePtrArray targets;
    channel_t channel = 0;
    size_t num_channels = 0;

    fixture_field.getFixtures( targets );

    CString title;

    for ( FixturePtrArray::iterator it = targets.begin(); it != targets.end(); it++, channel++ ) {
        Fixture* pf = (*it);
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
        form.addAuto( new ChannelControllerField( getVenue(), &targets, channel ) );

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
    SceneSelectField scene_field( "Scene to load", getVenue() );

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

        m_text_io.printf( "\nScene #%lu - %s %s\n", scene->getSceneNumber(), scene->getName(), scene->isPrivate() ? " [private]" : "" );
        if ( scene->getNumActors() == 0 )
            m_text_io.printf( "\n   NO FIXTURES\n" );

        ActorPtrArray actors = scene->getActors();

        for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); ++it ) {
            SceneActor* actor = (*it);
            Fixture* fixture = getVenue()->getFixture( actor->getFUID() );

            m_text_io.printf( " %3d %s @ %u UID=%lu\n", fixture->getFixtureNumber(), fixture->getFullName(), 
                                                       fixture->getChannelAddress(0), fixture->getUID() );

            for ( channel_t channel=0; channel < fixture->getNumChannels(); channel++ ) {
                m_text_io.printf( "      %2u - %s [%d]\n", 
                    channel+1,
                    fixture->getChannel( channel )->getName(),
                    actor->getChannelValue( fixture->mapChannel( channel ) ) );
            }
        }

        for ( size_t i=0; i < scene->getNumAnimations(); i++ ) {
            AbstractAnimation* animation = scene->getAnimation(i);
            CString animimation_synopsis = animation->getSynopsis();
            animimation_synopsis.Replace( "\n", "\n      " );

            m_text_io.printf( "  Animation: %s (ID %d)\n", animation->getName(), animation->getUID());
            m_text_io.printf( "      %s\n", (LPCTSTR)animimation_synopsis );
            m_text_io.printf( "      Signal( %s )\n", (LPCTSTR)animation->signal().getSynopsis() );
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
            InputField name_field( "Scene name", scene->getName() );
            InputField description_field( "Scene description", scene->getDescription() );
            BooleanField private_field( "Private scene", scene->isPrivate() );

            form.clear();
            form.add( name_field );
            form.add( description_field );
            form.add( private_field );

            if ( form.play() ) {
                scene->setName( name_field.getValue() );
                scene->setDescription( description_field.getValue() );
                scene->setPrivate( private_field.isSet() );
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

    UIDArray default_actors = getVenue()->getDefaultScene()->getActorUIDs();

    SceneSelectField scene_field( "Copy workspace fixtures to scene", getVenue() );
    FixtureSelect fixtures_field( "Select fixture(s) to copy", getVenue(), default_actors, default_actors );
    BooleanField remove_from_ws_field( "Remove from workspace", true );

    Form form( &m_text_io );
    form.add( scene_field );
    form.add( fixtures_field );
    form.add( remove_from_ws_field );

    if ( form.play() ) {
        Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );
        Scene* default_scene = getVenue()->getDefaultScene();

        UIDArray selected = fixtures_field.getUIDs();
        for ( UIDArray::iterator it=selected.begin(); it != selected.end(); ++it ) {
            scene->addActor( *default_scene->getActor( (*it) ) );
            if ( remove_from_ws_field.isSet() )
                default_scene->removeActor( (*it) );
        }

        if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
            getVenue()->loadScene();
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createScene( ) {
    UniqueSceneNumberField scene_number_field( "Create scene number", getVenue() );
    InputField name_field( "Scene name", "New scene" );
    InputField description_field( "Scene description", "" );
    BooleanField private_field( "Private scene", false );
    BooleanField copy_field( "Copy current fixtures", true );

    Form form( &m_text_io );
    form.add( scene_number_field );
    form.add( name_field );
    form.add( description_field );
    form.add( private_field );
    form.add( copy_field );

    if ( !form.play() )
        return;

    Scene scene( getVenue()->allocUID(), scene_number_field.getSceneNumber(),
                 name_field.getValue(), description_field.getValue() );
    scene.setPrivate( private_field.isSet() );

    getVenue()->addScene( scene );

    if ( copy_field.isSet() )
        getVenue()->copyDefaultFixturesToScene( scene.getUID() );

    getVenue()->clearAllCapturedActors();
    getVenue()->selectScene( scene.getUID() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::copyScene(void) {
    SceneSelectField scene_field( "Scene to copy", getVenue() );
    UniqueSceneNumberField scene_number_field( "New scene number", getVenue() );
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
        
    new_scene.setUID( getVenue()->allocUID() );
    new_scene.setSceneNumber( scene_number_field.getSceneNumber() );
    new_scene.setName( name_field.getValue() );
    new_scene.setDescription( description_field.getValue() );

    getVenue()->addScene( new_scene );
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
                SceneFixtureSelectField* scene_fixture_field = getField<SceneFixtureSelectField>( 1 );
                scene_fixture_field->selectScene( scene_field->getSceneUID() );
            }
        }

    public:
        MyForm( TextIO* input_stream) : Form( input_stream ) {}
    };

    SceneSelectField scene_select_field( "Scene to copy from", getVenue(), 0, true );
    SceneFixtureSelectField scene_fixture_field( "Scene fixture to copy", getVenue(), 0 );

    MyForm form( &m_text_io );
    form.add( scene_select_field );
    form.add( scene_fixture_field );
    if ( !form.play() )
        return;

    getVenue()->copySceneFixtureToDefault( scene_select_field.getSceneUID(),
                                        scene_fixture_field.getFixtureUID() );
    getVenue()->loadScene();
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::deleteFixtureFromScene(void) {

    class MyForm : public Form
    {
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                SceneSelectField* scene_field = getField<SceneSelectField>( field_num );
                SceneFixtureSelectField* scene_fixture_field = getField<SceneFixtureSelectField>( 1 );
                scene_fixture_field->selectScene( scene_field->getSceneUID() );
            }
        }

    public:
        MyForm( TextIO* input_stream) : Form( input_stream ) {}
    };

    SceneSelectField scene_select_field( "Delete fixture from scene", getVenue(), 0, true );
    SceneFixtureSelectField scene_fixture_field( "Delete scene fixture", getVenue(), 0 );
    
    MyForm form( &m_text_io );
    form.add( scene_select_field );
    form.add( scene_fixture_field );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_select_field.getSceneUID() );
    scene->removeActor( scene_fixture_field.getFixtureUID() );

    if ( scene->getUID() == getVenue()->getCurrentSceneUID() )
        getVenue()->loadScene();
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
            getVenue()->clearAnimations();

        scene->clearAnimations();

        if ( scene->getUID() == getVenue()->getCurrentSceneUID() )
            getVenue()->loadScene();
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
                anim_field->selectAnimations( scene );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            ConfirmForm( input_stream ), 
            m_venue(venue) {}
    };

    SceneSelectField scene_field( "Delete animation from scene", getVenue() );
    SceneAnimationSelectField animation_select_field( "Scene animation" );

    MyForm form( &m_text_io, getVenue() );
    form.add( scene_field );
    form.add( animation_select_field );
    if ( !form.play() )
        return;

    Scene* scene = getVenue()->getScene( scene_field.getSceneUID() );

    if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
        getVenue()->clearAnimations();

    scene->removeAnimation( animation_select_field.getAnimationUID() );

    if ( getVenue()->getCurrentSceneUID() == scene->getUID() )
        getVenue()->loadScene();
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

    FixtureSelectField fixture_select_field( "Fixture to delete", getVenue() );

    ConfirmForm form( &m_text_io );
    form.add( fixture_select_field );
    if ( form.play() ) {
            getVenue()->deleteFixture( fixture_select_field.getFixtureUID() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::updateFixture(void) {
    FixtureSelectField fixture_select_field( "Fixture to update", getVenue() );

    Form form( &m_text_io );
    form.add( fixture_select_field );

    if ( !form.play() )
        return;

    Fixture* pf = getVenue()->getFixture( fixture_select_field.getFixtureUID() );

    InputField name_field( "Fixture location", pf->getName() );
    InputField description_field( "Fixture description", pf->getDescription() );
    DmxAddressField dmx_address_field( "DMX base address", getVenue(), pf );

    form.clear();
    form.add( name_field );
    form.add( description_field );
    form.add( dmx_address_field );

    if ( !form.play() )
        return;

    pf->setName( name_field.getValue() );
    pf->setDescription( description_field.getValue() );
    pf->setAddress( dmx_address_field.getIntValue() );
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
        UniqueFixtureNumberField fixture_number_field;
        InputField name_field;
        InputField description_field;
        BooleanField address_overlap_field;
        DmxAddressField dmx_address_field;
        Venue*	m_venue;

        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                model_field.setManufacturer( manufacturer_field.getKeyValue() );
            }
            else if ( field_num == 1 ) {
                personality_field.setManuAndModel( manufacturer_field.getKeyValue(), model_field.getKeyValue() );
            }
            else if ( field_num == 2 ) {
                FixtureDefinition* fixdef = 
                    &FixtureDefinition::FixtureDefinitions[personality_field.getFUID()];
                dmx_address_field.setNumChannels( fixdef->getNumChannels() );
                channel_t base_address = m_venue->findFreeAddressRange( fixdef->getNumChannels() );
                if ( base_address == INVALID_CHANNEL )
                    throw FieldException( "There are no empty address ranges available for %d channels", fixdef->getNumChannels() );
                dmx_address_field.setInitialValue( base_address ); 
            }
            else if ( field_num == 6 ) {
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
            fixture_number_field( "Fixture number", venue ),
            name_field( "Fixture location", "Somewhere" ),
            description_field( "Fixture description", "" ),
            address_overlap_field( "Allow DMX addresses to overlap", false ),  
            dmx_address_field( "DMX base address", venue, NULL )
        {
            add( manufacturer_field );
            add( model_field );
            add( personality_field ); 
            add( fixture_number_field );
            add( name_field );
            add( description_field );
            add( address_overlap_field );
            add( dmx_address_field );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( !form.play() )
        return;

    Fixture fixture( getVenue()->allocUID(),
                     form.fixture_number_field.getFixtureNumber(), 
                     1,  
                     form.dmx_address_field.getIntValue(),
                     form.personality_field.getFUID(),
                     form.name_field.getValue(), 
                     form.description_field.getValue() );

    getVenue()->addFixture( fixture );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::createFixtureGroup( ) {
    FixturePtrArray fixtures = getVenue()->getFixtures();
    UIDArray fixture_uids;
    for ( FixturePtrArray::iterator it=fixtures.begin(); it != fixtures.end(); ++it )
        fixture_uids.push_back( (*it)->getUID() );
        
    UniqueGroupNumberField group_number_field( "Create group number", getVenue() );
    InputField name_field( "Group name", "New group" );
    InputField description_field( "Group description", "" );
    FixtureSelect fixture_select( "Fixtures (comma separated)", getVenue(), fixture_uids, UIDArray() );

    Form form( &m_text_io );
    form.add( group_number_field );
    form.add( name_field );
    form.add( description_field );
    form.add( fixture_select );
    if ( !form.play() )
        return;

    FixtureGroup group( getVenue()->allocUID(), 
                        group_number_field.getGroupNumber(),
                        name_field.getValue(), 
                        description_field.getValue() );

    group.setFixtures( UIDArrayToSet( fixture_select.getUIDs() ) );
    getVenue()->addFixtureGroup( group );
}
