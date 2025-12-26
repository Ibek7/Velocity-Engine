#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Resource streaming system for loading assets on demand
namespace Engine {

enum class StreamPriority {
    Critical,  // Load immediately
    High,      // Load soon
    Normal,    // Load when convenient
    Low        // Load when idle
};

enum class StreamState {
    Pending,
    Loading,
    Loaded,
    Failed,
    Unloading
};

struct StreamRequest {
    std::string resourcePath;
    StreamPriority priority;
    size_t estimatedSize;
    std::function<void(void*, size_t)> onComplete;
    std::function<void(const std::string&)> onError;
    void* userData;
};

class StreamingManager {
public:
    static StreamingManager& getInstance();

    // Request streaming
    void requestLoad(const std::string& path, StreamPriority priority,
                    std::function<void(void*, size_t)> onComplete,
                    std::function<void(const std::string&)> onError = nullptr);
    
    void requestUnload(const std::string& path);
    void cancelRequest(const std::string& path);
    
    // Priority management
    void changePriority(const std::string& path, StreamPriority newPriority);
    void promoteNearbyResources(float centerX, float centerY, float centerZ, float radius);
    
    // Update
    void update(float deltaTime);
    
    // Configuration
    void setMaxConcurrentLoads(int max) { m_maxConcurrentLoads = max; }
    void setMemoryBudget(size_t bytes) { m_memoryBudget = bytes; }
    void setBandwidthLimit(size_t bytesPerSecond) { m_bandwidthLimit = bytesPerSecond; }
    
    // Query
    StreamState getState(const std::string& path) const;
    bool isLoaded(const std::string& path) const;
    size_t getCurrentMemoryUsage() const { return m_currentMemoryUsage; }
    int getPendingRequestCount() const;
    
    // Memory management
    void unloadLRU(size_t targetBytes);
    void unloadAll();
    
private:
    StreamingManager();
    StreamingManager(const StreamingManager&) = delete;
    StreamingManager& operator=(const StreamingManager&) = delete;

    void processRequests();
    void startLoad(const StreamRequest& request);
    void completeLoad(const std::string& path, void* data, size_t size);
    void failLoad(const std::string& path, const std::string& error);

    struct LoadedResource {
        std::string path;
        void* data;
        size_t size;
        float lastAccessTime;
    };

    std::vector<StreamRequest> m_pendingRequests;
    std::vector<std::string> m_activeLoads;
    std::vector<LoadedResource> m_loadedResources;
    
    int m_maxConcurrentLoads;
    size_t m_memoryBudget;
    size_t m_currentMemoryUsage;
    size_t m_bandwidthLimit;
    
    float m_totalTime;
};

} // namespace Engine
