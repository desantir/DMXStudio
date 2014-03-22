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

#include "Bitmap.h"

template <class T>
class FindNextAvailable : public Bitmap
{
    FindNextAvailable( const  FindNextAvailable& );
    FindNextAvailable& operator=( const FindNextAvailable& );

public:
    FindNextAvailable( size_t initial_max = 1024, bool ignore_zero = true) :
        Bitmap( initial_max )
    {
        if ( ignore_zero )
            set(0);
    }

    ~FindNextAvailable( ) {
    }

    void insert( T value ) {
        set( value );
    }

    T nextAvailable() const {
        T next = 0;
       
        BYTE mask = 0x80;
        BYTE *ptr = m_bitmap;
        T fence = m_capacity * 2;

        for ( ; next < fence; next++ ) {
            if ( !(*ptr & mask) )
                break;

            if ( !(mask >>= 1) ) {
                ptr++;
                mask = 0x80;
            }
        }

        return next;
    }
};
