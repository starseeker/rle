/**
 * Test the sparse row detector directly
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>

// Declare external functions
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rle-file>" << std::endl;
        return 1;
    }
    
    const char* rle_file = argv[1];
    
    std::cout << "Reading " << rle_file << " ..." << std::endl;
    
    FILE* fp = fopen(rle_file, "rb");
    if (!fp) {
        std::cerr << "Failed to open: " << rle_file << std::endl;
        return 1;
    }
    
    icv_image_t* img = rle_read(fp);
    fclose(fp);
    
    if (!img) {
        std::cerr << "Failed to read RLE file" << std::endl;
        return 1;
    }
    
    std::cout << "Success! Image: " << img->width << " x " << img->height 
              << " channels=" << img->channels << std::endl;
    
    // Check first few rows
    std::cout << "\nFirst 20 rows:" << std::endl;
    for (size_t y = 0; y < 20 && y < img->height; y++) {
        size_t nonzero = 0;
        for (size_t x = 0; x < img->width; x++) {
            for (size_t c = 0; c < img->channels; c++) {
                double val = img->data[(y * img->width + x) * img->channels + c];
                if (val != 0.0) {
                    nonzero++;
                }
            }
        }
        std::cout << "  Row " << y << ": " << nonzero << " non-zero values" << std::endl;
    }
    
    // Cleanup
    bu_free(img->data, "image data");
    bu_free(img, "image");
    
    return 0;
}
