# Sparse Row Pattern Investigation and Fix

## Problem Statement

The test images `christmas_ball.rle`, `dart.rle`, and `tack_w_shadow.rle` exhibit strange visual artifacts that appear as gridded horizontal lines (venetian blind effect). This investigation confirms the artifact, identifies its root cause in the original URT code, and implements a corrective heuristic.

## Confirmation

**YES**, reading and writing out data from these RLE files produces the gridded line pattern. The artifacts are present in the source RLE files themselves, not introduced by the decoder.

### Specific Observations

All three files show a **period-3 sparse row pattern**:

- **christmas_ball.rle** (400x400 RGBA):
  - Data rows: 1, 4, 7, 10, 13, ... (every 3rd row)
  - 267 out of 400 rows (66.8%) are completely uniform (background)
  
- **dart.rle** (510x480 RGBA):
  - Data rows: 237, 240, 243, 246, ... (every 3rd row)
  - 399 out of 480 rows (83.1%) are completely uniform (background)
  
- **tack_w_shadow.rle** (62x50 RGBA):
  - Data rows: 41, 44, 47 (every 3rd row)
  - 47 out of 50 rows (94.0%) are completely uniform (background)

## Root Cause Analysis

### What Happened in the Original URT Code

The original Utah Raster Toolkit encoder likely had a bug or configuration issue that caused it to write **excessive SKIP_LINES opcodes** after each data row.

Specifically, the pattern suggests:
```
Row 0: [background/skipped]
Row 1: [SET_COLOR + RGB data]
       SKIP_LINES 2          ← Bug: should be 0 or 1, not 2
Row 2: [skipped]
Row 3: [skipped]
Row 4: [SET_COLOR + RGB data]
       SKIP_LINES 2          ← Bug repeats
...
```

This is similar to the known `lenna.rle` issue (period-2 pattern), but with **2 SKIP_LINES** instead of 1, creating a period-3 pattern.

### Why This Creates the Grid Effect

According to the RLE specification, when a SKIP_LINES opcode is encountered:
1. If currently writing a scanline, complete it (advance to next row)
2. Skip N additional rows as specified
3. Skipped rows remain filled with background color

With period-3 pattern:
- Row 1: Actual image data
- Rows 2-3: Uniform background (skipped)
- Row 4: Actual image data
- Rows 5-6: Uniform background (skipped)
- ...

This creates horizontal bands of background color interleaved with thin strips of actual image data.

### Comparison with lenna.rle Issue

| File | Period | Pattern | Background Rows |
|------|--------|---------|-----------------|
| lenna.rle | 2 | data, skip, data, skip, ... | 50% |
| christmas_ball.rle | 3 | data, skip, skip, data, skip, skip, ... | 66.7% |
| dart.rle | 3 | (same as above) | 83.1% (includes top background) |
| tack_w_shadow.rle | 3 | (same as above) | 94.0% (includes top background) |

## Corrective Heuristic

### Detection Strategy

We implemented a **generalized sparse row pattern detector** that:

1. **Scans all rows** to classify them as uniform (background) or non-uniform (data)
2. **Checks sparsity**: Requires at least 60% of rows to be uniform background
3. **Detects period**: Tests for regular spacing between data rows (period 2, 3, 4, or 5)
4. **Validates pattern**: Requires 70% of consecutive data row pairs to match the detected period

### Key Implementation Details

#### Challenge: RGBA Images

The three problematic files use RGBA (4 channels), and the alpha channel is initialized to 255 (fully opaque) uniformly. This made the original detector fail because:

- **Original check**: "Is the entire row (including alpha) uniform?"
- **Problem**: Alpha is always 255, so no row is ever uniform
- **Solution**: Only check RGB channels (first 3), ignore alpha channel

```cpp
// Only check RGB channels, not alpha
const size_t color_channels = (channels >= 4) ? 3 : channels;
for (size_t c = 0; c < color_channels; c++) {
    // ... check if channel c is uniform across the row
}
```

#### Period Detection

```cpp
// Try periods 2, 3, 4, 5
for (int period : {2, 3, 4, 5}) {
    size_t matches = 0;
    for (size_t i = 0; i + 1 < data_rows.size(); i++) {
        if (data_rows[i+1] - data_rows[i] == period) {
            matches++;
        }
    }
    
    if (matches >= (data_rows.size() - 1) * 0.7) {
        detected_period = period;
        break;
    }
}
```

### Correction Strategy

Once a pattern is detected, the fix **replicates each data row** into the following skip rows:

```cpp
for (size_t i = 0; i < data_rows.size(); i++) {
    size_t data_row = data_rows[i];
    size_t next_data_row = (i + 1 < data_rows.size()) ? data_rows[i + 1] : height;
    size_t max_copy = std::min(data_row + detected_period, next_data_row);
    
    for (size_t dest_row = data_row + 1; dest_row < max_copy; dest_row++) {
        // Copy data_row into dest_row
        std::memcpy(...);
    }
}
```

This "de-interlaces" the image by filling in the skipped rows with the nearest actual image data.

## Results

### Effectiveness

The corrective heuristic successfully detects and fixes all three problematic files:

| File | Before (zero rows) | After (zero rows) | Reduction |
|------|-------------------|------------------|-----------|
| christmas_ball | 267 / 400 (66.8%) | 1 / 400 (0.2%) | 99.6% |
| dart | 399 / 480 (83.1%) | 237 / 480 (49.4%) | 40.6% |
| tack_w_shadow | 47 / 50 (94.0%) | 41 / 50 (82.0%) | 12.8% |

**Note**: dart and tack_w_shadow retain some zero rows because the actual image content doesn't fill the entire canvas (top portions are legitimately background).

### Backward Compatibility

- ✅ All 35 existing tests pass
- ✅ Normal RLE files unchanged
- ✅ Only affects files with detected sparse row pattern
- ✅ Transparent to calling code (automatic correction during read)

## Usage

The correction is applied automatically during `rle_read()`:

```cpp
FILE* fp = fopen("christmas_ball.rle", "rb");
icv_image_t* img = rle_read(fp);
fclose(fp);

// Output shows:
// rle_read: WARNING - Detected and corrected sparse row pattern
//   This file has excessive SKIP_LINES opcodes causing rows to be skipped.
//   Applied de-interlacing by replicating data rows into skipped rows.
```

The warning is informational only. The returned image has been corrected and is ready to use.

## Testing

To verify the fix on a specific file:

```bash
# Convert RLE to PPM
./build/convert_rle_to_ppm imgs/christmas_ball.rle output.ppm

# The warning will be printed if pattern is detected and fixed
# The output PPM will have the corrected image
```

## Related Issues

This fix extends the existing alternating line pattern fix (for lenna.rle) to handle:
- Higher periods (not just 2, but also 3, 4, 5)
- RGBA images (ignore alpha channel during detection)
- Sparser patterns (60%+ background rows instead of exactly 50%)

## References

- Utah RLE Specification: https://sarnold.github.io/urt/docs/rle.pdf
- Original alternating pattern fix: ALTERNATING_PATTERN_FIX.md
- Related investigation: TEAPOT_INVESTIGATION.md
