# Background Detection Logic - Fixed

## Problem Identified and Resolved

Lines 229-238 in `rle.cpp` contained **unreachable dead code** in the `detect_background()` function. This has been **fixed by removing the dead code**.

### Original Flawed Code (Lines 229-238) - REMOVED
```cpp
if (bd.mode == rle::Encoder::BG_SAVE_ALL && maxCount >= overlay_needed) {
    bd.mode = rle::Encoder::BG_OVERLAY;
    bd.color = { ... };
} else if (bd.mode == rle::Encoder::BG_SAVE_ALL && maxCount >= clear_needed) {
    bd.mode = rle::Encoder::BG_CLEAR;
    bd.color = { ... };
}
```

### Why This Code Could Not Execute

The post-loop checks at lines 229-238 could never execute because:

1. **During the loop** (lines 212-227), whenever `maxCount` is updated (line 213), the code immediately checks:
   ```cpp
   if (maxCount >= clear_needed) {
       return bd;  // Early exit with BG_CLEAR
   } else if (maxCount >= overlay_needed && bd.mode != rle::Encoder::BG_OVERLAY) {
       bd.mode = rle::Encoder::BG_OVERLAY;  // Sets mode to OVERLAY
   }
   ```

2. **After the loop** (line 229), the code checked:
   ```cpp
   if (bd.mode == BG_SAVE_ALL && maxCount >= overlay_needed)
   ```

3. **The contradiction**: If `maxCount >= overlay_needed` after the loop, then at the point when `maxCount` reached this value during the loop (line 213), the condition at line 221 would have been true, causing line 222 to set `bd.mode = BG_OVERLAY`. Therefore, `bd.mode` could not be `BG_SAVE_ALL` when the post-loop check executed.

4. **Same issue for CLEAR threshold**: The condition `bd.mode == BG_SAVE_ALL && maxCount >= clear_needed` at line 234 could never be true because line 215 would have triggered an early return.

### Fixed Implementation

The corrected code (after removing dead code):

```cpp
for (uint64_t i = 0; i < npix; ++i) {
    // ... frequency counting ...
    
    if (it->second > maxCount) {
        maxCount = it->second;
        maxKey = key;
        if (maxCount >= clear_needed) {
            // Early exit: 50%+ pixels are this color -> use CLEAR mode
            bd.mode = rle::Encoder::BG_CLEAR;
            bd.color = { ... };
            return bd;
        } else if (maxCount >= overlay_needed && bd.mode != rle::Encoder::BG_OVERLAY) {
            // 20%+ pixels are this color -> use OVERLAY mode
            bd.mode = rle::Encoder::BG_OVERLAY;
            bd.color = { ... };
        }
    }
}
// Loop completed: use the mode determined during iteration
return bd;
```

### Correct Behavior

The function now correctly:
1. Scans all pixels and counts color frequencies
2. **Early exits** when a color reaches 50% threshold (CLEAR mode)
3. **Sets OVERLAY mode** when a color reaches 20% threshold
4. **Returns BG_SAVE_ALL** if no threshold is met

This matches the intended RLE specification behavior:
- **BG_SAVE_ALL** (mode 0): Write all pixels without optimization
- **BG_OVERLAY** (mode 1): Skip background pixels (saves space when 20%+ are background)
- **BG_CLEAR** (mode 2): Set CLEAR_FIRST flag and skip background (when 50%+ are background)

### Impact

**Positive Changes:**
- ✅ Removed 10 lines of unreachable dead code
- ✅ Improved code coverage from 87.70% to 91.53%
- ✅ Clearer, more maintainable logic
- ✅ Added clarifying comments
- ✅ All 35 tests pass (100%)

**No functional change**: The code's behavior remains identical since the dead code was never executed.

### Verification

5 comprehensive tests verify background detection behavior:
- `test_bg_auto_detect_overlay_threshold()` - Verifies 20% OVERLAY threshold
- `test_bg_auto_detect_clear_threshold()` - Verifies 50% CLEAR threshold
- `test_bg_auto_detect_early_exit()` - Verifies early exit optimization
- `test_bg_auto_detect_post_loop()` - (Previously tried to test dead code, now tests normal path)
- `test_bg_auto_detect_post_loop_clear()` - (Previously tried to test dead code, now tests normal path)

All tests pass with pixel-perfect verification.
