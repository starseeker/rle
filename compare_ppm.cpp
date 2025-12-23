/*
 * Compare two PPM files and report differences
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct PPM {
    int width, height;
    unsigned char *data;
};

PPM* read_ppm(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return NULL;
    
    PPM *ppm = new PPM();
    char magic[3];
    if (fscanf(fp, "%2s\n", magic) != 1 || strcmp(magic, "P6") != 0) {
        fclose(fp);
        delete ppm;
        return NULL;
    }
    
    if (fscanf(fp, "%d %d\n", &ppm->width, &ppm->height) != 2) {
        fclose(fp);
        delete ppm;
        return NULL;
    }
    
    int maxval;
    if (fscanf(fp, "%d\n", &maxval) != 1) {
        fclose(fp);
        delete ppm;
        return NULL;
    }
    
    ppm->data = new unsigned char[ppm->width * ppm->height * 3];
    size_t n = fread(ppm->data, 1, ppm->width * ppm->height * 3, fp);
    if (n != (size_t)(ppm->width * ppm->height * 3)) {
        fclose(fp);
        delete[] ppm->data;
        delete ppm;
        return NULL;
    }
    
    fclose(fp);
    return ppm;
}

void analyze_differences(const char *file1, const char *file2) {
    PPM *ppm1 = read_ppm(file1);
    PPM *ppm2 = read_ppm(file2);
    
    if (!ppm1 || !ppm2) {
        printf("Error reading files\n");
        if (ppm1) { delete[] ppm1->data; delete ppm1; }
        if (ppm2) { delete[] ppm2->data; delete ppm2; }
        return;
    }
    
    if (ppm1->width != ppm2->width || ppm1->height != ppm2->height) {
        printf("Dimension mismatch: %dx%d vs %dx%d\n", 
               ppm1->width, ppm1->height, ppm2->width, ppm2->height);
        delete[] ppm1->data;
        delete ppm1;
        delete[] ppm2->data;
        delete ppm2;
        return;
    }
    
    printf("Comparing %s vs %s (%dx%d)\n", file1, file2, ppm1->width, ppm1->height);
    
    int total_pixels = ppm1->width * ppm1->height;
    int diff_pixels = 0;
    int total_channels = total_pixels * 3;
    int diff_channels = 0;
    long long total_abs_diff = 0;
    int max_diff = 0;
    
    // Track which rows have differences
    int *row_diffs = new int[ppm1->height]();
    
    for (int i = 0; i < total_channels; i++) {
        int diff = abs((int)ppm1->data[i] - (int)ppm2->data[i]);
        if (diff > 0) {
            diff_channels++;
            total_abs_diff += diff;
            if (diff > max_diff) max_diff = diff;
            
            // Track row
            int pixel = i / 3;
            int y = pixel / ppm1->width;
            row_diffs[y]++;
        }
    }
    
    for (int i = 0; i < total_pixels; i++) {
        int idx = i * 3;
        if (ppm1->data[idx] != ppm2->data[idx] ||
            ppm1->data[idx+1] != ppm2->data[idx+1] ||
            ppm1->data[idx+2] != ppm2->data[idx+2]) {
            diff_pixels++;
        }
    }
    
    printf("  Different pixels: %d / %d (%.2f%%)\n", 
           diff_pixels, total_pixels, 100.0 * diff_pixels / total_pixels);
    printf("  Different channels: %d / %d (%.2f%%)\n", 
           diff_channels, total_channels, 100.0 * diff_channels / total_channels);
    printf("  Max difference: %d\n", max_diff);
    printf("  Average absolute difference: %.2f\n", 
           diff_channels > 0 ? (double)total_abs_diff / diff_channels : 0.0);
    
    // Check for patterns
    int rows_with_diffs = 0;
    for (int y = 0; y < ppm1->height; y++) {
        if (row_diffs[y] > 0) rows_with_diffs++;
    }
    
    printf("  Rows with differences: %d / %d (%.2f%%)\n", 
           rows_with_diffs, ppm1->height, 100.0 * rows_with_diffs / ppm1->height);
    
    // Show first few differing rows
    printf("\n  First 10 rows with differences:\n");
    int count = 0;
    for (int y = 0; y < ppm1->height && count < 10; y++) {
        if (row_diffs[y] > 0) {
            printf("    Row %d: %d different channels (%.2f%% of row)\n", 
                   y, row_diffs[y], 100.0 * row_diffs[y] / (ppm1->width * 3));
            count++;
        }
    }
    
    delete[] row_diffs;
    delete[] ppm1->data;
    delete ppm1;
    delete[] ppm2->data;
    delete ppm2;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s file1.ppm file2.ppm\n", argv[0]);
        return 1;
    }
    
    analyze_differences(argv[1], argv[2]);
    return 0;
}
