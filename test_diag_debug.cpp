#include "rle.hpp"
#include <iostream>
#include <vector>

extern "C" {
int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
}

int main() {
    const size_t W = 4, H = 4;
    
    // Create simple diagonal pattern
    icv_image_t* img = (icv_image_t*)calloc(1, sizeof(icv_image_t));
    img->width = W;
    img->height = H;
    img->channels = 3;
    img->data = (double*)calloc(W * H * 3, sizeof(double));
    
    for (size_t y = 0; y < H; y++) {
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            bool stripe = ((x + y) / 2) % 2 == 0;
            img->data[idx + 0] = stripe ? 1.0 : 0.2;
            img->data[idx + 1] = stripe ? 0.8 : 0.3;
            img->data[idx + 2] = stripe ? 0.6 : 0.4;
        }
    }
    
    std::cout << "Original pattern:\n";
    for (size_t y = 0; y < H; y++) {
        std::cout << "  Row " << y << ": ";
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            std::cout << "(" << img->data[idx] << "," << img->data[idx+1] << "," << img->data[idx+2] << ") ";
        }
        std::cout << "\n";
    }
    
    // Write
    FILE* fp = fopen("diag_debug.rle", "wb");
    int result = rle_write(img, fp);
    fclose(fp);
    std::cout << "Write result: " << result << "\n";
    
    // Read
    fp = fopen("diag_debug.rle", "rb");
    icv_image_t* read = rle_read(fp);
    fclose(fp);
    
    if (read) {
        std::cout << "\nRead back pattern:\n";
        for (size_t y = 0; y < H; y++) {
            std::cout << "  Row " << y << ": ";
            for (size_t x = 0; x < W; x++) {
                size_t idx = (y * W + x) * 3;
                std::cout << "(" << read->data[idx] << "," << read->data[idx+1] << "," << read->data[idx+2] << ") ";
            }
            std::cout << "\n";
        }
    }
    
    return 0;
}
