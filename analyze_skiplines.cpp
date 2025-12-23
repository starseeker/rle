/*
 * Analyze RLE file for skip line patterns
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint16_t read_u16(FILE *fp) {
    uint8_t b1 = fgetc(fp);
    uint8_t b2 = fgetc(fp);
    return (b2 << 8) | b1;
}

void analyze_skiplines(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open %s\n", filename);
        return;
    }
    
    // Skip header - read magic
    uint16_t magic = read_u16(fp);
    if (magic != 0x52cc) {
        fprintf(stderr, "Not a valid RLE file\n");
        fclose(fp);
        return;
    }
    
    // Read position and size
    int16_t xmin = read_u16(fp);
    int16_t ymin = read_u16(fp);
    int16_t xmax = read_u16(fp);
    int16_t ymax = read_u16(fp);
    
    int width = xmax - xmin + 1;
    int height = ymax - ymin + 1;
    
    uint8_t flags = fgetc(fp);
    uint8_t ncolors = fgetc(fp);
    uint8_t pixelbits = fgetc(fp);
    uint8_t ncmap = fgetc(fp);
    uint8_t cmaplen = fgetc(fp);
    
    printf("File: %s (%dx%d)\n", filename, width, height);
    printf("Flags: 0x%x, ncolors: %d, alpha: %s\n", 
           flags, ncolors, (flags & 0x04) ? "yes" : "no");
    
    // Skip to opcode data - skip background colors
    if (!(flags & 0x02)) {  // Not NO_BACKGROUND
        for (int i = 0; i < ncolors; i++) {
            read_u16(fp);
        }
    }
    
    // Skip colormap
    if (ncmap > 0) {
        int cmap_entries = (1 << cmaplen) * ncmap;
        for (int i = 0; i < cmap_entries; i++) {
            read_u16(fp);
        }
    }
    
    // Skip comments
    while (1) {
        int type = read_u16(fp);
        if (type == 0) break;
        
        int len = read_u16(fp);
        for (int i = 0; i < len; i++) {
            fgetc(fp);
        }
    }
    
    // Now analyze opcodes
    printf("\nAnalyzing opcodes...\n");
    
    int total_opcodes = 0;
    int skiplines_count = 0;
    int skiplines_1_count = 0;
    int total_skip_amount = 0;
    int scanline = ymin;
    int consecutive_skiplines_1 = 0;
    int max_consecutive_skiplines_1 = 0;
    
    while (!feof(fp)) {
        int op = fgetc(fp);
        if (op == EOF) break;
        
        total_opcodes++;
        
        if (op == 0) {  // SKIP_LINES
            int op1 = fgetc(fp);
            if (op1 == EOF) break;
            
            bool longForm = (op1 & 0x40) != 0;
            int skip = longForm ? read_u16(fp) : op1;
            
            skiplines_count++;
            total_skip_amount += skip;
            
            if (skip == 1) {
                skiplines_1_count++;
                consecutive_skiplines_1++;
                if (consecutive_skiplines_1 > max_consecutive_skiplines_1) {
                    max_consecutive_skiplines_1 = consecutive_skiplines_1;
                }
            } else {
                consecutive_skiplines_1 = 0;
            }
            
            scanline += skip;
        } else {
            consecutive_skiplines_1 = 0;
            
            if (op >= 64) {  // RUN or LITERAL
                bool longForm = (op & 0x40) != 0;
                int channel = op & 0x3f;
                
                if (longForm) {
                    fgetc(fp);  // skip count low
                    fgetc(fp);  // skip count high
                }
                
                if (op >= 128) {  // RUN
                    fgetc(fp);  // skip data byte
                } else {  // LITERAL
                    int count = longForm ? read_u16(fp) : (op & 0x3f);
                    for (int i = 0; i < count; i++) {
                        fgetc(fp);
                    }
                }
            } else if (op == 1) {  // EOF
                break;
            }
        }
    }
    
    fclose(fp);
    
    printf("\nResults:\n");
    printf("  Total opcodes: %d\n", total_opcodes);
    printf("  SKIP_LINES opcodes: %d (%.2f%%)\n", 
           skiplines_count, 100.0 * skiplines_count / total_opcodes);
    printf("  SKIP_LINES 1: %d (%.2f%% of all SKIP_LINES)\n", 
           skiplines_1_count, 
           skiplines_count > 0 ? 100.0 * skiplines_1_count / skiplines_count : 0);
    printf("  Total lines skipped: %d\n", total_skip_amount);
    printf("  Max consecutive SKIP_LINES 1: %d\n", max_consecutive_skiplines_1);
    
    if (skiplines_1_count > height / 2) {
        printf("\n  ⚠️  WARNING: High number of SKIP_LINES 1 opcodes!\n");
        printf("      This may indicate the alternating line pattern bug.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file.rle> [file2.rle ...]\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        analyze_skiplines(argv[i]);
        if (i < argc - 1) printf("\n========================================\n\n");
    }
    
    return 0;
}
