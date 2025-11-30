#include "streaming/AssetStreaming.h"
#include <algorithm>
#include <iostream>
#include <random>
#include <cassert>
#include <fstream>
#include <chrono>

namespace JJM {
namespace Streaming {

// -- AssetHandle implementation
AssetHandle::AssetHandle(std::shared_ptr<Asset> asset) : asset(asset) {
    if (auto shared = this->asset.lock()) {
        shared->addReference();
    }
}

AssetHandle::~AssetHandle() {
    if (auto shared = asset.lock()) {
        shared->removeReference();
    }
}

AssetHandle::AssetHandle(const AssetHandle& other) : asset(other.asset) {
    if (auto shared = asset.lock()) {
        shared->addReference();
    }
}

AssetHandle& AssetHandle::operator=(const AssetHandle& other) {
    if (this != &other) {
        // Release old reference
        if (auto oldShared = asset.lock()) {
            oldShared->removeReference();
        }
        
        // Acquire new reference
        asset = other.asset;
        if (auto newShared = asset.lock()) {
            newShared->addReference();
        }
    }
    return *this;
}

AssetHandle::AssetHandle(AssetHandle&& other) noexcept : asset(std::move(other.asset)) {
    // Move doesn't change reference count
}

AssetHandle& AssetHandle::operator=(AssetHandle&& other) noexcept {
    if (this != &other) {
        // Release old reference
        if (auto oldShared = asset.lock()) {
            oldShared->removeReference();
        }
        
        asset = std::move(other.asset);
    }
    return *this;
}

std::shared_ptr<Asset> AssetHandle::get() const {
    std::lock_guard<std::mutex> lock(handleMutex);
    if (auto shared = asset.lock()) {
        shared->markAccessed();
        return shared;
    }
    return nullptr;
}

bool AssetHandle::isValid() const {
    return !asset.expired();
}

bool AssetHandle::isLoaded() const {
    if (auto shared = asset.lock()) {
        return shared->getState() == AssetState::Loaded;
    }
    return false;
}

AssetState AssetHandle::getState() const {
    if (auto shared = asset.lock()) {
        return shared->getState();
    }
    return AssetState::Unloaded;
}

AssetLOD AssetHandle::getCurrentLOD() const {
    if (auto shared = asset.lock()) {
        return shared->getCurrentLOD();
    }
    return AssetLOD{};
}

// -- MemoryBudget implementation
MemoryBudget::MemoryBudget(size_t totalBudget) : totalBudget(totalBudget), usedMemory(0) {
    stats = Statistics{};
}

void MemoryBudget::setTotalBudget(size_t budget) {
    std::lock_guard<std::mutex> lock(budgetMutex);
    totalBudget = budget;
}

size_t MemoryBudget::getAvailableMemory() const {
    size_t used = usedMemory.load();
    size_t total = totalBudget.load();
    return (used < total) ? (total - used) : 0;
}

float MemoryBudget::getUsageRatio() const {
    size_t used = usedMemory.load();
    size_t total = totalBudget.load();
    return (total > 0) ? static_cast<float>(used) / static_cast<float>(total) : 0.0f;
}

bool MemoryBudget::canAllocate(size_t size) const {
    return getAvailableMemory() >= size;
}

bool MemoryBudget::allocate(size_t size) {
    auto start = std::chrono::high_resolution_clock::now();
    
    if (!canAllocate(size)) {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.failedAllocations++;
        return false;
    }
    
    usedMemory += size;
    
    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.totalAllocations++;
        size_t current = usedMemory.load();
        if (current > stats.peakUsage) {
            stats.peakUsage = current;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        stats.totalAllocationTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }
    
    return true;
}

void MemoryBudget::deallocate(size_t size) {
    size_t current = usedMemory.load();
    if (current >= size) {
        usedMemory -= size;
    } else {
        usedMemory = 0;  // Prevent underflow
    }
    
    std::lock_guard<std::mutex> statsLock(statsMutex);
    stats.totalDeallocations++;
}

void MemoryBudget::setCategoryBudget(const std::string& category, size_t budget) {
    std::lock_guard<std::mutex> lock(budgetMutex);
    categoryBudgets[category] = budget;
}

size_t MemoryBudget::getCategoryBudget(const std::string& category) const {
    std::lock_guard<std::mutex> lock(budgetMutex);
    auto it = categoryBudgets.find(category);
    return (it != categoryBudgets.end()) ? it->second : 0;
}

size_t MemoryBudget::getCategoryUsed(const std::string& category) const {
    std::lock_guard<std::mutex> lock(budgetMutex);
    auto it = categoryUsage.find(category);
    return (it != categoryUsage.end()) ? it->second : 0;
}

bool MemoryBudget::allocateFromCategory(const std::string& category, size_t size) {
    std::lock_guard<std::mutex> lock(budgetMutex);
    
    auto budgetIt = categoryBudgets.find(category);
    if (budgetIt == categoryBudgets.end()) {
        return false;  // Category not found
    }
    
    size_t categoryBudget = budgetIt->second;
    size_t categoryUsed = categoryUsage[category];
    
    if (categoryUsed + size > categoryBudget) {
        return false;  // Category budget exceeded
    }
    
    if (!canAllocate(size)) {
        return false;  // Total budget exceeded
    }
    
    categoryUsage[category] += size;
    usedMemory += size;
    return true;
}

void MemoryBudget::deallocateFromCategory(const std::string& category, size_t size) {
    std::lock_guard<std::mutex> lock(budgetMutex);
    
    auto& categoryUsed = categoryUsage[category];
    if (categoryUsed >= size) {
        categoryUsed -= size;
    } else {
        categoryUsed = 0;
    }
    
    deallocate(size);
}

MemoryBudget::Statistics MemoryBudget::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void MemoryBudget::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = Statistics{};
}

// -- LRUEvictionPolicy implementation
std::vector<std::string> LRUEvictionPolicy::selectAssetsForEviction(
    const std::unordered_map<std::string, std::shared_ptr<Asset>>& assets,
    size_t targetMemory) {
    
    std::lock_guard<std::mutex> lock(accessMutex);
    std::vector<std::string> candidates;
    
    // Create candidate list with access times
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>> candidatesWithTime;
    
    for (const auto& [id, asset] : assets) {
        if (asset && asset->canUnload()) {
            auto accessTime = asset->getLastAccessTime();
            candidatesWithTime.emplace_back(accessTime, id);
        }
    }
    
    // Sort by access time (oldest first)
    std::sort(candidatesWithTime.begin(), candidatesWithTime.end());
    
    // Select assets until we reach target memory
    size_t freedMemory = 0;
    for (const auto& [time, id] : candidatesWithTime) {
        auto assetIt = assets.find(id);
        if (assetIt != assets.end() && assetIt->second) {
            candidates.push_back(id);
            freedMemory += assetIt->second->getMemoryUsage();
            
            if (freedMemory >= targetMemory) {
                break;
            }
        }
    }
    
    return candidates;
}

void LRUEvictionPolicy::onAssetAccessed(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(accessMutex);
    accessTimes[assetId] = std::chrono::steady_clock::now();
}

void LRUEvictionPolicy::onAssetLoaded(const std::string& assetId) {
    onAssetAccessed(assetId);  // Loading counts as access
}

void LRUEvictionPolicy::onAssetUnloaded(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(accessMutex);
    accessTimes.erase(assetId);
}

// -- AssetCache implementation
AssetCache::AssetCache(size_t maxSize) : maxSize(maxSize) {
    stats = CacheStats{};
}

void AssetCache::put(const std::string& key, std::shared_ptr<Asset> asset) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    // Check if we need to evict
    if (cache.size() >= maxSize) {
        evictOldestEntries();
    }
    
    // Add or update entry
    CacheEntry entry;
    entry.asset = asset;
    entry.accessTime = std::chrono::steady_clock::now();
    entry.accessCount = 1;
    
    cache[key] = entry;
}

std::shared_ptr<Asset> AssetCache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    
    auto it = cache.find(key);
    if (it != cache.end()) {
        // Update access info
        it->second.accessTime = std::chrono::steady_clock::now();
        it->second.accessCount++;
        
        stats.hits++;
        stats.hitRate = static_cast<float>(stats.hits) / (stats.hits + stats.misses);
        
        return it->second.asset;
    }
    
    stats.misses++;
    stats.hitRate = static_cast<float>(stats.hits) / (stats.hits + stats.misses);
    
    return nullptr;
}

bool AssetCache::contains(const std::string& key) const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.find(key) != cache.end();
}

void AssetCache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.erase(key);
}

void AssetCache::clear() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();
}

void AssetCache::setMaxSize(size_t newMaxSize) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    maxSize = newMaxSize;
    
    // Evict excess entries if needed
    while (cache.size() > maxSize) {
        evictOldestEntries();
    }
}

size_t AssetCache::getCurrentSize() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return cache.size();
}

AssetCache::CacheStats AssetCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    return stats;
}

void AssetCache::resetStatistics() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    stats = CacheStats{};
}

void AssetCache::evictOldestEntries(size_t count) {
    // Find oldest entries by access time
    std::vector<std::pair<std::chrono::steady_clock::time_point, std::string>> entries;
    
    for (const auto& [key, entry] : cache) {
        entries.emplace_back(entry.accessTime, key);
    }
    
    // Sort by access time (oldest first)
    std::sort(entries.begin(), entries.end());
    
    // Remove the oldest entries
    size_t removed = 0;
    for (const auto& [time, key] : entries) {
        if (removed >= count) break;
        
        cache.erase(key);
        stats.evictions++;
        removed++;
    }
}

// -- LoadingWorker implementation
LoadingWorker::LoadingWorker(StreamingManager* manager, uint32_t workerId)
    : manager(manager), workerId(workerId) {
    stats = WorkerStats{};
}

LoadingWorker::~LoadingWorker() {
    stop();
}

void LoadingWorker::start() {
    if (!running.load()) {
        running = true;
        workerThread = std::thread(&LoadingWorker::workerLoop, this);
    }
}

void LoadingWorker::stop() {
    if (running.load()) {
        running = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
}

LoadingWorker::WorkerStats LoadingWorker::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void LoadingWorker::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = WorkerStats{};
}

void LoadingWorker::workerLoop() {
    while (running.load()) {
        if (processNextRequest()) {
            std::lock_guard<std::mutex> lock(statsMutex);
            stats.lastActivity = std::chrono::steady_clock::now();
        } else {
            // No work available, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

bool LoadingWorker::processNextRequest() {
    if (!manager || !manager->hasQueuedRequests()) {
        return false;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        LoadRequest request = manager->getNextRequest();
        
        // Process the request
        bool success = manager->loadAssetInternal(request.assetId, request.requestedLOD);
        
        // Update statistics
        {
            std::lock_guard<std::mutex> lock(statsMutex);
            if (success) {
                stats.requestsProcessed++;
                if (request.onComplete) {
                    auto asset = manager->getAssetDirect(request.assetId);
                    request.onComplete(asset);
                }
            } else {
                stats.requestsFailed++;
                if (request.onError) {
                    request.onError("Failed to load asset: " + request.assetId);
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            stats.totalProcessingTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(statsMutex);
        stats.requestsFailed++;
        std::cerr << "Worker " << workerId << " error: " << e.what() << std::endl;
        return false;
    }
}

// -- StreamingManager implementation
StreamingManager::StreamingManager() : memoryBudget(), assetCache(100) {
    evictionPolicy = std::make_unique<LRUEvictionPolicy>();
    stats = StreamingStats{};
}

StreamingManager::~StreamingManager() {
    shutdown();
}

bool StreamingManager::initialize(size_t workerCount, size_t memoryBudgetSize) {
    if (initialized.load()) {
        return true;
    }
    
    memoryBudget.setTotalBudget(memoryBudgetSize);
    maxConcurrentLoads = workerCount;
    
    // Create worker threads
    workers.clear();
    for (size_t i = 0; i < workerCount; ++i) {
        workers.push_back(std::make_unique<LoadingWorker>(this, static_cast<uint32_t>(i)));
    }
    
    startWorkers();
    initialized = true;
    
    return true;
}

void StreamingManager::shutdown() {
    if (!initialized.load()) {
        return;
    }
    
    shutdownRequested = true;
    
    // Stop workers
    stopWorkers();
    
    // Clear all assets
    {
        std::lock_guard<std::mutex> lock(assetMutex);
        loadedAssets.clear();
        assetRegistry.clear();
        assetHandles.clear();
    }
    
    // Clear queue
    clearQueue();
    
    initialized = false;
    shutdownRequested = false;
}

void StreamingManager::registerLoader(std::shared_ptr<IAssetLoader> loader) {
    loaders.push_back(loader);
}

void StreamingManager::unregisterLoader(std::shared_ptr<IAssetLoader> loader) {
    auto it = std::find(loaders.begin(), loaders.end(), loader);
    if (it != loaders.end()) {
        loaders.erase(it);
    }
}

bool StreamingManager::registerAsset(const AssetMetadata& metadata) {
    if (!validateAssetMetadata(metadata)) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(assetMutex);
    assetRegistry[metadata.id] = metadata;
    
    std::lock_guard<std::mutex> statsLock(statsMutex);
    stats.totalAssetsRegistered++;
    
    return true;
}

void StreamingManager::unregisterAsset(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(assetMutex);
    
    // Unload if loaded
    auto loadedIt = loadedAssets.find(assetId);
    if (loadedIt != loadedAssets.end()) {
        unloadAssetInternal(assetId);
    }
    
    // Remove from registry
    assetRegistry.erase(assetId);
}

bool StreamingManager::isAssetRegistered(const std::string& assetId) const {
    std::lock_guard<std::mutex> lock(assetMutex);
    return assetRegistry.find(assetId) != assetRegistry.end();
}

AssetHandle StreamingManager::loadAsset(const std::string& assetId, AssetPriority priority) {
    // Check if already loaded
    {
        std::lock_guard<std::mutex> lock(assetMutex);
        auto it = loadedAssets.find(assetId);
        if (it != loadedAssets.end() && it->second->getState() == AssetState::Loaded) {
            return AssetHandle(it->second);
        }
    }
    
    // Load synchronously
    auto metadata = assetRegistry.find(assetId);
    if (metadata != assetRegistry.end()) {
        AssetLOD defaultLOD = metadata->second.lodLevels.empty() ? 
            AssetLOD{} : metadata->second.lodLevels[0];
        
        if (loadAssetInternal(assetId, defaultLOD)) {
            std::lock_guard<std::mutex> lock(assetMutex);
            auto it = loadedAssets.find(assetId);
            if (it != loadedAssets.end()) {
                return AssetHandle(it->second);
            }
        }
    }
    
    return AssetHandle{};  // Return invalid handle on failure
}

AssetHandle StreamingManager::loadAssetAsync(const std::string& assetId, AssetPriority priority) {
    // Create load request
    LoadRequest request;
    request.assetId = assetId;
    request.priority = priority;
    
    auto metadataIt = assetRegistry.find(assetId);
    if (metadataIt != assetRegistry.end()) {
        request.requestedLOD = metadataIt->second.lodLevels.empty() ? 
            AssetLOD{} : metadataIt->second.lodLevels[0];
    }
    
    addLoadRequest(request);
    
    // Return handle that will be valid once loading completes
    std::lock_guard<std::mutex> lock(assetMutex);
    auto handleIt = assetHandles.find(assetId);
    if (handleIt != assetHandles.end()) {
        if (auto shared = handleIt->second.lock()) {
            return AssetHandle(shared);
        }
    }
    
    // Create placeholder asset for the handle
    // In a real implementation, you'd have proper placeholder/proxy objects
    return AssetHandle{};
}

std::future<AssetHandle> StreamingManager::loadAssetWithFuture(const std::string& assetId, AssetPriority priority) {
    auto promise = std::make_shared<std::promise<AssetHandle>>();
    auto future = promise->get_future();
    
    LoadRequest request;
    request.assetId = assetId;
    request.priority = priority;
    request.onComplete = [promise](std::shared_ptr<Asset> asset) {
        promise->set_value(AssetHandle(asset));
    };
    request.onError = [promise](const std::string& error) {
        promise->set_exception(std::make_exception_ptr(std::runtime_error(error)));
    };
    
    addLoadRequest(request);
    
    return future;
}

std::vector<AssetHandle> StreamingManager::loadAssets(const std::vector<std::string>& assetIds, AssetPriority priority) {
    std::vector<AssetHandle> handles;
    handles.reserve(assetIds.size());
    
    for (const auto& assetId : assetIds) {
        handles.push_back(loadAssetAsync(assetId, priority));
    }
    
    return handles;
}

AssetHandle StreamingManager::getAsset(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(assetMutex);
    auto it = loadedAssets.find(assetId);
    if (it != loadedAssets.end()) {
        return AssetHandle(it->second);
    }
    return AssetHandle{};
}

std::shared_ptr<Asset> StreamingManager::getAssetDirect(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(assetMutex);
    auto it = loadedAssets.find(assetId);
    return (it != loadedAssets.end()) ? it->second : nullptr;
}

bool StreamingManager::isAssetLoaded(const std::string& assetId) const {
    std::lock_guard<std::mutex> lock(assetMutex);
    auto it = loadedAssets.find(assetId);
    return it != loadedAssets.end() && it->second->getState() == AssetState::Loaded;
}

AssetState StreamingManager::getAssetState(const std::string& assetId) const {
    std::lock_guard<std::mutex> lock(assetMutex);
    auto it = loadedAssets.find(assetId);
    return (it != loadedAssets.end()) ? it->second->getState() : AssetState::Unloaded;
}

void StreamingManager::addLoadRequest(const LoadRequest& request) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        loadQueue.push(request);
        
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.totalLoadRequests++;
        stats.queueSize = loadQueue.size();
    }
    
    queueCondition.notify_one();
}

LoadRequest StreamingManager::getNextRequest() {
    std::unique_lock<std::mutex> lock(queueMutex);
    queueCondition.wait(lock, [this] { return !loadQueue.empty() || shutdownRequested.load(); });
    
    if (!loadQueue.empty()) {
        LoadRequest request = loadQueue.top();
        loadQueue.pop();
        
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.queueSize = loadQueue.size();
        
        return request;
    }
    
    return LoadRequest{};  // Return empty request if shutting down
}

bool StreamingManager::hasQueuedRequests() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return !loadQueue.empty();
}

size_t StreamingManager::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return loadQueue.size();
}

void StreamingManager::clearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!loadQueue.empty()) {
        loadQueue.pop();
    }
    
    std::lock_guard<std::mutex> statsLock(statsMutex);
    stats.queueSize = 0;
}

void StreamingManager::update(float deltaTime) {
    // Update cache
    updateCache();
    
    // Check for eviction needs
    if (memoryBudget.getUsageRatio() > 0.9f) {  // Evict when over 90% full
        performEviction();
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(assetMutex);
        std::lock_guard<std::mutex> statsLock(statsMutex);
        
        stats.assetsLoaded = 0;
        stats.assetsLoading = 0;
        stats.assetsFailed = 0;
        
        for (const auto& [id, asset] : loadedAssets) {
            switch (asset->getState()) {
                case AssetState::Loaded:
                    stats.assetsLoaded++;
                    break;
                case AssetState::Loading:
                    stats.assetsLoading++;
                    break;
                case AssetState::Failed:
                    stats.assetsFailed++;
                    break;
                default:
                    break;
            }
        }
        
        stats.memoryUsed = memoryBudget.getUsedMemory();
        stats.memoryBudget = memoryBudget.getTotalBudget();
        stats.queueSize = getQueueSize();
    }
}

void StreamingManager::setEvictionPolicy(std::unique_ptr<IEvictionPolicy> policy) {
    evictionPolicy = std::move(policy);
}

void StreamingManager::triggerEviction(size_t targetMemory) {
    if (targetMemory == 0) {
        targetMemory = memoryBudget.getTotalBudget() * 0.3f;  // Free 30% by default
    }
    
    performEviction();
}

void StreamingManager::setMaxConcurrentLoads(size_t maxLoads) {
    maxConcurrentLoads = maxLoads;
}

StreamingManager::StreamingStats StreamingManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return stats;
}

void StreamingManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex);
    stats = StreamingStats{};
}

std::vector<std::string> StreamingManager::getLoadedAssetIds() const {
    std::lock_guard<std::mutex> lock(assetMutex);
    std::vector<std::string> ids;
    ids.reserve(loadedAssets.size());
    
    for (const auto& [id, asset] : loadedAssets) {
        if (asset->getState() == AssetState::Loaded) {
            ids.push_back(id);
        }
    }
    
    return ids;
}

void StreamingManager::dumpStatistics() const {
    auto stats = getStatistics();
    
    std::cout << "\n=== Asset Streaming Statistics ===" << std::endl;
    std::cout << "Total Assets Registered: " << stats.totalAssetsRegistered << std::endl;
    std::cout << "Assets Loaded: " << stats.assetsLoaded << std::endl;
    std::cout << "Assets Loading: " << stats.assetsLoading << std::endl;
    std::cout << "Assets Failed: " << stats.assetsFailed << std::endl;
    std::cout << "Queue Size: " << stats.queueSize << std::endl;
    std::cout << "Memory Used: " << stats.memoryUsed << " / " << stats.memoryBudget << std::endl;
    std::cout << "Memory Usage: " << (stats.memoryBudget > 0 ? 
        (float(stats.memoryUsed) / stats.memoryBudget * 100.0f) : 0.0f) << "%" << std::endl;
    std::cout << "Average Load Time: " << stats.averageLoadTime << "ms" << std::endl;
    std::cout << "=================================\n" << std::endl;
}

// Internal helper methods
IAssetLoader* StreamingManager::findCompatibleLoader(const AssetMetadata& metadata) {
    for (auto& loader : loaders) {
        if (loader->canLoad(std::filesystem::path(metadata.path))) {
            return loader.get();
        }
    }
    return nullptr;
}

bool StreamingManager::loadAssetInternal(const std::string& assetId, const AssetLOD& lod) {
    // Find asset metadata
    auto metadataIt = assetRegistry.find(assetId);
    if (metadataIt == assetRegistry.end()) {
        return false;
    }
    
    const AssetMetadata& metadata = metadataIt->second;
    
    // Find compatible loader
    IAssetLoader* loader = findCompatibleLoader(metadata);
    if (!loader) {
        return false;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Check memory budget
    if (!memoryBudget.canAllocate(metadata.estimatedSize)) {
        performEviction();
        if (!memoryBudget.canAllocate(metadata.estimatedSize)) {
            return false;  // Still can't allocate after eviction
        }
    }
    
    // Create or get asset
    std::shared_ptr<Asset> asset;
    {
        std::lock_guard<std::mutex> lock(assetMutex);
        auto loadedIt = loadedAssets.find(assetId);
        if (loadedIt != loadedAssets.end()) {
            asset = loadedIt->second;
        } else {
            asset = loader->createAsset(metadata);
            if (!asset) {
                return false;
            }
            loadedAssets[assetId] = asset;
            assetHandles[assetId] = asset;
        }
    }
    
    // Set loading state
    asset->setState(AssetState::Loading);
    
    // Allocate memory
    if (!memoryBudget.allocate(metadata.estimatedSize)) {
        asset->setState(AssetState::Failed);
        return false;
    }
    
    // Load the asset
    bool success = loader->loadAsset(asset, std::filesystem::path(metadata.path), lod);
    
    if (success) {
        asset->setState(AssetState::Loaded);
        asset->setMemoryUsage(asset->calculateMemoryUsage());
        
        // Add to cache
        assetCache.put(assetId, asset);
        
        // Notify eviction policy
        if (evictionPolicy) {
            evictionPolicy->onAssetLoaded(assetId);
        }
    } else {
        asset->setState(AssetState::Failed);
        memoryBudget.deallocate(metadata.estimatedSize);
        
        // Remove from loaded assets
        std::lock_guard<std::mutex> lock(assetMutex);
        loadedAssets.erase(assetId);
    }
    
    // Update statistics
    auto end = std::chrono::high_resolution_clock::now();
    auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.totalLoadTime += loadTime;
        if (success) {
            float newAverage = static_cast<float>(stats.totalLoadTime.count()) / ++stats.assetsLoaded;
            stats.averageLoadTime = newAverage;
        }
    }
    
    return success;
}

void StreamingManager::unloadAssetInternal(const std::string& assetId) {
    std::lock_guard<std::mutex> lock(assetMutex);
    
    auto it = loadedAssets.find(assetId);
    if (it != loadedAssets.end()) {
        auto& asset = it->second;
        
        // Set unloading state
        asset->setState(AssetState::Unloading);
        
        // Unload the asset
        size_t memoryUsage = asset->getMemoryUsage();
        asset->unload();
        
        // Deallocate memory
        memoryBudget.deallocate(memoryUsage);
        
        // Remove from cache
        assetCache.remove(assetId);
        
        // Notify eviction policy
        if (evictionPolicy) {
            evictionPolicy->onAssetUnloaded(assetId);
        }
        
        // Remove from loaded assets
        loadedAssets.erase(it);
    }
}

bool StreamingManager::validateAssetMetadata(const AssetMetadata& metadata) const {
    if (metadata.id.empty() || metadata.path.empty() || metadata.type.empty()) {
        return false;
    }
    
    // Check if file exists
    if (!std::filesystem::exists(metadata.path)) {
        return false;
    }
    
    return true;
}

void StreamingManager::startWorkers() {
    for (auto& worker : workers) {
        worker->start();
    }
}

void StreamingManager::stopWorkers() {
    queueCondition.notify_all();  // Wake up all workers
    
    for (auto& worker : workers) {
        worker->stop();
    }
}

void StreamingManager::updateCache() {
    // Cache maintenance operations
    // This could include cleaning up expired weak references, etc.
}

void StreamingManager::performEviction() {
    if (!evictionPolicy) {
        return;
    }
    
    size_t targetMemory = memoryBudget.getTotalBudget() * 0.3f;  // Free 30%
    
    std::lock_guard<std::mutex> lock(assetMutex);
    auto assetsToEvict = evictionPolicy->selectAssetsForEviction(loadedAssets, targetMemory);
    
    for (const auto& assetId : assetsToEvict) {
        unloadAssetInternal(assetId);
        
        std::lock_guard<std::mutex> statsLock(statsMutex);
        stats.evictionCount++;
    }
}

// -- AssetUtils namespace implementation
namespace AssetUtils {

std::string generateAssetId(const std::filesystem::path& path) {
    // Simple hash-based ID generation
    std::hash<std::string> hasher;
    return std::to_string(hasher(path.string()));
}

std::string getAssetTypeFromPath(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga") {
        return "texture";
    } else if (extension == ".obj" || extension == ".fbx" || extension == ".dae" || extension == ".gltf") {
        return "mesh";
    } else if (extension == ".wav" || extension == ".mp3" || extension == ".ogg" || extension == ".flac") {
        return "audio";
    } else if (extension == ".ttf" || extension == ".otf") {
        return "font";
    } else {
        return "unknown";
    }
}

std::filesystem::path getLODPath(const std::filesystem::path& basePath, const AssetLOD& lod) {
    if (lod.suffix.empty()) {
        return basePath;
    }
    
    auto stem = basePath.stem();
    auto extension = basePath.extension();
    auto parent = basePath.parent_path();
    
    return parent / (stem.string() + lod.suffix + extension.string());
}

size_t estimateTextureSize(uint32_t width, uint32_t height, uint32_t channels, uint32_t bytesPerChannel) {
    return width * height * channels * bytesPerChannel;
}

size_t estimateMeshSize(uint32_t vertexCount, uint32_t indexCount, bool hasNormals, bool hasUVs) {
    size_t vertexSize = 3 * sizeof(float);  // Position
    if (hasNormals) vertexSize += 3 * sizeof(float);
    if (hasUVs) vertexSize += 2 * sizeof(float);
    
    return (vertexCount * vertexSize) + (indexCount * sizeof(uint32_t));
}

size_t estimateAudioSize(uint32_t sampleRate, uint32_t channels, float durationSeconds, uint32_t bitsPerSample) {
    return static_cast<size_t>(sampleRate * channels * durationSeconds * (bitsPerSample / 8));
}

AssetLOD calculateLODFromDistance(float distance, const std::vector<AssetLOD>& lodLevels) {
    if (lodLevels.empty()) {
        return AssetLOD{};
    }
    
    // Find the appropriate LOD level based on distance
    for (const auto& lod : lodLevels) {
        if (distance >= lod.distance) {
            return lod;
        }
    }
    
    // Return the highest quality LOD if distance is very close
    return lodLevels[0];
}

float calculateOptimalLODBias(float memoryPressure, float performanceTarget) {
    // Simple heuristic: increase LOD bias (lower quality) when memory pressure is high
    // or performance target is not being met
    float bias = 0.0f;
    
    if (memoryPressure > 0.8f) {
        bias += (memoryPressure - 0.8f) * 5.0f;  // Increase bias for high memory pressure
    }
    
    if (performanceTarget < 0.6f) {  // Below 60% of target performance
        bias += (0.6f - performanceTarget) * 3.0f;
    }
    
    return std::clamp(bias, 0.0f, 3.0f);  // Clamp to reasonable range
}

AssetMetadata createTextureMetadata(const std::string& id, const std::filesystem::path& path, uint32_t width, uint32_t height) {
    AssetMetadata metadata;
    metadata.id = id;
    metadata.path = path.string();
    metadata.type = "texture";
    metadata.estimatedSize = estimateTextureSize(width, height, 4, 1);  // Assume RGBA, 1 byte per channel
    
    // Add default LOD levels for textures
    metadata.lodLevels.push_back(AssetLOD{0, 0.0f, 1.0f, ""});        // Full resolution
    metadata.lodLevels.push_back(AssetLOD{1, 50.0f, 0.5f, "_lod1"});  // Half resolution
    metadata.lodLevels.push_back(AssetLOD{2, 100.0f, 0.25f, "_lod2"}); // Quarter resolution
    
    return metadata;
}

AssetMetadata createMeshMetadata(const std::string& id, const std::filesystem::path& path, uint32_t vertexCount) {
    AssetMetadata metadata;
    metadata.id = id;
    metadata.path = path.string();
    metadata.type = "mesh";
    metadata.estimatedSize = estimateMeshSize(vertexCount, vertexCount * 3, true, true);  // Assume indexed with normals and UVs
    
    return metadata;
}

AssetMetadata createAudioMetadata(const std::string& id, const std::filesystem::path& path, float duration) {
    AssetMetadata metadata;
    metadata.id = id;
    metadata.path = path.string();
    metadata.type = "audio";
    metadata.estimatedSize = estimateAudioSize(44100, 2, duration, 16);  // CD quality stereo
    
    return metadata;
}

} // namespace AssetUtils

} // namespace Streaming
} // namespace JJM