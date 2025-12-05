/**
 * @file rle.hpp
 * @brief Portable RLE (Run-Length Encoded) image encoder/decoder
 *
 * This is a hardened, portable implementation of RLE image encoding/decoding
 * designed for cross-platform use. It provides basic interfaces for reading
 * and writing RLE format images with error handling.
 */

#ifndef RLE_HPP
#define RLE_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace rle {

/**
 * @brief Error codes for RLE operations
 */
enum class ErrorCode {
    OK = 0,
    FILE_NOT_FOUND,
    INVALID_FORMAT,
    READ_ERROR,
    WRITE_ERROR,
    MEMORY_ERROR,
    INVALID_DIMENSIONS,
    UNSUPPORTED_FORMAT
};

/**
 * @brief Convert error code to string
 */
const char* error_string(ErrorCode code);

/**
 * @brief Image data structure
 */
struct Image {
    int width;
    int height;
    int channels;  // Number of color channels (e.g., 3 for RGB, 4 for RGBA)
    std::vector<uint8_t> data;  // Pixel data in row-major order
    
    Image() : width(0), height(0), channels(0) {}
    Image(int w, int h, int c) : width(w), height(h), channels(c) {
        data.resize(w * h * c);
    }
    
    size_t size() const { return width * height * channels; }
    bool valid() const { return width > 0 && height > 0 && channels > 0 && data.size() == size(); }
};

/**
 * @brief RLE Encoder/Decoder class
 */
class RLECodec {
public:
    RLECodec();
    ~RLECodec();
    
    /**
     * @brief Read an RLE image from file
     * @param filename Path to the RLE file
     * @param image Output image data
     * @return Error code indicating success or failure
     */
    ErrorCode read(const std::string& filename, Image& image);
    
    /**
     * @brief Write an RLE image to file
     * @param filename Path to the output RLE file
     * @param image Input image data
     * @return Error code indicating success or failure
     */
    ErrorCode write(const std::string& filename, const Image& image);
    
    /**
     * @brief Encode raw image data to RLE format
     * @param input Raw pixel data
     * @param output Encoded RLE data
     * @return Error code indicating success or failure
     */
    ErrorCode encode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    
    /**
     * @brief Decode RLE data to raw image format
     * @param input Encoded RLE data
     * @param output Decoded pixel data
     * @return Error code indicating success or failure
     */
    ErrorCode decode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    
    /**
     * @brief Get the last error message
     */
    std::string get_last_error() const { return last_error_; }
    
private:
    std::string last_error_;
    
    void set_error(const std::string& error) { last_error_ = error; }
};

/**
 * @brief Simple roundtrip validation helper
 * @param original Original image data
 * @param roundtripped Image data after encode->decode cycle
 * @return true if images match, false otherwise
 */
bool validate_roundtrip(const Image& original, const Image& roundtripped);

} // namespace rle

#endif // RLE_HPP
