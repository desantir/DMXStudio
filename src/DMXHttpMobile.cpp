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


#include "DMXHttpMobile.h"

#include "Venue.h"

// ----------------------------------------------------------------------------
//
DMXHttpMobile::DMXHttpMobile(void)
{
    m_rest_handlers[ DMX_URL_CONTROL_SCENE_SHOW ] = &DMXHttpMobile::control_scene_show;
    m_rest_handlers[ DMX_URL_CONTROL_CHASE_SHOW ] = &DMXHttpMobile::control_chase_show;
    m_rest_handlers[ DMX_URL_CONTROL_VENUE_BLACKOUT ] = &DMXHttpMobile::control_venue_blackout;
    m_rest_handlers[ DMX_URL_CONTROL_VENUE_DIMMER ] = &DMXHttpMobile::control_venue_dimmer;
    m_rest_handlers[ DMX_URL_CONTROL_VENUE_WHITEOUT ] = &DMXHttpMobile::control_venue_whiteout;
    m_rest_handlers[ DMX_URL_CONTROL_FIXTURE_CAPTURE ] = &DMXHttpMobile::control_fixture_capture;
    m_rest_handlers[ DMX_URL_CONTROL_FIXTURE_RELEASE ] = &DMXHttpMobile::control_fixture_release;
    m_rest_handlers[ DMX_URL_CONTROL_FIXTURE_CHANNEL ] = &DMXHttpMobile::control_fixture_channel;
    m_rest_handlers[ DMX_URL_CONTROL_VENUE_STROBE ] = &DMXHttpMobile::control_venue_strobe;
    m_rest_handlers[ DMX_URL_CONTROL_ANIMATION_SPEED ] = &DMXHttpMobile::control_animation_speed;
    m_rest_handlers[ DMX_URL_QUERY_VENUE ] = &DMXHttpMobile::query_venue;
    m_rest_handlers[ DMX_URL_CONTROL_MASTER_VOLUME ] = &DMXHttpMobile::control_master_volume;
    m_rest_handlers[ DMX_URL_QUERY_SOUND ] = &DMXHttpMobile::query_sound;
    m_rest_handlers[ DMX_URL_CONTROL_MUTE_VOLUME ] = &DMXHttpMobile::control_mute_volume;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_BACK ] = &DMXHttpMobile::control_music_track_back;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_FORWARD ] = &DMXHttpMobile::control_music_track_forward;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_STOP ] = &DMXHttpMobile::control_music_track_stop;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_PAUSE ] = &DMXHttpMobile::control_music_track_pause;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_PLAY ] = &DMXHttpMobile::control_music_track_play;
    m_rest_handlers[ DMX_URL_QUERY_MUSIC_PLAYLISTS ] = &DMXHttpMobile::query_music_playlists;
    m_rest_handlers[ DMX_URL_QUERY_MUSIC_PLAYLIST_TRACKS ] = &DMXHttpMobile::query_music_playlist_tracks;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_PLAY_TRACK ] = &DMXHttpMobile::control_music_play_track;
    m_rest_handlers[ DMX_URL_CONTROL_MUSIC_PLAY_PLAYLIST ] = &DMXHttpMobile::control_music_play_playlist;
}

// ----------------------------------------------------------------------------
//
DMXHttpMobile::~DMXHttpMobile(void)
{
}

// ----------------------------------------------------------------------------
//
DWORD DMXHttpMobile::processGetRequest( HttpWorkerThread* worker )
{
    CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    int pos = path.Find( '?' );
    if ( pos != -1 )                                        // Remove query string
       path = path.Left( pos );

    CString prefix( path );
    if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
        prefix += "/";

    if ( prefix == DMX_URL_ROOT ) {                        // Redirect to mobile index page
        CString redirect;
        redirect.Format( "%smobile/mobile.htm", DMX_URL_ROOT );
        return worker->sendRedirect( (LPCSTR)redirect );
    }

    // Invoke the approriate handler
    RestHandlerFunc func = NULL;
    UINT len = 0;
    for ( RestHandlerMap::iterator it=m_rest_handlers.begin(); it != m_rest_handlers.end(); it++ ) {
        if ( prefix.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
            func = it->second;
            len = strlen(it->first);
        }
    }

    if ( func != NULL ) {
        CString response;
        if ( (this->*func)( response, path.Mid( len ) ) )
            return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
        return worker->error_501();
    }

    // Perhaps this is a file request
    if ( path.Find( DMX_URL_ROOT, 0 ) == 0 ) {
        path.Replace( DMX_URL_ROOT, "" );
        return worker->sendFile( (LPCSTR)path, this ); 
    }

    return worker->error_404();
}

// ----------------------------------------------------------------------------
//
DWORD DMXHttpMobile::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    return worker->error_501();
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_animation_speed( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    DWORD sample_rate_ms;
        
    if ( sscanf_s( path_fragment, "%lu", &sample_rate_ms ) != 1 )
        return false;

    studio.getVenue()->setAnimationSampleRate( sample_rate_ms );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::query_venue( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    response.Format( "{ \"blackout\":%d, \"dimmer\":%u, \"whiteout\":%d, \"whiteout_strobe\":%u, \"animation_speed\": %lu }",
        studio.getVenue()->getUniverse()->isBlackout(), 
        studio.getVenue()->getMasterDimmer(),
        studio.getVenue()->getWhiteout(),
        studio.getVenue()->getWhiteoutStrobeMS(),
        studio.getVenue()->getAnimationSampleRate( ) );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::query_sound( CString& response, LPCSTR path_fragment )
{
    CString music_player_json;

    if ( studio.hasMusicPlayer() ) {
        DWORD length, remaining;
        UINT queued;
        DWORD track = studio.getMusicPlayer()->getPlayingTrack( &length, &remaining, &queued );

        music_player_json.Format( ", \"music_player\": { \"queued\": %d, \"playing\": ", queued );

        if ( track != 0 ) {
            CString track_name = studio.getMusicPlayer()->getTrackFullName( track );
            bool paused = studio.getMusicPlayer()->isTrackPaused();
            
            music_player_json.AppendFormat( 
                "1, \"track\": \"%s\", \"length\": %lu, \"remaining\": %lu, \"paused\": %d }",
                encodeHtmlString( track_name ), length, remaining, paused ? 1 : 0 );
        }
        else
            music_player_json.Append( "0 }" );
    }

    response.Format( "{ \"volume\": %u, \"mute\": %u, \"has_music_player\": %d %s }", 
        studio.getVenue()->getMasterVolume( ),
        studio.getVenue()->isMasterVolumeMute( ),
        studio.hasMusicPlayer() ? 1 : 0,
        (LPCSTR)music_player_json );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_play_track( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT        playlist_number, track_number;
    unsigned    queue;

    if ( sscanf_s( path_fragment, "%lu/%lu/%u", &playlist_number, &track_number, &queue ) != 3 )
        return false;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );
    if ( playlist_number == 0 || playlist_number >playlists.size() )
        return false;

    DWORD playlist_id = playlists[ playlist_number-1 ];

    PlayerItems tracks;
    studio.getMusicPlayer()->getTracks( playlist_id, tracks );
    if ( track_number == 0 || track_number >tracks.size() )
        return false;

    DWORD track_id = tracks[ track_number-1 ];
    studio.getMusicPlayer()->playTrack( track_id, queue ? true : false );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_play_playlist( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT       playlist_number;
    unsigned    queue;

    if ( sscanf_s( path_fragment, "%lu/%u", &playlist_number, &queue ) != 2 )
        return false;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );
    if ( playlist_number == 0 || playlist_number >playlists.size() )
        return false;

    DWORD playlist_id = playlists[ playlist_number-1 ];
    studio.getMusicPlayer()->playAllTracks( playlist_id, queue ? true : false );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::query_music_playlists( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    response =  "{ \"playlists\": [ ";
    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );

    UINT playlist_number = 1;
    for ( PlayerItems::iterator it=playlists.begin(); it != playlists.end(); it++, playlist_number++ ) {
        CString name = studio.getMusicPlayer()->getPlaylistName( (*it) );
        if ( it != playlists.begin() )
            response.Append( "," );
        response.AppendFormat( " { \"id\": %lu, \"name\": \"%s\" }", playlist_number, encodeHtmlString( name ) );
    }
    
    response.Append( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::query_music_playlist_tracks( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT playlist_number;

    if ( sscanf_s( path_fragment, "%u", &playlist_number ) != 1 )
        return false;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );
    if ( playlist_number == 0 || playlist_number >playlists.size() )
        return false;

    DWORD playlist_id = playlists[ playlist_number-1 ];

    response = "{ \"playlist\": [ ";

    PlayerItems tracks;
    studio.getMusicPlayer()->getTracks( playlist_id, tracks );

    UINT track_number = 1;
    for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); it++, track_number++ ) {
        CString name = studio.getMusicPlayer()->getTrackFullName( (*it) );
        if ( it != tracks.begin() )
            response.Append( "," );
        response.AppendFormat( " { \"id\": %lu, \"name\": \"%s\" }", track_number, encodeHtmlString( name ) );
    }
    
    response.Append( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_back( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->backTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_forward( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->forwardTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_stop( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->stopTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_pause( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( true );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_play( CString& response, LPCSTR path_fragment )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( false );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_master_volume( CString& response, LPCSTR path_fragment )
{
    UINT master_volume;
        
    if ( sscanf_s( path_fragment, "%u", &master_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolume( master_volume );

    return true;
}


// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_mute_volume( CString& response, LPCSTR path_fragment )
{
    UINT mute_volume;
        
    if ( sscanf_s( path_fragment, "%u", &mute_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolumeMute( mute_volume ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_scene_show( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID scene_id;

    if ( sscanf_s( path_fragment, "%lu", &scene_id ) != 1 )
        return false;

    Scene* scene = studio.getVenue()->getScene( scene_id );
    if ( !scene )
        return false;

    studio.getVenue()->selectScene( scene_id );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_chase_show( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID chase_id;
        
    if ( sscanf_s( path_fragment, "%lu", &chase_id ) != 1 )
        return false;

    if ( chase_id > 0 ) {
        Chase* chase = studio.getVenue()->getChase( chase_id );
        if ( !chase )
            return false;

        studio.getVenue()->startChase( chase_id );
    }
    else
        studio.getVenue()->stopChase();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_venue_blackout( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    unsigned blackout;
        
    if ( sscanf_s( path_fragment, "%u", &blackout ) != 1 )
        return false;

    studio.getVenue()->getUniverse()->setBlackout( blackout ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_venue_dimmer( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    int dimmer;

    if ( sscanf_s( path_fragment, "%d", &dimmer ) != 1 )
        return false;
    if ( dimmer < 0 || dimmer > 100 )
        return false;

    studio.getVenue()->setMasterDimmer( dimmer );
    studio.getVenue()->loadScene();

    return true;
}  

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_venue_whiteout( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    int whiteout;

    if ( sscanf_s( path_fragment, "%d", &whiteout ) != 1 )
        return false;
    if ( whiteout < 0 || whiteout > 4)
        return false;

    studio.getVenue()->setWhiteout( (WhiteoutMode)whiteout );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_venue_strobe( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UINT whiteout_strobe_ms;

    if ( sscanf_s( path_fragment, "%u", &whiteout_strobe_ms ) != 1 )
        return false;
    if ( whiteout_strobe_ms < 25 || whiteout_strobe_ms > 10000)
        return false;

    studio.getVenue()->setWhiteoutStrobeMS( whiteout_strobe_ms );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_fixture_capture( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( path_fragment, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id > 0 )
        studio.getVenue()->captureActor( fixture_id );

    response = getFixtureDivContent();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_fixture_release( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( path_fragment, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id != 0 )
        studio.getVenue()->releaseActor( fixture_id );
    else
        studio.getVenue()->clearAllCapturedActors();

    studio.getVenue()->loadScene();

    response = getFixtureDivContent();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_fixture_channel( CString& response, LPCSTR path_fragment )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
    channel_t channel;
    unsigned channel_value;

    if ( sscanf_s( path_fragment, "%lu/%u/%u", &fixture_id, &channel, &channel_value ) != 3 )
        return false;

    Fixture* pf = studio.getVenue()->getFixture( fixture_id );
    if ( !pf )
        return false;

    studio.getVenue()->setChannelValue( pf, channel, channel_value );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::substitute( LPCSTR marker, LPCSTR data, CString& marker_content )
{
    // TODO PARAMETERIZE PATHS (Low)
    // TODO Send objects and assemble in client i.e. no more HTML?

    if ( !strcmp( marker, "page_header" ) ) {
        // Cloning the header in JS is a non-starter in jQuery as there is a bug where you can not refomat the header div

        static LPCSTR BUTTON_HIGHLIGHT = "class=\"ui-state-persist ui-btn-active\"";

        marker_content.Append( "        <div id=\"header_id\" data-role=\"header\" data-id=\"dmxstudio_header\" >\n" );
        marker_content.Append( "		    <h2 id=\"header_title\" style=\"margin-right:0;margin-left:0;margin-top:5px;margin-bottom:5px;\">DMX Studio Remote</h2>\n" );
        marker_content.Append( "            <div data-role=\"navbar\">\n" );
        marker_content.Append( "	            <ul>\n" );
        marker_content.AppendFormat( "                    <li><a href=\"#venue\" %s>Venue</a></li>\n",
            ( !strcmp( data, "venue" ) ) ? BUTTON_HIGHLIGHT : "" );
        marker_content.AppendFormat( "		            <li><a href=\"#scene\" %s>Scene</a></li>\n",
            ( !strcmp( data, "scene" ) ) ? BUTTON_HIGHLIGHT : "" );
        marker_content.AppendFormat( "		            <li><a href=\"#chase\" %s>Chase</a></li>\n",
            ( !strcmp( data, "chase" ) ) ? BUTTON_HIGHLIGHT : "" );
        marker_content.AppendFormat( "		            <li><a href=\"#fixture\" %s>Fixture</a></li>\n",
            ( !strcmp( data, "fixture" ) ) ? BUTTON_HIGHLIGHT : "" );
        marker_content.AppendFormat( "		            <li><a href=\"#sound\" %s>Sound</a></li>\n",
            ( !strcmp( data, "sound" ) ) ? BUTTON_HIGHLIGHT : "" );
        marker_content.Append( "	            </ul>\n" );
        marker_content.Append( "            </div><!-- /navbar -->\n" );
        marker_content.Append( "	    </div><!-- /header -->\n" );
    }
    else  if ( !studio.getVenue() || !studio.getVenue()->isRunning() ) {
            marker_content = "";
    }
    else if ( !strcmp( marker, "fixture_slider_content" ) ) {
        marker_content = getFixtureDivContent();
    }
    else if ( !strcmp( marker, "fixture_select" ) ) {
        FixturePtrArray fixtures = studio.getVenue()->getFixtures();
        SceneActor* actor = studio.getVenue()->getCapturedActor();

        for ( FixturePtrArray::iterator it=fixtures.begin(); it != fixtures.end(); it++ ) {
            Fixture* fixture = (*it);
            LPCSTR selected = (actor != NULL && actor->getPFUID() == fixture->getUID()) ? "selected" : "";

            marker_content.AppendFormat( "<option value=\"%lu\" %s>%s</option>\n", fixture->getUID(), selected,
                encodeHtmlString( fixture->getFullName() ) );
        }
    }
    else if ( !strcmp( marker, "scene_buttons" ) ) {
        ScenePtrArray scenes = studio.getVenue()->getScenes();
        for ( ScenePtrArray::iterator it=scenes.begin(); it != scenes.end(); it++ ) {
            Scene* scene = (*it);
            CString active;

            if ( scene->isPrivate() )
                continue;

            if ( scene->getUID() == studio.getVenue()->getCurrentSceneUID() ) {
                active = " checked=\"checked\"";
            }

            marker_content.AppendFormat( "<input type=\"radio\" name=\"select_scene\" id=\"scene_%lu\" value=\"%lu\" %s />\n", 
                                            scene->getUID(), scene->getUID(), active );
            marker_content.AppendFormat( "<label for=\"scene_%lu\">%s</label>\n", 
                                            scene->getUID(), encodeHtmlString( scene->getName() ) );
        }
    }
    else if ( !strcmp( marker, "chase_buttons" ) ) {
        marker_content.AppendFormat( "<input type=\"radio\" name=\"chase_scene\" id=\"chase_0\" value=\"0\" data-theme=\"a\" %s />\n", 
                                     ( studio.getVenue()->getRunningChase() != 0 ) ? "" : " checked=\"checked\"" );
        marker_content.AppendFormat( "<label for=\"chase_0\">Chase Stopped</label>\n" );

        ChasePtrArray chases = studio.getVenue()->getChases();

        for ( ChasePtrArray::iterator it=chases.begin(); it != chases.end(); it++ ) {
            Chase* chase = (*it);
            CString active;

            if ( chase->isPrivate() )
                continue;

            if ( chase->getUID() == studio.getVenue()->getRunningChase() ) {
                active = " checked=\"checked\"";
            }

            marker_content.AppendFormat( "<input type=\"radio\" name=\"chase_scene\" id=\"chase_%lu\" value=\"%lu\" %s />\n", 
                                            chase->getUID(), chase->getUID(), active );
            marker_content.AppendFormat( "<label for=\"chase_%lu\">%s</label>\n", 
                                            chase->getUID(), encodeHtmlString( chase->getName() ) );
        }
    }
    else
        return false;

    return true;
}

// ----------------------------------------------------------------------------
//
CString DMXHttpMobile::getFixtureDivContent()
{
    CString marker_content;

    SceneActor* actor = studio.getVenue()->getCapturedActor();
    if ( actor != NULL ) {
        marker_content.AppendFormat( "<hr />\n" );

        Fixture* fixture = studio.getVenue()->getFixture( actor->getPFUID() );

        marker_content.AppendFormat( "<div>\n" );

        marker_content.AppendFormat( "<b>%s</b>", fixture->getFullName() );
        marker_content.AppendFormat( "<form>\n" );

        for ( channel_t channel=0; channel < fixture->getNumChannels(); channel++ ) {
            marker_content.AppendFormat( "<label for=\"slider-ch%d\">%s:</label><br />\n", 
                channel, encodeHtmlString( fixture->getChannel(channel)->getName() ) );
            marker_content.AppendFormat( "<input type=\"range\" name=\"slider-ch%d\" id=\"slider-ch%d.id\" data-highlight=\"true\" ", 
                channel, channel );
            marker_content.AppendFormat( "onchange=\"change_fixture_channel(%lu,%u,this.value); return false;\" ", 
                fixture->getUID(), channel );
            marker_content.AppendFormat( "value=\"%d\" min=\"0\" max=\"255\"/><br/>\n", 
                actor->getChannelValue( channel ) );
        }
        marker_content.AppendFormat( "</form>\n" );
        marker_content.AppendFormat( "</div>\n" );
    }

    Scene* scene = studio.getVenue()->getDefaultScene();
    UIDArray actors = scene->getActorUIDs();

    if ( actors.size() > 0 ) {
        marker_content.AppendFormat( "<hr />\n" );
    }

    for ( UIDArray::iterator it=actors.begin(); it != actors.end(); it++ ) {
        Fixture* fixture = studio.getVenue()->getFixture( (*it) );

        marker_content.AppendFormat( "<a data-icon=\"delete\" data-role=\"button\" data-theme=\"a\" data-mini=\"true\" onclick=\"release_fixture(%lu); return false;\">Release %s</a>\n",
            fixture->getUID(), encodeHtmlString( fixture->getFullName() ) );
    }

    if ( actors.size() > 1 ) {
        marker_content.AppendFormat("<a data-role=\"button\" data-icon=\"delete\" data-mini=\"true\" onclick=\"release_fixture(0); return false;\">Release All</a>" );
    }

    return marker_content;
}