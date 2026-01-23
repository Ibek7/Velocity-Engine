#include "network/PacketCompression.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Engine {

// PacketCompressor Implementation
bool PacketCompressor::compress(const uint8_t* data, size_t size,
                               std::vector<uint8_t>& outData,
                               CompressionAlgorithm algorithm) {
    outData.clear();
    
    // Add algorithm header
    outData.push_back(static_cast<uint8_t>(algorithm));
    
    switch (algorithm) {
        case CompressionAlgorithm::None:
            outData.insert(outData.end(), data, data + size);
            return true;
        case CompressionAlgorithm::RLE:
            return compressRLE(data, size, outData);
        case CompressionAlgorithm::Delta:
            return compressDelta(data, size, outData);
        case CompressionAlgorithm::LZ77:
            return compressLZ77(data, size, outData);
        default:
            return false;
    }
}

bool PacketCompressor::decompress(const uint8_t* data, size_t size,
                                 std::vector<uint8_t>& outData,
                                 CompressionAlgorithm algorithm) {
    if (size == 0) return false;
    
    CompressionAlgorithm actualAlgorithm = static_cast<CompressionAlgorithm>(data[0]);
    const uint8_t* compressedData = data + 1;
    size_t compressedSize = size - 1;
    
    switch (actualAlgorithm) {
        case CompressionAlgorithm::None:
            outData.insert(outData.end(), compressedData, compressedData + compressedSize);
            return true;
        case CompressionAlgorithm::RLE:
            return decompressRLE(compressedData, compressedSize, outData);
        case CompressionAlgorithm::Delta:
            return decompressDelta(compressedData, compressedSize, outData);
        case CompressionAlgorithm::LZ77:
            return decompressLZ77(compressedData, compressedSize, outData);
        default:
            return false;
    }
}

bool PacketCompressor::compressRLE(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    for (size_t i = 0; i < size; ) {
        uint8_t value = data[i];
        size_t count = 1;
        
        while (i + count < size && data[i + count] == value && count < 255) {
            count++;
        }
        
        out.push_back(static_cast<uint8_t>(count));
        out.push_back(value);
        i += count;
    }
    return true;
}

bool PacketCompressor::decompressRLE(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    for (size_t i = 0; i < size; i += 2) {
        if (i + 1 >= size) return false;
        
        uint8_t count = data[i];
        uint8_t value = data[i + 1];
        
        for (int j = 0; j < count; j++) {
            out.push_back(value);
        }
    }
    return true;
}

bool PacketCompressor::compressDelta(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    if (size == 0) return true;
    
    out.push_back(data[0]);
    for (size_t i = 1; i < size; i++) {
        int16_t delta = static_cast<int16_t>(data[i]) - static_cast<int16_t>(data[i - 1]);
        out.push_back(static_cast<uint8_t>(delta));
    }
    return true;
}

bool PacketCompressor::decompressDelta(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    if (size == 0) return true;
    
    out.push_back(data[0]);
    for (size_t i = 1; i < size; i++) {
        int16_t delta = static_cast<int8_t>(data[i]);
        uint8_t value = static_cast<uint8_t>(out.back() + delta);
        out.push_back(value);
    }
    return true;
}

bool PacketCompressor::compressLZ77(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    // Simplified LZ77 implementation
    const int windowSize = 256;
    
    for (size_t i = 0; i < size; ) {
        int bestLength = 0;
        int bestOffset = 0;
        
        int searchStart = std::max(0, static_cast<int>(i) - windowSize);
        
        for (int j = searchStart; j < static_cast<int>(i); j++) {
            int length = 0;
            while (i + length < size && data[j + length] == data[i + length] && length < 255) {
                length++;
            }
            
            if (length > bestLength) {
                bestLength = length;
                bestOffset = static_cast<int>(i) - j;
            }
        }
        
        if (bestLength > 2) {
            out.push_back(0xFF);  // Match marker
            out.push_back(static_cast<uint8_t>(bestOffset));
            out.push_back(static_cast<uint8_t>(bestLength));
            i += bestLength;
        } else {
            out.push_back(data[i]);
            i++;
        }
    }
    return true;
}

bool PacketCompressor::decompressLZ77(const uint8_t* data, size_t size, std::vector<uint8_t>& out) {
    for (size_t i = 0; i < size; ) {
        if (data[i] == 0xFF && i + 2 < size) {
            int offset = data[i + 1];
            int length = data[i + 2];
            
            size_t startPos = out.size() - offset;
            for (int j = 0; j < length; j++) {
                out.push_back(out[startPos + j]);
            }
            i += 3;
        } else {
            out.push_back(data[i]);
            i++;
        }
    }
    return true;
}

// BitPacker Implementation
BitPacker::BitPacker() : m_bitPosition(0) {
    m_data.reserve(128);
}

void BitPacker::writeBits(uint32_t value, int numBits) {
    for (int i = 0; i < numBits; i++) {
        size_t byteIndex = m_bitPosition / 8;
        size_t bitIndex = m_bitPosition % 8;
        
        if (byteIndex >= m_data.size()) {
            m_data.push_back(0);
        }
        
        if (value & (1 << i)) {
            m_data[byteIndex] |= (1 << bitIndex);
        }
        
        m_bitPosition++;
    }
}

void BitPacker::writeSignedBits(int32_t value, int numBits) {
    uint32_t unsignedValue = static_cast<uint32_t>(value);
    writeBits(unsignedValue, numBits);
}

void BitPacker::writeFloat(float value, float min, float max, int numBits) {
    float normalized = (value - min) / (max - min);
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    
    uint32_t quantized = static_cast<uint32_t>(normalized * ((1 << numBits) - 1));
    writeBits(quantized, numBits);
}

void BitPacker::writeBool(bool value) {
    writeBits(value ? 1 : 0, 1);
}

void BitPacker::clear() {
    m_data.clear();
    m_bitPosition = 0;
}

// BitUnpacker Implementation
BitUnpacker::BitUnpacker(const uint8_t* data, size_t sizeInBytes)
    : m_data(data)
    , m_sizeInBits(sizeInBytes * 8)
    , m_bitPosition(0) {
}

uint32_t BitUnpacker::readBits(int numBits) {
    uint32_t value = 0;
    
    for (int i = 0; i < numBits; i++) {
        if (m_bitPosition >= m_sizeInBits) break;
        
        size_t byteIndex = m_bitPosition / 8;
        size_t bitIndex = m_bitPosition % 8;
        
        if (m_data[byteIndex] & (1 << bitIndex)) {
            value |= (1 << i);
        }
        
        m_bitPosition++;
    }
    
    return value;
}

int32_t BitUnpacker::readSignedBits(int numBits) {
    uint32_t unsignedValue = readBits(numBits);
    return static_cast<int32_t>(unsignedValue);
}

float BitUnpacker::readFloat(float min, float max, int numBits) {
    uint32_t quantized = readBits(numBits);
    float normalized = static_cast<float>(quantized) / ((1 << numBits) - 1);
    return min + normalized * (max - min);
}

bool BitUnpacker::readBool() {
    return readBits(1) != 0;
}

} // namespace Engine
