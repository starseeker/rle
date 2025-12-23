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
/* convert an RLE image into raw data for the macintosh */
/* usage: rletoraw infile rawfile                  */

#include "stdio.h"
#include "rle.h"	/* you need to find this file on your system */

#ifdef USE_STDLIB_H
#include <stdlib.h>
#else

#ifdef VOID_STAR
extern void *malloc();
#else
extern char *malloc();
#endif
extern void free();

#endif /* USE_STDLIB_H */

void
main(argc, argv)
int	argc;
char	*argv[];
{
    char	       *infname = NULL;
    char	       *outfname = NULL;
   rle_hdr   the_hdr;
   unsigned char       *rows[3], pixel[3];
   int		       width, height, i, j, oflag;
   FILE		       *rawfile;

   if ( scanargs( argc, argv, "% o%-outfile!s infile%s",
		  &oflag, &outfname, &infname ) == 0 )
       exit( 1 );

   the_hdr.rle_file = rle_open_f( "rletoraw", infname, "r" );

   rle_get_setup(&the_hdr);
   RLE_CLR_BIT( the_hdr, RLE_ALPHA );
   for ( i = 3; i < the_hdr.ncolors; i++ )
      RLE_CLR_BIT( the_hdr, i );

   rawfile = rle_open_f( "rletoraw", outfname, "w" );

   width  = the_hdr.xmax - the_hdr.xmin + 1;
   height = the_hdr.ymax - the_hdr.ymin + 1;
   fprintf( stderr, "width = %d, height = %d\n", width, height );
   rows[0] = (unsigned char *)malloc(width);
   rows[1] = (unsigned char *)malloc(width);
   rows[2] = (unsigned char *)malloc(width);

   for ( i=0; i<height; i++ ) {
      rle_getrow( &the_hdr, rows );
      for ( j=0; j<width; j++ ) {
	 pixel[0] = rows[0][j];
	 pixel[1] = rows[1][j];
	 pixel[2] = rows[2][j];
	 fwrite( pixel, 1, 3, rawfile );
      }
   }

   fclose( the_hdr.rle_file );
   fclose( rawfile );
}

