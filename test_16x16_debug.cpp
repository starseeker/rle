#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    // Create 4x4 image for easier debugging
    const int W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3);
    
    std::cout << "Creating image with pattern:\n";
    for (int y = 0; y < H; y++) {
        std::cout << "  Row " << y << ": ";
        for (int x = 0; x < W; x++) {
            int idx = (y * W + x) * 3;
            data[idx + 0] = x * 64;      // R varies with x
            data[idx + 1] = y * 64;      // G varies with y (key for debugging)
            data[idx + 2] = 128;          // B constant
            std::cout << "G=" << int(data[idx+1]) << " ";
        }
        std::cout << "\n";
    }
    
    // Write
    FILE* fp = fopen("test_4x4.rle", "wb");
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Write failed: " << int(err) << "\n";
        return 1;
    }
    
    // Read
    std::vector<uint8_t> read_data;
    uint32_t rw, rh;
    bool has_alpha;
    
    fp = fopen("test_4x4.rle", "rb");
    ok = rle::read_rgb(fp, read_data, rw, rh, &has_alpha, nullptr, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Read failed: " << int(err) << "\n";
        return 1;
    }
    
    std::cout << "\nRead back:\n";
    for (uint32_t y = 0; y < rh; y++) {
        std::cout << "  Row " << y << ": ";
        for (uint32_t x = 0; x < rw; x++) {
            size_t idx = (y * rw + x) * 3;
            std::cout << "G=" << int(read_data[idx+1]) << " ";
        }
        std::cout << "\n";
    }
    
    return 0;
}
