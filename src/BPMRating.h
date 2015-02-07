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

enum BPMRating {
    BPM_NO_RATING = 0,              // No rating defined
    BPM_VERY_SLOW = 1,              // Very slow < 60 BPM
    BPM_SLOW = 2,                   // Slow 60-79 BPM
    BPM_MEDIUM = 3,                 // Medium 80-119 BMP
    BPM_FAST = 4,                   // Fast 120-149 BPM
    BPM_VERY_FAST = 5,              // Very fast > 150 BPM
    BPM_END
};

struct BPMInfo {
    LPCSTR name;
    UINT lower;
    UINT upper;

    BPMInfo( LPCSTR desc, UINT low, UINT high ) :
        name( desc ),
        lower( low ),
        upper( high ) {}
};

extern BPMRating computeBPMRating( UINT BPM );
extern BPMInfo BPMRatings[];
extern LPCSTR getRatingName( BPMRating rating );