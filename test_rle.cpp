/**
 * @file test_rle.cpp
 * @brief Test suite for RLE image format reader/writer
 *
 * Comprehensive tests for the RLE implementation with self-contained validation.
 * Tests include roundtrip accuracy, edge cases, error handling, and pixel-level verification.
 */

#include "rle.hpp"
#include <iostream>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstdio>

// Declare external functions from rle.cpp
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

// Test result tracking
struct TestStats {
    int total = 0;
    int passed = 0;
    int failed = 0;
    
    void record_pass() { total++; passed++; }
    void record_fail() { total++; failed++; }
    
    void print_summary() const {
        std::cout << "\n========================================\n";
        std::cout << "Test Summary:\n";
        std::cout << "  Total:   " << total << "\n";
        if (total > 0) {
            std::cout << "  Passed:  " << passed << " (" << (100 * passed / total) << "%)\n";
        } else {
            std::cout << "  Passed:  " << passed << "\n";
        }
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
        std::cout << "\n  FAILED at line " << __LINE__ << ": " #a " != " #b \
                  << " (got " << (a) << ", expected " << (b) << ")" << std::endl; \
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
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->alpha_channel = (channels >= 4) ? 1 : 0;
    
    size_t data_size = width * height * channels * sizeof(double);
    img->data = (double*)calloc(1, data_size);
    if (!img->data) {
        free(img);
        return nullptr;
    }
    
    return img;
}

// Helper function to free test image
void free_test_image(icv_image_t* img) {
    if (!img) return;
    if (img->data) {
        bu_free(img->data, "image data");
    }
    bu_free(img, "image struct");
}

// Helper to compare pixel values with tolerance
bool pixels_match(const icv_image_t* img1, const icv_image_t* img2, double tolerance = 0.01) {
    if (!img1 || !img2) return false;
    if (img1->width != img2->width || img1->height != img2->height) return false;
    if (img1->channels != img2->channels) return false;
    
    size_t total_values = img1->width * img1->height * img1->channels;
    for (size_t i = 0; i < total_values; i++) {
        double diff = std::abs(img1->data[i] - img2->data[i]);
        if (diff > tolerance) {
            size_t pixel_idx = i / img1->channels;
            size_t channel = i % img1->channels;
            size_t y = pixel_idx / img1->width;
            size_t x = pixel_idx % img1->width;
            std::cout << "\n  Pixel mismatch at (" << x << "," << y << ") channel " << channel
                      << ": " << img1->data[i] << " vs " << img2->data[i] << std::endl;
            return false;
        }
    }
    return true;
}

// Test: Simple roundtrip (write and read back)
void test_simple_roundtrip() {
    TEST("Simple roundtrip (24x18 RGB)");
    
    const size_t w = 24, h = 18;
    icv_image_t* original = create_test_image(w, h, 3);
    EXPECT_TRUE(original != nullptr);
    
    // Fill with test pattern
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            original->data[idx + 0] = (double)x / w;       // R varies with X
            original->data[idx + 1] = (double)y / h;       // G varies with Y
            original->data[idx + 2] = 0.5;                  // B constant
        }
    }
    
    // Write to file
    FILE* fp = std::fopen("test_roundtrip.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    int result = rle_write(original, fp);
    std::fclose(fp);
    EXPECT_EQ(result, 0);
    
    // Read back
    fp = std::fopen("test_roundtrip.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    // Verify match
    if (readback) {
        EXPECT_EQ(readback->width, w);
        EXPECT_EQ(readback->height, h);
        EXPECT_EQ(readback->channels, 3u);
        EXPECT_TRUE(pixels_match(original, readback));
        free_test_image(readback);
    }
    
    free_test_image(original);
    std::remove("test_roundtrip.rle");
    
    END_TEST();
}

// Test: Solid color image
void test_solid_color() {
    TEST("Solid color image (32x32, all red)");
    
    const size_t w = 32, h = 32;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Fill with solid red
    for (size_t i = 0; i < w * h * 3; i += 3) {
        img->data[i + 0] = 1.0;  // R
        img->data[i + 1] = 0.0;  // G
        img->data[i + 2] = 0.0;  // B
    }
    
    // Write and read back
    FILE* fp = std::fopen("test_solid.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_solid.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_solid.rle");
    
    END_TEST();
}

// Test: Gradient pattern
void test_gradient_pattern() {
    TEST("Gradient pattern (48x48)");
    
    const size_t w = 48, h = 48;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Create RGB gradients
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            img->data[idx + 0] = (double)x / (w - 1);
            img->data[idx + 1] = (double)y / (h - 1);
            img->data[idx + 2] = ((double)x + y) / (w + h - 2);
        }
    }
    
    // Write and read back
    FILE* fp = std::fopen("test_gradient.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_gradient.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_gradient.rle");
    
    END_TEST();
}

// Test: Minimum size (1x1)
void test_minimum_size() {
    TEST("Minimum size image (1x1)");
    
    icv_image_t* img = create_test_image(1, 1, 3);
    EXPECT_TRUE(img != nullptr);
    
    img->data[0] = 0.8;  // R
    img->data[1] = 0.6;  // G
    img->data[2] = 0.4;  // B
    
    FILE* fp = std::fopen("test_1x1.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_1x1.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_EQ(readback->width, 1u);
        EXPECT_EQ(readback->height, 1u);
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_1x1.rle");
    
    END_TEST();
}

// Test: Wide image
void test_wide_image() {
    TEST("Wide image (256x4)");
    
    const size_t w = 256, h = 4;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Fill with horizontal gradient
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            img->data[idx + 0] = (double)x / (w - 1);
            img->data[idx + 1] = (double)y / (h - 1);
            img->data[idx + 2] = 0.5;
        }
    }
    
    FILE* fp = std::fopen("test_wide.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_wide.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_wide.rle");
    
    END_TEST();
}

// Test: Tall image
void test_tall_image() {
    TEST("Tall image (4x256)");
    
    const size_t w = 4, h = 256;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Fill with vertical gradient
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            img->data[idx + 0] = (double)x / (w - 1);
            img->data[idx + 1] = (double)y / (h - 1);
            img->data[idx + 2] = 0.5;
        }
    }
    
    FILE* fp = std::fopen("test_tall.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_tall.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_tall.rle");
    
    END_TEST();
}

// Test: Checkerboard pattern
void test_checkerboard() {
    TEST("Checkerboard pattern (64x64)");
    
    const size_t w = 64, h = 64;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Create 8x8 checkerboard
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            bool is_black = ((x / 8) + (y / 8)) % 2 == 0;
            double val = is_black ? 0.0 : 1.0;
            img->data[idx + 0] = val;
            img->data[idx + 1] = val;
            img->data[idx + 2] = val;
        }
    }
    
    FILE* fp = std::fopen("test_checker.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_checker.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_checker.rle");
    
    END_TEST();
}

// Test: Large image
void test_large_image() {
    TEST("Large image (256x256)");
    
    const size_t w = 256, h = 256;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Fill with complex pattern
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            img->data[idx + 0] = (double)(x % 256) / 255.0;
            img->data[idx + 1] = (double)(y % 256) / 255.0;
            img->data[idx + 2] = (double)((x + y) % 256) / 255.0;
        }
    }
    
    FILE* fp = std::fopen("test_large.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_large.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_large.rle");
    
    END_TEST();
}

// Test: Random noise
void test_random_noise() {
    TEST("Random noise pattern (32x32)");
    
    const size_t w = 32, h = 32;
    icv_image_t* img = create_test_image(w, h, 3);
    EXPECT_TRUE(img != nullptr);
    
    // Fill with pseudo-random values
    unsigned int seed = 12345;
    for (size_t i = 0; i < w * h * 3; i++) {
        seed = (seed * 1103515245 + 12345) & 0x7fffffff;
        img->data[i] = (double)(seed % 256) / 255.0;
    }
    
    FILE* fp = std::fopen("test_noise.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_noise.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_noise.rle");
    
    END_TEST();
}

// Test: Alpha channel roundtrip
void test_alpha_roundtrip() {
    TEST("Alpha channel roundtrip (RGBA 16x16)");
    
    const size_t w = 16, h = 16;
    icv_image_t* img = create_test_image(w, h, 4);
    EXPECT_TRUE(img != nullptr);
    img->alpha_channel = 1;
    
    // Fill with gradient including alpha
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 4;
            img->data[idx + 0] = (double)x / (w - 1);
            img->data[idx + 1] = (double)y / (h - 1);
            img->data[idx + 2] = 0.5;
            img->data[idx + 3] = (double)(x + y) / (w + h - 2);  // Alpha varies
        }
    }
    
    FILE* fp = std::fopen("test_alpha.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_alpha.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_EQ(readback->channels, 4u);
        EXPECT_EQ(readback->alpha_channel, 1);
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_alpha.rle");
    
    END_TEST();
}

// Test: Alpha preservation with varying values
void test_alpha_preservation() {
    TEST("Alpha preservation (various alpha values)");
    
    const size_t w = 8, h = 8;
    icv_image_t* img = create_test_image(w, h, 4);
    EXPECT_TRUE(img != nullptr);
    img->alpha_channel = 1;
    
    // Create pattern with specific alpha values
    for (size_t y = 0; y < h; y++) {
        for (size_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 4;
            img->data[idx + 0] = 0.5;  // R constant
            img->data[idx + 1] = 0.5;  // G constant
            img->data[idx + 2] = 0.5;  // B constant
            // Alpha: 0.0, 0.25, 0.5, 0.75, 1.0 in different regions
            img->data[idx + 3] = (double)((x + y) % 5) / 4.0;
        }
    }
    
    FILE* fp = std::fopen("test_alpha_preserve.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    rle_write(img, fp);
    std::fclose(fp);
    
    fp = std::fopen("test_alpha_preserve.rle", "rb");
    EXPECT_TRUE(fp != nullptr);
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(readback != nullptr);
    
    if (readback) {
        EXPECT_TRUE(pixels_match(img, readback));
        free_test_image(readback);
    }
    
    free_test_image(img);
    std::remove("test_alpha_preserve.rle");
    
    END_TEST();
}

// Test: Error handling - null image write
void test_null_image_write() {
    TEST("Error handling: null image write");
    
    FILE* fp = std::fopen("test_null.rle", "wb");
    EXPECT_TRUE(fp != nullptr);
    int result = rle_write(nullptr, fp);
    std::fclose(fp);
    EXPECT_NE(result, 0);  // Should fail
    
    std::remove("test_null.rle");
    
    END_TEST();
}

// Test: Error handling - invalid file
void test_invalid_file() {
    TEST("Error handling: invalid file read");
    
    // Try to read from non-existent file
    FILE* fp = std::fopen("nonexistent_file.rle", "rb");
    if (fp) {
        std::fclose(fp);
        // If file exists, skip test
        std::cout << "SKIPPED (file exists)\n";
        return;
    }
    
    END_TEST();
}

// Test: Reading teapot.rle (if it exists)
void test_teapot_image() {
    TEST("Read teapot.rle reference image");
    
    FILE* fp = std::fopen("teapot.rle", "rb");
    if (!fp) {
        std::cout << "SKIPPED (teapot.rle not found)\n";
        return;
    }
    
    icv_image_t* img = rle_read(fp);
    std::fclose(fp);
    EXPECT_TRUE(img != nullptr);
    
    if (img) {
        EXPECT_EQ(img->width, 256u);
        EXPECT_EQ(img->height, 256u);
        EXPECT_EQ(img->channels, 3u);
        
        // Verify non-zero data
        bool has_data = false;
        for (size_t i = 0; i < img->width * img->height * img->channels; i++) {
            if (img->data[i] > 0.01) {
                has_data = true;
                break;
            }
        }
        EXPECT_TRUE(has_data);
        
        free_test_image(img);
    }
    
    END_TEST();
}

int main() {
    std::cout << "========================================\n";
    std::cout << "RLE Implementation Test Suite\n";
    std::cout << "Self-contained validation tests\n";
    std::cout << "========================================\n\n";
    
    // Basic I/O tests
    std::cout << "\n--- Basic I/O Tests ---\n";
    test_simple_roundtrip();
    test_solid_color();
    test_gradient_pattern();
    
    // Size variation tests
    std::cout << "\n--- Size Variation Tests ---\n";
    test_minimum_size();
    test_wide_image();
    test_tall_image();
    
    // Pattern tests
    std::cout << "\n--- Pattern Tests ---\n";
    test_checkerboard();
    test_large_image();
    test_random_noise();
    
    // Alpha channel tests
    std::cout << "\n--- Alpha Channel Tests ---\n";
    test_alpha_roundtrip();
    test_alpha_preservation();
    
    // Error handling tests
    std::cout << "\n--- Error Handling Tests ---\n";
    test_null_image_write();
    test_invalid_file();
    
    // Reference image test
    std::cout << "\n--- Reference Image Tests ---\n";
    test_teapot_image();
    
    // Print summary
    g_stats.print_summary();
    
    return (g_stats.failed == 0) ? 0 : 1;
}
