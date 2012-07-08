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


#include "MovementAnimation.h"

// ----------------------------------------------------------------------------
//
MovementAnimation::MovementAnimation( MovementAnimationType movement_type,
									   UINT	tilt_start,
									   UINT tile_end,
									   UINT pan_start,
									   UINT	pan_end,
									   bool backout_home_return,
				  					   UINT group_size,
				 					   UINT	home_wait_periods,
									   UINT	dest_wait_periods,
									   UINT positions,
									   bool alternate_groups,
									   UINT	pan_increment,
									   BYTE speed,
                                       float home_x,
                                       float home_y,
                                       float height,
                                       float fixture_spacing,
                                       float radius,
                                       CoordinateArray coordinates,
                                       bool run_once ) :
		m_movement_type( movement_type ),
		m_tilt_start( tilt_start ),
		m_tilt_end( tile_end ),
		m_pan_start( pan_start ),
		m_pan_end( pan_end ),
		m_backout_home_return( backout_home_return ),
		m_group_size( group_size ),
		m_home_wait_periods( home_wait_periods ),
		m_dest_wait_periods( dest_wait_periods ),
		m_positions( positions ),
		m_alternate_groups( alternate_groups ),	
		m_pan_increment( pan_increment ),
		m_speed( speed ),
        m_home_x( home_x ),
        m_home_y( home_y ),
        m_height( height ),
        m_fixture_spacing( fixture_spacing ),
        m_radius( radius ),
        m_coordinates( coordinates ),
        m_run_once( run_once )
{
}

// ----------------------------------------------------------------------------
//
MovementAnimation::MovementAnimation( const MovementAnimation& other ) {
	*this = other;
}

// ----------------------------------------------------------------------------
//
MovementAnimation& MovementAnimation::operator=( const MovementAnimation& other ) {
	m_movement_type = other.m_movement_type;
	m_tilt_start = other.m_tilt_start;
	m_tilt_end = other.m_tilt_end;
	m_pan_start = other.m_pan_start;
	m_pan_end = other.m_pan_end;
	m_backout_home_return = other.m_backout_home_return;
	m_group_size = other.m_group_size;
	m_home_wait_periods = other.m_home_wait_periods;
	m_dest_wait_periods = other.m_dest_wait_periods;
	m_positions = other.m_positions;
	m_alternate_groups = other.m_alternate_groups;	
	m_pan_increment = other.m_pan_increment;
	m_speed = other.m_speed;
    m_home_x = other.m_home_x;
    m_home_y = other.m_home_y;
    m_height = other.m_height;
    m_fixture_spacing = other.m_fixture_spacing;
    m_radius = other.m_radius;
    m_coordinates = other.m_coordinates;
    m_run_once = other.m_run_once;

	return *this;
}

// ----------------------------------------------------------------------------
//
CString MovementAnimation::getSynopsis(void) {
	CString synopsis;
	CString movement;

	switch ( m_movement_type ) {
		case MOVEMENT_RANDOM:       movement = "Random"; break;
		case MOVEMENT_FAN:          movement = "Fan"; break;
		case MOVEMENT_ROTATE:       movement = "Rotate"; break;
		case MOVEMENT_NOD:          movement = "Nod"; break;
		case MOVEMENT_XCROSS:       movement = "X-cross"; break;
		case MOVEMENT_MOONFLOWER:   movement = "Moonflower"; break;
		case MOVEMENT_COORDINATES:  movement = "Coordinates"; break;
	}

	synopsis.Format( "%s Movement( speed=%d )\n", movement, m_speed );
         
    if ( m_movement_type == MOVEMENT_COORDINATES ) {
        synopsis.AppendFormat( "Coordinates( " );
		for ( size_t index=0; index <  m_coordinates.size(); index++ )
            synopsis.AppendFormat( "%u,%u ", m_coordinates[index].m_pan, m_coordinates[index].m_tilt );
        synopsis.AppendFormat( ")\n" );
    }
    else if ( m_movement_type == MOVEMENT_MOONFLOWER ) {
	    synopsis.Format( "Grid( home=%.1f,%.1f height=%.1f spacing=%.1f radius=%.1f pan_incr=%d )\n",
            m_home_x, m_home_y, m_height, m_fixture_spacing, m_radius, m_pan_increment );
    }
    else {
	    synopsis.Format( "Degrees( tilt=%d-%d pan=%d-%d pan_incr=%d )\n",
            m_tilt_start, m_tilt_end, m_pan_start, m_pan_end, m_pan_increment );
    }

	synopsis.AppendFormat( "Options( blackout_home_return=%s home_wait_periods=%d destination_wait_periods=%d positions=%d run_once=%s )\n",
		 m_backout_home_return ? "yes" : "no", m_home_wait_periods,  m_dest_wait_periods, m_positions,
         m_run_once ? "yes" : "no" );

	synopsis.AppendFormat( "Fixture Groups( size=%d alternate=%s )",
		 m_group_size, m_alternate_groups ? "yes" : "no" );

	return synopsis;
}