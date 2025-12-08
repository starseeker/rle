// Copy the decoder with debug output
#include <cstdio>
#include <cstdint>
#include <vector>
#include <iostream>

// Minimal decoder with debug
int main() {
    FILE* f = fopen("debug.rle", "rb");
    
    // Skip header (16 bytes + background/colormap/comments)
    uint8_t hdr[16];
    fread(hdr, 1, 16, f);
    uint8_t flags = hdr[10];
    uint8_t ncolors = hdr[11];
    
    // Assuming NO_BACKGROUND flag (0x02)
    // No colormap
    // No comments  
    
    int scan_y = 0;
    int current_channel = -1;
    int scan_x = 0;
    const int W = 4, H = 4;
    
    std::cout << "Reading opcodes:\n";
    while (scan_y < H) {
        uint8_t op0, op1;
        if (fread(&op0, 1, 1, f) != 1) break;
        if (fread(&op1, 1, 1, f) != 1) break;
        
        uint8_t base = op0 & 0x3F;
        
        if (base == 2) {  // SET_COLOR
            int ch = op1;
            int new_channel = ch;
            
            std::cout << "  SET_COLOR(" << ch << ")";
            if (new_channel == 0 && current_channel >= 0) {
                scan_y++;
                std::cout << " -> incrementing scan_y to " << scan_y;
            }
            std::cout << "\n";
            
            current_channel = new_channel;
            scan_x = 0;
        } else if (base == 6) {  // RUN_DATA
            uint8_t enc = op1;
            uint16_t word;
            fread(&word, 2, 1, f);
            uint8_t value = word & 0xFF;
            
            int run_len = enc + 1;
            std::cout << "  RUN_DATA: len=" << run_len << ", value=" << int(value) 
                      << " -> writing to y=" << scan_y << ", channel=" << current_channel << "\n";
            
            scan_x += run_len;
        } else if (base == 7) {  // EOF
            std::cout << "  EOF\n";
            break;
        }
    }
    
    fclose(f);
    return 0;
}
