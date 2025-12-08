#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    // Create 16x16 image with pattern
    const int W = 16, H = 16;
    std::vector<uint8_t> data(W * H * 3);
    
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int idx = (y * W + x) * 3;
            data[idx + 0] = (x * 16) % 256;  // R varies with x
            data[idx + 1] = (y * 16) % 256;  // G varies with y
            data[idx + 2] = 128;              // B constant
        }
    }
    
    // Write
    FILE* fp = fopen("test_16x16.rle", "wb");
    rle::Error err;
    std::vector<std::string> comments = {"Test"};
    std::vector<uint8_t> bg = {};
    
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Write failed\n";
        return 1;
    }
    
    // Read
    std::vector<uint8_t> read_data;
    uint32_t rw, rh;
    bool has_alpha;
    
    fp = fopen("test_16x16.rle", "rb");
    ok = rle::read_rgb(fp, read_data, rw, rh, &has_alpha, nullptr, err);
    fclose(fp);
    
    if (!ok) {
        std::cout << "Read failed\n";
        return 1;
    }
    
    std::cout << "Read " << rw << "x" << rh << " image\n";
    
    // Check some pixels
    bool all_good = true;
    int errors = 0;
    for (int y = 0; y < H && errors < 10; y++) {
        for (int x = 0; x < W && errors < 10; x++) {
            int idx = (y * W + x) * 3;
            uint8_t exp_r = (x * 16) % 256;
            uint8_t exp_g = (y * 16) % 256;
            uint8_t exp_b = 128;
            
            if (read_data[idx+0] != exp_r || read_data[idx+1] != exp_g || read_data[idx+2] != exp_b) {
                std::cout << "Mismatch at (" << x << "," << y << "): ";
                std::cout << "expected (" << int(exp_r) << "," << int(exp_g) << "," << int(exp_b) << ") ";
                std::cout << "got (" << int(read_data[idx+0]) << "," << int(read_data[idx+1]) << "," << int(read_data[idx+2]) << ")\n";
                all_good = false;
                errors++;
            }
        }
    }
    
    if (all_good) {
        std::cout << "All pixels match!\n";
    }
    
    return all_good ? 0 : 1;
}
