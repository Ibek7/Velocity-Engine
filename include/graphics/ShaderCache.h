#ifndef SHADER_CACHE_H
#define SHADER_CACHE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <chrono>
#include <memory>

namespace JJM {
namespace Graphics {

/**
 * @brief Shader binary cache entry
 */
struct ShaderCacheEntry {
    std::vector<uint8_t> binary;
    uint64_t sourceHash;
    std::chrono::system_clock::time_point timestamp;
    uint32_t format;  // GL_SHADER_BINARY_FORMAT
    std::string driverVersion;
    
    bool isValid() const {
        return !binary.empty() && format != 0;
    }
};

/**
 * @brief Persistent shader compilation cache
 * 
 * Caches compiled shader binaries to disk to avoid recompilation
 * on subsequent runs. Significantly improves startup time.
 */
class ShaderCache {
private:
    std::filesystem::path m_cacheDirectory;
    std::unordered_map<std::string, ShaderCacheEntry> m_entries;
    bool m_enabled;
    size_t m_totalCacheSize;
    size_t m_maxCacheSize;
    
    // Statistics
    uint32_t m_cacheHits;
    uint32_t m_cacheMisses;
    
public:
    ShaderCache();
    ~ShaderCache();
    
    /**
     * @brief Initialize the cache system
     * @param cacheDir Directory to store cached shaders
     * @param maxSizeMB Maximum cache size in megabytes
     */
    void initialize(const std::filesystem::path& cacheDir, size_t maxSizeMB = 256);
    
    /**
     * @brief Shutdown and flush cache to disk
     */
    void shutdown();
    
    /**
     * @brief Check if a compiled shader exists in cache
     * @param shaderName Unique shader identifier
     * @param sourceHash Hash of shader source code
     * @return True if valid cache entry exists
     */
    bool hasEntry(const std::string& shaderName, uint64_t sourceHash) const;
    
    /**
     * @brief Retrieve cached shader binary
     * @param shaderName Unique shader identifier
     * @param sourceHash Hash of shader source code
     * @return Cached shader entry, or nullptr if not found
     */
    const ShaderCacheEntry* getEntry(const std::string& shaderName, uint64_t sourceHash) const;
    
    /**
     * @brief Store compiled shader in cache
     * @param shaderName Unique shader identifier
     * @param sourceHash Hash of shader source code
     * @param binary Compiled shader binary
     * @param format Shader binary format
     */
    void addEntry(const std::string& shaderName, uint64_t sourceHash,
                  const std::vector<uint8_t>& binary, uint32_t format);
    
    /**
     * @brief Remove a shader from cache
     * @param shaderName Unique shader identifier
     */
    void removeEntry(const std::string& shaderName);
    
    /**
     * @brief Clear all cached shaders
     */
    void clear();
    
    /**
     * @brief Load cache from disk
     */
    bool load();
    
    /**
     * @brief Save cache to disk
     */
    bool save();
    
    /**
     * @brief Get cache statistics
     */
    struct Stats {
        uint32_t hits;
        uint32_t misses;
        size_t totalSize;
        size_t entryCount;
        float hitRate;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Enable or disable caching
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Compute hash of shader source
     */
    static uint64_t computeHash(const std::string& source);
    
    /**
     * @brief Clean up old or invalid cache entries
     */
    void cleanup();
    
private:
    std::string getCacheFilePath(const std::string& shaderName) const;
    bool validateCacheEntry(const ShaderCacheEntry& entry) const;
    void evictOldestEntries(size_t targetSize);
};

} // namespace Graphics
} // namespace JJM

#endif // SHADER_CACHE_H
