# Comparison of RLE Decoder Implementations

## Test Images

We tested three images from the original Utah RLE Toolkit:
1. **christmas_ball.rle** - 400x400, RGBA
2. **dart.rle** - 510x480, RGBA
3. **tack_w_shadow.rle** - 62x50, RGBA

## Findings

### Differences Between Current Implementation and NewURT

All three images show pixel-level differences between our current implementation and newurt (BRL-CAD's utahrle):

#### christmas_ball.rle
- Different pixels: 118,295 / 160,000 (73.9%)
- Max difference: 255
- Average absolute difference: 61.52
- Pattern: Widespread differences across most rows (98.8% of rows affected)

Example pixel (66, 1):
- Current implementation: RGB(2, 2, 2)
- NewURT: RGB(0, 0, 0)

#### dart.rle
- Different pixels: 9,282 / 244,800 (3.8%)
- Max difference: 255
- Average absolute difference: 54.86
- Pattern: Localized differences (52.1% of rows affected)

Example pixel (127, 72):
- Current implementation: RGB(0, 0, 0)
- NewURT: RGB(10, 47, 10)

#### tack_w_shadow.rle
- Different pixels: 15 / 3,100 (0.48%)
- Max difference: 57
- Average absolute difference: 24.87
- Pattern: Very localized (only 3 rows affected)

Example pixel (36, 41):
- Current implementation: RGB(9, 9, 9)
- NewURT: RGB(0, 0, 0)

## Analysis

### Key Observations

1. **Not Alternating Pattern**: Unlike the lenna.rle/mandrill.rle issue, these differences are NOT in an alternating row pattern. This confirms the problem statement's suspicion that this is a different issue.

2. **Bidirectional Differences**: 
   - In christmas_ball: Current impl has non-zero where newurt has zero
   - In dart: Newurt has non-zero where current impl has zero
   - This suggests the implementations handle certain opcodes or edge cases differently

3. **Severity Varies**: christmas_ball has massive differences (74%), dart has moderate (3.8%), tack has minimal (0.5%)

### Potential Causes

1. **Background Initialization**: 
   - NewURT reinitializes each scanline before decoding
   - Current implementation initializes entire image once
   - Both use background color RGB(0,0,0) for these files

2. **Alpha Channel Handling**:
   - All three images have alpha channels
   - May affect how RGB channels are read or initialized

3. **Opcode Interpretation**:
   - Different handling of SKIP_PIXELS or channel boundaries
   - Possible off-by-one errors in pixel positioning

4. **Background Mode**:
   - NewURT has special logic for background mode 2 (CLEAR)
   - Checks if bg_color is 0 before deciding whether to use it

## Recommendations

### Further Investigation Needed

1. **Opcode-Level Tracing**: 
   - Trace exact opcode sequence for affected pixels
   - Compare how each implementation processes the same opcodes

2. **Original URT Comparison**:
   - Build converter using original URT library
   - Compare all three implementations (current, newurt, original)
   - Determine which is "correct"

3. **Visual Inspection**:
   - View the PPM output images
   - Determine which version looks visually correct
   - Check against any reference images if available

4. **Specification Review**:
   - Review Utah RLE specification for background handling
   - Check alpha channel interaction with RGB
   - Verify correct opcode semantics

### Questions to Answer

1. **Which implementation is correct?**
   - Is newurt fixing a bug in our implementation?
   - Or is newurt introducing a bug that our implementation avoids?
   - Or are both implementations handling edge cases differently, each with pros/cons?

2. **What is the root cause?**
   - Background initialization timing?
   - Alpha channel blending?
   - Opcode boundary conditions?

3. **What should we do?**
   - Adopt newurt's behavior?
   - Keep current implementation?
   - Create hybrid approach that handles both cases?

## Next Steps

1. Create visual comparison by viewing the PPM files
2. Build original URT converter for three-way comparison
3. Trace opcode-level decoding for specific differing pixels
4. Make determination of correct behavior
5. Implement fix if needed
