// Decode RLE files WITHOUT applying any de-interlacing fixes
// This will show the raw decoded data to identify patterns
#include "rle.hpp"
#include <cstdio>
#include <cstring>

extern "C" {
void* bu_calloc(size_t nelem, size_t elsize, const char*);
void bu_free(void*, const char*);
int bu_log(const char* fmt, ...);
}

// Temporarily disable pattern detection
#define DISABLE_PATTERN_FIX

icv_image_t* rle_read_raw(FILE *fp) {
    if (!fp) {
        bu_log("rle_read_raw: null file pointer\n");
        return nullptr;
    }

    rle::Decoder dec;
    if (!dec.decode_from_file(fp)) {
        bu_log("rle_read_raw: decode failed\n");
        return nullptr;
    }

    size_t w = dec.width();
    size_t h = dec.height();
    size_t nchannels = dec.nchannels();

    icv_image_t *img = (icv_image_t *)bu_calloc(1, sizeof(icv_image_t), "icv_image");
    if (!img) return nullptr;

    img->magic = 0x6269666d; // ICV_IMAGE_MAGIC
    img->width = w;
    img->height = h;
    img->channels = nchannels;
    img->alpha_channel = dec.has_alpha() ? 1 : 0;
    img->color_space = ICV_COLOR_SPACE_RGB;
    img->gamma_corr = 0.0;

    size_t npixels = w * h;
    img->data = (double *)bu_calloc(npixels * nchannels, sizeof(double), "image data");
    if (!img->data) {
        bu_free(img, "image");
        return nullptr;
    }

    const auto &px = dec.pixels();
    for (size_t i = 0; i < npixels * nchannels; i++) {
        img->data[i] = double(px[i]) / 255.0;
    }

    // NO pattern detection or fixing here!
    return img;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input.rle> <output.ppm>\n", argv[0]);
        return 1;
    }

    FILE* in = fopen(argv[1], "rb");
    if (!in) {
        fprintf(stderr, "Error: Cannot open %s\n", argv[1]);
        return 1;
    }

    printf("Decoding %s (RAW - no pattern fixes) to %s ...\n", argv[1], argv[2]);
    
    icv_image_t* img = rle_read_raw(in);
    fclose(in);

    if (!img) {
        fprintf(stderr, "Failed to decode RLE\n");
        return 1;
    }

    printf("  RLE Image: %zu x %zu channels=%zu alpha=%s\n",
           img->width, img->height, img->channels,
           img->alpha_channel ? "yes" : "no");

    // Write PPM
    FILE* out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "Error: Cannot open %s for writing\n", argv[2]);
        bu_free(img->data, "data");
        bu_free(img, "image");
        return 1;
    }

    fprintf(out, "P6\n%zu %zu\n255\n", img->width, img->height);
    
    for (size_t i = 0; i < img->width * img->height; i++) {
        // Write RGB (skip alpha if present)
        for (size_t c = 0; c < 3; c++) {
            double val = img->data[i * img->channels + c];
            if (val < 0.0) val = 0.0;
            if (val > 1.0) val = 1.0;
            uint8_t byte = (uint8_t)(val * 255.0 + 0.5);
            fputc(byte, out);
        }
    }

    fclose(out);
    bu_free(img->data, "data");
    bu_free(img, "image");

    printf("  ✓ Successfully decoded to %s (RAW - no pattern fixes)\n", argv[2]);
    return 0;
}
