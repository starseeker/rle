/**
 * @file test_alternating_lines.cpp
 * @brief Test to detect alternating black line patterns
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cmath>

// External functions from rle.cpp
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

icv_image_t* create_test_image(size_t width, size_t height, size_t channels) {
    icv_image_t *img = (icv_image_t*)calloc(1, sizeof(icv_image_t));
    if (!img) return nullptr;
    
    img->magic = 0x6269666d;  // ICV_IMAGE_MAGIC
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->alpha_channel = (channels >= 4) ? 1 : 0;
    img->color_space = ICV_COLOR_SPACE_RGB;
    img->gamma_corr = 0.0;
    
    size_t data_size = width * height * channels * sizeof(double);
    img->data = (double*)calloc(1, data_size);
    if (!img->data) {
        free(img);
        return nullptr;
    }
    
    return img;
}

void free_test_image(icv_image_t* img) {
    if (!img) return;
    if (img->data) bu_free(img->data, "image data");
    bu_free(img, "image struct");
}

bool check_for_alternating_black_lines(const icv_image_t* img) {
    if (!img || !img->data) return false;
    
    size_t black_lines = 0;
    size_t non_black_lines = 0;
    
    std::cout << "\nAnalyzing rows for black line pattern:\n";
    
    for (size_t y = 0; y < img->height; y++) {
        bool is_black = true;
        double sum = 0.0;
        
        for (size_t x = 0; x < img->width; x++) {
            size_t idx = (y * img->width + x) * img->channels;
            for (size_t c = 0; c < img->channels; c++) {
                sum += img->data[idx + c];
                if (img->data[idx + c] > 0.01) {
                    is_black = false;
                }
            }
        }
        
        double avg = sum / (img->width * img->channels);
        
        if (y < 10 || y >= img->height - 10) {
            std::cout << "  Row " << y << ": avg=" << avg << (is_black ? " (BLACK)" : "") << "\n";
        }
        
        if (is_black) {
            black_lines++;
        } else {
            non_black_lines++;
        }
    }
    
    std::cout << "\nSummary:\n";
    std::cout << "  Black lines: " << black_lines << "\n";
    std::cout << "  Non-black lines: " << non_black_lines << "\n";
    
    // Check if every other line is black (alternating pattern)
    if (black_lines > img->height / 4 && black_lines < 3 * img->height / 4) {
        std::cout << "\n  WARNING: Possible alternating black line pattern detected!\n";
        
        // Check if it's truly alternating
        bool alternating = true;
        for (size_t y = 0; y < std::min<size_t>(10, img->height); y++) {
            bool is_black = true;
            for (size_t x = 0; x < img->width && is_black; x++) {
                size_t idx = (y * img->width + x) * img->channels;
                for (size_t c = 0; c < img->channels; c++) {
                    if (img->data[idx + c] > 0.01) {
                        is_black = false;
                        break;
                    }
                }
            }
            
            std::cout << "  Row " << y << ": " << (is_black ? "BLACK" : "DATA") << "\n";
        }
        
        return true;
    }
    
    return false;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Alternating Black Lines Test\n";
    std::cout << "========================================\n\n";
    
    // Test 1: Read teapot.rle
    std::cout << "Test 1: Reading teapot.rle\n";
    std::cout << "----------------------------\n";
    
    FILE* fp = std::fopen("teapot.rle", "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open teapot.rle\n";
        std::cout << "Test SKIPPED\n";
        return 0;
    }
    
    icv_image_t* teapot = rle_read(fp);
    std::fclose(fp);
    
    if (!teapot) {
        std::cout << "ERROR: Failed to read teapot.rle\n";
        return 1;
    }
    
    std::cout << "Image dimensions: " << teapot->width << "x" << teapot->height << "\n";
    std::cout << "Channels: " << teapot->channels << "\n";
    
    bool has_pattern = check_for_alternating_black_lines(teapot);
    
    if (has_pattern) {
        std::cout << "\n✗ FAIL: Alternating black line pattern detected in teapot.rle!\n";
    } else {
        std::cout << "\n✓ PASS: No alternating black line pattern detected.\n";
    }
    
    free_test_image(teapot);
    
    // Test 2: Create a striped pattern and verify roundtrip
    std::cout << "\n\nTest 2: Striped pattern roundtrip\n";
    std::cout << "-----------------------------------\n";
    
    const size_t W = 32, H = 32;
    icv_image_t* striped = create_test_image(W, H, 3);
    if (!striped) {
        std::cout << "ERROR: Failed to create test image\n";
        return 1;
    }
    
    // Create alternating black and white stripes
    for (size_t y = 0; y < H; y++) {
        double value = (y % 2 == 0) ? 1.0 : 0.0;
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            striped->data[idx + 0] = value;
            striped->data[idx + 1] = value;
            striped->data[idx + 2] = value;
        }
    }
    
    std::cout << "Created striped image (alternating black/white rows)\n";
    
    fp = std::fopen("test_striped.rle", "wb");
    if (!fp) {
        std::cout << "ERROR: Could not open file for writing\n";
        free_test_image(striped);
        return 1;
    }
    
    int result = rle_write(striped, fp);
    std::fclose(fp);
    
    if (result != 0) {
        std::cout << "ERROR: Failed to write RLE file\n";
        free_test_image(striped);
        return 1;
    }
    
    fp = std::fopen("test_striped.rle", "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open file for reading\n";
        free_test_image(striped);
        return 1;
    }
    
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    
    if (!readback) {
        std::cout << "ERROR: Failed to read RLE file\n";
        free_test_image(striped);
        return 1;
    }
    
    // Compare
    bool match = true;
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            for (size_t c = 0; c < 3; c++) {
                double diff = std::abs(striped->data[idx + c] - readback->data[idx + c]);
                if (diff > 0.01) {
                    std::cout << "Mismatch at (" << x << "," << y << ") channel " << c << "\n";
                    match = false;
                }
            }
        }
    }
    
    if (match) {
        std::cout << "✓ PASS: Striped pattern preserved correctly in roundtrip\n";
    } else {
        std::cout << "✗ FAIL: Striped pattern not preserved!\n";
    }
    
    free_test_image(striped);
    free_test_image(readback);
    
    return 0;
}
