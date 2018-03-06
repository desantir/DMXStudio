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

#include "HttpRestServices.h"
#include "Venue.h"
#include "SimpleJsonBuilder.h"

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_soundsampler_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    session->getSoundSampler().attach( venue->getAudio() );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_soundsampler_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    session->getSoundSampler().detach();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_amplitude( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    SoundLevel levels;

    venue->getSoundLevels( levels );

    JsonBuilder json( response );

    json.startObject();
    json.add( "avg_amplitude", levels.avg_amplitude );
    json.add( "amplitude", levels.amplitude );
    json.add( "mute", venue->isMute() );
    json.add( "volume", venue->getMasterVolume() );
    json.add( "beat", levels.amplitude_beat );
    json.add( "index", levels.beat_index );
    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_soundsampler( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !session->getSoundSampler().isAttached() )
        session->getSoundSampler().attach( venue->getAudio() );

    ULONG sample_number;
    SampleSet samples = session->getSoundSampler().getSampleSet( sample_number );

    JsonBuilder json( response );

    json.startObject();
    json.add( "sample_number", sample_number );

    json.startArray( "audio_data" );
    for ( SampleSet::iterator it=samples.begin(); it != samples.end(); it++ ) {
        int value = 100 + (*it).getDB();
        if ( value < 0 )
            value = 0;
        
        json.add( value );
    }
    json.endArray( "audio_data" );

    json.endObject();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_beatsampler_start( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    SimpleJsonParser parser;

    session->getBeatDetector().removeAllFrequencyEvents();
    session->getBeats().clear();

    session->getBeatDetector().attach( venue->getAudio(), 64 );

    try {
        parser.parse( data );

        JsonNodePtrArray bins = parser.getObjects();

        for ( JsonNode* bin : bins ) {
            unsigned start_freq = bin->get<unsigned>( "start_freq" );
            unsigned end_freq = bin->get<unsigned>( "end_freq" );

            session->getBeats().emplace_back( start_freq, end_freq );
        }
    }
    catch ( std::exception& e ) {
        throw RestServiceException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    for ( BeatBinArray::iterator it=session->getBeats().begin(); it != session->getBeats().end(); ++it )
        session->getBeatDetector().addFrequencyEvent( (*it).getEvent(), (*it).getStartFreq(),  (*it).getEndFreq() );
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::control_beatsampler_stop( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    session->getBeatDetector().removeAllFrequencyEvents();
    session->getBeatDetector().detach();
    session->getBeats().clear();
}

// ----------------------------------------------------------------------------
//
void HttpRestServices::query_beatsampler( Venue* venue, DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !session->getBeatDetector().isAttached() )
        throw RestServiceException( "Beat detector not initialized" );

    JsonBuilder json( response );

    json.startArray();

    for ( BeatBinArray::iterator it=session->getBeats().begin(); it != session->getBeats().end(); ++it ) {
        json.startObject();
        json.add( "start_freq", (*it).getStartFreq() );
        json.add( "end_freq", (*it).getEndFreq() );
        json.add( "beat", (*it).isBeat( ) );
        json.endObject();
    }

    json.endArray();
}

