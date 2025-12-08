# Critical Bug Report: Y-Coordinate Reversal in RLE Implementation

## Summary
Pixel data is read in reverse Y-order. When an image is written and read back, row 0 contains data from row H-1, row 1 from row H-2, etc.

## Evidence

### Test Case 1: 16x16 Image with Y-gradient
```cpp
// Each row has unique G value: row 0 => G=0, row 1 => G=16, ..., row 15 => G=240
for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
        data[(y * 16 + x) * 3 + 1] = y * 16;  // G varies with Y
    }
}
```

**Expected after roundtrip:** Row 0 has G=0  
**Actual after roundtrip:** Row 0 has G=240 (from row 15)

### Test Case 2: 2x2 Image with Distinct Colors
```
Original memory layout:
  Row 0: Red, Green
  Row 1: Blue, Yellow

Read back:
  Row 0: Blue, Yellow  (from original row 1)
  Row 1: Black, Black  (defaults)
```

### Test Case 3: 4x4 Image
All rows read back as G=0 (first row's value repeated)

## Root Cause Analysis

### RLE Format Specification
- Scanlines are numbered Y=0 (bottom) to Y=H-1 (top)
- Y-axis increases upward (standard graphics coordinates)
- Scanline Y=0 should contain the bottom row of the visible image

### Our Memory Layout
- Arrays index from 0 to H-1
- Conventionally, index 0 is the top row
- Index H-1 is the bottom row

### The Bug
Neither encoder nor decoder performs Y-coordinate flipping, resulting in:
1. **Encoder:** Writes memory row 0 as scanline Y=0 (but row 0 is top, scanline 0 should be bottom)
2. **Decoder:** Reads scanline Y=0 into memory row 0 (but scanline 0 is bottom, should go to row H-1)

**Net Effect:** When we write and read our own files, the image gets vertically flipped.

## Why Existing Tests Passed

### Utah RLE Compatibility Tests
The teapot test "passes" because:
1. Teapot.rle was likely written with Y=0 as top (common convention)
2. Our decoder reads Y=0 into row 0
3. Both use the same (incorrect per spec) convention
4. Pixels match because both are consistently "wrong"

### Roundtrip Tests
None of the existing roundtrip tests validated pixel values:
- `test_bidirectional_roundtrip()` - only checks dimensions
- `test_utahrle_reads_our_file()` - only checks read succeeds
- `test_simple_roundtrip()` - only checks dimensions

## Impact

### Current Behavior
- ✅ Can read Utah RLE files (if they use top=0 convention)
- ✅ Utah RLE can read our files (we write top=0)
- ❌ Pixel data is incorrect when validated
- ❌ Images appear vertically flipped
- ❌ Incompatible with RLE spec if interpreted strictly

### Affects
- All image writes (RGB and RGBA)
- All image reads  
- Both `write_rgb()`/`read_rgb()` and `rle_write()`/`rle_read()`

## Proposed Solutions

### Option 1: Flip in Encoder Only
```cpp
// In Encoder::write(), line ~458
uint32_t mem_row = H - 1 - y;  // Write bottom row first
// Use mem_row for all pixel accesses
```
**Pros:** Writes spec-compliant files  
**Cons:** Breaks reading existing Utah RLE files if they don't follow spec

### Option 2: Flip in Decoder Only
```cpp
// In Decoder::read(), when writing pixels
uint32_t mem_row = H - 1 - (scan_y - ymin);
```
**Pros:** Reads spec-compliant files correctly  
**Cons:** Breaks reading existing Utah RLE files

### Option 3: Flip in Both (Recommended)
Apply both encoder and decoder flips. This:
- Makes our files spec-compliant
- Reads spec-compliant files correctly
- **May break compatibility** with existing Utah RLE files if they don't follow spec

### Option 4: Convention Detection
Detect whether a file uses top=0 or bottom=0 convention:
- Check some heuristic (e.g., first scanline opcode patterns)
- Apply flip conditionally
**Pros:** Maximum compatibility  
**Cons:** Complex, error-prone

## Recommendation
1. Test how actual Utah RLE tools write files (do they flip or not?)
2. If Utah RLE uses top=0 convention despite spec, stick with it (no changes needed)
3. If Utah RLE follows spec (bottom=0), implement Option 3
4. Add pixel-level validation to ALL tests

## Reproduction Steps
```bash
cd /home/runner/work/rle/rle
g++ -std=c++11 -o test_16x16 test_16x16.cpp -I.
./test_16x16
# Observe: Row 0 gets row 15's data
```

## Files Added for Testing
- `test_16x16.cpp` - 16x16 with Y-gradient
- `test_4x4` - Minimal test
- `test_rgb_minimal.cpp` - 2x2 RGB test
- `test_alpha_minimal.cpp` - 2x2 RGBA test

All demonstrate the Y-reversal bug.
