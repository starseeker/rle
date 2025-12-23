/*
 * This software is copyrighted as noted below.  It may be freely copied,
 * modified, and redistributed, provided that the copyright notice is 
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
 * rlehdr.c - Print header from an RLE file.
 * 
 * Author:	Spencer W. Thomas
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	Thu Jan 22 1987
 * Copyright (c) 1987, University of Utah
 */

#include <stdio.h>
#include <rle.h>
#ifdef USE_STDLIB_H
#include <stdlib.h>
#endif

void print_hdr(), print_map(), print_codes();

/*****************************************************************
 * TAG( main )
 * 
 * Read and print in human readable form the header of an RLE file.
 *
 * Usage:
 *	rlehdr [-m] [file]
 * Inputs:
 *	-m:		Dump color map contents.
 *	-d:		Dump entire file contents.
 * 	file:		RLE file to interpret.
 * Outputs:
 * 	Prints contents of file header.
 * Assumptions:
 *	[None]
 * Algorithm:
 *	[None]
 */

void
main( argc, argv )
int argc;
char **argv;
{
    CONST_DECL char ** fname = NULL;
    CONST_DECL char *stdname = "-";
    CONST_DECL char *the_file;
    int mflag = 0, dbg_flag = 0, nfname = 0;
    int rle_err, rle_cnt;

    if ( scanargs( argc, argv, "% m%- d%- infile%*s",
		   &mflag, &dbg_flag, &nfname, &fname ) == 0 )
	exit( 1 );

    if ( nfname == 0 )
    {
	nfname = 1;
	fname = &stdname;
    }

    for ( ; nfname > 0; fname++, nfname-- )
    {
	if ( (rle_dflt_hdr.rle_file =
	      rle_open_f(cmd_name( argv ), *fname, "r")) == stdin)
	    the_file = "Standard Input";
	else
	    the_file = *fname;

	for ( rle_cnt = 0;
	      (rle_err = rle_get_setup( &rle_dflt_hdr )) == RLE_SUCCESS;
	      rle_cnt++ )
	{
	    print_hdr( the_file, &rle_dflt_hdr, rle_cnt );
	    if ( mflag )
		print_map( &rle_dflt_hdr );
	    if ( dbg_flag )
		print_codes( &rle_dflt_hdr );
	    else
		while ( rle_getskip( &rle_dflt_hdr ) != 32768 )
		    ;
	}

	/* Check for an error.  EOF or EMPTY is ok if at least one image
	 * has been read.  Otherwise, print an error message.
	 */
	if ( rle_cnt == 0 || (rle_err != RLE_EOF && rle_err != RLE_EMPTY) )
	    rle_get_error( rle_err, cmd_name( argv ), *fname );
    }

    exit( 0 );			/* success */
}


/*****************************************************************
 * TAG( print_hdr )
 * 
 * Print the RLE header information given.
 *
 * Inputs:
 * 	fname:		Name of file header was read from.
 *	the_hdr:	Header information.
 * Outputs:
 * 	Prints info on stdout.
 * Assumptions:
 *	[None]
 * Algorithm:
 *	[None]
 */
void
print_hdr( fname, the_hdr, rle_cnt )
char *fname;
rle_hdr *the_hdr;
int rle_cnt;
{
    register int i;

    if ( rle_cnt > 0 )
	printf( "RLE header information for %s image %d:\n",
		fname, rle_cnt + 1 );
    else
	printf( "RLE header information for %s:\n", fname );
    printf( "Originally positioned at (%d, %d), size (%d, %d)\n",
	    the_hdr->xmin, the_hdr->ymin,
	    the_hdr->xmax - the_hdr->xmin + 1,
	    the_hdr->ymax - the_hdr->ymin + 1 );
    printf( "Saved %d color channels%s\n", the_hdr->ncolors,
	    the_hdr->alpha ? " plus Alpha channel" : "" );
    if ( the_hdr->background == 0 )
	printf( "No background information was saved\n" );
    else
    {
	if ( the_hdr->background == 1 )
	    printf( "Saved in overlay mode with original background color" );
	else
	    printf( "Screen will be cleared to background color" );
	for ( i = 0; i < the_hdr->ncolors; i++ )
	    printf( " %d", the_hdr->bg_color[i] );
	putchar( '\n' );
    }

    if ( the_hdr->ncmap > 0 )
	printf( "%d channels of color map %d entries long were saved.\n",
		the_hdr->ncmap, 1 << the_hdr->cmaplen );

    if ( the_hdr->comments != NULL && *the_hdr->comments != NULL )
    {
	printf( "Comments:\n" );
	for ( i = 0; the_hdr->comments[i] != NULL; i++ )
	    printf( "%s\n", the_hdr->comments[i] );
    }

    putchar( '\n' );

    fflush( stdout );
}

/*****************************************************************
 * TAG( print_map )
 * 
 * Print the color map from a the_hdr structure.
 * Inputs:
 * 	the_hdr:	Sv_hdr structure containing color map.
 * Outputs:
 * 	Prints color map (if present) on stdout.
 * Assumptions:
 *	[None]
 * Algorithm:
 *	[None]
 */
void
print_map( the_hdr )
rle_hdr *the_hdr;
{
    register int i, j;
    int c, maplen, ncmap;
    rle_map * cmap;

    if ( the_hdr->ncmap == 0 )
	return;

    maplen = (1 << the_hdr->cmaplen);
    ncmap = the_hdr->ncmap;
    cmap = the_hdr->cmap;

    printf( "Color map contents, values are 16-bit(8-bit):\n" );
    for ( i = 0; i < maplen; i++ )
    {
	printf( "%3d:\t", i );
	for ( j = 0, c = 0; j < ncmap; j++, c += maplen )
	    printf( "%5d(%3d)%c", cmap[i+c], cmap[i+c] >> 8,
		    j == ncmap - 1 ? '\n' : '\t' );

    }
}


/*****************************************************************
 * TAG( print_codes )
 * 
 * Print the RLE opcodes in an RLE file.
 * Inputs:
 * 	the_hdr:		Header for RLE file (already open).
 * Outputs:
 * 	Prints file contents on stderr.
 * Assumptions:
 * 	
 * Algorithm:
 *	[None]
 */
void
print_codes( the_hdr )
rle_hdr *the_hdr;
{
    rle_pixel ** scans;

    rle_row_alloc( the_hdr, &scans );

    rle_debug(1);
    while ( !feof( the_hdr->rle_file ) && 
	    !the_hdr->priv.get.is_eof )
	rle_getrow( the_hdr, scans );

    rle_row_free( the_hdr, scans );
}
