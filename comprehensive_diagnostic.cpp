// Comprehensive RLE Image Diagnostic Tool
// Checks for horizontal AND vertical patterns, channel ordering issues, etc.
#include "rle.hpp"
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <vector>
#include <algorithm>

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

void diagnose_image(const std::vector<uint8_t>& pixels, size_t width, size_t height, size_t channels) {
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║          COMPREHENSIVE RLE IMAGE DIAGNOSTIC REPORT            ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Image Dimensions: %zux%zu, %zu channels\n\n", width, height, channels);
    
    // === TEST 1: Horizontal Pattern Detection ===
    printf("═══ TEST 1: Horizontal Pattern Detection ═══\n");
    
    std::vector<bool> row_is_uniform(height);
    size_t uniform_row_count = 0;
    
    for (size_t y = 0; y < height; y++) {
        bool uniform = true;
        size_t color_chans = std::min(channels, size_t(3));
        
        // Get first pixel values
        std::vector<uint8_t> first_pixel(color_chans);
        for (size_t c = 0; c < color_chans; c++) {
            first_pixel[c] = pixels[(y * width) * channels + c];
        }
        
        // Compare all pixels in row
        for (size_t x = 1; x < width && uniform; x++) {
            for (size_t c = 0; c < color_chans; c++) {
                if (pixels[(y * width + x) * channels + c] != first_pixel[c]) {
                    uniform = false;
                    break;
                }
            }
        }
        
        row_is_uniform[y] = uniform;
        if (uniform) uniform_row_count++;
    }
    
    printf("  Uniform rows: %zu / %zu (%.1f%%)\n", uniform_row_count, height, 
           100.0 * uniform_row_count / height);
    
    // Check for periodic patterns
    if (uniform_row_count > height * 0.5) {
        std::vector<size_t> data_rows;
        for (size_t y = 0; y < height; y++) {
            if (!row_is_uniform[y]) {
                data_rows.push_back(y);
            }
        }
        
        if (data_rows.size() >= 2) {
            // Detect period
            for (int period = 2; period <= 5; period++) {
                size_t matches = 0;
                for (size_t i = 0; i + 1 < data_rows.size(); i++) {
                    if (data_rows[i+1] - data_rows[i] == period) {
                        matches++;
                    }
                }
                
                if (matches >= (data_rows.size() - 1) * 0.7) {
                    printf("  ⚠️  HORIZONTAL SPARSE PATTERN DETECTED (period=%d)\n", period);
                    printf("      Data rows: ");
                    for (size_t i = 0; i < std::min(data_rows.size(), size_t(10)); i++) {
                        printf("%zu ", data_rows[i]);
                    }
                    if (data_rows.size() > 10) printf("...");
                    printf("\n");
                    break;
                }
            }
        }
    } else {
        printf("  ✓ No horizontal sparse pattern detected\n");
    }
    
    // === TEST 2: Vertical Column Pattern Detection ===
    printf("\n═══ TEST 2: Vertical Column Channel Separation ═══\n");
    
    // Sample middle region
    size_t sample_y_start = height / 3;
    size_t sample_y_end = (2 * height) / 3;
    size_t sample_cols = std::min(width, size_t(300));
    
    size_t r_dominant_cols = 0, g_dominant_cols = 0, b_dominant_cols = 0;
    std::vector<size_t> r_cols, g_cols, b_cols;
    
    for (size_t x = 0; x < sample_cols; x++) {
        double r_sum = 0, g_sum = 0, b_sum = 0;
        size_t count = 0;
        
        for (size_t y = sample_y_start; y < sample_y_end; y++) {
            size_t idx = (y * width + x) * channels;
            uint8_t r = pixels[idx];
            uint8_t g = (channels > 1) ? pixels[idx + 1] : 0;
            uint8_t b = (channels > 2) ? pixels[idx + 2] : 0;
            
            // Skip near-black pixels
            if (r + g + b < 10) continue;
            
            r_sum += r;
            g_sum += g;
            b_sum += b;
            count++;
        }
        
        if (count < 5) continue;  // Need enough samples
        
        double r_avg = r_sum / count;
        double g_avg = g_sum / count;
        double b_avg = b_sum / count;
        
        // Check for single-channel dominance
        if (r_avg > std::max(g_avg, b_avg) * 1.8 && r_avg > 80) {
            r_dominant_cols++;
            if (r_cols.size() < 10) r_cols.push_back(x);
        } else if (g_avg > std::max(r_avg, b_avg) * 1.8 && g_avg > 80) {
            g_dominant_cols++;
            if (g_cols.size() < 10) g_cols.push_back(x);
        } else if (b_avg > std::max(r_avg, g_avg) * 1.8 && b_avg > 80) {
            b_dominant_cols++;
            if (b_cols.size() < 10) b_cols.push_back(x);
        }
    }
    
    printf("  Red-dominant columns: %zu\n", r_dominant_cols);
    if (r_cols.size() > 0) {
        printf("    Examples: ");
        for (size_t i = 0; i < r_cols.size(); i++) printf("%zu ", r_cols[i]);
        printf("\n");
    }
    
    printf("  Green-dominant columns: %zu\n", g_dominant_cols);
    if (g_cols.size() > 0) {
        printf("    Examples: ");
        for (size_t i = 0; i < g_cols.size(); i++) printf("%zu ", g_cols[i]);
        printf("\n");
    }
    
    printf("  Blue-dominant columns: %zu\n", b_dominant_cols);
    if (b_cols.size() > 0) {
        printf("    Examples: ");
        for (size_t i = 0; i < b_cols.size(); i++) printf("%zu ", b_cols[i]);
        printf("\n");
    }
    
    if (r_dominant_cols > 20 || g_dominant_cols > 20 || b_dominant_cols > 20) {
        printf("  ⚠️  VERTICAL CHANNEL SEPARATION DETECTED!\n");
        printf("      This suggests RGB channels are stored/decoded incorrectly.\n");
    } else if (r_dominant_cols + g_dominant_cols + b_dominant_cols > 10) {
        printf("  ⚠ Some channel-dominant columns found (may be normal image content)\n");
    } else {
        printf("  ✓ No vertical channel separation detected\n");
    }
    
    // === TEST 3: Channel Consistency Check ===
    printf("\n═══ TEST 3: Channel Consistency Check ===\n");
    
    // Check if RGB values are always equal (grayscale image)
    size_t grayscale_pixels = 0;
    size_t colored_pixels = 0;
    size_t total_nonzero = 0;
    
    for (size_t i = 0; i < width * height; i++) {
        size_t idx = i * channels;
        uint8_t r = pixels[idx];
        uint8_t g = (channels > 1) ? pixels[idx + 1] : r;
        uint8_t b = (channels > 2) ? pixels[idx + 2] : r;
        
        if (r + g + b < 10) continue;
        
        total_nonzero++;
        
        if (r == g && g == b) {
            grayscale_pixels++;
        } else {
            colored_pixels++;
        }
    }
    
    if (total_nonzero > 0) {
        printf("  Nonzero pixels: %zu\n", total_nonzero);
        printf("  Grayscale pixels (R=G=B): %zu (%.1f%%)\n", 
               grayscale_pixels, 100.0 * grayscale_pixels / total_nonzero);
        printf("  Colored pixels (R≠G≠B): %zu (%.1f%%)\n",
               colored_pixels, 100.0 * colored_pixels / total_nonzero);
        
        if (grayscale_pixels > total_nonzero * 0.95) {
            printf("  ✓ Image is grayscale (expected for some RLE files)\n");
        }
    }
    
    // === TEST 4: Sample Pixel Dump ===
    printf("\n═══ TEST 4: Sample Pixel Values ===\n");
    
    size_t sample_row = height / 2;
    printf("  Row %zu, pixels 100-109:\n", sample_row);
    for (size_t x = 100; x < 110 && x < width; x++) {
        size_t idx = (sample_row * width + x) * channels;
        printf("    [%3zu] ", x);
        for (size_t c = 0; c < std::min(channels, size_t(4)); c++) {
            printf("%02X ", pixels[idx + c]);
        }
        printf("\n");
    }
    
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                        END OF REPORT                           ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.rle>\n", argv[0]);
        fprintf(stderr, "\nThis tool performs comprehensive diagnostics on RLE images to detect:\n");
        fprintf(stderr, "  - Horizontal sparse row patterns (period-2, 3, 4, 5)\n");
        fprintf(stderr, "  - Vertical RGB channel separation\n");
        fprintf(stderr, "  - Channel ordering issues\n");
        fprintf(stderr, "  - Grayscale vs color content\n");
        return 1;
    }

    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s\n", argv[1]);
        return 1;
    }

    printf("Loading %s...\n", argv[1]);
    
    rle::Image img;
    rle::DecoderResult res = rle::Decoder::read(fp, img);
    fclose(fp);

    if (!res.ok) {
        fprintf(stderr, "Failed to decode: %s\n", rle::error_string(res.error));
        return 1;
    }

    diagnose_image(img.pixels, img.header.width(), img.header.height(), img.header.channels());
    
    return 0;
}
