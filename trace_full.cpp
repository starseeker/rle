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
    
    printf("Tracing through row 0 and row 1 transition:\n\n");
    
    uint32_t scan_y = h.ypos;
    int current_channel = -1;
    
    // Skip to the key opcodes
    int target_opcodes[] = {0, 15, 30, 43, 44};
    int current_target = 0;
    
    for (int op_count = 0; op_count < 100 && current_target < 5; op_count++) {
        uint8_t op0, op1;
        if (fread(&op0, 1, 1, fp) != 1) break;
        if (fread(&op1, 1, 1, fp) != 1) break;
        
        uint8_t base = op0 & ~rle::OPC_LONG_FLAG;
        
        if (op_count == target_opcodes[current_target]) {
            printf("[%3d] scan_y=%u, ch=%2d | ", op_count, scan_y, current_channel);
            
            if (base == rle::OPC_SET_COLOR) {
                int new_channel = op1;
                printf("SET_COLOR %d → ", new_channel);
                if (new_channel == 0 && current_channel >= 0) {
                    ++scan_y;
                    printf("++scan_y=%u, ", scan_y);
                }
                current_channel = new_channel;
                printf("ch=%d\n", current_channel);
            } else if (base == rle::OPC_SKIP_LINES) {
                uint16_t lines = op1;
                printf("SKIP_LINES %u → ", lines);
                if (current_channel >= 0) {
                    ++scan_y;
                    printf("++scan_y=%u (complete row), ", scan_y);
                }
                scan_y += lines;
                printf("scan_y+=%u → scan_y=%u\n", lines, scan_y);
                current_channel = -1;
            }
            current_target++;
        }
        
        // Skip data
        if (base == rle::OPC_RUN_DATA) {
            fseek(fp, 2, SEEK_CUR);
        } else if (base == rle::OPC_BYTE_DATA) {
            uint32_t count = op1 + 1;
            fseek(fp, count + (count & 1), SEEK_CUR);
        }
    }
    
    fclose(fp);
    return 0;
}
