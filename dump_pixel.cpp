#include "rle.hpp"
#include <cstdio>

// Declare external function
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

int main(int argc, char** argv) {
    if (argc < 4) {
        printf("Usage: %s <rle-file> <x> <y>\n", argv[0]);
        return 1;
    }
    
    const char* filename = argv[1];
    int x = atoi(argv[2]);
    int y = atoi(argv[3]);
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Failed to open: %s\n", filename);
        return 1;
    }
    
    icv_image_t* img = rle_read(fp);
    fclose(fp);
    
    if (!img) {
        printf("Failed to read RLE\n");
        return 1;
    }
    
    if (x < 0 || x >= (int)img->width || y < 0 || y >= (int)img->height) {
        printf("Pixel (%d, %d) out of bounds (image is %zux%zu)\n", 
               x, y, img->width, img->height);
        bu_free(img->data, "image data");
        bu_free(img, "image");
        return 1;
    }
    
    size_t idx = (y * img->width + x) * img->channels;
    printf("Pixel (%d, %d) in %s:\n", x, y, filename);
    printf("  Double values: R=%.10f G=%.10f B=%.10f",
           img->data[idx], img->data[idx+1], img->data[idx+2]);
    if (img->channels > 3) {
        printf(" A=%.10f", img->data[idx+3]);
    }
    printf("\n");
    
    printf("  As uint8: R=%d G=%d B=%d",
           (int)(img->data[idx] * 255.0 + 0.5),
           (int)(img->data[idx+1] * 255.0 + 0.5),
           (int)(img->data[idx+2] * 255.0 + 0.5));
    if (img->channels > 3) {
        printf(" A=%d", (int)(img->data[idx+3] * 255.0 + 0.5));
    }
    printf("\n");
    
    bu_free(img->data, "image data");
    bu_free(img, "image");
    
    return 0;
}
