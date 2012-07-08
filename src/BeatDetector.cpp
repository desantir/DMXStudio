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


#include "BeatDetector.h"

// ----------------------------------------------------------------------------
//
BeatDetector::BeatDetector( unsigned frequency_bins, unsigned sensitivity ) :
	m_audio_stream( NULL ),
	m_sensitivity_ms( sensitivity ),
	m_frequency_bins( frequency_bins ),
	m_sample_size(0),
	m_samples_per_second(0)
{
}

// ----------------------------------------------------------------------------
//
BeatDetector::~BeatDetector(void)
{
	detach();
}

// ----------------------------------------------------------------------------
//
void BeatDetector::attach( AudioInputStream* audio ) {
	detach();

	m_audio_stream = audio;

	if ( audio ) {
		m_sample_size = m_audio_stream->getSampleSize();
		m_samples_per_second = m_audio_stream->getFormat().Format.nSamplesPerSec;

		STUDIO_ASSERT( m_frequency_bins < m_sample_size/2, "Too many frequency bins" );		 // THROW ERROR
		STUDIO_ASSERT( m_frequency_bins > 0, "Too few frequency bins" );
		STUDIO_ASSERT( m_frequency_bins <= 64, "Too many frequency bins" );

		for ( unsigned i=0; i < m_frequency_bins; i++ )
			m_bins.push_back( FreqBin( m_samples_per_second / m_sample_size, m_sensitivity_ms ) );

		m_B = new double[ m_sample_size ];
		m_Es = new double[ m_sample_size ];	

		// Reset all beat frequency bins
		for ( FreqBinArray::iterator it=m_bins.begin(); it != m_bins.end(); it++ )
			(*it).reset();

		m_audio_stream->addAudioProcessor( this, ProcessorFormat( 2, m_frequency_bins > 1 ) );
	}
}

// ----------------------------------------------------------------------------
//
void BeatDetector::detach( ) {
	if ( m_audio_stream ) {
		m_audio_stream->removeAudioProcessor( this );

		delete m_B;
		delete m_Es;

		m_audio_stream = NULL;
		m_B = NULL;
		m_Es = NULL;
	}
}

// ----------------------------------------------------------------------------
//
void BeatDetector::addFrequencyEvent( CEvent* eventHandler, unsigned freq_low, unsigned freq_high )
{
	unsigned samples_per_bin = m_sample_size / m_frequency_bins; // Frequency samples per bin

	unsigned bin_start = ((freq_low * m_sample_size) / m_samples_per_second) / samples_per_bin;
	unsigned bin_end =  ((freq_high * m_sample_size) / m_samples_per_second) / samples_per_bin;

	DWORD mask[2] = { 0, 0 };

	for ( unsigned bin=bin_start; bin <= bin_end && bin < m_frequency_bins; bin++ ) {
		if ( bin >= 32 )
			mask[1] |= (1L<<(bin-32));
		else
			mask[0] |= (1L<<bin);
	}

	output( "Beat detect bins: %d to %d mask=%04x%04x\n\n", bin_start, bin_end, mask[1],mask[0] );

	m_event_handlers.push_back( BeatEvent( eventHandler, mask ) );
}

// ----------------------------------------------------------------------------
//
void BeatDetector::removeFrequencyEvent( CEvent* eventHandler )
{
	for ( BeatEventArray::iterator it=m_event_handlers.begin(); 
			it != m_event_handlers.end(); )
		if ( (*it).m_beatEvent == eventHandler )
			it = m_event_handlers.erase( it );
		else
			it++;
}

// ----------------------------------------------------------------------------
//
HRESULT BeatDetector::ProcessAmplitudes( WORD channels, size_t sample_size, float* sample_data[] ) {
	FreqBin& fb = m_bins[0];						// Always bin 0 for sound energy

	// R1 - Instant sound energy
	double e = 0.0;
	for ( unsigned bin=0; bin < m_sample_size; bin++ ) {
		e += ( sample_data[LEFT_CHANNEL][ bin ] * sample_data[LEFT_CHANNEL][ bin ] +
			   sample_data[RIGHT_CHANNEL][ bin ] * sample_data[RIGHT_CHANNEL][ bin ] );
	}

	e = e / m_sample_size;
	e = sqrt(e) * 100;

	// R2 - Average of our current samples
	double _E_ = fb.getAverage();

	// R5 - Compute Varience
	double V = fb.getVarience( _E_ );

	// R6 - linear degression
	double C = (-0.0025714F * V) + 1.5142857F;

	double diff = std::max<double>(e - C * _E_, 0 );
	double dAvg = fb.specAverage();
	double diff2 = std::max<double>( diff-dAvg, 0 );

	DWORD time_ms = GetCurrentTime();

	bool beat = fb.beat( diff2 > 0 && e > 2, time_ms );
	//bool beat = fb.beat( e > (C * _E_), time_ms ); 

	if ( beat ) {
		output( "*   \r" );

		for ( BeatEventArray::iterator it=m_event_handlers.begin(); 
			  it != m_event_handlers.end(); it++ )
			(*it).m_beatEvent->SetEvent();
	}

	fb.insertValue( e );
	fb.insertSpec( diff );

	return 0;
}

// ----------------------------------------------------------------------------
//
HRESULT BeatDetector::ProcessFFT( WORD channels, FFT_Result* fft_result[] )
{
	size_t fft_sample_size = fft_result[LEFT_CHANNEL]->getSampleSize();

	if ( channels > 2 )		// We only care about right and left
		channels = 2;

	// Collect amplitude and mix (not concern about audio "quality" here)
	for ( size_t i=0; i < fft_sample_size; i++ ) {
		float amplitude = fft_result[LEFT_CHANNEL]->getAmplitude( i );
		if ( channels > 1 )
			amplitude += fft_result[RIGHT_CHANNEL]->getAmplitude( i );
		m_B[i] = (amplitude / channels);
	}

	int bands = fft_sample_size / m_frequency_bins;		// Coverting from 512 back to 1024

	// R7 - Compute energy of the ## bands
	for ( unsigned i=0; i < m_frequency_bins; i++ ) {
		m_Es[i] = 0.0;

		for ( unsigned k=i*bands; k < (i+1)*bands; k++ )
			m_Es[i] += m_B[k];

		m_Es[i] = (m_Es[i] * m_frequency_bins) / m_sample_size;
	}

	DWORD mask[2] = { 0, 0 };
	double C = 1.4F;
	DWORD time_ms = GetCurrentTime();

	for ( unsigned k=0; k < m_frequency_bins; k++ ) {
		FreqBin& fb = m_bins[k];

		double _E_ = fb.getAverage();

		bool beat = fb.beat( m_Es[k] > (C * _E_), time_ms );

		if ( beat ) {
			output( "%02d ", k );

			if ( k >= 32 )
				mask[1] |= (1L<<(k-32));
			else
				mask[0] |= (1L<<k);
		}
		else {
			output( "-- " );
		}

		fb.insertValue( m_Es[k] );
	}

	for ( BeatEventArray::iterator it=m_event_handlers.begin(); 
			it != m_event_handlers.end(); it++ ) {
		if ( (*it).m_mask[0] & mask[0] || (*it).m_mask[1] & mask[1] ) {
			(*it).m_beatEvent->SetEvent();
			output( " *" );
		}
	}

	output( "    \r" );

	return 0;
}
