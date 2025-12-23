// Analyze PPM files for vertical color patterns
// This tool checks for unusual vertical column artifacts
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <vector>

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

void analyze_vertical_patterns(const PPMImage& img) {
    printf("\n=== Vertical Pattern Analysis ===\n");
    printf("Image: %zux%zu\n\n", img.width, img.height);
    
    // Check for vertical column patterns
    // Sample every Nth column to look for color channel artifacts
    const size_t sample_stride = 10;  // Check every 10 columns
    const size_t sample_height = std::min(img.height, size_t(100));  // Sample first 100 rows
    
    printf("Sampling columns (every %zu columns, first %zu rows):\n", sample_stride, sample_height);
    
    for (size_t x = 0; x < img.width && x < 100; x += sample_stride) {
        // Calculate average of each channel for this column
        double r_sum = 0, g_sum = 0, b_sum = 0;
        for (size_t y = 0; y < sample_height; y++) {
            size_t idx = (y * img.width + x) * 3;
            r_sum += img.data[idx];
            g_sum += img.data[idx + 1];
            b_sum += img.data[idx + 2];
        }
        double r_avg = r_sum / sample_height;
        double g_avg = g_sum / sample_height;
        double b_avg = b_sum / sample_height;
        
        printf("  Col %4zu: R=%.1f G=%.1f B=%.1f", x, r_avg, g_avg, b_avg);
        
        // Check if this column is dominated by a single channel
        if (r_avg > g_avg * 1.5 && r_avg > b_avg * 1.5) {
            printf(" [RED dominant]");
        } else if (g_avg > r_avg * 1.5 && g_avg > b_avg * 1.5) {
            printf(" [GREEN dominant]");
        } else if (b_avg > r_avg * 1.5 && b_avg > g_avg * 1.5) {
            printf(" [BLUE dominant]");
        }
        printf("\n");
    }
    
    // Detect periodic color patterns in columns
    printf("\nChecking for periodic RGB column patterns:\n");
    
    // Check if columns cycle through R, G, B dominance
    size_t rgb_pattern_count = 0;
    for (size_t x = 0; x + 2 < img.width && x < 300; x += 3) {
        double r0=0, g0=0, b0=0, r1=0, g1=0, b1=0, r2=0, g2=0, b2=0;
        
        for (size_t y = 0; y < sample_height; y++) {
            r0 += img.data[(y * img.width + x) * 3];
            g0 += img.data[(y * img.width + x) * 3 + 1];
            b0 += img.data[(y * img.width + x) * 3 + 2];
            
            r1 += img.data[(y * img.width + x + 1) * 3];
            g1 += img.data[(y * img.width + x + 1) * 3 + 1];
            b1 += img.data[(y * img.width + x + 1) * 3 + 2];
            
            r2 += img.data[(y * img.width + x + 2) * 3];
            g2 += img.data[(y * img.width + x + 2) * 3 + 1];
            b2 += img.data[(y * img.width + x + 2) * 3 + 2];
        }
        
        r0 /= sample_height; g0 /= sample_height; b0 /= sample_height;
        r1 /= sample_height; g1 /= sample_height; b1 /= sample_height;
        r2 /= sample_height; g2 /= sample_height; b2 /= sample_height;
        
        // Check if column x is R-dominant, x+1 is G-dominant, x+2 is B-dominant
        bool is_rgb_pattern = 
            (r0 > g0 * 1.3 && r0 > b0 * 1.3) &&  // Col 0: Red dominant
            (g1 > r1 * 1.3 && g1 > b1 * 1.3) &&  // Col 1: Green dominant
            (b2 > r2 * 1.3 && b2 > g2 * 1.3);    // Col 2: Blue dominant
        
        if (is_rgb_pattern) {
            rgb_pattern_count++;
            if (rgb_pattern_count <= 5) {
                printf("  RGB pattern detected at columns %zu-%zu-%zu: ", x, x+1, x+2);
                printf("R=%.1f/G=%.1f/B=%.1f, R=%.1f/G=%.1f/B=%.1f, R=%.1f/G=%.1f/B=%.1f\n",
                       r0, g0, b0, r1, g1, b1, r2, g2, b2);
            }
        }
    }
    
    if (rgb_pattern_count > 0) {
        printf("\n⚠️  VERTICAL RGB PATTERN DETECTED: %zu occurrences\n", rgb_pattern_count);
        printf("    This suggests vertical color separation (possible channel ordering issue)\n");
    } else {
        printf("\n✓ No obvious vertical RGB pattern detected\n");
    }
    
    // Check for row-wise analysis to compare
    printf("\n=== Horizontal Row Analysis (for comparison) ===\n");
    printf("Sampling rows (every 10 rows, first 100 pixels):\n");
    
    for (size_t y = 0; y < img.height && y < 100; y += 10) {
        double r_sum = 0, g_sum = 0, b_sum = 0;
        size_t sample_width = std::min(img.width, size_t(100));
        for (size_t x = 0; x < sample_width; x++) {
            size_t idx = (y * img.width + x) * 3;
            r_sum += img.data[idx];
            g_sum += img.data[idx + 1];
            b_sum += img.data[idx + 2];
        }
        double r_avg = r_sum / sample_width;
        double g_avg = g_sum / sample_width;
        double b_avg = b_sum / sample_width;
        
        printf("  Row %4zu: R=%.1f G=%.1f B=%.1f\n", y, r_avg, g_avg, b_avg);
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
    
    analyze_vertical_patterns(img);
    
    return 0;
}
