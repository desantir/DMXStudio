// Heavily based on  https://github.com/Soreine/pixel-art-maker.git

#include "DMXStudio.h"

#include "PaletteMaker.h"

struct SortObject {
    HSV         hsv;
    long        weight;
};

// ----------------------------------------------------------------------------
//
inline RGBWA getColor(int const x, int const y, JpegImage const& image) {
    return RGBWA( image.getValue(x, y, 0), image.getValue(x, y, 1), image.getValue(x, y, 2) );
}

// ----------------------------------------------------------------------------
// comparison function between two HSVColor, used for quicksort
//
int SortObjectCompare (const void * a, const void * b)
{
    HSV * hsv1 = &((SortObject*)a)->hsv, * hsv2 = &((SortObject*)b)->hsv;

    double v1, v2;
    
    if ( hsv1->h > 0.0 || hsv2->h > 0.0 ) {             // Sort by hue, but note this does not include value
        v1 = hsv1->h;
        v2 = hsv2->h;
    }
    else {                                              // Sort by value
        v1 = hsv1->v;
        v2 = hsv2->v;
    }

    if ( v1 > v2 ) 
	    return 1;
    
    if (v1 < v2) 
	    return -1;

	return 0;
}

// ----------------------------------------------------------------------------
//
bool generatePalette( const char *path, int const K, RGBWAArray& palette, ColorWeights& weights )
{
    return generatePalette( JpegImage( path ), K, palette,  weights );
}

// ----------------------------------------------------------------------------
//
bool generatePalette( const unsigned char * data, unsigned long size, int const K, RGBWAArray& palette, ColorWeights& weights )
{
    return generatePalette( JpegImage( data, size ), K, palette, weights );
}

// ----------------------------------------------------------------------------
//
bool generatePalette( JpegImage &image, int const K, RGBWAArray& palette, ColorWeights& weights ) {
    return generatePalette( image, 0, 0, image.getWidth(), image.getHeight(), K, palette, weights );
}

// ----------------------------------------------------------------------------
//
bool generatePalette( JpegImage &image, UINT top_x, UINT top_y, UINT width, UINT height, int const K, 
                            RGBWAArray& palette, ColorWeights& weights ) 
{
    weights.clear();
    palette.clear();

    // The list of the uniques colors from the image. It corresponds to
    // the points (in RGB space) for the K-mean algorithm.
    RGBWAArray pointList;

    // The number of occurrence of each color from the RGB space in the
    // input image.
    ColorHist ch;

    // Initialize the color histogram and the point list
    for (unsigned int y=top_y ; y < top_y+height; y++) {
	    for (unsigned int x=top_x ; x < top_x+width ; x++) {     // For each pixel
            RGBWA color = getColor(x, y, image );

	        // Add the color to the histogram
	        if ( !ch.addColor( color ) ) // If the color didn't exist already, add it to the list of all the differents colors
		        pointList.push_back( color );
	    }
    }

    // Check that there are enough different colors in the original image
    if (pointList.size() < (unsigned int) K) { 
        // Not enough colors
        palette = pointList;
        return true;
    }

    // Indicates if the kmeans have changed during the last step. False
    // means that the algorithm has converged.
    bool changed = true;

    // The number of point associated with each cluster
    long * clusterWeight = new long[K];

    // The centers of the clusters in the k-mean algorithm. At the end:
    // the computed color palette
    RGBWA * kmean = new RGBWA[K];

    // The next computed kmeans. Acts as an intermediate buffer 
    Triplet * nextKMean = new Triplet[K];

    // Initialize the clusters centers (kmeans) with colors from the image

	// Select K colors evenly spaced in the list of all unique colors
	std::vector<RGBWA>::iterator iterator = pointList.begin();
	unsigned int step = pointList.size()/(unsigned int) K;
	
    for (int i = 0; i < K; i++) {
	    // Initialize the kmean with this color
	    kmean[i] = (*iterator);
	
        // Next step
	    std::advance(iterator, step);
	}

    ////////////////// K-Mean Algorithm ////////////////////////

    // These arrays are to speed up the hot K-Mean Algorithm when in debug mode (and it makes a HUGE difference)
    BYTE * kmain_reds = new BYTE[K];
    BYTE * kmain_blues = new BYTE[K];
    BYTE * kmain_greens = new BYTE[K];

    // While the algorithm modifies the kmeans
    while ( changed ) {

	    // Initialize the next k-means array
	    for ( int i = 0 ; i < K ; i++ ) {
	        nextKMean[i] = Triplet(0,0,0);
	        clusterWeight[i] = 0;

            // Debug optimization (may help in non-debug too)
            kmain_reds[i] = kmean[i].red();
            kmain_greens[i] = kmean[i].green();
            kmain_blues[i] = kmean[i].blue();
	    }

	    // Update the clusters by re-associating the points to the closest kmean
        size_t colors = pointList.size();

        for ( RGBWA* c=&pointList[0]; colors--; c++ ) {
	        // Initialize the nearest kmean as the first of the list
	        int nearest = 0;

	        // Initialize with the maximum distance
	        double nearestDist = 4096; // sqrt(256*256*256)

            BYTE r=c->red(), g=c->green(), b=c->blue();

	        // Find the closest kmean
	        for (int i = 0 ; i < K ; i++ ) {
		        // Distance from the i-th kmean

                /** Compute the squared euclidian distance between two colors */
                register long rd = (long) (r - kmain_reds[i]);
                register long gd = (long) (g - kmain_greens[i]);
                register long bd = (long) (b - kmain_blues[i]);
                double dist = (rd*rd + gd*gd + bd*bd);

		        // If the distance is best
		        if (dist < nearestDist) {
		            // Update nearest variables
		            nearestDist = dist;
		            nearest = i;
		        }
	        }
	
	        // Account the number of pixel of this color
	        long weight = ch.getColor(r, g, b);

	        // Convert to a triplet
	        Triplet triplet( r, g, b );

	        // Multiply by the weight
	        triplet.multiply(weight);

	        // Add it to the total
	        nextKMean[nearest].add(triplet);

	        // Add to the total weight of the nearest kmean
	        clusterWeight[nearest] += weight;
	    }

	    // All color pixels have been re-assigned to their nearest cluster

	    // Divide the sums of color of each kmean by their weight. The
	    // calculated barycenter is the new kmean. Finally we check
	    // whether this has changed the kmeans.
      
	    changed = false;

	    // For each kmean
	    for (int i = 0 ; i < K ; i++ ) {
	        // If the cluster is not empty
	        if(clusterWeight[i] != 0)  // Calculate the barycenter
		        nextKMean[i].divide(clusterWeight[i]);

	        // Convert as color
	        RGBWA c = nextKMean[i].getColor();

	        // Update the changed boolean
	        changed |= !(c == kmean[i] );
	        
            // Update the kmean
	        kmean[i] = c;

            kmain_reds[i] = c.red();
            kmain_greens[i] = c.green();
            kmain_blues[i] = c.blue();
	    }
    }

    delete[] kmain_reds;
    delete[] kmain_blues;
    delete[] kmain_greens;

    // The algorithm has converged. The kmean represent our colors

    // Sort the palette color
    SortObject * sortItems = new SortObject[K];
    double total_weight = 0.0;

	// Fill it with the RGB colors from the palette
	for ( int i = 0; i < K; i++ ) {
        sortItems[i].hsv = kmean[i].toHSV();
        sortItems[i].weight = clusterWeight[i];

        // printf( "%s = HSV(%f, %f, %f)\n", kmean[i].asString(), sortItems[i].hsv.h, sortItems[i].hsv.s, sortItems[i].hsv.v );

        total_weight += clusterWeight[i];
    }

	// Quicksort the palette
	qsort( sortItems, K, sizeof(SortObject), SortObjectCompare);

    // Generate the palette
    for (int i=0 ; i < K ; i++ ) {
        palette.emplace_back( RGBWA( sortItems[i].hsv ) );
        weights.emplace_back( (double)sortItems[i].weight/total_weight );
    }

    delete[] sortItems;
    delete[] nextKMean;
    delete[] kmean;
    delete[] clusterWeight;

    return true;
}


