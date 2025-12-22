/**
 * @file dump_rle_opcodes.cpp
 * @brief Utility to dump first N opcodes from an RLE file
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>

int main(int argc, char** argv) {
    const char* filename = (argc > 1) ? argv[1] : "teapot.rle";
    int max_opcodes = (argc > 2) ? atoi(argv[2]) : 50;
    
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open " << filename << "\n";
        return 1;
    }
    
    // Read header
    rle::Header h;
    rle::Endian e;
    rle::Error err;
    
    if (!rle::read_header_auto(fp, h, e, err)) {
        std::cout << "ERROR: Failed to read header\n";
        fclose(fp);
        return 1;
    }
    
    std::cout << "Image: " << h.width() << "x" << h.height() << ", " << (int)h.channels() << " channels\n";
    std::cout << "Background: [";
    for (size_t i = 0; i < h.background.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << (int)h.background[i];
    }
    std::cout << "]\n\n";
    
    std::cout << "Opcodes:\n";
    
    int opcode_count = 0;
    uint32_t scan_y = h.ypos;
    int current_channel = -1;
    
    while (opcode_count < max_opcodes) {
        uint8_t op0, op1;
        if (fread(&op0, 1, 1, fp) != 1) break;
        if (fread(&op1, 1, 1, fp) != 1) break;
        
        uint8_t base = op0 & ~rle::OPC_LONG_FLAG;
        bool longForm = (op0 & rle::OPC_LONG_FLAG) != 0;
        
        printf("[%3d] scan_y=%3u ch=%2d | ", opcode_count, scan_y, current_channel);
        
        switch (base) {
            case rle::OPC_SKIP_LINES: {
                uint16_t lines = op1;
                if (longForm) {
                    uint8_t b0, b1;
                    fread(&b0, 1, 1, fp);
                    fread(&b1, 1, 1, fp);
                    lines = (e == rle::Endian::Little) ? (b0 | (b1 << 8)) : (b1 | (b0 << 8));
                }
                if (current_channel >= 0) scan_y++;
                scan_y += lines;
                current_channel = -1;
                printf("SKIP_LINES %u%s → scan_y=%u\n", lines, longForm?" (long)":"", scan_y);
                break;
            }
            case rle::OPC_SET_COLOR: {
                uint16_t ch = op1;
                int new_channel = (ch == 255 && h.has_alpha()) ? h.ncolors : int(ch);
                if (new_channel == 0 && current_channel >= 0) {
                    scan_y++;
                }
                current_channel = new_channel;
                printf("SET_COLOR %u → scan_y=%u ch=%d\n", ch, scan_y, current_channel);
                break;
            }
            case rle::OPC_SKIP_PIXELS: {
                uint16_t skip = op1;
                if (longForm) {
                    uint8_t b0, b1;
                    fread(&b0, 1, 1, fp);
                    fread(&b1, 1, 1, fp);
                    skip = (e == rle::Endian::Little) ? (b0 | (b1 << 8)) : (b1 | (b0 << 8));
                }
                printf("SKIP_PIXELS %u%s\n", skip, longForm?" (long)":"");
                break;
            }
            case rle::OPC_BYTE_DATA: {
                uint16_t enc = op1;
                if (longForm) {
                    uint8_t b0, b1;
                    fread(&b0, 1, 1, fp);
                    fread(&b1, 1, 1, fp);
                    enc = (e == rle::Endian::Little) ? (b0 | (b1 << 8)) : (b1 | (b0 << 8));
                }
                uint32_t count = enc + 1;
                // Skip data bytes
                fseek(fp, count + (count & 1), SEEK_CUR);
                printf("BYTE_DATA count=%u%s\n", count, longForm?" (long)":"");
                break;
            }
            case rle::OPC_RUN_DATA: {
                uint16_t enc = op1;
                if (longForm) {
                    uint8_t b0, b1;
                    fread(&b0, 1, 1, fp);
                    fread(&b1, 1, 1, fp);
                    enc = (e == rle::Endian::Little) ? (b0 | (b1 << 8)) : (b1 | (b0 << 8));
                }
                uint32_t run_len = enc + 1;
                // Skip value word
                fseek(fp, 2, SEEK_CUR);
                printf("RUN_DATA len=%u%s\n", run_len, longForm?" (long)":"");
                break;
            }
            case rle::OPC_EOF:
                printf("EOF\n");
                fclose(fp);
                return 0;
            default:
                printf("UNKNOWN opcode 0x%02x\n", base);
                fclose(fp);
                return 1;
        }
        
        opcode_count++;
    }
    
    fclose(fp);
    return 0;
}
