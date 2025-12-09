/**
 * @file test_utah_debug.cpp
 * @brief Diagnostic tests to debug Utah RLE cross-compatibility issues
 */

#include "rle.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "rle.h"
#include "rle_put.h"
}

// Test 1: Simplest possible test - solid color
void test_solid_color_our_write() {
    std::cout << "TEST: Solid color (our write, Utah read)\n";
    
    const size_t W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3, 128);  // All gray
    
    // Write with our implementation
    FILE* fp = fopen("test_solid.rle", "wb");
    rle::Error err;
    rle::write_rgb(fp, data.data(), W, H, {}, {}, false, rle::Encoder::BG_SAVE_ALL, err);
    fflush(fp);  // IMPORTANT: Flush before close
    fclose(fp);
    
    std::cout << "  Written with our impl\n";
    
    // Read with Utah RLE
    rle_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    fp = fopen("test_solid.rle", "rb");
    hdr.rle_file = fp;
    
    if (rle_get_setup(&hdr) != 0) {
        fclose(fp);
        std::cout << "  FAILED: Utah RLE setup failed\n";
        return;
    }
    
    std::cout << "  Utah RLE header: " << (hdr.xmax - hdr.xmin + 1) << "x" 
              << (hdr.ymax - hdr.ymin + 1) << ", ncolors=" << hdr.ncolors << "\n";
    
    int w = hdr.xmax - hdr.xmin + 1;
    int h = hdr.ymax - hdr.ymin + 1;
    
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(w);
    }
    
    // Read first scanline
    rle_getrow(&hdr, scanline);
    
    std::cout << "  First pixel from Utah: R=" << (int)scanline[0][0] 
              << " G=" << (int)scanline[1][0] 
              << " B=" << (int)scanline[2][0] << "\n";
    std::cout << "  Expected: R=128 G=128 B=128\n";
    
    for (int c = 0; c < 3; c++) {
        free(scanline[c]);
    }
    fclose(fp);
    remove("test_solid.rle");
}

// Test 2: Solid color with Utah write
void test_solid_color_utah_write() {
    std::cout << "\nTEST: Solid color (Utah write, our read)\n";
    
    const size_t W = 4, H = 4;
    
    // Write with Utah RLE
    rle_hdr out_hdr;
    memset(&out_hdr, 0, sizeof(out_hdr));
    out_hdr.xmin = 0;
    out_hdr.ymin = 0;
    out_hdr.xmax = W - 1;
    out_hdr.ymax = H - 1;
    out_hdr.ncolors = 3;
    out_hdr.alpha = 0;
    out_hdr.background = 0;
    
    FILE* fp = fopen("test_solid_utah.rle", "wb");
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
    fflush(fp);  // IMPORTANT: Flush before close
    fclose(fp);
    
    std::cout << "  Written with Utah\n";
    
    // Read with our implementation
    std::vector<uint8_t> our_read;
    uint32_t w, h;
    bool has_alpha;
    rle::Error err;
    
    fp = fopen("test_solid_utah.rle", "rb");
    bool ok = rle::read_rgb(fp, our_read, w, h, &has_alpha, nullptr, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "  FAILED: Our read failed, error=" << (int)err << "\n";
        remove("test_solid_utah.rle");
        return;
    }
    
    std::cout << "  Read size: " << w << "x" << h << ", " << our_read.size() << " bytes\n";
    
    if (our_read.size() > 0) {
        std::cout << "  First pixel from our impl: R=" << (int)our_read[0] 
                  << " G=" << (int)our_read[1] 
                  << " B=" << (int)our_read[2] << "\n";
        std::cout << "  Expected: R=128 G=128 B=128\n";
    } else {
        std::cout << "  FAILED: No data read\n";
    }
    
    remove("test_solid_utah.rle");
}

// Test 3: Check file format directly
void test_file_format() {
    std::cout << "\nTEST: File format comparison\n";
    
    const size_t W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3, 128);
    
    // Write with our implementation
    FILE* fp = fopen("test_our_format.rle", "wb");
    rle::Error err;
    rle::write_rgb(fp, data.data(), W, H, {}, {}, false, rle::Encoder::BG_SAVE_ALL, err);
    fflush(fp);
    fclose(fp);
    
    // Write with Utah
    rle_hdr out_hdr;
    memset(&out_hdr, 0, sizeof(out_hdr));
    out_hdr.xmin = 0;
    out_hdr.ymin = 0;
    out_hdr.xmax = W - 1;
    out_hdr.ymax = H - 1;
    out_hdr.ncolors = 3;
    out_hdr.alpha = 0;
    out_hdr.background = 0;
    
    fp = fopen("test_utah_format.rle", "wb");
    out_hdr.rle_file = fp;
    rle_put_setup(&out_hdr);
    
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(W);
        for (size_t x = 0; x < W; x++) {
            scanline[c][x] = 128;
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
    
    // Compare file sizes
    fp = fopen("test_our_format.rle", "rb");
    fseek(fp, 0, SEEK_END);
    long our_size = ftell(fp);
    fclose(fp);
    
    fp = fopen("test_utah_format.rle", "rb");
    fseek(fp, 0, SEEK_END);
    long utah_size = ftell(fp);
    fclose(fp);
    
    std::cout << "  Our file size: " << our_size << " bytes\n";
    std::cout << "  Utah file size: " << utah_size << " bytes\n";
    
    // Read first 50 bytes of each
    uint8_t our_bytes[50] = {0};
    uint8_t utah_bytes[50] = {0};
    
    fp = fopen("test_our_format.rle", "rb");
    fread(our_bytes, 1, 50, fp);
    fclose(fp);
    
    fp = fopen("test_utah_format.rle", "rb");
    fread(utah_bytes, 1, 50, fp);
    fclose(fp);
    
    std::cout << "  First 50 bytes comparison:\n";
    std::cout << "  Byte | Our | Utah | Match?\n";
    for (int i = 0; i < 50; i++) {
        std::cout << "   " << i << "  |  " << (int)our_bytes[i] 
                  << "  |  " << (int)utah_bytes[i] 
                  << "  | " << (our_bytes[i] == utah_bytes[i] ? "YES" : "NO") << "\n";
    }
    
    remove("test_our_format.rle");
    remove("test_utah_format.rle");
}

int main() {
    std::cout << "=== Utah RLE Debugging Tests ===\n\n";
    
    test_solid_color_our_write();
    test_solid_color_utah_write();
    test_file_format();
    
    std::cout << "\n=== Done ===\n";
    return 0;
}
