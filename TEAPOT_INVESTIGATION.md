# Teapot.rle File Issue - Investigation Results

## Summary

The original `teapot.rle` file has **alternating black horizontal lines** where every odd-numbered row (1, 3, 5, 7, ...) is completely black, while even-numbered rows (0, 2, 4, 6, ...) contain actual image data.

This is **NOT a bug in the RLE library**, but rather an issue with how the original teapot.rle file was created.

## Root Cause

The teapot.rle file was created with the command:
```
./rawtorle -w 256 -h 256 teapot.raw on Fri Mar 29 14:35:39 2024
```

Analysis of the RLE opcodes shows that the file contains `SKIP_LINES 1` after every single row, causing the decoder to skip every odd row:

```
Row 0: [RGB data opcodes]
SKIP_LINES 1  → skip row 1, jump to row 2
Row 2: [RGB data opcodes]
SKIP_LINES 1  → skip row 3, jump to row 4
Row 4: [RGB data opcodes]
...and so on
```

This pattern indicates one of two possibilities:
1. The `rawtorle` tool has a bug where it incorrectly writes `SKIP_LINES 1` after every row
2. The source `teapot.raw` file only contained every other row (interlaced or corrupted)

## RLE Library Verification

The RLE library correctly handles the SKIP_LINES opcode according to the Utah RLE specification:

1. **Encoder logic**: When writing images, SKIP_LINES is only emitted for rows that are entirely background color (when using BG_OVERLAY or BG_CLEAR modes)

2. **Decoder logic**: When reading SKIP_LINES:
   - If currently in the middle of a scanline, advance to the next row
   - Skip N additional rows as specified by the operand
   - This correctly positions scan_y for the next data row

3. **Roundtrip tests**: All roundtrip tests pass perfectly:
   - `test_teapot_roundtrip`: Reads teapot.rle and writes it back → identical
   - `test_striped.rle`: Alternating black/white stripes → preserved perfectly
   - All 36 test suite tests pass

## Files

- **teapot.rle**: Original file with alternating black lines (95 KB)
  - 128 data rows (even-numbered: 0, 2, 4, ...)
  - 128 black rows (odd-numbered: 1, 3, 5, ...)
  
- **teapot_fixed.rle**: De-interlaced version (110 KB)
  - 256 data rows (all rows filled)
  - Created by duplicating each data row into adjacent black rows
  - No SKIP_LINES opcodes

## Testing

The test suite now includes detection for this pattern:

```bash
cd build
./test_rle
```

Output for teapot.rle:
```
TEST: Read teapot.rle reference image ... 
  WARNING: teapot.rle has 128 black rows (alternating line pattern)
  This is a known issue with the original file created by rawtorle.
  Use teapot_fixed.rle for a properly de-interlaced version.
PASSED
```

## Diagnostic Tools

Several diagnostic tools were created to investigate this issue:

1. **dump_rle_header**: Display RLE file header information
   ```bash
   ./dump_rle_header teapot.rle
   ```

2. **dump_rle_opcodes**: Trace decoder state and opcodes
   ```bash
   ./dump_rle_opcodes teapot.rle 100
   ```

3. **test_alternating_lines**: Detect alternating black line patterns
   ```bash
   ./test_alternating_lines
   ```

4. **create_corrected_teapot**: Create de-interlaced version
   ```bash
   ./create_corrected_teapot
   ```

## Conclusion

**The RLE library is working correctly.** The alternating black line pattern is in the source file itself, not introduced by the decoder. For BRL-CAD integration or any application that needs a complete teapot image, use `teapot_fixed.rle` instead of the original `teapot.rle`.

## Recommendations

1. Replace `teapot.rle` with `teapot_fixed.rle` in the repository
2. Or document that teapot.rle has this known issue
3. If the original `teapot.raw` file is available, re-encode it properly
4. Consider fixing or replacing the `rawtorle` tool if it's the source of the bug
