/**
 * @file dump_rle_info.cpp
 * @brief Dump detailed information about RLE files including header and opcode analysis
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// Read 16-bit value (endian-aware)
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

// Read 8-bit value
uint8_t read_u8(FILE* fp) {
    uint8_t b;
    if (fread(&b, 1, 1, fp) != 1) return 0;
    return b;
}

void dump_rle_header(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "RLE File: " << filename << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Read magic number and detect endianness
    uint16_t magic_raw;
    {
        uint8_t b[2];
        if (fread(b, 1, 2, fp) != 2) {
            std::cerr << "Failed to read magic number" << std::endl;
            fclose(fp);
            return;
        }
        magic_raw = (b[0] << 8) | b[1];  // Read as big-endian for display
    }
    
    bool is_big_endian = false;
    std::cout << "Magic: 0x" << std::hex << magic_raw << std::dec;
    if (magic_raw == 0xcc52) {
        std::cout << " (valid RLE, BIG-ENDIAN)" << std::endl;
        is_big_endian = true;
    } else if (magic_raw == 0x52cc) {
        std::cout << " (valid RLE, LITTLE-ENDIAN)" << std::endl;
        is_big_endian = false;
    } else {
        std::cout << " (INVALID - expected 0xcc52 or 0x52cc)" << std::endl;
        fclose(fp);
        return;
    }
    
    // Define read function based on endianness
    auto read_u16 = is_big_endian ? read_u16_be : read_u16_le;
    
    // Read xpos, ypos
    int16_t xpos = read_u16(fp);
    int16_t ypos = read_u16(fp);
    std::cout << "Position: (" << xpos << ", " << ypos << ")" << std::endl;
    
    // Read xsize, ysize
    int16_t xsize = read_u16(fp);
    int16_t ysize = read_u16(fp);
    std::cout << "Size: " << xsize << " x " << ysize << std::endl;
    
    // Read flags
    uint8_t flags = read_u8(fp);
    std::cout << "Flags: 0x" << std::hex << (int)flags << std::dec << std::endl;
    std::cout << "  - CLEAR_FIRST: " << ((flags & 0x01) ? "Yes" : "No") << std::endl;
    std::cout << "  - NO_BACKGROUND: " << ((flags & 0x02) ? "Yes" : "No") << std::endl;
    std::cout << "  - ALPHA: " << ((flags & 0x04) ? "Yes" : "No") << std::endl;
    std::cout << "  - COMMENTS: " << ((flags & 0x08) ? "Yes" : "No") << std::endl;
    
    // Read ncolors
    uint8_t ncolors = read_u8(fp);
    std::cout << "Channels: " << (int)ncolors << std::endl;
    
    // Read pixelbits
    uint8_t pixelbits = read_u8(fp);
    std::cout << "Pixel Bits: " << (int)pixelbits << std::endl;
    
    // Read ncmap, cmaplen
    uint8_t ncmap = read_u8(fp);
    uint8_t cmaplen = read_u8(fp);
    std::cout << "Colormap: " << (int)ncmap << " x " << (int)cmaplen << std::endl;
    
    // Skip 5 reserved bytes
    fseek(fp, 5, SEEK_CUR);
    
    // Read background color (if present)
    if (!(flags & 0x02)) {
        std::cout << "Background: ";
        for (int i = 0; i < ncolors; i++) {
            uint16_t bg = read_u16(fp);
            std::cout << bg;
            if (i < ncolors - 1) std::cout << ", ";
        }
        std::cout << std::endl;
    }
    
    // Read colormap (if present)
    if (ncmap > 0 && cmaplen > 0) {
        std::cout << "Colormap present: " << (int)ncmap << " channels, " 
                  << (1 << cmaplen) << " entries" << std::endl;
        
        // Skip colormap data
        int cmap_entries = 1 << cmaplen;
        for (int c = 0; c < ncmap; c++) {
            for (int e = 0; e < cmap_entries; e++) {
                read_u16(fp);
            }
        }
    }
    
    // Read comments (if present)
    if (flags & 0x08) {
        std::cout << "\nComments:" << std::endl;
        while (true) {
            uint16_t type = read_u16(fp);
            if (type == 0) break; // End of comments
            
            uint16_t length = read_u16(fp);
            if (length == 0) continue;
            
            std::vector<char> value(length + 1, 0);
            if (fread(value.data(), 1, length, fp) != length) {
                std::cerr << "Failed to read comment value" << std::endl;
                break;
            }
            
            // Align to even boundary
            if (length % 2 == 1) {
                fgetc(fp);
            }
            
            std::cout << "  [Type " << type << "]: " << value.data() << std::endl;
        }
    }
    
    long data_start = ftell(fp);
    std::cout << "\nData starts at offset: " << data_start << " (0x" 
              << std::hex << data_start << std::dec << ")" << std::endl;
    
    fclose(fp);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <rle-file> [<rle-file2> ...]" << std::endl;
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        dump_rle_header(argv[i]);
        if (i < argc - 1) {
            std::cout << "\n\n" << std::endl;
        }
    }
    
    return 0;
}
