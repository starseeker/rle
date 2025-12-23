/**
 * @file convert_rle_to_ppm.cpp
 * @brief Convert RLE files to PPM format for visual inspection
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>

// Declare external functions from rle.cpp
icv_image_t* rle_read(FILE *fp);
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
 * Write PPM file (P6 format - binary)
 */
bool write_ppm(const char* filename, icv_image_t* img) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        return false;
    }
    
    // Write header
    fprintf(fp, "P6\n");
    fprintf(fp, "%zu %zu\n", img->width, img->height);
    fprintf(fp, "255\n");
    
    // Write pixel data
    for (size_t y = 0; y < img->height; y++) {
        for (size_t x = 0; x < img->width; x++) {
            size_t idx = (y * img->width + x) * img->channels;
            uint8_t r = double_to_uint8(img->data[idx]);
            uint8_t g = double_to_uint8(img->data[idx + 1]);
            uint8_t b = double_to_uint8(img->data[idx + 2]);
            fputc(r, fp);
            fputc(g, fp);
            fputc(b, fp);
        }
    }
    
    fclose(fp);
    return true;
}

/**
 * Convert RLE to PPM
 */
bool convert_rle_to_ppm(const char* rle_file, const char* ppm_file) {
    std::cout << "Converting " << rle_file << " to " << ppm_file << " ..." << std::endl;
    
    // Read RLE file
    FILE* rle_fp = fopen(rle_file, "rb");
    if (!rle_fp) {
        std::cerr << "Failed to open RLE file: " << rle_file << std::endl;
        return false;
    }
    
    icv_image_t* img = rle_read(rle_fp);
    fclose(rle_fp);
    
    if (!img) {
        std::cerr << "Failed to read RLE file" << std::endl;
        return false;
    }
    
    std::cout << "  RLE Image: " << img->width << " x " << img->height 
              << " channels=" << img->channels 
              << " alpha=" << (img->alpha_channel ? "yes" : "no") << std::endl;
    
    // Write PPM file
    bool success = write_ppm(ppm_file, img);
    
    // Cleanup
    bu_free(img->data, "image data");
    bu_free(img, "image");
    
    if (success) {
        std::cout << "  ✓ Successfully converted to " << ppm_file << std::endl;
    }
    
    return success;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <rle-file> [<output-ppm>]" << std::endl;
        std::cout << "   or: " << argv[0] << " --all    (convert all example files)" << std::endl;
        return 1;
    }
    
    if (std::string(argv[1]) == "--all") {
        std::cout << "Converting all example RLE files to PPM..." << std::endl;
        std::cout << "========================================" << std::endl;
        
        const char* files[][2] = {
            {"lenna.rle", "lenna_decoded.ppm"},
            {"imgs/lenna.rle", "imgs_lenna_decoded.ppm"},
            {"imgs/mandrill.rle", "imgs_mandrill_decoded.ppm"},
            {"imgs/dart.rle", "imgs_dart_decoded.ppm"},
            {"imgs/christmas_ball.rle", "imgs_christmas_ball_decoded.ppm"},
            {"imgs/tack_w_shadow.rle", "imgs_tack_w_shadow_decoded.ppm"},
        };
        
        int success_count = 0;
        int total_count = sizeof(files) / sizeof(files[0]);
        
        for (int i = 0; i < total_count; i++) {
            std::cout << "\n[" << (i+1) << "/" << total_count << "] ";
            if (convert_rle_to_ppm(files[i][0], files[i][1])) {
                success_count++;
            }
        }
        
        std::cout << "\n========================================" << std::endl;
        std::cout << "Converted " << success_count << " / " << total_count << " files" << std::endl;
        
        return (success_count == total_count) ? 0 : 1;
    }
    
    const char* rle_file = argv[1];
    const char* ppm_file = (argc >= 3) ? argv[2] : "output.ppm";
    
    return convert_rle_to_ppm(rle_file, ppm_file) ? 0 : 1;
}
