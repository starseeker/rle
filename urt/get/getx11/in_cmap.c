/*
 * This software is copyrighted as noted below.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notices are 
 * preserved on all copies.
 * 
 * There is no warranty or other guarantee of fitness for this software,
 * it is provided solely "as is".  Bug reports or fixes may be sent
 * to the author, who may or may not act on them as he desires.
 *
 * You may not include this software in a program or other software product
 * without supplying the source, or without informing the end-user that the 
 * source is available for no extra charge.
 *
 * If you modify this software, you should include a notice giving the
 * name of the person performing the modification, the date of modification,
 * and the reason for such modification.
 */

/* 
 * in_cmap.c - Jack with the Input colormap from the rle files.
 * 
 * Author:	Martin R. Friedmann 
 * 		Dept of Electrical Engineering and Computer Science
 *		University of Michigan
 * Date:	Tue, April 12, 1990
 * Copyright (c) 1990, University of Michigan
 */

#include "getx11.h"

/* uses global command line args (iflag, display_gamma) */
/* sets in_cmap, cmlen, ncmap and mono_color */

void get_dither_colors( img )
register image_information *img;
{
    register int i, j;

    /* Input map, at least 3 channels */
    if ( !img->sep_colors && img->mono_color )
	/* If using color map directly, apply display gamma, too. */
	img->in_cmap = buildmap( &rle_dflt_hdr, 3, img->gamma,
				 display_gamma );
    else
	img->in_cmap = buildmap( &rle_dflt_hdr, 3, img->gamma, 1.0 );
    

    if ( img->sep_colors && display_gamma != 1.0 )
	for (i = 0; i < 3; i++ ) 
	    for (j = 0; j < 256; j++)
		img->in_cmap[i][j] = (rle_pixel)
		    (0.5 + 255 * pow(img->in_cmap[i][j] / 255.0,
				     1.0/display_gamma));

    for (i = 0; i < 3; i++ ) {
	for (j = 0; j < 256; j++)
	    if ( img->in_cmap[i][j] != j )
		break;
	if (j < 256) break;
    }
    /* if i and j are maxed out, then in_cmap is the identity... */
    if ( i == 3 && j == 256 )
	img->in_cmap = NULL;
    else
	if ( debug_flag ) {
	    for (i = 0; i < 3; i++ ) {
		fprintf(stderr, "Input image colormap:\n");
		if ( i != 0 && img->in_cmap[i] == img->in_cmap[i-1] ) 
		    fprintf (stderr, "Channel #%d is the Same as #0\n");
		else
		    for (j = 0; j < 256; j += 16) {
			int k;
			fprintf (stderr, "%3d: ", j );
			for (k = 0; k < 16 ; k++ )
			    fprintf(stderr, "%3d ", img->in_cmap[i][j+k]);
			fprintf (stderr, "\n");
		    }
	    }
	}

    do {
	char * v;
	if ( (v = rle_getcom( "color_map_length", &rle_dflt_hdr )) != NULL ) {
	    img->cmlen = atoi( v );
	    /* Protect against bogus information */
	    if ( img->cmlen < 0 )
		img->cmlen = 0;
	    if ( img->cmlen > 256 )
		img->cmlen = 256;
	} else img->cmlen = 256;
	
    } while (0);

    img->ncmap = rle_dflt_hdr.ncmap;
    img->mono_color = (img->img_channels == 1 && img->ncmap == 3 &&
		       img->in_cmap && img->cmlen);

    /* make colormap monochrome...   Whatahack! */
    if ( img->mono_color && !img->color_dpy ) {
	for (j = 0; j < 256; j++)
	    img->in_cmap[0][j] = (rle_pixel)
		((35 * img->in_cmap[0][j] + 55 * img->in_cmap[1][j] +
		  10 * img->in_cmap[2][j]) / 100);
	
	img->mono_color = False;
    }
}    

int eq_cmap( cm1, cm2 )
register rle_pixel **cm1, **cm2;
{
    register int i, j;
    
    if (cm1 && cm2) {
	for (i = 0; i < 3; i++ ) 
	    for (j = 0; j < 256; j++)
		if ( cm1[i][j] != cm2[i][j] )
		    return 0;
    } else
	if (cm1 || cm2) 
	    return 0;
    return 1;
}
