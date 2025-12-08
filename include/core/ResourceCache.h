#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <list>
#include <functional>
#include <chrono>

namespace JJM {
namespace Core {

template<typename T>
struct CacheEntry {
    std::shared_ptr<T> resource;
    size_t size;
    std::chrono::steady_clock::time_point lastAccessed;
    int referenceCount;
    
    CacheEntry() : size(0), referenceCount(0) {}
};

enum class EvictionPolicy {
    LRU,
    LFU,
    FIFO
};

template<typename T>
class ResourceCache {
public:
    ResourceCache(size_t maxSizeBytes, EvictionPolicy policy = EvictionPolicy::LRU);
    ~ResourceCache();
    
    std::shared_ptr<T> get(const std::string& key);
    void put(const std::string& key, std::shared_ptr<T> resource, size_t size);
    
    bool contains(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    size_t getSize() const { return currentSize; }
    size_t getMaxSize() const { return maxSize; }
    size_t getCount() const { return cache.size(); }
    
    void setMaxSize(size_t maxSizeBytes) { maxSize = maxSizeBytes; }
    void setEvictionPolicy(EvictionPolicy policy) { this->policy = policy; }
    
    void setLoadFunction(std::function<std::shared_ptr<T>(const std::string&)> loader) {
        loadFunction = loader;
    }

private:
    std::unordered_map<std::string, CacheEntry<T>> cache;
    std::list<std::string> accessOrder;
    
    size_t maxSize;
    size_t currentSize;
    EvictionPolicy policy;
    
    std::function<std::shared_ptr<T>(const std::string&)> loadFunction;
    
    void evict();
    void evictLRU();
    void evictLFU();
    void evictFIFO();
    
    void updateAccess(const std::string& key);
};

template<typename T>
ResourceCache<T>::ResourceCache(size_t maxSizeBytes, EvictionPolicy policy)
    : maxSize(maxSizeBytes), currentSize(0), policy(policy) {}

template<typename T>
ResourceCache<T>::~ResourceCache() {
    clear();
}

template<typename T>
std::shared_ptr<T> ResourceCache<T>::get(const std::string& key) {
    auto it = cache.find(key);
    
    if (it != cache.end()) {
        updateAccess(key);
        return it->second.resource;
    }
    
    if (loadFunction) {
        auto resource = loadFunction(key);
        if (resource) {
            put(key, resource, sizeof(T));
            return resource;
        }
    }
    
    return nullptr;
}

template<typename T>
void ResourceCache<T>::put(const std::string& key, std::shared_ptr<T> resource, size_t size) {
    if (contains(key)) {
        remove(key);
    }
    
    while (currentSize + size > maxSize && !cache.empty()) {
        evict();
    }
    
    if (currentSize + size <= maxSize) {
        CacheEntry<T> entry;
        entry.resource = resource;
        entry.size = size;
        entry.lastAccessed = std::chrono::steady_clock::now();
        entry.referenceCount = 1;
        
        cache[key] = entry;
        accessOrder.push_back(key);
        currentSize += size;
    }
}

template<typename T>
bool ResourceCache<T>::contains(const std::string& key) const {
    return cache.find(key) != cache.end();
}

template<typename T>
void ResourceCache<T>::remove(const std::string& key) {
    auto it = cache.find(key);
    if (it != cache.end()) {
        currentSize -= it->second.size;
        cache.erase(it);
        
        accessOrder.remove(key);
    }
}

template<typename T>
void ResourceCache<T>::clear() {
    cache.clear();
    accessOrder.clear();
    currentSize = 0;
}

template<typename T>
void ResourceCache<T>::evict() {
    switch (policy) {
        case EvictionPolicy::LRU:
            evictLRU();
            break;
        case EvictionPolicy::LFU:
            evictLFU();
            break;
        case EvictionPolicy::FIFO:
            evictFIFO();
            break;
    }
}

template<typename T>
void ResourceCache<T>::evictLRU() {
    if (!accessOrder.empty()) {
        std::string key = accessOrder.front();
        remove(key);
    }
}

template<typename T>
void ResourceCache<T>::evictLFU() {
    if (cache.empty()) return;
    
    auto minIt = cache.begin();
    for (auto it = cache.begin(); it != cache.end(); ++it) {
        if (it->second.referenceCount < minIt->second.referenceCount) {
            minIt = it;
        }
    }
    
    remove(minIt->first);
}

template<typename T>
void ResourceCache<T>::evictFIFO() {
    if (!accessOrder.empty()) {
        std::string key = accessOrder.front();
        remove(key);
    }
}

template<typename T>
void ResourceCache<T>::updateAccess(const std::string& key) {
    auto it = cache.find(key);
    if (it != cache.end()) {
        it->second.lastAccessed = std::chrono::steady_clock::now();
        it->second.referenceCount++;
        
        if (policy == EvictionPolicy::LRU) {
            accessOrder.remove(key);
            accessOrder.push_back(key);
        }
    }
}

} // namespace Core
} // namespace JJM
