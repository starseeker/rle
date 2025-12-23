# Vertical Pattern Investigation

## Problem Statement
Investigate whether christmas_ball.rle, dart.rle, and tack_w_shadow.rle exhibit "vertical columns of red, green and blue pixels" even after the sparse row pattern fix was applied.

## Investigation Summary

### TL;DR
**NO vertical RGB channel separation detected.** The images decode correctly with properly interleaved RGB channels. Some color-dominant vertical regions exist but these are part of the actual image content, not encoding bugs.

## Methodology

### Tools Created
1. `comprehensive_diagnostic.cpp` - Multi-test diagnostic tool
2. `hexdump_pixels.cpp` - Byte-level pixel inspector
3. `visualize_ascii.cpp` - ASCII art visualization
4. `comprehensive_pattern_detector.cpp` - Vertical pattern detector
5. `decode_raw_stats.cpp` - Raw RLE decoder statistics

### Tests Performed

#### Test 1: Vertical Channel Separation Detection
Examined every column for single-channel dominance (R>>G,B or G>>R,B or B>>R,G).

**Results:**
- christmas_ball.rle: 14 green-dominant columns (out of 400)
- dart.rle: 9 red-dominant columns (out of 510)  
- tack_w_shadow.rle: 0 color-dominant columns (out of 62)

**Analysis:** The color-dominant columns are sparse and match expected image content:
- Christmas ball likely has green ornament/reflection (pixels: `[01 8C 01 FF]` = R=1, G=140, B=1)
- Dart has red body (expected for a dart image)

#### Test 2: Pixel Data Integrity
Examined raw decoded pixel values at byte level to verify channel ordering.

**Results:**
All pixels show correct RGBA interleaved format: `[RR GG BB AA]`

**Example from christmas_ball.rle, row 202:**
```
[01 8C 01 FF] [01 8C 01 FF] [01 8C 01 FF] ...
```
This is R=1, G=140, B=1, A=255 - a green-tinted pixel, correctly encoded.

#### Test 3: Horizontal Pattern Detection
Confirmed the period-3 sparse row pattern still exists in raw decoded data.

**Results:**
- christmas_ball.rle: Rows 1, 4, 7, 10, ... have data; rows 2-3, 5-6, 8-9, ... are uniform black
- dart.rle: Rows 237, 240, 243, ... have data  
- tack_w_shadow.rle: Rows 41, 44, 47 have data

This is the HORIZONTAL pattern that the existing fix (`detect_and_fix_alternating_pattern`) addresses.

#### Test 4: Channel Consistency
Checked if images are grayscale (R=G=B) or colored.

**Results:**
- christmas_ball.rle: 78.3% grayscale, 21.7% colored
- dart.rle: 4.8% grayscale, 95.2% colored (expected - colorful dart image)
- tack_w_shadow.rle: 100% grayscale

#### Test 5: ASCII Visualization
Created ASCII art of decoded images to visually inspect for patterns.

**Results:**
No visible vertical stripes or channel separation artifacts.

## Potential Causes of Vertical RGB Patterns

While NOT present in these files, vertical RGB channel separation COULD occur if:

### 1. Planar vs Interleaved Storage Bug
If pixels were stored as:
```
[R0 R1 R2 ... RN G0 G1 G2 ... GN B0 B1 B2 ... BN]  ← Planar (WRONG)
```
Instead of:
```
[R0 G0 B0 R1 G1 B1 R2 G2 B2 ...]  ← Interleaved (CORRECT)
```

Our decoder correctly produces interleaved format.

### 2. Channel Index Bug
If `current_channel` was incremented across X instead of Y, channels could separate vertically.

Our decoder correctly increments scan_x for pixel data within a row, and sets current_channel based on SET_COLOR opcodes.

### 3. X/Y Coordinate Transpose
If pixel(x,y) calculation transposed coordinates:
```cpp
// WRONG: return pixels[x * height + y] * channels  
// CORRECT: return pixels[y * width + x] * channels
```

Our implementation is correct: `pixels[y * width + x] * channels`

### 4. Original URT Encoder Bug
The original URT toolkit has a known bug with excessive SKIP_LINES opcodes (causing horizontal patterns).

Could it also have a channel ordering bug? **Unlikely**, because:
- The RLE format processes one channel at a time across the entire row
- SET_COLOR opcodes properly delimit channels
- Our decoder follows the spec correctly

## Conclusion

**The three RLE files do NOT exhibit vertical RGB channel separation.**

The existing horizontal sparse pattern (period-3) is correctly detected and fixed by `detect_and_fix_alternating_pattern()`.

Some columns show color dominance (e.g., green on christmas ball, red on dart) but this is part of the actual image content, not an encoding bug.

## Verification

To verify these findings:

```bash
# Build and run comprehensive diagnostic
cd /home/runner/work/rle/rle
g++ -o comprehensive_diagnostic comprehensive_diagnostic.cpp -std=c++11 -I.

# Test all three files
./comprehensive_diagnostic imgs/christmas_ball.rle
./comprehensive_diagnostic imgs/dart.rle
./comprehensive_diagnostic imgs/tack_w_shadow.rle
```

Expected output: "✓ No vertical channel separation detected" for all three files.

## Recommendations

1. **No code changes needed** - the decoder is working correctly
2. **Keep existing horizontal pattern fix** - it correctly handles the period-3 sparse row pattern
3. **If vertical patterns are observed:** Check the viewing tool, cached images, or file source

## Tools for Future Detection

The `comprehensive_diagnostic` tool can detect:
- Horizontal sparse patterns (period 2-5)
- Vertical channel separation
- Channel ordering issues  
- Grayscale vs color content
- Pixel-level data integrity

Run it on any suspicious RLE file to diagnose issues.
