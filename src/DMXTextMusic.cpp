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

#define VERIFY_PLAYER_LOGGED_IN \
            STUDIO_ASSERT( studio.getMusicPlayer()->isLoggedIn(), "User is not logged into the music player" )

// ----------------------------------------------------------------------------
//
void DMXTextUI::musicPlayerLogin()
{
    if ( !studio.hasMusicPlayer() || studio.getMusicPlayer()->isLoggedIn() )
        return;

    InputField username_field( "Music Player Username", studio.getMusicPlayer()->getUsername() );
    InputField password_field( "Music Player Password", "" );
    password_field.setPassword( true );

    Form form( &m_text_io );
    form.setStopOnLastField( false );
    form.add( username_field );
    form.add( password_field );

    m_text_io.printf( "\n" );

    for ( ;; ) {
        if ( !form.play() )
            break;
    
        if ( studio.getMusicPlayer()->signon( username_field.getValue(), password_field.getValue() ) )
            break;

        CString login_error = studio.getMusicPlayer()->getLastPlayerError();
        if ( login_error.GetLength() != 0 )
            m_text_io.printf( "ERROR: %s\n\n", login_error );

        m_text_io.printf( "Unable to logon - try again\n" );

        password_field.setValue( "" );
    }

    password_field.setValue( "               " );
}

// ----------------------------------------------------------------------------
//
class PlaylistField : public NumberedListField
{
    PlayerItems   m_playlists;

public:
    PlaylistField( LPCSTR name ) :
        NumberedListField( name )
    {
        studio.getMusicPlayer()->getPlaylists( m_playlists );

        int key = 1;

        for ( PlayerItems::iterator it=m_playlists.begin(); it != m_playlists.end(); ++it ) {
            CString name = studio.getMusicPlayer()->getPlaylistName( (*it) );
            addKeyValue( key++, name );
        }
        
        setDefaultListValue( 1 );      
    }

    DWORD getPlaylist() {
        return m_playlists[ getListValue()-1 ];
    }
};

// ----------------------------------------------------------------------------
//
class TrackListField : public NumberedListField
{
    DWORD                   m_playlist;
    PlayerItems             m_tracks;
    std::map<int,CString>   m_id_to_link;


public:
    TrackListField( LPCSTR name ) :
        NumberedListField( name )
    {
    }

    void setPlaylist( DWORD playlist ) {
        studio.getMusicPlayer()->getTracks( playlist, m_tracks );

        clear();

        int key = 1;

        for ( PlayerItems::iterator it=m_tracks.begin(); it != m_tracks.end(); ++it ) {
            CString track_name, artist_name, track_link;

            studio.getMusicPlayer()->getTrackInfo( (*it), &track_name, &artist_name, NULL, NULL, NULL, &track_link );

            m_id_to_link[ key ] = track_link;

            track_name.AppendFormat( "by %s", artist_name );

            addKeyValue( key++, track_name );

        }
        
        setDefaultListValue( 1 );  
    }

    DWORD getTrack() {
        return m_tracks.size() == 0 ? NULL : m_tracks[getListValue()-1 ];
    }

    CString getTrackFullName() {
        if ( m_tracks.size() == 0 )
            return "";
            
        return CString( getKeyValue() );
    }

    CString getTrackLink() {
        if ( m_tracks.size() == 0 )
            return "";
            
        return CString( getKeyValue() );
    }
};

// ----------------------------------------------------------------------------
//
void DMXTextUI::listPlaylists()
{
    VERIFY_PLAYER_LOGGED_IN;

    PlayerItems playlists;
    studio.getMusicPlayer()->getPlaylists( playlists );

    for ( PlayerItems::iterator it=playlists.begin(); it != playlists.end(); ++it )
        m_text_io.printf( "   %s\n", studio.getMusicPlayer()->getPlaylistName( (*it) ) );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::listTracks()
{
    VERIFY_PLAYER_LOGGED_IN;

    PlaylistField playlist_field( "Playlist" );

    Form form( &m_text_io );
    form.add( playlist_field );

    if ( form.play() ) {
        PlayerItems tracks;

        studio.getMusicPlayer()->getTracks( playlist_field.getPlaylist(), tracks );

        for ( PlayerItems::iterator it=tracks.begin(); it != tracks.end(); ++it ) {
            CString track_name = studio.getMusicPlayer()->getTrackFullName( (*it) );
            m_text_io.printf( "%s\n", track_name );
        }
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::playTrack()
{
    VERIFY_PLAYER_LOGGED_IN;

    selectTrack( false );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::queueTrack()
{
    VERIFY_PLAYER_LOGGED_IN;

    selectTrack( true );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::queuePlaylist()
{
    VERIFY_PLAYER_LOGGED_IN;

    selectPlaylist( true );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::playPlaylist()
{
    VERIFY_PLAYER_LOGGED_IN;

    selectPlaylist( false );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::pauseTrack()
{
    VERIFY_PLAYER_LOGGED_IN;

    studio.getMusicPlayer()->pauseTrack( !studio.getMusicPlayer()->isTrackPaused() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::stopTrack()
{
    VERIFY_PLAYER_LOGGED_IN;

    studio.getMusicPlayer()->stopTrack( );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::forwardTrack()
{
    VERIFY_PLAYER_LOGGED_IN;

    studio.getMusicPlayer()->forwardTrack( );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::backTrack()
{
    VERIFY_PLAYER_LOGGED_IN;

    studio.getMusicPlayer()->backTrack( );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::showQueuedTracks()
{
    VERIFY_PLAYER_LOGGED_IN;

    PlayerItems queued_tracks;
    studio.getMusicPlayer()->getQueuedTracks( queued_tracks );

    for ( PlayerItems::iterator it=queued_tracks.begin(); it != queued_tracks.end(); ++it )
        m_text_io.printf( "   %s\n", studio.getMusicPlayer()->getTrackFullName( (*it) ) );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::selectPlaylist( bool queue )
{
    VERIFY_PLAYER_LOGGED_IN;

    PlaylistField playlist_field( "Playlist" );

    Form form( &m_text_io );
    form.add( playlist_field );

    if ( form.play() ) {
        studio.getMusicPlayer()->playAllTracks( playlist_field.getPlaylist(), queue );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::selectTrack( bool queue )
{
    PlaylistField playlist_field( "Playlist" );
    TrackListField tracks_field( "Track" );

    class MyForm : public Form {
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                PlaylistField* playlist_field = getField<PlaylistField>( 0 );
                TrackListField* tracks_field = getField<TrackListField>( 1 );
                tracks_field->setPlaylist( playlist_field->getPlaylist() );
            }
        }

    public:
        MyForm( TextIO* input_stream ) :
            Form( input_stream ) {}
    };

    MyForm form( &m_text_io );
    form.add( playlist_field );
    form.add( tracks_field );

    if ( form.play() ) {
        studio.getMusicPlayer()->playTrack( tracks_field.getTrack(), queue );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::musicSceneSelect()
{
    getVenue()->setMusicSceneSelectEnabled( !getVenue()->isMusicSceneSelectEnabled() );
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::musicMapTrack(void)
{
    VERIFY_PLAYER_LOGGED_IN;

    class MyForm : public Form {
        Venue*              m_venue;

        NumberedListField   m_map_target_field;
        PlaylistField       m_playlist_field;
        TrackListField      m_tracks_field;
        NumberedListField   m_map_to_field;
        ChaseSelectField    m_chase_select_field;
        SceneSelectField    m_scene_select_field;

        void fieldLeaveNotify( size_t field_num ) {
            if ( getField<Field>(field_num) == &m_map_target_field ) {
                m_playlist_field.setHidden( m_map_target_field.getListValue() != 1 );
                m_tracks_field.setHidden( m_map_target_field.getListValue() != 1 );
            }
            else if ( getField<Field>(field_num) == &m_map_to_field ) {
                m_chase_select_field.setHidden( m_map_to_field.getListValue() != 2 );
                m_scene_select_field.setHidden( m_map_to_field.getListValue() != 1 );
            }
            else if ( getField<Field>(field_num) == &m_playlist_field ) {
                m_tracks_field.setPlaylist( m_playlist_field.getPlaylist() );
            }
        }

    public:
        MyForm( TextIO* input_stream, Venue* venue ) :
            Form( input_stream, "Map Track To Scene" ),
            m_venue( venue ),
            m_map_target_field( "Map" ),
            m_playlist_field( "Playlist" ),
            m_tracks_field( "Track" ),
            m_map_to_field( "Show" ),
            m_chase_select_field( "Chase", venue ),
            m_scene_select_field( "Scene", venue )          
        {
            m_map_target_field.addKeyValue( 1, "Track" );
            m_map_target_field.addKeyValue( 2, "Silence" );
            m_map_target_field.addKeyValue( 3, "Unmapped tracks" );
            m_map_target_field.setDefaultListValue( 1 );

            m_map_to_field.addKeyValue( 1, "Scene" );
            m_map_to_field.addKeyValue( 2, "Chase" );
            m_map_to_field.addKeyValue( 3, "Random Scene" );
            m_map_to_field.addKeyValue( 4, "Random Chase" );
            m_map_to_field.addKeyValue( 5, "Random Scene (BPM)" );
            m_map_to_field.setDefaultListValue( 1 );

            add( m_map_target_field );
            add( m_playlist_field );
            add( m_tracks_field );
            add( m_map_to_field );
            add( m_chase_select_field );
            add( m_scene_select_field );
        }

        void addMusicMapping() {
            CString track_full_name, track_link;
            MusicSelectorType type = (MusicSelectorType)m_map_to_field.getListValue();
            UID uid = 0L;
            
            if ( type == MST_CHASE )
                uid = m_chase_select_field.getChaseUID();
            else  if ( type == MST_SCENE )
                uid = m_scene_select_field.getSceneUID();

            switch ( m_map_target_field.getListValue() ) {
                case 1:
                    track_full_name = m_tracks_field.getTrackFullName();
                    track_link = m_tracks_field.getTrackLink();
                    break;
                case 2:
                    track_full_name = SILENCE_TRACK_NAME;
                    track_link = SILENCE_TRACK_LINK;
                    break;
                case 3:
                    track_full_name = UNMAPPED_TRACK_NAME;
                    track_link =  UNMAPPED_TRACK_LINK;
                    break;
            }

            MusicSceneSelector music_scene_selector( track_full_name, track_link, type, uid );
            m_venue->addMusicMapping( music_scene_selector );
        }
    };

    MyForm form( &m_text_io, getVenue() );
    if ( form.play() ) {
        form.addMusicMapping();
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::musicRemoveMapping(void)
{
    VERIFY_PLAYER_LOGGED_IN;

    PlaylistField playlist_field( "Playlist" );
    TrackListField tracks_field( "Track" );

    class MyForm : public Form {
        void fieldLeaveNotify( size_t field_num ) {
            if ( field_num == 0 ) {
                PlaylistField* playlist_field = getField<PlaylistField>( 0 );
                TrackListField* tracks_field = getField<TrackListField>( 1 );
                tracks_field->setPlaylist( playlist_field->getPlaylist() );
            }
        }

    public:
        MyForm( TextIO* input_stream ) :
            Form( input_stream, "Remove Track Mapping" ) {}
    };

    MyForm form( &m_text_io );
    form.add( playlist_field );
    form.add( tracks_field );

    if ( form.play() ) {
        getVenue()->deleteMusicMapping( tracks_field.getTrackFullName() );
    }
}

// ----------------------------------------------------------------------------
//
void DMXTextUI::musicMapShow()
{
    MusicSceneSelectMap& mm = getVenue()->music_scene_select_map();

    for ( MusicSceneSelectMap::iterator it=mm.begin(); it != mm.end(); ++it ) {
        CString target( "UNKNOWN" );

        if ( it->second.m_selection_type == MST_SCENE ) {
            Scene* scene = getVenue()->getScene( it->second.m_selection_uid );
            if ( scene )
                target.Format( "Scene '%s'", scene->getName() );
        }
        else if ( it->second.m_selection_type == MST_CHASE ) {
            Chase* chase = getVenue()->getChase( it->second.m_selection_uid );
            if ( chase )
                target.Format( "Chase '%s'", chase->getName() );
        }
        else if ( it->second.m_selection_type == MST_RANDOM_SCENE ) {
            target.Format( "Random Scene" );
        }
        else if ( it->second.m_selection_type == MST_RANDOM_CHASE ) {
            target.Format( "Random Chase" );
        }
        else if ( it->second.m_selection_type == MST_SCENE_BY_BPM ) {
            target.Format( "Random Scene (BPM)" );
        }

        m_text_io.printf( "   %s -> %s\n", it->second.m_track_full_name, target );
    }
}