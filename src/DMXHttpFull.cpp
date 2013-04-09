/* 
Copyright (C) 2011-2013 Robert DeSantis
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

#include "DMXHttpFull.h"

#include "Venue.h"

#define DMX_URL_QUERY_SCENES                    DMX_URL_ROOT_FULL "query/scenes/"
#define DMX_URL_QUERY_CHASES                    DMX_URL_ROOT_FULL "query/chases/"
#define DMX_URL_QUERY_FIXTURES                  DMX_URL_ROOT_FULL "query/fixtures/"
#define DMX_URL_QUERY_VENUE_STATUS              DMX_URL_ROOT_FULL "query/venue/status/"
#define DMX_URL_QUERY_VENUE_DESCRIBE            DMX_URL_ROOT_FULL "query/venue/describe/"
#define DMX_URL_QUERY_FIXTURE_DEFINITIONS       DMX_URL_ROOT_FULL "query/fixture/definitions"

#define DMX_URL_CONTROL_VENUE_BLACKOUT          DMX_URL_ROOT_FULL "control/venue/blackout/"
#define DMX_URL_CONTROL_VENUE_WHITEOUT          DMX_URL_ROOT_FULL "control/venue/whiteout/"
#define DMX_URL_CONTROL_VENUE_MASTERDIMMER      DMX_URL_ROOT_FULL "control/venue/masterdimmer/" 
#define DMX_URL_CONTROL_VENUE_STROBE            DMX_URL_ROOT_FULL "control/venue/strobe/" 
#define DMX_URL_CONTROL_SCENE_SHOW              DMX_URL_ROOT_FULL "control/scene/show/" 
#define DMX_URL_CONTROL_CHASE_SHOW              DMX_URL_ROOT_FULL "control/chase/show/" 

#define DMX_URL_CONTROL_FIXTURE_CHANNELS        DMX_URL_ROOT_FULL "control/fixture/channels/"
#define DMX_URL_CONTROL_FIXTURE                 DMX_URL_ROOT_FULL "control/fixture/"
#define DMX_URL_CONTROL_FIXTUREGROUP            DMX_URL_ROOT_FULL "control/fixturegroup/"

#define DMX_URL_CONTROL_VENUE_VOLUME_MASTER     DMX_URL_ROOT_FULL "control/venue/volume/master/"
#define DMX_URL_CONTROL_VENUE_VOLUME_MUTE       DMX_URL_ROOT_FULL "control/venue/volume/mute/"

#define DMX_URL_CONTROL_SOUNDSAMPLER_START      DMX_URL_ROOT_FULL "control/soundsampler/start/"
#define DMX_URL_CONTROL_SOUNDSAMPLER_STOP       DMX_URL_ROOT_FULL "control/soundsampler/stop/"
#define DMX_URL_QUERY_SOUNDSAMPLER              DMX_URL_ROOT_FULL "query/soundsampler/"

#define DMX_URL_CONTROL_BEATSAMPLER_START       DMX_URL_ROOT_FULL "control/beatsampler/start/"
#define DMX_URL_CONTROL_BEATSAMPLER_STOP        DMX_URL_ROOT_FULL "control/beatsampler/stop/"
#define DMX_URL_QUERY_BEATSAMPLER               DMX_URL_ROOT_FULL "query/beatsampler/"

#define DMX_URL_DELETE_SCENE                    DMX_URL_ROOT_FULL "delete/scene/"
#define DMX_URL_DELETE_CHASE                    DMX_URL_ROOT_FULL "delete/chase/"
#define DMX_URL_DELETE_FIXTURE                  DMX_URL_ROOT_FULL "delete/fixture/"
#define DMX_URL_DELETE_FIXTUREGROUP             DMX_URL_ROOT_FULL "delete/fixturegroup/"

#define DMX_URL_EDIT_SCENE_COPY_FIXTURES        DMX_URL_ROOT_FULL "edit/scene/copy_fixtures/"
#define DMX_URL_EDIT_VENUE_UPDATE               DMX_URL_ROOT_FULL "edit/venue/update/"
#define DMX_URL_EDIT_SCENE_CREATE               DMX_URL_ROOT_FULL "edit/scene/create/"
#define DMX_URL_EDIT_SCENE_UPDATE               DMX_URL_ROOT_FULL "edit/scene/update/"
#define DMX_URL_EDIT_SCENE_COPY                 DMX_URL_ROOT_FULL "edit/scene/copy/"
#define DMX_URL_EDIT_CHASE_CREATE               DMX_URL_ROOT_FULL "edit/chase/create/"
#define DMX_URL_EDIT_CHASE_UPDATE               DMX_URL_ROOT_FULL "edit/chase/update/"
#define DMX_URL_EDIT_CHASE_COPY                 DMX_URL_ROOT_FULL "edit/chase/copy/"
#define DMX_URL_EDIT_FIXTURE_CREATE             DMX_URL_ROOT_FULL "edit/fixture/create/"
#define DMX_URL_EDIT_FIXTURE_UPDATE             DMX_URL_ROOT_FULL "edit/fixture/update/"

#define DMX_URL_EDIT_VENUE_SAVE                 DMX_URL_ROOT_FULL "edit/venue/save/"
#define DMX_URL_EDIT_VENUE_LOAD                 DMX_URL_ROOT_FULL "edit/venue/load/"
#define DMX_URL_EDIT_VENUE_NEW                  DMX_URL_ROOT_FULL "edit/venue/new/"

#define DMX_URL_EDIT_FIXTUREGROUP_CREATE        DMX_URL_ROOT_FULL "edit/fixturegroup/create/"
#define DMX_URL_EDIT_FIXTUREGROUP_UPDATE        DMX_URL_ROOT_FULL "edit/fixturegroup/update/"

#define DMX_URL_VENUE_UPLOAD                    DMX_URL_ROOT_FULL "venue/upload/"

// ----------------------------------------------------------------------------
//
DMXHttpFull::DMXHttpFull(void) :
    m_sound_sampler( 2 )
{
    m_rest_get_handlers[ DMX_URL_QUERY_SCENES ] = &DMXHttpFull::query_scenes;
    m_rest_get_handlers[ DMX_URL_QUERY_CHASES ] = &DMXHttpFull::query_chases;
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURES ] = &DMXHttpFull::query_fixtures;
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_STATUS ] = &DMXHttpFull::query_venue_status;
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_DESCRIBE ] = &DMXHttpFull::query_venue_describe;
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURE_DEFINITIONS ] = &DMXHttpFull::query_fixture_definitions;

    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_BLACKOUT ] = &DMXHttpFull::control_venue_blackout;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_WHITEOUT ] = &DMXHttpFull::control_venue_whiteout;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_MASTERDIMMER ] = &DMXHttpFull::control_venue_masterdimmer;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_STROBE ] = &DMXHttpFull::control_venue_strobe;
    m_rest_get_handlers[ DMX_URL_CONTROL_SCENE_SHOW ] = &DMXHttpFull::control_scene_show;
    m_rest_get_handlers[ DMX_URL_CONTROL_CHASE_SHOW ] = &DMXHttpFull::control_chase_show;

    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_VOLUME_MASTER ] = &DMXHttpFull::control_master_volume;
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_VOLUME_MUTE ] = &DMXHttpFull::control_mute_volume;   
    m_rest_get_handlers[ DMX_URL_CONTROL_SOUNDSAMPLER_START ] = &DMXHttpFull::control_soundsampler_start;
    m_rest_get_handlers[ DMX_URL_CONTROL_SOUNDSAMPLER_STOP ] = &DMXHttpFull::control_soundsampler_stop;
    m_rest_get_handlers[ DMX_URL_QUERY_SOUNDSAMPLER ] = &DMXHttpFull::query_soundsampler;
    m_rest_get_handlers[DMX_URL_CONTROL_BEATSAMPLER_STOP ] = &DMXHttpFull::control_beatsampler_stop;
    m_rest_get_handlers[DMX_URL_QUERY_BEATSAMPLER ] = &DMXHttpFull::query_beatsampler;

    m_rest_get_handlers[ DMX_URL_DELETE_SCENE ] = &DMXHttpFull::delete_scene;
    m_rest_get_handlers[ DMX_URL_DELETE_CHASE ] = &DMXHttpFull::delete_chase;
    m_rest_get_handlers[ DMX_URL_DELETE_FIXTURE ] = &DMXHttpFull::delete_fixture;
    m_rest_get_handlers[ DMX_URL_DELETE_FIXTUREGROUP ] = &DMXHttpFull::delete_fixturegroup;

    // POST request handlers
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTURE_CHANNELS ] = &DMXHttpFull::control_fixture_channels;
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTURE ] = &DMXHttpFull::control_fixture;
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTUREGROUP ] = &DMXHttpFull::control_fixture_group;

    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_COPY_FIXTURES ] = &DMXHttpFull::edit_scene_copy_fixtures;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_UPDATE ] = &DMXHttpFull::edit_venue_update;
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_CREATE  ] = &DMXHttpFull::edit_scene_create;
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_UPDATE ] = &DMXHttpFull::edit_scene_update;
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_COPY ] = &DMXHttpFull::edit_scene_copy;
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_CREATE  ] = &DMXHttpFull::edit_chase_create;
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_UPDATE ] = &DMXHttpFull::edit_chase_update;
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_COPY ] = &DMXHttpFull::edit_chase_copy;
    m_rest_post_handlers[ DMX_URL_EDIT_FIXTURE_CREATE ] = &DMXHttpFull::edit_fixture_create;
    m_rest_post_handlers[ DMX_URL_EDIT_FIXTURE_UPDATE ] = &DMXHttpFull::edit_fixture_update;

    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_SAVE ] = &DMXHttpFull::edit_venue_save;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_LOAD ] = &DMXHttpFull::edit_venue_load;
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_NEW ] = &DMXHttpFull::edit_venue_new;

    m_rest_post_handlers[DMX_URL_EDIT_FIXTUREGROUP_CREATE ] = &DMXHttpFull::edit_fixturegroup_create;
    m_rest_post_handlers[DMX_URL_EDIT_FIXTUREGROUP_UPDATE ] = &DMXHttpFull::edit_fixturegroup_update;

    m_rest_post_handlers[ DMX_URL_VENUE_UPLOAD ] = &DMXHttpFull::venue_upload;

    m_rest_post_handlers[ DMX_URL_CONTROL_BEATSAMPLER_START ] = &DMXHttpFull::control_beatsampler_start;
}

// ----------------------------------------------------------------------------
//
DMXHttpFull::~DMXHttpFull(void)
{
    m_sound_sampler.detach();
}

// ----------------------------------------------------------------------------
//
DWORD DMXHttpFull::processGetRequest( HttpWorkerThread* worker )
{
    CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    int pos = path.Find( '?' );
    if ( pos != -1 )                                        // Remove query string
       path = path.Left( pos );

    CString prefix( path );
    if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
        prefix += "/";

    if ( prefix == DMX_URL_ROOT_FULL ) {                  // Redirect to full index page
        return worker->sendRedirect( DMX_URL_FULL_HOME );
    }

    // Invoke the approriate handler
    RestGetHandlerFunc func = NULL;
    UINT len = 0;
    for ( RestGetHandlerMap::iterator it=m_rest_get_handlers.begin(); it != m_rest_get_handlers.end(); ++it ) {
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

        if ( path == "full/venue/download/" ) {
            CString venue_xml, venue_xml_name;
            studio.writeVenueToString( venue_xml );
            venue_xml_name.Format( "%s.xml", studio.getVenue()->getName() == NULL ? "venue" : studio.getVenue()->getName() );
            return worker->sendAttachment( (LPBYTE)(LPCSTR)venue_xml, venue_xml.GetLength(), "text/xml", (LPCSTR)venue_xml_name ); 
        }

        return worker->sendFile( (LPCSTR)path, this ); 
    }

    return worker->error_404();
}

// ----------------------------------------------------------------------------
//
DWORD DMXHttpFull::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    CString url_path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    if ( url_path.GetLength() > 0 && url_path[url_path.GetLength()-1] != '/' )
        url_path += "/";

    CString contentType( worker->getContentType() );
    int semi = contentType.Find( ";" );
    if ( semi != -1 )
        contentType = contentType.Left( semi );

    printf( "url=%s\nContentType=%s\nrequest=%s\n", url_path, (LPCSTR)contentType, contents );

    // Only accept JSON and multipart content types
    if ( contentType != "application/json" && contentType != "multipart/form-data" )
        return worker->sendResponse( 415, "Unsupported content type", NULL );

    // Invoke the approriate handler
    RestPostHandlerFunc func = NULL;
    UINT len = 0;
    for ( RestPostHandlerMap::iterator it=m_rest_post_handlers.begin(); it != m_rest_post_handlers.end(); ++it ) {
        if ( url_path.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
            func = it->second;
            len = strlen(it->first);
        }
    }

    if ( func != NULL ) {
        CString response;
        if ( (this->*func)( response, (LPCSTR)contents, size, (LPCSTR)contentType ) )
            return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
    }

    return worker->error_501();
}

// ----------------------------------------------------------------------------
//
bool CompareObjectNumber( DObject* o1,  DObject* o2 ) {
    return o1->getNumber() < o2->getNumber();
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::control_soundsampler_start( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    m_sound_sampler.attach( studio.getVenue()->getAudio() );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::control_soundsampler_stop( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    m_sound_sampler.detach();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::query_soundsampler( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    if ( !m_sound_sampler.isAttached() )
         m_sound_sampler.attach( studio.getVenue()->getAudio() );

    ULONG sample_number;
    SampleSet samples = m_sound_sampler.getSampleSet( sample_number );

    response.Format( "{ \"sample_number\":%lu, \"audio_data\": [", sample_number );

    bool first = true;
    for ( SampleSet::iterator it=samples.begin(); it != samples.end(); it++, first=false ) {
        int value = 100 + (*it).getDB();
        if ( value < 0 )
            value = 0;
        
        if ( !first )
            response.AppendFormat( ",%d", value );
        else
            response.AppendFormat( "%d", value );
    }

    response.Append( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::control_beatsampler_start( CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    PARSER_LIST bins;

    m_beat_sampler.removeAllFrequencyEvents();
    m_beats.clear();

    m_beat_sampler.attach( studio.getVenue()->getAudio(), 64 );

    try {
        parser.parse( data );

        bins = parser.get<PARSER_LIST>( "" );

        for ( PARSER_LIST::iterator it=bins.begin(); it != bins.end(); ++it ) {
            SimpleJsonParser& bin_parser = (*it);

            unsigned start_freq = bin_parser.get<unsigned>( "start_freq" );
            unsigned end_freq = bin_parser.get<unsigned>( "end_freq" );

            m_beats.push_back( BeatBin( start_freq, end_freq ) );
        }
    }
    catch ( std::exception& e ) {
        DMXStudio::log( StudioException( "JSON parser error (%s) data (%s)", e.what(), data ) );
        return false;
    }

    for ( BeatBinArray::iterator it=m_beats.begin(); it != m_beats.end(); ++it )
        m_beat_sampler.addFrequencyEvent( (*it).getEvent(), (*it).getStartFreq(),  (*it).getEndFreq() );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::control_beatsampler_stop( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    m_beat_sampler.removeAllFrequencyEvents();
    m_beat_sampler.detach();
    m_beats.clear();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpFull::query_beatsampler( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    if ( !m_beat_sampler.isAttached() )
         return false;

    response.Format( "[" );

    for ( BeatBinArray::iterator it=m_beats.begin(); it != m_beats.end(); ++it ) {
        if ( it != m_beats.begin() )
            response.Append( "," );

        response.AppendFormat( "{ \"start_freq\":%u, \"end_freq\":%u, \"beat\":%s }",
                    (*it).getStartFreq(), (*it).getEndFreq(), (*it).isBeat( ) ? "true" : "false" );
    }

    response.Append( "]" );

    return true;
}

























#if 0

// ----------------------------------------------------------------------------
//
bool DMXHttpUI::control_animation_speed( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    DWORD sample_rate_ms;
        
    if ( sscanf_s( data, "%lu", &sample_rate_ms ) != 1 )
        return false;

    studio.getVenue()->setAnimationSampleRate( sample_rate_ms );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpUI::query_sound( CString& response, LPCSTR data )
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
                encodeJsonString( track_name ), length, remaining, paused ? 1 : 0 );
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
bool DMXHttpMobile::control_music_play_track( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT        playlist_number, track_number;
    unsigned    queue;

    if ( sscanf_s( data, "%lu/%lu/%u", &playlist_number, &track_number, &queue ) != 3 )
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
bool DMXHttpMobile::control_music_play_playlist( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT       playlist_number;
    unsigned    queue;

    if ( sscanf_s( data, "%lu/%u", &playlist_number, &queue ) != 2 )
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
bool DMXHttpMobile::query_music_playlists( CString& response, LPCSTR data )
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
        response.AppendFormat( " { \"id\": %lu, \"name\": \"%s\" }", playlist_number, encodeJsonString( name ) );
    }
    
    response.Append( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::query_music_playlist_tracks( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    UINT playlist_number;

    if ( sscanf_s( data, "%u", &playlist_number ) != 1 )
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
        response.AppendFormat( " { \"id\": %lu, \"name\": \"%s\" }", track_number, encodeJsonString( name ) );
    }
    
    response.Append( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_back( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->backTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_forward( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->forwardTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_stop( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->stopTrack( );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_pause( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( true );
    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpMobile::control_music_track_play( CString& response, LPCSTR data )
{
    if ( !studio.hasMusicPlayer() )
        return false;

    studio.getMusicPlayer()->pauseTrack( false );
    return true;
}




#endif