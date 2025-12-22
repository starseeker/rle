#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cmath>

int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

int main(int argc, char** argv) {
    const char* filename = (argc > 1) ? argv[1] : "teapot.rle";
    
    printf("Checking file: %s\n", filename);
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("ERROR: Could not open %s\n", filename);
        return 1;
    }
    
    icv_image_t* img = rle_read(fp);
    fclose(fp);
    
    if (!img) {
        printf("ERROR: Failed to read %s\n", filename);
        return 1;
    }
    
    printf("Dimensions: %zux%zu, channels: %zu\n", img->width, img->height, img->channels);
    
    size_t black_lines = 0;
    for (size_t y = 0; y < std::min<size_t>(20, img->height); y++) {
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
        printf("  Row %zu: %s\n", y, is_black ? "BLACK" : "DATA");
        if (is_black) black_lines++;
    }
    
    bu_free(img->data, "data");
    bu_free(img, "img");
    return 0;
}
