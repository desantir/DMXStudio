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

#include "stdafx.h"

class Bitmap
{
protected:
    size_t  m_capacity;
    BYTE*   m_bitmap;

private:
    Bitmap( const  Bitmap& );
    Bitmap& operator=( const Bitmap& );

public:
    Bitmap( size_t initial_max = 1024 ) :
        m_capacity( initial_max / 8 )
    {
        m_bitmap = (BYTE *)calloc( m_capacity, 1 );
    }

    ~Bitmap( ) {
        free( m_bitmap );
    }

    void set( size_t bit ) {
        unsigned byte_num = bit / 8;
        unsigned bit_num = 7-(bit % 8);

        if ( byte_num >= m_capacity ) {
            m_bitmap = (BYTE *)realloc( m_bitmap, m_capacity*2 );
            memset( &m_bitmap[m_capacity], 0, m_capacity );
            m_capacity *= 2;
        }

        m_bitmap[ byte_num ] |= (1<<bit_num);
    }

    bool isSet( size_t bit ) const {
        unsigned byte_num = bit / 8;
        unsigned bit_num = 7-(bit % 8);

        if ( byte_num >= m_capacity )
            return false;

        return (m_bitmap[ byte_num ] & (1<<bit_num)) ? true : false;
    }

    void clear() {
        memset( m_bitmap, 0, m_capacity );
    }
};
