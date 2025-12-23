/*
 * Converter using BRL-CAD's utahrle library (newurt)
 * This tool reads an RLE file using the newurt library and writes a PPM file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rle.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.rle output.ppm\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    /* Open input RLE file */
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open input file %s\n", input_file);
        return 1;
    }

    /* Setup RLE header */
    rle_hdr in_hdr;
    rle_dflt_hdr.rle_file = fp;
    in_hdr = rle_dflt_hdr;
    
    /* Read RLE header */
    if (rle_get_setup(&in_hdr) != 0) {
        fprintf(stderr, "Error: Cannot read RLE header from %s\n", input_file);
        fclose(fp);
        return 1;
    }

    /* Extract dimensions and channels */
    int width = in_hdr.xmax - in_hdr.xmin + 1;
    int height = in_hdr.ymax - in_hdr.ymin + 1;
    int ncolors = in_hdr.ncolors;
    int alpha = in_hdr.alpha ? 1 : 0;
    
    printf("Reading %s: %dx%d, ncolors=%d, alpha=%d\n", 
           input_file, width, height, ncolors, alpha);
    
    if (in_hdr.bg_color) {
        printf("  Background color: RGB(%d, %d, %d)\n",
               ncolors > 0 ? in_hdr.bg_color[0] : 0,
               ncolors > 1 ? in_hdr.bg_color[1] : 0,
               ncolors > 2 ? in_hdr.bg_color[2] : 0);
    } else {
        printf("  No background color\n");
    }

    /* Allocate scanline buffers using rle_row_alloc */
    rle_pixel **rows = NULL;
    if (rle_row_alloc(&in_hdr, &rows) < 0) {
        fprintf(stderr, "Error: Cannot allocate row buffers\n");
        fclose(fp);
        return 1;
    }

    /* Allocate image buffer */
    unsigned char *image = (unsigned char *)malloc(width * height * 3);
    if (!image) {
        fprintf(stderr, "Error: Cannot allocate image buffer\n");
        for (int i = 0; i < ncolors + alpha; i++) free(rows[i]);
        free(rows);
        fclose(fp);
        return 1;
    }

    /* Initialize image to background color or black */
    if (in_hdr.bg_color) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                for (int c = 0; c < 3 && c < ncolors; c++) {
                    image[idx + c] = in_hdr.bg_color[c];
                }
            }
        }
    } else {
        memset(image, 0, width * height * 3);
    }

    /* Read scanlines */
    for (int y = 0; y < height; y++) {
        int scan_y = in_hdr.ymin + y;
        rle_getrow(&in_hdr, rows);
        
        /* Copy to image buffer (flip vertically for PPM) */
        int dst_y = height - 1 - y;
        for (int x = 0; x < width; x++) {
            int idx = (dst_y * width + x) * 3;
            for (int c = 0; c < 3 && c < ncolors; c++) {
                image[idx + c] = rows[c][x];
            }
        }
    }

    fclose(fp);

    /* Free row buffers */
    rle_row_free(&in_hdr, rows);

    /* Write PPM file */
    FILE *out = fopen(output_file, "wb");
    if (!out) {
        fprintf(stderr, "Error: Cannot open output file %s\n", output_file);
        free(image);
        return 1;
    }

    fprintf(out, "P6\n%d %d\n255\n", width, height);
    fwrite(image, 1, width * height * 3, out);
    fclose(out);
    free(image);

    printf("Successfully converted to %s\n", output_file);
    return 0;
}
