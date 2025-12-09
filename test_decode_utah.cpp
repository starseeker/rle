/**
 * @file test_decode_utah.cpp
 * @brief Debug our decoder reading Utah RLE files
 */

#include "rle.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdio>

extern "C" {
#include "rle.h"
#include "rle_put.h"
}

int main() {
    std::cout << "=== Debugging Our Decoder with Utah Files ===\n\n";
    
    // Create a simple Utah RLE file
    const size_t W = 4, H = 4;
    
    rle_hdr out_hdr;
    memset(&out_hdr, 0, sizeof(out_hdr));
    out_hdr.xmin = 0;
    out_hdr.ymin = 0;
    out_hdr.xmax = W - 1;
    out_hdr.ymax = H - 1;
    out_hdr.ncolors = 3;
    out_hdr.alpha = 0;
    out_hdr.background = 0;
    
    FILE* fp = fopen("utah_simple.rle", "wb");
    if (!fp) {
        std::cout << "  FAILED: Cannot create test file\n";
        return 1;
    }
    out_hdr.rle_file = fp;
    rle_put_setup(&out_hdr);
    
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(W);
        for (size_t x = 0; x < W; x++) {
            scanline[c][x] = 128;  // All gray
        }
    }
    
    for (size_t y = 0; y < H; y++) {
        rle_putrow(scanline, W, &out_hdr);
    }
    
    rle_puteof(&out_hdr);
    for (int c = 0; c < 3; c++) {
        free(scanline[c]);
    }
    fflush(fp);
    fclose(fp);
    
    std::cout << "Created utah_simple.rle\n";
    
    // Hexdump the file
    fp = fopen("utah_simple.rle", "rb");
    std::vector<uint8_t> file_data;
    uint8_t byte;
    while (fread(&byte, 1, 1, fp) == 1) {
        file_data.push_back(byte);
    }
    fclose(fp);
    
    std::cout << "File size: " << file_data.size() << " bytes\n";
    std::cout << "Hexdump:\n";
    for (size_t i = 0; i < file_data.size(); i++) {
        if (i % 16 == 0) {
            if (i > 0) std::cout << "\n";
            printf("%04zx: ", i);
        }
        printf("%02x ", file_data[i]);
    }
    std::cout << "\n\n";
    
    // Try to read with our implementation - with debug output
    std::cout << "Reading with our implementation...\n";
    
    std::vector<uint8_t> our_read;
    uint32_t w, h;
    bool has_alpha;
    rle::Error err;
    
    fp = fopen("utah_simple.rle", "rb");
    bool ok = rle::read_rgb(fp, our_read, w, h, &has_alpha, nullptr, err);
    fclose(fp);
    
    std::cout << "Result: " << (ok ? "SUCCESS" : "FAILED") << "\n";
    std::cout << "Error code: " << (int)err << "\n";
    std::cout << "Size: " << w << "x" << h << "\n";
    std::cout << "Data size: " << our_read.size() << " bytes\n";
    
    if (our_read.size() >= 12) {
        std::cout << "First 4 pixels:\n";
        for (int i = 0; i < 4; i++) {
            int idx = i * 3;
            printf("  Pixel %d: R=%d G=%d B=%d\n", i, 
                   our_read[idx], our_read[idx+1], our_read[idx+2]);
        }
    }
    
    std::cout << "\nExpected: All pixels should be R=128 G=128 B=128\n";
    
    remove("utah_simple.rle");
    
    return 0;
}
