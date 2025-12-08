#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    // Read the Utah RLE file
    FILE* fp = fopen("utah_2x2.rle", "rb");
    if (!fp) {
        std::cout << "Failed to open file\n";
        return 1;
    }
    
    std::vector<uint8_t> data;
    uint32_t w, h;
    bool has_alpha;
    rle::Error err;
    
    bool ok = rle::read_rgb(fp, data, w, h, &has_alpha, nullptr, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Failed to read: " << int(err) << "\n";
        return 1;
    }
    
    std::cout << "Read " << w << "x" << h << " image\n";
    std::cout << "Pixels in memory (top-to-bottom, left-to-right):\n";
    for (uint32_t y = 0; y < h; y++) {
        std::cout << "  Row " << y << ": ";
        for (uint32_t x = 0; x < w; x++) {
            size_t idx = (y * w + x) * 3;
            uint8_t r = data[idx + 0];
            uint8_t g = data[idx + 1];
            uint8_t b = data[idx + 2];
            
            if (r == 255 && g == 0 && b == 0) std::cout << "Red ";
            else if (r == 0 && g == 255 && b == 0) std::cout << "Green ";
            else if (r == 0 && g == 0 && b == 255) std::cout << "Blue ";
            else if (r == 255 && g == 255 && b == 0) std::cout << "Yellow ";
            else std::cout << "(" << int(r) << "," << int(g) << "," << int(b) << ") ";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nExpected (top-to-bottom):\n";
    std::cout << "  Row 0: Blue Yellow\n";
    std::cout << "  Row 1: Red Green\n";
    
    return 0;
}
