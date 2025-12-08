# RLE - Portable Run-Length Encoding Library

[![Cross-Platform CI](https://github.com/starseeker/rle/actions/workflows/ci.yml/badge.svg)](https://github.com/starseeker/rle/actions/workflows/ci.yml)

A portable, hardened implementation of Run-Length Encoding (RLE) for image data, designed for cross-platform compatibility and integration testing with the Utah RLE reference implementation.

## Overview

This project provides a clean, modern C++ implementation of RLE encoding/decoding with:
- **Portable design**: Works on Linux, macOS, and Windows
- **Simple API**: Easy-to-use encoder/decoder interface
- **Error handling**: Comprehensive error reporting
- **Reference conformance**: Integration with BRL-CAD/utahrle for validation
- **Test harness**: Comprehensive testing framework for validation

## Building

### Prerequisites
- CMake 3.10 or later
- C++11 compatible compiler (GCC, Clang, MSVC)

### Build Instructions

```bash
# Clone the repository with submodules
git clone --recursive https://github.com/starseeker/rle.git
cd rle

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
cd build
ctest --output-on-failure
```

## Project Structure

```
rle/
├── rle.hpp              # Header file with RLE codec interface
├── rle.cpp              # Implementation of RLE encoder/decoder
├── test_rle.cpp         # Test harness with validation suite
├── CMakeLists.txt       # Build configuration
├── .github/workflows/
│   └── ci.yml          # Cross-platform CI workflow
├── utahrle/            # Utah RLE reference (submodule)
└── README.md           # This file
```

## API Usage

### Basic Example

```cpp
#include "rle.hpp"

// Create an image
rle::Image image(100, 100, 3);  // 100x100 RGB image
// ... fill image.data with pixel values ...

// Encode and write to file
rle::RLECodec codec;
rle::ErrorCode result = codec.write("output.rle", image);
if (result != rle::ErrorCode::OK) {
    std::cerr << "Error: " << codec.get_last_error() << std::endl;
}

// Read and decode from file
rle::Image loaded;
result = codec.read("output.rle", loaded);
if (result != rle::ErrorCode::OK) {
    std::cerr << "Error: " << codec.get_last_error() << std::endl;
}

// Validate roundtrip
if (rle::validate_roundtrip(image, loaded)) {
    std::cout << "Roundtrip successful!" << std::endl;
}
```

### Direct Encode/Decode

```cpp
#include "rle.hpp"

rle::RLECodec codec;
std::vector<uint8_t> raw_data = { /* your data */ };
std::vector<uint8_t> encoded, decoded;

// Encode
codec.encode(raw_data, encoded);

// Decode
codec.decode(encoded, decoded);
```

## Testing Framework

The project includes two comprehensive test suites:
- **test_rle.cpp**: Primary test suite for functional validation and Utah RLE compatibility
- **test_coverage.cpp**: Extended test suite focused on code coverage and edge cases

### Test Coverage (37 Tests Total - All Passing)

**Code Coverage**: 89.1% line coverage, 97.7% function coverage

#### Primary Test Suite (test_rle.cpp - 19 Tests)

#### Basic API Tests
- ✅ Error code string validation
- ✅ Header validation with various configurations

#### Basic I/O Tests
- ✅ Simple write/read roundtrip
- ✅ Solid color image handling
- ✅ Gradient pattern encoding

#### Corner Case Tests
- ✅ Minimum size image (1x1)
- ✅ Wide images (256x1)
- ✅ Tall images (1x256)
- ✅ Checkerboard pattern (worst-case for RLE)

#### Error Handling Tests
- ✅ Null image write error handling
- ✅ Invalid file read error handling
- ✅ Corrupted header detection

#### Stress Tests
- ✅ Large images (512x512)
- ✅ Random noise patterns

#### Utah RLE Compatibility Tests
- ✅ Read files written by utahrle
- ✅ Utahrle reads files written by our implementation
- ✅ Bidirectional roundtrip compatibility
- ✅ Edge case compatibility (1x1 images)
- ✅ Large image compatibility

### Key Findings

During testing, we identified and fixed one behavioral difference between the RLE specification and the actual utahrle implementation:

**Null Byte with NO_BACKGROUND Flag**: When the NO_BACKGROUND flag is set, utahrle writes a single null byte after the header. Our implementation was updated to match this behavior for full compatibility.

See [TEST_RESULTS.md](TEST_RESULTS.md) for detailed test results and findings.

#### Extended Coverage Test Suite (test_coverage.cpp - 18 Tests)
- ✅ Comprehensive error string coverage
- ✅ Invalid header validation (dimensions, pixelbits, ncolors, background)
- ✅ Write error paths (invalid channels, oversized dimensions)
- ✅ Read error paths (null pointer, truncated header)
- ✅ Format features (comments, alpha channel, grayscale, various color counts)
- ✅ Background mode coverage (different detection modes)
- ✅ Flag combinations (CLEAR_FIRST, NO_BACKGROUND, ALPHA)
- ✅ Colormap validation and edge cases

### Running Tests

```bash
# Build and run all tests
cmake --build build
cd build
ctest --output-on-failure --verbose

# Or run test executables directly
./test_rle
./test_coverage
```

### Code Coverage Analysis

To generate a code coverage report:

```bash
# Configure with coverage flags
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" -DCMAKE_C_FLAGS="--coverage"

# Build and run tests
cmake --build build
cd build
./test_rle
./test_coverage

# Generate coverage report (requires lcov)
lcov --capture --directory . --output-file coverage.info \
     --exclude '/usr/*' --exclude '*/utahrle/*' --exclude '*/test_*.cpp'
genhtml coverage.info --output-directory coverage_html

# View the report
open coverage_html/index.html  # macOS
xdg-open coverage_html/index.html  # Linux
```

Current coverage metrics:
- **Lines**: 89.1% (498 of 559 lines)
- **Functions**: 97.7% (42 of 43 functions)

Uncovered code primarily consists of:
- Rare error paths (memory allocation failures, internal errors)
- Alternative background detection modes (BG_OVERLAY, BG_CLEAR)
- Edge cases in format conversion

## CI/CD

The project uses GitHub Actions for continuous integration with two workflows:

### Build and Test Workflow
Runs on three platforms:
- **Linux** (Ubuntu latest)
- **macOS** (latest)
- **Windows** (latest, Visual Studio 2022)

Steps:
1. Checks out code with submodules
2. Configures CMake with platform-specific generators
3. Builds the project in Release mode
4. Runs the test suite via CTest
5. Validates clean execution on all platforms

### Code Coverage Workflow
Runs on Ubuntu with:
1. Installation of lcov coverage tools
2. Debug build with coverage instrumentation
3. Execution of all test suites
4. Generation of detailed coverage reports
5. Upload of HTML coverage report as artifact

See [`.github/workflows/ci.yml`](.github/workflows/ci.yml) for details.

## Utah RLE Reference

This project includes the BRL-CAD Utah RLE library as a git submodule for reference conformance testing. The submodule provides:
- Reference implementation for validation
- Test data and examples
- Format specification compliance

To update the submodule:
```bash
git submodule update --remote utahrle
```

## Error Handling

All API functions return `ErrorCode` enum values:
- `OK`: Operation successful
- `FILE_NOT_FOUND`: Input file not found
- `INVALID_FORMAT`: Invalid RLE format or corrupted data
- `READ_ERROR`: File read failure
- `WRITE_ERROR`: File write failure
- `MEMORY_ERROR`: Memory allocation failure
- `INVALID_DIMENSIONS`: Invalid image dimensions
- `UNSUPPORTED_FORMAT`: Unsupported image format

Use `codec.get_last_error()` to retrieve detailed error messages.

## Contributing

When adding new features or tests:
1. Ensure all existing tests pass
2. Add appropriate test cases in `test_rle.cpp`
3. Update this README with new functionality
4. Verify CI passes on all platforms

## License

(License information to be added)

## References

- [BRL-CAD Utah RLE Library](https://github.com/BRL-CAD/utahrle)
- [Run-Length Encoding on Wikipedia](https://en.wikipedia.org/wiki/Run-length_encoding)
