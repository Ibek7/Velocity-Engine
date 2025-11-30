#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <functional>
#include <filesystem>
#include <chrono>
#include <future>
#include <any>

namespace JJM {
namespace Streaming {

// Forward declarations
class Asset;
class AssetLoader;
class AssetHandle;
class StreamingManager;

// Asset priority levels
enum class AssetPriority {
    Critical = 0,    // Must be loaded immediately (player assets, UI)
    High = 1,        // Important for gameplay (nearby objects)
    Medium = 2,      // Background loading (distant objects)
    Low = 3,         // Preemptive loading (cache warming)
    Background = 4   // Lowest priority (prefetching)
};

// Asset loading state
enum class AssetState {
    Unloaded,       // Not loaded, not in queue
    Queued,         // In loading queue, waiting
    Loading,        // Currently being loaded
    Loaded,         // Successfully loaded and available
    Failed,         // Loading failed, error state
    Unloading,      // Currently being unloaded
    Expired         // Marked for removal/eviction
};

// Level of Detail (LOD) specification
struct AssetLOD {
    uint32_t level;           // 0 = highest quality, higher = lower quality
    float distance;           // Distance threshold for this LOD
    float memoryBudget;       // Memory budget factor (1.0 = full size)
    std::string suffix;       // File suffix for LOD variants ("_lod1", "_low", etc.)
    
    AssetLOD(uint32_t lvl = 0, float dist = 0.0f, float budget = 1.0f, const std::string& suf = "")
        : level(lvl), distance(dist), memoryBudget(budget), suffix(suf) {}
    
    bool operator<(const AssetLOD& other) const { return level < other.level; }
    bool operator==(const AssetLOD& other) const { return level == other.level; }
};

// Asset metadata and loading information
struct AssetMetadata {
    std::string id;                          // Unique asset identifier
    std::string path;                        // File system path
    std::string type;                        // Asset type (texture, mesh, audio, etc.)
    size_t estimatedSize;                    // Estimated memory usage in bytes
    AssetPriority priority;                  // Loading priority
    std::vector<AssetLOD> lodLevels;         // Available LOD levels
    AssetLOD currentLOD;                     // Currently loaded LOD
    std::unordered_map<std::string, std::any> customData;  // Custom metadata
    
    AssetMetadata()
        : estimatedSize(0), priority(AssetPriority::Medium), currentLOD() {}
};

// Asset loading request
struct LoadRequest {
    std::string assetId;
    AssetPriority priority;
    AssetLOD requestedLOD;
    std::function<void(std::shared_ptr<Asset>)> onComplete;
    std::function<void(const std::string&)> onError;
    std::chrono::steady_clock::time_point timestamp;
    float progressWeight;                    // For progress calculation
    
    LoadRequest()
        : priority(AssetPriority::Medium), progressWeight(1.0f) {
        timestamp = std::chrono::steady_clock::now();
    }
    
    bool operator<(const LoadRequest& other) const {
        // Higher priority first, then older requests first
        if (priority != other.priority) {
            return priority > other.priority;  // Note: reversed for priority queue
        }
        return timestamp > other.timestamp;
    }
};

// Base asset class
class Asset {
public:
    virtual ~Asset() = default;
    
    // Asset information
    const std::string& getId() const { return metadata.id; }
    const std::string& getType() const { return metadata.type; }
    size_t getMemoryUsage() const { return memoryUsage; }
    AssetState getState() const { return state; }
    const AssetLOD& getCurrentLOD() const { return metadata.currentLOD; }
    
    // Asset lifecycle
    virtual bool load(const std::filesystem::path& path, const AssetLOD& lod) = 0;
    virtual void unload() = 0;
    virtual bool reload(const AssetLOD& newLOD) { return false; }  // Optional
    
    // Memory management
    virtual size_t calculateMemoryUsage() const = 0;
    virtual bool canUnload() const { return referenceCount == 0; }
    
    // Reference counting for automatic unloading
    void addReference() { ++referenceCount; }
    void removeReference() { --referenceCount; }
    uint32_t getReferenceCount() const { return referenceCount; }
    
    // Access tracking for LRU eviction
    void markAccessed() { lastAccessTime = std::chrono::steady_clock::now(); }
    std::chrono::steady_clock::time_point getLastAccessTime() const { return lastAccessTime; }
    
    // State management (public for manager access)
    void setState(AssetState newState) { state = newState; }
    void setMemoryUsage(size_t usage) { memoryUsage = usage; }
    
    // Metadata access
    const AssetMetadata& getMetadata() const { return metadata; }
    void setMetadata(const AssetMetadata& meta) { metadata = meta; }
    
protected:
    AssetMetadata metadata;
    std::atomic<AssetState> state{AssetState::Unloaded};
    std::atomic<uint32_t> referenceCount{0};
    std::atomic<size_t> memoryUsage{0};
    std::chrono::steady_clock::time_point lastAccessTime;
    mutable std::mutex assetMutex;
};

// Asset handle for safe asset access
class AssetHandle {
public:
    AssetHandle() = default;
    explicit AssetHandle(std::shared_ptr<Asset> asset);
    ~AssetHandle();
    
    // Copy and move semantics
    AssetHandle(const AssetHandle& other);
    AssetHandle& operator=(const AssetHandle& other);
    AssetHandle(AssetHandle&& other) noexcept;
    AssetHandle& operator=(AssetHandle&& other) noexcept;
    
    // Asset access
    std::shared_ptr<Asset> get() const;
    Asset* operator->() const { return get().get(); }
    Asset& operator*() const { return *get(); }
    
    // Validity checking
    bool isValid() const;
    bool isLoaded() const;
    AssetState getState() const;
    
    // Async operations
    void loadAsync(AssetPriority priority = AssetPriority::Medium);
    std::future<bool> loadAsyncWithFuture(AssetPriority priority = AssetPriority::Medium);
    
    // LOD management
    bool requestLOD(const AssetLOD& lod);
    AssetLOD getCurrentLOD() const;
    
private:
    std::weak_ptr<Asset> asset;
    mutable std::mutex handleMutex;
};

// Asset loader interface
class IAssetLoader {
public:
    virtual ~IAssetLoader() = default;
    
    // Loader capabilities
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
    virtual std::vector<std::string> getSupportedTypes() const = 0;
    virtual bool canLoad(const std::filesystem::path& path) const = 0;
    
    // Asset creation and loading
    virtual std::shared_ptr<Asset> createAsset(const AssetMetadata& metadata) = 0;
    virtual bool loadAsset(std::shared_ptr<Asset> asset, const std::filesystem::path& path, const AssetLOD& lod) = 0;
    virtual AssetMetadata extractMetadata(const std::filesystem::path& path) = 0;
    
    // Performance optimization
    virtual bool supportsStreaming() const { return false; }
    virtual bool supportsLOD() const { return false; }
    virtual size_t estimateLoadTime(const AssetMetadata& metadata) const { return 100; }  // ms
};

// Memory budget management
class MemoryBudget {
public:
    MemoryBudget(size_t totalBudget = 256 * 1024 * 1024);  // 256MB default
    ~MemoryBudget() = default;
    
    // Budget management
    void setTotalBudget(size_t budget);
    size_t getTotalBudget() const { return totalBudget; }
    size_t getUsedMemory() const { return usedMemory; }
    size_t getAvailableMemory() const;
    float getUsageRatio() const;
    
    // Memory allocation
    bool canAllocate(size_t size) const;
    bool allocate(size_t size);
    void deallocate(size_t size);
    
    // Category-based budgets
    void setCategoryBudget(const std::string& category, size_t budget);
    size_t getCategoryBudget(const std::string& category) const;
    size_t getCategoryUsed(const std::string& category) const;
    bool allocateFromCategory(const std::string& category, size_t size);
    void deallocateFromCategory(const std::string& category, size_t size);
    
    // Statistics
    struct Statistics {
        size_t peakUsage;
        size_t totalAllocations;
        size_t totalDeallocations;
        std::chrono::milliseconds totalAllocationTime;
        size_t failedAllocations;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
private:
    std::atomic<size_t> totalBudget;
    std::atomic<size_t> usedMemory;
    std::unordered_map<std::string, size_t> categoryBudgets;
    std::unordered_map<std::string, size_t> categoryUsage;
    mutable std::mutex budgetMutex;
    
    mutable Statistics stats;
    mutable std::mutex statsMutex;
};

// Asset eviction policy interface
class IEvictionPolicy {
public:
    virtual ~IEvictionPolicy() = default;
    
    virtual std::vector<std::string> selectAssetsForEviction(
        const std::unordered_map<std::string, std::shared_ptr<Asset>>& assets,
        size_t targetMemory) = 0;
    
    virtual void onAssetAccessed(const std::string& assetId) {}
    virtual void onAssetLoaded(const std::string& assetId) {}
    virtual void onAssetUnloaded(const std::string& assetId) {}
};

// LRU (Least Recently Used) eviction policy
class LRUEvictionPolicy : public IEvictionPolicy {
public:
    LRUEvictionPolicy() = default;
    ~LRUEvictionPolicy() = default;
    
    std::vector<std::string> selectAssetsForEviction(
        const std::unordered_map<std::string, std::shared_ptr<Asset>>& assets,
        size_t targetMemory) override;
    
    void onAssetAccessed(const std::string& assetId) override;
    void onAssetLoaded(const std::string& assetId) override;
    void onAssetUnloaded(const std::string& assetId) override;
    
private:
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> accessTimes;
    mutable std::mutex accessMutex;
};

// Smart cache for frequently accessed assets
class AssetCache {
public:
    AssetCache(size_t maxSize = 100);
    ~AssetCache() = default;
    
    // Cache operations
    void put(const std::string& key, std::shared_ptr<Asset> asset);
    std::shared_ptr<Asset> get(const std::string& key);
    bool contains(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    // Cache management
    void setMaxSize(size_t maxSize);
    size_t getMaxSize() const { return maxSize; }
    size_t getCurrentSize() const;
    
    // Statistics
    struct CacheStats {
        size_t hits;
        size_t misses;
        size_t evictions;
        float hitRate;
    };
    
    CacheStats getStatistics() const;
    void resetStatistics();
    
private:
    struct CacheEntry {
        std::shared_ptr<Asset> asset;
        std::chrono::steady_clock::time_point accessTime;
        size_t accessCount;
    };
    
    std::unordered_map<std::string, CacheEntry> cache;
    size_t maxSize;
    mutable std::mutex cacheMutex;
    
    mutable CacheStats stats;
    void evictOldestEntries(size_t count = 1);
};

// Background loading worker
class LoadingWorker {
public:
    LoadingWorker(StreamingManager* manager, uint32_t workerId);
    ~LoadingWorker();
    
    // Worker lifecycle
    void start();
    void stop();
    bool isRunning() const { return running; }
    
    // Work processing
    void processRequests();
    bool hasWork() const;
    
    // Statistics
    struct WorkerStats {
        size_t requestsProcessed;
        size_t requestsFailed;
        std::chrono::milliseconds totalProcessingTime;
        std::chrono::steady_clock::time_point lastActivity;
    };
    
    WorkerStats getStatistics() const;
    void resetStatistics();
    
private:
    StreamingManager* manager;
    uint32_t workerId;
    std::atomic<bool> running{false};
    std::thread workerThread;
    
    mutable WorkerStats stats;
    mutable std::mutex statsMutex;
    
    void workerLoop();
    bool processNextRequest();
};

// Main streaming manager
class StreamingManager {
public:
    StreamingManager();
    ~StreamingManager();
    
    // System lifecycle
    bool initialize(size_t workerCount = 4, size_t memoryBudget = 256 * 1024 * 1024);
    void shutdown();
    bool isInitialized() const { return initialized; }
    
    // Asset registration and management
    void registerLoader(std::shared_ptr<IAssetLoader> loader);
    void unregisterLoader(std::shared_ptr<IAssetLoader> loader);
    
    bool registerAsset(const AssetMetadata& metadata);
    void unregisterAsset(const std::string& assetId);
    bool isAssetRegistered(const std::string& assetId) const;
    
    // Asset loading
    AssetHandle loadAsset(const std::string& assetId, AssetPriority priority = AssetPriority::Medium);
    AssetHandle loadAssetAsync(const std::string& assetId, AssetPriority priority = AssetPriority::Medium);
    std::future<AssetHandle> loadAssetWithFuture(const std::string& assetId, AssetPriority priority = AssetPriority::Medium);
    
    // Batch operations
    std::vector<AssetHandle> loadAssets(const std::vector<std::string>& assetIds, AssetPriority priority = AssetPriority::Medium);
    void preloadAssets(const std::vector<std::string>& assetIds);
    void unloadAssets(const std::vector<std::string>& assetIds);
    
    // Asset access
    AssetHandle getAsset(const std::string& assetId);
    std::shared_ptr<Asset> getAssetDirect(const std::string& assetId);
    bool isAssetLoaded(const std::string& assetId) const;
    AssetState getAssetState(const std::string& assetId) const;
    
    // LOD management
    bool requestAssetLOD(const std::string& assetId, const AssetLOD& lod);
    AssetLOD getCurrentAssetLOD(const std::string& assetId) const;
    void setGlobalLODBias(float bias) { globalLODBias = bias; }
    float getGlobalLODBias() const { return globalLODBias; }
    
    // Memory management
    MemoryBudget& getMemoryBudget() { return memoryBudget; }
    const MemoryBudget& getMemoryBudget() const { return memoryBudget; }
    void setEvictionPolicy(std::unique_ptr<IEvictionPolicy> policy);
    void triggerEviction(size_t targetMemory = 0);
    
    // Request queue management
    void addLoadRequest(const LoadRequest& request);
    LoadRequest getNextRequest();
    bool hasQueuedRequests() const;
    size_t getQueueSize() const;
    void clearQueue();
    
    // Update and maintenance
    void update(float deltaTime);
    void garbageCollect();
    void validateAssets();
    
    // Configuration
    void setMaxConcurrentLoads(size_t maxLoads);
    size_t getMaxConcurrentLoads() const { return maxConcurrentLoads; }
    
    void setLoadTimeout(std::chrono::milliseconds timeout) { loadTimeout = timeout; }
    std::chrono::milliseconds getLoadTimeout() const { return loadTimeout; }
    
    // Statistics and diagnostics
    struct StreamingStats {
        size_t totalAssetsRegistered;
        size_t assetsLoaded;
        size_t assetsLoading;
        size_t assetsFailed;
        size_t totalLoadRequests;
        size_t queueSize;
        size_t memoryUsed;
        size_t memoryBudget;
        float averageLoadTime;
        std::chrono::milliseconds totalLoadTime;
        
        // Performance metrics
        float loadThroughput;        // Assets per second
        float memoryThroughput;      // Bytes per second
        size_t cacheHitRate;
        size_t evictionCount;
        
        // Worker statistics
        size_t activeWorkers;
        size_t totalRequestsProcessed;
        float workerEfficiency;
    };
    
    StreamingStats getStatistics() const;
    void resetStatistics();
    
    // Debug and profiling
    std::vector<std::string> getLoadedAssetIds() const;
    std::vector<std::string> getQueuedAssetIds() const;
    void dumpStatistics() const;
    void enableProfiling(bool enable) { profilingEnabled = enable; }
    bool isProfilingEnabled() const { return profilingEnabled; }
    
private:
    // Core state
    std::atomic<bool> initialized{false};
    std::atomic<bool> shutdownRequested{false};
    
    // Asset management
    std::unordered_map<std::string, AssetMetadata> assetRegistry;
    std::unordered_map<std::string, std::shared_ptr<Asset>> loadedAssets;
    std::unordered_map<std::string, std::weak_ptr<Asset>> assetHandles;
    mutable std::mutex assetMutex;
    
    // Loading infrastructure
    std::vector<std::shared_ptr<IAssetLoader>> loaders;
    std::vector<std::unique_ptr<LoadingWorker>> workers;
    std::priority_queue<LoadRequest> loadQueue;
    std::atomic<size_t> activeLoads{0};
    std::atomic<size_t> maxConcurrentLoads{4};
    std::chrono::milliseconds loadTimeout{30000};  // 30 seconds
    mutable std::mutex queueMutex;
    std::condition_variable queueCondition;
    
    // Memory management
    MemoryBudget memoryBudget;
    std::unique_ptr<IEvictionPolicy> evictionPolicy;
    AssetCache assetCache;
    std::atomic<float> globalLODBias{0.0f};
    
    // Statistics
    mutable StreamingStats stats;
    mutable std::mutex statsMutex;
    std::atomic<bool> profilingEnabled{false};
    
    // Internal methods
    IAssetLoader* findCompatibleLoader(const AssetMetadata& metadata);
    bool loadAssetInternal(const std::string& assetId, const AssetLOD& lod);
    void unloadAssetInternal(const std::string& assetId);
    bool validateAssetMetadata(const AssetMetadata& metadata) const;
    
    // Worker management
    void startWorkers();
    void stopWorkers();
    LoadingWorker* getAvailableWorker();
    
    // Cache and eviction
    void updateCache();
    void performEviction();
    
    // Friend classes for internal access
    friend class LoadingWorker;
    friend class AssetHandle;
};

// Utility functions for asset management
namespace AssetUtils {
    // Path utilities
    std::string generateAssetId(const std::filesystem::path& path);
    std::string getAssetTypeFromPath(const std::filesystem::path& path);
    std::filesystem::path getLODPath(const std::filesystem::path& basePath, const AssetLOD& lod);
    
    // Size estimation
    size_t estimateTextureSize(uint32_t width, uint32_t height, uint32_t channels, uint32_t bytesPerChannel = 1);
    size_t estimateMeshSize(uint32_t vertexCount, uint32_t indexCount, bool hasNormals = true, bool hasUVs = true);
    size_t estimateAudioSize(uint32_t sampleRate, uint32_t channels, float durationSeconds, uint32_t bitsPerSample = 16);
    
    // LOD utilities
    AssetLOD calculateLODFromDistance(float distance, const std::vector<AssetLOD>& lodLevels);
    float calculateOptimalLODBias(float memoryPressure, float performanceTarget);
    
    // Metadata helpers
    AssetMetadata createTextureMetadata(const std::string& id, const std::filesystem::path& path, uint32_t width, uint32_t height);
    AssetMetadata createMeshMetadata(const std::string& id, const std::filesystem::path& path, uint32_t vertexCount);
    AssetMetadata createAudioMetadata(const std::string& id, const std::filesystem::path& path, float duration);
}

} // namespace Streaming
} // namespace JJM