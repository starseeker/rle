/**
 * @file test_teapot_roundtrip.cpp
 * @brief Test to verify that reading and writing teapot.rle preserves image data exactly
 * 
 * This test reads the teapot.rle file, writes it back out to a new file,
 * then reads both files and compares them pixel-by-pixel to ensure 
 * there are no issues in the read/write logic.
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

// External functions from rle.cpp
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

bool compare_images(const icv_image_t* img1, const icv_image_t* img2) {
    if (!img1 || !img2) {
        std::cout << "ERROR: One or both images are null\n";
        return false;
    }
    
    if (img1->width != img2->width) {
        std::cout << "ERROR: Width mismatch: " << img1->width << " vs " << img2->width << "\n";
        return false;
    }
    
    if (img1->height != img2->height) {
        std::cout << "ERROR: Height mismatch: " << img1->height << " vs " << img2->height << "\n";
        return false;
    }
    
    if (img1->channels != img2->channels) {
        std::cout << "ERROR: Channels mismatch: " << img1->channels << " vs " << img2->channels << "\n";
        return false;
    }
    
    size_t total_values = img1->width * img1->height * img1->channels;
    size_t errors = 0;
    size_t max_errors_to_show = 20;
    
    for (size_t i = 0; i < total_values; i++) {
        double diff = std::abs(img1->data[i] - img2->data[i]);
        if (diff > 0.004) {  // Allow small tolerance due to rounding (1/255 ≈ 0.00392)
            if (errors < max_errors_to_show) {
                size_t pixel_idx = i / img1->channels;
                size_t channel = i % img1->channels;
                size_t y = pixel_idx / img1->width;
                size_t x = pixel_idx % img1->width;
                std::cout << "  Pixel mismatch at (" << x << "," << y << ") channel " << channel
                          << ": " << img1->data[i] << " vs " << img2->data[i] 
                          << " (diff: " << diff << ")\n";
            }
            errors++;
        }
    }
    
    if (errors > 0) {
        std::cout << "ERROR: " << errors << " pixel value mismatches found\n";
        return false;
    }
    
    return true;
}

void print_image_sample(const icv_image_t* img, const char* label) {
    std::cout << "\n" << label << ":\n";
    std::cout << "  Dimensions: " << img->width << "x" << img->height << "\n";
    std::cout << "  Channels: " << img->channels << "\n";
    std::cout << "  Alpha channel: " << img->alpha_channel << "\n";
    
    // Print first 10 pixels
    std::cout << "  First 10 pixels:\n";
    size_t pixels_to_show = std::min<size_t>(10, img->width * img->height);
    for (size_t i = 0; i < pixels_to_show; i++) {
        size_t idx = i * img->channels;
        std::cout << "    Pixel " << i << " (" << (i % img->width) << "," << (i / img->width) << "): ";
        for (size_t c = 0; c < img->channels; c++) {
            std::cout << img->data[idx + c];
            if (c < img->channels - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
    
    // Print middle row sample
    size_t mid_y = img->height / 2;
    std::cout << "  Middle row (y=" << mid_y << "), first 10 pixels:\n";
    for (size_t x = 0; x < std::min<size_t>(10, img->width); x++) {
        size_t idx = (mid_y * img->width + x) * img->channels;
        std::cout << "    Pixel (" << x << "," << mid_y << "): ";
        for (size_t c = 0; c < img->channels; c++) {
            std::cout << img->data[idx + c];
            if (c < img->channels - 1) std::cout << ", ";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Teapot RLE Roundtrip Test\n";
    std::cout << "========================================\n\n";
    
    const char* input_file = "teapot.rle";
    const char* output_file = "teapot_roundtrip.rle";
    
    // Step 1: Read original teapot.rle
    std::cout << "Step 1: Reading original teapot.rle...\n";
    FILE* fp = std::fopen(input_file, "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open " << input_file << "\n";
        return 1;
    }
    
    icv_image_t* original = rle_read(fp);
    std::fclose(fp);
    
    if (!original) {
        std::cout << "ERROR: Failed to read " << input_file << "\n";
        return 1;
    }
    
    std::cout << "SUCCESS: Read image with dimensions " 
              << original->width << "x" << original->height 
              << ", " << original->channels << " channels\n";
    print_image_sample(original, "Original image");
    
    // Step 2: Write to new file
    std::cout << "\nStep 2: Writing to " << output_file << "...\n";
    fp = std::fopen(output_file, "wb");
    if (!fp) {
        std::cout << "ERROR: Could not open " << output_file << " for writing\n";
        bu_free(original->data, "image data");
        bu_free(original, "image");
        return 1;
    }
    
    int result = rle_write(original, fp);
    std::fclose(fp);
    
    if (result != 0) {
        std::cout << "ERROR: Failed to write " << output_file << "\n";
        bu_free(original->data, "image data");
        bu_free(original, "image");
        return 1;
    }
    
    std::cout << "SUCCESS: Wrote image to " << output_file << "\n";
    
    // Step 3: Read the written file
    std::cout << "\nStep 3: Reading back " << output_file << "...\n";
    fp = std::fopen(output_file, "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open " << output_file << " for reading\n";
        bu_free(original->data, "image data");
        bu_free(original, "image");
        return 1;
    }
    
    icv_image_t* roundtrip = rle_read(fp);
    std::fclose(fp);
    
    if (!roundtrip) {
        std::cout << "ERROR: Failed to read back " << output_file << "\n";
        bu_free(original->data, "image data");
        bu_free(original, "image");
        return 1;
    }
    
    std::cout << "SUCCESS: Read back image with dimensions " 
              << roundtrip->width << "x" << roundtrip->height 
              << ", " << roundtrip->channels << " channels\n";
    print_image_sample(roundtrip, "Roundtrip image");
    
    // Step 4: Compare pixel-by-pixel
    std::cout << "\nStep 4: Comparing images pixel-by-pixel...\n";
    bool match = compare_images(original, roundtrip);
    
    // Cleanup
    bu_free(original->data, "image data");
    bu_free(original, "image");
    bu_free(roundtrip->data, "image data");
    bu_free(roundtrip, "image");
    
    if (match) {
        std::cout << "\n✓ SUCCESS: Images match perfectly!\n";
        std::cout << "The read/write logic preserves image data correctly.\n";
        return 0;
    } else {
        std::cout << "\n✗ FAILURE: Images do not match!\n";
        std::cout << "There is a logic error in the read/write code.\n";
        return 1;
    }
}
