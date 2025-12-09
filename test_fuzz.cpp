/*
 * test_fuzz.cpp - Fuzzing test for RLE encoder/decoder
 *
 * This test provides optional fuzzing capability to discover edge cases
 * and validate robustness of the RLE implementation.
 *
 * Build with:
 *   g++ -std=c++11 -O2 test_fuzz.cpp rle.cpp -o test_fuzz
 *
 * Run with:
 *   ./test_fuzz [iterations] [max_width] [max_height]
 *
 * Examples:
 *   ./test_fuzz                    # Quick test: 1000 iterations, 256x256 max
 *   ./test_fuzz 10000              # Extended test: 10000 iterations
 *   ./test_fuzz 100000 512 512     # Stress test: large images
 *
 * Integration with AFL/LibFuzzer:
 *   See FUZZING_GUIDE section below for integration instructions.
 */

#include "rle.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>

// Test statistics
struct FuzzStats {
    size_t total_tests = 0;
    size_t passed = 0;
    size_t failed = 0;
    size_t exceptions = 0;
    size_t invalid_inputs = 0;
    size_t decode_errors = 0;
    size_t pixel_mismatches = 0;
};

// Random number generator
static std::mt19937 rng;

// Initialize RNG with seed
void init_rng(uint32_t seed) {
    rng.seed(seed);
}

// Generate random image with various characteristics
rle::Image generate_random_image(uint32_t max_w, uint32_t max_h) {
    std::uniform_int_distribution<uint32_t> size_dist(1, std::max(1u, std::min(max_w, max_h)));
    std::uniform_int_distribution<uint32_t> wide_dist(1, max_w);
    std::uniform_int_distribution<uint32_t> tall_dist(1, max_h);
    std::uniform_int_distribution<uint32_t> pattern_dist(0, 10);
    std::uniform_int_distribution<uint32_t> color_dist(0, 255);
    std::uniform_int_distribution<uint32_t> bool_dist(0, 1);
    
    rle::Image img;
    rle::Header& h = img.header;
    
    // Random dimensions (favor interesting cases)
    uint32_t pattern = pattern_dist(rng);
    if (pattern == 0) {
        // Square image
        uint32_t size = size_dist(rng);
        h.xlen = h.ylen = size;
    } else if (pattern == 1) {
        // Wide image
        h.xlen = wide_dist(rng);
        h.ylen = size_dist(rng) / 2 + 1;
    } else if (pattern == 2) {
        // Tall image
        h.xlen = size_dist(rng) / 2 + 1;
        h.ylen = tall_dist(rng);
    } else if (pattern == 3) {
        // Tiny image (1x1 to 4x4)
        h.xlen = color_dist(rng) % 4 + 1;
        h.ylen = color_dist(rng) % 4 + 1;
    } else {
        // Random dimensions
        h.xlen = size_dist(rng);
        h.ylen = size_dist(rng);
    }
    
    h.xpos = 0;
    h.ypos = 0;
    h.ncolors = 3;
    h.pixelbits = 8;
    h.ncmap = 0;
    h.cmaplen = 0;
    
    // Random alpha channel
    if (bool_dist(rng)) {
        h.flags |= rle::FLAG_ALPHA;
    }
    
    // Random background
    if (bool_dist(rng)) {
        h.background = {
            uint8_t(color_dist(rng)),
            uint8_t(color_dist(rng)),
            uint8_t(color_dist(rng))
        };
    } else {
        h.flags |= rle::FLAG_NO_BACKGROUND;
    }
    
    // Random comments
    if (bool_dist(rng) && bool_dist(rng)) {
        h.comments.push_back("Fuzz test image");
        h.flags |= rle::FLAG_COMMENT;
    }
    
    // Allocate pixels
    rle::Error err;
    if (!img.allocate(err)) {
        throw std::runtime_error("Failed to allocate image");
    }
    
    // Generate pixel pattern
    std::uniform_int_distribution<uint32_t> fill_pattern(0, 8);
    uint32_t fill = fill_pattern(rng);
    
    size_t pixel_count = size_t(h.xlen) * h.ylen;
    size_t channels = h.channels();
    
    if (fill == 0) {
        // Solid color
        uint8_t r = color_dist(rng);
        uint8_t g = color_dist(rng);
        uint8_t b = color_dist(rng);
        for (size_t i = 0; i < pixel_count; ++i) {
            img.pixels[i * channels + 0] = r;
            img.pixels[i * channels + 1] = g;
            img.pixels[i * channels + 2] = b;
            if (h.has_alpha()) {
                img.pixels[i * channels + 3] = color_dist(rng);
            }
        }
    } else if (fill == 1) {
        // Horizontal gradient
        for (uint32_t y = 0; y < h.ylen; ++y) {
            for (uint32_t x = 0; x < h.xlen; ++x) {
                uint32_t denom = (h.xlen > 1) ? (h.xlen - 1) : 1;
                uint8_t val = uint8_t((x * 255) / denom);
                size_t idx = (y * h.xlen + x) * channels;
                img.pixels[idx + 0] = val;
                img.pixels[idx + 1] = val;
                img.pixels[idx + 2] = val;
                if (h.has_alpha()) {
                    img.pixels[idx + 3] = 255;
                }
            }
        }
    } else if (fill == 2) {
        // Vertical gradient
        for (uint32_t y = 0; y < h.ylen; ++y) {
            uint32_t denom = (h.ylen > 1) ? (h.ylen - 1) : 1;
            uint8_t val = uint8_t((y * 255) / denom);
            for (uint32_t x = 0; x < h.xlen; ++x) {
                size_t idx = (y * h.xlen + x) * channels;
                img.pixels[idx + 0] = val;
                img.pixels[idx + 1] = val;
                img.pixels[idx + 2] = val;
                if (h.has_alpha()) {
                    img.pixels[idx + 3] = 255;
                }
            }
        }
    } else if (fill == 3) {
        // Checkerboard
        uint32_t min_dim = (h.xlen < h.ylen) ? h.xlen : h.ylen;
        uint32_t block_size = (min_dim / 8 > 0) ? (min_dim / 8) : 1;
        for (uint32_t y = 0; y < h.ylen; ++y) {
            for (uint32_t x = 0; x < h.xlen; ++x) {
                bool checker = ((x / block_size) + (y / block_size)) % 2;
                uint8_t val = checker ? 255 : 0;
                size_t idx = (y * h.xlen + x) * channels;
                img.pixels[idx + 0] = val;
                img.pixels[idx + 1] = val;
                img.pixels[idx + 2] = val;
                if (h.has_alpha()) {
                    img.pixels[idx + 3] = 255;
                }
            }
        }
    } else if (fill == 4) {
        // Random noise
        for (size_t i = 0; i < pixel_count * channels; ++i) {
            img.pixels[i] = color_dist(rng);
        }
    } else if (fill == 5) {
        // Alternating rows
        for (uint32_t y = 0; y < h.ylen; ++y) {
            uint8_t val = (y % 2) ? 255 : 0;
            for (uint32_t x = 0; x < h.xlen; ++x) {
                size_t idx = (y * h.xlen + x) * channels;
                img.pixels[idx + 0] = val;
                img.pixels[idx + 1] = val;
                img.pixels[idx + 2] = val;
                if (h.has_alpha()) {
                    img.pixels[idx + 3] = 255;
                }
            }
        }
    } else if (fill == 6) {
        // Sparse random (mostly background)
        if (!h.background.empty()) {
            // Fill with background
            for (size_t i = 0; i < pixel_count; ++i) {
                size_t idx = i * channels;
                img.pixels[idx + 0] = h.background[0];
                img.pixels[idx + 1] = h.background[1];
                img.pixels[idx + 2] = h.background[2];
                if (h.has_alpha()) {
                    img.pixels[idx + 3] = 255;
                }
            }
            // Add random non-background pixels
            size_t sparse_count = (pixel_count / 10 < 100) ? (pixel_count / 10) : 100;
            for (uint32_t i = 0; i < sparse_count; ++i) {
                uint32_t x = color_dist(rng) % h.xlen;
                uint32_t y = color_dist(rng) % h.ylen;
                size_t idx = (y * h.xlen + x) * channels;
                img.pixels[idx + 0] = color_dist(rng);
                img.pixels[idx + 1] = color_dist(rng);
                img.pixels[idx + 2] = color_dist(rng);
            }
        } else {
            // Random if no background
            for (size_t i = 0; i < pixel_count * channels; ++i) {
                img.pixels[i] = color_dist(rng);
            }
        }
    } else {
        // Mixed pattern
        for (uint32_t y = 0; y < h.ylen; ++y) {
            for (uint32_t x = 0; x < h.xlen; ++x) {
                size_t idx = (y * h.xlen + x) * channels;
                img.pixels[idx + 0] = uint8_t((x + y) % 256);
                img.pixels[idx + 1] = uint8_t((x * 2) % 256);
                img.pixels[idx + 2] = uint8_t((y * 3) % 256);
                if (h.has_alpha()) {
                    img.pixels[idx + 3] = uint8_t((x ^ y) % 256);
                }
            }
        }
    }
    
    return img;
}

// Fuzz test: roundtrip encode/decode
bool fuzz_roundtrip(const rle::Image& original, FuzzStats& stats, bool verbose) {
    stats.total_tests++;
    
    try {
        // Encode to memory buffer
        FILE* f = tmpfile();
        if (!f) {
            if (verbose) std::cerr << "Failed to create tmpfile\n";
            stats.failed++;
            return false;
        }
        
        // Use BG_SAVE_ALL mode (most reliable, always encodes all pixels)
        // BG_OVERLAY and BG_CLEAR are optimization paths that skip background pixels
        auto bg_mode = rle::Encoder::BackgroundMode::BG_SAVE_ALL;
        
        rle::Error err;
        bool write_ok = rle::Encoder::write(f, original, bg_mode, err);
        
        if (!write_ok) {
            if (verbose) {
                std::cerr << "Encode failed: " << rle::error_string(err) 
                          << " [" << original.header.xlen << "x" << original.header.ylen << "]\n";
            }
            fclose(f);
            stats.invalid_inputs++;
            return false;
        }
        
        // Rewind and decode
        rewind(f);
        
        rle::Image decoded;
        auto res = rle::Decoder::read(f, decoded);
        fclose(f);
        
        if (!res.ok) {
            if (verbose) {
                std::cerr << "Decode failed: " << rle::error_string(res.error) << "\n";
            }
            stats.decode_errors++;
            return false;
        }
        
        // Verify dimensions match
        if (decoded.header.xlen != original.header.xlen ||
            decoded.header.ylen != original.header.ylen ||
            decoded.header.channels() != original.header.channels()) {
            if (verbose) {
                std::cerr << "Dimension mismatch\n";
            }
            stats.pixel_mismatches++;
            return false;
        }
        
        // Verify pixels match
        size_t pixel_count = size_t(original.header.xlen) * original.header.ylen;
        size_t channels = original.header.channels();
        
        for (size_t i = 0; i < pixel_count * channels; ++i) {
            if (original.pixels[i] != decoded.pixels[i]) {
                if (verbose) {
                    size_t pixel = i / channels;
                    size_t chan = i % channels;
                    uint32_t x = pixel % original.header.xlen;
                    uint32_t y = pixel / original.header.xlen;
                    std::cerr << "Pixel mismatch at (" << x << "," << y << ") channel " << chan
                              << ": expected " << (int)original.pixels[i]
                              << ", got " << (int)decoded.pixels[i] << "\n";
                }
                stats.pixel_mismatches++;
                return false;
            }
        }
        
        stats.passed++;
        return true;
        
    } catch (const std::exception& e) {
        if (verbose) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
        stats.exceptions++;
        return false;
    }
}

// Print statistics
void print_stats(const FuzzStats& stats, double elapsed_sec) {
    std::cout << "\n=== Fuzz Test Results ===\n";
    std::cout << "Total tests:      " << stats.total_tests << "\n";
    std::cout << "Passed:           " << stats.passed 
              << " (" << (100.0 * stats.passed / std::max(1ul, stats.total_tests)) << "%)\n";
    std::cout << "Failed:           " << stats.failed << "\n";
    std::cout << "  Invalid inputs: " << stats.invalid_inputs << "\n";
    std::cout << "  Decode errors:  " << stats.decode_errors << "\n";
    std::cout << "  Pixel errors:   " << stats.pixel_mismatches << "\n";
    std::cout << "  Exceptions:     " << stats.exceptions << "\n";
    std::cout << "Elapsed time:     " << elapsed_sec << " seconds\n";
    std::cout << "Tests/second:     " << (stats.total_tests / elapsed_sec) << "\n";
    std::cout << "\n";
    
    if (stats.passed == stats.total_tests) {
        std::cout << "✅ ALL TESTS PASSED\n";
    } else {
        std::cout << "⚠️  SOME TESTS FAILED\n";
    }
}

int main(int argc, char** argv) {
    // Parse command line arguments
    size_t iterations = 1000;
    uint32_t max_width = 256;
    uint32_t max_height = 256;
    bool verbose = false;
    
    if (argc > 1) iterations = std::atoi(argv[1]);
    if (argc > 2) max_width = std::atoi(argv[2]);
    if (argc > 3) max_height = std::atoi(argv[3]);
    if (argc > 4 && std::string(argv[4]) == "-v") verbose = true;
    
    // Initialize RNG with time-based seed
    uint32_t seed = std::chrono::system_clock::now().time_since_epoch().count() & 0xFFFFFFFF;
    init_rng(seed);
    
    std::cout << "=== RLE Fuzz Testing ===\n";
    std::cout << "Iterations:   " << iterations << "\n";
    std::cout << "Max size:     " << max_width << "x" << max_height << "\n";
    std::cout << "Random seed:  " << seed << "\n";
    std::cout << "Verbose:      " << (verbose ? "yes" : "no") << "\n";
    std::cout << "\nRunning tests...\n";
    
    FuzzStats stats;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run fuzz tests
    for (size_t i = 0; i < iterations; ++i) {
        if (!verbose && i % 100 == 0) {
            std::cout << "Progress: " << i << "/" << iterations << "\r" << std::flush;
        }
        
        try {
            rle::Image img = generate_random_image(max_width, max_height);
            fuzz_roundtrip(img, stats, verbose);
        } catch (const std::exception& e) {
            if (verbose) {
                std::cerr << "Image generation failed: " << e.what() << "\n";
            }
            stats.exceptions++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    
    print_stats(stats, elapsed);
    
    return (stats.passed == stats.total_tests) ? 0 : 1;
}

/*
 * FUZZING_GUIDE:
 *
 * Integration with AFL (American Fuzzy Lop):
 * ------------------------------------------
 * 1. Compile with AFL:
 *    afl-g++ -std=c++11 -O2 test_fuzz.cpp rle.cpp -o test_fuzz_afl
 *
 * 2. Create seed inputs:
 *    mkdir fuzz_in fuzz_out
 *    cp teapot.rle fuzz_in/
 *
 * 3. Run AFL:
 *    afl-fuzz -i fuzz_in -o fuzz_out ./test_fuzz_afl @@
 *
 * Integration with LibFuzzer:
 * ---------------------------
 * 1. Create fuzz target (test_fuzz_libfuzzer.cpp):
 *
 *    #include "rle.hpp"
 *    extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
 *        FILE* f = fmemopen((void*)data, size, "rb");
 *        if (!f) return 0;
 *        rle::Image img;
 *        rle::Decoder::read(f, img);
 *        fclose(f);
 *        return 0;
 *    }
 *
 * 2. Compile with LibFuzzer:
 *    clang++ -fsanitize=fuzzer,address -std=c++11 test_fuzz_libfuzzer.cpp rle.cpp
 *
 * 3. Run:
 *    ./a.out -max_len=1048576 -rss_limit_mb=2048
 */
