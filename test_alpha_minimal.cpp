#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    // Create RGBA data: 2x2 image with specific alpha values
    std::vector<uint8_t> rgba = {
        255, 0, 0, 128,  // Red, 50% transparent
        0, 255, 0, 192,  // Green, 75% transparent
        0, 0, 255, 64,   // Blue, 25% transparent
        255, 255, 0, 255 // Yellow, opaque
    };
    
    // Write using rle.hpp API
    rle::Error err;
    std::vector<std::string> comments = {"Test"};
    std::vector<uint8_t> bg = {};
    
    FILE* fp = fopen("minimal_alpha.rle", "wb");
    bool ok = rle::write_rgb(fp, rgba.data(), 2, 2, comments, bg, true, 
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    std::cout << "Write: " << (ok ? "OK" : "FAILED") << " err=" << int(err) << "\n";
    
    // Read back
    std::vector<uint8_t> read_data;
    uint32_t w, h;
    bool has_alpha;
    
    fp = fopen("minimal_alpha.rle", "rb");
    ok = rle::read_rgb(fp, read_data, w, h, &has_alpha, nullptr, err);
    fclose(fp);
    
    std::cout << "Read: " << (ok ? "OK" : "FAILED") << " err=" << int(err) << "\n";
    std::cout << "Size: " << w << "x" << h << " has_alpha=" << has_alpha << "\n";
    std::cout << "Data size: " << read_data.size() << "\n";
    
    if (ok && read_data.size() >= 16) {
        std::cout << "Original RGBA values:\n";
        for (size_t i = 0; i < 4; i++) {
            std::cout << "  Pixel " << i << ": R=" << int(rgba[i*4+0]) 
                      << " G=" << int(rgba[i*4+1]) 
                      << " B=" << int(rgba[i*4+2]) 
                      << " A=" << int(rgba[i*4+3]) << "\n";
        }
        std::cout << "Read RGBA values:\n";
        for (size_t i = 0; i < 4; i++) {
            std::cout << "  Pixel " << i << ": R=" << int(read_data[i*4+0]) 
                      << " G=" << int(read_data[i*4+1]) 
                      << " B=" << int(read_data[i*4+2]) 
                      << " A=" << int(read_data[i*4+3]) << "\n";
        }
    }
    
    return 0;
}
