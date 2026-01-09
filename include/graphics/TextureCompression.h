#ifndef TEXTURE_COMPRESSION_H
#define TEXTURE_COMPRESSION_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace JJM {
namespace Graphics {

/**
 * @brief Supported texture compression formats
 */
enum class CompressionFormat {
    None,
    // Block Compression (BC) - DirectX/Windows
    BC1,         // DXT1 - RGB with 1-bit alpha
    BC2,         // DXT3 - RGBA with explicit alpha
    BC3,         // DXT5 - RGBA with interpolated alpha
    BC4,         // Single channel
    BC5,         // Two channels (normal maps)
    BC6H,        // HDR RGB
    BC7,         // High quality RGBA
    // Ericsson Texture Compression (ETC)
    ETC1,        // RGB only
    ETC2_RGB,    // RGB
    ETC2_RGBA,   // RGBA
    ETC2_RGB_A1, // RGB with 1-bit alpha
    EAC_R11,     // Single channel 11-bit
    EAC_RG11,    // Two channels 11-bit
    // Adaptive Scalable Texture Compression (ASTC)
    ASTC_4x4,
    ASTC_5x4,
    ASTC_5x5,
    ASTC_6x5,
    ASTC_6x6,
    ASTC_8x5,
    ASTC_8x6,
    ASTC_8x8,
    ASTC_10x5,
    ASTC_10x6,
    ASTC_10x8,
    ASTC_10x10,
    ASTC_12x10,
    ASTC_12x12
};

/**
 * @brief Compression quality settings
 */
enum class CompressionQuality {
    Fast,       // Quick compression, lower quality
    Normal,     // Balanced quality/speed
    High,       // Better quality, slower
    Maximum     // Best quality, slowest
};

/**
 * @brief Texture compression parameters
 */
struct CompressionParams {
    CompressionFormat format;
    CompressionQuality quality;
    bool generateMipmaps;
    bool sRGB;               // sRGB color space
    bool normalMap;          // Optimize for normal maps
    bool preserveAlpha;      // Ensure alpha channel is preserved
    float alphaThreshold;    // For BC1/ETC2_RGB_A1 alpha testing
    
    CompressionParams()
        : format(CompressionFormat::None)
        , quality(CompressionQuality::Normal)
        , generateMipmaps(true)
        , sRGB(false)
        , normalMap(false)
        , preserveAlpha(false)
        , alphaThreshold(0.5f)
    {}
};

/**
 * @brief Compressed texture data
 */
struct CompressedTextureData {
    CompressionFormat format;
    int width;
    int height;
    int mipLevels;
    std::vector<std::vector<uint8_t>> mipData;  // Data for each mip level
    size_t totalSize;
    
    CompressedTextureData()
        : format(CompressionFormat::None)
        , width(0)
        , height(0)
        , mipLevels(0)
        , totalSize(0)
    {}
    
    bool isValid() const {
        return width > 0 && height > 0 && !mipData.empty();
    }
};

/**
 * @brief Texture compression utility
 * 
 * Provides compression and decompression of textures using various formats
 * optimized for different platforms and use cases.
 */
class TextureCompression {
public:
    /**
     * @brief Compress raw texture data
     * @param data Raw RGBA8 texture data
     * @param width Texture width
     * @param height Texture height
     * @param params Compression parameters
     * @return Compressed texture data
     */
    static CompressedTextureData compress(
        const uint8_t* data,
        int width,
        int height,
        const CompressionParams& params
    );
    
    /**
     * @brief Decompress texture data back to RGBA8
     * @param compressed Compressed texture data
     * @param mipLevel Mip level to decompress (0 = full resolution)
     * @return Raw RGBA8 texture data
     */
    static std::vector<uint8_t> decompress(
        const CompressedTextureData& compressed,
        int mipLevel = 0
    );
    
    /**
     * @brief Check if a compression format is supported
     * @param format Compression format to check
     * @return True if format is supported on current platform
     */
    static bool isFormatSupported(CompressionFormat format);
    
    /**
     * @brief Get recommended format for platform
     * @param hasAlpha Whether texture has alpha channel
     * @param isNormalMap Whether texture is a normal map
     * @return Recommended compression format
     */
    static CompressionFormat getRecommendedFormat(bool hasAlpha, bool isNormalMap);
    
    /**
     * @brief Calculate compressed size for given parameters
     * @param width Texture width
     * @param height Texture height
     * @param format Compression format
     * @param mipLevels Number of mip levels
     * @return Compressed data size in bytes
     */
    static size_t calculateCompressedSize(
        int width,
        int height,
        CompressionFormat format,
        int mipLevels = 1
    );
    
    /**
     * @brief Get block size for compression format
     * @param format Compression format
     * @return Block size in pixels (e.g., 4 for BC formats, variable for ASTC)
     */
    static void getBlockSize(CompressionFormat format, int& blockWidth, int& blockHeight);
    
    /**
     * @brief Get bytes per block for compression format
     */
    static int getBytesPerBlock(CompressionFormat format);
    
    /**
     * @brief Get format name as string
     */
    static std::string getFormatName(CompressionFormat format);
    
    /**
     * @brief Generate mipmaps for compressed texture
     */
    static std::vector<std::vector<uint8_t>> generateMipmaps(
        const uint8_t* data,
        int width,
        int height,
        const CompressionParams& params
    );
    
private:
    // Format-specific compression functions
    static std::vector<uint8_t> compressBC1(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> compressBC3(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> compressBC4(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> compressBC5(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> compressBC7(const uint8_t* data, int width, int height, CompressionQuality quality);
    static std::vector<uint8_t> compressETC2(const uint8_t* data, int width, int height, bool hasAlpha);
    static std::vector<uint8_t> compressASTC(const uint8_t* data, int width, int height, 
                                            int blockWidth, int blockHeight, CompressionQuality quality);
    
    // Format-specific decompression functions
    static std::vector<uint8_t> decompressBC1(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> decompressBC3(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> decompressBC5(const uint8_t* data, int width, int height);
    static std::vector<uint8_t> decompressETC2(const uint8_t* data, int width, int height, bool hasAlpha);
    static std::vector<uint8_t> decompressASTC(const uint8_t* data, int width, int height,
                                              int blockWidth, int blockHeight);
};

} // namespace Graphics
} // namespace JJM

#endif // TEXTURE_COMPRESSION_H
