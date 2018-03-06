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

#include "HttpRestServices.h"
#include "Venue.h"

void sceneToJson( Venue* venue, JsonBuilder& json, Scene* scene );

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID uid;
    if ( sscanf_s( data, "%lu", &uid ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    Scene* scene = venue->getScene( uid );
    if ( scene == NULL )
        throw RestServiceException( "Invalid scene UID" );

    JsonBuilder json( response );
    json.startArray();
    sceneToJson( venue, json, scene );
    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_scenes( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    ScenePtrArray scenes = venue->getScenes();
    std::sort( scenes.begin(), scenes.end(), CompareObjectNumber );

    JsonBuilder json( response );
    json.startArray();

    for ( ScenePtrArray::iterator it=scenes.begin(); it != scenes.end(); it++ ) {
        sceneToJson( venue, json, (*it) );
    }

    json.endArray();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::delete_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    UID scene_id;
        
    if ( sscanf_s( data, "%lu", &scene_id ) != 1 )
        throw RestServiceException( "Invalid service arguments" );

    if ( !venue->deleteScene( scene_id ) )
        throw RestServiceException( "Invalid scene UID" );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_scene_copy_fixtures( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;
    bool clear;
    bool keep_groups;
    bool keep_animations;
    UIDArray scene_uids;
    UIDArray actor_uids;

    try {
        parser.parse( data );

        clear = parser.get<bool>( "clear" );
        scene_uids = parser.get<UIDArray>( "scene_ids" );
        keep_groups = parser.get<bool>( "keep_groups" );
        keep_animations = parser.get<bool>( "keep_animations" );
        actor_uids = parser.get<UIDArray>( "fixture_ids" );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    venue->moveDefaultFixturesToScene( scene_uids, actor_uids, keep_groups, clear, keep_animations );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::edit_scene( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, EditMode mode )
{
    SimpleJsonParser parser;
    UID scene_id;
    CString name, description;
    SceneNumber number;
    BPMRating bpm_rating;
    Scene* scene = NULL;
    JsonNodePtrArray actorParsers;
    ActorList actors;
    JsonNodePtrArray animationParsers;
    AnimationReferenceArray animation_refs;
    Acts acts;
     
    try {
        parser.parse( data );

        scene_id = parser.get<ULONG>( "id" );
        name = parser.get<CString>( "name" );
        description = parser.get<CString>( "description" );
        number = parser.get<ULONG>( "number" );
        bpm_rating = (BPMRating)parser.get<UINT>( "bpm_rating" );
        actorParsers = parser.getObjects( "actors" );
        animationParsers = parser.getObjects("animation_refs");
        acts = parser.get<Acts>( "acts" );

        for ( JsonNode* actor : actorParsers ) {
			channel_value channel_values[DMX_PACKET_SIZE];
            size_t channels = 0;

            for ( JsonNode* channel : actor->getObjects( "channels" ) ) {
                channel_values[channels++] = channel->get<BYTE>( "value" );
            }

            if ( actor->get<bool>( "is_group" ) ) {
                FixtureGroup* group = venue->getFixtureGroup( actor->get<DWORD>( "id" ) );
                SceneActor sceneActor( venue, group );
                sceneActor.setBaseChannelValues( channels, channel_values );
                sceneActor.setPaletteReferences( actor->getArrayAsVector<UID>( "palette_refs" ) );
                actors.push_back( sceneActor );
            }
            else {
                Fixture* fixture = venue->getFixture( actor->get<DWORD>( "id" ) );
                SceneActor sceneActor( fixture );
                sceneActor.setBaseChannelValues( channels, channel_values );
                sceneActor.setPaletteReferences( actor->getArrayAsVector<UID>( "palette_refs" ) );
                actors.push_back( sceneActor );
            }
        }

        for ( JsonNode* anim : animationParsers )
            animation_refs.emplace_back( anim->get<ULONG>( "id" ), anim->get<UIDArray>( "actors" ) );
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    if ( scene_id != 0 ) {
        scene = venue->getScene( scene_id );
        if ( !scene )
            throw RestServiceException( "Invalid scene UID" );
    }

    // Make sure number is unique
    if ( mode != UPDATE || (scene && number != scene->getNumber()) ) {
        if ( venue->getSceneByNumber( number ) != NOUID ) 
            throw RestServiceException( "Scene number must be unique" );
    }

    switch ( mode ) {
        case NEW: {
            Scene new_scene;
            scene_id = venue->addScene( new_scene );
            scene = venue->getScene( scene_id );
            break;
        }

        case COPY: {
            Scene new_scene( *scene );
            new_scene.setUID( NOUID );
            scene_id = venue->addScene( new_scene );
            scene = venue->getScene( scene_id );
        }
        
        // Fall through 

        case UPDATE:
            break;
    }

    scene->setName( name );
    scene->setSceneNumber( number );
    scene->setBPMRating( bpm_rating );
    scene->setDescription( description );
    scene->setActs( acts );
    scene->setActors( actors );

    // Add animation references to the scene and handle private animations
    venue->setSceneAnimationReferences( scene, animation_refs, mode != UPDATE );

    // Compute final channel values for all actors
    venue->computeActorFinalValues( scene );

    if ( mode == NEW )              // Do this LAST as it will free unreferenced animation we want to keep
        venue->clearAllCapturedActors();

    if ( mode != UPDATE )            // Switch to new scene
        venue->selectScene( scene->getUID() );
    else
        venue->sceneUpdated( scene->getUID() );
}

// ----------------------------------------------------------------------------
//
void sceneToJson( Venue* venue, JsonBuilder& json, Scene* scene )
{
    json.startObject();
    json.add( "id", scene->getUID() );
    json.add( "number", scene->getSceneNumber() );
    json.add( "name", scene->getName() );
    json.add( "bpm_rating", scene->getBPMRating() );
    json.add( "description", scene->getDescription() );
    json.add( "created", scene->getCreated() );
    json.add( "is_default", (scene == venue->getDefaultScene()) );
    json.add( "is_running", (venue->getCurrentSceneUID() == scene->getUID()) );
    json.addArray<Acts>( "acts", scene->getActs() );

    json.startArray( "actors" );

    ActorPtrArray actors = scene->getActors();

    for ( ActorPtrArray::iterator it=actors.begin(); it != actors.end(); ++it ) {
        Fixture* fixture = NULL;

        json.startObject();
        json.add( "id", (*it)->getActorUID() );
        json.add( "is_group", (*it)->isGroup() );

        if ( (*it)->isGroup() ) {
            fixture = venue->getGroupRepresentative( (*it)->getActorUID() );
        }
        else {
            fixture = venue->getFixture( (*it)->getActorUID() );
            json.add( "address", (int)fixture->getAddress() );
        }

        json.addArray<UIDArray>( "palette_refs", (*it)->getPaletteReferences() );

        json.startArray( "channels" );
        if ( fixture != NULL ) {
            for ( size_t channel=0; channel < fixture->getNumChannels(); channel++ ) {
                Channel* ch = fixture->getChannel( channel );
				channel_value value = (*it)->getBaseChannelValue( channel );
                ChannelValueRange* range = ch->getRange( value );
                LPCSTR range_name = range ? range->getName() : "";

                json.startObject();
                json.add( "channel", (int)channel );
                json.add( "name", ch->getName() );
                json.add( "value", value );
                json.add( "range_name", range_name );
                json.endObject();
            }
        }
        json.endArray( "channels" );

        json.endObject();
    }

    json.endArray( "actors" );

    json.startArray( "animation_refs" );
    for ( AnimationReference& animation : scene->animations() ) {
        json.startObject();
        json.add( "id", animation.getUID() );
        json.addArray<UIDArray>( "actors", animation.getActors() );
        json.endObject();
    }
    json.endArray( "animation_refs" );

    json.endObject();
}
