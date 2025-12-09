#include "rle.hpp"
#include <cstdio>
#include <cstring>

int main() {
    // Simple test: 20 rows, rows 5-14 are background
    rle::Image img;
    img.header.xpos = 0; img.header.ypos = 0;
    img.header.xlen = 10; img.header.ylen = 20;
    img.header.ncolors = 3; img.header.pixelbits = 8;
    img.header.ncmap = 0; img.header.cmaplen = 0;
    img.header.background = {100, 150, 200};
    img.header.flags = 0;  // Background enabled
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Allocate failed\n");
        return 1;
    }
    
    // Rows 0-4: Pattern A (50, 75, 25)
    for (uint32_t y = 0; y < 5; y++) {
        for (uint32_t x = 0; x < 10; x++) {
            img.pixel(x, y)[0] = 50;
            img.pixel(x, y)[1] = 75;
            img.pixel(x, y)[2] = 25;
        }
    }
    // Rows 5-14: Background (100, 150, 200) - already set
    // Rows 15-19: Pattern B (200, 100, 50)
    for (uint32_t y = 15; y < 20; y++) {
        for (uint32_t x = 0; x < 10; x++) {
            img.pixel(x, y)[0] = 200;
            img.pixel(x, y)[1] = 100;
            img.pixel(x, y)[2] = 50;
        }
    }
    
    printf("Original image:\n");
    for (uint32_t y = 0; y < 20; y++) {
        printf("Row %2d: R=%3d G=%3d B=%3d\n", y, 
               img.pixel(0, y)[0], img.pixel(0, y)[1], img.pixel(0, y)[2]);
    }
    
    // Write with BG_OVERLAY
    FILE* f = fopen("/tmp/test_skip.rle", "wb");
    if (!f) return 1;
    rle::Error werr;
    if (!rle::Encoder::write(f, img, rle::Encoder::BG_OVERLAY, werr)) {
        fprintf(stderr, "Write failed: %s\n", rle::error_string(werr));
        fclose(f);
        return 1;
    }
    fclose(f);
    
    // Read back
    f = fopen("/tmp/test_skip.rle", "rb");
    if (!f) return 1;
    rle::Image out;
    rle::DecoderResult res = rle::Decoder::read(f, out);
    fclose(f);
    
    if (!res.ok) {
        fprintf(stderr, "Read failed: %s\n", rle::error_string(res.error));
        return 1;
    }
    
    printf("\nDecoded image:\n");
    for (uint32_t y = 0; y < 20; y++) {
        printf("Row %2d: R=%3d G=%3d B=%3d%s\n", y, 
               out.pixel(0, y)[0], out.pixel(0, y)[1], out.pixel(0, y)[2],
               (out.pixel(0, y)[0] != img.pixel(0, y)[0] ||
                out.pixel(0, y)[1] != img.pixel(0, y)[1] ||
                out.pixel(0, y)[2] != img.pixel(0, y)[2]) ? " <== MISMATCH" : "");
    }
    
    return 0;
}
