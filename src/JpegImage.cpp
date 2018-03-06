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

#include "JpegImage.h"

// ----------------------------------------------------------------------------
//
void error_exit( j_common_ptr cinfo ) {
    jlib_error_mgr* c_err = (jlib_error_mgr*) cinfo->err;
    (*cinfo->err->format_message)(cinfo,c_err->message);
    jpeg_destroy(cinfo);
    longjmp(c_err->setjmp_buffer,1);
}

// ----------------------------------------------------------------------------
//
JpegImage::JpegImage() :
    m_jpeg( NULL )
{
}

// ----------------------------------------------------------------------------
//
JpegImage::JpegImage( const unsigned char* data, unsigned long size ) :
    m_jpeg( NULL )
{
    readJPEG( NULL, data, size );
}

// ----------------------------------------------------------------------------
//
JpegImage::JpegImage( const char * filename ) :
    m_jpeg( NULL )
{
#pragma warning(disable : 4996)

    FILE *const nfile = fopen(filename, "rb" );
    if ( nfile == NULL )
        throw std::exception( "Cannot open JPEG file" );

    try {
        readJPEG( nfile, NULL, 0 );
    }
    catch( ... ) {
        fclose( nfile );
        throw;
    }
}

// ----------------------------------------------------------------------------
//
void JpegImage::readJPEG( FILE* nfile, const unsigned char *inbuffer, unsigned long inbuffer_size )
{
    struct jpeg_decompress_struct cinfo;
    struct jlib_error_mgr jerr;

    cinfo.err = jpeg_std_error( &jerr.original );
    jerr.original.error_exit = error_exit;

    if ( setjmp( jerr.setjmp_buffer ) ) {
        throw std::exception( jerr.message );
    }

    jpeg_create_decompress(&cinfo);

    if ( nfile != NULL )
        jpeg_stdio_src( &cinfo, nfile );
    else
        jpeg_mem_src( &cinfo, inbuffer, inbuffer_size );
    
    jpeg_read_header( &cinfo,TRUE );
    jpeg_start_decompress( &cinfo );

    unsigned char* buffer = NULL;

    try {
        if ( cinfo.output_components!=1 && cinfo.output_components!=3 && cinfo.output_components!=4 ) {
            throw std::exception( "Unsupported color depth" );
        }

        initialize( cinfo.output_width, cinfo.output_height, cinfo.output_components );

        unsigned char *ptr_r = m_jpeg, 
            *ptr_g = m_jpeg + 1UL * m_pixel_width * m_pixel_height,
            *ptr_b = m_jpeg + 2UL * m_pixel_width * m_pixel_height,
            *ptr_a = m_jpeg + 3UL * m_pixel_width * m_pixel_height;

        buffer = (unsigned char*)malloc( cinfo.output_width * cinfo.output_components );
        JSAMPROW row_pointer[1];

        while ( cinfo.output_scanline < cinfo.output_height ) {
            *row_pointer = buffer;

            if ( jpeg_read_scanlines( &cinfo, row_pointer, 1 ) != 1 )
                throw std::exception( "JPEG missing data" );

            const unsigned char *ptrs = buffer;

            switch ( cinfo.output_components ) {
                case 1:
                    for ( size_t i=0; i < m_pixel_width; i++ )
                        *ptr_r++ = *ptrs++;
                    break;

                case 3:
                    for ( size_t i=0; i < m_pixel_width; i++ ) {
                        *ptr_r++ = *ptrs++;
                        *ptr_g++ = *ptrs++;
                        *ptr_b++ = *ptrs++;
                    }
                    break;

                case 4:
                    for ( size_t i=0; i < m_pixel_width; i++ ) {
                        *ptr_r++ = *ptrs++;
                        *ptr_g++ = *ptrs++;
                        *ptr_b++ = *ptrs++;
                        *ptr_a++ = *ptrs++;
                    }
                    break;
            }
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        free( buffer );
    }
    catch( ... ) {
        if ( buffer != NULL )
            free( buffer );
        throw;
    }
}


