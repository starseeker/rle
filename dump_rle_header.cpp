/**
 * @file dump_rle_header.cpp
 * @brief Utility to dump RLE file header information
 */

#include "rle.hpp"
#include <iostream>
#include <cstdio>

int main(int argc, char** argv) {
    const char* filename = (argc > 1) ? argv[1] : "teapot.rle";
    
    std::cout << "Reading RLE header from: " << filename << "\n\n";
    
    FILE* fp = std::fopen(filename, "rb");
    if (!fp) {
        std::cout << "ERROR: Could not open " << filename << "\n";
        return 1;
    }
    
    rle::Header h;
    rle::Endian e;
    rle::Error err;
    
    if (!rle::read_header_auto(fp, h, e, err)) {
        std::cout << "ERROR: Failed to read header: " << rle::error_string(err) << "\n";
        std::fclose(fp);
        return 1;
    }
    
    std::cout << "Header Information:\n";
    std::cout << "  Magic: 0x" << std::hex << rle::RLE_MAGIC << std::dec << "\n";
    std::cout << "  Endian: " << (e == rle::Endian::Little ? "Little" : "Big") << "\n";
    std::cout << "  Position (xpos, ypos): (" << h.xpos << ", " << h.ypos << ")\n";
    std::cout << "  Dimensions (xlen, ylen): (" << h.xlen << ", " << h.ylen << ")\n";
    std::cout << "  Width: " << h.width() << "\n";
    std::cout << "  Height: " << h.height() << "\n";
    std::cout << "  Channels: " << (int)h.channels() << "\n";
    std::cout << "  Ncolors: " << (int)h.ncolors << "\n";
    std::cout << "  Pixelbits: " << (int)h.pixelbits << "\n";
    std::cout << "  Flags: 0x" << std::hex << (int)h.flags << std::dec << "\n";
    std::cout << "    - CLEAR_FIRST: " << (h.clear_first() ? "yes" : "no") << "\n";
    std::cout << "    - NO_BACKGROUND: " << (h.no_background() ? "yes" : "no") << "\n";
    std::cout << "    - ALPHA: " << (h.has_alpha() ? "yes" : "no") << "\n";
    std::cout << "    - COMMENT: " << (h.has_comments() ? "yes" : "no") << "\n";
    
    if (!h.no_background()) {
        std::cout << "  Background color: [";
        for (size_t i = 0; i < h.background.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << (int)h.background[i];
        }
        std::cout << "]\n";
    }
    
    if (h.ncmap > 0) {
        std::cout << "  Colormap: " << (int)h.ncmap << " channels x " 
                  << (1 << h.cmaplen) << " entries = " << h.colormap.size() << " values\n";
    }
    
    if (h.has_comments()) {
        std::cout << "  Comments (" << h.comments.size() << "):\n";
        for (const auto& comment : h.comments) {
            std::cout << "    - " << comment << "\n";
        }
    }
    
    std::fclose(fp);
    
    return 0;
}
