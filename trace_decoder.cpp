#include "rle.hpp"
#include <iostream>
#include <cstdio>

int main() {
    FILE* fp = fopen("teapot.rle", "rb");
    if (!fp) return 1;
    
    rle::Header h;
    rle::Endian e;
    rle::Error err;
    if (!rle::read_header_auto(fp, h, e, err)) { fclose(fp); return 1; }
    
    printf("Simulating decoder for first 10 opcodes:\n\n");
    
    uint32_t scan_y = h.ypos;
    int current_channel = -1;
    uint32_t scan_x = h.xpos;
    
    printf("Initial state: scan_y=%u, current_channel=%d\n\n", scan_y, current_channel);
    
    for (int op_count = 0; op_count < 10; op_count++) {
        uint8_t op0, op1;
        if (fread(&op0, 1, 1, fp) != 1) break;
        if (fread(&op1, 1, 1, fp) != 1) break;
        
        uint8_t base = op0 & ~rle::OPC_LONG_FLAG;
        bool longForm = (op0 & rle::OPC_LONG_FLAG) != 0;
        
        printf("[%d] Before: scan_y=%u, current_channel=%d\n", op_count, scan_y, current_channel);
        
        switch (base) {
            case rle::OPC_SKIP_LINES: {
                uint16_t lines = op1;
                printf("    Opcode: SKIP_LINES %u\n", lines);
                printf("    Logic: if (current_channel >= 0) → %s\n", current_channel >= 0 ? "TRUE" : "FALSE");
                if (current_channel >= 0) {
                    ++scan_y;
                    printf("           ++scan_y → scan_y=%u\n", scan_y);
                }
                scan_y += lines;
                printf("           scan_y += %u → scan_y=%u\n", lines, scan_y);
                current_channel = -1;
                printf("           current_channel = -1\n");
                break;
            }
            case rle::OPC_SET_COLOR: {
                uint16_t ch = op1;
                int new_channel = (ch == 255 && h.has_alpha()) ? h.ncolors : int(ch);
                printf("    Opcode: SET_COLOR %u (new_channel=%d)\n", ch, new_channel);
                printf("    Logic: if (new_channel==0 && current_channel>=0) → %s\n", 
                       (new_channel == 0 && current_channel >= 0) ? "TRUE" : "FALSE");
                if (new_channel == 0 && current_channel >= 0) {
                    ++scan_y;
                    printf("           ++scan_y → scan_y=%u\n", scan_y);
                }
                current_channel = new_channel;
                scan_x = h.xpos;
                printf("           current_channel=%d, scan_x=%u\n", current_channel, scan_x);
                break;
            }
            default:
                printf("    Opcode: Other (0x%02x)\n", base);
                // Skip data
                if (base == rle::OPC_RUN_DATA) {
                    fseek(fp, 2, SEEK_CUR);
                } else if (base == rle::OPC_BYTE_DATA) {
                    uint32_t count = op1 + 1;
                    fseek(fp, count + (count & 1), SEEK_CUR);
                }
                break;
        }
        
        printf("    After: scan_y=%u, current_channel=%d\n\n", scan_y, current_channel);
    }
    
    fclose(fp);
    return 0;
}
