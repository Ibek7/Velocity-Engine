#include "ecs/EntityPool.h"
#include <cstring>
#include <cstdlib>

namespace JJM {
namespace ECS {

// EntityPool implementation
EntityPool::EntityPool(EntityManager* manager, size_t initialSize)
    : manager(manager), poolSize(0) {
    grow(initialSize);
}

EntityPool::~EntityPool() {
    clear();
}

Entity* EntityPool::acquire() {
    if (availableEntities.empty()) {
        grow(poolSize / 2 + 1);
    }
    
    Entity* entity = availableEntities.front();
    availableEntities.pop();
    
    if (onAcquire) {
        onAcquire(entity);
    }
    
    return entity;
}

void EntityPool::release(Entity* entity) {
    if (!entity) return;
    
    resetEntity(entity);
    availableEntities.push(entity);
    
    if (onRelease) {
        onRelease(entity);
    }
}

void EntityPool::resize(size_t newSize) {
    if (newSize > poolSize) {
        grow(newSize - poolSize);
    }
}

void EntityPool::clear() {
    for (Entity* entity : entities) {
        if (manager) {
            manager->destroyEntity(entity);
        }
    }
    entities.clear();
    
    while (!availableEntities.empty()) {
        availableEntities.pop();
    }
    
    poolSize = 0;
}

void EntityPool::grow(size_t count) {
    if (!manager) return;
    
    for (size_t i = 0; i < count; ++i) {
        Entity* entity = manager->createEntity();
        if (entity) {
            entities.push_back(entity);
            availableEntities.push(entity);
            poolSize++;
        }
    }
}

void EntityPool::resetEntity(Entity* entity) {
    if (!entity) return;
    
    // Remove all components from entity
    // This would require reflection or component tracking
}

// ComponentPool implementation
ComponentPool::ComponentPool(size_t initialSize) {}

ComponentPool::~ComponentPool() {
    clear();
}

Component* ComponentPool::acquire(const std::string& type) {
    auto it = pools.find(type);
    if (it == pools.end()) {
        return nullptr;
    }
    
    PoolData& pool = it->second;
    if (pool.available.empty()) {
        grow(type, pool.components.size() / 2 + 1);
    }
    
    Component* component = pool.available.front();
    pool.available.pop();
    return component;
}

void ComponentPool::release(Component* component) {
    if (!component) return;
    
    // Find which pool this component belongs to
    // This would require storing component type info
}

void ComponentPool::registerComponentType(const std::string& type,
                                         std::function<Component*()> factory) {
    PoolData pool;
    pool.factory = factory;
    pools[type] = pool;
}

void ComponentPool::resize(const std::string& type, size_t newSize) {
    auto it = pools.find(type);
    if (it != pools.end()) {
        size_t currentSize = it->second.components.size();
        if (newSize > currentSize) {
            grow(type, newSize - currentSize);
        }
    }
}

void ComponentPool::clear() {
    for (auto& pair : pools) {
        for (Component* component : pair.second.components) {
            delete component;
        }
    }
    pools.clear();
}

void ComponentPool::grow(const std::string& type, size_t count) {
    auto it = pools.find(type);
    if (it == pools.end() || !it->second.factory) return;
    
    PoolData& pool = it->second;
    for (size_t i = 0; i < count; ++i) {
        Component* component = pool.factory();
        if (component) {
            pool.components.push_back(component);
            pool.available.push(component);
        }
    }
}

// EntityPoolManager implementation
EntityPoolManager& EntityPoolManager::getInstance() {
    static EntityPoolManager instance;
    return instance;
}

void EntityPoolManager::registerPool(const std::string& name, EntityManager* manager, size_t initialSize) {
    pools[name] = std::make_unique<EntityPool>(manager, initialSize);
}

void EntityPoolManager::unregisterPool(const std::string& name) {
    pools.erase(name);
}

EntityPool* EntityPoolManager::getPool(const std::string& name) {
    auto it = pools.find(name);
    if (it != pools.end()) {
        return it->second.get();
    }
    return nullptr;
}

Entity* EntityPoolManager::acquire(const std::string& poolName) {
    EntityPool* pool = getPool(poolName);
    return pool ? pool->acquire() : nullptr;
}

void EntityPoolManager::release(const std::string& poolName, Entity* entity) {
    EntityPool* pool = getPool(poolName);
    if (pool) {
        pool->release(entity);
    }
}

void EntityPoolManager::clearAll() {
    pools.clear();
}

// PoolAllocator implementation
PoolAllocator::PoolAllocator(size_t blockSize, size_t blockCount)
    : blockSize(blockSize), blockCount(blockCount), memory(nullptr) {
    initialize();
}

PoolAllocator::~PoolAllocator() {
    if (memory) {
        std::free(memory);
    }
}

void* PoolAllocator::allocate() {
    if (freeBlocks.empty()) {
        return nullptr;
    }
    
    void* block = freeBlocks.front();
    freeBlocks.pop();
    return block;
}

void PoolAllocator::deallocate(void* ptr) {
    if (ptr) {
        freeBlocks.push(ptr);
    }
}

void PoolAllocator::initialize() {
    memory = std::malloc(blockSize * blockCount);
    
    if (memory) {
        char* current = static_cast<char*>(memory);
        for (size_t i = 0; i < blockCount; ++i) {
            freeBlocks.push(current);
            current += blockSize;
        }
    }
}

} // namespace ECS
} // namespace JJM
