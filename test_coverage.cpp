/**
 * @file test_coverage.cpp
 * @brief Extended test suite for RLE implementation to achieve comprehensive code coverage
 *
 * This test suite is specifically designed to exercise uncovered code paths,
 * edge cases, and error conditions to increase code coverage and validate
 * robustness of the RLE implementation.
 */

#include "rle.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <vector>
#include <algorithm>

// Include utahrle headers for compatibility validation
extern "C" {
#include "rle.h"
#include "rle_put.h"
}

// Forward declarations for rle.cpp API functions
// Note: These should ideally be in a proper header but are duplicated here for testing
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

// Helper function to create test file paths
inline std::string test_file_path(const char* filename) {
    return std::string(filename);
}

// Test result tracking
struct TestStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    
    void record_pass() { total++; passed++; }
    void record_fail() { total++; failed++; }
    
    void print_summary() const {
        std::cout << "\n========================================\n";
        std::cout << "Coverage Test Summary:\n";
        std::cout << "  Total:   " << total << "\n";
        std::cout << "  Passed:  " << passed << "\n";
        std::cout << "  Failed:  " << failed << "\n";
        std::cout << "========================================\n";
    }
};

TestStats g_stats;

// Helper macros for testing
#define TEST(name) \
    std::cout << "TEST: " << name << " ... "; \
    bool test_passed = true;

#define EXPECT_TRUE(condition) \
    if (!(condition)) { \
        std::cout << "\n  FAILED at line " << __LINE__ << ": " #condition << std::endl; \
        test_passed = false; \
    }

#define EXPECT_FALSE(condition) \
    if ((condition)) { \
        std::cout << "\n  FAILED at line " << __LINE__ << ": !(" #condition ")" << std::endl; \
        test_passed = false; \
    }

#define EXPECT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cout << "\n  FAILED at line " << __LINE__ << ": " #a " != " #b << std::endl; \
        test_passed = false; \
    }

#define EXPECT_NE(a, b) \
    if ((a) == (b)) { \
        std::cout << "\n  FAILED at line " << __LINE__ << ": " #a " == " #b << std::endl; \
        test_passed = false; \
    }

#define END_TEST() \
    if (test_passed) { \
        std::cout << "PASSED\n"; \
        g_stats.record_pass(); \
    } else { \
        g_stats.record_fail(); \
    }

// Helper function to create test image
icv_image_t* create_test_image(size_t width, size_t height, size_t channels = 3) {
    icv_image_t *img = (icv_image_t*)calloc(1, sizeof(icv_image_t));
    if (!img) return nullptr;
    
    img->magic = 0x6269666d;  // ICV_IMAGE_MAGIC
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->alpha_channel = (channels > 3) ? 1 : 0;
    img->color_space = ICV_COLOR_SPACE_RGB;
    img->gamma_corr = 0.0;
    
    size_t total_pixels = width * height * channels;
    img->data = (double*)calloc(total_pixels, sizeof(double));
    if (!img->data) {
        free(img);
        return nullptr;
    }
    
    // Fill with default test pattern
    for (size_t i = 0; i < total_pixels; i++) {
        img->data[i] = double(i % 256) / 255.0;
    }
    
    return img;
}

void free_test_image(icv_image_t* img) {
    if (img) {
        if (img->data) {
            bu_free(img->data, "test image data");
        }
        bu_free(img, "test image");
    }
}

// =============================================================================
// Error String Coverage Tests
// =============================================================================

void test_all_error_strings() {
    TEST("All error string coverage");
    
    // Test all error codes to ensure they have string representations
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::OK), "OK") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::BAD_MAGIC), "Bad magic") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::HEADER_TRUNCATED), "Header truncated") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::UNSUPPORTED_ENDIAN), "Unsupported endian") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::DIM_TOO_LARGE), "Dimensions exceed max") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::PIXELS_TOO_LARGE), "Pixel count exceeds max") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::ALLOC_TOO_LARGE), "Allocation exceeds cap") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::COLORMAP_TOO_LARGE), "Colormap exceeds cap") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::COMMENT_TOO_LARGE), "Comment block too large") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::INVALID_NCOLORS), "Invalid ncolors") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::INVALID_PIXELBITS), "Invalid pixelbits") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::INVALID_BG_BLOCK), "Invalid background block") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::OPCODE_OVERFLOW), "Opcode operand overflow") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::OPCODE_UNKNOWN), "Unknown opcode") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::TRUNCATED_OPCODE), "Truncated opcode data") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::OP_COUNT_EXCEEDED), "Opcode count per row exceeded") == 0);
    EXPECT_TRUE(std::strcmp(rle::error_string(rle::Error::INTERNAL_ERROR), "Internal error") == 0);
    
    END_TEST();
}

// =============================================================================
// Header Validation Coverage Tests
// =============================================================================

void test_invalid_header_dimensions() {
    TEST("Invalid header dimensions");
    
    rle::Header h;
    h.xlen = 0;  // Invalid
    h.ylen = 100;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::DIM_TOO_LARGE);
    
    // Test too large dimensions
    h.xlen = rle::MAX_DIM + 1;  // Exceeds MAX_DIM
    h.ylen = 100;
    valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::DIM_TOO_LARGE);
    
    END_TEST();
}

void test_invalid_pixelbits() {
    TEST("Invalid pixelbits");
    
    rle::Header h;
    h.xlen = 100;
    h.ylen = 100;
    h.ncolors = 3;
    h.pixelbits = 16;  // Invalid - only 8 is supported
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::INVALID_PIXELBITS);
    
    END_TEST();
}

void test_invalid_ncolors() {
    TEST("Invalid ncolors");
    
    rle::Header h;
    h.xlen = 100;
    h.ylen = 100;
    h.ncolors = 0;  // Invalid
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::INVALID_NCOLORS);
    
    // Test ncolors > 254
    h.ncolors = 255;  // Invalid
    valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::INVALID_NCOLORS);
    
    END_TEST();
}

void test_invalid_background() {
    TEST("Invalid background block");
    
    rle::Header h;
    h.xlen = 100;
    h.ylen = 100;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = 0;  // No NO_BACKGROUND flag
    h.background = {128, 128};  // Wrong size - should be 3
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::INVALID_BG_BLOCK);
    
    END_TEST();
}

void test_too_large_pixels() {
    TEST("Too large pixel count");
    
    rle::Header h;
    h.xlen = rle::MAX_DIM + 1;  // Exceeds MAX_DIM
    h.ylen = rle::MAX_DIM + 1;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_FALSE(valid);
    // Will fail DIM_TOO_LARGE check before PIXELS_TOO_LARGE
    EXPECT_EQ(err, rle::Error::DIM_TOO_LARGE);
    
    END_TEST();
}

// =============================================================================
// Error Path Coverage in rle_write
// =============================================================================

void test_write_invalid_channels() {
    TEST("Write with invalid channel count");
    
    icv_image_t* img = create_test_image(10, 10, 1);  // Only 1 channel - invalid for RLE
    EXPECT_TRUE(img != nullptr);
    
    if (img) {
        FILE* fp = std::fopen(test_file_path("test_invalid_channels.rle").c_str(), "wb");
        EXPECT_TRUE(fp != nullptr);
        
        int result = rle_write(img, fp);
        std::fclose(fp);
        EXPECT_NE(result, 0);  // Should fail
        
        free_test_image(img);
    }
    
    END_TEST();
}

void test_write_oversized_dimensions() {
    TEST("Write with oversized dimensions");
    
    icv_image_t* img = create_test_image(10, 10, 3);
    EXPECT_TRUE(img != nullptr);
    
    if (img) {
        // Manually set dimensions to exceed maximum
        img->width = rle::MAX_DIM + 1;
        img->height = rle::MAX_DIM + 1;
        
        FILE* fp = std::fopen(test_file_path("test_oversized.rle").c_str(), "wb");
        EXPECT_TRUE(fp != nullptr);
        
        int result = rle_write(img, fp);
        std::fclose(fp);
        EXPECT_NE(result, 0);  // Should fail
        
        // Restore dimensions before freeing
        img->width = 10;
        img->height = 10;
        free_test_image(img);
    }
    
    END_TEST();
}

// =============================================================================
// Error Path Coverage in rle_read
// =============================================================================

void test_read_null_pointer() {
    TEST("Read with null file pointer");
    
    icv_image_t* img = rle_read(nullptr);
    EXPECT_TRUE(img == nullptr);  // Should fail gracefully
    
    END_TEST();
}

void test_read_truncated_header() {
    TEST("Read truncated header");
    
    // Write a file with incomplete header
    FILE* fp = std::fopen(test_file_path("test_truncated.rle").c_str(), "wb");
    EXPECT_TRUE(fp != nullptr);
    
    // Write magic number but nothing else
    // RLE_MAGIC = 0xCC52, stored in little endian format
    uint8_t data[] = {
        uint8_t(rle::RLE_MAGIC & 0xFF),         // 0x52
        uint8_t((rle::RLE_MAGIC >> 8) & 0xFF)   // 0xCC
    };
    std::fwrite(data, 1, sizeof(data), fp);
    std::fclose(fp);
    
    // Try to read it
    fp = std::fopen(test_file_path("test_truncated.rle").c_str(), "rb");
    EXPECT_TRUE(fp != nullptr);
    
    icv_image_t* img = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(img == nullptr);  // Should fail
    
    std::remove(test_file_path("test_truncated.rle").c_str());
    
    END_TEST();
}

// =============================================================================
// Format Feature Coverage Tests
// =============================================================================

void test_comments_flag() {
    TEST("RLE with comments flag");
    
    // Create a file with comments using raw rle.hpp API
    FILE* fp = std::fopen(test_file_path("test_comments.rle").c_str(), "wb");
    EXPECT_TRUE(fp != nullptr);
    
    rle::Header h;
    h.xlen = 10;
    h.ylen = 10;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND | rle::FLAG_COMMENT;
    h.comments = {"Test comment", "Another comment"};
    
    bool write_ok = rle::write_header(fp, h);
    EXPECT_TRUE(write_ok);
    
    // Write minimal pixel data
    for (int y = 0; y < 10; y++) {
        // Skip to next scanline
        uint8_t skip = 0x40;  // EOF opcode
        std::fwrite(&skip, 1, 1, fp);
    }
    
    std::fclose(fp);
    
    // Now read it back using rle_read
    fp = std::fopen(test_file_path("test_comments.rle").c_str(), "rb");
    EXPECT_TRUE(fp != nullptr);
    
    icv_image_t* img = rle_read(fp);
    std::fclose(fp);
    
    if (img) {
        EXPECT_EQ(img->width, 10u);
        EXPECT_EQ(img->height, 10u);
        free_test_image(img);
    }
    
    std::remove(test_file_path("test_comments.rle").c_str());
    
    END_TEST();
}

void test_alpha_channel() {
    TEST("RLE with alpha channel");
    
    // Create a raw RLE file with alpha channel using rle.hpp API directly
    FILE* fp = std::fopen(test_file_path("test_alpha.rle").c_str(), "wb");
    EXPECT_TRUE(fp != nullptr);
    
    rle::Header h;
    h.xlen = 10;
    h.ylen = 10;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND | rle::FLAG_ALPHA;  // Set alpha flag
    
    bool write_ok = rle::write_header(fp, h);
    EXPECT_TRUE(write_ok);
    
    // Write minimal pixel data (all black with full alpha)
    for (int y = 0; y < 10; y++) {
        // EOF opcode
        uint8_t eof = 0x40;
        std::fwrite(&eof, 1, 1, fp);
    }
    
    std::fclose(fp);
    
    // Read back using rle_read
    fp = std::fopen(test_file_path("test_alpha.rle").c_str(), "rb");
    EXPECT_TRUE(fp != nullptr);
    
    icv_image_t* loaded = rle_read(fp);
    std::fclose(fp);
    
    if (loaded) {
        EXPECT_EQ(loaded->width, 10u);
        EXPECT_EQ(loaded->height, 10u);
        // Note: rle_read implementation may convert to RGB internally
        // The important thing is that it can read alpha channel files
        EXPECT_TRUE(loaded->channels >= 3);
        free_test_image(loaded);
    }
    
    std::remove(test_file_path("test_alpha.rle").c_str());
    
    END_TEST();
}

void test_single_color_channel() {
    TEST("RLE with single color channel (grayscale)");
    
    // Create grayscale image (ncolors = 1)
    FILE* fp = std::fopen(test_file_path("test_gray.rle").c_str(), "wb");
    EXPECT_TRUE(fp != nullptr);
    
    rle::Header h;
    h.xlen = 10;
    h.ylen = 10;
    h.ncolors = 1;  // Grayscale
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    bool write_ok = rle::write_header(fp, h);
    EXPECT_TRUE(write_ok);
    
    // Write minimal pixel data (all black)
    for (int y = 0; y < 10; y++) {
        // EOF opcode
        uint8_t eof = 0x40;
        std::fwrite(&eof, 1, 1, fp);
    }
    
    std::fclose(fp);
    
    // Read it back
    fp = std::fopen(test_file_path("test_gray.rle").c_str(), "rb");
    EXPECT_TRUE(fp != nullptr);
    
    icv_image_t* img = rle_read(fp);
    std::fclose(fp);
    
    if (img) {
        EXPECT_EQ(img->width, 10u);
        EXPECT_EQ(img->height, 10u);
        free_test_image(img);
    }
    
    std::remove(test_file_path("test_gray.rle").c_str());
    
    END_TEST();
}

void test_various_ncolors() {
    TEST("RLE with various color channel counts");
    
    // Test ncolors = 2 (uncommon but valid)
    FILE* fp = std::fopen(test_file_path("test_2colors.rle").c_str(), "wb");
    EXPECT_TRUE(fp != nullptr);
    
    rle::Header h;
    h.xlen = 8;
    h.ylen = 8;
    h.ncolors = 2;  // 2 color channels
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    bool write_ok = rle::write_header(fp, h);
    EXPECT_TRUE(write_ok);
    
    // Write minimal pixel data
    for (int y = 0; y < 8; y++) {
        uint8_t eof = 0x40;
        std::fwrite(&eof, 1, 1, fp);
    }
    
    std::fclose(fp);
    
    // Read it back
    fp = std::fopen(test_file_path("test_2colors.rle").c_str(), "rb");
    EXPECT_TRUE(fp != nullptr);
    
    icv_image_t* img = rle_read(fp);
    std::fclose(fp);
    
    if (img) {
        EXPECT_EQ(img->width, 8u);
        EXPECT_EQ(img->height, 8u);
        free_test_image(img);
    }
    
    std::remove(test_file_path("test_2colors.rle").c_str());
    
    END_TEST();
}

void test_background_modes() {
    TEST("Different background detection modes");
    
    // Create image with a dominant background color
    icv_image_t* img = create_test_image(20, 20, 3);
    EXPECT_TRUE(img != nullptr);
    
    if (img) {
        // Fill with mostly red (should be detected as background)
        for (size_t i = 0; i < img->width * img->height; i++) {
            img->data[i * 3 + 0] = 1.0;  // R
            img->data[i * 3 + 1] = 0.0;  // G
            img->data[i * 3 + 2] = 0.0;  // B
        }
        
        // Add a few different pixels
        img->data[0] = 0.0;  // Change first pixel
        img->data[1] = 1.0;
        img->data[2] = 0.0;
        
        FILE* fp = std::fopen(test_file_path("test_bg_modes.rle").c_str(), "wb");
        EXPECT_TRUE(fp != nullptr);
        
        int result = rle_write(img, fp);
        std::fclose(fp);
        EXPECT_EQ(result, 0);
        
        // Read back
        fp = std::fopen(test_file_path("test_bg_modes.rle").c_str(), "rb");
        EXPECT_TRUE(fp != nullptr);
        
        icv_image_t* loaded = rle_read(fp);
        std::fclose(fp);
        
        if (loaded) {
            EXPECT_EQ(loaded->width, 20u);
            EXPECT_EQ(loaded->height, 20u);
            free_test_image(loaded);
        }
        
        free_test_image(img);
        std::remove(test_file_path("test_bg_modes.rle").c_str());
    }
    
    END_TEST();
}

void test_clear_first_flag() {
    TEST("RLE with CLEAR_FIRST flag");
    
    FILE* fp = std::fopen(test_file_path("test_clear.rle").c_str(), "wb");
    EXPECT_TRUE(fp != nullptr);
    
    rle::Header h;
    h.xlen = 10;
    h.ylen = 10;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND | rle::FLAG_CLEAR_FIRST;
    
    bool write_ok = rle::write_header(fp, h);
    EXPECT_TRUE(write_ok);
    
    // Write minimal pixel data
    for (int y = 0; y < 10; y++) {
        uint8_t eof = 0x40;
        std::fwrite(&eof, 1, 1, fp);
    }
    
    std::fclose(fp);
    
    // Read it back
    fp = std::fopen(test_file_path("test_clear.rle").c_str(), "rb");
    EXPECT_TRUE(fp != nullptr);
    
    icv_image_t* img = rle_read(fp);
    std::fclose(fp);
    
    if (img) {
        EXPECT_EQ(img->width, 10u);
        EXPECT_EQ(img->height, 10u);
        free_test_image(img);
    }
    
    std::remove(test_file_path("test_clear.rle").c_str());
    
    END_TEST();
}

// =============================================================================
// Colormap Tests
// =============================================================================

void test_colormap_validation() {
    TEST("Colormap validation");
    
    rle::Header h;
    h.xlen = 10;
    h.ylen = 10;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    h.ncmap = 3;
    h.cmaplen = 8;
    
    // Create colormap - should have ncmap * (1 << cmaplen) entries
    size_t entries = 3 * 256;  // 3 * 2^8
    h.colormap.resize(entries, 0x8080);  // Fill with mid-gray
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_TRUE(valid);
    EXPECT_EQ(err, rle::Error::OK);
    
    // Test invalid colormap size
    h.colormap.resize(10);  // Wrong size
    valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::COLORMAP_TOO_LARGE);
    
    END_TEST();
}

void test_colormap_too_large() {
    TEST("Colormap too large");
    
    rle::Header h;
    h.xlen = 10;
    h.ylen = 10;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.flags = rle::FLAG_NO_BACKGROUND;
    h.ncmap = 4;  // Invalid - should be <= 3
    h.cmaplen = 8;
    
    rle::Error err;
    bool valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::COLORMAP_TOO_LARGE);
    
    // Test cmaplen too large
    h.ncmap = 3;
    h.cmaplen = 9;  // Invalid - should be <= 8
    valid = h.validate(err);
    EXPECT_FALSE(valid);
    EXPECT_EQ(err, rle::Error::COLORMAP_TOO_LARGE);
    
    END_TEST();
}



// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "RLE Implementation Coverage Tests\n";
    std::cout << "Extended tests for code coverage\n";
    std::cout << "========================================\n\n";
    
    // Error string coverage
    std::cout << "\n--- Error String Coverage ---\n";
    test_all_error_strings();
    
    // Header validation coverage
    std::cout << "\n--- Header Validation Coverage ---\n";
    test_invalid_header_dimensions();
    test_invalid_pixelbits();
    test_invalid_ncolors();
    test_invalid_background();
    test_too_large_pixels();
    
    // Error path coverage in rle_write
    std::cout << "\n--- Write Error Path Coverage ---\n";
    test_write_invalid_channels();
    test_write_oversized_dimensions();
    
    // Error path coverage in rle_read
    std::cout << "\n--- Read Error Path Coverage ---\n";
    test_read_null_pointer();
    test_read_truncated_header();
    
    // Format feature coverage
    std::cout << "\n--- Format Feature Coverage ---\n";
    test_comments_flag();
    test_alpha_channel();
    test_single_color_channel();
    test_various_ncolors();
    test_background_modes();
    test_clear_first_flag();
    
    // Colormap coverage
    std::cout << "\n--- Colormap Coverage ---\n";
    test_colormap_validation();
    test_colormap_too_large();
    
    // Print summary
    g_stats.print_summary();
    
    // Return failure if any tests failed
    return (g_stats.failed > 0) ? 1 : 0;
}
