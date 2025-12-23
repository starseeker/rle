// Comprehensive vertical pattern detector
// Detects any kind of repetitive pattern in vertical columns
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <vector>
#include <map>

struct PPMImage {
    size_t width;
    size_t height;
    std::vector<uint8_t> data;  // RGB data
};

bool read_ppm(const char* filename, PPMImage& img) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open %s\n", filename);
        return false;
    }
    
    char magic[3];
    if (fscanf(fp, "%2s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fprintf(stderr, "Error: Not a P6 PPM file\n");
        fclose(fp);
        return false;
    }
    
    int width, height, maxval;
    if (fscanf(fp, "%d %d %d", &width, &height, &maxval) != 3) {
        fprintf(stderr, "Error: Cannot read PPM header\n");
        fclose(fp);
        return false;
    }
    
    fgetc(fp); // consume the whitespace after maxval
    
    img.width = width;
    img.height = height;
    img.data.resize(width * height * 3);
    
    if (fread(img.data.data(), 1, img.data.size(), fp) != img.data.size()) {
        fprintf(stderr, "Error: Cannot read PPM data\n");
        fclose(fp);
        return false;
    }
    
    fclose(fp);
    return true;
}

void analyze_all_patterns(const PPMImage& img) {
    printf("\n=== Comprehensive Pattern Analysis ===\n");
    printf("Image: %zux%zu\n\n", img.width, img.height);
    
    // Skip mostly black images
    size_t nonzero_count = 0;
    for (size_t i = 0; i < img.data.size(); i++) {
        if (img.data[i] > 10) nonzero_count++;
    }
    
    if (nonzero_count < img.data.size() * 0.1) {
        printf("Image is mostly black/background (%.1f%% nonzero pixels)\n", 
               100.0 * nonzero_count / img.data.size());
        printf("Skipping detailed analysis.\n");
        return;
    }
    
    printf("Image has %.1f%% nonzero pixels\n\n", 
           100.0 * nonzero_count / img.data.size());
    
    // 1. Check for column-wise channel swapping (period-3 pattern: R,G,B,R,G,B...)
    printf("=== Test 1: Consecutive columns with single-channel data ===\n");
    
    size_t period3_patterns = 0;
    for (size_t x = 0; x + 2 < img.width && x < 300; x += 3) {
        // Sample middle rows where we expect image content
        size_t start_row = img.height / 3;
        size_t end_row = (2 * img.height) / 3;
        
        double r0_avg = 0, g0_avg = 0, b0_avg = 0;
        double r1_avg = 0, g1_avg = 0, b1_avg = 0;
        double r2_avg = 0, g2_avg = 0, b2_avg = 0;
        size_t count = 0;
        
        for (size_t y = start_row; y < end_row; y++) {
            // Column x
            size_t idx0 = (y * img.width + x) * 3;
            r0_avg += img.data[idx0];
            g0_avg += img.data[idx0 + 1];
            b0_avg += img.data[idx0 + 2];
            
            // Column x+1
            size_t idx1 = (y * img.width + x + 1) * 3;
            r1_avg += img.data[idx1];
            g1_avg += img.data[idx1 + 1];
            b1_avg += img.data[idx1 + 2];
            
            // Column x+2
            size_t idx2 = (y * img.width + x + 2) * 3;
            r2_avg += img.data[idx2];
            g2_avg += img.data[idx2 + 1];
            b2_avg += img.data[idx2 + 2];
            
            count++;
        }
        
        if (count == 0) continue;
        
        r0_avg /= count; g0_avg /= count; b0_avg /= count;
        r1_avg /= count; g1_avg /= count; b1_avg /= count;
        r2_avg /= count; g2_avg /= count; b2_avg /= count;
        
        // Skip if all columns are near-black
        if (r0_avg + g0_avg + b0_avg < 30 &&
            r1_avg + g1_avg + b1_avg < 30 &&
            r2_avg + g2_avg + b2_avg < 30) {
            continue;
        }
        
        // Check if we have: col0 has only R, col1 has only G, col2 has only B
        bool col0_red_only = (r0_avg > std::max(g0_avg, b0_avg) * 1.5 && r0_avg > 50);
        bool col1_green_only = (g1_avg > std::max(r1_avg, b1_avg) * 1.5 && g1_avg > 50);
        bool col2_blue_only = (b2_avg > std::max(r2_avg, g2_avg) * 1.5 && b2_avg > 50);
        
        // OR check the reverse: col0 has only G, col1 has only B, col2 has only R
        bool col0_green_only = (g0_avg > std::max(r0_avg, b0_avg) * 1.5 && g0_avg > 50);
        bool col1_blue_only = (b1_avg > std::max(r1_avg, g1_avg) * 1.5 && b1_avg > 50);
        bool col2_red_only = (r2_avg > std::max(g2_avg, b2_avg) * 1.5 && r2_avg > 50);
        
        // OR check for BGR pattern
        bool col0_blue_only = (b0_avg > std::max(r0_avg, g0_avg) * 1.5 && b0_avg > 50);
        bool col2_green_only = (g2_avg > std::max(r2_avg, b2_avg) * 1.5 && g2_avg > 50);
        
        if ((col0_red_only && col1_green_only && col2_blue_only) ||
            (col0_green_only && col1_blue_only && col2_red_only) ||
            (col0_blue_only && col1_green_only && col2_red_only)) {
            period3_patterns++;
            if (period3_patterns <= 3) {
                printf("  Columns %zu-%zu-%zu: Potential channel separation\n", x, x+1, x+2);
                printf("    Col %zu: R=%.1f G=%.1f B=%.1f\n", x, r0_avg, g0_avg, b0_avg);
                printf("    Col %zu: R=%.1f G=%.1f B=%.1f\n", x+1, r1_avg, g1_avg, b1_avg);
                printf("    Col %zu: R=%.1f G=%.1f B=%.1f\n", x+2, r2_avg, g2_avg, b2_avg);
            }
        }
    }
    
    if (period3_patterns > 10) {
        printf("\n⚠️  VERTICAL CHANNEL SEPARATION DETECTED: %zu patterns\n", period3_patterns);
        printf("    This suggests vertical columns with separated R/G/B channels.\n");
        printf("    Likely cause: Pixel data stored/read in wrong order (planar instead of interleaved?)\n");
    } else if (period3_patterns > 0) {
        printf("\n  Found %zu potential patterns (may be normal image content)\n", period3_patterns);
    } else {
        printf("  ✓ No channel separation detected\n");
    }
    
    // 2. Look at pixel values directly in a small region
    printf("\n=== Test 2: Raw pixel dump (middle region) ===\n");
    size_t sample_y = img.height / 2;
    size_t sample_x_start = img.width / 3;
    
    printf("Row %zu, pixels %zu-%zu:\n", sample_y, sample_x_start, sample_x_start + 9);
    for (size_t x = sample_x_start; x < sample_x_start + 10 && x < img.width; x++) {
        size_t idx = (sample_y * img.width + x) * 3;
        printf("  [%3zu] R=%3u G=%3u B=%3u", x, 
               img.data[idx], img.data[idx+1], img.data[idx+2]);
        
        // Annotate if single-channel dominant
        uint8_t r = img.data[idx];
        uint8_t g = img.data[idx+1];
        uint8_t b = img.data[idx+2];
        
        if (r > g * 2 && r > b * 2 && r > 100) printf(" [R-dominant]");
        else if (g > r * 2 && g > b * 2 && g > 100) printf(" [G-dominant]");
        else if (b > r * 2 && b > g * 2 && b > 100) printf(" [B-dominant]");
        
        printf("\n");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.ppm>\n", argv[0]);
        return 1;
    }
    
    PPMImage img;
    if (!read_ppm(argv[1], img)) {
        return 1;
    }
    
    analyze_all_patterns(img);
    
    return 0;
}
