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

#pragma once

#include "DMXStudio.h"

class BeatDetector;

class FreqBin
{
	friend class BeatDetector;

	double*		m_E;					// E history buffer
	double*		m_S;					// Spec history buffer
	DWORD		m_nextBeatMS;			// MS before next beat is detected 
	unsigned	m_sensitivity_ms;		// Sensitivity
	unsigned	m_size;
	bool		m_beat;					// Is beat

public:
	FreqBin( unsigned bin_size, unsigned sensitivity_ms ) :
		m_sensitivity_ms(sensitivity_ms),
		m_size( bin_size ) {
		m_E = new double[ m_size ];
		m_S = new double[ m_size ];
		m_beat = false;
	}

	~FreqBin() {
		delete m_E;
		delete m_S;
	}

	FreqBin( FreqBin& other ) {
		m_nextBeatMS = other.m_nextBeatMS;
		m_size = other.m_size;
		m_sensitivity_ms = other.m_sensitivity_ms;
		m_E = new double[ m_size ];
		m_S = new double[ m_size ];
	}

	inline unsigned getSize() const {
		return m_size;
	}

	inline void insertValue( double e ) {
		memmove( &m_E[1], &m_E[0], (m_size-1)*sizeof(double) );
		m_E[0] = e;
	}

	inline void insertSpec( double e ) {
		memmove( &m_S[1], &m_S[0], (m_size-1)*sizeof(double) );
		m_S[0] = e;
	}

	inline double getAverage() {
		double _E_ = 0.0;
		for ( unsigned i=0; i < m_size; i++ )
			_E_ += m_E[i];
		_E_ /= (double)m_size;
		return _E_;
	}

	inline double specAverage() {
		double _S_ = 0.0;
		int count = 0;
		for ( unsigned i=0; i < m_size; i++ ) {
			if ( m_S[i] > 0 ) {
				_S_ += m_S[i];
				count++;
			}
		}
		if ( count > 0 )
			_S_ /= (double)count;
		return _S_;
	}

	inline double getVarience( double _E_ ) {
		double V = 0.0;
		for ( unsigned i=0; i < m_size; i++ )
			V += pow(m_E[i] - _E_, 2);
		V /= (double)m_size;
		return V;
	}

	void reset( ) {
		for ( unsigned i=0; i < m_size; i++ ) 
			m_E[i] = 9999999.0;

		m_nextBeatMS = 0;
	}

	bool beat( bool beat, DWORD time_ms ) {
		if ( beat ) {
			if ( m_nextBeatMS < time_ms ) {
				m_nextBeatMS = time_ms + m_sensitivity_ms;
				m_beat = true;
			}
			else
				beat = false;
		}
		else
			m_beat = false;

		return beat;
	}
};

typedef std::vector<FreqBin> FreqBinArray;
