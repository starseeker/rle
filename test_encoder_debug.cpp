#include "rle.hpp"
#include <iostream>
#include <vector>

// Minimal encoder with debug output
int main() {
    const int W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3);
    
    // Create test pattern
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int idx = (y * W + x) * 3;
            data[idx + 0] = 128;
            data[idx + 1] = y * 64;  // G varies with row
            data[idx + 2] = 64;
        }
    }
    
    std::cout << "Memory layout (row, G value):\n";
    for (int y = 0; y < H; y++) {
        int idx = (y * W) * 3;
        std::cout << "  Row " << y << ": G=" << int(data[idx + 1]) << "\n";
    }
    
    // Now let's check what pixel(x, y) returns for our Image structure
    rle::Header h;
    h.xpos = 0; h.ypos = 0;
    h.xlen = W; h.ylen = H;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.ncmap = 0; h.cmaplen = 0;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    rle::Image img;
    img.header = h;
    rle::Error err;
    img.allocate(err);
    
    // Copy data
    std::memcpy(img.pixels.data(), data.data(), W * H * 3);
    
    std::cout << "\nAccessing via img.pixel(x, y):\n";
    for (uint32_t y = 0; y < H; y++) {
        const uint8_t* p = img.pixel(0, y);  // Get first pixel of row y
        std::cout << "  img.pixel(0, " << y << "): G=" << int(p[1]) << "\n";
    }
    
    std::cout << "\nConclusion: If pixel(0, y) matches memory row y, then the issue\n";
    std::cout << "is not in pixel() but somewhere else in the encode/decode chain.\n";
    
    return 0;
}
