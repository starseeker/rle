/**
 * @file test_vertical_flip.cpp
 * @brief Test to check if RLE format uses bottom-to-top scanline ordering
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <vector>

// External functions from rle.cpp
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);
icv_image_t* create_test_image(size_t width, size_t height, size_t channels);
void free_test_image(icv_image_t* img);

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
    if (img->data) {
        bu_free(img->data, "image data");
    }
    bu_free(img, "image struct");
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Vertical Flip Test\n";
    std::cout << "========================================\n\n";
    
    const size_t W = 8, H = 8;
    
    // Create a gradient image: top rows are bright, bottom rows are dark
    icv_image_t* img = create_test_image(W, H, 3);
    if (!img) {
        std::cout << "ERROR: Failed to create test image\n";
        return 1;
    }
    
    // Fill image with gradient: y=0 (top) is white, y=7 (bottom) is black
    for (size_t y = 0; y < H; y++) {
        double intensity = 1.0 - (double)y / (H - 1);  // Top=1.0, bottom=0.0
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            img->data[idx + 0] = intensity;
            img->data[idx + 1] = intensity;
            img->data[idx + 2] = intensity;
        }
    }
    
    std::cout << "Created test gradient image " << W << "x" << H << "\n";
    std::cout << "Original image (top to bottom):\n";
    for (size_t y = 0; y < H; y++) {
        std::cout << "  Row " << y << ": ";
        size_t idx = (y * W) * 3;
        std::cout << img->data[idx + 0] << "\n";
    }
    
    // Write to file
    FILE* fp = std::fopen("test_gradient.rle", "wb");
    if (!fp) {
        std::cout << "ERROR: Could not open file for writing\n";
        free_test_image(img);
        return 1;
    }
    
    int result = rle_write(img, fp);
    std::fclose(fp);
    
    if (result != 0) {
        std::cout << "ERROR: Failed to write RLE file\n";
        free_test_image(img);
        return 1;
    }
    
    std::cout << "\nWrote image to file\n";
    
    // Read it back
    fp = std::fopen("test_gradient.rle", "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open file for reading\n";
        free_test_image(img);
        return 1;
    }
    
    icv_image_t* readback = rle_read(fp);
    std::fclose(fp);
    
    if (!readback) {
        std::cout << "ERROR: Failed to read RLE file\n";
        free_test_image(img);
        return 1;
    }
    
    std::cout << "\nRead back image (top to bottom):\n";
    for (size_t y = 0; y < H; y++) {
        std::cout << "  Row " << y << ": ";
        size_t idx = (y * W) * 3;
        std::cout << readback->data[idx + 0] << "\n";
    }
    
    // Check if image is flipped
    bool is_flipped = true;
    bool is_same = true;
    
    for (size_t y = 0; y < H; y++) {
        size_t orig_idx = (y * W) * 3;
        size_t flip_idx = ((H - 1 - y) * W) * 3;
        
        double orig_val = img->data[orig_idx];
        double read_val = readback->data[orig_idx];
        double flip_val = readback->data[flip_idx];
        
        if (std::abs(orig_val - read_val) < 0.01) {
            is_flipped = false;
        }
        if (std::abs(orig_val - flip_val) < 0.01) {
            is_same = false;
        }
    }
    
    std::cout << "\nAnalysis:\n";
    if (is_same && !is_flipped) {
        std::cout << "  ✓ Image is preserved correctly (same orientation)\n";
    } else if (is_flipped && !is_same) {
        std::cout << "  ✗ Image is FLIPPED vertically!\n";
        std::cout << "  RLE format uses bottom-to-top scanline order\n";
    } else {
        std::cout << "  ? Image has different pattern (neither same nor flipped)\n";
    }
    
    free_test_image(img);
    free_test_image(readback);
    
    return 0;
}
