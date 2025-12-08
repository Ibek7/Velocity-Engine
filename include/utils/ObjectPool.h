#pragma once

#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace Utils {

template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initialSize = 32, size_t maxSize = 1024);
    ~ObjectPool();
    
    template<typename... Args>
    T* acquire(Args&&... args);
    
    void release(T* object);
    
    void clear();
    void shrink();
    
    size_t getActiveCount() const { return activeCount; }
    size_t getPoolSize() const { return pool.size(); }
    size_t getMaxSize() const { return maxSize; }
    
    void setMaxSize(size_t max) { maxSize = max; }

private:
    struct PooledObject {
        T* object;
        bool active;
        
        PooledObject() : object(nullptr), active(false) {}
    };
    
    std::vector<PooledObject> pool;
    size_t activeCount;
    size_t maxSize;
    
    void grow();
};

template<typename T>
ObjectPool<T>::ObjectPool(size_t initialSize, size_t maxSize)
    : activeCount(0), maxSize(maxSize) {
    
    pool.reserve(initialSize);
    for (size_t i = 0; i < initialSize; ++i) {
        PooledObject obj;
        obj.object = new T();
        obj.active = false;
        pool.push_back(obj);
    }
}

template<typename T>
ObjectPool<T>::~ObjectPool() {
    clear();
}

template<typename T>
template<typename... Args>
T* ObjectPool<T>::acquire(Args&&... args) {
    for (auto& pooled : pool) {
        if (!pooled.active) {
            pooled.active = true;
            ++activeCount;
            
            *pooled.object = T(std::forward<Args>(args)...);
            
            return pooled.object;
        }
    }
    
    if (pool.size() < maxSize) {
        grow();
        return acquire(std::forward<Args>(args)...);
    }
    
    return nullptr;
}

template<typename T>
void ObjectPool<T>::release(T* object) {
    if (!object) return;
    
    for (auto& pooled : pool) {
        if (pooled.object == object && pooled.active) {
            pooled.active = false;
            --activeCount;
            return;
        }
    }
}

template<typename T>
void ObjectPool<T>::clear() {
    for (auto& pooled : pool) {
        if (pooled.object) {
            delete pooled.object;
            pooled.object = nullptr;
        }
    }
    pool.clear();
    activeCount = 0;
}

template<typename T>
void ObjectPool<T>::shrink() {
    std::vector<PooledObject> newPool;
    newPool.reserve(activeCount);
    
    for (auto& pooled : pool) {
        if (pooled.active) {
            newPool.push_back(pooled);
        } else {
            delete pooled.object;
        }
    }
    
    pool = std::move(newPool);
}

template<typename T>
void ObjectPool<T>::grow() {
    size_t newSize = std::min(pool.size() * 2, maxSize);
    size_t toAdd = newSize - pool.size();
    
    for (size_t i = 0; i < toAdd; ++i) {
        PooledObject obj;
        obj.object = new T();
        obj.active = false;
        pool.push_back(obj);
    }
}

template<typename T>
class PoolAllocator {
public:
    PoolAllocator(size_t blockSize, size_t blockCount);
    ~PoolAllocator();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const { return blockSize; }
    size_t getUsedCount() const { return usedCount; }
    size_t getTotalCount() const { return blockCount; }

private:
    size_t blockSize;
    size_t blockCount;
    size_t usedCount;
    
    void* memory;
    void** freeList;
    size_t freeCount;
};

template<typename T>
PoolAllocator<T>::PoolAllocator(size_t blockSize, size_t blockCount)
    : blockSize(blockSize), blockCount(blockCount), usedCount(0), freeCount(blockCount) {
    
    memory = ::operator new(blockSize * blockCount);
    freeList = new void*[blockCount];
    
    char* ptr = static_cast<char*>(memory);
    for (size_t i = 0; i < blockCount; ++i) {
        freeList[i] = ptr + i * blockSize;
    }
}

template<typename T>
PoolAllocator<T>::~PoolAllocator() {
    ::operator delete(memory);
    delete[] freeList;
}

template<typename T>
void* PoolAllocator<T>::allocate() {
    if (freeCount == 0) {
        return nullptr;
    }
    
    void* block = freeList[--freeCount];
    ++usedCount;
    return block;
}

template<typename T>
void PoolAllocator<T>::deallocate(void* ptr) {
    if (!ptr || freeCount >= blockCount) {
        return;
    }
    
    freeList[freeCount++] = ptr;
    --usedCount;
}

} // namespace Utils
} // namespace JJM
