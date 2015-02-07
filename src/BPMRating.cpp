/* 
Copyright (C) 2014 Robert DeSantis
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

BPMInfo BPMRatings[] = {
    BPMInfo( "No rating", 0, 999 ),                    // BPM_NO_RATING
    BPMInfo( "Very slow < 60 BPM", 0, 59 ),            // BPM_VERY_SLOW
    BPMInfo( "Slow 60-79 BPM", 60, 79 ),               // BPM_SLOW
    BPMInfo( "Medium 80-119 BPM", 80, 119 ),           // BPM_MEDIUM
    BPMInfo( "Fast 120-149 BPM", 120, 149 ),           // BPM_FAST
    BPMInfo( "Very fast >= 150 BPM", 150, 999 )        // BPM_VERY_FAST
};

// ----------------------------------------------------------------------------
//
BPMRating computeBPMRating( UINT BPM ) {
    UINT rating=BPM_END;

    while ( rating > BPM_NO_RATING )                    // Backwards through list - find rating
        if ( BPM >= BPMRatings[--rating].lower )
            break;

    return (BPMRating)rating;
}

// ----------------------------------------------------------------------------
//
LPCSTR getRatingName( BPMRating rating ) {
    if ( rating >= BPM_END )
        return "INVALID RATING VALUE";
    return BPMRatings[rating].name;
} 