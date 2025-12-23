// Create ASCII art visualization of PPM image to see patterns
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

void visualize_region(const PPMImage& img, size_t start_y, size_t start_x, size_t height, size_t width) {
    printf("\n=== ASCII Visualization ===\n");
    printf("Region: rows %zu-%zu, cols %zu-%zu\n\n", 
           start_y, start_y + height - 1, start_x, start_x + width - 1);
    
    // Header with column numbers
    printf("     ");
    for (size_t x = 0; x < width && x < 80; x++) {
        printf("%zu", (start_x + x) % 10);
    }
    printf("\n");
    
    for (size_t y = start_y; y < start_y + height && y < img.height; y++) {
        printf("%4zu ", y);
        for (size_t x = start_x; x < start_x + width && x < start_x + 80 && x < img.width; x++) {
            size_t idx = (y * img.width + x) * 3;
            uint8_t r = img.data[idx];
            uint8_t g = img.data[idx + 1];
            uint8_t b = img.data[idx + 2];
            
            // Convert to grayscale intensity
            uint8_t intensity = (r + g + b) / 3;
            
            // Different character for different intensities
            char c;
            if (intensity < 32) c = ' ';
            else if (intensity < 64) c = '.';
            else if (intensity < 96) c = ':';
            else if (intensity < 128) c = 'o';
            else if (intensity < 160) c = 'O';
            else if (intensity < 192) c = '0';
            else if (intensity < 224) c = '@';
            else c = '#';
            
            // Color annotation for strong single-channel
            if (r > 200 && g < 50 && b < 50) c = 'R';  // Red
            else if (g > 200 && r < 50 && b < 50) c = 'G';  // Green
            else if (b > 200 && r < 50 && g < 50) c = 'B';  // Blue
            
            printf("%c", c);
        }
        printf("\n");
    }
    
    printf("\nLegend: ' '=black, .=dark, :=dim, o=medium, O=bright, 0=brighter, @=very bright, #=white\n");
    printf("        R=red pixels, G=green pixels, B=blue pixels (if present)\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.ppm> [start_y] [start_x] [height] [width]\n", argv[0]);
        return 1;
    }
    
    PPMImage img;
    if (!read_ppm(argv[1], img)) {
        return 1;
    }
    
    printf("Image: %zux%zu\n", img.width, img.height);
    
    size_t start_y = (argc > 2) ? atoi(argv[2]) : img.height / 3;
    size_t start_x = (argc > 3) ? atoi(argv[3]) : img.width / 3;
    size_t height = (argc > 4) ? atoi(argv[4]) : 30;
    size_t width = (argc > 5) ? atoi(argv[5]) : 80;
    
    visualize_region(img, start_y, start_x, height, width);
    
    return 0;
}
