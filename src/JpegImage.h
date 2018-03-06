/* 
Copyright (C) 2017 Robert DeSantis
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

#include <stdlib.h>
#include <memory>

#include "jpeglib.h"
#include "setjmp.h"

class JpegImage {
    unsigned int    m_pixel_width;
    unsigned int    m_pixel_height;
    unsigned int    m_pixel_depth;
    unsigned long   m_data_size;

    unsigned char*  m_jpeg;

    JpegImage& operator=( const JpegImage& rhs );

public:
    JpegImage( );
    JpegImage( const char * filename );
    JpegImage( const unsigned char * data, unsigned long size );

    JpegImage( const JpegImage& other )
    {
        initialize( other.m_pixel_width, other.m_pixel_height, other.m_pixel_depth );
        memcpy( m_jpeg, other.m_jpeg, m_data_size );
    }

    ~JpegImage() {
        free( m_jpeg );
    }

    inline unsigned int getHeight() const {
        return m_pixel_height;
    }

    inline unsigned int getWidth() const {
        return m_pixel_width;
    }

    inline unsigned int getDepth() const {
        return m_pixel_depth;
    }

    inline unsigned char* getData() {
        return m_jpeg;
    }

    inline unsigned char getValue( const int column, const int row, const int channel ) const {
        return m_jpeg[ (channel * m_pixel_width * m_pixel_height) + (row*m_pixel_width) + column ];
    }

    void readJPEG( FILE* nfile, const unsigned char *inbuffer, unsigned long inbuffer_size );

private:
    void initialize( unsigned int pixel_width, unsigned int pixel_height, unsigned int pixel_depth )
    {
        m_pixel_width = pixel_width;
        m_pixel_height = pixel_height;
        m_pixel_depth = pixel_depth;

        m_data_size = m_pixel_width * m_pixel_height * m_pixel_depth;

        m_jpeg = (unsigned char*)malloc( m_data_size );
    }
};

struct jlib_error_mgr {
    struct jpeg_error_mgr original;
    jmp_buf setjmp_buffer;
    char message[JMSG_LENGTH_MAX];
};

