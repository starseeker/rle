/*
 * test_unusual_paths.cpp - Tests for legitimate but unusual code paths
 * 
 * This test suite exercises valid but less common RLE format features:
 * - Background mode optimizations (BG_OVERLAY, BG_CLEAR)
 * - Long form opcodes (runs/skips > 255 pixels)
 * - Large uniform regions
 * - Extended skip operations
 *
 * These are not error cases but legitimate optimizations and extended
 * format features that should be tested for completeness.
 */

#include "rle.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    static void name(); \
    static void name##_wrapper() { \
        tests_run++; \
        printf("Running %s...", #name); \
        fflush(stdout); \
        name(); \
        tests_passed++; \
        printf(" PASSED\n"); \
    } \
    static void name()

// Helper: Create image with specific pattern (doesn't allocate, caller must call allocate)
static rle::Image create_image_header(uint32_t w, uint32_t h, uint8_t ncolors = 3) {
    rle::Image img;
    img.header.xpos = 0;
    img.header.ypos = 0;
    img.header.xlen = uint16_t(w);
    img.header.ylen = uint16_t(h);
    img.header.ncolors = ncolors;
    img.header.pixelbits = 8;
    img.header.ncmap = 0;
    img.header.cmaplen = 0;
    img.header.flags |= rle::FLAG_NO_BACKGROUND;  // Default to no background
    return img;
}

// Helper: Create and allocate image
static rle::Image create_image(uint32_t w, uint32_t h, uint8_t ncolors = 3) {
    rle::Image img = create_image_header(w, h, ncolors);
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate image: %s\n", rle::error_string(err));
        exit(1);
    }
    return img;
}

// Helper: Write and read back image
static bool roundtrip(const rle::Image& img, rle::Image& out, 
                      rle::Encoder::BackgroundMode bg_mode = rle::Encoder::BG_SAVE_ALL) {
    const char* filename = "test_unusual_paths.rle";
    
    // Write
    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    
    rle::Error err;
    bool write_ok = rle::Encoder::write(f, img, bg_mode, err);
    fclose(f);
    if (!write_ok) {
        fprintf(stderr, "Write failed: %s\n", rle::error_string(err));
        return false;
    }
    
    // Read
    f = fopen(filename, "rb");
    if (!f) return false;
    
    auto res = rle::Decoder::read(f, out);
    fclose(f);
    remove(filename);
    
    if (res.error != rle::Error::OK) {
        fprintf(stderr, "Read failed: %s\n", rle::error_string(res.error));
        return false;
    }
    
    return true;
}

// Helper: Compare images pixel-by-pixel
static bool images_match(const rle::Image& a, const rle::Image& b) {
    if (a.header.width() != b.header.width() || a.header.height() != b.header.height()) return false;
    if (a.header.channels() != b.header.channels()) return false;
    
    for (uint32_t y = 0; y < a.header.height(); y++) {
        for (uint32_t x = 0; x < a.header.width(); x++) {
            for (size_t c = 0; c < a.header.channels(); c++) {
                if (a.pixel(x, y)[c] != b.pixel(x, y)[c]) {
                    fprintf(stderr, "Mismatch at (%u,%u) channel %zu: %u != %u\n",
                            x, y, c, a.pixel(x, y)[c], b.pixel(x, y)[c]);
                    return false;
                }
            }
        }
    }
    return true;
}

//==============================================================================
// BACKGROUND MODE TESTS (BG_OVERLAY, BG_CLEAR)
//==============================================================================

TEST(test_bg_overlay_entire_rows) {
    // Create image with entire rows matching background color
    // This triggers the SkipLines opcode path (lines 471-480)
    rle::Image img = create_image_header(100, 50);
    
    // Set background to red
    img.header.background = {100, 150, 200};
    img.header.flags &= ~rle::FLAG_NO_BACKGROUND;  // Enable background
    
    // Allocate after setting background
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate: %s\n", rle::error_string(err));
        exit(1);
    }
    
    // ALL pixels are now initialized to background (100, 150, 200)
    // Change rows 0-9 and 20-49 to different values
    // Leave rows 10-19 as background
    for (uint32_t y = 0; y < 10; y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = 50;
            img.pixel(x, y)[1] = 75;
            img.pixel(x, y)[2] = 25;
        }
    }
    for (uint32_t y = 20; y < img.header.height(); y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = 200;
            img.pixel(x, y)[1] = 100;
            img.pixel(x, y)[2] = 50;
        }
    }
    // Rows 10-19 remain at background (100, 150, 200)
    
    // Test with BG_OVERLAY mode
    rle::Image out;
    if (!roundtrip(img, out, rle::Encoder::BG_OVERLAY)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

TEST(test_bg_overlay_partial_rows) {
    // Create image with background pixels within rows (not entire rows)
    // This triggers SkipPixels opcode path (lines 492-506)
    rle::Image img = create_image_header(200, 30);
    
    // Set background to blue
    img.header.background = {0, 0, 255};
    img.header.flags &= ~rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate: %s\n", rle::error_string(err));
        exit(1);
    }
    
    // Pattern: background pixels in middle of each row
    for (uint32_t y = 0; y < img.header.height(); y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            if (x >= 50 && x < 150) {
                // Background pixels in middle (100 pixels wide)
                img.pixel(x, y)[0] = 0;
                img.pixel(x, y)[1] = 0;
                img.pixel(x, y)[2] = 255;
            } else {
                // Non-background at edges
                img.pixel(x, y)[0] = uint8_t(x);
                img.pixel(x, y)[1] = uint8_t(y);
                img.pixel(x, y)[2] = uint8_t((x + y) % 256);
            }
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out, rle::Encoder::BG_OVERLAY)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

TEST(test_bg_clear_mode) {
    // Test BG_CLEAR mode which sets CLEAR_FIRST flag
    rle::Image img = create_image_header(80, 60);
    
    // Set background to green
    img.header.background = {0, 255, 0};
    img.header.flags &= ~rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate: %s\n", rle::error_string(err));
        exit(1);
    }
    
    // Fill some rows with background, others with patterns
    for (uint32_t y = 0; y < img.header.height(); y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            if (y < 20 || y >= 40) {
                // Background rows
                img.pixel(x, y)[0] = 0;
                img.pixel(x, y)[1] = 255;
                img.pixel(x, y)[2] = 0;
            } else {
                // Pattern rows
                img.pixel(x, y)[0] = uint8_t(x * 3);
                img.pixel(x, y)[1] = uint8_t(y * 2);
                img.pixel(x, y)[2] = uint8_t((x + y) % 256);
            }
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out, rle::Encoder::BG_CLEAR)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

//==============================================================================
// LONG FORM OPCODE TESTS (>255 pixels)
//==============================================================================

TEST(test_long_run_data) {
    // Create image with run length > 255 pixels
    // This triggers long form RUN_DATA opcode (lines 516)
    rle::Image img = create_image(512, 20);
    
    // Fill each row with uniform color (512 pixel run)
    for (uint32_t y = 0; y < img.header.height(); y++) {
        uint8_t r = uint8_t(y * 10);
        uint8_t g = uint8_t(y * 5);
        uint8_t b = uint8_t(255 - y * 10);
        
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = r;
            img.pixel(x, y)[1] = g;
            img.pixel(x, y)[2] = b;
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

TEST(test_long_skip_pixels) {
    // Create image with background pixel span > 255
    // This triggers long form SKIP_PIXELS opcode (line 500)
    rle::Image img = create_image_header(600, 15);
    
    // Set background
    img.header.background = {128, 128, 128};
    img.header.flags &= ~rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate: %s\n", rle::error_string(err));
        exit(1);
    }
    
    // Pattern: 50 non-bg, 300 bg, 50 non-bg, 200 bg pixels per row
    for (uint32_t y = 0; y < img.header.height(); y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            if (x < 50 || (x >= 350 && x < 400)) {
                // Non-background
                img.pixel(x, y)[0] = uint8_t(x % 256);
                img.pixel(x, y)[1] = uint8_t(y * 10);
                img.pixel(x, y)[2] = 200;
            } else {
                // Background (300+ pixel spans)
                img.pixel(x, y)[0] = 128;
                img.pixel(x, y)[1] = 128;
                img.pixel(x, y)[2] = 128;
            }
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out, rle::Encoder::BG_OVERLAY)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

TEST(test_long_skip_lines) {
    // Create image with > 255 consecutive background rows
    // This triggers long form SKIP_LINES opcode (line 478)
    rle::Image img = create_image_header(100, 300);
    
    // Set background to yellow
    img.header.background = {255, 255, 0};
    img.header.flags &= ~rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate: %s\n", rle::error_string(err));
        exit(1);
    }
    
    // First 10 rows: pattern
    for (uint32_t y = 0; y < 10; y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = uint8_t(x * 2);
            img.pixel(x, y)[1] = uint8_t(y * 20);
            img.pixel(x, y)[2] = 100;
        }
    }
    
    // Rows 10-269: background (260 consecutive rows)
    for (uint32_t y = 10; y < 270; y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = 255;
            img.pixel(x, y)[1] = 255;
            img.pixel(x, y)[2] = 0;
        }
    }
    
    // Last 30 rows: pattern
    for (uint32_t y = 270; y < img.header.height(); y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = uint8_t((x + y) % 256);
            img.pixel(x, y)[1] = 150;
            img.pixel(x, y)[2] = uint8_t(y);
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out, rle::Encoder::BG_OVERLAY)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

TEST(test_long_byte_data) {
    // Create image that generates BYTE_DATA with > 255 pixels
    // This triggers long form BYTE_DATA opcode (line 540)
    // Need a pattern with no runs >= 3 pixels for 256+ consecutive pixels
    rle::Image img = create_image(512, 10);
    
    // Create checkerboard pattern (no runs >= 3)
    for (uint32_t y = 0; y < img.header.height(); y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            // Alternate every 2 pixels
            uint8_t val = ((x / 2) % 2) ? 0 : 255;
            img.pixel(x, y)[0] = val;
            img.pixel(x, y)[1] = uint8_t(x % 256);
            img.pixel(x, y)[2] = uint8_t(y * 25);
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

//==============================================================================
// COMBINED TESTS
//==============================================================================

TEST(test_combined_long_and_background) {
    // Test combining long form opcodes with background optimization
    rle::Image img = create_image_header(600, 300);
    
    // Set background
    img.header.background = {64, 64, 64};
    img.header.flags &= ~rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate: %s\n", rle::error_string(err));
        exit(1);
    }
    
    // Pattern that exercises multiple paths:
    // - Long skip lines (rows 50-299 = 250 rows of background)
    // - Long runs (solid color spans > 255 pixels)
    // - Long skip pixels (background spans > 255 pixels)
    
    // First 50 rows: mixed pattern
    for (uint32_t y = 0; y < 50; y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            if (x < 100) {
                // Solid color (long run)
                img.pixel(x, y)[0] = 200;
                img.pixel(x, y)[1] = 100;
                img.pixel(x, y)[2] = 50;
            } else if (x < 400) {
                // Background (long skip)
                img.pixel(x, y)[0] = 64;
                img.pixel(x, y)[1] = 64;
                img.pixel(x, y)[2] = 64;
            } else {
                // Pattern
                img.pixel(x, y)[0] = uint8_t(x % 256);
                img.pixel(x, y)[1] = uint8_t(y);
                img.pixel(x, y)[2] = 128;
            }
        }
    }
    
    // Rows 50-299: all background (long skip lines)
    for (uint32_t y = 50; y < 300; y++) {
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = 64;
            img.pixel(x, y)[1] = 64;
            img.pixel(x, y)[2] = 64;
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out, rle::Encoder::BG_OVERLAY)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

TEST(test_rgba_with_long_runs) {
    // Test RGBA with long form opcodes
    rle::Image img = create_image(400, 20, 3);
    img.header.flags |= rle::FLAG_ALPHA;  // Enable alpha channel
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Failed to allocate RGBA image\n");
        exit(1);
    }
    
    // Fill with uniform RGBA (triggers long runs for all 4 channels)
    for (uint32_t y = 0; y < img.header.height(); y++) {
        uint8_t r = uint8_t(y * 10);
        uint8_t g = uint8_t(y * 5);
        uint8_t b = uint8_t(255 - y * 10);
        uint8_t a = uint8_t(200 - y * 5);
        
        for (uint32_t x = 0; x < img.header.width(); x++) {
            img.pixel(x, y)[0] = r;
            img.pixel(x, y)[1] = g;
            img.pixel(x, y)[2] = b;
            img.pixel(x, y)[3] = a;
        }
    }
    
    rle::Image out;
    if (!roundtrip(img, out)) {
        fprintf(stderr, "Roundtrip failed\n");
        exit(1);
    }
    
    if (!images_match(img, out)) {
        fprintf(stderr, "Images don't match\n");
        exit(1);
    }
}

//==============================================================================
// MAIN
//==============================================================================

int main() {
    printf("=== RLE Unusual Paths Test Suite ===\n");
    printf("Testing legitimate but uncommon code paths\n\n");
    
    // Background mode tests
    printf("\n--- Background Mode Optimization Tests ---\n");
    test_bg_overlay_entire_rows_wrapper();
    test_bg_overlay_partial_rows_wrapper();
    test_bg_clear_mode_wrapper();
    
    // Long form opcode tests
    printf("\n--- Long Form Opcode Tests (>255) ---\n");
    test_long_run_data_wrapper();
    test_long_skip_pixels_wrapper();
    test_long_skip_lines_wrapper();
    test_long_byte_data_wrapper();
    
    // Combined tests
    printf("\n--- Combined Feature Tests ---\n");
    test_combined_long_and_background_wrapper();
    test_rgba_with_long_runs_wrapper();
    
    printf("\n=== Results ===\n");
    printf("Tests passed: %d/%d\n", tests_passed, tests_run);
    
    if (tests_passed == tests_run) {
        printf("\n✅ All unusual path tests PASSED\n");
        return 0;
    } else {
        printf("\n❌ Some tests FAILED\n");
        return 1;
    }
}
