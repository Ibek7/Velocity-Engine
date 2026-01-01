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
    
    bool operator<(const LoadRequest& other) const {
        return static_cast<int>(priority) < static_cast<int>(other.priority);
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
    
    // Statistics tracking
    std::atomic<size_t> totalMemoryUsed;
    std::atomic<size_t> failedLoads;
    std::atomic<float> totalLoadTime;
    std::atomic<size_t> loadCount;
    
    // Resource limits
    size_t maxMemoryLimit;
    
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
    
    // Statistics
    ResourceStats getStats() const;
    void resetStats();
    
    // Clear all resources
    void clear();
    
private:
    void workerThread();
    void processLoadRequest(const LoadRequest& request);
};

} // namespace Core
} // namespace JJM

#endif // RESOURCE_MANAGER_H
