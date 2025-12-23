/*                           R L E . C P P
 * BRL-CAD
 *
 * Hardened Utah RLE read/write implementation for libicv using the internal
 * clean-room codec (rle.hpp). Legacy utahrle code paths removed.
 *
 * Key fixes:
 *   - Do not fclose(fp) in rle_read; caller owns FILE* (prevents double free).
 *   - Uses rle.hpp MAX_* limits and hardened decoder.
 *   - Background detection bounded and early-exiting.
 *   - Deterministic comments (timestamp/software/format).
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdarg>


#include "rle.hpp"   /* rle */

void *
bu_calloc(size_t nelem, size_t elsize, const char *)
{
    return calloc(nelem, elsize);
}

#define BU_ALLOC(_ptr, _type) _ptr = (_type *)bu_calloc(1, sizeof(_type), #_type " (BU_ALLOC) ")

void
bu_free(void *ptr, const char *str)
{
    const char *nul = "(null)";
    if (!str)
        str = nul;

    /* noisily report "marked" pointers */
    if (ptr == (char *)(-1L)) {
        fprintf(stderr, "%p free ERROR %s\n", ptr, str);
        return;
    }

    /* Here we intentionally wipe out the first four bytes before the
     * actual free() as a basic memory safeguard.  While we're not
     * guaranteed anything after free(), some implementations leave
     * the zapped value intact and it can help with debugging.
     */
    *((uint32_t *)ptr) = 0xFFFFFFFF;    /* zappo! */

    free(ptr);
}

int
bu_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);

    return 0;
}

#define BRLCAD_OK 0
#define BRLCAD_ERROR 1

#define ICV_IMAGE_MAGIC			0x6269666d /**< bifm */

#define ICV_IMAGE_INIT(_i) { \
        (_i)->magic = ICV_IMAGE_MAGIC; \
        (_i)->width = (_i)->height = (_i)->channels = (_i)->alpha_channel = 0; \
        (_i)->gamma_corr = 0.0; \
        (_i)->data = NULL; \
    }


namespace {

// Detection thresholds for sparse row pattern
constexpr size_t PATTERN_SAMPLE_COUNT = 20;         // Number of row pairs to check (legacy, not used in new detector)
constexpr double UNIFORM_TOLERANCE = 0.001;          // Max variation for uniform row

inline bool safe_mul_u64(uint64_t a, uint64_t b, uint64_t limit, uint64_t &out) {
    if (!a || !b) { out = 0; return true; }
    if (a > limit / b) return false;
    out = a * b;
    return true;
}

inline uint8_t dbl_to_u8(double v) {
    if (v < 0.0) v = 0.0;
    if (v > 1.0) v = 1.0;
    return static_cast<uint8_t>(lrint(v * 255.0));
}
inline double u8_to_dbl(uint8_t v) { return double(v) / 255.0; }

bool icv_to_u8_interleaved(const icv_image_t *img, std::vector<uint8_t> &buf, bool &has_alpha) {
    if (!img || !img->data || img->channels < 3) return false;
    uint64_t npix;
    if (!safe_mul_u64(img->width, img->height, rle::MAX_PIXELS, npix)) return false;
    if (!npix) return false;

    // Determine if we have alpha channel
    has_alpha = (img->channels >= 4 && img->alpha_channel != 0);
    size_t channels_out = has_alpha ? 4 : 3;

    uint64_t total_bytes;
    if (!safe_mul_u64(npix, channels_out, rle::MAX_ALLOC_BYTES, total_bytes)) return false;

    try { buf.resize(static_cast<size_t>(npix) * channels_out); }
    catch (...) { return false; }

    const double *src = img->data;
    if (has_alpha) {
        // Convert RGBA
        for (uint64_t i = 0; i < npix; ++i) {
            buf[4*i + 0] = dbl_to_u8(src[4*i + 0]);  // R
            buf[4*i + 1] = dbl_to_u8(src[4*i + 1]);  // G
            buf[4*i + 2] = dbl_to_u8(src[4*i + 2]);  // B
            buf[4*i + 3] = dbl_to_u8(src[4*i + 3]);  // A
        }
    } else {
        // Convert RGB
        for (uint64_t i = 0; i < npix; ++i) {
            buf[3*i + 0] = dbl_to_u8(src[3*i + 0]);  // R
            buf[3*i + 1] = dbl_to_u8(src[3*i + 1]);  // G
            buf[3*i + 2] = dbl_to_u8(src[3*i + 2]);  // B
        }
    }
    return true;
}

bool u8_interleaved_to_icv(const std::vector<uint8_t> &buf, size_t w, size_t h, bool has_alpha, icv_image_t *out) {
    if (!out || !w || !h) return false;
    uint64_t npix;
    if (!safe_mul_u64(w, h, rle::MAX_PIXELS, npix)) return false;
    
    size_t channels = has_alpha ? 4 : 3;
    if (buf.size() < npix * channels) return false;

    uint64_t elems;
    if (!safe_mul_u64(npix, channels, rle::MAX_ALLOC_BYTES / sizeof(double), elems)) return false;

    double *data = static_cast<double *>(
        bu_calloc(static_cast<size_t>(npix) * channels, sizeof(double), "rle_icv_data"));
    if (!data) return false;

    if (has_alpha) {
        // Convert RGBA
        for (uint64_t i = 0; i < npix; ++i) {
            data[4*i + 0] = u8_to_dbl(buf[4*i + 0]);  // R
            data[4*i + 1] = u8_to_dbl(buf[4*i + 1]);  // G
            data[4*i + 2] = u8_to_dbl(buf[4*i + 2]);  // B
            data[4*i + 3] = u8_to_dbl(buf[4*i + 3]);  // A
        }
    } else {
        // Convert RGB
        for (uint64_t i = 0; i < npix; ++i) {
            data[3*i + 0] = u8_to_dbl(buf[3*i + 0]);  // R
            data[3*i + 1] = u8_to_dbl(buf[3*i + 1]);  // G
            data[3*i + 2] = u8_to_dbl(buf[3*i + 2]);  // B
        }
    }

    out->width = w;
    out->height = h;
    out->channels = channels;
    out->alpha_channel = has_alpha ? 1 : 0;
    out->color_space = ICV_COLOR_SPACE_RGB;
    out->data = data;
    out->magic = ICV_IMAGE_MAGIC;
    return true;
}

struct BackgroundDecision {
    std::vector<uint8_t> color;
    rle::Encoder::BackgroundMode mode;
};

BackgroundDecision detect_background(const std::vector<uint8_t> &rgb, size_t w, size_t h) {
    static constexpr size_t UNIQUE_CAP = 65536;
    static constexpr double CLEAR_THRESH = 0.50;
    static constexpr double OVERLAY_THRESH = 0.20;

    BackgroundDecision bd;
    bd.mode = rle::Encoder::BG_SAVE_ALL;

    uint64_t npix;
    if (!safe_mul_u64(w, h, rle::MAX_PIXELS, npix)) return bd;
    if (!npix || rgb.size() < npix * 3) return bd;

    uint64_t clear_needed   = uint64_t(npix * CLEAR_THRESH);
    uint64_t overlay_needed = uint64_t(npix * OVERLAY_THRESH);

    std::unordered_map<uint32_t, uint64_t> freq;
    freq.reserve(std::min<uint64_t>(npix, 4096));

    uint64_t maxCount = 0;
    uint32_t maxKey = 0;

    for (uint64_t i = 0; i < npix; ++i) {
        uint32_t key = (uint32_t(rgb[3*i + 0]) << 16) |
                       (uint32_t(rgb[3*i + 1]) << 8)  |
                       uint32_t(rgb[3*i + 2]);
        auto it = freq.find(key);
        if (it == freq.end()) {
            if (freq.size() >= UNIQUE_CAP) return bd;
            it = freq.emplace(key, 1).first;
        } else {
            ++(it->second);
        }
        if (it->second > maxCount) {
            maxCount = it->second;
            maxKey = key;
            if (maxCount >= clear_needed) {
                // Early exit: 50%+ pixels are this color -> use CLEAR mode
                bd.mode = rle::Encoder::BG_CLEAR;
                bd.color = { uint8_t((maxKey >> 16) & 0xFF),
                             uint8_t((maxKey >> 8)  & 0xFF),
                             uint8_t(maxKey & 0xFF) };
                return bd;
            } else if (maxCount >= overlay_needed && bd.mode != rle::Encoder::BG_OVERLAY) {
                // 20%+ pixels are this color -> use OVERLAY mode
                bd.mode = rle::Encoder::BG_OVERLAY;
                bd.color = { uint8_t((maxKey >> 16) & 0xFF),
                             uint8_t((maxKey >> 8)  & 0xFF),
                             uint8_t(maxKey & 0xFF) };
            }
        }
    }
    // Loop completed: use the mode determined during iteration
    return bd;
}

std::vector<std::string> build_comments() {
    std::vector<std::string> comments;
#if RLE_TIMESTAMP_ENABLED
    comments.emplace_back(std::string("CREATED=") + rle::rle_utc_timestamp());
#endif
    comments.emplace_back("SOFTWARE=BRL-CAD libicv");
    comments.emplace_back("FORMAT=UtahRLE");
    return comments;
}

void log_rle_error(const char *context, rle::Error e) {
    if (e == rle::Error::OK) return;
    bu_log("%s: RLE error: %s\n", context, rle::error_string(e));
}

/**
 * Detect and fix sparse row pattern caused by excessive SKIP_LINES opcodes
 * 
 * Some Utah RLE files from the original toolkit have SKIP_LINES opcodes
 * after every data row, causing the decoder to skip rows. The number of
 * skipped rows can be 1, 2, or more, creating various visual artifacts.
 * 
 * Detection strategy:
 * 1. Sample rows throughout the image
 * 2. Identify which rows have actual image data (non-uniform rows)
 * 3. Check if data rows follow a regular pattern (e.g., every 2nd, 3rd, or 4th row)
 * 4. Require at least 60% of rows to be uniform (background/skipped)
 * 
 * Fix strategy:
 * - Replicate each data row into the following skip rows
 * - For pattern with period N: copy row i to rows i+1, i+2, ..., i+(N-1)
 * 
 * @param img The image to check and potentially fix (uses double data 0-1)
 * @return true if pattern was detected and fixed, false otherwise
 */
bool detect_and_fix_alternating_pattern(icv_image_t* img) {
    if (!img || !img->data || img->height < 4) {
        return false;
    }
    
    const size_t width = img->width;
    const size_t height = img->height;
    const size_t channels = img->channels;
    
    // Helper lambda to check if a row is uniform (all pixels have same RGB value)
    // Note: We only check the color channels (RGB), not alpha, because alpha is
    // often initialized to 255 uniformly, which would interfere with detection.
    auto is_row_uniform = [&](size_t row) -> bool {
        const size_t color_channels = (channels >= 4) ? 3 : channels;  // Check only RGB, not alpha
        for (size_t c = 0; c < color_channels; c++) {
            double first_val = img->data[(row * width) * channels + c];
            for (size_t x = 0; x < width; x++) {
                double val = img->data[(row * width + x) * channels + c];
                if (std::fabs(val - first_val) > UNIFORM_TOLERANCE) {
                    return false;
                }
            }
        }
        return true;
    };
    
    // Sample all rows to find which ones have data
    std::vector<bool> row_has_data(height);
    size_t uniform_count = 0;
    
    for (size_t y = 0; y < height; y++) {
        bool uniform = is_row_uniform(y);
        row_has_data[y] = !uniform;
        if (uniform) uniform_count++;
    }
    
    // Require at least 60% of rows to be uniform to consider this as a sparse pattern
    if (uniform_count < height * 0.6) {
        return false;
    }
    
    // Find the positions of data rows
    std::vector<size_t> data_rows;
    for (size_t y = 0; y < height; y++) {
        if (row_has_data[y]) {
            data_rows.push_back(y);
        }
    }
    
    if (data_rows.size() < 2) {
        return false;  // Need at least 2 data rows to detect a pattern
    }
    
    // Try to detect the period (spacing between data rows)
    // Common patterns: every 2nd row (period=2), every 3rd row (period=3), every 4th row (period=4)
    std::vector<int> periods_to_test = {2, 3, 4, 5};
    int detected_period = 0;
    
    for (int period : periods_to_test) {
        size_t matches = 0;
        for (size_t i = 0; i + 1 < data_rows.size(); i++) {
            int diff = static_cast<int>(data_rows[i+1] - data_rows[i]);
            if (diff == period) {
                matches++;
            }
        }
        
        // If at least 70% of consecutive data row pairs have this spacing
        if (matches >= (data_rows.size() - 1) * 0.7) {
            detected_period = period;
            break;
        }
    }
    
    if (detected_period == 0) {
        return false;  // No regular pattern detected
    }
    
    // Apply de-interlacing: replicate data rows into the following skip rows
    for (size_t i = 0; i < data_rows.size(); i++) {
        size_t data_row = data_rows[i];
        
        // Copy this row into the following rows up to the next data row or period rows
        size_t next_data_row = (i + 1 < data_rows.size()) ? data_rows[i + 1] : height;
        size_t max_copy = std::min(data_row + detected_period, next_data_row);
        
        for (size_t dest_row = data_row + 1; dest_row < max_copy; dest_row++) {
            std::memcpy(&img->data[(dest_row * width) * channels],
                       &img->data[(data_row * width) * channels],
                       width * channels * sizeof(double));
        }
    }
    
    return true;
}

} /* anonymous namespace */

/* -------------------- Public API -------------------- */

int
rle_write(icv_image_t *bif, FILE *fp)
{
    if (!bif || !fp) {
        bu_log("rle_write: null image or file pointer\n");
        return BRLCAD_ERROR;
    }
    if (bif->channels < 3) {
        bu_log("rle_write: image must have at least 3 channels (RGB)\n");
        return BRLCAD_ERROR;
    }
    if (bif->width > rle::MAX_DIM || bif->height > rle::MAX_DIM) {
        bu_log("rle_write: dimensions exceed maximum (%u x %u)\n",
               rle::MAX_DIM, rle::MAX_DIM);
        return BRLCAD_ERROR;
    }

    bool has_alpha = false;
    std::vector<uint8_t> data;
    if (!icv_to_u8_interleaved(bif, data, has_alpha)) {
        bu_log("rle_write: conversion to 8-bit buffer failed\n");
        return BRLCAD_ERROR;
    }

    // Background detection only looks at RGB (first 3 channels)
    // Extract RGB for background detection if we have alpha
    std::vector<uint8_t> rgb_only;
    if (has_alpha) {
        size_t npix = bif->width * bif->height;
        rgb_only.resize(npix * 3);
        for (size_t i = 0; i < npix; ++i) {
            rgb_only[3*i + 0] = data[4*i + 0];  // R
            rgb_only[3*i + 1] = data[4*i + 1];  // G
            rgb_only[3*i + 2] = data[4*i + 2];  // B
        }
    }
    
    BackgroundDecision bgd = detect_background(has_alpha ? rgb_only : data, bif->width, bif->height);
    std::vector<std::string> comments = build_comments();

    rle::Error err;
    bool ok = rle::write_rgb(fp,
                                     data.data(),
                                     static_cast<uint32_t>(bif->width),
                                     static_cast<uint32_t>(bif->height),
                                     comments,
                                     bgd.color,
                                     has_alpha,
                                     bgd.mode,
                                     err);
    if (!ok || err != rle::Error::OK) {
        log_rle_error("rle_write", err);
        return BRLCAD_ERROR;
    }
    return BRLCAD_OK;
}

icv_image_t*
rle_read(FILE *fp)
{
    if (!fp) {
        bu_log("rle_read: null file pointer\n");
        return NULL;
    }

    std::vector<uint8_t> rgb;
    uint32_t width = 0, height = 0;
    bool has_alpha = false;
    std::vector<std::string> comments;
    rle::Error err;

    bool ok = rle::read_rgb(fp, rgb, width, height,
                                    &has_alpha, &comments, err);

    if (!ok || err != rle::Error::OK) {
        log_rle_error("rle_read", err);
        /* Do not fclose(fp); caller owns the FILE* */
        return NULL;
    }

    if (width > rle::MAX_DIM || height > rle::MAX_DIM) {
        bu_log("rle_read: dimensions exceed maximum (%u x %u)\n",
               rle::MAX_DIM, rle::MAX_DIM);
        return NULL;
    }

    icv_image_t *img = NULL;
    BU_ALLOC(img, struct icv_image);
    ICV_IMAGE_INIT(img);

    if (!u8_interleaved_to_icv(rgb, static_cast<size_t>(width),
                               static_cast<size_t>(height), has_alpha, img)) {
        bu_log("rle_read: buffer to icv image conversion failed\n");
        bu_free(img, "icv_image");
        return NULL;
    }

    /* Fault-tolerant mode: Detect and fix sparse row pattern
     * Some Utah RLE files (from the original toolkit) have excessive SKIP_LINES
     * opcodes after data rows, causing the decoder to skip multiple rows.
     * These skipped rows remain filled with background color, creating visual
     * artifacts (horizontal banding with period 2, 3, or more).
     * 
     * Detection: Check if most rows are uniform (background) and the non-uniform
     * rows follow a regular pattern (e.g., every 2nd, 3rd, or 4th row).
     * 
     * Fix: Replicate each data row into the following skip rows.
     */
    if (detect_and_fix_alternating_pattern(img)) {
        bu_log("rle_read: WARNING - Detected and corrected sparse row pattern\n");
        bu_log("  This file has excessive SKIP_LINES opcodes causing rows to be skipped.\n");
        bu_log("  Applied de-interlacing by replicating data rows into skipped rows.\n");
    }

    return img;
}

/* Local Variables:
 * tab-width: 8
 * mode: C++
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
