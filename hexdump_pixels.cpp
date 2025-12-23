// Hexdump-style viewer for RLE pixel data
// Shows raw pixel bytes to detect any ordering issues
#include "rle.hpp"
#include <cstdio>
#include <cstdarg>

extern "C" {
void* bu_calloc(size_t nelem, size_t elsize, const char*) {
    return calloc(nelem, elsize);
}
void bu_free(void* ptr, const char*) {
    if (ptr) {
        *((uint32_t *)ptr) = 0xFFFFFFFF;
        free(ptr);
    }
}
int bu_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
    return 0;
}
}

void hexdump_pixels(const std::vector<uint8_t>& pixels, size_t width, size_t height, size_t channels,
                    size_t start_y, size_t start_x, size_t num_rows, size_t num_cols) {
    printf("\n=== Pixel Hexdump ===\n");
    printf("Starting at pixel (%zu, %zu), %zu rows x %zu cols\n", start_x, start_y, num_rows, num_cols);
    printf("Channels: %zu\n\n", channels);
    
    for (size_t y = start_y; y < start_y + num_rows && y < height; y++) {
        printf("Row %3zu: ", y);
        for (size_t x = start_x; x < start_x + num_cols && x < width; x++) {
            size_t idx = (y * width + x) * channels;
            printf("[");
            for (size_t c = 0; c < channels; c++) {
                printf("%02X", pixels[idx + c]);
                if (c < channels - 1) printf(" ");
            }
            printf("] ");
        }
        printf("\n");
    }
    
    printf("\nFormat: [RR GG BB AA] for RGBA, [RR GG BB] for RGB\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.rle> [start_y] [start_x] [rows] [cols]\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s\n", argv[1]);
        return 1;
    }

    printf("Decoding %s (RAW)...\n", argv[1]);
    
    rle::Image img;
    rle::DecoderResult res = rle::Decoder::read(fp, img);
    fclose(fp);

    if (!res.ok) {
        fprintf(stderr, "Failed to decode: %s\n", rle::error_string(res.error));
        return 1;
    }

    printf("Successfully decoded: %ux%u, %u channels\n",
           img.header.width(), img.header.height(), img.header.channels());
    
    size_t start_y = (argc > 2) ? atoi(argv[2]) : 1;
    size_t start_x = (argc > 3) ? atoi(argv[3]) : 100;
    size_t num_rows = (argc > 4) ? atoi(argv[4]) : 10;
    size_t num_cols = (argc > 5) ? atoi(argv[5]) : 10;
    
    hexdump_pixels(img.pixels, img.header.width(), img.header.height(), img.header.channels(),
                   start_y, start_x, num_rows, num_cols);
    
    return 0;
}
