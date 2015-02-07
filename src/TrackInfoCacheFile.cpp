/* 
Copyright (C) 2015 Robert DeSantis
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

#include "TrackInfoCacheFile.h"

// ----------------------------------------------------------------------------
//
TrackInfoCacheFile::TrackInfoCacheFile( LPCSTR filename ) :
    m_filename( filename )
{
}

// ----------------------------------------------------------------------------
//
TrackInfoCacheFile::~TrackInfoCacheFile(void)
{
}

// ----------------------------------------------------------------------------
//
bool TrackInfoCacheFile::read( AudioTrackInfoCache& cache )
{
    TiXmlDocument doc;
    if ( !doc.LoadFile( m_filename ) ) {
        m_last_error = doc.ErrorDesc();
        return false;
    }

    cache.clear();

    TiXmlElement* root = doc.FirstChildElement( "audio_track_info" );
    if ( root ) {
        TiXmlElement* entry = root->FirstChildElement( "entry" );
        while ( entry ) {
            AudioInfo info;
            CString link = read_text_element( entry, "link" );

            strncpy_s( info.id, read_text_element( entry, "id" ), 512 );
            strncpy_s( info.song_type, read_text_element( entry, "song_type" ), 512 );

            TiXmlElement* attrs = entry->FirstChildElement( "attrs" );
            if ( !attrs )
                continue;

            info.key = read_int_attribute( attrs, "key" );
            info.mode = read_int_attribute( attrs, "mode" );
            info.time_signature = read_int_attribute( attrs, "time_signature" );

            info.energy = read_double_attribute( attrs, "energy" );
            info.liveness = read_double_attribute( attrs, "liveness" );
            info.tempo = read_double_attribute( attrs, "tempo" );
            info.speechiness = read_double_attribute( attrs, "speechiness" );
            info.acousticness = read_double_attribute( attrs, "acousticness" );
            info.instrumentalness = read_double_attribute( attrs, "instrumentalness" );
            info.duration = read_double_attribute( attrs, "duration" );
            info.loudness = read_double_attribute( attrs, "loudness" );
            info.valence = read_double_attribute( attrs, "valence" );
            info.danceability = read_double_attribute( attrs, "danceability" );

            cache[ link ] = info;

            entry = entry->NextSiblingElement();
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
//
void TrackInfoCacheFile::write( AudioTrackInfoCache& cache )
{
    TiXmlDocument doc;

    TiXmlElement root( "audio_track_info" );

    for (  AudioTrackInfoCache::value_type pair : cache ) {
        TiXmlElement entry( "entry" );
        add_text_element( entry, "link", pair.first );
        add_text_element( entry, "id", pair.second.id );
        add_text_element( entry, "song_type", pair.second.song_type );

        TiXmlElement attributes( "attrs" );
        add_attribute( attributes, "key", pair.second.key );
        add_attribute( attributes, "mode", pair.second.mode );
        add_attribute( attributes, "time_signature", pair.second.time_signature );
        add_attribute( attributes, "energy", pair.second.energy );
        add_attribute( attributes, "liveness", pair.second.liveness );
        add_attribute( attributes, "tempo", pair.second.tempo );
        add_attribute( attributes, "speechiness", pair.second.speechiness );
        add_attribute( attributes, "acousticness", pair.second.acousticness );
        add_attribute( attributes, "instrumentalness", pair.second.instrumentalness );
        add_attribute( attributes, "duration", pair.second.duration );
        add_attribute( attributes, "loudness", pair.second.loudness );
        add_attribute( attributes, "valence", pair.second.valence );
        add_attribute( attributes, "danceability", pair.second.danceability );

        entry.InsertEndChild( attributes );
        root.InsertEndChild( entry );
    }

    doc.InsertEndChild( root ); 
    doc.SaveFile( m_filename );
}


