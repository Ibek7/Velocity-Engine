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
    
    // Priority-based streaming
    void updateAssetPriority(const std::string& assetId, AssetPriority newPriority);
    void setDistanceBasedPriority(const std::string& assetId, float distance, float criticalDistance = 50.0f);
    void setVisibilityBasedPriority(const std::string& assetId, bool visible, bool inFrustum);
    void recalculatePriorities();  // Recalculate all priorities based on game state
    
    // Distance and visibility tracking
    struct ViewerPosition {
        float x, y, z;
        float viewDistance;
        std::vector<std::string> frustumAssets;  // Assets in view frustum
    };
    
    void setViewerPosition(const ViewerPosition& position);
    ViewerPosition getViewerPosition() const { return m_viewerPosition; }
    
    // Async loading with progress tracking
    struct AsyncLoadGroup {
        std::string groupId;
        std::vector<std::string> assetIds;
        std::atomic<size_t> loadedCount{0};
        std::function<void(float)> onProgress;  // Progress 0.0-1.0
        std::function<void()> onComplete;
    };
    
    void loadAssetGroup(const AsyncLoadGroup& group);
    float getGroupProgress(const std::string& groupId) const;
    void cancelGroup(const std::string& groupId);
    
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
    
    // Priority and visibility tracking
    ViewerPosition m_viewerPosition;
    std::unordered_map<std::string, float> m_assetDistances;
    std::unordered_map<std::string, AsyncLoadGroup> m_loadGroups;
    mutable std::mutex m_priorityMutex;
    
    // Statistics
    mutable StreamingStats stats;
    mutable std::mutex statsMutex;
    std::atomic<bool> profilingEnabled{false};
    
    // Internal methods
    IAssetLoader* findCompatibleLoader(const AssetMetadata& metadata);
    bool loadAssetInternal(const std::string& assetId, const AssetLOD& lod);
    void unloadAssetInternal(const std::string& assetId);
    bool validateAssetMetadata(const AssetMetadata& metadata) const;
    AssetPriority calculatePriorityFromDistance(float distance, float criticalDistance) const;
    
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

// ============================================================================
// STREAMING REGIONS - Spatial-based asset streaming
// ============================================================================

struct StreamingRegion {
    std::string regionId;
    glm::vec3 center{0.0f};
    float radius{100.0f};
    std::vector<std::string> assetIds;
    AssetPriority defaultPriority{AssetPriority::Medium};
    bool isActive{false};
    float loadDistance{150.0f};     // Distance at which to start loading
    float unloadDistance{200.0f};   // Distance at which to unload
    
    bool contains(const glm::vec3& point) const {
        return glm::distance(center, point) <= radius;
    }
    
    float distanceTo(const glm::vec3& point) const {
        return glm::distance(center, point);
    }
};

class StreamingRegionManager {
public:
    StreamingRegionManager(StreamingManager& streaming) : streamingManager(streaming) {}
    
    // Region management
    void registerRegion(const StreamingRegion& region);
    void unregisterRegion(const std::string& regionId);
    StreamingRegion* getRegion(const std::string& regionId);
    const std::vector<StreamingRegion>& getRegions() const { return regions; }
    
    // Position-based streaming
    void updateViewerPosition(const glm::vec3& position);
    void updateViewerPositions(const std::vector<glm::vec3>& positions);  // Multiple viewers
    
    // Region state
    std::vector<std::string> getActiveRegions() const;
    std::vector<std::string> getLoadingRegions() const;
    std::vector<std::string> getPendingRegions() const;
    
    // Configuration
    void setHysteresisMargin(float margin) { hysteresisMargin = margin; }
    void setPredictionEnabled(bool enabled) { predictionEnabled = enabled; }
    void setVelocityPredictionTime(float seconds) { velocityPredictionTime = seconds; }
    
    // Velocity-based prediction
    void setViewerVelocity(const glm::vec3& velocity);
    std::vector<std::string> predictNextRegions(float lookAheadTime) const;
    
    // Debug
    struct RegionStats {
        size_t totalRegions;
        size_t activeRegions;
        size_t loadingRegions;
        size_t assetsInActiveRegions;
        size_t assetsLoaded;
        float memoryUsedByActiveRegions;
    };
    RegionStats getStatistics() const;

private:
    StreamingManager& streamingManager;
    std::vector<StreamingRegion> regions;
    glm::vec3 currentPosition{0.0f};
    glm::vec3 currentVelocity{0.0f};
    float hysteresisMargin{10.0f};
    bool predictionEnabled{true};
    float velocityPredictionTime{2.0f};
    
    void activateRegion(StreamingRegion& region);
    void deactivateRegion(StreamingRegion& region);
    bool shouldActivateRegion(const StreamingRegion& region, const glm::vec3& position) const;
    bool shouldDeactivateRegion(const StreamingRegion& region, const glm::vec3& position) const;
};

// ============================================================================
// PREFETCH SYSTEM - Predictive asset loading
// ============================================================================

enum class PrefetchStrategy {
    None,               // No prefetching
    Spatial,            // Based on proximity to streaming regions
    Sequential,         // Based on asset access patterns
    GraphBased,         // Based on scene graph traversal
    MLPredicted,        // Machine learning predictions
    Hybrid              // Combination of strategies
};

struct PrefetchHint {
    std::string assetId;
    float probability{1.0f};        // Likelihood of needing this asset
    float estimatedTimeToNeed{0.0f}; // Seconds until asset is needed
    AssetPriority suggestedPriority{AssetPriority::Low};
    std::string reason;             // Debug info about why prefetching
};

class PrefetchPredictor {
public:
    virtual ~PrefetchPredictor() = default;
    virtual std::vector<PrefetchHint> predict(const std::string& currentContext) = 0;
    virtual void recordAccess(const std::string& assetId, float timestamp) = 0;
    virtual void train() {}  // For ML-based predictors
};

class SequentialPrefetcher : public PrefetchPredictor {
public:
    std::vector<PrefetchHint> predict(const std::string& currentContext) override;
    void recordAccess(const std::string& assetId, float timestamp) override;
    
    // Configuration
    void setPatternWindowSize(size_t size) { patternWindowSize = size; }
    void setMinConfidence(float confidence) { minConfidence = confidence; }
    
private:
    size_t patternWindowSize{5};
    float minConfidence{0.3f};
    std::vector<std::pair<std::string, float>> accessHistory;
    std::unordered_map<std::string, std::vector<std::string>> transitionGraph;
};

class SpatialPrefetcher : public PrefetchPredictor {
public:
    SpatialPrefetcher(StreamingRegionManager& regionMgr) : regionManager(regionMgr) {}
    
    std::vector<PrefetchHint> predict(const std::string& currentContext) override;
    void recordAccess(const std::string& assetId, float timestamp) override;
    
    void setLookAheadDistance(float distance) { lookAheadDistance = distance; }
    void setMaxPrefetchCount(size_t count) { maxPrefetchCount = count; }
    
private:
    StreamingRegionManager& regionManager;
    float lookAheadDistance{50.0f};
    size_t maxPrefetchCount{10};
};

class PrefetchManager {
public:
    PrefetchManager(StreamingManager& streaming) : streamingManager(streaming) {}
    
    // Strategy management
    void setStrategy(PrefetchStrategy strategy);
    PrefetchStrategy getStrategy() const { return currentStrategy; }
    void addPredictor(std::unique_ptr<PrefetchPredictor> predictor);
    
    // Prefetch control
    void update(const std::string& currentContext);
    void addManualHint(const PrefetchHint& hint);
    void clearHints();
    void executePrefetch();
    
    // Configuration
    void setMaxPrefetchBudget(size_t bytes) { maxPrefetchBudget = bytes; }
    void setPrefetchThreshold(float probability) { prefetchThreshold = probability; }
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    // Statistics
    struct PrefetchStats {
        size_t hitsCount;       // Prefetched assets that were actually used
        size_t missCount;       // Prefetched assets that were never used
        size_t wastedBytes;     // Memory used by unused prefetched assets
        float hitRate;
        float averagePredictionTime;
    };
    PrefetchStats getStatistics() const;
    void resetStatistics();
    
private:
    StreamingManager& streamingManager;
    PrefetchStrategy currentStrategy{PrefetchStrategy::None};
    std::vector<std::unique_ptr<PrefetchPredictor>> predictors;
    std::vector<PrefetchHint> pendingHints;
    std::unordered_set<std::string> prefetchedAssets;
    size_t maxPrefetchBudget{100 * 1024 * 1024};  // 100MB default
    float prefetchThreshold{0.5f};
    bool enabled{true};
    PrefetchStats stats{};
};

// ============================================================================
// DEPENDENCY TRACKING - Asset dependency graph
// ============================================================================

struct AssetDependency {
    std::string dependentAssetId;   // Asset that depends on another
    std::string dependencyAssetId;  // Asset being depended upon
    bool isRequired{true};          // If false, dependency is optional
    bool loadFirst{true};           // If true, load dependency before dependent
    float priority{1.0f};           // Priority multiplier for dependency
};

class DependencyGraph {
public:
    // Dependency management
    void addDependency(const AssetDependency& dep);
    void removeDependency(const std::string& dependentId, const std::string& dependencyId);
    void clearDependencies(const std::string& assetId);
    
    // Querying
    std::vector<std::string> getDependencies(const std::string& assetId) const;
    std::vector<std::string> getDependents(const std::string& assetId) const;
    std::vector<std::string> getAllDependencies(const std::string& assetId) const;  // Recursive
    std::vector<std::string> getAllDependents(const std::string& assetId) const;    // Recursive
    
    // Topological operations
    std::vector<std::string> getLoadOrder(const std::string& assetId) const;
    std::vector<std::string> getLoadOrder(const std::vector<std::string>& assetIds) const;
    bool hasCyclicDependency(const std::string& assetId) const;
    std::vector<std::pair<std::string, std::string>> detectCycles() const;
    
    // Graph info
    size_t getDependencyCount(const std::string& assetId) const;
    size_t getDependentCount(const std::string& assetId) const;
    size_t getTotalEdges() const;
    size_t getTotalNodes() const;
    
    // Serialization
    void serialize(std::ostream& stream) const;
    void deserialize(std::istream& stream);
    
    // Visualization (for debugging)
    std::string toDotFormat() const;  // GraphViz DOT format
    
private:
    std::unordered_map<std::string, std::vector<AssetDependency>> dependencies;
    std::unordered_map<std::string, std::vector<std::string>> reverseDependencies;
    
    void topologicalSort(const std::string& assetId, 
                         std::unordered_set<std::string>& visited,
                         std::vector<std::string>& result) const;
    bool detectCycleDFS(const std::string& node,
                        std::unordered_set<std::string>& visited,
                        std::unordered_set<std::string>& recursionStack) const;
};

class DependencyAwareLoader {
public:
    DependencyAwareLoader(StreamingManager& streaming, DependencyGraph& graph)
        : streamingManager(streaming), dependencyGraph(graph) {}
    
    // Loading with dependency resolution
    void loadWithDependencies(const std::string& assetId, AssetPriority priority = AssetPriority::Medium);
    void loadWithDependencies(const std::vector<std::string>& assetIds, AssetPriority priority = AssetPriority::Medium);
    
    // Unloading with dependency checking
    bool canSafelyUnload(const std::string& assetId) const;
    void unloadWithDependents(const std::string& assetId);
    
    // Batch operations
    struct LoadBatch {
        std::vector<std::string> assetIds;
        std::vector<std::string> loadOrder;
        size_t estimatedSize;
        float estimatedTime;
    };
    LoadBatch createLoadBatch(const std::vector<std::string>& assetIds) const;
    void executeLoadBatch(const LoadBatch& batch, AssetPriority priority = AssetPriority::Medium);
    
private:
    StreamingManager& streamingManager;
    DependencyGraph& dependencyGraph;
};

// ============================================================================
// COMPRESSION SUPPORT - Asset compression and decompression
// ============================================================================

enum class CompressionFormat {
    None,
    LZ4,            // Fast compression/decompression
    ZSTD,           // Balanced compression ratio/speed
    LZMA,           // High compression ratio
    Custom          // User-defined compression
};

struct CompressionInfo {
    CompressionFormat format{CompressionFormat::None};
    size_t uncompressedSize{0};
    size_t compressedSize{0};
    uint32_t compressionLevel{0};   // Format-specific level
    std::string dictionaryId;       // Optional compression dictionary
    
    float getCompressionRatio() const {
        return compressedSize > 0 ? static_cast<float>(uncompressedSize) / compressedSize : 1.0f;
    }
};

class ICompressor {
public:
    virtual ~ICompressor() = default;
    virtual CompressionFormat getFormat() const = 0;
    virtual std::vector<uint8_t> compress(const uint8_t* data, size_t size, uint32_t level = 0) = 0;
    virtual std::vector<uint8_t> decompress(const uint8_t* data, size_t compressedSize, size_t uncompressedSize) = 0;
    virtual size_t getMaxCompressedSize(size_t uncompressedSize) const = 0;
    
    // Dictionary support (optional)
    virtual void setDictionary(const std::vector<uint8_t>& dictionary) {}
    virtual void clearDictionary() {}
};

class LZ4Compressor : public ICompressor {
public:
    CompressionFormat getFormat() const override { return CompressionFormat::LZ4; }
    std::vector<uint8_t> compress(const uint8_t* data, size_t size, uint32_t level = 0) override;
    std::vector<uint8_t> decompress(const uint8_t* data, size_t compressedSize, size_t uncompressedSize) override;
    size_t getMaxCompressedSize(size_t uncompressedSize) const override;
};

class ZSTDCompressor : public ICompressor {
public:
    CompressionFormat getFormat() const override { return CompressionFormat::ZSTD; }
    std::vector<uint8_t> compress(const uint8_t* data, size_t size, uint32_t level = 0) override;
    std::vector<uint8_t> decompress(const uint8_t* data, size_t compressedSize, size_t uncompressedSize) override;
    size_t getMaxCompressedSize(size_t uncompressedSize) const override;
    
    void setDictionary(const std::vector<uint8_t>& dictionary) override;
    void clearDictionary() override;
    
private:
    std::vector<uint8_t> dictionary;
};

class CompressionManager {
public:
    // Compressor registration
    void registerCompressor(std::unique_ptr<ICompressor> compressor);
    ICompressor* getCompressor(CompressionFormat format);
    
    // High-level operations
    std::vector<uint8_t> compressData(const uint8_t* data, size_t size, CompressionFormat format, uint32_t level = 0);
    std::vector<uint8_t> decompressData(const uint8_t* data, size_t compressedSize, size_t uncompressedSize, CompressionFormat format);
    
    // Auto-select best format
    CompressionFormat selectOptimalFormat(size_t dataSize, bool preferSpeed = true) const;
    
    // Dictionary management
    void registerDictionary(const std::string& id, const std::vector<uint8_t>& dictionary);
    const std::vector<uint8_t>* getDictionary(const std::string& id) const;
    
    // Statistics
    struct CompressionStats {
        size_t totalBytesCompressed;
        size_t totalBytesDecompressed;
        size_t totalBytesSaved;
        float averageCompressionRatio;
        float averageCompressionSpeed;    // MB/s
        float averageDecompressionSpeed;  // MB/s
    };
    CompressionStats getStatistics() const;
    
private:
    std::unordered_map<CompressionFormat, std::unique_ptr<ICompressor>> compressors;
    std::unordered_map<std::string, std::vector<uint8_t>> dictionaries;
    mutable CompressionStats stats{};
};

// ============================================================================
// STREAMING CONFIGURATION - Comprehensive settings
// ============================================================================

struct StreamingConfiguration {
    // Memory settings
    size_t maxMemoryBudget{512 * 1024 * 1024};      // 512MB
    size_t reservedMemory{64 * 1024 * 1024};        // 64MB reserved
    float memoryPressureThreshold{0.85f};
    
    // Worker settings
    size_t workerCount{4};
    size_t maxConcurrentLoads{8};
    std::chrono::milliseconds loadTimeout{30000};
    
    // Cache settings
    size_t cacheSize{256 * 1024 * 1024};            // 256MB
    float cacheEvictionThreshold{0.9f};
    
    // Prefetch settings
    PrefetchStrategy prefetchStrategy{PrefetchStrategy::Hybrid};
    size_t prefetchBudget{100 * 1024 * 1024};       // 100MB
    float prefetchThreshold{0.5f};
    
    // Compression settings
    CompressionFormat defaultCompression{CompressionFormat::LZ4};
    bool decompressOnLoad{true};
    
    // Region streaming
    float regionLoadDistance{150.0f};
    float regionUnloadDistance{200.0f};
    float hysteresisMargin{10.0f};
    bool velocityPrediction{true};
    
    // Debug settings
    bool enableProfiling{false};
    bool enableLogging{false};
    bool validateDependencies{true};
    
    // Serialization
    void saveToFile(const std::filesystem::path& path) const;
    static StreamingConfiguration loadFromFile(const std::filesystem::path& path);
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
    
    // Compression helpers
    CompressionInfo analyzeForCompression(const uint8_t* data, size_t size);
    bool isCompressible(const std::string& assetType);
    CompressionFormat recommendCompression(const std::string& assetType, size_t size);
}

} // namespace Streaming
} // namespace JJM