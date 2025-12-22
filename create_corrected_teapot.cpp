#include "rle.hpp"
#include <iostream>
#include <cstdio>

int rle_write(icv_image_t *bif, FILE *fp);
icv_image_t* rle_read(FILE *fp);
void bu_free(void *ptr, const char *str);

int main() {
    printf("Creating corrected teapot.rle by de-interlacing...\n\n");
    
    // Read the buggy teapot.rle
    FILE* fp = fopen("teapot.rle", "rb");
    if (!fp) {
        printf("ERROR: Could not open teapot.rle\n");
        return 1;
    }
    
    icv_image_t* buggy = rle_read(fp);
    fclose(fp);
    
    if (!buggy) {
        printf("ERROR: Failed to read teapot.rle\n");
        return 1;
    }
    
    printf("Read buggy teapot: %zux%zu, %zu channels\n", buggy->width, buggy->height, buggy->channels);
    
    // Check how many rows are black
    size_t black_rows = 0;
    size_t data_rows = 0;
    for (size_t y = 0; y < buggy->height; y++) {
        bool is_black = true;
        for (size_t x = 0; x < buggy->width && is_black; x++) {
            size_t idx = (y * buggy->width + x) * buggy->channels;
            for (size_t c = 0; c < buggy->channels; c++) {
                if (buggy->data[idx + c] > 0.01) {
                    is_black = false;
                    break;
                }
            }
        }
        if (is_black) black_rows++; else data_rows++;
    }
    
    printf("Found %zu black rows, %zu data rows\n", black_rows, data_rows);
    
    // Create a de-interlaced image by duplicating each data row into adjacent black rows
    icv_image_t* fixed = (icv_image_t*)calloc(1, sizeof(icv_image_t));
    fixed->magic = buggy->magic;
    fixed->width = buggy->width;
    fixed->height = buggy->height;
    fixed->channels = buggy->channels;
    fixed->alpha_channel = buggy->alpha_channel;
    fixed->color_space = buggy->color_space;
    fixed->gamma_corr = buggy->gamma_corr;
    
    size_t data_size = buggy->width * buggy->height * buggy->channels * sizeof(double);
    fixed->data = (double*)calloc(1, data_size);
    
    // Copy data, filling in black rows with adjacent data rows
    for (size_t y = 0; y < buggy->height; y++) {
        bool is_black = true;
        for (size_t x = 0; x < buggy->width && is_black; x++) {
            size_t idx = (y * buggy->width + x) * buggy->channels;
            for (size_t c = 0; c < buggy->channels; c++) {
                if (buggy->data[idx + c] > 0.01) {
                    is_black = false;
                    break;
                }
            }
        }
        
        size_t source_row = y;
        if (is_black) {
            // Use previous row if available, otherwise next row
            if (y > 0) source_row = y - 1;
            else if (y < buggy->height - 1) source_row = y + 1;
        }
        
        // Copy the row
        for (size_t x = 0; x < buggy->width; x++) {
            size_t src_idx = (source_row * buggy->width + x) * buggy->channels;
            size_t dst_idx = (y * buggy->width + x) * buggy->channels;
            for (size_t c = 0; c < buggy->channels; c++) {
                fixed->data[dst_idx + c] = buggy->data[src_idx + c];
            }
        }
    }
    
    printf("Created de-interlaced image\n");
    
    // Write the fixed image
    fp = fopen("teapot_fixed.rle", "wb");
    if (!fp) {
        printf("ERROR: Could not create teapot_fixed.rle\n");
        bu_free(buggy->data, "data");
        bu_free(buggy, "img");
        free(fixed->data);
        free(fixed);
        return 1;
    }
    
    int result = rle_write(fixed, fp);
    fclose(fp);
    
    if (result != 0) {
        printf("ERROR: Failed to write teapot_fixed.rle\n");
    } else {
        printf("SUCCESS: Created teapot_fixed.rle\n");
    }
    
    bu_free(buggy->data, "data");
    bu_free(buggy, "img");
    free(fixed->data);
    free(fixed);
    
    return result;
}
