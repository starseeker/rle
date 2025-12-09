/**
 * @file test_utah_gradient_debug.cpp
 * @brief Debug gradient encoding/decoding
 */

#include <iostream>
#include <vector>
#include <cstring>
#include <cstdio>

extern "C" {
#include "rle.h"
#include "rle_put.h"
}

#include "rle.hpp"

int main() {
    std::cout << "=== Utah Gradient Debug ===\n\n";
    
    const size_t W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3);
    
    // Simple gradient
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            data[idx + 0] = (x * 255) / (W - 1);
            data[idx + 1] = (y * 255) / (H - 1);
            data[idx + 2] = 128;
        }
    }
    
    std::cout << "Original data (first 4 pixels):\n";
    for (int i = 0; i < 4; i++) {
        printf("  Pixel %d: R=%d G=%d B=%d\n", i,
               data[i*3], data[i*3+1], data[i*3+2]);
    }
    std::cout << "\n";
    
    // Write with Utah RLE
    rle_hdr out_hdr;
    memset(&out_hdr, 0, sizeof(out_hdr));
    out_hdr.xmin = 0;
    out_hdr.ymin = 0;
    out_hdr.xmax = W - 1;
    out_hdr.ymax = H - 1;
    out_hdr.ncolors = 3;
    out_hdr.alpha = 0;
    out_hdr.background = 0;  // No background
    
    FILE* fp = fopen("utah_gradient.rle", "wb");
    out_hdr.rle_file = fp;
    rle_put_setup(&out_hdr);
    
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(W);
    }
    
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            scanline[0][x] = data[idx + 0];
            scanline[1][x] = data[idx + 1];
            scanline[2][x] = data[idx + 2];
        }
        rle_putrow(scanline, W, &out_hdr);
    }
    
    rle_puteof(&out_hdr);
    for (int c = 0; c < 3; c++) {
        free(scanline[c]);
    }
    fflush(fp);
    fclose(fp);
    
    std::cout << "Utah RLE file written\n\n";
    
    // Read back with Utah RLE
    rle_hdr in_hdr;
    memset(&in_hdr, 0, sizeof(in_hdr));
    fp = fopen("utah_gradient.rle", "rb");
    in_hdr.rle_file = fp;
    rle_get_setup(&in_hdr);
    
    std::cout << "Utah RLE read header:\n";
    printf("  Size: %dx%d\n", in_hdr.xmax - in_hdr.xmin + 1, in_hdr.ymax - in_hdr.ymin + 1);
    printf("  ncolors: %d\n", in_hdr.ncolors);
    printf("  flags: 0x%02x\n", in_hdr.bits[0]);
    printf("  background: %d\n", in_hdr.background);
    std::cout << "\n";
    
    int w = in_hdr.xmax - in_hdr.xmin + 1;
    
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(w);
    }
    
    std::vector<uint8_t> utah_read(W * H * 3);
    for (int y = in_hdr.ymin; y <= in_hdr.ymax; y++) {
        rle_getrow(&in_hdr, scanline);
        int row = y - in_hdr.ymin;
        for (int x = 0; x < w; x++) {
            size_t idx = (row * W + x) * 3;
            utah_read[idx + 0] = scanline[0][x];
            utah_read[idx + 1] = scanline[1][x];
            utah_read[idx + 2] = scanline[2][x];
        }
    }
    
    for (int c = 0; c < 3; c++) {
        free(scanline[c]);
    }
    fclose(fp);
    
    std::cout << "Utah RLE reads back (first 4 pixels):\n";
    for (int i = 0; i < 4; i++) {
        printf("  Pixel %d: R=%d G=%d B=%d\n", i,
               utah_read[i*3], utah_read[i*3+1], utah_read[i*3+2]);
    }
    std::cout << "\n";
    
    // Read with our implementation
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    rle::Error err;
    
    fp = fopen("utah_gradient.rle", "rb");
    bool ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    
    std::cout << "Our implementation reads back (first 4 pixels):\n";
    if (ok && our_read.size() >= 12) {
        for (int i = 0; i < 4; i++) {
            printf("  Pixel %d: R=%d G=%d B=%d\n", i,
                   our_read[i*3], our_read[i*3+1], our_read[i*3+2]);
        }
    } else {
        std::cout << "  FAILED: error=" << (int)err << "\n";
    }
    std::cout << "\n";
    
    remove("utah_gradient.rle");
    
    return 0;
}
