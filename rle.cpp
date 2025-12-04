/**
 * @file rle.cpp
 * @brief Implementation of portable RLE encoder/decoder
 */

#include "rle.hpp"
#include <fstream>
#include <cstring>
#include <sstream>

namespace rle {

const char* error_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::OK: return "Success";
        case ErrorCode::FILE_NOT_FOUND: return "File not found";
        case ErrorCode::INVALID_FORMAT: return "Invalid format";
        case ErrorCode::READ_ERROR: return "Read error";
        case ErrorCode::WRITE_ERROR: return "Write error";
        case ErrorCode::MEMORY_ERROR: return "Memory error";
        case ErrorCode::INVALID_DIMENSIONS: return "Invalid dimensions";
        case ErrorCode::UNSUPPORTED_FORMAT: return "Unsupported format";
        default: return "Unknown error";
    }
}

RLECodec::RLECodec() : last_error_("") {}

RLECodec::~RLECodec() {}

ErrorCode RLECodec::read(const std::string& filename, Image& image) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        set_error("Failed to open file: " + filename);
        return ErrorCode::FILE_NOT_FOUND;
    }
    
    // Simple RLE header format (for now, just dimensions and channels)
    // Format: "RLE\n" magic, width, height, channels (4 bytes each)
    char magic[4];
    file.read(magic, 4);
    if (!file || std::strncmp(magic, "RLE\n", 4) != 0) {
        set_error("Invalid RLE magic header");
        return ErrorCode::INVALID_FORMAT;
    }
    
    int32_t width, height, channels;
    file.read(reinterpret_cast<char*>(&width), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&height), sizeof(int32_t));
    file.read(reinterpret_cast<char*>(&channels), sizeof(int32_t));
    
    if (!file || width <= 0 || height <= 0 || channels <= 0 || channels > 4) {
        set_error("Invalid image dimensions");
        return ErrorCode::INVALID_DIMENSIONS;
    }
    
    // Read encoded data
    std::vector<uint8_t> encoded_data;
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(16, std::ios::beg);  // Skip header (4 + 3*4 = 16 bytes)
    
    size_t encoded_size = file_size - 16;
    encoded_data.resize(encoded_size);
    file.read(reinterpret_cast<char*>(encoded_data.data()), encoded_size);
    
    if (!file) {
        set_error("Failed to read encoded data");
        return ErrorCode::READ_ERROR;
    }
    
    file.close();
    
    // Decode the data
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.data.clear();
    
    ErrorCode result = decode(encoded_data, image.data);
    if (result != ErrorCode::OK) {
        return result;
    }
    
    // Validate decoded size
    if (image.data.size() != static_cast<size_t>(width * height * channels)) {
        set_error("Decoded size mismatch");
        return ErrorCode::INVALID_FORMAT;
    }
    
    return ErrorCode::OK;
}

ErrorCode RLECodec::write(const std::string& filename, const Image& image) {
    if (!image.valid()) {
        set_error("Invalid image data");
        return ErrorCode::INVALID_DIMENSIONS;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        set_error("Failed to open file for writing: " + filename);
        return ErrorCode::WRITE_ERROR;
    }
    
    // Write header
    file.write("RLE\n", 4);
    int32_t width = image.width;
    int32_t height = image.height;
    int32_t channels = image.channels;
    file.write(reinterpret_cast<const char*>(&width), sizeof(int32_t));
    file.write(reinterpret_cast<const char*>(&height), sizeof(int32_t));
    file.write(reinterpret_cast<const char*>(&channels), sizeof(int32_t));
    
    if (!file) {
        set_error("Failed to write header");
        return ErrorCode::WRITE_ERROR;
    }
    
    // Encode and write data
    std::vector<uint8_t> encoded_data;
    ErrorCode result = encode(image.data, encoded_data);
    if (result != ErrorCode::OK) {
        return result;
    }
    
    file.write(reinterpret_cast<const char*>(encoded_data.data()), encoded_data.size());
    
    if (!file) {
        set_error("Failed to write encoded data");
        return ErrorCode::WRITE_ERROR;
    }
    
    file.close();
    return ErrorCode::OK;
}

ErrorCode RLECodec::encode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.clear();
    
    if (input.empty()) {
        return ErrorCode::OK;
    }
    
    try {
        // Simple RLE encoding: [count][value] pairs
        // If count >= 128, it's a run, otherwise it's literal data
        size_t i = 0;
        while (i < input.size()) {
            // Check for run
            size_t run_length = 1;
            while (i + run_length < input.size() && 
                   input[i] == input[i + run_length] && 
                   run_length < 127) {
                run_length++;
            }
            
            if (run_length >= 3) {
                // Encode as run: [128 + count][value]
                output.push_back(128 + static_cast<uint8_t>(run_length));
                output.push_back(input[i]);
                i += run_length;
            } else {
                // Encode as literal: [count][values...]
                size_t literal_length = 1;
                while (i + literal_length < input.size() && literal_length < 127) {
                    // Check if next bytes form a run
                    size_t next_run = 1;
                    while (i + literal_length + next_run < input.size() &&
                           input[i + literal_length] == input[i + literal_length + next_run] &&
                           next_run < 3) {
                        next_run++;
                    }
                    if (next_run >= 3) break;  // Start of a new run
                    literal_length++;
                }
                
                output.push_back(static_cast<uint8_t>(literal_length));
                for (size_t j = 0; j < literal_length; j++) {
                    output.push_back(input[i + j]);
                }
                i += literal_length;
            }
        }
    } catch (const std::exception& e) {
        set_error(std::string("Encoding error: ") + e.what());
        return ErrorCode::MEMORY_ERROR;
    }
    
    return ErrorCode::OK;
}

ErrorCode RLECodec::decode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.clear();
    
    if (input.empty()) {
        return ErrorCode::OK;
    }
    
    try {
        size_t i = 0;
        while (i < input.size()) {
            uint8_t count_byte = input[i++];
            
            if (count_byte >= 128) {
                // Run of repeated values
                size_t count = count_byte - 128;
                if (count == 0) {
                    set_error("Invalid run count (must be >= 1)");
                    return ErrorCode::INVALID_FORMAT;
                }
                if (i >= input.size()) {
                    set_error("Invalid run encoding");
                    return ErrorCode::INVALID_FORMAT;
                }
                uint8_t value = input[i++];
                for (size_t j = 0; j < count; j++) {
                    output.push_back(value);
                }
            } else {
                // Literal values
                size_t count = count_byte;
                if (count == 0) {
                    set_error("Invalid literal count (must be >= 1)");
                    return ErrorCode::INVALID_FORMAT;
                }
                if (i + count > input.size()) {
                    set_error("Invalid literal encoding");
                    return ErrorCode::INVALID_FORMAT;
                }
                for (size_t j = 0; j < count; j++) {
                    output.push_back(input[i++]);
                }
            }
        }
    } catch (const std::exception& e) {
        set_error(std::string("Decoding error: ") + e.what());
        return ErrorCode::MEMORY_ERROR;
    }
    
    return ErrorCode::OK;
}

bool validate_roundtrip(const Image& original, const Image& roundtripped) {
    if (original.width != roundtripped.width ||
        original.height != roundtripped.height ||
        original.channels != roundtripped.channels) {
        return false;
    }
    
    if (original.data.size() != roundtripped.data.size()) {
        return false;
    }
    
    return std::memcmp(original.data.data(), roundtripped.data.data(), 
                       original.data.size()) == 0;
}

} // namespace rle
