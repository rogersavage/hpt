#include <stdio.h>
#include <stdlib.h>

void fractal_noise(int seed_start, 
int range, int width, int octaves, float fbias, 
int* output){
    srand(seed_start);
    float seed[width][width];
    for(int y=0; y<width; y++)
    for(int x=0; x<width; x++)
	seed[x][y] = (float)(rand() % 1000000) 
	    / 1000000.0f;

    for(int y=0; y<width; y++){
	for(int x=0; x<width; x++){
	    float fscale = 1.0f;
	    float fnoise = 0.0f;
	    float fscaleacc = 0.0f;

	    for(int o=0; o<octaves; o++){
		int npitch = width >> o;
		int nsamplex1 = (x/npitch) * npitch;
		int nsampley1 = (y/npitch) * npitch;

		int nsamplex2 = (nsamplex1 + npitch)
		    % width;
		int nsampley2 = (nsampley1 + npitch)
		    % width;
		float fblendx = 
		    (float) (x - nsamplex1) /
		    (float)npitch;
		float fblendy = (float) 
		    (y - nsampley1) /
		    (float)npitch;

		float fsamplet = (1.0f - fblendx) *
		    seed[nsamplex1][nsampley1]
		    + fblendx * 
		    seed[nsamplex2][nsampley1];

		float fsampleb = (1.0f - fblendx) *
		    seed[nsamplex1][nsampley2]
		    + fblendx * 
		    seed[nsamplex2][nsampley2];

		fnoise += (fblendy * (fsampleb - 
		    fsamplet) + fsamplet) * fscale;
		fscaleacc += fscale;
		fscale = fscale / fbias;
	    }
	    output[x + y * width] = (int) (range * 
		    (fnoise / fscaleacc));
	}
    }
}
