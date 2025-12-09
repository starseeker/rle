#include "rle.hpp"
#include <cstdio>
#include <cstring>

// Modified decoder with tracing
class TracingDecoder {
public:
    static rle::DecoderResult read_with_trace(FILE* f, rle::Image& img) {
        rle::DecoderResult res;
        if (!f) { res.error = rle::Error::INTERNAL_ERROR; return res; }
        
        rle::Header h; rle::Endian e; rle::Error herr;
        if (!rle::read_header_auto(f, h, e, herr)) { res.error = herr; return res; }
        img.header = h;
        
        rle::Error aerr;
        if (!img.allocate(aerr)) { res.error = aerr; return res; }
        
        const uint32_t W = h.width();
        const uint32_t H = h.height();
        const uint8_t chans = h.channels();
        const uint32_t xmin = h.xpos, xmax = xmin + W;
        const uint32_t ymin = h.ypos, ymax = ymin + H;
        
        uint32_t scan_x = xmin, scan_y = ymin;
        int current_channel = -1;
        uint64_t opsThisRow = 0;
        
        while (true) {
            if (++opsThisRow > uint64_t(rle::MAX_OPS_PER_ROW_FACTOR) * W * H) {
                res.error = rle::Error::OP_COUNT_EXCEEDED;
                return res;
            }
            
            uint8_t opc;
            if (!rle::read_u8(f, opc)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; }
            
            bool longForm = (opc & rle::OPC_LONG_FLAG) != 0;
            uint8_t base = opc & ~rle::OPC_LONG_FLAG;
            uint8_t op1 = 0;
            if (!longForm && base != rle::OPC_EOF) {
                if (!rle::read_u8(f, op1)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; }
            }
            
            printf("Opcode: 0x%02x (base=0x%02x, long=%d, op1=%d) scan_y=%d current_channel=%d\n", 
                   opc, base, longForm, op1, scan_y, current_channel);
            
            switch (base) {
                case rle::OPC_SKIP_LINES: {
                    uint16_t lines;
                    if (longForm) { if (!rle::read_u16(f, e, lines)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; } }
                    else lines = op1;
                    // If we were in the middle of a scanline, complete it first
                    if (current_channel >= 0) {
                        printf("  SKIP_LINES(%d): Completing current line, scan_y %d -> %d\n", lines, scan_y, scan_y + 1);
                        ++scan_y;
                    }
                    printf("  SKIP_LINES(%d): scan_y %d -> %d, current_channel -> -1\n", lines, scan_y, scan_y + lines);
                    scan_y += lines; scan_x = xmin; current_channel = -1;
                    continue;
                }
                case rle::OPC_SET_COLOR: {
                    if (longForm) { res.error = rle::Error::OPCODE_UNKNOWN; return res; }
                    uint16_t ch = op1;
                    int new_channel = (ch == 255 && h.has_alpha()) ? h.ncolors : int(ch);
                    
                    if (new_channel == 0 && current_channel >= 0) {
                        printf("  SET_COLOR(%d): Advancing scan_y %d -> %d (end of prev line)\n", ch, scan_y, scan_y + 1);
                        ++scan_y;
                    } else {
                        printf("  SET_COLOR(%d): channel %d -> %d, scan_y stays %d\n", ch, current_channel, new_channel, scan_y);
                    }
                    current_channel = new_channel;
                    scan_x = xmin;
                } break;
                case rle::OPC_SKIP_PIXELS: {
                    uint16_t skip;
                    if (longForm) { if (!rle::read_u16(f, e, skip)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; } }
                    else skip = op1;
                    printf("  SKIP_PIXELS(%d)\n", skip);
                    scan_x += skip;
                } break;
                case rle::OPC_BYTE_DATA: {
                    uint16_t enc;
                    if (longForm) { if (!rle::read_u16(f, e, enc)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; } }
                    else enc = op1;
                    uint32_t count = uint32_t(enc) + 1;
                    printf("  BYTE_DATA(%d bytes)\n", count);
                    uint32_t remaining = (xmin + W > scan_x) ? (xmin + W - scan_x) : 0;
                    uint32_t to_write = (count < remaining) ? count : remaining;
                    uint32_t to_discard = count - to_write;
                    for (uint32_t i = 0; i < to_write; ++i) {
                        uint8_t pv;
                        if (!rle::read_u8(f, pv)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; }
                        if (current_channel >= 0 && current_channel < int(chans))
                            img.pixel(scan_x - xmin, scan_y - ymin)[current_channel] = pv;
                        ++scan_x;
                    }
                    for (uint32_t i = 0; i < to_discard; ++i) {
                        uint8_t tmp;
                        if (!rle::read_u8(f, tmp)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; }
                        ++scan_x;
                    }
                    if (count & 1) { uint8_t filler; if (!rle::read_u8(f, filler)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; } }
                } break;
                case rle::OPC_RUN_DATA: {
                    uint16_t enc;
                    if (longForm) { if (!rle::read_u16(f, e, enc)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; } }
                    else enc = op1;
                    uint32_t run_len = uint32_t(enc) + 1;
                    uint16_t word;
                    if (!rle::read_u16(f, e, word)) { res.error = rle::Error::TRUNCATED_OPCODE; return res; }
                    uint8_t pv = uint8_t(word & 0xFF);
                    printf("  RUN_DATA(%d pixels, value=%d)\n", run_len, pv);
                    uint32_t remaining = (xmin + W > scan_x) ? (xmin + W - scan_x) : 0;
                    uint32_t to_write = (run_len < remaining) ? run_len : remaining;
                    uint32_t to_skip = run_len - to_write;
                    for (uint32_t i = 0; i < to_write; ++i) {
                        if (current_channel >= 0 && current_channel < int(chans))
                            img.pixel(scan_x - xmin, scan_y - ymin)[current_channel] = pv;
                        ++scan_x;
                    }
                    scan_x += to_skip;
                } break;
                case rle::OPC_EOF:
                    printf("  EOF\n");
                    res.ok = true; res.error = rle::Error::OK; res.endian = e; return res;
                default:
                    res.error = rle::Error::OPCODE_UNKNOWN; return res;
            }
        }
        res.ok = true; res.error = rle::Error::OK; res.endian = e; return res;
    }
};

int main() {
    // Simple test: 20 rows, rows 5-14 are background
    rle::Image img;
    img.header.xpos = 0; img.header.ypos = 0;
    img.header.xlen = 10; img.header.ylen = 20;
    img.header.ncolors = 3; img.header.pixelbits = 8;
    img.header.ncmap = 0; img.header.cmaplen = 0;
    img.header.background = {100, 150, 200};
    img.header.flags = 0;  // Background enabled
    
    rle::Error err;
    if (!img.allocate(err)) {
        fprintf(stderr, "Allocate failed\n");
        return 1;
    }
    
    // Rows 0-4: Pattern A (50, 75, 25)
    for (uint32_t y = 0; y < 5; y++) {
        for (uint32_t x = 0; x < 10; x++) {
            img.pixel(x, y)[0] = 50;
            img.pixel(x, y)[1] = 75;
            img.pixel(x, y)[2] = 25;
        }
    }
    // Rows 5-14: Background (100, 150, 200) - already set
    // Rows 15-19: Pattern B (200, 100, 50)
    for (uint32_t y = 15; y < 20; y++) {
        for (uint32_t x = 0; x < 10; x++) {
            img.pixel(x, y)[0] = 200;
            img.pixel(x, y)[1] = 100;
            img.pixel(x, y)[2] = 50;
        }
    }
    
    // Write with BG_OVERLAY
    FILE* f = fopen("/tmp/test_skip.rle", "wb");
    if (!f) return 1;
    rle::Error werr;
    if (!rle::Encoder::write(f, img, rle::Encoder::BG_OVERLAY, werr)) {
        fprintf(stderr, "Write failed: %s\n", rle::error_string(werr));
        fclose(f);
        return 1;
    }
    fclose(f);
    
    // Read back with tracing
    f = fopen("/tmp/test_skip.rle", "rb");
    if (!f) return 1;
    rle::Image out;
    rle::DecoderResult res = TracingDecoder::read_with_trace(f, out);
    fclose(f);
    
    if (!res.ok) {
        fprintf(stderr, "Read failed: %s\n", rle::error_string(res.error));
        return 1;
    }
    
    printf("\n===== Result comparison =====\n");
    for (uint32_t y = 0; y < 20; y++) {
        printf("Row %2d: R=%3d G=%3d B=%3d%s\n", y, 
               out.pixel(0, y)[0], out.pixel(0, y)[1], out.pixel(0, y)[2],
               (out.pixel(0, y)[0] != img.pixel(0, y)[0] ||
                out.pixel(0, y)[1] != img.pixel(0, y)[1] ||
                out.pixel(0, y)[2] != img.pixel(0, y)[2]) ? " <== MISMATCH" : "");
    }
    
    return 0;
}
