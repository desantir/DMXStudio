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
bool HttpRestServices::control_soundsampler_start( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    session->getSoundSampler().attach( studio.getVenue()->getAudio() );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_soundsampler_stop( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    session->getSoundSampler().detach();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_soundsampler( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    if ( !session->getSoundSampler().isAttached() )
        session->getSoundSampler().attach( studio.getVenue()->getAudio() );

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

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_beatsampler_start( DMXHttpSession* session, CString& response, LPCSTR data, DWORD size, LPCSTR content_type )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    SimpleJsonParser parser;
    PARSER_LIST bins;

    session->getBeatDetector().removeAllFrequencyEvents();
    session->getBeats().clear();

    session->getBeatDetector().attach( studio.getVenue()->getAudio(), 64 );

    try {
        parser.parse( data );

        bins = parser.get<PARSER_LIST>( "" );

        for ( PARSER_LIST::iterator it=bins.begin(); it != bins.end(); ++it ) {
            SimpleJsonParser& bin_parser = (*it);

            unsigned start_freq = bin_parser.get<unsigned>( "start_freq" );
            unsigned end_freq = bin_parser.get<unsigned>( "end_freq" );

            session->getBeats().push_back( BeatBin( start_freq, end_freq ) );
        }
    }
    catch ( std::exception& e ) {
        throw StudioException( "JSON parser error (%s) data (%s)", e.what(), data );
    }

    for ( BeatBinArray::iterator it=session->getBeats().begin(); it != session->getBeats().end(); ++it )
        session->getBeatDetector().addFrequencyEvent( (*it).getEvent(), (*it).getStartFreq(),  (*it).getEndFreq() );

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::control_beatsampler_stop( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    session->getBeatDetector().removeAllFrequencyEvents();
    session->getBeatDetector().detach();
    session->getBeats().clear();

    return true;
}

// ----------------------------------------------------------------------------
//
bool HttpRestServices::query_beatsampler( DMXHttpSession* session, CString& response, LPCSTR data )
{
    if ( !studio.getVenue() || !studio.getVenue()->isRunning() )
        return false;

    if ( !session->getBeatDetector().isAttached() )
         return false;

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

    return true;
}

