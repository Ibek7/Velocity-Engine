#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "graphics/Texture.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace JJM {
namespace Core {

// Resource loading priority
enum class LoadPriority {
    Low,
    Normal,
    High,
    Critical
};

// Resource load request
struct LoadRequest {
    std::string id;
    std::string filePath;
    LoadPriority priority;
    std::function<void(bool)> callback;
    std::function<void(float)> progressCallback;  // Progress 0.0-1.0
    size_t estimatedSize;  // Estimated file size for progress tracking
    
    LoadRequest() : priority(LoadPriority::Normal), estimatedSize(0) {}
    
    bool operator<(const LoadRequest& other) const {
        return static_cast<int>(priority) < static_cast<int>(other.priority);
    }
};

// Resource loading progress tracker
struct LoadProgress {
    size_t totalItems;
    size_t loadedItems;
    size_t failedItems;
    size_t totalBytes;
    size_t loadedBytes;
    float percentage;
    std::string currentItem;
    bool isComplete;
    
    LoadProgress()
        : totalItems(0), loadedItems(0), failedItems(0)
        , totalBytes(0), loadedBytes(0), percentage(0.0f)
        , isComplete(false)
    {}
    
    void update() {
        if (totalItems > 0) {
            percentage = static_cast<float>(loadedItems) / totalItems;
        }
        isComplete = (loadedItems + failedItems) >= totalItems;
    }
};

// Resource statistics
struct ResourceStats {
    size_t totalMemoryUsed;
    size_t textureCount;
    size_t pendingLoads;
    size_t failedLoads;
    float avgLoadTime;
};

class ResourceManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Graphics::Texture>> textures;
    Graphics::Renderer* renderer;
    
    static ResourceManager* instance;
    
    // Thread pool for async loading
    std::vector<std::thread> workerThreads;
    std::priority_queue<LoadRequest> loadQueue;
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::atomic<bool> shutdownRequested;
    std::atomic<size_t> pendingLoads;
    
    // LRU cache for resource management
    struct CacheEntry {
        std::string id;
        std::shared_ptr<Graphics::Texture> resource;
        size_t memorySize;
        size_t accessCount;
        std::chrono::steady_clock::time_point lastAccessTime;
        
        CacheEntry() : memorySize(0), accessCount(0) {}
    };
    
    std::unordered_map<std::string, CacheEntry> cache;
    std::list<std::string> lruList;  // Most recently used at front
    std::mutex cacheMutex;
    
    // Statistics tracking
    std::atomic<size_t> totalMemoryUsed;
    std::atomic<size_t> failedLoads;
    std::atomic<float> totalLoadTime;
    std::atomic<size_t> loadCount;
    std::atomic<size_t> cacheHits;
    std::atomic<size_t> cacheMisses;
    std::atomic<size_t> evictions;
    
    // Resource limits
    size_t maxMemoryLimit;
    size_t maxCacheSize;
    
    // Progress tracking
    mutable std::mutex m_progressMutex;
    LoadProgress m_currentProgress;
    
    ResourceManager();

public:
    ~ResourceManager();
    
    static ResourceManager* getInstance();
    static void destroyInstance();
    
    void setRenderer(Graphics::Renderer* r) { renderer = r; }
    
    // Thread pool management
    void initializeThreadPool(size_t threadCount = 4);
    void shutdownThreadPool();
    
    // Texture management
    std::shared_ptr<Graphics::Texture> loadTexture(const std::string& id, const std::string& filePath);
    std::shared_ptr<Graphics::Texture> getTexture(const std::string& id);
    void unloadTexture(const std::string& id);
    void unloadAllTextures();
    
    // Async loading
    std::future<std::shared_ptr<Graphics::Texture>> loadTextureAsync(
        const std::string& id, 
        const std::string& filePath,
        LoadPriority priority = LoadPriority::Normal);
    void loadTextureWithCallback(
        const std::string& id,
        const std::string& filePath,
        std::function<void(std::shared_ptr<Graphics::Texture>)> callback,
        LoadPriority priority = LoadPriority::Normal);
    
    // Batch loading
    void loadBatch(const std::vector<std::pair<std::string, std::string>>& resources,
                   std::function<void(size_t loaded, size_t total)> progressCallback = nullptr);
    
    // Async batch loading with detailed progress
    void loadBatchAsync(const std::vector<std::pair<std::string, std::string>>& resources,
                       std::function<void(const LoadProgress&)> progressCallback = nullptr,
                       std::function<void(bool success)> completionCallback = nullptr);
    
    // Progress tracking
    LoadProgress getCurrentProgress() const;
    bool isLoadInProgress() const { return !m_currentProgress.isComplete; }
    
    // Resource queries
    bool hasTexture(const std::string& id) const;
    size_t getTextureCount() const { return textures.size(); }
    bool isLoading() const { return pendingLoads > 0; }
    size_t getPendingLoadCount() const { return pendingLoads; }
    
    // Memory management
    void setMemoryLimit(size_t bytes) { maxMemoryLimit = bytes; }
    size_t getMemoryLimit() const { return maxMemoryLimit; }
    size_t getMemoryUsed() const { return totalMemoryUsed; }
    void evictLRU(size_t bytesToFree);
    
    // LRU cache management
    void setCacheSize(size_t maxEntries) { maxCacheSize = maxEntries; }
    size_t getCacheSize() const { return cache.size(); }
    void clearCache();
    void touchResource(const std::string& id);  // Mark as recently used
    float getCacheHitRate() const {
        size_t hits = cacheHits.load();
        size_t misses = cacheMisses.load();
        if (hits + misses == 0) return 0.0f;
        return static_cast<float>(hits) / (hits + misses);
    }
    
    // Statistics
    ResourceStats getStats() const;
    void resetStats();
    
    // Clear all resources
    void clear();
    
private:
    void workerThread();
    void processLoadRequest(const LoadRequest& request);
};

// =============================================================================
// ASSET BUNDLE SYSTEM
// =============================================================================

/**
 * @brief Bundle compression types
 */
enum class BundleCompression {
    None,
    LZ4,            // Fast compression
    ZSTD,           // Good balance
    LZMA,           // High compression ratio
    Custom
};

/**
 * @brief Asset entry in a bundle
 */
struct BundleAssetEntry {
    std::string assetId;
    std::string assetType;          // "Texture", "Audio", "Mesh", "Animation", etc.
    std::string originalPath;
    
    // Location in bundle
    size_t offset{0};
    size_t compressedSize{0};
    size_t uncompressedSize{0};
    
    // Metadata
    uint32_t checksum{0};
    uint32_t version{0};
    std::unordered_map<std::string, std::string> metadata;
    
    // Dependencies
    std::vector<std::string> dependencies;
};

/**
 * @brief Bundle header information
 */
struct BundleHeader {
    static constexpr uint32_t MAGIC = 0x4A4A4D42;  // "JJMB"
    static constexpr uint32_t CURRENT_VERSION = 1;
    
    uint32_t magic{MAGIC};
    uint32_t version{CURRENT_VERSION};
    uint32_t assetCount{0};
    uint32_t compression{0};
    
    // Checksums
    uint32_t headerChecksum{0};
    uint32_t contentChecksum{0};
    
    // Sizes
    size_t headerSize{0};
    size_t totalSize{0};
    size_t uncompressedSize{0};
    
    // Metadata
    std::string bundleName;
    std::string buildTime;
    std::string platform;
    std::string buildTag;
    
    bool isValid() const { return magic == MAGIC && version <= CURRENT_VERSION; }
};

/**
 * @brief Loaded asset bundle
 */
class AssetBundle {
public:
    AssetBundle();
    ~AssetBundle();
    
    // Loading
    bool loadFromFile(const std::string& filepath);
    bool loadFromMemory(const uint8_t* data, size_t size);
    void unload();
    bool isLoaded() const { return loaded; }
    
    // Asset access
    bool hasAsset(const std::string& assetId) const;
    std::vector<uint8_t> getAssetData(const std::string& assetId);
    const BundleAssetEntry* getAssetEntry(const std::string& assetId) const;
    std::vector<std::string> getAssetIds() const;
    std::vector<std::string> getAssetIdsByType(const std::string& type) const;
    
    // Async loading
    std::future<std::vector<uint8_t>> getAssetDataAsync(const std::string& assetId);
    
    // Bundle info
    const BundleHeader& getHeader() const { return header; }
    const std::string& getName() const { return header.bundleName; }
    const std::string& getPath() const { return bundlePath; }
    size_t getAssetCount() const { return assets.size(); }
    size_t getTotalSize() const { return header.totalSize; }
    
    // Dependencies
    std::vector<std::string> getDependencies(const std::string& assetId) const;
    std::vector<std::string> getAllDependencies() const;
    
    // Validation
    bool validateChecksum() const;
    bool validateAsset(const std::string& assetId) const;
    
private:
    BundleHeader header;
    std::unordered_map<std::string, BundleAssetEntry> assets;
    std::string bundlePath;
    bool loaded{false};
    
    // File mapping for efficient access
    uint8_t* mappedData{nullptr};
    size_t mappedSize{0};
    
    std::vector<uint8_t> decompressData(const BundleAssetEntry& entry);
};

/**
 * @brief Asset bundle builder for creating bundles
 */
class BundleBuilder {
public:
    BundleBuilder();
    ~BundleBuilder();
    
    // Configuration
    void setName(const std::string& name) { bundleName = name; }
    void setPlatform(const std::string& platform) { targetPlatform = platform; }
    void setCompression(BundleCompression compression, int level = -1);
    void setBuildTag(const std::string& tag) { buildTag = tag; }
    
    // Adding assets
    void addAsset(const std::string& assetId, const std::string& filepath, const std::string& type);
    void addAsset(const std::string& assetId, const std::vector<uint8_t>& data, const std::string& type);
    void addAssetWithMetadata(const std::string& assetId, const std::string& filepath,
                               const std::string& type,
                               const std::unordered_map<std::string, std::string>& metadata);
    void removeAsset(const std::string& assetId);
    void clearAssets();
    
    // Dependencies
    void addDependency(const std::string& assetId, const std::string& dependencyId);
    void setDependencies(const std::string& assetId, const std::vector<std::string>& dependencies);
    
    // Building
    bool build(const std::string& outputPath);
    bool buildIncremental(const std::string& outputPath, const std::string& previousBundlePath);
    
    // Progress
    using ProgressCallback = std::function<void(const std::string& asset, int current, int total)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback = callback; }
    
    // Statistics
    struct BuildStats {
        size_t totalAssets;
        size_t totalUncompressedSize;
        size_t totalCompressedSize;
        float compressionRatio;
        float buildTimeSeconds;
    };
    BuildStats getLastBuildStats() const { return lastStats; }
    
private:
    struct PendingAsset {
        std::string assetId;
        std::string filepath;
        std::string type;
        std::vector<uint8_t> data;
        std::unordered_map<std::string, std::string> metadata;
        std::vector<std::string> dependencies;
        bool hasFileData{false};
    };
    
    std::string bundleName;
    std::string targetPlatform;
    std::string buildTag;
    BundleCompression compression{BundleCompression::LZ4};
    int compressionLevel{-1};
    
    std::vector<PendingAsset> pendingAssets;
    ProgressCallback progressCallback;
    BuildStats lastStats{};
    
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
    uint32_t calculateChecksum(const uint8_t* data, size_t size);
};

/**
 * @brief Bundle manager for handling multiple bundles
 */
class BundleManager {
public:
    static BundleManager* getInstance();
    static void destroyInstance();
    
    // Bundle loading
    bool loadBundle(const std::string& bundlePath);
    bool loadBundleAsync(const std::string& bundlePath, std::function<void(bool)> callback);
    void unloadBundle(const std::string& bundleName);
    void unloadAllBundles();
    
    // Bundle queries
    bool isBundleLoaded(const std::string& bundleName) const;
    AssetBundle* getBundle(const std::string& bundleName);
    std::vector<std::string> getLoadedBundleNames() const;
    
    // Asset access (searches all loaded bundles)
    bool hasAsset(const std::string& assetId) const;
    std::vector<uint8_t> getAssetData(const std::string& assetId);
    std::string findAssetBundle(const std::string& assetId) const;
    
    // Dependency resolution
    std::vector<std::string> resolveDependencies(const std::string& assetId) const;
    bool loadAssetWithDependencies(const std::string& assetId);
    
    // Bundle streaming
    void setStreamingEnabled(bool enabled) { streamingEnabled = enabled; }
    void setStreamingBudget(size_t bytes) { streamingBudget = bytes; }
    void prioritizeBundle(const std::string& bundleName);
    
    // Memory management
    size_t getTotalBundleMemory() const;
    void setMemoryBudget(size_t bytes) { memoryBudget = bytes; }
    void evictLeastUsed(size_t bytesToFree);
    
    // Statistics
    struct BundleStats {
        size_t loadedBundles;
        size_t totalAssets;
        size_t totalMemory;
        size_t cacheHits;
        size_t cacheMisses;
    };
    BundleStats getStatistics() const;
    
private:
    static BundleManager* instance;
    
    std::unordered_map<std::string, std::unique_ptr<AssetBundle>> bundles;
    std::unordered_map<std::string, std::string> assetToBundleMap;
    mutable std::mutex bundleMutex;
    
    bool streamingEnabled{false};
    size_t streamingBudget{256 * 1024 * 1024};  // 256MB
    size_t memoryBudget{512 * 1024 * 1024};     // 512MB
    
    mutable BundleStats stats{};
    
    BundleManager();
    ~BundleManager();
    
    void buildAssetIndex();
    void updateAssetIndex(const std::string& bundleName);
};

// =============================================================================
// BUNDLE PATCHING AND VERSIONING
// =============================================================================

/**
 * @brief Patch operation type
 */
enum class PatchOperation {
    Add,            // New asset
    Modify,         // Modified asset
    Remove,         // Removed asset
    Rename          // Renamed asset (includes old and new ID)
};

/**
 * @brief Single patch entry
 */
struct PatchEntry {
    PatchOperation operation;
    std::string assetId;
    std::string newAssetId;     // For rename operations
    std::string assetType;
    
    // For add/modify
    size_t offset{0};
    size_t compressedSize{0};
    size_t uncompressedSize{0};
    uint32_t checksum{0};
};

/**
 * @brief Bundle patch file
 */
struct BundlePatch {
    static constexpr uint32_t MAGIC = 0x4A4A4D50;  // "JJMP"
    
    uint32_t magic{MAGIC};
    uint32_t sourceVersion{0};
    uint32_t targetVersion{0};
    std::string bundleName;
    std::vector<PatchEntry> entries;
    size_t patchDataOffset{0};
    
    bool isValid() const { return magic == MAGIC; }
};

/**
 * @brief Bundle patcher for creating and applying patches
 */
class BundlePatcher {
public:
    BundlePatcher();
    
    // Create patch
    bool createPatch(const std::string& oldBundlePath,
                     const std::string& newBundlePath,
                     const std::string& patchOutputPath);
    
    // Apply patch
    bool applyPatch(const std::string& bundlePath,
                    const std::string& patchPath,
                    const std::string& outputPath);
    
    // Verify patch
    bool verifyPatch(const std::string& bundlePath, const std::string& patchPath) const;
    
    // Patch info
    BundlePatch loadPatchInfo(const std::string& patchPath) const;
    
    // Progress
    using ProgressCallback = std::function<void(const std::string& operation, float progress)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback = callback; }
    
private:
    ProgressCallback progressCallback;
    
    std::vector<PatchEntry> diffBundles(const AssetBundle& oldBundle, const AssetBundle& newBundle);
};

/**
 * @brief Bundle version manager
 */
class BundleVersionManager {
public:
    struct VersionInfo {
        uint32_t version;
        std::string bundleName;
        std::string downloadUrl;
        size_t size;
        uint32_t checksum;
        std::string releaseNotes;
    };
    
    // Version checking
    bool checkForUpdates(const std::string& manifestUrl);
    std::vector<VersionInfo> getAvailableUpdates() const;
    
    // Current versions
    void registerLocalVersion(const std::string& bundleName, uint32_t version);
    uint32_t getLocalVersion(const std::string& bundleName) const;
    
    // Download management
    bool downloadBundle(const VersionInfo& info, const std::string& outputPath,
                        std::function<void(size_t downloaded, size_t total)> progress = nullptr);
    bool downloadPatch(const std::string& bundleName, uint32_t fromVersion, uint32_t toVersion,
                       const std::string& outputPath);
    
    // Manifest handling
    void loadManifest(const std::string& filepath);
    void setManifestUrl(const std::string& url) { manifestUrl = url; }
    
private:
    std::string manifestUrl;
    std::unordered_map<std::string, uint32_t> localVersions;
    std::vector<VersionInfo> availableVersions;
};

// =============================================================================
// VIRTUAL FILE SYSTEM FOR BUNDLES
// =============================================================================

/**
 * @brief Virtual file system mount point
 */
struct VFSMountPoint {
    std::string mountPath;          // e.g., "/textures/"
    std::string bundleName;
    int priority{0};                // Higher priority mounts override lower
    bool writable{false};
};

/**
 * @brief Virtual file system for unified asset access
 */
class BundleVFS {
public:
    static BundleVFS* getInstance();
    static void destroyInstance();
    
    // Mount management
    void mount(const std::string& bundleName, const std::string& mountPath, int priority = 0);
    void unmount(const std::string& mountPath);
    void unmountBundle(const std::string& bundleName);
    
    // File access (uses virtual paths)
    bool exists(const std::string& virtualPath) const;
    std::vector<uint8_t> read(const std::string& virtualPath);
    size_t getSize(const std::string& virtualPath) const;
    
    // Directory operations
    std::vector<std::string> listDirectory(const std::string& virtualPath) const;
    bool isDirectory(const std::string& virtualPath) const;
    
    // Path resolution
    std::string resolveToBundle(const std::string& virtualPath) const;
    std::string resolveToAssetId(const std::string& virtualPath) const;
    
    // Search
    std::vector<std::string> findFiles(const std::string& pattern) const;
    std::vector<std::string> findFilesByType(const std::string& assetType) const;
    
private:
    static BundleVFS* instance;
    
    std::vector<VFSMountPoint> mountPoints;
    mutable std::mutex vfsMutex;
    
    BundleVFS();
    ~BundleVFS();
    
    const VFSMountPoint* findMountPoint(const std::string& virtualPath) const;
    void sortMountPoints();
};

} // namespace Core
} // namespace JJM

#endif // RESOURCE_MANAGER_H
