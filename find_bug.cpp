#include "rle.hpp"
#include <iostream>
#include <vector>

int main() {
    const int W = 4, H = 4;
    std::vector<uint8_t> data(W * H * 3);
    
    // Create test pattern - each row has unique G value
    std::cout << "Creating input data:\n";
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int idx = (y * W + x) * 3;
            data[idx + 0] = 128;
            data[idx + 1] = y * 64;  // Unique per row
            data[idx + 2] = 64;
        }
        int idx = (y * W) * 3;
        std::cout << "  Input row " << y << " (index " << idx << "): G=" << int(data[idx + 1]) << "\n";
    }
    
    // Write
    FILE* fp = fopen("debug.rle", "wb");
    rle::Error err;
    std::vector<std::string> comments = {};
    std::vector<uint8_t> bg = {};
    
    // Manually create Image to inspect
    rle::Header h;
    h.xpos = 0; h.ypos = 0;
    h.xlen = W; h.ylen = H;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.ncmap = 0; h.cmaplen = 0;
    h.flags = rle::FLAG_NO_BACKGROUND;
    
    rle::Image img;
    img.header = h;
    img.allocate(err);
    std::memcpy(img.pixels.data(), data.data(), W * H * 3);
    
    std::cout << "\nAfter copying to Image.pixels:\n";
    for (uint32_t y = 0; y < H; y++) {
        const uint8_t* p = img.pixel(0, y);
        std::cout << "  img.pixel(0, " << y << "): G=" << int(p[1]) << "\n";
    }
    
    // Write
    rle::Encoder::write(fp, img, rle::Encoder::BG_SAVE_ALL, err);
    fclose(fp);
    
    // Read back
    std::cout << "\nReading back:\n";
    fp = fopen("debug.rle", "rb");
    rle::Image img2;
    rle::DecoderResult dr = rle::Decoder::read(fp, img2);
    fclose(fp);
    
    if (dr.ok) {
        std::cout << "Decoder succeeded, checking pixels:\n";
        for (uint32_t y = 0; y < H; y++) {
            const uint8_t* p = img2.pixel(0, y);
            std::cout << "  img2.pixel(0, " << y << "): G=" << int(p[1]) << "\n";
        }
        
        std::cout << "\nRaw img2.pixels data:\n";
        for (int y = 0; y < H; y++) {
            int idx = (y * W) * 3;
            std::cout << "  Index " << idx << " (row " << y << "): G=" << int(img2.pixels[idx + 1]) << "\n";
        }
    } else {
        std::cout << "Decoder failed: " << int(dr.error) << "\n";
    }
    
    return 0;
}
