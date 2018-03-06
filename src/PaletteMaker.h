// Heavily based on  https://github.com/Soreine/pixel-art-maker.git

#pragma once

#include "DMXStudio.h"
#include "RGBWA.h"
#include "JpegImage.h"

class Triplet {
public :
    long values[3];

    Triplet() {}

    Triplet(long x, long y, long z) {
        values[0] = x;
        values[1] = y;
        values[2] = z;
    }

    /** Add each relative coordinates of the given Triplet to this one. */
    inline void add(Triplet const& t) {
        values[0] += t.values[0];
        values[1] += t.values[1];
        values[2] += t.values[2];
    }

    /** Add the R,G,B components of 'c' multiplied by 'm' to the x, y, z
    values of this Triplet */
    inline void multiply(unsigned int m) {
        values[0] *=m;
        values[1] *=m;
        values[2] *=m;
    }

    /** Divide all the values by 'd' */
    inline void divide(long d) {
        values[0] = lround((double)values[0]/(double)d);
        values[1] = lround((double)values[1]/(double)d);
        values[2] = lround((double)values[2]/(double)d);
    }

    /** Interpret this Triplet as a color */
    inline RGBWA getColor() const {
        return RGBWA( (unsigned char)values[0], (unsigned char)values[1], (unsigned char)values[2]);
    }
};

#define COLOR_RANGE 256

/** Can hold a count for the occurrence of each color in an image */
class ColorHist {
    struct tab3d { unsigned int count[COLOR_RANGE][COLOR_RANGE][COLOR_RANGE]; } * m_rgbSpace;

public :
    ColorHist() {
        m_rgbSpace = (tab3d *)calloc( 1, sizeof(tab3d) );
    }

    ~ColorHist() {
        free( m_rgbSpace );
    }

    /** Get the count for the given color */
    inline unsigned int getColor(RGBWA& c) const {
        return m_rgbSpace->count[c.red()][c.green()][c.blue()];
    }

    /** Get the count for the color with the given RGB components */
    inline unsigned int getColor(unsigned char r, unsigned  char g, unsigned  char b) const {
        return m_rgbSpace->count[r][g][b];
    }

    inline bool addColor(RGBWA& c) {
        return m_rgbSpace->count[c.red()][c.green()][c.blue()]++ > 0;
    }
};

extern bool generatePalette( const char* filename, int const K, RGBWAArray& palette, ColorWeights& weights );

extern bool generatePalette( const unsigned char * data, unsigned long size, 
                            int const K, RGBWAArray& palette, ColorWeights& weights );

extern bool generatePalette( JpegImage &image, UINT top_x, UINT top_y, UINT width, UINT height, int const K, 
                             RGBWAArray& palette, ColorWeights& weights );

extern bool generatePalette( JpegImage &image, int const K, RGBWAArray& palette, ColorWeights& weights );
