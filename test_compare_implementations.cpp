#include "rle.hpp"
#include <iostream>
#include <vector>
#include <cstring>
extern "C" {
#include "rle.h"
}

int main() {
    // Create a 4x4 test image with our implementation
    const int W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3);
    
    std::cout << "Creating 4x4 image where row Y has G=Y*64:\n";
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int idx = (y * W + x) * 3;
            data[idx + 0] = 128;      // R
            data[idx + 1] = y * 64;   // G - varies with row
            data[idx + 2] = 64;       // B
        }
        std::cout << "  Memory row " << y << ": G=" << (y*64) << "\n";
    }
    
    // Write with our implementation
    FILE* fp = fopen("our_test.rle", "wb");
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Write failed\n";
        return 1;
    }
    
    // Read back with our implementation
    std::cout << "\nReading with our implementation:\n";
    std::vector<uint8_t> our_data;
    uint32_t our_w, our_h;
    bool has_alpha;
    
    fp = fopen("our_test.rle", "rb");
    ok = rle::read_rgb(fp, our_data, our_w, our_h, &has_alpha, nullptr, err);
    fclose(fp);
    
    if (ok) {
        for (uint32_t y = 0; y < our_h; y++) {
            size_t idx = (y * our_w) * 3 + 1;  // Get first pixel's G value
            std::cout << "  Memory row " << y << ": G=" << int(our_data[idx]) << "\n";
        }
    }
    
    // Read back with Utah RLE
    std::cout << "\nReading with Utah RLE:\n";
    rle_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    fp = fopen("our_test.rle", "rb");
    hdr.rle_file = fp;
    
    if (rle_get_setup(&hdr) == 0) {
        int w = hdr.xmax - hdr.xmin + 1;
        rle_pixel* scanline[3];
        for (int c = 0; c < 3; c++) {
            scanline[c] = (rle_pixel*)malloc(w);
        }
        
        // Utah reads scanlines from ymin to ymax
        for (int y = hdr.ymin; y <= hdr.ymax; y++) {
            rle_getrow(&hdr, scanline);
            std::cout << "  Scanline y=" << y << ": G=" << int(scanline[1][0]) << "\n";
        }
        
        for (int c = 0; c < 3; c++) {
            free(scanline[c]);
        }
    }
    fclose(fp);
    
    std::cout << "\nInterpretation:\n";
    std::cout << "- Our memory row 0 should be the TOP of the image\n";
    std::cout << "- If Utah reads scanline y=0 first, and it matches our row 0,\n";
    std::cout << "  then y=0 is also the TOP (not bottom)\n";
    
    return 0;
}
