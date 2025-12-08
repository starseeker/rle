#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    const size_t W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3);
    
    // Simple gradient: G varies with Y
    std::cout << "Original data:\n";
    for (size_t y = 0; y < H; y++) {
        std::cout << "  Row " << y << ": ";
        for (size_t x = 0; x < W; x++) {
            size_t idx = (y * W + x) * 3;
            data[idx + 0] = 0;
            data[idx + 1] = (y * 255) / (H - 1);
            data[idx + 2] = 0;
            std::cout << "G=" << int(data[idx + 1]) << " ";
        }
        std::cout << "\n";
    }
    
    // Write with our implementation
    FILE* fp = fopen("simple.rle", "wb");
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    bool ok = rle::write_rgb(fp, data.data(), W, H, comments, bg, false,
                              rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    std::cout << "\nWrite result: " << (ok ? "OK" : "FAILED") << "\n";
    
    // Read back
    std::vector<uint8_t> readback;
    uint32_t rw, rh;
    bool has_alpha;
    
    fp = fopen("simple.rle", "rb");
    ok = rle::read_rgb(fp, readback, rw, rh, &has_alpha, nullptr, err);
    fclose(fp);
    
    std::cout << "Read result: " << (ok ? "OK" : "FAILED") << "\n";
    std::cout << "\nRead back data:\n";
    for (size_t y = 0; y < rh; y++) {
        std::cout << "  Row " << y << ": ";
        for (size_t x = 0; x < rw; x++) {
            size_t idx = (y * rw + x) * 3;
            std::cout << "G=" << int(readback[idx + 1]) << " ";
        }
        std::cout << "\n";
    }
    
    // Check if they match
    bool match = true;
    for (size_t i = 0; i < data.size(); i++) {
        if (data[i] != readback[i]) {
            match = false;
            break;
        }
    }
    
    std::cout << "\nResult: " << (match ? "MATCH" : "MISMATCH") << "\n";
    
    return match ? 0 : 1;
}
