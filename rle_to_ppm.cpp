/*
 * rle_to_ppm - Convert Utah RLE format to PPM format
 *
 * This utility demonstrates the RLE decoder's colormap support by
 * converting RLE files (with or without colormaps) to standard PPM format.
 *
 * Usage:
 *   rle_to_ppm input.rle [output.ppm]
 *
 * Features:
 *   - Applies colormaps correctly (if present in RLE file)
 *   - Handles RGB and RGBA images (alpha channel discarded in PPM)
 *   - Converts grayscale to RGB for PPM compatibility
 *   - Can decode files that ImageMagick's convert cannot (e.g., mandrill.rle)
 *
 * Examples:
 *   rle_to_ppm mandrill.rle mandrill.ppm
 *   rle_to_ppm lenna.rle > lenna.ppm
 */

#include "rle.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.rle> [output.ppm]\n", argv[0]);
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argc > 2 ? argv[2] : nullptr;
    
    FILE* f = fopen(input_file, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", input_file);
        return 1;
    }
    
    rle::Image img;
    auto result = rle::Decoder::read(f, img);
    fclose(f);
    
    if (!result.ok) {
        fprintf(stderr, "Failed to decode: %s\n", rle::error_string(result.error));
        return 1;
    }
    
    FILE* out = output_file ? fopen(output_file, "wb") : stdout;
    if (!out) {
        fprintf(stderr, "Cannot open output file\n");
        return 1;
    }
    
    // Write PPM header
    fprintf(out, "P6\n%u %u\n255\n", img.header.width(), img.header.height());
    
    // Write pixel data (RGB only, ignore alpha if present)
    // RLE format uses bottom-up scanlines (y=0 at bottom), but PPM expects top-down (y=0 at top).
    // Write rows in reverse order to convert from bottom-up to top-down.
    for (int y = img.header.height() - 1; y >= 0; y--) {
        for (size_t x = 0; x < img.header.width(); x++) {
            const uint8_t* p = img.pixel(x, y);
            if (img.header.ncolors >= 3) {
                fwrite(p, 1, 3, out);
            } else if (img.header.ncolors == 1) {
                // Grayscale - replicate to RGB
                fputc(p[0], out);
                fputc(p[0], out);
                fputc(p[0], out);
            }
        }
    }
    
    if (output_file) fclose(out);
    return 0;
}
