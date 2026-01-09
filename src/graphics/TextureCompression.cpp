#include "graphics/TextureCompression.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace JJM {
namespace Graphics {

CompressedTextureData TextureCompression::compress(
    const uint8_t* data,
    int width,
    int height,
    const CompressionParams& params)
{
    if (!data || width <= 0 || height <= 0) {
        throw std::invalid_argument("Invalid texture data or dimensions");
    }
    
    if (!isFormatSupported(params.format)) {
        throw std::runtime_error("Compression format not supported on this platform");
    }
    
    CompressedTextureData result;
    result.format = params.format;
    result.width = width;
    result.height = height;
    result.mipLevels = params.generateMipmaps ? 
        static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1 : 1;
    
    // Compress base level
    std::vector<uint8_t> compressed;
    
    switch (params.format) {
        case CompressionFormat::BC1:
            compressed = compressBC1(data, width, height);
            break;
        case CompressionFormat::BC3:
            compressed = compressBC3(data, width, height);
            break;
        case CompressionFormat::BC5:
            compressed = compressBC5(data, width, height);
            break;
        case CompressionFormat::BC7:
            compressed = compressBC7(data, width, height, params.quality);
            break;
        case CompressionFormat::ETC2_RGB:
        case CompressionFormat::ETC2_RGBA:
            compressed = compressETC2(data, width, height, params.format == CompressionFormat::ETC2_RGBA);
            break;
        case CompressionFormat::ASTC_4x4:
        case CompressionFormat::ASTC_6x6:
        case CompressionFormat::ASTC_8x8: {
            int bw, bh;
            getBlockSize(params.format, bw, bh);
            compressed = compressASTC(data, width, height, bw, bh, params.quality);
            break;
        }
        default:
            throw std::runtime_error("Compression format not implemented");
    }
    
    result.mipData.push_back(std::move(compressed));
    result.totalSize = result.mipData[0].size();
    
    // Generate mipmaps if requested
    if (params.generateMipmaps && result.mipLevels > 1) {
        auto mipmaps = generateMipmaps(data, width, height, params);
        for (size_t i = 1; i < mipmaps.size(); ++i) {
            result.mipData.push_back(std::move(mipmaps[i]));
            result.totalSize += result.mipData.back().size();
        }
    }
    
    return result;
}

std::vector<uint8_t> TextureCompression::decompress(
    const CompressedTextureData& compressed,
    int mipLevel)
{
    if (!compressed.isValid() || mipLevel >= compressed.mipLevels) {
        throw std::invalid_argument("Invalid compressed texture or mip level");
    }
    
    int mipWidth = std::max(1, compressed.width >> mipLevel);
    int mipHeight = std::max(1, compressed.height >> mipLevel);
    const uint8_t* data = compressed.mipData[mipLevel].data();
    
    switch (compressed.format) {
        case CompressionFormat::BC1:
            return decompressBC1(data, mipWidth, mipHeight);
        case CompressionFormat::BC3:
            return decompressBC3(data, mipWidth, mipHeight);
        case CompressionFormat::BC5:
            return decompressBC5(data, mipWidth, mipHeight);
        case CompressionFormat::ETC2_RGB:
        case CompressionFormat::ETC2_RGBA:
            return decompressETC2(data, mipWidth, mipHeight, compressed.format == CompressionFormat::ETC2_RGBA);
        case CompressionFormat::ASTC_4x4:
        case CompressionFormat::ASTC_6x6:
        case CompressionFormat::ASTC_8x8: {
            int bw, bh;
            getBlockSize(compressed.format, bw, bh);
            return decompressASTC(data, mipWidth, mipHeight, bw, bh);
        }
        default:
            throw std::runtime_error("Decompression format not implemented");
    }
}

bool TextureCompression::isFormatSupported(CompressionFormat format) {
    // In a real implementation, query GPU capabilities
    // For now, assume most formats are supported
    switch (format) {
        case CompressionFormat::BC1:
        case CompressionFormat::BC3:
        case CompressionFormat::BC5:
        case CompressionFormat::BC7:
        case CompressionFormat::ETC2_RGB:
        case CompressionFormat::ETC2_RGBA:
        case CompressionFormat::ASTC_4x4:
        case CompressionFormat::ASTC_6x6:
        case CompressionFormat::ASTC_8x8:
            return true;
        default:
            return false;
    }
}

CompressionFormat TextureCompression::getRecommendedFormat(bool hasAlpha, bool isNormalMap) {
    // Platform-specific recommendations
    // Desktop (Windows/DirectX) -> BC formats
    // Mobile (iOS/Android) -> ASTC formats
    // Fallback -> ETC2 formats
    
    if (isNormalMap) {
        return CompressionFormat::BC5;  // Best for normal maps
    }
    
    if (hasAlpha) {
        return CompressionFormat::BC7;  // High quality RGBA
    }
    
    return CompressionFormat::BC1;  // Simple RGB
}

size_t TextureCompression::calculateCompressedSize(
    int width,
    int height,
    CompressionFormat format,
    int mipLevels)
{
    size_t totalSize = 0;
    int bytesPerBlock = getBytesPerBlock(format);
    int blockWidth, blockHeight;
    getBlockSize(format, blockWidth, blockHeight);
    
    for (int mip = 0; mip < mipLevels; ++mip) {
        int mipWidth = std::max(1, width >> mip);
        int mipHeight = std::max(1, height >> mip);
        
        int blocksX = (mipWidth + blockWidth - 1) / blockWidth;
        int blocksY = (mipHeight + blockHeight - 1) / blockHeight;
        
        totalSize += blocksX * blocksY * bytesPerBlock;
    }
    
    return totalSize;
}

void TextureCompression::getBlockSize(CompressionFormat format, int& blockWidth, int& blockHeight) {
    switch (format) {
        case CompressionFormat::BC1:
        case CompressionFormat::BC2:
        case CompressionFormat::BC3:
        case CompressionFormat::BC4:
        case CompressionFormat::BC5:
        case CompressionFormat::BC6H:
        case CompressionFormat::BC7:
        case CompressionFormat::ETC1:
        case CompressionFormat::ETC2_RGB:
        case CompressionFormat::ETC2_RGBA:
            blockWidth = blockHeight = 4;
            break;
        case CompressionFormat::ASTC_4x4:
            blockWidth = blockHeight = 4;
            break;
        case CompressionFormat::ASTC_5x4:
            blockWidth = 5; blockHeight = 4;
            break;
        case CompressionFormat::ASTC_6x6:
            blockWidth = blockHeight = 6;
            break;
        case CompressionFormat::ASTC_8x8:
            blockWidth = blockHeight = 8;
            break;
        default:
            blockWidth = blockHeight = 1;
    }
}

int TextureCompression::getBytesPerBlock(CompressionFormat format) {
    switch (format) {
        case CompressionFormat::BC1:
        case CompressionFormat::BC4:
        case CompressionFormat::ETC1:
        case CompressionFormat::ETC2_RGB:
            return 8;
        case CompressionFormat::BC2:
        case CompressionFormat::BC3:
        case CompressionFormat::BC5:
        case CompressionFormat::BC6H:
        case CompressionFormat::BC7:
        case CompressionFormat::ETC2_RGBA:
            return 16;
        case CompressionFormat::ASTC_4x4:
        case CompressionFormat::ASTC_5x4:
        case CompressionFormat::ASTC_6x6:
        case CompressionFormat::ASTC_8x8:
            return 16;  // ASTC always uses 16 bytes per block
        default:
            return 0;
    }
}

std::string TextureCompression::getFormatName(CompressionFormat format) {
    switch (format) {
        case CompressionFormat::BC1: return "BC1 (DXT1)";
        case CompressionFormat::BC3: return "BC3 (DXT5)";
        case CompressionFormat::BC5: return "BC5";
        case CompressionFormat::BC7: return "BC7";
        case CompressionFormat::ETC2_RGB: return "ETC2 RGB";
        case CompressionFormat::ETC2_RGBA: return "ETC2 RGBA";
        case CompressionFormat::ASTC_4x4: return "ASTC 4x4";
        case CompressionFormat::ASTC_6x6: return "ASTC 6x6";
        case CompressionFormat::ASTC_8x8: return "ASTC 8x8";
        default: return "Unknown";
    }
}

std::vector<std::vector<uint8_t>> TextureCompression::generateMipmaps(
    const uint8_t* data,
    int width,
    int height,
    const CompressionParams& params)
{
    std::vector<std::vector<uint8_t>> mipmaps;
    
    // TODO: Implement proper mipmap generation with filtering
    // For now, just return empty vector - mipmaps will be generated during compression
    
    return mipmaps;
}

// Simplified BC1 compression (DXT1)
std::vector<uint8_t> TextureCompression::compressBC1(const uint8_t* data, int width, int height) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    std::vector<uint8_t> compressed(blocksX * blocksY * 8);
    
    // TODO: Implement actual BC1 compression algorithm
    // This is a placeholder that would need a proper implementation
    // using color endpoint encoding and index selection
    
    return compressed;
}

std::vector<uint8_t> TextureCompression::compressBC3(const uint8_t* data, int width, int height) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    std::vector<uint8_t> compressed(blocksX * blocksY * 16);
    
    // TODO: Implement BC3 compression (BC1 RGB + BC4 Alpha)
    
    return compressed;
}

std::vector<uint8_t> TextureCompression::compressBC5(const uint8_t* data, int width, int height) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    std::vector<uint8_t> compressed(blocksX * blocksY * 16);
    
    // TODO: Implement BC5 compression (two BC4 channels for normal maps)
    
    return compressed;
}

std::vector<uint8_t> TextureCompression::compressBC7(const uint8_t* data, int width, int height, CompressionQuality quality) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    std::vector<uint8_t> compressed(blocksX * blocksY * 16);
    
    // TODO: Implement BC7 compression with quality modes
    
    return compressed;
}

std::vector<uint8_t> TextureCompression::compressETC2(const uint8_t* data, int width, int height, bool hasAlpha) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    int blockSize = hasAlpha ? 16 : 8;
    std::vector<uint8_t> compressed(blocksX * blocksY * blockSize);
    
    // TODO: Implement ETC2 compression
    
    return compressed;
}

std::vector<uint8_t> TextureCompression::compressASTC(const uint8_t* data, int width, int height,
                                                      int blockWidth, int blockHeight, CompressionQuality quality) {
    int blocksX = (width + blockWidth - 1) / blockWidth;
    int blocksY = (height + blockHeight - 1) / blockHeight;
    std::vector<uint8_t> compressed(blocksX * blocksY * 16);
    
    // TODO: Implement ASTC compression
    
    return compressed;
}

std::vector<uint8_t> TextureCompression::decompressBC1(const uint8_t* data, int width, int height) {
    std::vector<uint8_t> decompressed(width * height * 4);
    
    // TODO: Implement BC1 decompression
    
    return decompressed;
}

std::vector<uint8_t> TextureCompression::decompressBC3(const uint8_t* data, int width, int height) {
    std::vector<uint8_t> decompressed(width * height * 4);
    
    // TODO: Implement BC3 decompression
    
    return decompressed;
}

std::vector<uint8_t> TextureCompression::decompressBC5(const uint8_t* data, int width, int height) {
    std::vector<uint8_t> decompressed(width * height * 4);
    
    // TODO: Implement BC5 decompression
    
    return decompressed;
}

std::vector<uint8_t> TextureCompression::decompressETC2(const uint8_t* data, int width, int height, bool hasAlpha) {
    std::vector<uint8_t> decompressed(width * height * 4);
    
    // TODO: Implement ETC2 decompression
    
    return decompressed;
}

std::vector<uint8_t> TextureCompression::decompressASTC(const uint8_t* data, int width, int height,
                                                       int blockWidth, int blockHeight) {
    std::vector<uint8_t> decompressed(width * height * 4);
    
    // TODO: Implement ASTC decompression
    
    return decompressed;
}

} // namespace Graphics
} // namespace JJM
