#include "streaming/StreamingManager.h"
#include <algorithm>
#include <fstream>

namespace Engine {

StreamingManager::StreamingManager()
    : m_maxConcurrentLoads(4)
    , m_memoryBudget(512 * 1024 * 1024) // 512 MB
    , m_currentMemoryUsage(0)
    , m_bandwidthLimit(10 * 1024 * 1024) // 10 MB/s
    , m_totalTime(0.0f)
{
}

StreamingManager& StreamingManager::getInstance() {
    static StreamingManager instance;
    return instance;
}

void StreamingManager::requestLoad(const std::string& path, StreamPriority priority,
                                   std::function<void(void*, size_t)> onComplete,
                                   std::function<void(const std::string&)> onError) {
    // Check if already loaded
    for (auto& resource : m_loadedResources) {
        if (resource.path == path) {
            resource.lastAccessTime = m_totalTime;
            if (onComplete) {
                onComplete(resource.data, resource.size);
            }
            return;
        }
    }
    
    // Check if already loading
    for (const auto& loadPath : m_activeLoads) {
        if (loadPath == path) {
            return;
        }
    }
    
    // Check if already requested
    for (auto& request : m_pendingRequests) {
        if (request.resourcePath == path) {
            // Update priority if higher
            if (priority < request.priority) {
                request.priority = priority;
            }
            return;
        }
    }
    
    // Add new request
    StreamRequest request;
    request.resourcePath = path;
    request.priority = priority;
    request.estimatedSize = 1024 * 1024; // Default 1MB estimate
    request.onComplete = onComplete;
    request.onError = onError;
    request.userData = nullptr;
    
    m_pendingRequests.push_back(request);
    
    // Sort by priority
    std::sort(m_pendingRequests.begin(), m_pendingRequests.end(),
        [](const StreamRequest& a, const StreamRequest& b) {
            return a.priority < b.priority;
        });
}

void StreamingManager::requestUnload(const std::string& path) {
    for (auto it = m_loadedResources.begin(); it != m_loadedResources.end(); ++it) {
        if (it->path == path) {
            if (it->data) {
                delete[] static_cast<uint8_t*>(it->data);
            }
            m_currentMemoryUsage -= it->size;
            m_loadedResources.erase(it);
            return;
        }
    }
}

void StreamingManager::cancelRequest(const std::string& path) {
    m_pendingRequests.erase(
        std::remove_if(m_pendingRequests.begin(), m_pendingRequests.end(),
            [&path](const StreamRequest& req) { return req.resourcePath == path; }),
        m_pendingRequests.end()
    );
}

void StreamingManager::changePriority(const std::string& path, StreamPriority newPriority) {
    for (auto& request : m_pendingRequests) {
        if (request.resourcePath == path) {
            request.priority = newPriority;
            
            // Re-sort
            std::sort(m_pendingRequests.begin(), m_pendingRequests.end(),
                [](const StreamRequest& a, const StreamRequest& b) {
                    return a.priority < b.priority;
                });
            return;
        }
    }
}

void StreamingManager::promoteNearbyResources(float centerX, float centerY, float centerZ, float radius) {
    // TODO: Promote resources near position
    (void)centerX;
    (void)centerY;
    (void)centerZ;
    (void)radius;
}

void StreamingManager::update(float deltaTime) {
    m_totalTime += deltaTime;
    processRequests();
}

void StreamingManager::processRequests() {
    // Start new loads if under limit
    while (static_cast<int>(m_activeLoads.size()) < m_maxConcurrentLoads && !m_pendingRequests.empty()) {
        StreamRequest request = m_pendingRequests.front();
        m_pendingRequests.erase(m_pendingRequests.begin());
        
        // Check memory budget
        if (m_currentMemoryUsage + request.estimatedSize > m_memoryBudget) {
            // Unload LRU to make space
            unloadLRU(request.estimatedSize);
        }
        
        startLoad(request);
    }
}

void StreamingManager::startLoad(const StreamRequest& request) {
    m_activeLoads.push_back(request.resourcePath);
    
    // Simulate async load (in real implementation, use thread pool)
    std::ifstream file(request.resourcePath, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        failLoad(request.resourcePath, "Failed to open file");
        if (request.onError) {
            request.onError("Failed to open file: " + request.resourcePath);
        }
        return;
    }
    
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    uint8_t* data = new uint8_t[size];
    file.read(reinterpret_cast<char*>(data), size);
    file.close();
    
    completeLoad(request.resourcePath, data, size);
    
    if (request.onComplete) {
        request.onComplete(data, size);
    }
}

void StreamingManager::completeLoad(const std::string& path, void* data, size_t size) {
    // Remove from active loads
    m_activeLoads.erase(
        std::remove(m_activeLoads.begin(), m_activeLoads.end(), path),
        m_activeLoads.end()
    );
    
    // Add to loaded resources
    LoadedResource resource;
    resource.path = path;
    resource.data = data;
    resource.size = size;
    resource.lastAccessTime = m_totalTime;
    
    m_loadedResources.push_back(resource);
    m_currentMemoryUsage += size;
}

void StreamingManager::failLoad(const std::string& path, const std::string& error) {
    (void)error;
    
    // Remove from active loads
    m_activeLoads.erase(
        std::remove(m_activeLoads.begin(), m_activeLoads.end(), path),
        m_activeLoads.end()
    );
}

StreamState StreamingManager::getState(const std::string& path) const {
    // Check if loaded
    for (const auto& resource : m_loadedResources) {
        if (resource.path == path) {
            return StreamState::Loaded;
        }
    }
    
    // Check if loading
    for (const auto& loadPath : m_activeLoads) {
        if (loadPath == path) {
            return StreamState::Loading;
        }
    }
    
    // Check if pending
    for (const auto& request : m_pendingRequests) {
        if (request.resourcePath == path) {
            return StreamState::Pending;
        }
    }
    
    return StreamState::Failed;
}

bool StreamingManager::isLoaded(const std::string& path) const {
    return getState(path) == StreamState::Loaded;
}

int StreamingManager::getPendingRequestCount() const {
    return m_pendingRequests.size() + m_activeLoads.size();
}

void StreamingManager::unloadLRU(size_t targetBytes) {
    // Sort by last access time
    std::sort(m_loadedResources.begin(), m_loadedResources.end(),
        [](const LoadedResource& a, const LoadedResource& b) {
            return a.lastAccessTime < b.lastAccessTime;
        });
    
    size_t freedBytes = 0;
    
    while (freedBytes < targetBytes && !m_loadedResources.empty()) {
        auto& resource = m_loadedResources.front();
        
        if (resource.data) {
            delete[] static_cast<uint8_t*>(resource.data);
        }
        
        freedBytes += resource.size;
        m_currentMemoryUsage -= resource.size;
        
        m_loadedResources.erase(m_loadedResources.begin());
    }
}

void StreamingManager::unloadAll() {
    for (auto& resource : m_loadedResources) {
        if (resource.data) {
            delete[] static_cast<uint8_t*>(resource.data);
        }
    }
    
    m_loadedResources.clear();
    m_currentMemoryUsage = 0;
}

} // namespace Engine
