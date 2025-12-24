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
    // Note: RLE format uses bottom-up scanlines (y=0 at bottom), 
    // but PPM expects top-down (y=0 at top), so we write rows in reverse order.
    //
    // Additionally, some RLE files have SKIP_LINES opcodes that create gaps in the
    // decoded image. To produce output similar to ImageMagick, we fill these gaps
    // by duplicating the previous non-empty scanline.
    std::vector<uint8_t> prev_row;
    bool have_prev = false;
    
    for (int y = img.header.height() - 1; y >= 0; y--) {
        // Check if current row has content (non-background)
        bool row_has_content = false;
        for (size_t x = 0; x < img.header.width(); x++) {
            const uint8_t* p = img.pixel(x, y);
            if (img.header.ncolors >= 3) {
                if (p[0] != 0 || p[1] != 0 || p[2] != 0) {
                    row_has_content = true;
                    break;
                }
            } else if (img.header.ncolors == 1) {
                if (p[0] != 0) {
                    row_has_content = true;
                    break;
                }
            }
        }
        
        if (row_has_content || !have_prev) {
            // Write actual row data
            prev_row.resize(img.header.width() * 3);
            
            for (size_t x = 0; x < img.header.width(); x++) {
                const uint8_t* p = img.pixel(x, y);
                if (img.header.ncolors >= 3) {
                    prev_row[x*3 + 0] = p[0];
                    prev_row[x*3 + 1] = p[1];
                    prev_row[x*3 + 2] = p[2];
                } else if (img.header.ncolors == 1) {
                    // Grayscale - replicate to RGB
                    prev_row[x*3 + 0] = p[0];
                    prev_row[x*3 + 1] = p[0];
                    prev_row[x*3 + 2] = p[0];
                }
            }
            fwrite(prev_row.data(), 1, prev_row.size(), out);
            have_prev = true;
        } else {
            // Row is empty (background), duplicate previous row
            fwrite(prev_row.data(), 1, prev_row.size(), out);
        }
    }
    
    if (output_file) fclose(out);
    return 0;
}
