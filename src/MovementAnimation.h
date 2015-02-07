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

#include "IVisitor.h"

#define MAX_RANDOM_POSITIONS 1000
#define MAX_WAIT_PERIODS	 500

typedef enum {
    MOVEMENT_RANDOM = 1,							// Move to random locations
    MOVEMENT_FAN = 2,								// Fan beams
    MOVEMENT_ROTATE = 3,							// Simple rotate at tilt angle
    MOVEMENT_NOD = 4,								// Simple up and down
    MOVEMENT_XCROSS = 5,							// Cross fixture beams
    MOVEMENT_COORDINATES = 6,                       // Move to specific coordinates
    MOVEMENT_MOONFLOWER = 7, 						// Moonflower in and out effect
    MOVEMENT_SINEWAVE = 8                           // Sine wave movement
} MovementAnimationType;

struct FixtureCoordinate
{
    UINT    m_pan;
    UINT    m_tilt;

    FixtureCoordinate( UINT pan, UINT tilt ) :
        m_pan( pan ),
        m_tilt( tilt )
    {}
};

typedef std::vector<FixtureCoordinate> CoordinateArray;

class MovementAnimation
{
    friend class VenueWriter;
    friend class VenueReader;

public:
    MovementAnimationType	m_movement_type;		// Movement animation type
    UINT					m_tilt_start;			// Start end tilt angles (some may only use start)
    UINT					m_tilt_end;
    UINT					m_pan_start;			// Start end pan angles (some may only use start)
    UINT					m_pan_end;
    UINT					m_pan_increment;		// Movement pan increment (when applicable)
    bool					m_backout_home_return;	// When true, beam is left on during return to home
    UINT					m_home_wait_periods;	// Number of periods to wait at home position
    UINT					m_dest_wait_periods;	// Number of periods to wait at destination period
    UINT					m_positions;			// Number of positions to generate
    UINT					m_group_size;			// Size of fixture groups (when application)
    bool					m_alternate_groups;		// Alternate home and destination for each group
    BYTE					m_speed;				// Directly sets movement speed (0=fastest)
    CoordinateArray         m_coordinates;          // Movement coordinates
    bool                    m_run_once;             // Run animation once on scene start
    UINT                    m_head_number;          // Fixture pan/tilt head index (0=all, 1-n is specific pair)

    float                   m_home_x;               // Moonflower cartesian plane coordinates
    float                   m_home_y;               //
    float                   m_height;               //
    float                   m_fixture_spacing;      //
    float                   m_radius;               //

    MovementAnimation( MovementAnimationType movement_type = MOVEMENT_RANDOM,
                       UINT	tilt_start = 90,
                       UINT tilt_end = 270,
                       UINT pan_start = 0,
                       UINT	pan_end = 360,
                       bool backout_home_return = false,
                       UINT group_size = 1,
                       UINT	home_wait_periods = 1,
                       UINT	dest_wait_periods = 1,
                       UINT positions = 5,
                       bool alternate_groups = false,
                       UINT	pan_increment = 1,
                       BYTE speed = 0,
                       float home_x = 0,
                       float home_y = -5,
                       float height = 7,
                       float fixture_spacing = 2,
                       float radius = 5,
                       CoordinateArray coordinates = CoordinateArray(),
                       bool run_once = false,
                       UINT head_number = 0 );

    MovementAnimation( const MovementAnimation& other );

    MovementAnimation& operator=( const MovementAnimation& other );

    void accept( IVisitor* visitor) {
        visitor->visit(this);
    }

    virtual CString getSynopsis(void);
};
