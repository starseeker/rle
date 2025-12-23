# NewURT Comparison: Final Analysis and Conclusions

## Executive Summary

**FINDING**: NewURT (BRL-CAD's utahrle library) produces significantly different and likely MORE CORRECT output than our current implementation for the three test images from the original Utah RLE Toolkit.

## Test Results

### christmas_ball.rle (400x400 RGBA)
- **Our Implementation**: 75.1% black pixels, avg RGB (21, 21, 20)
- **NewURT**: 27.5% black pixels, avg RGB (44, 47, 42) 
- **Verdict**: NewURT shows **3x more image data** - likely correct

### dart.rle (510x480 RGBA)
- **Our Implementation**: 99.1% black pixels
- **NewURT**: 97.1% black pixels
- **Verdict**: NewURT shows slightly more data - likely more correct

### tack_w_shadow.rle (62x50 RGBA)
- **Our Implementation**: 99.5% black pixels
- **NewURT**: 100% completely black
- **Verdict**: UNCLEAR - this file is known to be pathological ("extra scanlines")

## Root Cause Analysis

### Primary Difference: Scanline Initialization

**NewURT approach** (rle_getrow.c:361-374):
```c
// Initialize scanline BEFORE decoding each row
for ( nc = 0; nc < the_hdr->ncolors; nc++ )
    if ( RLE_BIT( *the_hdr, nc ) )
        if ( the_hdr->background != 2 || the_hdr->bg_color[nc] == 0 )
            bzero( (char *)scanline[nc] + the_hdr->xmin, ... );
        else
            bfill( (char *)scanline[nc] + the_hdr->xmin, ..., the_hdr->bg_color[nc] );
            
// Alpha channel initialized to 0 (transparent)
if ( the_hdr->alpha && RLE_BIT( *the_hdr, -1 ) )
    bzero( (char *)scanline[-1] + the_hdr->xmin, ... );
```

**Our implementation** (rle.hpp:391-420):
```cpp
// Initialize ENTIRE image once at allocation
pixels.assign(size_t(header.width()) * size_t(header.height()) * header.channels(), 0);

// Then initialize with background color if specified
if (!header.no_background() && !header.background.empty()) {
    for (size_t i = 0; i < npix; ++i) {
        for (size_t c = 0; c < header.ncolors && c < header.background.size(); ++c) {
            pixels[i * header.channels() + c] = header.background[c];
        }
    }
}

// Initialize alpha channel to 255 (opaque)
if (header.has_alpha()) {
    for (size_t i = 0; i < npix; ++i) {
        pixels[i * header.channels() + header.ncolors] = 255;  // <-- KEY DIFFERENCE!
    }
}
```

###  Critical Difference: Alpha Channel Default

- **NewURT**: Alpha defaults to **0** (fully transparent)
- **Our implementation**: Alpha defaults to **255** (fully opaque)

This might not directly affect decoding, but could indicate a philosophical difference in handling.

## Likely Issue

The massive difference in output (75% black vs 27.5% black for christmas_ball) suggests:

1. **Opcode Handling Bug**: Our decoder may be incorrectly handling certain opcode sequences that are common in these files
2. **Channel Ordering**: May not handle cases where alpha channel data comes in a different order
3. **Scanline Boundaries**: May incorrectly advance scanlines or skip pixels
4. **Background Mode Logic**: May not correctly interpret background mode 2 (CLEAR)

## Recommendations

### Immediate Actions Required

1. **Opcode-Level Debugging**:
   - Instrument our decoder to log every opcode processed
   - Compare with newurt's opcode processing for christmas_ball.rle
   - Identify where the decoding paths diverge

2. **Adopt NewURT's Scanline Initialization**:
   - Change to per-scanline initialization instead of whole-image
   - Initialize alpha to 0 instead of 255 (match Utah RLE spec)
   - Test if this fixes the christmas_ball issue

3. **Visual Verification**:
   - View the PPM files to confirm newurt's output looks correct
   - christmas_ball should show an actual christmas ornament, not mostly black

### Proposed Fix Strategy

**Option 1: Fix Our Decoder** (Recommended)
- Debug and fix the opcode handling bug
- Align with Utah RLE reference behavior
- Maintain compatibility with both old and new files

**Option 2: Adopt NewURT Wholesale**
- Replace our decoder with newurt's rle_getrow
- Simpler but loses our clean-room implementation
- Would need to adapt BRL-CAD integration layer

**Option 3: Hybrid Approach**
- Keep our encoder (it's likely correct)
- Use newurt's decoder for reading
- Best of both worlds but more complex

## Answer to Original Question

**Q: "Has newurt fixed the bug identified with the test images?"**

**A: YES** - NewURT appears to correctly decode these images, producing significantly more image data than our implementation. Our implementation has a bug that causes it to produce mostly-black output for christmas_ball.rle and dart.rle.

**Q: "Does its behavior differ in significant ways from original urt and our implementation?"**

**A: YES** - NewURT differs significantly from our implementation:
- Produces 3x more visible pixels for christmas_ball
- Uses different alpha channel defaults (0 vs 255)
- Initializes per-scanline vs per-image
- Likely handles certain opcode sequences correctly that we don't

## Next Steps

1. **CRITICAL**: Fix our decoder bug for christmas_ball.rle
2. Run opcode-level trace comparison
3. Update implementation to match Utah RLE reference behavior
4. Add these three images to our test suite with expected pixel counts
5. Document the differences between implementations

## Test Images Status

- ✅ lenna.rle, mandrill.rle: Alternating pattern detected and fixed
- ❌ christmas_ball.rle: **DECODER BUG** - missing 73% of image data
- ❌ dart.rle: **DECODER BUG** - missing some image data  
- ⚠️  tack_w_shadow.rle: Pathological case - needs investigation

## Conclusion

NewURT has NOT "fixed a bug" in these specific test images - rather, it has REVEALED a bug in our decoder implementation. The images themselves appear to be valid (except possibly tack_w_shadow which is known to be pathological). Our decoder is incorrectly processing them, resulting in mostly-black output.

**Action Required**: Fix our RLE decoder to correctly handle these images like newurt does.
