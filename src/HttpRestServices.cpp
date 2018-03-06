/* 
    Copyright (C) 2012-2017 Robert DeSantis
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
HttpRestServices::HttpRestServices( UINT port ) :
    IRequestHandler( port )
{
    addService( "events/",                           &HttpRestServices::fetch_events, VENUE_NOT_REQUIRED );

    addService( "query/scene/",                      &HttpRestServices::query_scene );
    addService( "query/chase/",                      &HttpRestServices::query_chase );
    addService( "query/fixture/",                    &HttpRestServices::query_fixture );
    addService( "query/animation/",                  &HttpRestServices::query_animation );
    addService( "query/palette/",                    &HttpRestServices::query_palette );

    addService( "query/scenes/",                     &HttpRestServices::query_scenes );
    addService( "query/chases/",                     &HttpRestServices::query_chases );
    addService( "query/fixtures/",                   &HttpRestServices::query_fixtures );
    addService( "query/venue/status/",               &HttpRestServices::query_venue_status );
    addService( "query/animations/",                 &HttpRestServices::query_animations );
    addService( "query/palettes/",                   &HttpRestServices::query_palettes );

    addService( "delete/scene/",                     &HttpRestServices::delete_scene );
    addService( "delete/chase/",                     &HttpRestServices::delete_chase );
    addService( "delete/fixture/",                   &HttpRestServices::delete_fixture );
    addService( "delete/fixturegroup/",              &HttpRestServices::delete_fixturegroup );
    addService( "delete/animation/",                 &HttpRestServices::delete_animation );
    addService( "delete/palette/",                   &HttpRestServices::delete_palette );

    addService( "query/venue/describe/",             &HttpRestServices::query_venue_describe, VENUE_REQUIRED );
    addService( "query/venue/layout/",               &HttpRestServices::query_venue_layout, VENUE_REQUIRED );
    addService( "query/fixture/definitions/",        &HttpRestServices::query_fixture_definitions, VENUE_NOT_REQUIRED );
    addService( "query/soundsampler/",               &HttpRestServices::query_soundsampler );
    addService( "query/beatsampler/",                &HttpRestServices::query_beatsampler );
    addService( "query/amplitude/",                  &HttpRestServices::query_amplitude );
    addService( "query/music/matcher/",              &HttpRestServices::query_music_matcher );
    addService( "query/music/matcher/search/",       &HttpRestServices::query_music_matcher_search );
    addService( "query/music/track/analyze/",        &HttpRestServices::query_music_track_analysis, PLAYER_AND_VENUE_REQUIRED );
    addService( "query/music/playlists/",            &HttpRestServices::query_music_playlists, PLAYER_AND_VENUE_REQUIRED );
    addService( "query/music/playlist/tracks/",      &HttpRestServices::query_music_playlist_tracks, PLAYER_AND_VENUE_REQUIRED );
    addService( "query/music/videos/",               &HttpRestServices::query_music_videos, PLAYER_AND_VENUE_REQUIRED );
    addService( "query/music/queued/",               &HttpRestServices::query_music_queued, PLAYER_AND_VENUE_REQUIRED );
    addService( "query/music/played/",               &HttpRestServices::query_music_played, PLAYER_AND_VENUE_REQUIRED );
    addService( "query/animationlevels/",            &HttpRestServices::query_animation_levels, VENUE_REQUIRED );
	addService( "query/video/palette/",				 &HttpRestServices::query_video_palette, VENUE_REQUIRED );
    addService( "query/music/player/web_login/",     &HttpRestServices::query_player_web_login, PLAYER_AND_VENUE_REQUIRED );

    addService( "control/venue/blackout/",           &HttpRestServices::control_venue_blackout );
    addService( "control/venue/whiteout/",           &HttpRestServices::control_venue_whiteout );
    addService( "control/venue/masterdimmer/",       &HttpRestServices::control_venue_masterdimmer ); 
    addService( "control/venue/strobe/",             &HttpRestServices::control_venue_strobe ); 
    addService( "control/venue/animationspeed/",     &HttpRestServices::control_venue_animation_speed );
    addService( "control/venue/whiteout/color/",     &HttpRestServices::control_venue_whiteout_color ); 
    addService( "control/venue/whiteout/effect/",    &HttpRestServices::control_venue_whiteout_effect );
    addService( "control/venue/music_match/",        &HttpRestServices::control_venue_music_match );
    addService( "control/scene/show/",               &HttpRestServices::control_scene_show );
    addService( "control/scene/stage/",              &HttpRestServices::control_scene_stage );
    addService( "control/chase/show/",               &HttpRestServices::control_chase_show );
    addService( "control/chase/step/",               &HttpRestServices::control_chase_step );
    addService( "control/venue/volume/master/",      &HttpRestServices::control_master_volume );
    addService( "control/venue/volume/mute/",        &HttpRestServices::control_mute_volume ); 
    addService( "control/soundsampler/start/",       &HttpRestServices::control_soundsampler_start );
    addService( "control/soundsampler/stop/",        &HttpRestServices::control_soundsampler_stop );
    addService( "control/beatsampler/stop/",         &HttpRestServices::control_beatsampler_stop );
    addService( "control/music/track/back/",         &HttpRestServices::control_music_track_back, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/track/forward/",      &HttpRestServices::control_music_track_forward, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/track/stop/",         &HttpRestServices::control_music_track_stop, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/track/pause/",        &HttpRestServices::control_music_track_pause, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/track/play/",         &HttpRestServices::control_music_track_play, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/queue/track/",        &HttpRestServices::control_music_queue_track, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/play/track/",         &HttpRestServices::control_music_play_track, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/music/play/playlist/",      &HttpRestServices::control_music_play_playlist, PLAYER_AND_VENUE_REQUIRED );
	addService( "control/music/queue/playlist/",	 &HttpRestServices::control_music_queue_playlist, PLAYER_AND_VENUE_REQUIRED );
    addService( "control/animation/stop/",           &HttpRestServices::control_animation_stop );
    addService( "control/animation/start/",          &HttpRestServices::control_animation_start );
    addService( "control/animation/test/start",      &HttpRestServices::control_animation_test_start ); 
    addService( "control/animation/test/stop",       &HttpRestServices::control_animation_test_stop ); 
 
    addService( "venue/control/quickscene/stop",     &HttpRestServices::control_quickscene_stop, VENUE_REQUIRED ); 

    // POST request handlers

    addService( "control/fixture/channel/",         &HttpRestServices::control_fixture_channel );
    addService( "control/fixture/palettes/",        &HttpRestServices::control_fixture_palettes );
    addService( "control/fixture/",                 &HttpRestServices::control_fixture );
    addService( "control/beatsampler/start/",       &HttpRestServices::control_beatsampler_start );
    addService( "control/venue/whiteout/palette/",  &HttpRestServices::control_venue_whiteout_palette );
    addService( "control/music/player/login/",      &HttpRestServices::player_login, PLAYER_AND_VENUE_REQUIRED );
	addService( "control/fixture/position/",		&HttpRestServices::control_fixture_position );

	addService( "query/video/search/",				&HttpRestServices::query_video_search, PLAYER_AND_VENUE_REQUIRED);
    addService( "create/video/palette",				&HttpRestServices::create_video_palette, VENUE_REQUIRED);
    addService( "create/image/palette",				&HttpRestServices::create_image_palette, VENUE_REQUIRED);
    addService( "palette/upload/",                  &HttpRestServices::generate_image_palette, VENUE_NOT_REQUIRED );
    addService( "query/video/ids/",                 &HttpRestServices::query_video_ids, PLAYER_AND_VENUE_REQUIRED );

    addService( "edit/scene/copy_fixtures/",        &HttpRestServices::edit_scene_copy_fixtures );
    addService( "edit/scene/create/",               &HttpRestServices::edit_scene_create );
    addService( "edit/scene/update/",               &HttpRestServices::edit_scene_update );
    addService( "edit/scene/copy/",                 &HttpRestServices::edit_scene_copy );
    addService( "edit/chase/create/",               &HttpRestServices::edit_chase_create );
    addService( "edit/chase/update/",               &HttpRestServices::edit_chase_update );
    addService( "edit/chase/copy/",                 &HttpRestServices::edit_chase_copy );
    addService( "edit/fixture/create/",             &HttpRestServices::edit_fixture_create );
    addService( "edit/fixture/update/",             &HttpRestServices::edit_fixture_update );
    addService( "edit/fixturegroup/create/",        &HttpRestServices::edit_fixturegroup_create );
    addService( "edit/fixturegroup/update/",        &HttpRestServices::edit_fixturegroup_update );
    addService( "edit/music/matcher/load/",         &HttpRestServices::edit_music_matcher_load );
    addService( "edit/music/matcher/update/",       &HttpRestServices::edit_music_matcher_update );
    addService( "edit/animation/create/",           &HttpRestServices::edit_animation_create );
    addService( "edit/animation/update/",           &HttpRestServices::edit_animation_update );
    addService( "edit/animation/copy/",             &HttpRestServices::edit_animation_copy );   
    addService( "edit/palette/create/",             &HttpRestServices::edit_palette_create );
    addService( "edit/palette/update/",             &HttpRestServices::edit_palette_update );
    addService( "edit/palette/copy/",               &HttpRestServices::edit_palette_copy );   

    addService( "edit/venue/layout/save/",          &HttpRestServices::edit_venue_layout_save, VENUE_REQUIRED );
    addService( "edit/venue/update/",               &HttpRestServices::edit_venue_update, VENUE_REQUIRED );
    addService( "edit/venue/save/",                 &HttpRestServices::edit_venue_save, VENUE_REQUIRED );
    addService( "edit/venue/load/",                 &HttpRestServices::edit_venue_load, VENUE_NOT_REQUIRED );
    addService( "edit/venue/new/",                  &HttpRestServices::edit_venue_new, VENUE_NOT_REQUIRED );
    addService( "venue/upload/",                    &HttpRestServices::venue_upload, VENUE_NOT_REQUIRED );
	addService( "venue/create/quickscene/",			&HttpRestServices::venue_create_quickscene ); 

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
    CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    int pos = path.Find( '?' );
    if ( pos != -1 )                                        // Remove query string
        path = path.Left( pos );

    try {
        CString prefix( path );
        if ( prefix.GetLength() > 0 && prefix[prefix.GetLength()-1] != '/' )
            prefix += "/";

        // Invoke the approriate handler
        RestService<RestGetHandlerFunc >* service = NULL;
        LPCSTR handler_name;

        UINT len = 0;
        for ( RestGetHandlerMap::iterator it=m_rest_get_handlers.begin(); it != m_rest_get_handlers.end(); ++it ) {
            if ( prefix.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
                service = &it->second;
                handler_name = it->first;
                len = strlen(it->first);
            }
        }

        if ( service != NULL ) {
            Venue* venue = getStudio( service->m_flags, handler_name );
            CString response;
        
            (this->*service->m_handler)( venue, worker->getSession(), response, path.Mid( len ) );

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
    catch ( RestServiceException& rest_error ) {
        rest_error.setPath( path );

        DMXStudio::log( rest_error );
        return worker->error_501( rest_error.what() );
    }
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501( ex.what() );
    }
}

// ----------------------------------------------------------------------------
//
DWORD HttpRestServices::processPostRequest( HttpWorkerThread* worker, BYTE* contents, DWORD size  ) 
{
    CString path( CW2A( worker->getRequest()->CookedUrl.pAbsPath ) );
    if ( path.GetLength() > 0 && path[path.GetLength()-1] != '/' )
        path += "/";

    try {
        CString contentType( worker->getContentType() );
        int semi = contentType.Find( ";" );
        if ( semi != -1 )
            contentType = contentType.Left( semi );

        if ( studio.isDebug() )
            studio.log( "url=%s\nContentType=%s\nrequest=%s\n", path, (LPCSTR)contentType, contents );

        // Only accept JSON and multipart content types
        if ( contentType != "application/json" && contentType != "multipart/form-data" )
            return worker->sendResponse( 415, "Unsupported content type", NULL );

        // Invoke the approriate handler
        RestService<RestPostHandlerFunc>* service = NULL;
        LPCSTR handler_name;

        UINT len = 0;
        for ( RestPostHandlerMap::iterator it=m_rest_post_handlers.begin(); it != m_rest_post_handlers.end(); ++it ) {
            if ( path.Find( it->first, 0 ) == 0 && strlen(it->first) > len ) {
                service = &it->second;
                handler_name = it->first;
                len = strlen(it->first);
            }
        }

        if ( service != NULL ) {
            Venue* venue = getStudio( service->m_flags, handler_name );
            CString response;

            (this->*service->m_handler)( venue, worker->getSession(), response, (LPCSTR)contents, size, (LPCSTR)contentType );

            return worker->sendResponse( 200, "OK", response.GetLength() > 0 ? (LPCSTR)response : NULL );
        }

        throw RestServiceException( "No handler found" );
    }
    catch ( RestServiceException& rest_error ) {
        rest_error.setPath( path );

        DMXStudio::log( rest_error );
        return worker->error_501( rest_error.what() );
    }
    catch( std::exception& ex ) {
        DMXStudio::log( ex );
        return worker->error_501( ex.what() );
    }
}

// ----------------------------------------------------------------------------
//
Venue* HttpRestServices::getStudio( ServiceFlags flags, LPCSTR handler_name ) {
    if ( flags & VENUE_NOT_REQUIRED )
        return NULL;

    Venue* venue = studio.getVenue();
    if ( (flags & VENUE_REQUIRED) && venue == NULL )
        throw RestServiceException( "Service '%s' requires a venue", handler_name );

    if ( (flags & RUNNING_REQUIRED) && !venue->isRunning() )
        throw RestServiceException( "Service '%s' requires a running venue", handler_name );

    if ( (flags & PLAYER_REQUIRED) && !studio.hasMusicPlayer() )
        throw RestServiceException( "Service '%s' requires music player", handler_name );

    return venue;
}

// ----------------------------------------------------------------------------
//
bool CompareObjectNumber( DObject* o1,  DObject* o2 ) {
    return o1->getNumber() < o2->getNumber();
}

// ----------------------------------------------------------------------------
//
bool CompareObjectNumber2( DObject& o1,  DObject& o2 ) {
    return o1.getNumber() < o2.getNumber();
}