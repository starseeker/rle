/**
 * @file test_positional.cpp
 * @brief Comprehensive positional and feature validation tests for RLE implementation
 *
 * Tests randomized RGB patterns to ensure positional logic is correct across
 * various image sizes and feature combinations. Self-contained validation with
 * pixel-level accuracy verification.
 */

#include "rle.hpp"
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cmath>

// No external dependencies - self-contained tests

// Declare external functions from rle.cpp
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);

// Test statistics
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::cout << "  FAILED: " << msg << "\n"; \
        tests_failed++; \
        return false; \
    } \
} while(0)

#define TEST_SUCCESS() do { \
    std::cout << "  PASSED\n"; \
    tests_passed++; \
    return true; \
} while(0)

// Helper to create icv_image_t
icv_image_t* create_icv_image(size_t width, size_t height, size_t channels) {
    icv_image_t* img = (icv_image_t*)calloc(1, sizeof(icv_image_t));
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = (double*)calloc(width * height * channels, sizeof(double));
    img->alpha_channel = (channels == 4) ? 1 : 0;
    return img;
}

void free_icv_image(icv_image_t* img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

// Helper to compare images pixel by pixel
bool compare_images(const icv_image_t* img1, const icv_image_t* img2, const char* test_name) {
    if (img1->width != img2->width || img1->height != img2->height) {
        std::cout << "  " << test_name << ": Size mismatch (" 
                  << img1->width << "x" << img1->height << " vs " 
                  << img2->width << "x" << img2->height << ")\n";
        return false;
    }
    
    if (img1->channels != img2->channels) {
        std::cout << "  " << test_name << ": Channel count mismatch (" 
                  << img1->channels << " vs " << img2->channels << ")\n";
        return false;
    }
    
    size_t npixels = img1->width * img1->height;
    size_t channels = img1->channels;
    
    int mismatches = 0;
    for (size_t i = 0; i < npixels * channels && mismatches < 10; i++) {
        // Allow small rounding differences
        double diff = std::abs(img1->data[i] - img2->data[i]);
        if (diff > 0.01) {
            size_t pixel = i / channels;
            size_t chan = i % channels;
            size_t y = pixel / img1->width;
            size_t x = pixel % img1->width;
            std::cout << "  " << test_name << ": Mismatch at (" << x << "," << y 
                      << ") channel " << chan << ": " << img1->data[i] 
                      << " vs " << img2->data[i] << "\n";
            mismatches++;
        }
    }
    
    return mismatches == 0;
}

// Test 1: Randomized RGB pattern
bool test_random_rgb_pattern() {
    std::cout << "TEST: Random RGB pattern (32x32)... ";
    
    const size_t W = 32, H = 32;
    icv_image_t* original = create_icv_image(W, H, 3);
    
    // Fill with random but reproducible values
    srand(12345);
    for (size_t i = 0; i < W * H * 3; i++) {
        original->data[i] = (rand() % 256) / 255.0;
    }
    
    // Write with our implementation
    FILE* fp = fopen("test_random.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    int result = rle_write(original, fp);
    fclose(fp);
    TEST_ASSERT(result == 0, "Write failed");
    
    // Read back with our implementation
    fp = fopen("test_random.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    icv_image_t* readback = rle_read(fp);
    fclose(fp);
    TEST_ASSERT(readback != nullptr, "Read failed");
    
    // Compare pixel by pixel
    bool match = compare_images(original, readback, "Random RGB");
    
    free_icv_image(original);
    free_icv_image(readback);
    
    TEST_ASSERT(match, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 2: Randomized RGBA pattern
bool test_random_rgba_pattern() {
    std::cout << "TEST: Random RGBA pattern (32x32)... ";
    
    const size_t W = 32, H = 32;
    icv_image_t* original = create_icv_image(W, H, 4);
    
    // Fill with random but reproducible values
    srand(54321);
    for (size_t i = 0; i < W * H * 4; i++) {
        original->data[i] = (rand() % 256) / 255.0;
    }
    
    // Write with our implementation
    FILE* fp = fopen("test_random_alpha.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    int result = rle_write(original, fp);
    fclose(fp);
    TEST_ASSERT(result == 0, "Write failed");
    
    // Read back
    fp = fopen("test_random_alpha.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    icv_image_t* readback = rle_read(fp);
    fclose(fp);
    TEST_ASSERT(readback != nullptr, "Read failed");
    
    // Compare
    bool match = compare_images(original, readback, "Random RGBA");
    
    free_icv_image(original);
    free_icv_image(readback);
    
    TEST_ASSERT(match, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 3: Checkerboard pattern (tests position independence)
bool test_checkerboard_pattern() {
    std::cout << "TEST: Checkerboard pattern (64x64)... ";
    
    const size_t W = 64, H = 64;
    icv_image_t* original = create_icv_image(W, H, 3);
    
    // Create checkerboard
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            bool white = ((x / 8) + (y / 8)) % 2 == 0;
            double val = white ? 1.0 : 0.0;
            original->data[idx + 0] = val;
            original->data[idx + 1] = val;
            original->data[idx + 2] = val;
        }
    }
    
    // Write and read back
    FILE* fp = fopen("test_checkerboard.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    int result = rle_write(original, fp);
    fclose(fp);
    TEST_ASSERT(result == 0, "Write failed");
    
    fp = fopen("test_checkerboard.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    icv_image_t* readback = rle_read(fp);
    fclose(fp);
    TEST_ASSERT(readback != nullptr, "Read failed");
    
    bool match = compare_images(original, readback, "Checkerboard");
    
    free_icv_image(original);
    free_icv_image(readback);
    
    TEST_ASSERT(match, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 4: Gradient in all directions
bool test_gradient_all_directions() {
    std::cout << "TEST: X and Y gradients combined (48x48)... ";
    
    const size_t W = 48, H = 48;
    icv_image_t* original = create_icv_image(W, H, 3);
    
    // Create gradient: R varies with X, G varies with Y, B is constant
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            original->data[idx + 0] = x / (double)(W - 1);  // R: X gradient
            original->data[idx + 1] = y / (double)(H - 1);  // G: Y gradient
            original->data[idx + 2] = 0.5;                  // B: constant
        }
    }
    
    // Write and read back
    FILE* fp = fopen("test_gradient.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    int result = rle_write(original, fp);
    fclose(fp);
    TEST_ASSERT(result == 0, "Write failed");
    
    fp = fopen("test_gradient.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    icv_image_t* readback = rle_read(fp);
    fclose(fp);
    TEST_ASSERT(readback != nullptr, "Read failed");
    
    bool match = compare_images(original, readback, "Gradient");
    
    free_icv_image(original);
    free_icv_image(readback);
    
    TEST_ASSERT(match, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 5: Large random pattern with various features  
bool test_large_random_with_alpha() {
    std::cout << "TEST: Large random RGBA with all features (128x128)... ";
    
    const size_t W = 128, H = 128;
    std::vector<uint8_t> data(W * H * 4);
    
    // Fill with random pattern including alpha
    srand(99999);
    for (size_t i = 0; i < W * H * 4; i++) {
        data[i] = rand() % 256;
    }
    
    // Write with alpha enabled
    FILE* fp = fopen("test_large_alpha.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {"Test", "Large RGBA"};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, true,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Write failed");
    
    // Read back
    std::vector<uint8_t> readback;
    uint32_t rw, rh;
    bool has_alpha;
    std::vector<std::string> comments_out;
    
    fp = fopen("test_large_alpha.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    ok = rle::read_rgb(fp, readback, rw, rh, &has_alpha, &comments_out, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Read failed");
    TEST_ASSERT(rw == W && rh == H, "Size mismatch");
    TEST_ASSERT(has_alpha, "Alpha flag not preserved");
    
    // Compare byte-by-byte
    int mismatches = 0;
    for (size_t i = 0; i < W * H * 4 && mismatches < 10; i++) {
        if (data[i] != readback[i]) {
            size_t pixel = i / 4;
            size_t y = pixel / W;
            size_t x = pixel % W;
            size_t chan = i % 4;
            std::cout << "  Mismatch at (" << x << "," << y << ") chan " << chan 
                      << ": expected " << int(data[i]) << ", got " << int(readback[i]) << "\n";
            mismatches++;
        }
    }
    
    TEST_ASSERT(mismatches == 0, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 6: Per-pixel unique values (no two pixels the same)
bool test_all_unique_pixels() {
    std::cout << "TEST: All unique pixel values (64x64)... ";
    
    const size_t W = 64, H = 64;
    std::vector<uint8_t> data(W * H * 3);
    
    // Create pattern where each pixel has a unique color
    // Use pixel index to generate unique RGB combinations
    for (size_t i = 0; i < W * H; i++) {
        data[3*i + 0] = (i * 7) % 256;       // R varies
        data[3*i + 1] = (i * 13) % 256;      // G varies differently
        data[3*i + 2] = (i * 19) % 256;      // B varies differently again
    }
    
    // Write
    FILE* fp = fopen("test_unique.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Write failed");
    
    // Read back
    std::vector<uint8_t> readback;
    uint32_t rw, rh;
    bool has_alpha;
    
    fp = fopen("test_unique.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    ok = rle::read_rgb(fp, readback, rw, rh, &has_alpha, nullptr, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Read failed");
    TEST_ASSERT(rw == W && rh == H, "Size mismatch");
    
    // Compare byte-by-byte
    int mismatches = 0;
    for (size_t i = 0; i < W * H * 3 && mismatches < 10; i++) {
        if (data[i] != readback[i]) {
            size_t pixel = i / 3;
            size_t y = pixel / W;
            size_t x = pixel % W;
            size_t chan = i % 3;
            std::cout << "  Mismatch at (" << x << "," << y << ") chan " << chan 
                      << ": expected " << int(data[i]) << ", got " << int(readback[i]) << "\n";
            mismatches++;
        }
    }
    
    TEST_ASSERT(mismatches == 0, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 7: Diagonal stripe pattern (using write_rgb/read_rgb directly)
bool test_diagonal_stripes() {
    std::cout << "TEST: Diagonal stripe pattern (40x40)... ";
    
    const size_t W = 40, H = 40;
    std::vector<uint8_t> data(W * H * 3);
    
    // Create diagonal stripes
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            bool stripe = ((x + y) / 4) % 2 == 0;
            data[idx + 0] = stripe ? 255 : 51;
            data[idx + 1] = stripe ? 204 : 77;
            data[idx + 2] = stripe ? 153 : 102;
        }
    }
    
    // Write with BG_SAVE_ALL to ensure all pixels are encoded
    FILE* fp = fopen("test_diag.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Write failed");
    
    // Read back
    std::vector<uint8_t> readback;
    uint32_t rw, rh;
    bool has_alpha;
    
    fp = fopen("test_diag.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    ok = rle::read_rgb(fp, readback, rw, rh, &has_alpha, nullptr, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Read failed");
    TEST_ASSERT(rw == W && rh == H, "Size mismatch");
    
    // Compare byte-by-byte
    int mismatches = 0;
    for (size_t i = 0; i < W * H * 3 && mismatches < 10; i++) {
        if (data[i] != readback[i]) {
            size_t pixel = i / 3;
            size_t y = pixel / W;
            size_t x = pixel % W;
            size_t chan = i % 3;
            std::cout << "  Mismatch at (" << x << "," << y << ") chan " << chan 
                      << ": expected " << int(data[i]) << ", got " << int(readback[i]) << "\n";
            mismatches++;
        }
    }
    
    TEST_ASSERT(mismatches == 0, "Pixels don't match");
    TEST_SUCCESS();
}

// Test 8: Edge pixel pattern (tests boundary handling)
bool test_edge_pixels() {
    std::cout << "TEST: Edge pixel pattern (50x50)... ";
    
    const size_t W = 50, H = 50;
    std::vector<uint8_t> data(W * H * 3);
    
    // Fill interior with one color, edges with different colors
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            if (x == 0 || x == W-1 || y == 0 || y == H-1) {
                // Edge pixels - unique for each edge
                if (x == 0) {  // Left
                    data[idx + 0] = 255;
                    data[idx + 1] = 0;
                    data[idx + 2] = 0;
                } else if (x == W-1) {  // Right
                    data[idx + 0] = 0;
                    data[idx + 1] = 255;
                    data[idx + 2] = 0;
                } else if (y == 0) {  // Top
                    data[idx + 0] = 0;
                    data[idx + 1] = 0;
                    data[idx + 2] = 255;
                } else {  // Bottom
                    data[idx + 0] = 255;
                    data[idx + 1] = 255;
                    data[idx + 2] = 0;
                }
            } else {
                // Interior - gray
                data[idx + 0] = 128;
                data[idx + 1] = 128;
                data[idx + 2] = 128;
            }
        }
    }
    
    // Write with BG_SAVE_ALL
    FILE* fp = fopen("test_edges.rle", "wb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for writing");
    
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Write failed");
    
    // Read back
    std::vector<uint8_t> readback;
    uint32_t rw, rh;
    bool has_alpha;
    
    fp = fopen("test_edges.rle", "rb");
    TEST_ASSERT(fp != nullptr, "Failed to open file for reading");
    ok = rle::read_rgb(fp, readback, rw, rh, &has_alpha, nullptr, err);
    fclose(fp);
    TEST_ASSERT(ok && err == rle::Error::OK, "Read failed");
    TEST_ASSERT(rw == W && rh == H, "Size mismatch");
    
    // Compare byte-by-byte
    int mismatches = 0;
    for (size_t i = 0; i < W * H * 3 && mismatches < 10; i++) {
        if (data[i] != readback[i]) {
            size_t pixel = i / 3;
            size_t y = pixel / W;
            size_t x = pixel % W;
            size_t chan = i % 3;
            std::cout << "  Mismatch at (" << x << "," << y << ") chan " << chan 
                      << ": expected " << int(data[i]) << ", got " << int(readback[i]) << "\n";
            mismatches++;
        }
    }
    
    TEST_ASSERT(mismatches == 0, "Pixels don't match");
    TEST_SUCCESS();
}

int main() {
    std::cout << "=== Positional and Feature Validation Tests ===\n\n";
    
    test_random_rgb_pattern();
    test_random_rgba_pattern();
    test_checkerboard_pattern();
    test_gradient_all_directions();
    test_large_random_with_alpha();
    test_all_unique_pixels();
    test_diagonal_stripes();
    test_edge_pixels();
    
    // Clean up test files
    remove("test_random.rle");
    remove("test_random_alpha.rle");
    remove("test_checkerboard.rle");
    remove("test_gradient.rle");
    remove("test_large_alpha.rle");
    remove("test_unique.rle");
    remove("test_diag.rle");
    remove("test_edges.rle");
    
    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    std::cout << "Total:  " << (tests_passed + tests_failed) << "\n";
    
    return tests_failed > 0 ? 1 : 0;
}
