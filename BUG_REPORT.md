# Critical Bug Report: Y-Coordinate Issue in RLE Implementation

## ✅ STATUS: FIXED (commit 394ad28)

## Summary
~~Pixel data is read in reverse Y-order. When an image is written and read back, row 0 contains data from row H-1, row 1 from row H-2, etc.~~

**ACTUAL BUG:** Decoder was not incrementing scan_y between scanlines. Only the last scanline was being decoded correctly, with all previous scanlines remaining at default values (zeros). This created the appearance of Y-reversal in tests.

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

### Initial Hypothesis (INCORRECT)
Initially suspected Y-coordinate flipping issue based on RLE spec saying Y=0 is bottom. Investigation showed this was wrong.

### Actual Root Cause (CORRECT)
The RLE decoder was missing critical logic to advance between scanlines:

1. **RLE Format:** Scanlines are delimited implicitly by SET_COLOR opcodes
2. **Convention:** When SET_COLOR(0) appears after other channels, it signals a new scanline
3. **Bug:** Decoder set scan_x = 0 on SET_COLOR but never incremented scan_y
4. **Effect:** All scanlines were decoded to y=0, overwriting each other repeatedly
5. **Result:** Only the last scanline survived, all others remained at default (zero)

### The Fix
```cpp
case OPC_SET_COLOR: {
    int new_channel = (ch == 255 && h.has_alpha()) ? h.ncolors : int(ch);
    // If we're moving to channel 0 after having processed other channels,
    // it means we've finished the previous scanline
    if (new_channel == 0 && current_channel >= 0) {
        ++scan_y;
    }
    current_channel = new_channel;
    scan_x = xmin;
}
```

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

## Investigation Process

### Testing Utah RLE Convention
Created tests to understand Utah RLE's actual Y-coordinate convention:
- Utah RLE writes scanlines from ymin to ymax sequentially
- Utah RLE reads them back in the same order
- Scanline y=0 corresponds to memory row 0 (top, not bottom)
- **Conclusion:** Despite RLE spec theory, implementations use y=0 = TOP

### Discovering the Real Bug
1. Verified encoder writes scanlines in correct order (y=0, y=1, y=2, y=3)
2. Verified pixel() accessor returns correct memory addresses
3. Added debug output to decoder - found scan_y never incremented!
4. Only last scanline was being decoded, overwriting position 0 repeatedly

### Solution Implemented
Added scan_y increment logic in SET_COLOR opcode handler. No Y-flipping needed.

## Test Results After Fix

### All Tests Passing ✅
- **Main test suite:** 22/22 passing (was 21/22)
- **Coverage tests:** 18/18 passing
- **Alpha channel:** Now works perfectly

### Pixel-Level Validation ✅
All test cases now return correct data:
- `test_16x16.cpp`: All 256 pixels match expected Y-gradient pattern
- `test_4x4`: All 4 rows have correct unique G values  
- `test_rgb_minimal.cpp`: 2x2 RGB pixels all match exactly
- `test_alpha_minimal.cpp`: 2x2 RGBA including alpha values match exactly
- `teapot.rle`: Utah RLE compatibility maintained

## Files Added for Testing and Debugging
- `test_16x16.cpp` - 16x16 with Y-gradient pattern
- `test_4x4` - Minimal 4x4 test case
- `test_rgb_minimal.cpp` - 2x2 RGB test
- `test_alpha_minimal.cpp` - 2x2 RGBA test  
- `find_bug.cpp` - Instrumented test that isolated the decoder bug
- `test_decoder_debug.cpp` - Manual decoder trace with debug output
- `test_encoder_debug.cpp` - Verified encoder writes correctly
- `test_compare_implementations.cpp` - Compared our impl vs Utah RLE
- `test_utah_convention.cpp` - Determined Utah RLE's Y-coordinate convention

These tests exposed the bug and verified the fix.
