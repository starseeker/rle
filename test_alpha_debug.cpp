#include "rle.hpp"
#include <cstdio>
#include <iostream>

extern "C" {
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);
}

int main() {
    // Create a small RGBA image
    icv_image_t img;
    img.magic = 0x6269666d;
    img.width = 4;
    img.height = 4;
    img.channels = 4;
    img.alpha_channel = 1;
    img.color_space = ICV_COLOR_SPACE_RGB;
    img.gamma_corr = 0.0;
    img.flags = 0;
    
    img.data = (double*)calloc(4 * 4 * 4, sizeof(double));
    for (int i = 0; i < 4 * 4; i++) {
        img.data[i * 4 + 0] = 1.0;  // R
        img.data[i * 4 + 1] = 0.0;  // G
        img.data[i * 4 + 2] = 0.0;  // B
        img.data[i * 4 + 3] = double(i) / 15.0;  // A - varying
    }
    
    std::cout << "Writing RGBA image...\n";
    FILE* fp = fopen("debug_alpha.rle", "wb");
    int result = rle_write(&img, fp);
    fclose(fp);
    std::cout << "Write result: " << result << "\n";
    
    std::cout << "Reading back...\n";
    fp = fopen("debug_alpha.rle", "rb");
    icv_image_t* loaded = rle_read(fp);
    fclose(fp);
    
    if (loaded) {
        std::cout << "Loaded image: " << loaded->width << "x" << loaded->height << "\n";
        std::cout << "Channels: " << loaded->channels << "\n";
        std::cout << "Alpha channel: " << loaded->alpha_channel << "\n";
        
        std::cout << "First few alpha values:\n";
        std::cout << "Original: ";
        for (int i = 0; i < 4; i++) {
            std::cout << img.data[i * 4 + 3] << " ";
        }
        std::cout << "\nLoaded: ";
        for (int i = 0; i < 4; i++) {
            std::cout << loaded->data[i * 4 + 3] << " ";
        }
        std::cout << "\n";
        
        bu_free(loaded->data, "data");
        bu_free(loaded, "img");
    } else {
        std::cout << "Failed to read image\n";
    }
    
    free(img.data);
    return 0;
}
