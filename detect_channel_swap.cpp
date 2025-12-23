// Detect vertical column channel swapping patterns
// This checks if consecutive pixels show channel swapping (e.g., R->G->B->R->G->B...)
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

void analyze_channel_ordering(const PPMImage& img) {
    printf("\n=== Channel Ordering Analysis ===\n");
    printf("Image: %zux%zu\n\n", img.width, img.height);
    
    // Look at a specific region where we expect real image data
    // Start from somewhere in the middle to avoid background
    size_t start_row = img.height / 3;
    size_t end_row = std::min(start_row + 20, img.height);
    size_t start_col = img.width / 3;
    size_t end_col = std::min(start_col + 30, img.width);
    
    printf("Examining region: rows %zu-%zu, cols %zu-%zu\n\n", 
           start_row, end_row, start_col, end_col);
    
    // Print out pixel values in a grid to see if there's a pattern
    printf("First 10 pixels in row %zu (showing R,G,B):\n", start_row);
    for (size_t x = start_col; x < start_col + 10 && x < img.width; x++) {
        size_t idx = (start_row * img.width + x) * 3;
        printf("  Pixel[%zu]: R=%3u G=%3u B=%3u", 
               x, img.data[idx], img.data[idx+1], img.data[idx+2]);
        
        // Check if this looks like a channel-swapped pattern
        uint8_t r = img.data[idx];
        uint8_t g = img.data[idx+1];
        uint8_t b = img.data[idx+2];
        
        // Detect if one channel is dominant
        if (r > 200 && g < 50 && b < 50) printf(" [RED-only]");
        else if (g > 200 && r < 50 && b < 50) printf(" [GREEN-only]");
        else if (b > 200 && r < 50 && g < 50) printf(" [BLUE-only]");
        
        printf("\n");
    }
    
    // Check for vertical columns showing single-channel dominance
    printf("\nChecking for vertical single-channel columns:\n");
    
    size_t red_only_cols = 0;
    size_t green_only_cols = 0;
    size_t blue_only_cols = 0;
    
    for (size_t x = 0; x < img.width; x++) {
        size_t red_count = 0, green_count = 0, blue_count = 0;
        size_t sample_count = 0;
        
        // Sample this column
        for (size_t y = start_row; y < end_row && y < img.height; y++) {
            size_t idx = (y * img.width + x) * 3;
            uint8_t r = img.data[idx];
            uint8_t g = img.data[idx+1];
            uint8_t b = img.data[idx+2];
            
            // Skip black/background pixels
            if (r + g + b < 10) continue;
            
            sample_count++;
            
            // Check dominant channel
            if (r > 100 && r > g * 2 && r > b * 2) red_count++;
            else if (g > 100 && g > r * 2 && g > b * 2) green_count++;
            else if (b > 100 && b > r * 2 && b > g * 2) blue_count++;
        }
        
        if (sample_count >= 5) {
            if (red_count >= sample_count * 0.8) {
                red_only_cols++;
                if (red_only_cols <= 5) {
                    printf("  Column %zu: RED-dominant (%zu/%zu pixels)\n", x, red_count, sample_count);
                }
            } else if (green_count >= sample_count * 0.8) {
                green_only_cols++;
                if (green_only_cols <= 5) {
                    printf("  Column %zu: GREEN-dominant (%zu/%zu pixels)\n", x, green_count, sample_count);
                }
            } else if (blue_count >= sample_count * 0.8) {
                blue_only_cols++;
                if (blue_only_cols <= 5) {
                    printf("  Column %zu: BLUE-dominant (%zu/%zu pixels)\n", x, blue_count, sample_count);
                }
            }
        }
    }
    
    printf("\nSummary:\n");
    printf("  Red-dominant columns: %zu\n", red_only_cols);
    printf("  Green-dominant columns: %zu\n", green_only_cols);
    printf("  Blue-dominant columns: %zu\n", blue_only_cols);
    
    if (red_only_cols > 10 || green_only_cols > 10 || blue_only_cols > 10) {
        printf("\n⚠️  VERTICAL CHANNEL SEPARATION DETECTED!\n");
        printf("    This suggests the image has vertical columns with separated RGB channels.\n");
        
        // Check if there's a repeating pattern
        if (red_only_cols > 5 && green_only_cols > 5 && blue_only_cols > 5) {
            printf("    Pattern suggests R-G-B column cycling (possible interleaved storage issue)\n");
        }
    } else {
        printf("\n✓ No obvious vertical channel separation detected\n");
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
    
    analyze_channel_ordering(img);
    
    return 0;
}
