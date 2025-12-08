#include "rle.hpp"
#include <iostream>
#include <vector>
#include <cstring>
extern "C" {
#include "rle.h"
}

int main() {
    if (!fopen("teapot.rle", "rb")) {
        std::cout << "teapot.rle not found\n";
        return 1;
    }
    
    // Read with our implementation
    std::cout << "Reading teapot.rle with our implementation:\n";
    FILE* fp = fopen("teapot.rle", "rb");
    std::vector<uint8_t> our_data;
    uint32_t our_w, our_h;
    bool has_alpha;
    rle::Error err;
    
    if (rle::read_rgb(fp, our_data, our_w, our_h, &has_alpha, nullptr, err)) {
        std::cout << "  Size: " << our_w << "x" << our_h << "\n";
        std::cout << "  Row 0, pixel 0: R=" << int(our_data[0]) << " G=" << int(our_data[1]) << " B=" << int(our_data[2]) << "\n";
        std::cout << "  Row 0, pixel 1: R=" << int(our_data[3]) << " G=" << int(our_data[4]) << " B=" << int(our_data[5]) << "\n";
        std::cout << "  Row 1, pixel 0: R=" << int(our_data[our_w*3+0]) << " G=" << int(our_data[our_w*3+1]) << " B=" << int(our_data[our_w*3+2]) << "\n";
    }
    fclose(fp);
    
    // Read with Utah RLE
    std::cout << "\nReading teapot.rle with Utah RLE:\n";
    rle_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    fp = fopen("teapot.rle", "rb");
    hdr.rle_file = fp;
    
    if (rle_get_setup(&hdr) == 0) {
        std::cout << "  Size: " << (hdr.xmax - hdr.xmin + 1) << "x" << (hdr.ymax - hdr.ymin + 1) << "\n";
        std::cout << "  ymin=" << hdr.ymin << ", ymax=" << hdr.ymax << "\n";
        
        int w = hdr.xmax - hdr.xmin + 1;
        rle_pixel* scanline[3];
        for (int c = 0; c < 3; c++) {
            scanline[c] = (rle_pixel*)malloc(w);
        }
        
        // Read first scanline (y=ymin)
        rle_getrow(&hdr, scanline);
        std::cout << "  Scanline y=" << hdr.ymin << ", pixel 0: R=" << int(scanline[0][0]) 
                  << " G=" << int(scanline[1][0]) << " B=" << int(scanline[2][0]) << "\n";
        std::cout << "  Scanline y=" << hdr.ymin << ", pixel 1: R=" << int(scanline[0][1]) 
                  << " G=" << int(scanline[1][1]) << " B=" << int(scanline[2][1]) << "\n";
        
        // Read second scanline
        rle_getrow(&hdr, scanline);
        std::cout << "  Scanline y=" << (hdr.ymin+1) << ", pixel 0: R=" << int(scanline[0][0]) 
                  << " G=" << int(scanline[1][0]) << " B=" << int(scanline[2][0]) << "\n";
        
        for (int c = 0; c < 3; c++) {
            free(scanline[c]);
        }
    }
    fclose(fp);
    
    std::cout << "\nIf the pixel values match, then both implementations\n";
    std::cout << "are using the same Y-coordinate convention.\n";
    
    return 0;
}
