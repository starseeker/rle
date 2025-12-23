#include "rle.hpp"
#include <iostream>
#include <cstdio>

extern icv_image_t* rle_read(FILE *fp);
extern void bu_free(void *ptr, const char *str);

int main() {
    FILE* fp = fopen("lenna.rle", "rb");
    if (!fp) {
        std::cerr << "Failed to open lenna.rle" << std::endl;
        return 1;
    }
    
    icv_image_t* img = rle_read(fp);
    fclose(fp);
    
    if (!img) {
        std::cerr << "Failed to read RLE file" << std::endl;
        return 1;
    }
    
    std::cout << "Image loaded: " << img->width << " x " << img->height << std::endl;
    
    // Check first few rows
    std::cout << "First 10 rows average:" << std::endl;
    for (int y = 0; y < 10; y++) {
        double avg = 0.0;
        for (size_t x = 0; x < img->width; x++) {
            for (size_t c = 0; c < img->channels; c++) {
                avg += img->data[(y * img->width + x) * img->channels + c];
            }
        }
        avg /= (img->width * img->channels);
        std::cout << "  Row " << y << ": " << avg << std::endl;
    }
    
    bu_free(img->data, "image data");
    bu_free(img, "image");
    
    return 0;
}
