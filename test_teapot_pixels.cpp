#include "rle.hpp"
#include <iostream>

int main() {
    FILE* fp = fopen("teapot.rle", "rb");
    std::vector<uint8_t> data;
    uint32_t w, h;
    bool has_alpha;
    rle::Error err;
    
    bool ok = rle::read_rgb(fp, data, w, h, &has_alpha, nullptr, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Failed to read\n";
        return 1;
    }
    
    std::cout << "Read " << w << "x" << h << " image\n";
    std::cout << "First few pixels:\n";
    for (int i = 0; i < 10; i++) {
        std::cout << "  Pixel " << i << ": R=" << int(data[i*3+0]) 
                  << " G=" << int(data[i*3+1]) 
                  << " B=" << int(data[i*3+2]) << "\n";
    }
    
    return 0;
}
