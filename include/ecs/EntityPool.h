#pragma once

#include "ecs/Entity.h"
#include "ecs/EntityManager.h"
#include <vector>
#include <memory>
#include <functional>
#include <queue>

namespace JJM {
namespace ECS {

template<typename T>
class ObjectPool {
public:
    ObjectPool(size_t initialSize = 100);
    ~ObjectPool();
    
    T* acquire();
    void release(T* obj);
    
    void resize(size_t newSize);
    void clear();
    
    size_t getPoolSize() const { return pool.size(); }
    size_t getAvailableCount() const { return available.size(); }
    size_t getActiveCount() const { return pool.size() - available.size(); }

private:
    std::vector<std::unique_ptr<T>> pool;
    std::queue<T*> available;
    
    void grow(size_t count);
};

class EntityPool {
public:
    EntityPool(EntityManager* manager, size_t initialSize = 100);
    ~EntityPool();
    
    Entity* acquire();
    void release(Entity* entity);
    
    void resize(size_t newSize);
    void clear();
    
    size_t getPoolSize() const { return poolSize; }
    size_t getAvailableCount() const { return availableEntities.size(); }
    size_t getActiveCount() const { return poolSize - availableEntities.size(); }
    
    void setOnAcquire(std::function<void(Entity*)> callback) {
        onAcquire = callback;
    }
    
    void setOnRelease(std::function<void(Entity*)> callback) {
        onRelease = callback;
    }

private:
    EntityManager* manager;
    std::vector<Entity*> entities;
    std::queue<Entity*> availableEntities;
    size_t poolSize;
    
    std::function<void(Entity*)> onAcquire;
    std::function<void(Entity*)> onRelease;
    
    void grow(size_t count);
    void resetEntity(Entity* entity);
};

class ComponentPool {
public:
    ComponentPool(size_t initialSize = 100);
    ~ComponentPool();
    
    Component* acquire(const std::string& type);
    void release(Component* component);
    
    void registerComponentType(const std::string& type,
                               std::function<Component*()> factory);
    
    void resize(const std::string& type, size_t newSize);
    void clear();

private:
    struct PoolData {
        std::vector<Component*> components;
        std::queue<Component*> available;
        std::function<Component*()> factory;
    };
    
    std::unordered_map<std::string, PoolData> pools;
    
    void grow(const std::string& type, size_t count);
};

class EntityPoolManager {
public:
    static EntityPoolManager& getInstance();
    
    void registerPool(const std::string& name, EntityManager* manager, size_t initialSize = 100);
    void unregisterPool(const std::string& name);
    
    EntityPool* getPool(const std::string& name);
    
    Entity* acquire(const std::string& poolName);
    void release(const std::string& poolName, Entity* entity);
    
    void clearAll();

private:
    EntityPoolManager() {}
    ~EntityPoolManager() {}
    EntityPoolManager(const EntityPoolManager&) = delete;
    EntityPoolManager& operator=(const EntityPoolManager&) = delete;
    
    std::unordered_map<std::string, std::unique_ptr<EntityPool>> pools;
};

template<typename T>
class PooledVector {
public:
    PooledVector(size_t chunkSize = 1024);
    ~PooledVector();
    
    T& operator[](size_t index);
    const T& operator[](size_t index) const;
    
    void push_back(const T& value);
    void pop_back();
    
    size_t size() const { return count; }
    bool empty() const { return count == 0; }
    
    void clear();
    void reserve(size_t capacity);

private:
    size_t chunkSize;
    size_t count;
    std::vector<std::unique_ptr<T[]>> chunks;
    
    void allocateChunk();
};

class PoolAllocator {
public:
    PoolAllocator(size_t blockSize, size_t blockCount);
    ~PoolAllocator();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const { return blockSize; }
    size_t getTotalBlocks() const { return blockCount; }
    size_t getUsedBlocks() const { return blockCount - freeBlocks.size(); }

private:
    size_t blockSize;
    size_t blockCount;
    void* memory;
    std::queue<void*> freeBlocks;
    
    void initialize();
};

// Template implementations

template<typename T>
ObjectPool<T>::ObjectPool(size_t initialSize) {
    grow(initialSize);
}

template<typename T>
ObjectPool<T>::~ObjectPool() {
    clear();
}

template<typename T>
T* ObjectPool<T>::acquire() {
    if (available.empty()) {
        grow(pool.size() / 2 + 1);
    }
    
    T* obj = available.front();
    available.pop();
    return obj;
}

template<typename T>
void ObjectPool<T>::release(T* obj) {
    if (obj) {
        available.push(obj);
    }
}

template<typename T>
void ObjectPool<T>::resize(size_t newSize) {
    if (newSize > pool.size()) {
        grow(newSize - pool.size());
    }
}

template<typename T>
void ObjectPool<T>::clear() {
    pool.clear();
    while (!available.empty()) {
        available.pop();
    }
}

template<typename T>
void ObjectPool<T>::grow(size_t count) {
    size_t oldSize = pool.size();
    pool.reserve(oldSize + count);
    
    for (size_t i = 0; i < count; ++i) {
        auto obj = std::make_unique<T>();
        available.push(obj.get());
        pool.push_back(std::move(obj));
    }
}

template<typename T>
PooledVector<T>::PooledVector(size_t chunkSize)
    : chunkSize(chunkSize), count(0) {}

template<typename T>
PooledVector<T>::~PooledVector() {
    clear();
}

template<typename T>
T& PooledVector<T>::operator[](size_t index) {
    size_t chunkIndex = index / chunkSize;
    size_t offsetIndex = index % chunkSize;
    return chunks[chunkIndex][offsetIndex];
}

template<typename T>
const T& PooledVector<T>::operator[](size_t index) const {
    size_t chunkIndex = index / chunkSize;
    size_t offsetIndex = index % chunkSize;
    return chunks[chunkIndex][offsetIndex];
}

template<typename T>
void PooledVector<T>::push_back(const T& value) {
    if (count >= chunks.size() * chunkSize) {
        allocateChunk();
    }
    
    (*this)[count++] = value;
}

template<typename T>
void PooledVector<T>::pop_back() {
    if (count > 0) {
        --count;
    }
}

template<typename T>
void PooledVector<T>::clear() {
    count = 0;
    chunks.clear();
}

template<typename T>
void PooledVector<T>::reserve(size_t capacity) {
    size_t requiredChunks = (capacity + chunkSize - 1) / chunkSize;
    while (chunks.size() < requiredChunks) {
        allocateChunk();
    }
}

template<typename T>
void PooledVector<T>::allocateChunk() {
    chunks.push_back(std::make_unique<T[]>(chunkSize));
}

} // namespace ECS
} // namespace JJM
