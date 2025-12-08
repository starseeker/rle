#include <cstdio>
#include <cstring>
#include <cstdint>
#include <iostream>
extern "C" {
#include "rle.h"
#include "rle_put.h"
}

int main() {
    // Write a 4x4 test image with Utah RLE where each row has a unique G value
    rle_hdr out_hdr;
    memset(&out_hdr, 0, sizeof(out_hdr));
    out_hdr.xmin = 0;
    out_hdr.ymin = 0;
    out_hdr.xmax = 3;  // 4 pixels wide
    out_hdr.ymax = 3;  // 4 pixels tall
    out_hdr.ncolors = 3;
    out_hdr.alpha = 0;
    out_hdr.background = 0;
    
    FILE* fp = fopen("utah_test.rle", "wb");
    out_hdr.rle_file = fp;
    rle_put_setup(&out_hdr);
    
    // Write scanlines - Utah RLE writes from ymin to ymax
    // Let's mark each scanline with a unique G value
    std::cout << "Writing with Utah RLE (ymin=0 to ymax=3):\n";
    for (int y = 0; y <= 3; y++) {
        rle_pixel* scanline[3];
        for (int c = 0; c < 3; c++) {
            scanline[c] = (rle_pixel*)malloc(4);
        }
        
        // Fill scanline with R=128, G=y*64, B=64
        for (int x = 0; x < 4; x++) {
            scanline[0][x] = 128;      // R
            scanline[1][x] = y * 64;   // G - unique per scanline
            scanline[2][x] = 64;       // B
        }
        
        std::cout << "  Writing scanline y=" << y << " with G=" << (y*64) << "\n";
        rle_putrow(scanline, 4, &out_hdr);
        
        for (int c = 0; c < 3; c++) {
            free(scanline[c]);
        }
    }
    
    rle_puteof(&out_hdr);
    fclose(fp);
    
    // Now read it back with Utah RLE
    std::cout << "\nReading back with Utah RLE:\n";
    rle_hdr in_hdr;
    memset(&in_hdr, 0, sizeof(in_hdr));
    fp = fopen("utah_test.rle", "rb");
    in_hdr.rle_file = fp;
    rle_get_setup(&in_hdr);
    
    std::cout << "Header: " << (in_hdr.xmax - in_hdr.xmin + 1) << "x" 
              << (in_hdr.ymax - in_hdr.ymin + 1) << "\n";
    std::cout << "ymin=" << in_hdr.ymin << ", ymax=" << in_hdr.ymax << "\n";
    
    rle_pixel* scanline[3];
    for (int c = 0; c < 3; c++) {
        scanline[c] = (rle_pixel*)malloc(4);
    }
    
    for (int y = in_hdr.ymin; y <= in_hdr.ymax; y++) {
        rle_getrow(&in_hdr, scanline);
        std::cout << "  Read scanline y=" << y << " with G=" << int(scanline[1][0]) << "\n";
    }
    
    for (int c = 0; c < 3; c++) {
        free(scanline[c]);
    }
    fclose(fp);
    
    std::cout << "\nConclusion: If G values match the y coordinate, then Utah RLE stores\n";
    std::cout << "scanline y=0 first and y=3 last (top-to-bottom in file order).\n";
    
    return 0;
}
