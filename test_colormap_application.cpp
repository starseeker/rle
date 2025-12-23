#include "rle.hpp"
#include <cstdio>
#include <cstring>

int main() {
    // Test with mandrill.rle which has a colormap
    FILE* f = fopen("imgs/mandrill.rle", "rb");
    if (!f) {
        fprintf(stderr, "Cannot open mandrill.rle\n");
        return 1;
    }
    
    rle::Image img;
    auto result = rle::Decoder::read(f, img);
    fclose(f);
    
    if (!result.ok) {
        fprintf(stderr, "Failed to decode: %s\n", rle::error_string(result.error));
        return 1;
    }
    
    printf("Successfully decoded mandrill.rle\n");
    printf("Dimensions: %ux%u\n", img.header.width(), img.header.height());
    printf("Has colormap: ncmap=%u, cmaplen=%u\n", img.header.ncmap, img.header.cmaplen);
    
    // Show first few pixel values (should now be post-colormap)
    printf("\nFirst 5 pixel values (AFTER colormap application):\n");
    for (int i = 0; i < 5 && i < img.header.width(); i++) {
        const uint8_t* p = img.pixel(i, 0);
        printf("  [%d] R=%u G=%u B=%u\n", i, p[0], p[1], p[2]);
    }
    
    // Test roundtrip: encode and decode again
    FILE* tmp = fopen("/tmp/test_mandrill.rle", "wb");
    if (!tmp) {
        fprintf(stderr, "Cannot create temp file\n");
        return 1;
    }
    
    rle::Error err;
    if (!rle::Encoder::write(tmp, img, rle::Encoder::BG_SAVE_ALL, err)) {
        fprintf(stderr, "Failed to encode: %s\n", rle::error_string(err));
        fclose(tmp);
        return 1;
    }
    fclose(tmp);
    
    // Read it back
    tmp = fopen("/tmp/test_mandrill.rle", "rb");
    if (!tmp) {
        fprintf(stderr, "Cannot open temp file\n");
        return 1;
    }
    
    rle::Image img2;
    result = rle::Decoder::read(tmp, img2);
    fclose(tmp);
    
    if (!result.ok) {
        fprintf(stderr, "Failed to decode roundtrip: %s\n", rle::error_string(result.error));
        return 1;
    }
    
    // Compare pixels
    bool match = true;
    size_t diff_count = 0;
    for (size_t y = 0; y < img.header.height() && y < img2.header.height(); y++) {
        for (size_t x = 0; x < img.header.width() && x < img2.header.width(); x++) {
            const uint8_t* p1 = img.pixel(x, y);
            const uint8_t* p2 = img2.pixel(x, y);
            for (size_t c = 0; c < img.header.channels() && c < img2.header.channels(); c++) {
                if (p1[c] != p2[c]) {
                    if (diff_count < 5) {
                        printf("Pixel mismatch at (%zu,%zu) channel %zu: %u vs %u\n", 
                               x, y, c, p1[c], p2[c]);
                    }
                    diff_count++;
                    match = false;
                }
            }
        }
    }
    
    if (match) {
        printf("\nRoundtrip test: PASSED - all pixels match\n");
        return 0;
    } else {
        printf("\nRoundtrip test: FAILED - %zu pixels differ\n", diff_count);
        return 1;
    }
}
