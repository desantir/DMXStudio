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


#include "SceneMovementAnimator.h"

/**
    Tilt: 180 degrees is straight up
    Pan: 0 degrees is towards front of fixture

    Animation Data:
        Criss-cross: start/end tilt, start/end pan, return on state, fixture group size (1,2,etc)
        Fan in/ out: start/end tilt, start/end pan,  return on state
        Circle round and back: start/end pan,  tilt,  return on state
        Random position:  return on state, stay at location periods, number positions (pan and tilt provides min/max)
        Nod up and down: start/end tilt, start/end pan,  return on state, pan increment, alternate fixtures

        Start/end tilt: Start end tilt angles (some may only use start)
        Start/end pan: Start end pan angles (some may only use start)
        Return on state: Beam is lit during return travel
        Fixture group size: When multiple fixtures, broken into groups of this size (1,2,etc)
        Stay at location: Number of periods! the fixture stays at the target position
        Number positions: For random movement, number of random positions generated
        Alternate fixtures: When applicable, fixture group begin/end will be reversed
        Angle increment: For nod, rotation angle increment 
*/

const char* SceneMovementAnimator::className = "SceneMovementAnimator";

void compute_fastest_path( UINT pan_current, UINT tilt_current, UINT& pan_target, UINT& tilt_target );

void compute_pan_tilt( UINT fixture_number, double height, double fixture_spacing, 
                       double target_x, double target_y, UINT& pan, UINT& tilt );

// ----------------------------------------------------------------------------
//
SceneMovementAnimator::SceneMovementAnimator( UID animation_uid, 
                                        AnimationSignal signal,
                                        UIDArray actors,
                                        MovementAnimation movement ) :
    SceneChannelAnimator( animation_uid, signal ),
    m_movement( movement )
{
    m_actors = actors;
}

// ----------------------------------------------------------------------------
//
SceneMovementAnimator::~SceneMovementAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneMovementAnimator::clone() {
    return new SceneMovementAnimator( m_uid, m_signal, m_actors, m_movement );
}

// ----------------------------------------------------------------------------
//
CString SceneMovementAnimator::getSynopsis(void) {
    CString synopsis;

    synopsis.Format( "%s\n%s", 
        m_movement.getSynopsis(),
        AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet )
{
    m_animation_task = task;
    m_channel_animations.clear();
    m_run_once = movement().m_run_once;

    FixturePtrArray participants;

    // Determine which channels will be participating
    for ( UIDArray::iterator it=m_actors.begin(); it != m_actors.end(); ++it ) {
        Fixture* pf = m_animation_task->getFixture( (*it) );
        STUDIO_ASSERT( pf != NULL, "Missing fixture UID=%lu", (*it) );
        if ( pf->canMove() )
            participants.push_back( pf );
    }

    STUDIO_ASSERT( movement().m_group_size >= 1, "Movement group size must be >= 1" );
    STUDIO_ASSERT( movement().m_home_wait_periods > 0 && movement().m_home_wait_periods < MAX_WAIT_PERIODS, 
                "Home wait periods must be between 1 and %d", MAX_WAIT_PERIODS );
    STUDIO_ASSERT( movement().m_dest_wait_periods > 0 && movement().m_dest_wait_periods < MAX_WAIT_PERIODS, 
                "Destination wait periods must be between 1 and %d", MAX_WAIT_PERIODS );

    // TODO: At some point, pre-compute the channel arrays once rather on each scene load as
    // that will be a problem for many fixtures/animations in a single scene.

    switch ( movement().m_movement_type ) {
        case MOVEMENT_RANDOM:							// Move to random locations
            genRandomMovement( task, participants );
            break;

        case MOVEMENT_FAN:								// Fan beams
            genFanMovement( task, participants );
            break;

        case MOVEMENT_ROTATE:							// Simple rotate at tilt angle
            genRotateMovement( task, participants );
            break;

        case MOVEMENT_NOD:								// Simple up and down
            genNodMovement( task, participants );
            break;

        case MOVEMENT_XCROSS:							// Cross fixture beams
            genXcrossMovement( task, participants );
            break;

        case MOVEMENT_MOONFLOWER:						// Moonflower effect
            genMoonflowerMovement( task, participants );
            break;

        case MOVEMENT_COORDINATES:						// Absolute coordinates effect
            genCoordinatesMovement( task, participants );
            break;
    }

    SceneChannelAnimator::initAnimation( task, time_ms, dmx_packet );
}

// NOTE: ALL MOVEMENT GENERATORS POPULATE DIMMER CHANNEL VALUE ARRAY WITH 0 FOR OFF AND 255 FOR ON
//       THESE VALUES MUST BE NORMALIZED TO THE APPRORIATE HIGH/LOW VALUES OF THE SPECFIC FIXTURES'S
//       DIMMER CHANNEL.

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::genXcrossMovement( AnimationTask* task, FixturePtrArray& participants )
{
    bool forward = true;				// Channel value population direction for alternate

    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        UINT pan_start = movement().m_pan_start;
        UINT pan_end = movement().m_pan_end;
        UINT tilt_start = movement().m_tilt_start;
        UINT tilt_end = movement().m_tilt_end;

        if ( !forward ) {
            swap<UINT>( pan_start, pan_end );
        }

        // Go home
        tilt.push_back( tilt_start );
        pan.push_back( pan_start );
        dimmer.push_back( movement().m_backout_home_return ? 0 : 255 );
        speed.push_back( 0 );

        // Go to destination
        for ( UINT i=0; i < movement().m_dest_wait_periods; i++ ) {
            tilt.push_back( tilt_end );
            pan.push_back( pan_end );
            dimmer.push_back( 255 );
            speed.push_back( movement().m_speed );
        }

        // Go back home
        for ( UINT i=0; i < movement().m_home_wait_periods-1; i++ ) {
            tilt.push_back( tilt_start );
            pan.push_back( pan_start );
            dimmer.push_back( movement().m_backout_home_return ? 0 : 255 );
            speed.push_back( 0 );
        }

        // Populate the channel arrays for the next group of fixtures
        populateChannelAnimations( task, participants, particpant_index, tilt, pan, dimmer, speed, movement().m_group_size );

        if ( movement().m_alternate_groups )
            forward = !forward;
    }
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::genFanMovement( AnimationTask* task,  FixturePtrArray& participants )
{
    UINT pan_start = movement().m_pan_start;
    UINT pan_end = movement().m_pan_end;
    UINT tilt_start = movement().m_tilt_start;
    UINT tilt_end = movement().m_tilt_end;

    if ( pan_start > pan_end )
        swap<UINT>( pan_start, pan_end );

    UINT pan_increment = pan_end-pan_start;
    UINT pan_home = pan_start + (pan_increment/2);

    if ( participants.size() > 2 )
        pan_increment = (UINT)(((float)pan_increment)/(participants.size()-1)+0.5);

    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        // Go home
        tilt.push_back( tilt_start );
        pan.push_back( pan_home );
        dimmer.push_back( movement().m_backout_home_return ? 0 : 255 );
        speed.push_back( 0 );

        // Go to destination
        for ( UINT i=0; i < movement().m_dest_wait_periods; i++ ) {
            tilt.push_back( tilt_end );
            pan.push_back( pan_start+(pan_increment*particpant_index) );
            dimmer.push_back( 255 );
            speed.push_back( movement().m_speed );
        }

        // Go back home
        for ( UINT i=0; i < movement().m_home_wait_periods-1; i++ ) {
            tilt.push_back( tilt_start );
            pan.push_back( pan_home );
            dimmer.push_back( movement().m_backout_home_return ? 0 : 255 );
            speed.push_back( 0 );
        }

        // Populate the channel arrays for the next group of fixtures
        populateChannelAnimations( task, participants, particpant_index, tilt, pan, dimmer, speed, 1 );
    }
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::genNodMovement( AnimationTask* task, FixturePtrArray& participants )
{
    bool forward = true;				// Channel value population direction for alternate

    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        UINT pan_start = movement().m_pan_start;
        UINT pan_end = movement().m_pan_end;
        UINT tilt_start = movement().m_tilt_start;
        UINT tilt_end = movement().m_tilt_end;

        if ( !forward ) {
            swap<UINT>( tilt_start, tilt_end );
        }

        // Go home
        tilt.push_back( tilt_start );
        pan.push_back( pan_start );
        dimmer.push_back( movement().m_backout_home_return ? 0 : 255 );
        speed.push_back( 0 );

        if ( pan_start > pan_end )
            swap<UINT>( pan_start, pan_end );

        for ( int pan_angle=pan_start; pan_angle <= (int)pan_end; pan_angle += movement().m_pan_increment ) {
            // Go to destination
            for ( UINT i=0; i < movement().m_dest_wait_periods; i++ ) {
                tilt.push_back( tilt_end );
                pan.push_back( pan_angle );
                dimmer.push_back( 255 );
                speed.push_back( movement().m_speed );
            }

            // Go back home
            for ( UINT i=0; i < movement().m_home_wait_periods; i++ ) {
                tilt.push_back( tilt_start );
                pan.push_back( pan_angle );
                dimmer.push_back( 255 );
                speed.push_back( 0 );
            }
        }

        if ( !movement().m_backout_home_return ) {
            for ( int pan_angle=pan_end-=movement().m_pan_increment; 
                  pan_angle >= (int)pan_start; pan_angle-=movement().m_pan_increment ) {
                // Go to destination
                for ( UINT i=0; i < movement().m_dest_wait_periods; i++ ) {
                    tilt.push_back( tilt_end );
                    pan.push_back( pan_angle );
                    dimmer.push_back( 255 );
                    speed.push_back( movement().m_speed );
                }

                // Go back home (account for home on wrap which will account for 1 period)
                UINT periods = movement().m_home_wait_periods;
                if ( pan_angle == pan_start )
                    periods--;

                for ( UINT i=0; i < periods; i++ ) {
                    tilt.push_back( tilt_start );
                    pan.push_back( pan_angle );
                    dimmer.push_back( 255 );
                    speed.push_back( 0 );
                }
            }
        }

        // Populate the channel arrays for the next group of fixtures
        populateChannelAnimations( task, participants, particpant_index, tilt, pan, dimmer, speed, movement().m_group_size );

        if ( movement().m_alternate_groups )
            forward = !forward;
    }
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::genCoordinatesMovement( AnimationTask* task, FixturePtrArray& participants )
{
    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        // Generate random locations within the tilt and pan bounds
        for ( UINT index=0; index < movement().m_coordinates.size(); index++ ) {
            UINT tilt_angle = movement().m_coordinates[index].m_tilt;
            UINT pan_angle = movement().m_coordinates[index].m_pan;

            if ( index > 0 && movement().m_backout_home_return ) {
                pan.push_back( pan_angle );
                tilt.push_back( tilt_angle );
                speed.push_back( 0 );
                dimmer.push_back( 0 );
            }

            for ( UINT wait=0; wait < movement().m_dest_wait_periods; wait++ ) {
                pan.push_back( pan_angle );
                tilt.push_back( tilt_angle );
                speed.push_back( movement().m_speed );
                dimmer.push_back( 255 );
            }
        }

        // Populate the channel arrays for the next group of fixtures
        populateChannelAnimations( task, participants, particpant_index, tilt, pan, dimmer, speed, movement().m_group_size );
    }
}

// ----------------------------------------------------------------------------
//
static UINT random_angle( UINT start_angle, UINT end_angle ) {
    UINT angle_range = ( start_angle > end_angle ) ? 
        start_angle-end_angle+1 : end_angle-start_angle+1;
    return start_angle + (rand() % (angle_range+1));
}

void SceneMovementAnimator::genRandomMovement( AnimationTask* task, FixturePtrArray& participants )
{
    STUDIO_ASSERT( movement().m_positions > 0 && movement().m_positions < MAX_RANDOM_POSITIONS, 
                "Random postions should be between 1 and %d", MAX_RANDOM_POSITIONS );

    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        // Generate random locations within the tilt and pan bounds
        for ( UINT generate=0; generate < movement().m_positions; generate++ ) {
            UINT tilt_angle = random_angle( movement().m_tilt_start, movement().m_tilt_end );
            UINT pan_angle = random_angle( movement().m_pan_start, movement().m_pan_end );

            for ( UINT wait=0; wait < movement().m_dest_wait_periods; wait++ ) {
                pan.push_back( pan_angle );
                tilt.push_back( tilt_angle );
                speed.push_back( movement().m_speed );
                dimmer.push_back( 255 );
            }
        }

        // Populate the channel arrays for the next group of fixtures
        populateChannelAnimations( task, participants, particpant_index, tilt, pan, dimmer, speed, movement().m_group_size );
    }
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::genRotateMovement( AnimationTask* task, FixturePtrArray& participants )
{
    bool forward = true;				// Channel value population direction for alternate

    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        UINT pan_start = movement().m_pan_start;
        UINT pan_end = movement().m_pan_end;
        UINT tilt_start = movement().m_tilt_start;
        UINT tilt_end = movement().m_tilt_end;

        if ( !forward ) {
            swap<UINT>( pan_start, pan_end );
            swap<UINT>( tilt_start, tilt_end );
        }

        // Go home
        tilt.push_back( tilt_start );
        pan.push_back( pan_start );
        dimmer.push_back( 0 );
        speed.push_back( 0 );

        // Go to destination
        for ( UINT i=0; i < movement().m_dest_wait_periods; i++ ) {
            tilt.push_back( tilt_end );
            pan.push_back( pan_end );
            dimmer.push_back( 255 );
            speed.push_back( movement().m_speed );
        }

        // Go back home (this wraps so account for the go home already added)
        for ( UINT i=0; i < movement().m_home_wait_periods-1; i++ ) {
            tilt.push_back( tilt_start );
            pan.push_back( pan_start );
            dimmer.push_back( 0 );
            speed.push_back( 0 );
        }

        // Populate the channel arrays for the next group of fixtures
        populateChannelAnimations( task, participants, particpant_index, tilt, pan, 
                                   dimmer, speed, movement().m_group_size );

        if ( movement().m_alternate_groups )
            forward = !forward;
    }
}

// ----------------------------------------------------------------------------
// NOTE: Dimmer channel should contain 0 for full off and 255 for full on and
// no other values
//
void SceneMovementAnimator::populateChannelAnimations( AnimationTask* task, 
                        FixturePtrArray& participants, size_t& particpant_index,
                        AngleList& tilt, AngleList& pan, ChannelValueArray& dimmer,
                        ChannelValueArray& speed, size_t group_size )
{
    size_t end = particpant_index + group_size;

    // If run once, add last entry to shut down the lights
    if ( m_run_once && dimmer.size() > 0 ) {
        dimmer.push_back( 0 );
    }

    for ( ; particpant_index < end && particpant_index < participants.size(); particpant_index++ ) {

        Fixture* pf = participants[ particpant_index ];

        channel_t pan_channel = INVALID_CHANNEL;
        channel_t tilt_channel = INVALID_CHANNEL;
        channel_t dimmer_channel = INVALID_CHANNEL;
        channel_t speed_channel = INVALID_CHANNEL;

        getFixtureChannels( pf, speed_channel, pan_channel, tilt_channel, dimmer_channel );

        if ( tilt_channel != INVALID_CHANNEL && tilt.size() ) {
            m_channel_animations.push_back( 
                ChannelAnimation( pf->getUID(), tilt_channel, CAM_LIST, 
                                    anglesToValues( pf->getChannel( tilt_channel ), tilt ) ) );
        }

        if ( pan_channel != INVALID_CHANNEL && pan.size() ) {
            m_channel_animations.push_back( 
                ChannelAnimation( pf->getUID(), pan_channel, CAM_LIST, 
                                    anglesToValues( pf->getChannel( pan_channel ), pan ) ) );
        }

        if ( speed_channel != INVALID_CHANNEL && speed.size() ) {
            m_channel_animations.push_back( 
                ChannelAnimation( pf->getUID(), speed_channel, CAM_LIST, speed ) );
        }

        if ( dimmer_channel != INVALID_CHANNEL && dimmer.size() ) {
            Channel* ch = pf->getChannel( dimmer_channel );
            STUDIO_ASSERT( ch, "Can't access dimmer channel %d on fixture %s", dimmer_channel, pf->getFullName() );

            BYTE low = ch->getDimmerLowestIntensity();
            BYTE high = ch->getDimmerHighestIntensity();

            // Replace all 255 dimmer high value with the fixture's dimmer value iff the actors value != 0
            SceneActor* actor = task->getScene()->getActor( pf->getUID() );
            if ( actor && actor->getChannelValue( dimmer_channel ) != 0 )
                high = actor->getChannelValue( dimmer_channel );

            if ( low != 0 || high != 255 ) {                // Special case odd dimmer values
                for ( size_t i=0; i < dimmer.size(); i++ )
                    dimmer[i] = ( dimmer[i] == 255 ) ? high : low;
            }

            m_channel_animations.push_back( 
                ChannelAnimation( pf->getUID(), dimmer_channel, CAM_LIST, dimmer ) );
        }
    }
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::getFixtureChannels( Fixture* pf, channel_t& speed_channel, channel_t& pan_channel, 
                                                channel_t& tilt_channel, channel_t& dimmer_channel )
{
    pan_channel = INVALID_CHANNEL;
    tilt_channel = INVALID_CHANNEL;
    dimmer_channel = INVALID_CHANNEL;
    speed_channel = INVALID_CHANNEL;

    for ( channel_t channel=0; channel < pf->getNumChannels(); channel++ ) {
        Channel* cp = pf->getChannel( channel );

        if ( cp->getType() == CHNLT_MOVEMENT_SPEED && speed_channel == INVALID_CHANNEL ) {
            speed_channel = channel;
        }
        if ( cp->getType() == CHNLT_TILT && tilt_channel == INVALID_CHANNEL ) {
            tilt_channel = channel;
        }
        else if ( cp->getType() == CHNLT_PAN && pan_channel == INVALID_CHANNEL  ) {
            pan_channel = channel;
        }
        else if ( cp->isDimmer() && dimmer_channel == INVALID_CHANNEL ) {
            dimmer_channel = channel;
        }
    }
}

// ----------------------------------------------------------------------------
//
ChannelValueArray SceneMovementAnimator::anglesToValues( Channel* channel, AngleList& angles )
{
    ChannelValueArray values;

    for ( AngleList::iterator it=angles.begin(); it != angles.end(); ++it )
        values.push_back( channel->convertAngleToValue( (*it) ) );

    STUDIO_ASSERT( values.size() > 0, "Invalid angles array size" );
    return values;
}

// ----------------------------------------------------------------------------
//
void SceneMovementAnimator::genMoonflowerMovement( AnimationTask* task, FixturePtrArray& participants )
{
    STUDIO_ASSERT( movement().m_height > 0, "Height must be a positive integer > 0" );

    double fixture_angle_increment = 360 / participants.size();
    double start_angle = 0;

    for ( size_t particpant_index=0; particpant_index < participants.size(); ) {
        AngleList pan;
        AngleList tilt;
        ChannelValueArray dimmer;
        ChannelValueArray speed;

        UINT pan_start;
        UINT tilt_start;

        compute_pan_tilt( particpant_index, movement().m_height, movement().m_fixture_spacing, 
                          movement().m_home_x, movement().m_home_y, pan_start, tilt_start );

        double angle = start_angle;

        for ( size_t i=0; i < movement().m_positions; i++ ) {
            // Go home
            tilt.push_back( tilt_start );
            pan.push_back( pan_start );

            double target_x = movement().m_home_x + movement().m_radius * cos( angle * 180 / M_PI );
            double target_y = movement().m_home_y + movement().m_radius * sin( angle * 180 / M_PI );

            //printf( "fixture %d: %f,%f to x,y=%f,%f angle=%f\n", particpant_index, home_x, home_y, target_x, target_y, angle );

            UINT pan_target;
            UINT tilt_target;

            compute_pan_tilt( particpant_index, movement().m_height, movement().m_fixture_spacing, 
                              target_x, target_y, pan_target, tilt_target );

            //printf( "%d: pan_start=%d tilt_start=%d pan_end=%d tilt_end=%d\n", particpant_index, pan_start, tilt_start, pan_target, tilt_target);

            compute_fastest_path( pan_start, tilt_start, pan_target, tilt_target );

            //printf( "%d: pan_start=%d tilt_start=%d pan_end=%d tilt_end=%d\n", particpant_index, pan_start, tilt_start, pan_target, tilt_target);

            // Go to destination
            for ( UINT i=0; i < movement().m_dest_wait_periods; i++ ) {
                tilt.push_back( tilt_target );
                pan.push_back( pan_target );
            }

            // Go back home
            for ( UINT i=0; i < movement().m_home_wait_periods-1; i++ ) {
                tilt.push_back( tilt_start );
                pan.push_back( pan_start );
            }

            angle += movement().m_pan_increment;
            if ( angle > 360 ) 
                angle -= 360;
        }

        populateChannelAnimations( task, participants, particpant_index, tilt, pan, dimmer, speed, 1 );

        start_angle += fixture_angle_increment;
    }
}

// ----------------------------------------------------------------------------
//
void compute_fastest_path( UINT pan_current, UINT tilt_current, UINT& pan_target, UINT& tilt_target )
{
    UINT pan_alt = pan_target;

    if ( pan_target < pan_current && pan_target <= 540-180 )    // mins/max need to come from the fixture
        pan_alt = pan_target + 180;
    else if ( pan_target > pan_current && pan_target >= 180 )
        pan_alt = pan_target - 180;

    int pan_travel_target = abs( (int)pan_target - (int)pan_current );
    int pan_travel_alt = abs( (int)pan_alt - (int)pan_current );

    if ( pan_travel_target <= pan_travel_alt )
        return;

    int tilt_alt = tilt_target;
    int tilt_diff = abs( (int)tilt_target - 180 );

    if ( tilt_target < tilt_current )                          // mins/max need to come from the fixture
        tilt_alt = 180+tilt_diff;
    else if ( tilt_target > tilt_current )
        tilt_alt = 180-tilt_diff;

    if ( tilt_alt > 270 || tilt_alt < 45 )                      // Illegal move
        return;

    pan_target = pan_alt;
    tilt_target = tilt_alt;
}

// ----------------------------------------------------------------------------
// Compute pan and tilt based on a cartesian coordinate system 
// Fixture are virtually aligned on the X axis where y = 0 and x >= 0
//
void compute_pan_tilt( UINT fixture_number, double height, double fixture_spacing, 
                       double target_x, double target_y, UINT& pan, UINT& tilt )
{
    // Placement of fixtures in the virtual space
    double fixture_x = fixture_number * fixture_spacing;
    double fixture_y = 0;

    // Compute distances from fixture to target
    double x = abs(target_x - fixture_x);
    double y = abs(target_y - fixture_y);

    // Compute tilt angle (Z axis angle) in degrees using right angle fixture -> floor -> target
    double opposite = sqrt( (double)(x*x + y*y) );  // Size of opposite line segment (floor segment fixture to target)
    double tilt_angle = 180 - ((atan( opposite / height ) * 180 / M_PI));   // 180 degrees is light straight up

    // Compute pan angle in degrees
    double pan_angle = (x==0) ? 0 : atan2( y, x ) * 180 / M_PI;
    //printf( "    x=%f,y=%f %f\n",x,y,pan_angle );

    if ( target_x >= 0 && target_y >= 0 ) {                  // Q1
        //printf( "q1 %d\n", target_x > fixture_x );
        if( pan_angle == 0 )
            pan_angle = fixture_x <target_x ? 90 : fixture_x > target_x ? 270 : 180;
        else if ( target_x > fixture_x )
            pan_angle += 90;
        else
            pan_angle = 270 - pan_angle;
    }
    else if ( target_x < 0 && target_y >= 0 ) {             // Q2
        //printf( "q2\n" );
        pan_angle = 270-pan_angle;
    }
    else if ( target_x < 0 && target_y < 0 ) {              // Q3
        //printf( "q3\n" );
        pan_angle = 360-(90-pan_angle);
    }
    else if ( target_x >= 0 && target_y < 0 ) {             // Q4
        //printf( "q4 %d\n", target_x > fixture_x );
        if ( target_x > fixture_x )
            pan_angle = 90-pan_angle;
        else if ( target_x != fixture_x )
            pan_angle = 270+pan_angle;
    }

    pan_angle += 360;
    if ( pan_angle >= 540 )
        pan_angle -= 360;
        
    //printf( "F%d: ( %.1f, %.1f ) to ( %.1f, %.1f ) height=%.1f, opposite=%.2f, tilt=%.2f pan=%.2f\n----------------------------------\n", 
    //        fixture_number, fixture_x, fixture_y, target_x, target_y, height, opposite, tilt_angle, pan_angle );

    pan = static_cast<UINT>(pan_angle+0.5);
    tilt = static_cast<UINT>(tilt_angle+0.5);
}

