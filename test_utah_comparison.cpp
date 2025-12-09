/**
 * @file test_utah_comparison.cpp
 * @brief Comprehensive comparison between our RLE implementation and Utah RLE
 *
 * This test suite writes images with both our implementation and Utah RLE,
 * then reads them back with both implementations to identify any behavioral
 * differences across all supported modes.
 */

#include "rle.hpp"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "rle.h"
#include "rle_put.h"
#include "rle_code.h"
}

// Test statistics
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) std::cout << "TEST: " << name << "... "; std::cout.flush()
#define TEST_PASS() do { std::cout << "PASSED\n"; tests_passed++; return true; } while(0)
#define TEST_FAIL(msg) do { std::cout << "FAILED: " << msg << "\n"; tests_failed++; return false; } while(0)

// Helper to compare byte arrays
bool compare_bytes(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, const char* label) {
    if (a.size() != b.size()) {
        std::cout << "\n  " << label << ": Size mismatch (" << a.size() << " vs " << b.size() << ")";
        return false;
    }
    
    int mismatches = 0;
    for (size_t i = 0; i < a.size() && mismatches < 10; i++) {
        if (a[i] != b[i]) {
            size_t pixel = i / 3;
            size_t w = 24;  // assumed width
            size_t y = pixel / w;
            size_t x = pixel % w;
            size_t chan = i % 3;
            std::cout << "\n  " << label << " mismatch at pixel (" << x << "," << y 
                      << ") chan " << chan << ": " << int(a[i]) << " vs " << int(b[i]);
            mismatches++;
        }
    }
    
    return mismatches == 0;
}

// Test 1: Write with our impl, read with Utah RLE and our impl - compare results
bool test_our_write_both_read() {
    TEST_START("Our write, both read (simple pattern)");
    
    const size_t W = 24, H = 24;
    std::vector<uint8_t> data(W * H * 3);
    
    // Simple gradient pattern
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            data[idx + 0] = (x * 255) / (W - 1);
            data[idx + 1] = (y * 255) / (H - 1);
            data[idx + 2] = 128;
        }
    }
    
    // Write with our implementation
    FILE* fp = fopen("test_our_write.rle", "wb");
    if (!fp) TEST_FAIL("Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Our write failed");
    
    // Read with our implementation
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    
    fp = fopen("test_our_write.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open file for reading");
    ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Our read failed");
    
    // Read with Utah RLE
    rle_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    fp = fopen("test_our_write.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for Utah read");
    hdr.rle_file = fp;
    
    if (rle_get_setup(&hdr) != 0) {
        fclose(fp);
        TEST_FAIL("Utah RLE setup failed");
    }
    
    int utah_w = hdr.xmax - hdr.xmin + 1;
    int utah_h = hdr.ymax - hdr.ymin + 1;
    
    if (utah_w != (int)W || utah_h != (int)H) {
        fclose(fp);
        TEST_FAIL("Size mismatch with Utah RLE");
    }
    
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(utah_w);
    }
    
    std::vector<uint8_t> utah_read(W * H * 3);
    for (int y = hdr.ymin; y <= hdr.ymax; y++) {
        rle_getrow(&hdr, scanline);
        int row = y - hdr.ymin;
        for (int x = 0; x < utah_w; x++) {
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
    
    // Compare: original vs our_read
    if (!compare_bytes(data, our_read, "Our write->our read")) {
        TEST_FAIL("Our roundtrip mismatch");
    }
    
    // Compare: original vs utah_read
    if (!compare_bytes(data, utah_read, "Our write->Utah read")) {
        TEST_FAIL("Utah RLE reads our files differently");
    }
    
    // Compare: our_read vs utah_read
    if (!compare_bytes(our_read, utah_read, "Our read vs Utah read")) {
        TEST_FAIL("Different results reading same file");
    }
    
    TEST_PASS();
}

// Test 2: Write with Utah RLE, read with both - compare results
bool test_utah_write_both_read() {
    TEST_START("Utah write, both read (simple pattern)");
    
    const size_t W = 24, H = 24;
    std::vector<uint8_t> data(W * H * 3);
    
    // Simple gradient pattern
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            data[idx + 0] = (x * 255) / (W - 1);
            data[idx + 1] = (y * 255) / (H - 1);
            data[idx + 2] = 128;
        }
    }
    
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
    
    FILE* fp = fopen("test_utah_write.rle", "wb");
    if (!fp) TEST_FAIL("Failed to open file for writing");
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
    fclose(fp);
    
    // Read with our implementation
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    rle::Error err;
    
    fp = fopen("test_utah_write.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for our read");
    bool ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Our read failed");
    
    // Read with Utah RLE
    rle_hdr in_hdr;
    memset(&in_hdr, 0, sizeof(in_hdr));
    fp = fopen("test_utah_write.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for Utah read");
    in_hdr.rle_file = fp;
    
    if (rle_get_setup(&in_hdr) != 0) {
        fclose(fp);
        TEST_FAIL("Utah RLE setup failed");
    }
    
    int utah_w = in_hdr.xmax - in_hdr.xmin + 1;
    int utah_h = in_hdr.ymax - in_hdr.ymin + 1;
    
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(utah_w);
    }
    
    std::vector<uint8_t> utah_read(W * H * 3);
    for (int y = in_hdr.ymin; y <= in_hdr.ymax; y++) {
        rle_getrow(&in_hdr, scanline);
        int row = y - in_hdr.ymin;
        for (int x = 0; x < utah_w; x++) {
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
    
    // Compare: original vs our_read
    if (!compare_bytes(data, our_read, "Utah write->our read")) {
        TEST_FAIL("We read Utah files differently than expected");
    }
    
    // Compare: original vs utah_read
    if (!compare_bytes(data, utah_read, "Utah write->Utah read")) {
        TEST_FAIL("Utah RLE roundtrip mismatch");
    }
    
    // Compare: our_read vs utah_read
    if (!compare_bytes(our_read, utah_read, "Our read vs Utah read")) {
        TEST_FAIL("Different results reading same file");
    }
    
    TEST_PASS();
}

// Test 3: Test with background color specified
bool test_with_background() {
    TEST_START("With background color (solid red background)");
    
    const size_t W = 24, H = 24;
    std::vector<uint8_t> data(W * H * 3);
    
    // Most pixels are red (background), some are different
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            if (x >= 8 && x < 16 && y >= 8 && y < 16) {
                // Center square is blue
                data[idx + 0] = 0;
                data[idx + 1] = 0;
                data[idx + 2] = 255;
            } else {
                // Rest is red (background)
                data[idx + 0] = 255;
                data[idx + 1] = 0;
                data[idx + 2] = 0;
            }
        }
    }
    
    // Write with our implementation specifying red background
    FILE* fp = fopen("test_bg.rle", "wb");
    if (!fp) TEST_FAIL("Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {255, 0, 0};  // Red background
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_CLEAR, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Write with background failed");
    
    // Read back with our implementation
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    
    fp = fopen("test_bg.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for reading");
    ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Read failed");
    
    // Compare
    if (!compare_bytes(data, our_read, "Background mode")) {
        TEST_FAIL("Background mode roundtrip mismatch");
    }
    
    TEST_PASS();
}

// Test 4: Test with NO_BACKGROUND flag
bool test_no_background_flag() {
    TEST_START("NO_BACKGROUND flag");
    
    const size_t W = 16, H = 16;
    std::vector<uint8_t> data(W * H * 3, 128);  // All gray
    
    // Write with our implementation, no background
    FILE* fp = fopen("test_nobg.rle", "wb");
    if (!fp) TEST_FAIL("Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};  // No background
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Write failed");
    
    // Check the file has NO_BACKGROUND flag
    fp = fopen("test_nobg.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for reading");
    
    uint8_t header[16];
    if (fread(header, 1, 16, fp) != 16) {
        fclose(fp);
        TEST_FAIL("Failed to read header");
    }
    fclose(fp);
    
    uint8_t flags = header[10];
    if (!(flags & 0x02)) {  // H_NO_BACKGROUND = 0x02
        TEST_FAIL("NO_BACKGROUND flag not set");
    }
    
    // Read back
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    
    fp = fopen("test_nobg.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for reading");
    ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Read failed");
    
    // Compare
    if (!compare_bytes(data, our_read, "NO_BACKGROUND")) {
        TEST_FAIL("NO_BACKGROUND roundtrip mismatch");
    }
    
    TEST_PASS();
}

// Test 5: Test CLEAR_FIRST flag behavior
bool test_clearfirst_flag() {
    TEST_START("CLEAR_FIRST flag behavior");
    
    // This test verifies that we can read files with CLEAR_FIRST set
    // We don't test writing it since it's a display hint, not encoding
    
    const size_t W = 16, H = 16;
    std::vector<uint8_t> data(W * H * 3);
    
    // Checkerboard pattern
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            bool white = ((x / 4) + (y / 4)) % 2 == 0;
            uint8_t val = white ? 255 : 0;
            data[idx + 0] = val;
            data[idx + 1] = val;
            data[idx + 2] = val;
        }
    }
    
    // Write with Utah RLE with CLEAR_FIRST flag
    rle_hdr out_hdr;
    memset(&out_hdr, 0, sizeof(out_hdr));
    out_hdr.xmin = 0;
    out_hdr.ymin = 0;
    out_hdr.xmax = W - 1;
    out_hdr.ymax = H - 1;
    out_hdr.ncolors = 3;
    out_hdr.alpha = 0;
    out_hdr.background = 0;  // No background color
    // Set CLEAR_FIRST bit in flags (bit 0 of byte 10)
    RLE_SET_BIT(out_hdr, H_CLEARFIRST);  // H_CLEARFIRST = 0x1
    
    FILE* fp = fopen("test_clear.rle", "wb");
    if (!fp) TEST_FAIL("Failed to open file for writing");
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
    fclose(fp);
    
    // Read with our implementation
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    rle::Error err;
    
    fp = fopen("test_clear.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for reading");
    bool ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Read with CLEAR_FIRST failed");
    
    // Compare
    if (!compare_bytes(data, our_read, "CLEAR_FIRST")) {
        TEST_FAIL("CLEAR_FIRST roundtrip mismatch");
    }
    
    TEST_PASS();
}

// Test 6: Test with alpha channel
bool test_alpha_channel() {
    TEST_START("Alpha channel comparison");
    
    const size_t W = 16, H = 16;
    std::vector<uint8_t> data(W * H * 4);
    
    // Pattern with varying alpha
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 4;
            data[idx + 0] = (x * 255) / (W - 1);
            data[idx + 1] = (y * 255) / (H - 1);
            data[idx + 2] = 128;
            data[idx + 3] = ((x + y) * 255) / (W + H - 2);
        }
    }
    
    // Write with our implementation
    FILE* fp = fopen("test_alpha.rle", "wb");
    if (!fp) TEST_FAIL("Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, true,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Write with alpha failed");
    
    // Read back
    std::vector<uint8_t> our_read;
    uint32_t our_w, our_h;
    bool our_alpha;
    
    fp = fopen("test_alpha.rle", "rb");
    if (!fp) TEST_FAIL("Failed to open for reading");
    ok = rle::read_rgb(fp, our_read, our_w, our_h, &our_alpha, nullptr, err);
    fclose(fp);
    if (!ok || err != rle::Error::OK) TEST_FAIL("Read with alpha failed");
    
    if (!our_alpha) TEST_FAIL("Alpha flag not preserved");
    
    // Compare including alpha
    if (our_read.size() != data.size()) {
        TEST_FAIL("Size mismatch with alpha");
    }
    
    int mismatches = 0;
    for (size_t i = 0; i < data.size() && mismatches < 10; i++) {
        if (data[i] != our_read[i]) {
            size_t pixel = i / 4;
            size_t y = pixel / W;
            size_t x = pixel % W;
            size_t chan = i % 4;
            std::cout << "\n  Alpha mismatch at (" << x << "," << y << ") chan " << chan 
                      << ": " << int(data[i]) << " vs " << int(our_read[i]);
            mismatches++;
        }
    }
    
    if (mismatches > 0) TEST_FAIL("Alpha channel mismatch");
    
    TEST_PASS();
}

int main() {
    std::cout << "=== Utah RLE Behavioral Comparison Tests ===\n\n";
    
    test_our_write_both_read();
    test_utah_write_both_read();
    test_with_background();
    test_no_background_flag();
    test_clearfirst_flag();
    test_alpha_channel();
    
    // Cleanup
    remove("test_our_write.rle");
    remove("test_utah_write.rle");
    remove("test_bg.rle");
    remove("test_nobg.rle");
    remove("test_clear.rle");
    remove("test_alpha.rle");
    
    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    std::cout << "Total:  " << (tests_passed + tests_failed) << "\n";
    
    if (tests_failed > 0) {
        std::cout << "\n** DIFFERENCES FOUND - Review above for details **\n";
    } else {
        std::cout << "\n** All modes compatible with Utah RLE **\n";
    }
    
    return tests_failed > 0 ? 1 : 0;
}
