/* 
    Copyright (C) 2012-2016 Robert DeSantis
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

#include "HttpRestServices.h"
#include "SimpleJsonBuilder.h"
#include "Venue.h"

// ----------------------------------------------------------------------------
//
HttpRestServices::HttpRestServices(void)
{
    m_rest_get_handlers[ DMX_URL_QUERY_SCENE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_scene );
    m_rest_get_handlers[ DMX_URL_QUERY_CHASE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_chase );
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_fixture );

    m_rest_get_handlers[ DMX_URL_QUERY_SCENES ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_scenes );
    m_rest_get_handlers[ DMX_URL_QUERY_CHASES ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_chases );
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURES ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_fixtures );
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_STATUS ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_venue_status );

    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_BLACKOUT ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_venue_blackout );
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_WHITEOUT ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_venue_whiteout );
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_MUSIC_MATCH ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_venue_music_match );
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_MASTERDIMMER ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_venue_masterdimmer );
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_STROBE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_venue_strobe );
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_WHITEOUT_COLOR ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_venue_whiteout_color );
    m_rest_get_handlers[ DMX_URL_CONTROL_SCENE_SHOW ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_scene_show );
    m_rest_get_handlers[ DMX_URL_CONTROL_SCENE_STAGE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_scene_stage );
    m_rest_get_handlers[ DMX_URL_CONTROL_CHASE_SHOW ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_chase_show );
    m_rest_get_handlers[ DMX_URL_CONTROL_FIXTURE_CAPTURE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_fixture_capture );
    m_rest_get_handlers[ DMX_URL_CONTROL_FIXTURE_RELEASE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_fixture_release );
    m_rest_get_handlers[ DMX_URL_CONTROL_FIXTURE_CHANNEL ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_fixture_channel );

    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_VOLUME_MASTER ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_master_volume );
    m_rest_get_handlers[ DMX_URL_CONTROL_VENUE_VOLUME_MUTE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_mute_volume );   
    m_rest_get_handlers[ DMX_URL_CONTROL_SOUNDSAMPLER_START ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_soundsampler_start );
    m_rest_get_handlers[ DMX_URL_CONTROL_SOUNDSAMPLER_STOP ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_soundsampler_stop );
    m_rest_get_handlers[ DMX_URL_QUERY_SOUNDSAMPLER ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_soundsampler );
    m_rest_get_handlers[ DMX_URL_CONTROL_BEATSAMPLER_STOP ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_beatsampler_stop );
    m_rest_get_handlers[ DMX_URL_QUERY_BEATSAMPLER ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_beatsampler );
    m_rest_get_handlers[ DMX_URL_CONTROL_ANIMATION_SPEED ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_animation_speed );

    m_rest_get_handlers[ DMX_URL_DELETE_SCENE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::delete_scene );
    m_rest_get_handlers[ DMX_URL_DELETE_CHASE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::delete_chase );
    m_rest_get_handlers[ DMX_URL_DELETE_FIXTURE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::delete_fixture );
    m_rest_get_handlers[ DMX_URL_DELETE_FIXTUREGROUP ] = RestService<RestGetHandlerFunc>( &HttpRestServices::delete_fixturegroup );

    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_BACK ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_track_back, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_FORWARD ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_track_forward, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_STOP ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_track_stop, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_PAUSE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_track_pause, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_TRACK_PLAY ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_track_play,PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_PLAYLISTS ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_playlists, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_PLAYLIST_TRACKS ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_playlist_tracks, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_QUEUED ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_queued, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_PLAYED ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_played, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_PLAY_TRACK ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_play_track, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_QUEUE_TRACK ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_queue_track, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_CONTROL_MUSIC_PLAY_PLAYLIST ] = RestService<RestGetHandlerFunc>( &HttpRestServices::control_music_play_playlist, PLAYER_AND_VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_TRACK_ANALYZE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_track_analysis, PLAYER_AND_VENUE_REQUIRED );

    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_MATCHER ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_matcher );
    m_rest_get_handlers[ DMX_URL_QUERY_MUSIC_MATCHER_SEARCH ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_music_matcher_search );

    m_rest_get_handlers[ DMX_URL_EVENTS ] = RestService<RestGetHandlerFunc>( &HttpRestServices::fetch_events, VENUE_NOT_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_FIXTURE_DEFINITIONS ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_fixture_definitions, VENUE_NOT_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_DESCRIBE ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_venue_describe, VENUE_REQUIRED );
    m_rest_get_handlers[ DMX_URL_QUERY_VENUE_LAYOUT ] = RestService<RestGetHandlerFunc>( &HttpRestServices::query_venue_layout, VENUE_REQUIRED );

    // POST request handlers

    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTURE_CHANNELS ] = RestService<RestPostHandlerFunc>( &HttpRestServices::control_fixture_channels );
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTURE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::control_fixture );
    m_rest_post_handlers[ DMX_URL_CONTROL_FIXTUREGROUP ] = RestService<RestPostHandlerFunc>( &HttpRestServices::control_fixture_group );

    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_COPY_FIXTURES ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_scene_copy_fixtures );
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_CREATE  ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_scene_create );
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_UPDATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_scene_update );
    m_rest_post_handlers[ DMX_URL_EDIT_SCENE_COPY ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_scene_copy );
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_CREATE  ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_chase_create );
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_UPDATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_chase_update );
    m_rest_post_handlers[ DMX_URL_EDIT_CHASE_COPY ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_chase_copy );
    m_rest_post_handlers[ DMX_URL_EDIT_FIXTURE_CREATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_fixture_create );
    m_rest_post_handlers[ DMX_URL_EDIT_FIXTURE_UPDATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_fixture_update );
    m_rest_post_handlers[ DMX_URL_EDIT_MUSIC_MATCHER_LOAD ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_music_matcher_load );
    m_rest_post_handlers[ DMX_URL_EDIT_MUSIC_MATCHER_UPDATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_music_matcher_update );

    m_rest_post_handlers[DMX_URL_EDIT_FIXTUREGROUP_CREATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_fixturegroup_create );
    m_rest_post_handlers[DMX_URL_EDIT_FIXTUREGROUP_UPDATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_fixturegroup_update );

    m_rest_post_handlers[ DMX_URL_CONTROL_BEATSAMPLER_START ] = RestService<RestPostHandlerFunc>( &HttpRestServices::control_beatsampler_start );

    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_LAYOUT_SAVE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_venue_layout_save, VENUE_REQUIRED );

    m_rest_post_handlers[ DMX_URL_CONTROL_MUSIC_PLAYER_LOGIN ] = RestService<RestPostHandlerFunc>( &HttpRestServices::player_login, PLAYER_AND_VENUE_REQUIRED );

    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_UPDATE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_venue_update, VENUE_REQUIRED );
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_SAVE ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_venue_save, VENUE_REQUIRED );
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_LOAD ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_venue_load, VENUE_NOT_REQUIRED );
    m_rest_post_handlers[ DMX_URL_EDIT_VENUE_NEW ] = RestService<RestPostHandlerFunc>( &HttpRestServices::edit_venue_new, VENUE_NOT_REQUIRED );
    m_rest_post_handlers[ DMX_URL_VENUE_UPLOAD ] = RestService<RestPostHandlerFunc>( &HttpRestServices::venue_upload, VENUE_NOT_REQUIRED );
}

// ----------------------------------------------------------------------------
//
HttpRestServices::~HttpRestServices(void)
{
}

// ----------------------------------------------------------------------------
//
DWORD HttpRestServices::processGetRequest( HttpWorkerThread* worker )
{
    try {
        CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
        int pos = path.Find( '?' );
        if ( pos != -1 )                                        // Remove query string
           path = path.Left( pos );

        CString prefix( path );
        if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
            prefix += "/";

        // Invoke the approriate handler
        RestService<RestGetHandlerFunc >* service = NULL;

        UINT len = 0;
        for ( RestGetHandlerMap::iterator it=m_rest_get_handlers.begin(); it != m_rest_get_handlers.end(); ++it ) {
            if ( prefix.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
                service = &it->second;
                len = strlen(it->first);
            }
        }

        if ( service != NULL ) {
            Venue* venue = getStudio( service->m_flags );
            CString response;

            if ( !(this->*service->m_handler)( venue, worker->getSession(), response, path.Mid( len ) ) )
                return worker->error_501();

            return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
        }

        // Perhaps this is a file request
        if ( path.Find( DMX_URL_ROOT, 0 ) == 0 ) {
            path.Replace( DMX_URL_ROOT, "" );

            if ( path == "rest/venue/download/" ) {
                CString venue_xml, venue_xml_name;
                studio.writeVenueToString( venue_xml );
                venue_xml_name.Format( "%s.xml", studio.getVenue()->getName() == NULL ? "venue" : studio.getVenue()->getName() );
                return worker->sendAttachment( (LPBYTE)(LPCSTR)venue_xml, venue_xml.GetLength(), "text/xml", (LPCSTR)venue_xml_name ); 
            }

            return worker->sendFile( (LPCSTR)path, this ); 
        }

        return worker->error_404();
    }
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501();
    }
}

// ----------------------------------------------------------------------------
//
DWORD HttpRestServices::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    try {
        CString url_path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
        if ( url_path.GetLength() > 0 && url_path[url_path.GetLength()-1] != '/' )
            url_path += "/";

        CString contentType( worker->getContentType() );
        int semi = contentType.Find( ";" );
        if ( semi != -1 )
            contentType = contentType.Left( semi );

        if ( studio.isDebug() )
            studio.log( "url=%s\nContentType=%s\nrequest=%s\n", url_path, (LPCSTR)contentType, contents );

        // Only accept JSON and multipart content types
        if ( contentType != "application/json" && contentType != "multipart/form-data" )
            return worker->sendResponse( 415, "Unsupported content type", NULL );

        // Invoke the approriate handler
        RestService<RestPostHandlerFunc>* service = NULL;

        UINT len = 0;
        for ( RestPostHandlerMap::iterator it=m_rest_post_handlers.begin(); it != m_rest_post_handlers.end(); ++it ) {
            if ( url_path.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
                service = &it->second;
                len = strlen(it->first);
            }
        }

        if ( service != NULL ) {
            Venue* venue = getStudio( service->m_flags );
            CString response;

            if ( (this->*service->m_handler)( venue, worker->getSession(), response, (LPCSTR)contents, size, (LPCSTR)contentType ) )
                return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
        }

        return worker->error_501();
    }
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501();
    }
}

// ----------------------------------------------------------------------------
//
Venue* HttpRestServices::getStudio( ServiceFlags flags ) {
    if ( flags & VENUE_NOT_REQUIRED )
        return NULL;

    Venue* venue = studio.getVenue();
    if ( (flags & VENUE_REQUIRED) && venue == NULL )
        throw StudioException( "Service requires a venue" );

    if ( (flags & RUNNING_REQUIRED) && !venue->isRunning() )
        throw StudioException( "Service requires a running venue" );

    if ( (flags & PLAYER_REQUIRED) && !studio.hasMusicPlayer() )
        throw StudioException( "Service requires music player" );

    return venue;
}

// ----------------------------------------------------------------------------
//
bool CompareObjectNumber( DObject* o1,  DObject* o2 ) {
    return o1->getNumber() < o2->getNumber();
}
