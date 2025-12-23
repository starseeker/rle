#include "rle.hpp"
#include <cstdio>
#include <cstdlib>

// Simple PPM reader
bool read_ppm(const char* filename, std::vector<uint8_t>& pixels, int& width, int& height) {
    FILE* f = fopen(filename, "rb");
    if (!f) return false;
    
    char magic[3];
    if (fscanf(f, "%2s", magic) != 1 || (magic[0] != 'P' || (magic[1] != '6' && magic[1] != '3'))) {
        fclose(f);
        return false;
    }
    
    // Skip comments
    int c;
    while ((c = fgetc(f)) == '#') {
        while ((c = fgetc(f)) != '\n' && c != EOF);
    }
    ungetc(c, f);
    
    int maxval;
    if (fscanf(f, "%d %d %d", &width, &height, &maxval) != 3) {
        fclose(f);
        return false;
    }
    
    // Skip one whitespace
    fgetc(f);
    
    pixels.resize(width * height * 3);
    
    if (magic[1] == '6') {
        // Binary PPM
        if (maxval == 255) {
            // 8-bit
            if (fread(pixels.data(), 1, pixels.size(), f) != pixels.size()) {
                fclose(f);
                return false;
            }
        } else if (maxval == 65535) {
            // 16-bit - read and scale down
            for (size_t i = 0; i < pixels.size(); i++) {
                uint16_t hi = fgetc(f);
                uint16_t lo = fgetc(f);
                uint16_t val = (hi << 8) | lo;
                pixels[i] = val >> 8; // Take high byte
            }
        } else {
            fclose(f);
            return false;
        }
    }
    
    fclose(f);
    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rle_file> [ppm_file]\n", argv[0]);
        return 1;
    }
    
    const char* rle_file = argv[1];
    const char* ppm_file = argc > 2 ? argv[2] : nullptr;
    
    // Read RLE file
    FILE* f = fopen(rle_file, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open %s\n", rle_file);
        return 1;
    }
    
    rle::Image img;
    auto result = rle::Decoder::read(f, img);
    fclose(f);
    
    if (!result.ok) {
        fprintf(stderr, "Failed to decode RLE: %s\n", rle::error_string(result.error));
        return 1;
    }
    
    printf("RLE file: %s\n", rle_file);
    printf("  Dimensions: %ux%u, channels=%u\n", img.header.width(), img.header.height(), img.header.channels());
    printf("  Colormap: ncmap=%u, cmaplen=%u\n", img.header.ncmap, img.header.cmaplen);
    
    // Show first few pixel values from RLE
    printf("  First 5 RLE pixels:\n");
    for (int i = 0; i < 5 && i < img.header.width(); i++) {
        const uint8_t* p = img.pixel(i, 0);
        if (img.header.ncolors >= 3) {
            printf("    [%d] R=%u G=%u B=%u", i, p[0], p[1], p[2]);
            if (img.header.has_alpha()) printf(" A=%u", p[3]);
            printf("\n");
        }
    }
    
    // If PPM file provided, compare
    if (ppm_file) {
        std::vector<uint8_t> ppm_pixels;
        int ppm_w, ppm_h;
        if (!read_ppm(ppm_file, ppm_pixels, ppm_w, ppm_h)) {
            fprintf(stderr, "Failed to read PPM file: %s\n", ppm_file);
            return 1;
        }
        
        printf("\nPPM file: %s\n", ppm_file);
        printf("  Dimensions: %dx%d\n", ppm_w, ppm_h);
        
        // Show first few pixel values from PPM
        printf("  First 5 PPM pixels:\n");
        for (int i = 0; i < 5 && i < ppm_w; i++) {
            const uint8_t* p = &ppm_pixels[i * 3];
            printf("    [%d] R=%u G=%u B=%u\n", i, p[0], p[1], p[2]);
        }
        
        // Compare dimensions
        if (ppm_w != img.header.width() || ppm_h != img.header.height()) {
            printf("\nWARNING: Dimensions don't match!\n");
        }
        
        // Compare pixels
        size_t diff_count = 0;
        size_t total = std::min(size_t(ppm_w * ppm_h), size_t(img.header.width() * img.header.height()));
        
        for (size_t y = 0; y < std::min(size_t(ppm_h), size_t(img.header.height())); y++) {
            for (size_t x = 0; x < std::min(size_t(ppm_w), size_t(img.header.width())); x++) {
                const uint8_t* rle_p = img.pixel(x, y);
                const uint8_t* ppm_p = &ppm_pixels[(y * ppm_w + x) * 3];
                
                for (int c = 0; c < 3 && c < img.header.ncolors; c++) {
                    if (rle_p[c] != ppm_p[c]) {
                        diff_count++;
                        break;
                    }
                }
            }
        }
        
        printf("\nComparison: %zu/%zu pixels differ (%.1f%%)\n", 
               diff_count, total, 100.0 * diff_count / total);
    }
    
    return 0;
}
