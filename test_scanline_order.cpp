#include <cstdio>
#include <cstdint>
#include <iostream>
#include <cstring>
extern "C" {
#include "rle.h"
#include "rle_put.h"
}

int main() {
    // Create a 2x2 image with Utah RLE where we know the order
    rle_hdr hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.xmin = 0;
    hdr.ymin = 0;
    hdr.xmax = 1;  // 2 pixels wide (xmin to xmax inclusive)
    hdr.ymax = 1;  // 2 pixels tall
    hdr.ncolors = 3;
    hdr.alpha = 0;
    hdr.background = 0;
    hdr.rle_file = stdout;
    
    FILE* fp = fopen("utah_2x2.rle", "wb");
    hdr.rle_file = fp;
    rle_put_setup(&hdr);
    
    // Utah RLE writes bottom-to-top, so:
    // Row at y=0 (bottom) should be written first
    // Row at y=1 (top) should be written second
    
    // Allocate scanline buffers
    rle_pixel* scanline0[3];
    rle_pixel* scanline1[3];
    for (int c = 0; c < 3; c++) {
        scanline0[c] = (rle_pixel*)malloc(2);
        scanline1[c] = (rle_pixel*)malloc(2);
    }
    
    // Write y=0 (bottom row): Red, Green
    scanline0[0][0] = 255; scanline0[1][0] = 0;   scanline0[2][0] = 0;    // Red
    scanline0[0][1] = 0;   scanline0[1][1] = 255; scanline0[2][1] = 0;    // Green
    rle_putrow(scanline0, 2, &hdr);
    
    // Write y=1 (top row): Blue, Yellow
    scanline1[0][0] = 0;   scanline1[1][0] = 0;   scanline1[2][0] = 255;  // Blue
    scanline1[0][1] = 255; scanline1[1][1] = 255; scanline1[2][1] = 0;    // Yellow
    rle_putrow(scanline1, 2, &hdr);
    
    rle_puteof(&hdr);
    fclose(fp);
    
    for (int c = 0; c < 3; c++) {
        free(scanline0[c]);
        free(scanline1[c]);
    }
    
    std::cout << "Created utah_2x2.rle with known pattern:\n";
    std::cout << "  y=0 (bottom): Red, Green\n";
    std::cout << "  y=1 (top):    Blue, Yellow\n";
    std::cout << "\nIn top-to-bottom memory layout this should be:\n";
    std::cout << "  Row 0 (top):    Blue, Yellow\n";
    std::cout << "  Row 1 (bottom): Red, Green\n";
    
    return 0;
}
