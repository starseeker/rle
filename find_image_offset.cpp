/**
 * @file find_image_offset.cpp
 * @brief Find the best vertical/horizontal offset between two PPM images
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

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
 * Calculate sum of absolute differences between two regions
 */
uint64_t calculate_sad(const PPMImage* img1, const PPMImage* img2, 
                       int offset_x, int offset_y, size_t compare_w, size_t compare_h) {
    uint64_t sad = 0;
    
    for (size_t y = 0; y < compare_h; y++) {
        for (size_t x = 0; x < compare_w; x++) {
            size_t img1_y = y;
            size_t img1_x = x;
            size_t img2_y = y + offset_y;
            size_t img2_x = x + offset_x;
            
            if (img1_y >= img1->height || img1_x >= img1->width ||
                img2_y >= img2->height || img2_x >= img2->width) {
                // Out of bounds - add penalty
                sad += 255 * 3;
                continue;
            }
            
            size_t idx1 = (img1_y * img1->width + img1_x) * 3;
            size_t idx2 = (img2_y * img2->width + img2_x) * 3;
            
            for (int c = 0; c < 3; c++) {
                sad += abs(img1->data[idx1 + c] - img2->data[idx2 + c]);
            }
        }
    }
    
    return sad;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <ppm1> <ppm2>" << std::endl;
        std::cout << "Finds the best offset to align ppm1 onto ppm2" << std::endl;
        return 1;
    }
    
    PPMImage* img1 = read_ppm(argv[1]);
    if (!img1) return 1;
    
    PPMImage* img2 = read_ppm(argv[2]);
    if (!img2) {
        delete img1;
        return 1;
    }
    
    std::cout << "Image 1: " << img1->width << " x " << img1->height << std::endl;
    std::cout << "Image 2: " << img2->width << " x " << img2->height << std::endl;
    
    // Search for best offset
    int max_offset_x = 10;
    int max_offset_y = 50;  // Search up to 50 rows vertical offset
    
    size_t compare_w = std::min(img1->width, img2->width) - max_offset_x;
    size_t compare_h = std::min(img1->height, img2->height) - max_offset_y;
    
    std::cout << "\nSearching for best alignment..." << std::endl;
    std::cout << "  Offset range: x=[" << -max_offset_x << ", " << max_offset_x 
              << "], y=[" << -max_offset_y << ", " << max_offset_y << "]" << std::endl;
    std::cout << "  Comparison region: " << compare_w << " x " << compare_h << std::endl;
    
    uint64_t best_sad = UINT64_MAX;
    int best_offset_x = 0;
    int best_offset_y = 0;
    
    for (int dy = -max_offset_y; dy <= max_offset_y; dy++) {
        for (int dx = -max_offset_x; dx <= max_offset_x; dx++) {
            uint64_t sad = calculate_sad(img1, img2, dx, dy, compare_w, compare_h);
            if (sad < best_sad) {
                best_sad = sad;
                best_offset_x = dx;
                best_offset_y = dy;
            }
        }
    }
    
    std::cout << "\nBest alignment found:" << std::endl;
    std::cout << "  Offset: (" << best_offset_x << ", " << best_offset_y << ")" << std::endl;
    std::cout << "  SAD: " << best_sad << std::endl;
    
    // Calculate average difference per pixel
    uint64_t total_pixels = compare_w * compare_h;
    double avg_diff_per_pixel = (double)best_sad / (total_pixels * 3);
    std::cout << "  Average diff per channel: " << avg_diff_per_pixel << std::endl;
    
    if (avg_diff_per_pixel < 1.0) {
        std::cout << "\n✓ EXCELLENT MATCH (avg diff < 1)" << std::endl;
    } else if (avg_diff_per_pixel < 5.0) {
        std::cout << "\n✓ GOOD MATCH (avg diff < 5)" << std::endl;
    } else if (avg_diff_per_pixel < 20.0) {
        std::cout << "\n~ FAIR MATCH (avg diff < 20)" << std::endl;
    } else {
        std::cout << "\n✗ POOR MATCH (avg diff >= 20)" << std::endl;
    }
    
    // Show interpretation
    if (best_offset_y != 0 || best_offset_x != 0) {
        std::cout << "\nInterpretation:" << std::endl;
        if (best_offset_y > 0) {
            std::cout << "  Image 2 is offset " << best_offset_y << " rows DOWN from image 1" << std::endl;
            std::cout << "  (Image 1 starts " << best_offset_y << " rows earlier than image 2)" << std::endl;
        } else if (best_offset_y < 0) {
            std::cout << "  Image 2 is offset " << (-best_offset_y) << " rows UP from image 1" << std::endl;
            std::cout << "  (Image 1 starts " << (-best_offset_y) << " rows later than image 2)" << std::endl;
        }
        if (best_offset_x > 0) {
            std::cout << "  Image 2 is offset " << best_offset_x << " columns RIGHT from image 1" << std::endl;
        } else if (best_offset_x < 0) {
            std::cout << "  Image 2 is offset " << (-best_offset_x) << " columns LEFT from image 1" << std::endl;
        }
    } else {
        std::cout << "\nImages are already aligned (no offset needed)" << std::endl;
    }
    
    delete img1;
    delete img2;
    
    return 0;
}
