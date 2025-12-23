/**
 * @file test_lenna_comparison.cpp
 * @brief Compare lenna.rle with lenna.ppm (ground truth) to detect visual artifacts
 *
 * This tool reads both the RLE file and the PPM file, and compares them
 * pixel-by-pixel to identify any discrepancies that might indicate visual
 * artifacts such as horizontal or vertical banding.
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

// Declare external functions from rle.cpp
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

struct PPMImage {
    size_t width;
    size_t height;
    std::vector<uint8_t> data; // RGB data
};

/**
 * Read a PPM file (P6 format - binary)
 */
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
 * Convert double [0,1] to uint8_t [0,255]
 */
uint8_t double_to_uint8(double v) {
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return static_cast<uint8_t>(lrint(v * 255.0));
}

/**
 * Analyze horizontal banding by checking row similarity
 */
void analyze_horizontal_banding(icv_image_t* img) {
    std::cout << "\n=== Analyzing Horizontal Banding ===" << std::endl;
    
    int consecutive_black_rows = 0;
    int max_consecutive_black = 0;
    int total_black_rows = 0;
    
    for (size_t y = 0; y < img->height; y++) {
        bool is_black = true;
        bool has_variation = false;
        double first_r = 0, first_g = 0, first_b = 0;
        
        for (size_t x = 0; x < img->width; x++) {
            size_t idx = (y * img->width + x) * img->channels;
            double r = img->data[idx];
            double g = img->data[idx + 1];
            double b = img->data[idx + 2];
            
            if (x == 0) {
                first_r = r; first_g = g; first_b = b;
            }
            
            // Check if pixel is not black
            if (r > 0.01 || g > 0.01 || b > 0.01) {
                is_black = false;
            }
            
            // Check for variation within row
            if (fabs(r - first_r) > 0.01 || fabs(g - first_g) > 0.01 || fabs(b - first_b) > 0.01) {
                has_variation = true;
            }
        }
        
        if (is_black) {
            consecutive_black_rows++;
            total_black_rows++;
            if (consecutive_black_rows > max_consecutive_black) {
                max_consecutive_black = consecutive_black_rows;
            }
        } else {
            consecutive_black_rows = 0;
        }
        
        // Report alternating pattern (like teapot.rle issue)
        if (y > 0 && y % 2 == 1 && is_black) {
            // Check if even rows have data and odd rows are black
            if (y == 1) {
                std::cout << "Warning: Potential alternating line pattern detected (odd rows black)" << std::endl;
            }
        }
    }
    
    std::cout << "Total black rows: " << total_black_rows << " / " << img->height 
              << " (" << (100.0 * total_black_rows / img->height) << "%)" << std::endl;
    std::cout << "Max consecutive black rows: " << max_consecutive_black << std::endl;
}

/**
 * Analyze vertical banding by checking column similarity
 */
void analyze_vertical_banding(icv_image_t* img) {
    std::cout << "\n=== Analyzing Vertical Banding ===" << std::endl;
    
    // Sample every 10th column to avoid excessive computation
    int stride = std::max(1, (int)(img->width / 50));
    int suspicious_columns = 0;
    
    for (size_t x = 0; x < img->width; x += stride) {
        bool is_uniform = true;
        double first_r = 0, first_g = 0, first_b = 0;
        
        for (size_t y = 0; y < img->height; y++) {
            size_t idx = (y * img->width + x) * img->channels;
            double r = img->data[idx];
            double g = img->data[idx + 1];
            double b = img->data[idx + 2];
            
            if (y == 0) {
                first_r = r; first_g = g; first_b = b;
            }
            
            // Check for variation within column
            if (fabs(r - first_r) > 0.01 || fabs(g - first_g) > 0.01 || fabs(b - first_b) > 0.01) {
                is_uniform = false;
                break;
            }
        }
        
        if (is_uniform) {
            suspicious_columns++;
        }
    }
    
    int total_sampled = (img->width + stride - 1) / stride;
    std::cout << "Uniform columns: " << suspicious_columns << " / " << total_sampled 
              << " sampled (" << (100.0 * suspicious_columns / total_sampled) << "%)" << std::endl;
}

/**
 * Compare RLE image with PPM ground truth
 */
void compare_images(const char* rle_file, const char* ppm_file) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Comparing RLE and PPM images" << std::endl;
    std::cout << "RLE: " << rle_file << std::endl;
    std::cout << "PPM: " << ppm_file << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Read RLE file
    FILE* rle_fp = fopen(rle_file, "rb");
    if (!rle_fp) {
        std::cerr << "Failed to open RLE file: " << rle_file << std::endl;
        return;
    }
    icv_image_t* rle_img = rle_read(rle_fp);
    fclose(rle_fp);
    
    if (!rle_img) {
        std::cerr << "Failed to read RLE file" << std::endl;
        return;
    }
    
    std::cout << "\nRLE Image Info:" << std::endl;
    std::cout << "  Dimensions: " << rle_img->width << " x " << rle_img->height << std::endl;
    std::cout << "  Channels: " << rle_img->channels << std::endl;
    std::cout << "  Has Alpha: " << (rle_img->alpha_channel ? "Yes" : "No") << std::endl;
    
    // Read PPM file
    PPMImage* ppm_img = read_ppm(ppm_file);
    if (!ppm_img) {
        std::cerr << "Failed to read PPM file" << std::endl;
        bu_free(rle_img->data, "image data");
        bu_free(rle_img, "image");
        return;
    }
    
    std::cout << "\nPPM Image Info:" << std::endl;
    std::cout << "  Dimensions: " << ppm_img->width << " x " << ppm_img->height << std::endl;
    std::cout << "  Channels: 3 (RGB)" << std::endl;
    
    // Check dimension mismatch
    if (rle_img->width != ppm_img->width || rle_img->height != ppm_img->height) {
        std::cout << "\n*** DIMENSION MISMATCH ***" << std::endl;
        std::cout << "RLE: " << rle_img->width << " x " << rle_img->height << std::endl;
        std::cout << "PPM: " << ppm_img->width << " x " << ppm_img->height << std::endl;
        
        // Determine the overlap region
        size_t min_width = std::min(rle_img->width, ppm_img->width);
        size_t min_height = std::min(rle_img->height, ppm_img->height);
        std::cout << "Will compare overlap region: " << min_width << " x " << min_height << std::endl;
    }
    
    // Analyze banding in RLE image
    analyze_horizontal_banding(rle_img);
    analyze_vertical_banding(rle_img);
    
    // Compare pixels in the overlap region
    size_t compare_width = std::min(rle_img->width, ppm_img->width);
    size_t compare_height = std::min(rle_img->height, ppm_img->height);
    
    std::cout << "\n=== Pixel Comparison ===" << std::endl;
    
    uint64_t total_pixels = compare_width * compare_height;
    uint64_t differing_pixels = 0;
    uint64_t total_diff = 0;
    uint64_t max_diff = 0;
    
    for (size_t y = 0; y < compare_height; y++) {
        for (size_t x = 0; x < compare_width; x++) {
            size_t rle_idx = (y * rle_img->width + x) * rle_img->channels;
            size_t ppm_idx = (y * ppm_img->width + x) * 3;
            
            uint8_t rle_r = double_to_uint8(rle_img->data[rle_idx]);
            uint8_t rle_g = double_to_uint8(rle_img->data[rle_idx + 1]);
            uint8_t rle_b = double_to_uint8(rle_img->data[rle_idx + 2]);
            
            uint8_t ppm_r = ppm_img->data[ppm_idx];
            uint8_t ppm_g = ppm_img->data[ppm_idx + 1];
            uint8_t ppm_b = ppm_img->data[ppm_idx + 2];
            
            uint64_t diff_r = abs(rle_r - ppm_r);
            uint64_t diff_g = abs(rle_g - ppm_g);
            uint64_t diff_b = abs(rle_b - ppm_b);
            uint64_t diff = diff_r + diff_g + diff_b;
            
            if (diff > 0) {
                differing_pixels++;
                total_diff += diff;
                if (diff > max_diff) {
                    max_diff = diff;
                }
                
                // Report first few mismatches
                if (differing_pixels <= 10) {
                    std::cout << "  Mismatch at (" << x << ", " << y << "): "
                              << "RLE=(" << (int)rle_r << "," << (int)rle_g << "," << (int)rle_b << ") "
                              << "PPM=(" << (int)ppm_r << "," << (int)ppm_g << "," << (int)ppm_b << ") "
                              << "Diff=" << diff << std::endl;
                }
            }
        }
    }
    
    std::cout << "\nComparison Results:" << std::endl;
    std::cout << "  Total pixels compared: " << total_pixels << std::endl;
    std::cout << "  Differing pixels: " << differing_pixels 
              << " (" << (100.0 * differing_pixels / total_pixels) << "%)" << std::endl;
    
    if (differing_pixels > 0) {
        double avg_diff = (double)total_diff / differing_pixels;
        std::cout << "  Average difference (per differing pixel): " << avg_diff << std::endl;
        std::cout << "  Maximum difference: " << max_diff << std::endl;
    }
    
    if (differing_pixels == 0) {
        std::cout << "\n✓ PERFECT MATCH: RLE and PPM images are identical!" << std::endl;
    } else if (differing_pixels < total_pixels * 0.01) {
        std::cout << "\n✓ GOOD MATCH: Less than 1% of pixels differ" << std::endl;
    } else {
        std::cout << "\n✗ SIGNIFICANT DIFFERENCES: Visual artifacts likely present" << std::endl;
    }
    
    // Cleanup
    bu_free(rle_img->data, "image data");
    bu_free(rle_img, "image");
    delete ppm_img;
}

int main(int argc, char** argv) {
    const char* rle_file = "lenna.rle";
    const char* ppm_file = "lenna.ppm";
    
    if (argc == 3) {
        rle_file = argv[1];
        ppm_file = argv[2];
    }
    
    compare_images(rle_file, ppm_file);
    
    // Also test other RLE files
    std::cout << "\n\n" << std::endl;
    compare_images("imgs/lenna.rle", ppm_file);
    
    return 0;
}
