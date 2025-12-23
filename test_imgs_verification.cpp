/**
 * @file test_imgs_verification.cpp
 * @brief Comprehensive verification of RLE encoder/decoder against ImageMagick ground truth
 * 
 * This test exercises all RLE images in the imgs/ directory and verifies:
 * 1. Decoder produces identical output to ImageMagick ground truth PPM files
 * 2. Encoder can encode the decoded data
 * 3. Roundtrip (decode -> encode -> decode) preserves pixel data
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>

// Declare external functions from rle.cpp
icv_image_t* rle_read(FILE *fp);
int rle_write(icv_image_t *img, FILE *fp);
void bu_free(void *ptr, const char *str);

/**
 * Convert double [0,1] to uint8_t [0,255]
 */
uint8_t double_to_uint8(double v) {
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return static_cast<uint8_t>(lrint(v * 255.0));
}

/**
 * Read PPM file (P6 format)
 */
struct PPMImage {
    size_t width;
    size_t height;
    std::vector<uint8_t> data; // RGB data
};

PPMImage* read_ppm(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Failed to open PPM file: " << filename << std::endl;
        return nullptr;
    }
    
    PPMImage* img = new PPMImage();
    
    // Read magic number
    char magic[3] = {0};
    if (fscanf(fp, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        std::cerr << "Invalid PPM magic number (expected P6)" << std::endl;
        fclose(fp);
        delete img;
        return nullptr;
    }
    
    // Skip whitespace and comments
    int c;
    while ((c = fgetc(fp)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '#')) {
        if (c == '#') {
            while ((c = fgetc(fp)) != EOF && c != '\n');
        }
    }
    ungetc(c, fp);
    
    // Read width and height
    if (fscanf(fp, "%zu %zu", &img->width, &img->height) != 2) {
        std::cerr << "Failed to read PPM dimensions" << std::endl;
        fclose(fp);
        delete img;
        return nullptr;
    }
    
    // Read max value
    int maxval;
    if (fscanf(fp, "%d", &maxval) != 1 || maxval != 255) {
        std::cerr << "Invalid PPM maxval (expected 255, got " << maxval << ")" << std::endl;
        fclose(fp);
        delete img;
        return nullptr;
    }
    
    // Skip single whitespace after maxval
    fgetc(fp);
    
    // Read pixel data
    size_t pixel_count = img->width * img->height * 3;
    img->data.resize(pixel_count);
    size_t read = fread(img->data.data(), 1, pixel_count, fp);
    if (read != pixel_count) {
        std::cerr << "Failed to read all PPM pixel data (expected " << pixel_count 
                  << ", got " << read << ")" << std::endl;
        fclose(fp);
        delete img;
        return nullptr;
    }
    
    fclose(fp);
    return img;
}

/**
 * Compare RLE decoded image with PPM ground truth
 * Returns true if they match (within tolerance)
 */
bool compare_with_ground_truth(icv_image_t* rle_img, PPMImage* ppm_img, const char* name) {
    if (rle_img->width != ppm_img->width || rle_img->height != ppm_img->height) {
        std::cerr << "  ✗ DIMENSION MISMATCH: RLE=" << rle_img->width << "x" << rle_img->height
                  << " PPM=" << ppm_img->width << "x" << ppm_img->height << std::endl;
        return false;
    }
    
    uint64_t total_pixels = rle_img->width * rle_img->height;
    uint64_t differing_pixels = 0;
    
    for (size_t y = 0; y < rle_img->height; y++) {
        for (size_t x = 0; x < rle_img->width; x++) {
            size_t rle_idx = (y * rle_img->width + x) * rle_img->channels;
            size_t ppm_idx = (y * ppm_img->width + x) * 3;
            
            uint8_t rle_r = double_to_uint8(rle_img->data[rle_idx]);
            uint8_t rle_g = double_to_uint8(rle_img->data[rle_idx + 1]);
            uint8_t rle_b = double_to_uint8(rle_img->data[rle_idx + 2]);
            
            uint8_t ppm_r = ppm_img->data[ppm_idx];
            uint8_t ppm_g = ppm_img->data[ppm_idx + 1];
            uint8_t ppm_b = ppm_img->data[ppm_idx + 2];
            
            if (rle_r != ppm_r || rle_g != ppm_g || rle_b != ppm_b) {
                differing_pixels++;
                
                // Report first few mismatches
                if (differing_pixels <= 5) {
                    std::cerr << "    Mismatch at (" << x << ", " << y << "): "
                              << "RLE=(" << (int)rle_r << "," << (int)rle_g << "," << (int)rle_b << ") "
                              << "PPM=(" << (int)ppm_r << "," << (int)ppm_g << "," << (int)ppm_b << ")" << std::endl;
                }
            }
        }
    }
    
    if (differing_pixels > 0) {
        std::cerr << "  ✗ PIXEL MISMATCH: " << differing_pixels << " / " << total_pixels 
                  << " pixels differ (" << (100.0 * differing_pixels / total_pixels) << "%)" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Compare two RLE images for pixel-perfect match
 */
bool compare_images(icv_image_t* img1, icv_image_t* img2, const char* name) {
    if (img1->width != img2->width || img1->height != img2->height || 
        img1->channels != img2->channels) {
        std::cerr << "  ✗ STRUCTURE MISMATCH in " << name << std::endl;
        return false;
    }
    
    size_t total_values = img1->width * img1->height * img1->channels;
    uint64_t differing_values = 0;
    
    for (size_t i = 0; i < total_values; i++) {
        // Compare as uint8 values to match how we encode/decode
        uint8_t v1 = double_to_uint8(img1->data[i]);
        uint8_t v2 = double_to_uint8(img2->data[i]);
        
        if (v1 != v2) {
            differing_values++;
            
            if (differing_values <= 5) {
                size_t pixel_idx = i / img1->channels;
                size_t channel = i % img1->channels;
                size_t x = pixel_idx % img1->width;
                size_t y = pixel_idx / img1->width;
                
                std::cerr << "    Mismatch at pixel (" << x << ", " << y << ") channel " << channel
                          << ": " << (int)v1 << " vs " << (int)v2 << std::endl;
            }
        }
    }
    
    if (differing_values > 0) {
        std::cerr << "  ✗ ROUNDTRIP FAILED: " << differing_values << " values differ" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Test a single RLE file
 */
bool test_rle_file(const char* rle_path, const char* ppm_path, const char* name) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Testing: " << name << std::endl;
    std::cout << "  RLE: " << rle_path << std::endl;
    std::cout << "  PPM: " << ppm_path << std::endl;
    std::cout << "========================================" << std::endl;
    
    bool all_passed = true;
    
    // Step 1: Read the original RLE file
    std::cout << "\n[1/4] Reading original RLE file..." << std::endl;
    FILE* rle_fp = fopen(rle_path, "rb");
    if (!rle_fp) {
        std::cerr << "  ✗ Failed to open RLE file" << std::endl;
        return false;
    }
    
    icv_image_t* original_img = rle_read(rle_fp);
    fclose(rle_fp);
    
    if (!original_img) {
        std::cerr << "  ✗ Failed to read RLE file" << std::endl;
        return false;
    }
    
    std::cout << "  ✓ RLE decoded: " << original_img->width << "x" << original_img->height 
              << " channels=" << original_img->channels 
              << " alpha=" << (original_img->alpha_channel ? "yes" : "no") << std::endl;
    
    // Step 2: Compare with ImageMagick ground truth
    std::cout << "\n[2/4] Comparing with ImageMagick ground truth..." << std::endl;
    PPMImage* ground_truth = read_ppm(ppm_path);
    if (!ground_truth) {
        std::cerr << "  ✗ Failed to read ground truth PPM" << std::endl;
        bu_free(original_img->data, "image data");
        bu_free(original_img, "image");
        return false;
    }
    
    if (compare_with_ground_truth(original_img, ground_truth, name)) {
        std::cout << "  ✓ DECODER PERFECT: Matches ImageMagick ground truth" << std::endl;
    } else {
        std::cerr << "  ✗ DECODER FAILED: Does not match ground truth" << std::endl;
        all_passed = false;
    }
    
    delete ground_truth;
    
    // Step 3: Encode back to RLE
    std::cout << "\n[3/4] Encoding back to RLE (roundtrip test)..." << std::endl;
    std::string tmp_rle = std::string("/tmp/") + name + "_roundtrip.rle";
    FILE* tmp_fp = fopen(tmp_rle.c_str(), "wb");
    if (!tmp_fp) {
        std::cerr << "  ✗ Failed to create temporary RLE file" << std::endl;
        bu_free(original_img->data, "image data");
        bu_free(original_img, "image");
        return false;
    }
    
    int write_result = rle_write(original_img, tmp_fp);
    fclose(tmp_fp);
    
    if (write_result != 0) {
        std::cerr << "  ✗ ENCODER FAILED: rle_write returned " << write_result << std::endl;
        bu_free(original_img->data, "image data");
        bu_free(original_img, "image");
        return false;
    }
    
    std::cout << "  ✓ Successfully encoded to RLE" << std::endl;
    
    // Step 4: Decode the re-encoded RLE and compare
    std::cout << "\n[4/4] Decoding re-encoded RLE (verify roundtrip)..." << std::endl;
    tmp_fp = fopen(tmp_rle.c_str(), "rb");
    if (!tmp_fp) {
        std::cerr << "  ✗ Failed to open re-encoded RLE file" << std::endl;
        bu_free(original_img->data, "image data");
        bu_free(original_img, "image");
        return false;
    }
    
    icv_image_t* roundtrip_img = rle_read(tmp_fp);
    fclose(tmp_fp);
    
    if (!roundtrip_img) {
        std::cerr << "  ✗ Failed to decode re-encoded RLE" << std::endl;
        bu_free(original_img->data, "image data");
        bu_free(original_img, "image");
        return false;
    }
    
    if (compare_images(original_img, roundtrip_img, name)) {
        std::cout << "  ✓ ROUNDTRIP PERFECT: Encode->Decode preserves all data" << std::endl;
    } else {
        std::cerr << "  ✗ ROUNDTRIP FAILED: Data changed after encode->decode" << std::endl;
        all_passed = false;
    }
    
    // Cleanup
    bu_free(original_img->data, "image data");
    bu_free(original_img, "image");
    bu_free(roundtrip_img->data, "image data");
    bu_free(roundtrip_img, "image");
    
    // Summary
    std::cout << "\n========================================" << std::endl;
    if (all_passed) {
        std::cout << "✓✓✓ " << name << ": ALL TESTS PASSED ✓✓✓" << std::endl;
    } else {
        std::cout << "✗✗✗ " << name << ": SOME TESTS FAILED ✗✗✗" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    
    return all_passed;
}

int main(int argc, char** argv) {
    std::cout << "========================================" << std::endl;
    std::cout << "RLE Encoder/Decoder Verification Suite" << std::endl;
    std::cout << "Testing against ImageMagick ground truth" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Test files: {name, rle_path, ppm_ground_truth_path}
    struct TestFile {
        const char* name;
        const char* rle_path;
        const char* ppm_path;
    };
    
    TestFile test_files[] = {
        {"christmas_ball", "imgs/christmas_ball.rle", "imgs_christmas_ball_decoded.ppm"},
        {"dart", "imgs/dart.rle", "imgs_dart_decoded.ppm"},
        {"lenna", "imgs/lenna.rle", "imgs_lenna_decoded.ppm"},
        {"mandrill", "imgs/mandrill.rle", "imgs_mandrill_decoded.ppm"},
        {"tack_w_shadow", "imgs/tack_w_shadow.rle", "imgs_tack_w_shadow_decoded.ppm"},
    };
    
    int total_tests = sizeof(test_files) / sizeof(test_files[0]);
    int passed_tests = 0;
    
    for (int i = 0; i < total_tests; i++) {
        if (test_rle_file(test_files[i].rle_path, test_files[i].ppm_path, test_files[i].name)) {
            passed_tests++;
        }
    }
    
    // Final summary
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "FINAL SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total files tested: " << total_tests << std::endl;
    std::cout << "Passed: " << passed_tests << " (" << (100.0 * passed_tests / total_tests) << "%)" << std::endl;
    std::cout << "Failed: " << (total_tests - passed_tests) << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (passed_tests == total_tests) {
        std::cout << "\n✓✓✓ ALL VERIFICATION TESTS PASSED ✓✓✓" << std::endl;
        std::cout << "The RLE encoder and decoder are working correctly!" << std::endl;
        return 0;
    } else {
        std::cout << "\n✗✗✗ SOME VERIFICATION TESTS FAILED ✗✗✗" << std::endl;
        std::cout << "Please review the failures above." << std::endl;
        return 1;
    }
}
