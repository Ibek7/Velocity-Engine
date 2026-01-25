#ifndef RESOURCE_PREFETCHING_H
#define RESOURCE_PREFETCHING_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace JJM {
namespace Core {

// Forward declarations
class Resource;

// Priority levels for resource loading
enum class LoadPriority {
    CRITICAL = 0,    // Must load immediately (player character, current level)
    HIGH = 1,        // Load very soon (nearby areas, important NPCs)
    MEDIUM = 2,      // Load when possible (distant objects, effects)
    LOW = 3,         // Load in background (optional content, far areas)
    DEFERRED = 4     // Load only if memory available
};

// Resource load state
enum class LoadState {
    UNLOADED,
    QUEUED,
    LOADING,
    LOADED,
    FAILED
};

// Resource type categories
enum class ResourceType {
    TEXTURE,
    MESH,
    AUDIO,
    SHADER,
    MATERIAL,
    ANIMATION,
    SCRIPT,
    PREFAB,
    SCENE,
    OTHER
};

// Resource metadata
struct ResourceInfo {
    std::string path;
    ResourceType type;
    LoadState state;
    LoadPriority priority;
    size_t estimatedSize;      // Bytes
    float distanceToPlayer;    // For spatial prefetching
    float lastAccessTime;      // For LRU cache
    int accessCount;           // Usage tracking
    std::vector<std::string> dependencies;  // Other resources needed
    void* resourcePtr;         // Pointer to loaded resource
    
    ResourceInfo()
        : type(ResourceType::OTHER), state(LoadState::UNLOADED),
          priority(LoadPriority::MEDIUM), estimatedSize(0),
          distanceToPlayer(0), lastAccessTime(0), accessCount(0),
          resourcePtr(nullptr) {}
};

// Prefetch hint - suggests what to load
struct PrefetchHint {
    std::string resourcePath;
    LoadPriority priority;
    float probability;         // 0-1, how likely it will be needed
    float timeUntilNeeded;     // Seconds until expected to be needed
    std::string reason;        // Debug info
    
    PrefetchHint(const std::string& path, LoadPriority prio = LoadPriority::MEDIUM)
        : resourcePath(path), priority(prio), probability(1.0f),
          timeUntilNeeded(0.0f) {}
};

// Prefetch strategy - different ways to predict resource needs
class IPrefetchStrategy {
public:
    virtual ~IPrefetchStrategy() = default;
    
    // Generate prefetch hints based on current game state
    virtual std::vector<PrefetchHint> generateHints(const void* gameState) = 0;
    
    // Update strategy based on actual usage
    virtual void recordUsage(const std::string& resourcePath, bool wasNeeded) = 0;
    
    virtual std::string getName() const = 0;
};

// Spatial prefetching - load resources near player
class SpatialPrefetchStrategy : public IPrefetchStrategy {
public:
    SpatialPrefetchStrategy();
    
    void setPlayerPosition(float x, float y, float z);
    void setPlayerVelocity(float vx, float vy, float vz);
    void setPrefetchRadius(float radius) { m_prefetchRadius = radius; }
    void setPredictionTime(float time) { m_predictionTime = time; }
    
    std::vector<PrefetchHint> generateHints(const void* gameState) override;
    void recordUsage(const std::string& resourcePath, bool wasNeeded) override;
    std::string getName() const override { return "Spatial"; }
    
    // Register spatial resources
    void registerResource(const std::string& path, float x, float y, float z);
    
private:
    float m_playerPos[3];
    float m_playerVel[3];
    float m_prefetchRadius;
    float m_predictionTime;
    
    struct SpatialResource {
        std::string path;
        float position[3];
    };
    std::vector<SpatialResource> m_spatialResources;
};

// Sequential prefetching - load next resources in a sequence
class SequentialPrefetchStrategy : public IPrefetchStrategy {
public:
    SequentialPrefetchStrategy();
    
    std::vector<PrefetchHint> generateHints(const void* gameState) override;
    void recordUsage(const std::string& resourcePath, bool wasNeeded) override;
    std::string getName() const override { return "Sequential"; }
    
    // Define sequences (e.g., level progression)
    void defineSequence(const std::string& name, const std::vector<std::string>& resources);
    void setCurrentSequence(const std::string& name, int currentIndex);
    void setLookahead(int count) { m_lookahead = count; }
    
private:
    struct Sequence {
        std::vector<std::string> resources;
        int currentIndex;
    };
    std::unordered_map<std::string, Sequence> m_sequences;
    std::string m_activeSequence;
    int m_lookahead;  // How many ahead to prefetch
};

// Pattern-based prefetching - learn from player behavior
class PatternPrefetchStrategy : public IPrefetchStrategy {
public:
    PatternPrefetchStrategy();
    
    std::vector<PrefetchHint> generateHints(const void* gameState) override;
    void recordUsage(const std::string& resourcePath, bool wasNeeded) override;
    std::string getName() const override { return "Pattern"; }
    
    // Learning parameters
    void setLearningRate(float rate) { m_learningRate = rate; }
    void setMinConfidence(float conf) { m_minConfidence = conf; }
    
private:
    float m_learningRate;
    float m_minConfidence;
    
    // Pattern: "after loading A, B is often loaded"
    struct Pattern {
        std::string triggerResource;
        std::string followupResource;
        float confidence;
        int observationCount;
    };
    std::vector<Pattern> m_patterns;
    std::vector<std::string> m_recentLoads;  // Recent history
    
    void updatePatterns(const std::string& newResource);
};

// Dependency prefetching - load required dependencies
class DependencyPrefetchStrategy : public IPrefetchStrategy {
public:
    DependencyPrefetchStrategy();
    
    std::vector<PrefetchHint> generateHints(const void* gameState) override;
    void recordUsage(const std::string& resourcePath, bool wasNeeded) override;
    std::string getName() const override { return "Dependency"; }
    
    void registerDependency(const std::string& resource, const std::string& dependency);
    
private:
    std::unordered_map<std::string, std::vector<std::string>> m_dependencies;
};

// Resource load request
struct LoadRequest {
    std::string resourcePath;
    LoadPriority priority;
    float timestamp;
    std::function<void(Resource*)> callback;
    
    LoadRequest(const std::string& path, LoadPriority prio)
        : resourcePath(path), priority(prio), timestamp(0) {}
    
    bool operator<(const LoadRequest& other) const {
        // Higher priority (lower enum value) comes first
        if (priority != other.priority)
            return priority > other.priority;
        return timestamp > other.timestamp;  // Older requests first
    }
};

// Main resource prefetching system
class ResourcePrefetchingSystem {
public:
    ResourcePrefetchingSystem();
    ~ResourcePrefetchingSystem();
    
    void initialize(size_t maxMemoryBytes);
    void shutdown();
    
    // Update system (call once per frame)
    void update(float deltaTime);
    
    // Strategy management
    void addStrategy(std::unique_ptr<IPrefetchStrategy> strategy);
    void removeStrategy(const std::string& name);
    void enableStrategy(const std::string& name, bool enable);
    
    // Manual prefetch requests
    void prefetch(const std::string& resourcePath, LoadPriority priority = LoadPriority::MEDIUM);
    void prefetchBatch(const std::vector<std::string>& resources, LoadPriority priority = LoadPriority::MEDIUM);
    
    // Resource queries
    bool isLoaded(const std::string& resourcePath) const;
    LoadState getLoadState(const std::string& resourcePath) const;
    Resource* getResource(const std::string& resourcePath);
    
    // Memory management
    void setMaxMemory(size_t bytes) { m_maxMemoryBytes = bytes; }
    size_t getMaxMemory() const { return m_maxMemoryBytes; }
    size_t getCurrentMemoryUsage() const;
    void setMemoryPressureThreshold(float threshold) { m_memoryPressureThreshold = threshold; }
    
    // Unload resources to free memory
    void unloadLeastRecentlyUsed(size_t targetBytes);
    void unloadResource(const std::string& resourcePath);
    void clearCache();
    
    // Statistics
    struct Stats {
        int totalPrefetchHints;
        int successfulPrefetches;
        int wastedPrefetches;       // Prefetched but never used
        int cacheMisses;            // Needed but not prefetched
        float averageLoadTime;
        float cacheHitRate;
        size_t memoryUsed;
        size_t memoryAvailable;
    };
    
    Stats getStatistics() const;
    void resetStatistics();
    
    // Configuration
    void setMaxConcurrentLoads(int max) { m_maxConcurrentLoads = max; }
    void setMinTimeBetweenUpdates(float seconds) { m_minUpdateInterval = seconds; }
    void enableAsyncLoading(bool enable) { m_asyncLoadingEnabled = enable; }
    void enablePredictiveLoading(bool enable) { m_predictiveLoadingEnabled = enable; }
    
    // Debug
    void printResourceState() const;
    std::vector<std::string> getQueuedResources() const;
    std::vector<std::string> getLoadedResources() const;
    
private:
    // Resource tracking
    std::unordered_map<std::string, ResourceInfo> m_resources;
    
    // Prefetch strategies
    std::vector<std::unique_ptr<IPrefetchStrategy>> m_strategies;
    std::unordered_set<std::string> m_disabledStrategies;
    
    // Load queue (priority queue)
    std::priority_queue<LoadRequest> m_loadQueue;
    std::vector<LoadRequest> m_activeLoads;
    
    // Memory management
    size_t m_maxMemoryBytes;
    float m_memoryPressureThreshold;  // 0-1, trigger unload above this
    
    // Threading
    std::vector<std::thread> m_workerThreads;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
    bool m_shutdownRequested;
    
    // Configuration
    int m_maxConcurrentLoads;
    float m_minUpdateInterval;
    float m_lastUpdateTime;
    bool m_asyncLoadingEnabled;
    bool m_predictiveLoadingEnabled;
    
    // Statistics
    mutable Stats m_stats;
    
    // Internal methods
    void gatherPrefetchHints();
    void processLoadQueue();
    void workerThreadFunction();
    Resource* loadResourceSync(const std::string& path);
    void evictIfNeeded();
    float calculateLoadPriority(const ResourceInfo& info) const;
    void updateResourceMetadata(const std::string& path);
    
    // Callbacks
    std::unordered_map<std::string, std::vector<std::function<void(Resource*)>>> m_loadCallbacks;
    void triggerCallbacks(const std::string& path, Resource* resource);
};

// Global prefetch manager
class PrefetchManager {
public:
    static PrefetchManager& getInstance();
    
    ResourcePrefetchingSystem* getSystem() { return m_system.get(); }
    
    // Convenience methods
    static void prefetch(const std::string& path, LoadPriority priority = LoadPriority::MEDIUM);
    static bool isLoaded(const std::string& path);
    static Resource* get(const std::string& path);
    
private:
    PrefetchManager();
    ~PrefetchManager();
    
    std::unique_ptr<ResourcePrefetchingSystem> m_system;
};

} // namespace Core
} // namespace JJM

#endif // RESOURCE_PREFETCHING_H
