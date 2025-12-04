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

The test harness (`test_rle.cpp`) provides:

### Current Test Coverage
- ✅ Error code string validation
- ✅ Image structure validation
- ✅ Simple encode/decode roundtrip
- ✅ Empty data handling
- ✅ Run-length encoding efficiency
- ✅ Diverse data patterns (worst-case RLE)
- ✅ Image file I/O roundtrip
- ✅ Error path validation (invalid files, invalid images)
- ✅ Pattern data with mixed runs and literals

### Future Test Cases (Planned)
The following test areas are stubbed for future implementation:

1. **Utah RLE Conformance**
   - Cross-validation with utahrle reference implementation
   - File format compatibility tests
   - Decode comparison tests

2. **Extended Format Support**
   - Multi-channel images (grayscale, RGB, RGBA)
   - Large image handling
   - Edge cases (1x1, very wide, very tall images)

3. **Performance Benchmarks**
   - Encoding speed tests
   - Decoding speed tests
   - Memory usage profiling
   - Compression ratio analysis

4. **Robustness Testing**
   - Corrupted file handling
   - Truncated data recovery
   - Memory limit testing
   - Fuzz testing with random data

5. **Integration Tests**
   - End-to-end workflows
   - Real-world image samples
   - Interoperability with other tools

### Running Tests

```bash
# Build and run all tests
cmake --build build
cd build
ctest --output-on-failure --verbose

# Or run the test executable directly
./test_rle
```

## CI/CD

The project uses GitHub Actions for continuous integration across three platforms:
- **Linux** (Ubuntu latest)
- **macOS** (latest)
- **Windows** (latest, Visual Studio 2022)

The CI workflow:
1. Checks out code with submodules
2. Configures CMake with platform-specific generators
3. Builds the project in Release mode
4. Runs the test suite via CTest
5. Validates clean execution on all platforms

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
