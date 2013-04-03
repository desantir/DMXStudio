/* 
Copyright (C) 2012 Robert DeSantis
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

#include "DMXHttpRestServices.h"

#include "Venue.h"

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::query_venue_status( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    response.Format( "{ \"blackout\":%d, \"auto_blackout\":%d, \"dimmer\":%u, \"whiteout\":%d, \"whiteout_strobe\":%u, \"animation_speed\": %lu, \"current_scene\":%lu, \"current_chase\":%lu, ",
        studio.getVenue()->getUniverse()->isBlackout(), 
        studio.getVenue()->isLightBlackout(),
        studio.getVenue()->getMasterDimmer(),
        studio.getVenue()->getWhiteout(),
        studio.getVenue()->getWhiteoutStrobeMS(),
        studio.getVenue()->getAnimationSampleRate( ),
        studio.getVenue()->getCurrentSceneUID(),
        studio.getVenue()->getRunningChase() );

    response.AppendFormat( "\"master_volume\": %u, \"mute\": %s, \"has_music_player\": %s, \"venue_filename\":\"%s\", \"captured_fixtures\": [", 
        studio.getVenue()->getMasterVolume( ),
        studio.getVenue()->isMasterVolumeMute( ) ? "true" : "false",
        studio.hasMusicPlayer() ? "true" : "false",
        encodeHtmlString( studio.getVenueFileName()) );

    UIDArray captured_actors = studio.getVenue()->getDefaultScene()->getActorUIDs();

    for ( UIDArray::iterator it=captured_actors.begin(); it != captured_actors.end(); ++it ) {
        if ( it != captured_actors.begin() )
            response.Append( "," );
        response.AppendFormat( "%lu", (*it) );
    }

    response.Append( "]}" );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_venue_blackout( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    unsigned blackout;
        
    if ( sscanf_s( data, "%u", &blackout ) != 1 )
        return false;

    studio.getVenue()->getUniverse()->setBlackout( blackout ? true : false );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_venue_whiteout( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    int whiteout;

    if ( sscanf_s( data, "%d", &whiteout ) != 1 )
        return false;
    if ( whiteout < 0 || whiteout > 4)
        return false;

    studio.getVenue()->setWhiteout( (WhiteoutMode)whiteout );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_venue_masterdimmer( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    int dimmer;

    if ( sscanf_s( data, "%d", &dimmer ) != 1 )
        return false;
    if ( dimmer < 0 || dimmer > 100 )
        return false;

    studio.getVenue()->setMasterDimmer( dimmer );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_scene_show( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID scene_id;

    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        return false;

    if ( scene_id == 0 )
        scene_id = studio.getVenue()->getDefaultScene() ->getUID();

    Scene* scene = studio.getVenue()->getScene( scene_id );
    if ( !scene )
        return false;

    studio.getVenue()->selectScene( scene_id );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_chase_show( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID chase_id;
        
    if ( sscanf_s( data, "%lu", &chase_id ) != 1 )
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
bool DMXHttpRestServices::control_venue_strobe( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UINT whiteout_strobe_ms;

    if ( sscanf_s( data, "%u", &whiteout_strobe_ms ) != 1 )
        return false;
    if ( whiteout_strobe_ms < 25 || whiteout_strobe_ms > 10000)
        return false;

    studio.getVenue()->setWhiteoutStrobeMS( whiteout_strobe_ms );
    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_fixture_release( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
        
    if ( sscanf_s( data, "%lu", &fixture_id ) != 1 )
        return false;

    if ( fixture_id != 0 )
        studio.getVenue()->releaseActor( fixture_id );
    else
        studio.getVenue()->clearAllCapturedActors();

    studio.getVenue()->loadScene();

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_fixture_channel( CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    UID fixture_id;
    channel_t channel;
    unsigned channel_value;

    if ( sscanf_s( data, "%lu/%u/%u", &fixture_id, &channel, &channel_value ) != 3 )
        return false;

    Fixture* pf = studio.getVenue()->getFixture( fixture_id );
    if ( !pf )
        return false;

    studio.getVenue()->captureAndSetChannelValue( pf, channel, channel_value );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_master_volume( CString& response, LPCSTR data )
{
    UINT master_volume;
        
    if ( sscanf_s( data, "%u", &master_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolume( master_volume );

    return true;
}

// ----------------------------------------------------------------------------
//
bool DMXHttpRestServices::control_mute_volume( CString& response, LPCSTR data )
{
    UINT mute_volume;
        
    if ( sscanf_s( data, "%u", &mute_volume ) != 1 )
        return false;

    studio.getVenue()->setMasterVolumeMute( mute_volume ? true : false );

    return true;
}
