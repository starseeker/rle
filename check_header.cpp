#include <cstdio>
#include <cstdint>
#include <iostream>

int main() {
    FILE* fp = fopen("our_test.rle", "rb");
    
    // Read magic
    uint8_t buf[16];
    fread(buf, 1, 16, fp);
    
    uint16_t magic = buf[0] | (buf[1] << 8);
    int16_t xpos = buf[2] | (buf[3] << 8);
    int16_t ypos = buf[4] | (buf[5] << 8);
    int16_t xlen = buf[6] | (buf[7] << 8);
    int16_t ylen = buf[8] | (buf[9] << 8);
    uint8_t flags = buf[10];
    uint8_t ncolors = buf[11];
    uint8_t pixelbits = buf[12];
    
    std::cout << "Header values:\n";
    std::cout << "  magic: 0x" << std::hex << magic << std::dec << "\n";
    std::cout << "  xpos: " << xpos << "\n";
    std::cout << "  ypos: " << ypos << "\n";
    std::cout << "  xlen: " << xlen << "\n";
    std::cout << "  ylen: " << ylen << "\n";
    std::cout << "  flags: 0x" << std::hex << int(flags) << std::dec << "\n";
    std::cout << "  ncolors: " << int(ncolors) << "\n";
    std::cout << "  pixelbits: " << int(pixelbits) << "\n";
    std::cout << "\nxmax = xpos + xlen - 1 = " << (xpos + xlen - 1) << "\n";
    std::cout << "ymax = ypos + ylen - 1 = " << (ypos + ylen - 1) << "\n";
    
    fclose(fp);
    return 0;
}
