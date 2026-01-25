#include "core/ResourcePrefetching.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>

namespace JJM {
namespace Core {

// SpatialPrefetchStrategy implementation
SpatialPrefetchStrategy::SpatialPrefetchStrategy()
    : m_prefetchRadius(50.0f), m_predictionTime(2.0f) {
    m_playerPos[0] = m_playerPos[1] = m_playerPos[2] = 0;
    m_playerVel[0] = m_playerVel[1] = m_playerVel[2] = 0;
}

void SpatialPrefetchStrategy::setPlayerPosition(float x, float y, float z) {
    m_playerPos[0] = x;
    m_playerPos[1] = y;
    m_playerPos[2] = z;
}

void SpatialPrefetchStrategy::setPlayerVelocity(float vx, float vy, float vz) {
    m_playerVel[0] = vx;
    m_playerVel[1] = vy;
    m_playerVel[2] = vz;
}

std::vector<PrefetchHint> SpatialPrefetchStrategy::generateHints(const void* gameState) {
    std::vector<PrefetchHint> hints;
    
    // Predicted future position
    float futurePos[3] = {
        m_playerPos[0] + m_playerVel[0] * m_predictionTime,
        m_playerPos[1] + m_playerVel[1] * m_predictionTime,
        m_playerPos[2] + m_playerVel[2] * m_predictionTime
    };
    
    for (const auto& resource : m_spatialResources) {
        float dx = resource.position[0] - m_playerPos[0];
        float dy = resource.position[1] - m_playerPos[1];
        float dz = resource.position[2] - m_playerPos[2];
        float distCurrent = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        float fdx = resource.position[0] - futurePos[0];
        float fdy = resource.position[1] - futurePos[1];
        float fdz = resource.position[2] - futurePos[2];
        float distFuture = std::sqrt(fdx*fdx + fdy*fdy + fdz*fdz);
        
        // Load if within prefetch radius or will be soon
        if (distCurrent < m_prefetchRadius || distFuture < m_prefetchRadius) {
            PrefetchHint hint(resource.path);
            
            // Priority based on distance
            if (distCurrent < m_prefetchRadius * 0.3f) {
                hint.priority = LoadPriority::HIGH;
            } else if (distCurrent < m_prefetchRadius * 0.6f) {
                hint.priority = LoadPriority::MEDIUM;
            } else {
                hint.priority = LoadPriority::LOW;
            }
            
            hint.probability = 1.0f - (distCurrent / m_prefetchRadius);
            hint.timeUntilNeeded = distCurrent / std::max(0.1f, std::sqrt(
                m_playerVel[0]*m_playerVel[0] +
                m_playerVel[1]*m_playerVel[1] +
                m_playerVel[2]*m_playerVel[2]
            ));
            hint.reason = "Spatial proximity";
            
            hints.push_back(hint);
        }
    }
    
    return hints;
}

void SpatialPrefetchStrategy::recordUsage(const std::string& resourcePath, bool wasNeeded) {
    // Could track accuracy and adjust prefetch radius
}

void SpatialPrefetchStrategy::registerResource(const std::string& path, float x, float y, float z) {
    SpatialResource resource;
    resource.path = path;
    resource.position[0] = x;
    resource.position[1] = y;
    resource.position[2] = z;
    m_spatialResources.push_back(resource);
}

// SequentialPrefetchStrategy implementation
SequentialPrefetchStrategy::SequentialPrefetchStrategy()
    : m_lookahead(3) {
}

std::vector<PrefetchHint> SequentialPrefetchStrategy::generateHints(const void* gameState) {
    std::vector<PrefetchHint> hints;
    
    if (m_activeSequence.empty()) return hints;
    
    auto it = m_sequences.find(m_activeSequence);
    if (it == m_sequences.end()) return hints;
    
    const Sequence& seq = it->second;
    
    // Prefetch next N resources in sequence
    for (int i = 1; i <= m_lookahead; ++i) {
        int index = seq.currentIndex + i;
        if (index >= static_cast<int>(seq.resources.size())) break;
        
        PrefetchHint hint(seq.resources[index]);
        
        // Priority decreases with distance in sequence
        if (i == 1) hint.priority = LoadPriority::HIGH;
        else if (i == 2) hint.priority = LoadPriority::MEDIUM;
        else hint.priority = LoadPriority::LOW;
        
        hint.probability = 1.0f - (static_cast<float>(i) / m_lookahead);
        hint.timeUntilNeeded = static_cast<float>(i) * 5.0f;  // Estimate
        hint.reason = "Sequential: " + m_activeSequence;
        
        hints.push_back(hint);
    }
    
    return hints;
}

void SequentialPrefetchStrategy::recordUsage(const std::string& resourcePath, bool wasNeeded) {
    // Could track accuracy
}

void SequentialPrefetchStrategy::defineSequence(const std::string& name,
                                               const std::vector<std::string>& resources) {
    Sequence seq;
    seq.resources = resources;
    seq.currentIndex = 0;
    m_sequences[name] = seq;
}

void SequentialPrefetchStrategy::setCurrentSequence(const std::string& name, int currentIndex) {
    m_activeSequence = name;
    auto it = m_sequences.find(name);
    if (it != m_sequences.end()) {
        it->second.currentIndex = currentIndex;
    }
}

// PatternPrefetchStrategy implementation
PatternPrefetchStrategy::PatternPrefetchStrategy()
    : m_learningRate(0.1f), m_minConfidence(0.5f) {
}

std::vector<PrefetchHint> PatternPrefetchStrategy::generateHints(const void* gameState) {
    std::vector<PrefetchHint> hints;
    
    if (m_recentLoads.empty()) return hints;
    
    // Look for patterns based on recent loads
    std::string lastLoaded = m_recentLoads.back();
    
    for (const auto& pattern : m_patterns) {
        if (pattern.triggerResource == lastLoaded && pattern.confidence >= m_minConfidence) {
            PrefetchHint hint(pattern.followupResource);
            hint.probability = pattern.confidence;
            hint.priority = LoadPriority::MEDIUM;
            hint.reason = "Pattern: " + std::to_string(pattern.confidence);
            hints.push_back(hint);
        }
    }
    
    return hints;
}

void PatternPrefetchStrategy::recordUsage(const std::string& resourcePath, bool wasNeeded) {
    updatePatterns(resourcePath);
}

void PatternPrefetchStrategy::updatePatterns(const std::string& newResource) {
    m_recentLoads.push_back(newResource);
    
    // Keep only recent history
    if (m_recentLoads.size() > 10) {
        m_recentLoads.erase(m_recentLoads.begin());
    }
    
    // Update patterns
    if (m_recentLoads.size() >= 2) {
        std::string trigger = m_recentLoads[m_recentLoads.size() - 2];
        std::string followup = newResource;
        
        // Find or create pattern
        bool found = false;
        for (auto& pattern : m_patterns) {
            if (pattern.triggerResource == trigger && pattern.followupResource == followup) {
                // Increase confidence
                pattern.confidence += m_learningRate * (1.0f - pattern.confidence);
                pattern.observationCount++;
                found = true;
                break;
            }
        }
        
        if (!found) {
            Pattern newPattern;
            newPattern.triggerResource = trigger;
            newPattern.followupResource = followup;
            newPattern.confidence = 0.3f;
            newPattern.observationCount = 1;
            m_patterns.push_back(newPattern);
        }
    }
    
    // Decay unused patterns
    for (auto& pattern : m_patterns) {
        pattern.confidence *= 0.99f;
    }
    
    // Remove very low confidence patterns
    m_patterns.erase(
        std::remove_if(m_patterns.begin(), m_patterns.end(),
                      [](const Pattern& p) { return p.confidence < 0.1f; }),
        m_patterns.end()
    );
}

// DependencyPrefetchStrategy implementation
DependencyPrefetchStrategy::DependencyPrefetchStrategy() {
}

std::vector<PrefetchHint> DependencyPrefetchStrategy::generateHints(const void* gameState) {
    // Would need access to currently loading resources
    // For now, return empty
    return std::vector<PrefetchHint>();
}

void DependencyPrefetchStrategy::recordUsage(const std::string& resourcePath, bool wasNeeded) {
}

void DependencyPrefetchStrategy::registerDependency(const std::string& resource,
                                                   const std::string& dependency) {
    m_dependencies[resource].push_back(dependency);
}

// ResourcePrefetchingSystem implementation
ResourcePrefetchingSystem::ResourcePrefetchingSystem()
    : m_maxMemoryBytes(512 * 1024 * 1024),  // 512 MB default
      m_memoryPressureThreshold(0.8f),
      m_shutdownRequested(false),
      m_maxConcurrentLoads(4),
      m_minUpdateInterval(0.1f),
      m_lastUpdateTime(0),
      m_asyncLoadingEnabled(true),
      m_predictiveLoadingEnabled(true) {
    m_stats = Stats();
}

ResourcePrefetchingSystem::~ResourcePrefetchingSystem() {
    shutdown();
}

void ResourcePrefetchingSystem::initialize(size_t maxMemoryBytes) {
    m_maxMemoryBytes = maxMemoryBytes;
    
    // Start worker threads
    if (m_asyncLoadingEnabled) {
        int threadCount = std::max(1, std::min(m_maxConcurrentLoads, 4));
        for (int i = 0; i < threadCount; ++i) {
            m_workerThreads.emplace_back(&ResourcePrefetchingSystem::workerThreadFunction, this);
        }
    }
}

void ResourcePrefetchingSystem::shutdown() {
    m_shutdownRequested = true;
    m_queueCondition.notify_all();
    
    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    m_workerThreads.clear();
    clearCache();
}

void ResourcePrefetchingSystem::update(float deltaTime) {
    m_lastUpdateTime += deltaTime;
    
    if (m_lastUpdateTime < m_minUpdateInterval) return;
    m_lastUpdateTime = 0;
    
    // Update resource metadata
    for (auto& pair : m_resources) {
        updateResourceMetadata(pair.first);
    }
    
    // Generate prefetch hints from strategies
    if (m_predictiveLoadingEnabled) {
        gatherPrefetchHints();
    }
    
    // Process load queue
    processLoadQueue();
    
    // Evict resources if memory pressure is high
    if (getCurrentMemoryUsage() > m_maxMemoryBytes * m_memoryPressureThreshold) {
        evictIfNeeded();
    }
}

void ResourcePrefetchingSystem::addStrategy(std::unique_ptr<IPrefetchStrategy> strategy) {
    m_strategies.push_back(std::move(strategy));
}

void ResourcePrefetchingSystem::removeStrategy(const std::string& name) {
    m_strategies.erase(
        std::remove_if(m_strategies.begin(), m_strategies.end(),
                      [&name](const std::unique_ptr<IPrefetchStrategy>& s) {
                          return s->getName() == name;
                      }),
        m_strategies.end()
    );
}

void ResourcePrefetchingSystem::enableStrategy(const std::string& name, bool enable) {
    if (enable) {
        m_disabledStrategies.erase(name);
    } else {
        m_disabledStrategies.insert(name);
    }
}

void ResourcePrefetchingSystem::prefetch(const std::string& resourcePath, LoadPriority priority) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Check if already loaded or queued
    auto it = m_resources.find(resourcePath);
    if (it != m_resources.end()) {
        if (it->second.state == LoadState::LOADED || it->second.state == LoadState::LOADING) {
            return;
        }
    }
    
    // Add to queue
    LoadRequest request(resourcePath, priority);
    request.timestamp = m_lastUpdateTime;
    m_loadQueue.push(request);
    
    // Update resource info
    if (it == m_resources.end()) {
        ResourceInfo info;
        info.path = resourcePath;
        info.state = LoadState::QUEUED;
        info.priority = priority;
        m_resources[resourcePath] = info;
    } else {
        it->second.state = LoadState::QUEUED;
        it->second.priority = priority;
    }
    
    m_queueCondition.notify_one();
}

void ResourcePrefetchingSystem::prefetchBatch(const std::vector<std::string>& resources,
                                             LoadPriority priority) {
    for (const auto& path : resources) {
        prefetch(path, priority);
    }
}

bool ResourcePrefetchingSystem::isLoaded(const std::string& resourcePath) const {
    auto it = m_resources.find(resourcePath);
    return it != m_resources.end() && it->second.state == LoadState::LOADED;
}

LoadState ResourcePrefetchingSystem::getLoadState(const std::string& resourcePath) const {
    auto it = m_resources.find(resourcePath);
    return it != m_resources.end() ? it->second.state : LoadState::UNLOADED;
}

Resource* ResourcePrefetchingSystem::getResource(const std::string& resourcePath) {
    auto it = m_resources.find(resourcePath);
    if (it != m_resources.end() && it->second.state == LoadState::LOADED) {
        it->second.accessCount++;
        it->second.lastAccessTime = m_lastUpdateTime;
        return static_cast<Resource*>(it->second.resourcePtr);
    }
    
    m_stats.cacheMisses++;
    return nullptr;
}

size_t ResourcePrefetchingSystem::getCurrentMemoryUsage() const {
    size_t total = 0;
    for (const auto& pair : m_resources) {
        if (pair.second.state == LoadState::LOADED) {
            total += pair.second.estimatedSize;
        }
    }
    return total;
}

void ResourcePrefetchingSystem::unloadLeastRecentlyUsed(size_t targetBytes) {
    // Collect loaded resources
    std::vector<std::pair<std::string, ResourceInfo*>> loaded;
    for (auto& pair : m_resources) {
        if (pair.second.state == LoadState::LOADED) {
            loaded.push_back({pair.first, &pair.second});
        }
    }
    
    // Sort by last access time (LRU)
    std::sort(loaded.begin(), loaded.end(),
             [](const auto& a, const auto& b) {
                 return a.second->lastAccessTime < b.second->lastAccessTime;
             });
    
    // Unload until target met
    size_t freed = 0;
    for (const auto& pair : loaded) {
        if (freed >= targetBytes) break;
        
        unloadResource(pair.first);
        freed += pair.second->estimatedSize;
    }
}

void ResourcePrefetchingSystem::unloadResource(const std::string& resourcePath) {
    auto it = m_resources.find(resourcePath);
    if (it != m_resources.end() && it->second.state == LoadState::LOADED) {
        // Free resource
        if (it->second.resourcePtr) {
            // delete static_cast<Resource*>(it->second.resourcePtr);
            it->second.resourcePtr = nullptr;
        }
        it->second.state = LoadState::UNLOADED;
    }
}

void ResourcePrefetchingSystem::clearCache() {
    for (auto& pair : m_resources) {
        if (pair.second.state == LoadState::LOADED && pair.second.resourcePtr) {
            // delete static_cast<Resource*>(pair.second.resourcePtr);
        }
    }
    m_resources.clear();
}

ResourcePrefetchingSystem::Stats ResourcePrefetchingSystem::getStatistics() const {
    m_stats.memoryUsed = getCurrentMemoryUsage();
    m_stats.memoryAvailable = m_maxMemoryBytes - m_stats.memoryUsed;
    
    if (m_stats.successfulPrefetches + m_stats.cacheMisses > 0) {
        m_stats.cacheHitRate = static_cast<float>(m_stats.successfulPrefetches) /
                              (m_stats.successfulPrefetches + m_stats.cacheMisses);
    }
    
    return m_stats;
}

void ResourcePrefetchingSystem::resetStatistics() {
    m_stats = Stats();
}

void ResourcePrefetchingSystem::printResourceState() const {
    std::cout << "=== Resource Prefetch State ===" << std::endl;
    std::cout << "Total resources tracked: " << m_resources.size() << std::endl;
    
    int loaded = 0, loading = 0, queued = 0;
    for (const auto& pair : m_resources) {
        if (pair.second.state == LoadState::LOADED) loaded++;
        else if (pair.second.state == LoadState::LOADING) loading++;
        else if (pair.second.state == LoadState::QUEUED) queued++;
    }
    
    std::cout << "Loaded: " << loaded << ", Loading: " << loading << ", Queued: " << queued << std::endl;
    std::cout << "Memory: " << (getCurrentMemoryUsage() / 1024 / 1024) << " MB / "
              << (m_maxMemoryBytes / 1024 / 1024) << " MB" << std::endl;
}

std::vector<std::string> ResourcePrefetchingSystem::getQueuedResources() const {
    std::vector<std::string> result;
    for (const auto& pair : m_resources) {
        if (pair.second.state == LoadState::QUEUED) {
            result.push_back(pair.first);
        }
    }
    return result;
}

std::vector<std::string> ResourcePrefetchingSystem::getLoadedResources() const {
    std::vector<std::string> result;
    for (const auto& pair : m_resources) {
        if (pair.second.state == LoadState::LOADED) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void ResourcePrefetchingSystem::gatherPrefetchHints() {
    std::vector<PrefetchHint> allHints;
    
    // Gather hints from all enabled strategies
    for (const auto& strategy : m_strategies) {
        if (m_disabledStrategies.find(strategy->getName()) != m_disabledStrategies.end()) {
            continue;
        }
        
        auto hints = strategy->generateHints(nullptr);
        allHints.insert(allHints.end(), hints.begin(), hints.end());
    }
    
    m_stats.totalPrefetchHints += static_cast<int>(allHints.size());
    
    // Queue prefetch requests
    for (const auto& hint : allHints) {
        if (hint.probability > 0.3f) {  // Threshold
            prefetch(hint.resourcePath, hint.priority);
        }
    }
}

void ResourcePrefetchingSystem::processLoadQueue() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    // Start new loads if slots available
    while (m_activeLoads.size() < static_cast<size_t>(m_maxConcurrentLoads) &&
           !m_loadQueue.empty()) {
        LoadRequest request = m_loadQueue.top();
        m_loadQueue.pop();
        
        // Update state
        auto it = m_resources.find(request.resourcePath);
        if (it != m_resources.end()) {
            it->second.state = LoadState::LOADING;
        }
        
        m_activeLoads.push_back(request);
    }
}

void ResourcePrefetchingSystem::workerThreadFunction() {
    while (!m_shutdownRequested) {
        LoadRequest request("", LoadPriority::MEDIUM);
        
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCondition.wait(lock, [this] {
                return !m_activeLoads.empty() || m_shutdownRequested;
            });
            
            if (m_shutdownRequested) break;
            
            if (!m_activeLoads.empty()) {
                request = m_activeLoads.back();
                m_activeLoads.pop_back();
            } else {
                continue;
            }
        }
        
        // Load resource (outside lock)
        Resource* resource = loadResourceSync(request.resourcePath);
        
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            auto it = m_resources.find(request.resourcePath);
            if (it != m_resources.end()) {
                if (resource) {
                    it->second.state = LoadState::LOADED;
                    it->second.resourcePtr = resource;
                    m_stats.successfulPrefetches++;
                } else {
                    it->second.state = LoadState::FAILED;
                }
            }
        }
        
        // Trigger callbacks
        triggerCallbacks(request.resourcePath, resource);
    }
}

Resource* ResourcePrefetchingSystem::loadResourceSync(const std::string& path) {
    // TODO: Implement actual resource loading
    // This is a stub
    return nullptr;
}

void ResourcePrefetchingSystem::evictIfNeeded() {
    size_t currentUsage = getCurrentMemoryUsage();
    if (currentUsage > m_maxMemoryBytes * m_memoryPressureThreshold) {
        size_t targetFree = currentUsage - m_maxMemoryBytes * 0.7f;
        unloadLeastRecentlyUsed(targetFree);
    }
}

float ResourcePrefetchingSystem::calculateLoadPriority(const ResourceInfo& info) const {
    float priority = static_cast<float>(info.priority);
    priority -= info.distanceToPlayer * 0.01f;  // Closer = higher priority
    priority += info.accessCount * 0.1f;        // More used = higher priority
    return priority;
}

void ResourcePrefetchingSystem::updateResourceMetadata(const std::string& path) {
    // Update timestamps, distances, etc.
}

void ResourcePrefetchingSystem::triggerCallbacks(const std::string& path, Resource* resource) {
    auto it = m_loadCallbacks.find(path);
    if (it != m_loadCallbacks.end()) {
        for (auto& callback : it->second) {
            callback(resource);
        }
        m_loadCallbacks.erase(it);
    }
}

// PrefetchManager implementation
PrefetchManager& PrefetchManager::getInstance() {
    static PrefetchManager instance;
    return instance;
}

PrefetchManager::PrefetchManager() {
    m_system = std::make_unique<ResourcePrefetchingSystem>();
    m_system->initialize(512 * 1024 * 1024);  // 512 MB default
}

PrefetchManager::~PrefetchManager() {
    m_system->shutdown();
}

void PrefetchManager::prefetch(const std::string& path, LoadPriority priority) {
    getInstance().getSystem()->prefetch(path, priority);
}

bool PrefetchManager::isLoaded(const std::string& path) {
    return getInstance().getSystem()->isLoaded(path);
}

Resource* PrefetchManager::get(const std::string& path) {
    return getInstance().getSystem()->getResource(path);
}

} // namespace Core
} // namespace JJM
