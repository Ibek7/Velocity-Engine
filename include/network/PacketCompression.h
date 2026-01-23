#pragma once

#include <vector>
#include <cstdint>
#include <string>

/**
 * @file PacketCompression.h
 * @brief Network packet compression utilities
 * 
 * Provides various compression algorithms optimized for network packets,
 * including delta compression, run-length encoding, and LZ-based compression.
 */

namespace Engine {

/**
 * @enum CompressionAlgorithm
 * @brief Available compression algorithms
 */
enum class CompressionAlgorithm {
    None,           ///< No compression
    RLE,            ///< Run-length encoding (good for repetitive data)
    Delta,          ///< Delta encoding (good for incremental updates)
    LZ77,           ///< LZ77 compression (general purpose)
    Huffman,        ///< Huffman coding (good for text/symbols)
    Hybrid          ///< Combination of multiple algorithms
};

/**
 * @class PacketCompressor
 * @brief Compresses and decompresses network packets
 */
class PacketCompressor {
public:
    /**
     * @brief Compress data using specified algorithm
     * @param data Input data
     * @param size Input size
     * @param outData Output compressed data
     * @param outSize Output compressed size
     * @param algorithm Compression algorithm
     * @return True if successful
     */
    static bool compress(const uint8_t* data, size_t size,
                        std::vector<uint8_t>& outData,
                        CompressionAlgorithm algorithm = CompressionAlgorithm::Hybrid);
    
    /**
     * @brief Decompress data
     * @param data Compressed data
     * @param size Compressed size
     * @param outData Output decompressed data
     * @param algorithm Compression algorithm used
     * @return True if successful
     */
    static bool decompress(const uint8_t* data, size_t size,
                          std::vector<uint8_t>& outData,
                          CompressionAlgorithm algorithm = CompressionAlgorithm::Hybrid);
    
    /**
     * @brief Estimate compression ratio
     * @param data Input data
     * @param size Input size
     * @param algorithm Algorithm to test
     * @return Estimated compression ratio (< 1.0 means compression)
     */
    static float estimateCompressionRatio(const uint8_t* data, size_t size,
                                         CompressionAlgorithm algorithm);
    
    /**
     * @brief Get recommended algorithm for data
     * @param data Input data
     * @param size Input size
     * @return Recommended algorithm
     */
    static CompressionAlgorithm recommendAlgorithm(const uint8_t* data, size_t size);

private:
    static bool compressRLE(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
    static bool decompressRLE(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
    
    static bool compressDelta(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
    static bool decompressDelta(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
    
    static bool compressLZ77(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
    static bool decompressLZ77(const uint8_t* data, size_t size, std::vector<uint8_t>& out);
};

/**
 * @class DeltaEncoder
 * @brief Encodes incremental state updates efficiently
 */
class DeltaEncoder {
public:
    /**
     * @brief Encode delta between old and new state
     * @param oldState Previous state
     * @param newState Current state
     * @param outDelta Output delta data
     * @return True if delta encoding beneficial
     */
    static bool encodeDelta(const std::vector<uint8_t>& oldState,
                           const std::vector<uint8_t>& newState,
                           std::vector<uint8_t>& outDelta);
    
    /**
     * @brief Apply delta to reconstruct new state
     * @param oldState Previous state
     * @param delta Delta data
     * @param outNewState Reconstructed state
     * @return True if successful
     */
    static bool applyDelta(const std::vector<uint8_t>& oldState,
                          const std::vector<uint8_t>& delta,
                          std::vector<uint8_t>& outNewState);
    
    /**
     * @brief Encode float delta with tolerance
     * @param oldValue Previous value
     * @param newValue Current value
     * @param tolerance Minimum difference to encode
     * @param outDelta Output encoded delta
     * @return True if change significant enough
     */
    static bool encodeFloatDelta(float oldValue, float newValue, float tolerance,
                                std::vector<uint8_t>& outDelta);
};

/**
 * @class BitPacker
 * @brief Packs data into compact bit representations
 */
class BitPacker {
public:
    BitPacker();
    
    /**
     * @brief Write bits to buffer
     * @param value Value to write
     * @param numBits Number of bits to write
     */
    void writeBits(uint32_t value, int numBits);
    
    /**
     * @brief Write signed integer
     * @param value Value to write
     * @param numBits Number of bits
     */
    void writeSignedBits(int32_t value, int numBits);
    
    /**
     * @brief Write float with quantization
     * @param value Float value
     * @param min Minimum value
     * @param max Maximum value
     * @param numBits Bits for quantization
     */
    void writeFloat(float value, float min, float max, int numBits);
    
    /**
     * @brief Write boolean
     * @param value Boolean value
     */
    void writeBool(bool value);
    
    /**
     * @brief Get packed data
     * @return Packed byte array
     */
    const std::vector<uint8_t>& getData() const { return m_data; }
    
    /**
     * @brief Get bit size
     * @return Number of bits written
     */
    size_t getBitSize() const { return m_bitPosition; }
    
    /**
     * @brief Clear buffer
     */
    void clear();

private:
    std::vector<uint8_t> m_data;
    size_t m_bitPosition;
};

/**
 * @class BitUnpacker
 * @brief Unpacks bit-packed data
 */
class BitUnpacker {
public:
    BitUnpacker(const uint8_t* data, size_t sizeInBytes);
    
    /**
     * @brief Read bits from buffer
     * @param numBits Number of bits to read
     * @return Read value
     */
    uint32_t readBits(int numBits);
    
    /**
     * @brief Read signed integer
     * @param numBits Number of bits
     * @return Read value
     */
    int32_t readSignedBits(int numBits);
    
    /**
     * @brief Read quantized float
     * @param min Minimum value
     * @param max Maximum value
     * @param numBits Bits used for quantization
     * @return Float value
     */
    float readFloat(float min, float max, int numBits);
    
    /**
     * @brief Read boolean
     * @return Boolean value
     */
    bool readBool();
    
    /**
     * @brief Get current bit position
     * @return Bit position
     */
    size_t getBitPosition() const { return m_bitPosition; }
    
    /**
     * @brief Check if more data available
     * @return True if more data
     */
    bool hasMoreData() const { return m_bitPosition < m_sizeInBits; }

private:
    const uint8_t* m_data;
    size_t m_sizeInBits;
    size_t m_bitPosition;
};

/**
 * @class NetworkBufferOptimizer
 * @brief Optimizes network buffer usage
 */
class NetworkBufferOptimizer {
public:
    /**
     * @brief Analyze buffer for optimization opportunities
     * @param data Buffer data
     * @param size Buffer size
     * @return Optimization report
     */
    struct OptimizationReport {
        float compressionRatio;
        CompressionAlgorithm recommendedAlgorithm;
        size_t potentialSavings;
        bool worthCompressing;
    };
    
    static OptimizationReport analyze(const uint8_t* data, size_t size);
    
    /**
     * @brief Pack struct with bit-level precision
     * @param data Structure data
     * @param size Structure size
     * @param fieldBits Bits per field
     * @param outPacked Output packed data
     * @return True if successful
     */
    static bool packStruct(const uint8_t* data, size_t size,
                          const std::vector<int>& fieldBits,
                          std::vector<uint8_t>& outPacked);
};

} // namespace Engine
