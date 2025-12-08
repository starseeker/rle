#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    // Create RGB data: 2x2 image
    std::vector<uint8_t> rgb = {
        255, 0, 0,      // Red
        0, 255, 0,      // Green
        0, 0, 255,      // Blue  
        255, 255, 0     // Yellow
    };
    
    // Write using rle.hpp API
    rle::Error err;
    std::vector<std::string> comments = {"Test"};
    std::vector<uint8_t> bg = {};
    
    FILE* fp = fopen("minimal_rgb.rle", "wb");
    bool ok = rle::write_rgb(fp, rgb.data(), 2, 2, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    std::cout << "Write: " << (ok ? "OK" : "FAILED") << "\n";
    
    // Read back
    std::vector<uint8_t> read_data;
    uint32_t w, h;
    bool has_alpha;
    
    fp = fopen("minimal_rgb.rle", "rb");
    ok = rle::read_rgb(fp, read_data, w, h, &has_alpha, nullptr, err);
    fclose(fp);
    
    std::cout << "Read: " << (ok ? "OK" : "FAILED") << "\n";
    std::cout << "Size: " << w << "x" << h << "\n";
    
    if (ok && read_data.size() >= 12) {
        std::cout << "Original RGB values:\n";
        for (size_t i = 0; i < 4; i++) {
            std::cout << "  Pixel " << i << ": R=" << int(rgb[i*3+0]) 
                      << " G=" << int(rgb[i*3+1]) 
                      << " B=" << int(rgb[i*3+2]) << "\n";
        }
        std::cout << "Read RGB values:\n";
        for (size_t i = 0; i < 4; i++) {
            std::cout << "  Pixel " << i << ": R=" << int(read_data[i*3+0]) 
                      << " G=" << int(read_data[i*3+1]) 
                      << " B=" << int(read_data[i*3+2]) << "\n";
        }
    }
    
    return 0;
}
