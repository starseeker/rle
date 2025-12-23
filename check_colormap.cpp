#include "rle.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rle_file>\n", argv[0]);
        return 1;
    }
    
    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", argv[1]);
        return 1;
    }
    
    rle::Image img;
    auto result = rle::Decoder::read(f, img);
    fclose(f);
    
    if (!result.ok) {
        fprintf(stderr, "Failed to decode: %s\n", rle::error_string(result.error));
        return 1;
    }
    
    printf("File: %s\n", argv[1]);
    printf("Dimensions: %ux%u\n", img.header.width(), img.header.height());
    printf("Channels: %u (ncolors=%u, alpha=%d)\n", img.header.channels(), img.header.ncolors, img.header.has_alpha());
    printf("Colormap entries: ncmap=%u, cmaplen=%u\n", img.header.ncmap, img.header.cmaplen);
    
    if (img.header.ncmap > 0) {
        size_t total = img.header.ncmap * (1 << img.header.cmaplen);
        printf("Total colormap values: %zu (should be %u * %u = %u)\n", 
               img.header.colormap.size(), img.header.ncmap, (1 << img.header.cmaplen), 
               img.header.ncmap * (1 << img.header.cmaplen));
        
        // Show first few colormap entries
        printf("First 10 colormap entries:\n");
        size_t map_len = (1 << img.header.cmaplen);
        for (size_t i = 0; i < 10 && i < map_len; i++) {
            if (img.header.ncmap == 1) {
                printf("  [%zu] = %u\n", i, img.header.colormap[i] >> 8);
            } else if (img.header.ncmap == 3) {
                printf("  [%zu] R=%u G=%u B=%u\n", i, 
                       img.header.colormap[i] >> 8,
                       img.header.colormap[map_len + i] >> 8,
                       img.header.colormap[2*map_len + i] >> 8);
            }
        }
        
        // Show first pixel values (raw, before colormap)
        printf("\nFirst 5 pixel values (RAW, before colormap application):\n");
        for (int i = 0; i < 5 && i < img.header.width(); i++) {
            const uint8_t* p = img.pixel(i, 0);
            if (img.header.ncolors == 3) {
                printf("  [%d] R=%u G=%u B=%u\n", i, p[0], p[1], p[2]);
            } else {
                printf("  [%d] = %u\n", i, p[0]);
            }
        }
    }
    
    return 0;
}
