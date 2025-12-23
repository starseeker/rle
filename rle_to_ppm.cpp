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
    for (size_t y = 0; y < img.header.height(); y++) {
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
