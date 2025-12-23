// Decode RLE and show row statistics WITHOUT applying any pattern fixes
// This bypasses the detect_and_fix_alternating_pattern function
#include "rle.hpp"
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <vector>
#include <cmath>

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

void analyze_rows(const std::vector<uint8_t>& pixels, size_t width, size_t height, size_t channels) {
    printf("\n=== Row Analysis (first 50 rows) ===\n");
    
    for (size_t y = 0; y < std::min(height, size_t(50)); y++) {
        // Calculate average RGB for this row
        double r_sum = 0, g_sum = 0, b_sum = 0;
        size_t color_channels = std::min(channels, size_t(3));
        
        for (size_t x = 0; x < width; x++) {
            size_t idx = (y * width + x) * channels;
            r_sum += pixels[idx];
            if (color_channels > 1) g_sum += pixels[idx + 1];
            if (color_channels > 2) b_sum += pixels[idx + 2];
        }
        
        double r_avg = r_sum / width;
        double g_avg = (color_channels > 1) ? (g_sum / width) : 0;
        double b_avg = (color_channels > 2) ? (b_sum / width) : 0;
        
        // Check if row is uniform
        bool uniform = true;
        uint8_t first_r = pixels[(y * width) * channels];
        uint8_t first_g = (color_channels > 1) ? pixels[(y * width) * channels + 1] : 0;
        uint8_t first_b = (color_channels > 2) ? pixels[(y * width) * channels + 2] : 0;
        
        for (size_t x = 1; x < width && uniform; x++) {
            size_t idx = (y * width + x) * channels;
            if (pixels[idx] != first_r) uniform = false;
            if (color_channels > 1 && pixels[idx + 1] != first_g) uniform = false;
            if (color_channels > 2 && pixels[idx + 2] != first_b) uniform = false;
        }
        
        printf("Row %3zu: R=%6.1f G=%6.1f B=%6.1f", y, r_avg, g_avg, b_avg);
        if (uniform) {
            printf(" [UNIFORM: %u,%u,%u]", first_r, first_g, first_b);
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.rle>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s\n", argv[1]);
        return 1;
    }

    printf("Decoding %s (RAW - no pattern fixes)...\n", argv[1]);
    
    rle::Image img;
    rle::DecoderResult res = rle::Decoder::read(fp, img);
    fclose(fp);

    if (!res.ok) {
        fprintf(stderr, "Failed to decode: %s\n", rle::error_string(res.error));
        return 1;
    }

    printf("Successfully decoded: %ux%u, %u channels, alpha=%s\n",
           img.header.width(), img.header.height(),
           img.header.channels(),
           img.header.has_alpha() ? "yes" : "no");
    
    analyze_rows(img.pixels, img.header.width(), img.header.height(), img.header.channels());
    
    // Also check for vertical patterns
    printf("\n=== Column Analysis (first 50 columns, middle rows) ===\n");
    size_t mid_row_start = img.header.height() / 3;
    size_t mid_row_end = std::min((2 * img.header.height()) / 3, img.header.height());
    
    for (size_t x = 0; x < std::min(img.header.width(), uint32_t(50)); x++) {
        double r_sum = 0, g_sum = 0, b_sum = 0;
        size_t color_channels = std::min(img.header.channels(), uint8_t(3));
        size_t count = mid_row_end - mid_row_start;
        
        for (size_t y = mid_row_start; y < mid_row_end; y++) {
            size_t idx = (y * img.header.width() + x) * img.header.channels();
            r_sum += img.pixels[idx];
            if (color_channels > 1) g_sum += img.pixels[idx + 1];
            if (color_channels > 2) b_sum += img.pixels[idx + 2];
        }
        
        double r_avg = r_sum / count;
        double g_avg = (color_channels > 1) ? (g_sum / count) : 0;
        double b_avg = (color_channels > 2) ? (b_sum / count) : 0;
        
        printf("Col %3zu: R=%6.1f G=%6.1f B=%6.1f", x, r_avg, g_avg, b_avg);
        
        // Check for single-channel dominance
        if (r_avg > std::max(g_avg, b_avg) * 1.5 && r_avg > 50) printf(" [R-dominant]");
        else if (g_avg > std::max(r_avg, b_avg) * 1.5 && g_avg > 50) printf(" [G-dominant]");
        else if (b_avg > std::max(r_avg, g_avg) * 1.5 && b_avg > 50) printf(" [B-dominant]");
        
        printf("\n");
    }
    
    return 0;
}
