#include <iostream>
#include <cstring>
#include <cstdint>
extern "C" {
#include "rle.h"
#include "rle_put.h"
}

int main() {
    rle_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    
    FILE* fp = fopen("utah_2x2.rle", "rb");
    hdr.rle_file = fp;
    
    int result = rle_get_setup(&hdr);
    std::cout << "Setup result: " << result << "\n";
    std::cout << "Image: " << (hdr.xmax - hdr.xmin + 1) << "x" << (hdr.ymax - hdr.ymin + 1) << "\n";
    std::cout << "ncolors: " << hdr.ncolors << "\n";
    
    // Allocate scanline buffers
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(hdr.xmax - hdr.xmin + 1);
    }
    
    std::cout << "\nReading scanlines (bottom-to-top in file, displaying as read):\n";
    for (int y = hdr.ymin; y <= hdr.ymax; y++) {
        rle_getrow(&hdr, scanline);
        std::cout << "  y=" << y << ": ";
        for (int x = 0; x < hdr.xmax - hdr.xmin + 1; x++) {
            uint8_t r = scanline[0][x];
            uint8_t g = scanline[1][x];
            uint8_t b = scanline[2][x];
            
            if (r == 255 && g == 0 && b == 0) std::cout << "Red ";
            else if (r == 0 && g == 255 && b == 0) std::cout << "Green ";
            else if (r == 0 && g == 0 && b == 255) std::cout << "Blue ";
            else if (r == 255 && g == 255 && b == 0) std::cout << "Yellow ";
            else std::cout << "(" << int(r) << "," << int(g) << "," << int(b) << ") ";
        }
        std::cout << "\n";
    }
    
    for (int c = 0; c < 3; c++) {
        free(scanline[c]);
    }
    fclose(fp);
    
    return 0;
}
