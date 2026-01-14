#ifndef TEXTURE_STREAMING_H
#define TEXTURE_STREAMING_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>

namespace JJM {
namespace Graphics {

/**
 * @brief Represents a mipmap level in texture streaming
 */
struct MipLevel {
    uint32_t width;
    uint32_t height;
    uint32_t dataSize;
    std::vector<uint8_t> data;
    bool loaded;
    
    MipLevel() : width(0), height(0), dataSize(0), loaded(false) {}
};

/**
 * @brief Priority levels for texture streaming
 */
enum class StreamPriority {
    Critical = 0,   // Currently visible, high detail
    High = 1,       // Currently visible, medium detail
    Medium = 2,     // Recently visible or nearby
    Low = 3,        // Background or distant
    Deferred = 4    // Not immediately needed
};

/**
 * @brief Streaming state of a texture
 */
enum class StreamState {
    Unloaded,       // Not loaded at all
    Loading,        // Currently being loaded
    PartiallyLoaded,// Some mip levels loaded
    FullyLoaded,    // All mip levels loaded
    Evicted         // Was loaded but removed from memory
};

/**
 * @brief Configuration for texture streaming system
 */
struct StreamingConfig {
    size_t maxMemoryBudget;      // Maximum memory in bytes
    size_t minMemoryThreshold;   // Minimum free memory before eviction
    uint32_t maxConcurrentLoads; // Max simultaneous loads
    uint32_t framesBeforeEviction; // Frames to wait before evicting
    bool enablePredictiveLoading; // Preload based on movement
    float lodBias;               // Bias for LOD selection
    
    StreamingConfig() 
        : maxMemoryBudget(512 * 1024 * 1024) // 512 MB
        , minMemoryThreshold(64 * 1024 * 1024) // 64 MB
        , maxConcurrentLoads(4)
        , framesBeforeEviction(300) // ~5 seconds at 60fps
        , enablePredictiveLoading(true)
        , lodBias(0.0f) {}
};

/**
 * @brief Statistics for texture streaming
 */
struct StreamingStats {
    size_t currentMemoryUsage;
    size_t peakMemoryUsage;
    uint32_t texturesLoaded;
    uint32_t texturesEvicted;
    uint32_t activeStreams;
    uint32_t pendingStreams;
    float averageLoadTime;
    uint32_t cacheMisses;
    
    StreamingStats() 
        : currentMemoryUsage(0)
        , peakMemoryUsage(0)
        , texturesLoaded(0)
        , texturesEvicted(0)
        , activeStreams(0)
        , pendingStreams(0)
        , averageLoadTime(0.0f)
        , cacheMisses(0) {}
};

/**
 * @brief Request for loading texture data
 */
struct StreamRequest {
    uint32_t textureId;
    uint32_t mipLevel;
    StreamPriority priority;
    float distance;      // Distance from camera
    uint64_t frameRequested;
    
    bool operator<(const StreamRequest& other) const {
        if (priority != other.priority)
            return priority > other.priority; // Higher priority = smaller value
        return distance > other.distance; // Closer = higher priority
    }
};

/**
 * @brief Managed texture with streaming support
 */
class StreamedTexture {
public:
    StreamedTexture(uint32_t id, const std::string& path);
    ~StreamedTexture();
    
    uint32_t getId() const { return m_id; }
    const std::string& getPath() const { return m_path; }
    StreamState getState() const { return m_state; }
    uint32_t getLoadedMipLevel() const { return m_loadedMipLevel; }
    uint32_t getMaxMipLevels() const { return static_cast<uint32_t>(m_mipLevels.size()); }
    size_t getMemoryUsage() const { return m_memoryUsage; }
    
    void setState(StreamState state) { m_state = state; }
    void setLoadedMipLevel(uint32_t level) { m_loadedMipLevel = level; }
    void updateLastAccessFrame(uint64_t frame) { m_lastAccessFrame = frame; }
    uint64_t getLastAccessFrame() const { return m_lastAccessFrame; }
    
    bool loadMipLevel(uint32_t level);
    void unloadMipLevel(uint32_t level);
    void unloadAll();
    
    const MipLevel* getMipLevel(uint32_t level) const;
    MipLevel* getMipLevel(uint32_t level);
    
private:
    uint32_t m_id;
    std::string m_path;
    StreamState m_state;
    uint32_t m_loadedMipLevel;
    uint64_t m_lastAccessFrame;
    size_t m_memoryUsage;
    std::vector<MipLevel> m_mipLevels;
    std::mutex m_mutex;
};

/**
 * @brief Main texture streaming system
 * Manages texture loading, eviction, and memory budgets
 */
class TextureStreamingSystem {
public:
    TextureStreamingSystem();
    ~TextureStreamingSystem();
    
    // Configuration
    void configure(const StreamingConfig& config);
    const StreamingConfig& getConfig() const { return m_config; }
    
    // Texture management
    uint32_t registerTexture(const std::string& path);
    void unregisterTexture(uint32_t textureId);
    
    // Streaming control
    void requestTexture(uint32_t textureId, StreamPriority priority, float distance = 0.0f);
    void update(float deltaTime, const float* cameraPosition = nullptr);
    void flushPendingRequests();
    
    // Memory management
    size_t getCurrentMemoryUsage() const { return m_stats.currentMemoryUsage; }
    size_t getMemoryBudget() const { return m_config.maxMemoryBudget; }
    bool isMemoryAvailable(size_t required) const;
    void evictLeastRecentlyUsed(size_t targetBytes);
    
    // Query
    StreamedTexture* getTexture(uint32_t textureId);
    const StreamedTexture* getTexture(uint32_t textureId) const;
    StreamState getTextureState(uint32_t textureId) const;
    uint32_t getOptimalMipLevel(uint32_t textureId, float distance) const;
    
    // Statistics
    const StreamingStats& getStats() const { return m_stats; }
    void resetStats();
    
    // Debug
    void dumpStatus() const;
    std::vector<uint32_t> getLoadedTextures() const;
    
private:
    void processStreamQueue();
    void evictIfNeeded();
    void updatePredictiveLoading(const float* cameraPosition);
    uint32_t calculateMipLevelFromDistance(float distance, uint32_t maxMips) const;
    
    StreamingConfig m_config;
    StreamingStats m_stats;
    
    std::unordered_map<uint32_t, std::unique_ptr<StreamedTexture>> m_textures;
    std::priority_queue<StreamRequest> m_streamQueue;
    
    uint32_t m_nextTextureId;
    uint64_t m_currentFrame;
    std::atomic<bool> m_isProcessing;
    
    mutable std::mutex m_mutex;
    mutable std::mutex m_queueMutex;
};

} // namespace Graphics
} // namespace JJM

#endif // TEXTURE_STREAMING_H
