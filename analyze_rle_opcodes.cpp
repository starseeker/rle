/**
 * @file analyze_rle_opcodes.cpp
 * @brief Analyze RLE opcodes to detect SKIP_LINES patterns
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>

// Endian reading functions
uint16_t read_u16_le(FILE* fp) {
    uint8_t b[2];
    if (fread(b, 1, 2, fp) != 2) return 0;
    return b[0] | (b[1] << 8);
}

uint16_t read_u16_be(FILE* fp) {
    uint8_t b[2];
    if (fread(b, 1, 2, fp) != 2) return 0;
    return (b[0] << 8) | b[1];
}

uint8_t read_u8(FILE* fp) {
    uint8_t b;
    if (fread(&b, 1, 1, fp) != 1) return 0;
    return b;
}

enum Opcode {
    OPC_SKIP_LINES  = 0x01,
    OPC_SET_COLOR   = 0x02,
    OPC_SKIP_PIXELS = 0x03,
    OPC_BYTE_DATA   = 0x05,
    OPC_RUN_DATA    = 0x06,
    OPC_EOF         = 0x07,
    OPC_LONG_FLAG   = 0x40
};

void analyze_rle_opcodes(const char* filename, int max_opcodes = 5000) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "RLE Opcode Analysis: " << filename << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Detect endianness and skip header
    uint8_t magic_bytes[2];
    fread(magic_bytes, 1, 2, fp);
    bool is_big_endian = (magic_bytes[0] == 0xcc && magic_bytes[1] == 0x52);
    auto read_u16 = is_big_endian ? read_u16_be : read_u16_le;
    
    // Skip rest of basic header (xpos, ypos, xsize, ysize, flags, ncolors, pixelbits, ncmap, cmaplen, reserved)
    fseek(fp, 11, SEEK_SET);  // After magic (2 bytes)
    
    uint8_t flags = read_u8(fp);
    fseek(fp, -1, SEEK_CUR);  // Back up one
    
    uint8_t ncolors = read_u8(fp);
    fseek(fp, 8, SEEK_CUR);  // Skip to after reserved bytes (pos 12)
    
    // Skip background if present
    if (!(flags & 0x02)) {  // NO_BACKGROUND flag
        for (int i = 0; i < ncolors; i++) {
            read_u16(fp);
        }
    }
    
    // Skip colormap (not common in these files)
    
    // Skip comments if present
    if (flags & 0x08) {
        while (true) {
            uint16_t type = read_u16(fp);
            if (type == 0) break;
            uint16_t length = read_u16(fp);
            if (length == 0) continue;
            fseek(fp, length + (length % 2), SEEK_CUR);  // Skip content + padding
        }
    }
    
    std::cout << "\nScanning opcodes..." << std::endl;
    
    int opcode_count = 0;
    int skip_lines_count = 0;
    int set_color_count = 0;
    int current_row = 0;
    int current_channel = -1;
    std::vector<int> skip_lines_after_row;
    
    while (opcode_count < max_opcodes) {
        uint8_t op0 = read_u8(fp);
        if (feof(fp)) break;
        
        uint8_t op1 = read_u8(fp);
        if (feof(fp)) break;
        
        uint8_t base = op0 & ~OPC_LONG_FLAG;
        bool longForm = (op0 & OPC_LONG_FLAG) != 0;
        
        opcode_count++;
        
        switch (base) {
            case OPC_SKIP_LINES: {
                uint16_t lines = longForm ? read_u16(fp) : op1;
                skip_lines_count++;
                
                if (skip_lines_count <= 20) {  // Show first 20
                    std::cout << "  Opcode " << opcode_count << ": SKIP_LINES " << lines 
                              << " (after row " << current_row << ")" << std::endl;
                }
                
                skip_lines_after_row.push_back(current_row);
                
                if (current_channel >= 0) current_row++;
                current_row += lines;
                current_channel = -1;
                break;
            }
            
            case OPC_SET_COLOR: {
                uint8_t ch = op1;
                int new_channel = (ch == 255) ? ncolors : int(ch);
                
                if (new_channel == 0 && current_channel >= 0) {
                    current_row++;
                }
                current_channel = new_channel;
                set_color_count++;
                break;
            }
            
            case OPC_SKIP_PIXELS: {
                uint16_t skip = longForm ? read_u16(fp) : op1;
                break;
            }
            
            case OPC_BYTE_DATA: {
                uint16_t enc = longForm ? read_u16(fp) : op1;
                uint32_t count = uint32_t(enc) + 1;
                fseek(fp, count + (count % 2), SEEK_CUR);  // Skip data + padding
                break;
            }
            
            case OPC_RUN_DATA: {
                uint16_t enc = longForm ? read_u16(fp) : op1;
                read_u16(fp);  // Skip run value
                break;
            }
            
            case OPC_EOF:
                std::cout << "\n  Opcode " << opcode_count << ": EOF" << std::endl;
                goto done;
                
            default:
                std::cerr << "\n  Unknown opcode: 0x" << std::hex << (int)base << std::dec << std::endl;
                goto done;
        }
    }
    
done:
    std::cout << "\n========================================" << std::endl;
    std::cout << "Summary:" << std::endl;
    std::cout << "  Total opcodes processed: " << opcode_count << std::endl;
    std::cout << "  SKIP_LINES opcodes: " << skip_lines_count << std::endl;
    std::cout << "  SET_COLOR opcodes: " << set_color_count << std::endl;
    
    // Analyze skip pattern
    if (skip_lines_count > 0) {
        std::cout << "\nSKIP_LINES Pattern Analysis:" << std::endl;
        
        // Check if skip happens after every row
        int consecutive_skips = 0;
        for (size_t i = 0; i < skip_lines_after_row.size() - 1; i++) {
            if (skip_lines_after_row[i+1] == skip_lines_after_row[i] + 2) {  // Skip happened after every other row
                consecutive_skips++;
            }
        }
        
        if (consecutive_skips > skip_lines_count / 2) {
            std::cout << "  ⚠️  ALTERNATING LINE PATTERN DETECTED!" << std::endl;
            std::cout << "  SKIP_LINES appears to occur after most data rows" << std::endl;
            std::cout << "  This causes every other row to be skipped, creating visual artifacts" << std::endl;
        } else {
            std::cout << "  Normal skip pattern (not after every row)" << std::endl;
        }
    }
    
    std::cout << "========================================" << std::endl;
    
    fclose(fp);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <rle-file> [max-opcodes]" << std::endl;
        return 1;
    }
    
    int max_opcodes = 5000;
    if (argc >= 3) {
        max_opcodes = atoi(argv[2]);
    }
    
    analyze_rle_opcodes(argv[1], max_opcodes);
    return 0;
}
