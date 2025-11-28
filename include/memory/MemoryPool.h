#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace JJM {
namespace Memory {

class MemoryBlock {
private:
    void* data;
    size_t size;
    bool allocated;
    MemoryBlock* next;
    
public:
    MemoryBlock(size_t size);
    ~MemoryBlock();
    
    void* getData() const { return data; }
    size_t getSize() const { return size; }
    bool isAllocated() const { return allocated; }
    
    void setAllocated(bool state) { allocated = state; }
    void setNext(MemoryBlock* block) { next = block; }
    MemoryBlock* getNext() const { return next; }
};

class MemoryPool {
private:
    size_t blockSize;
    size_t poolSize;
    void* poolMemory;
    
    MemoryBlock* freeList;
    std::vector<MemoryBlock*> allBlocks;
    
    mutable std::mutex poolMutex;
    std::atomic<size_t> allocatedBytes;
    std::atomic<size_t> allocatedBlocks;
    
public:
    MemoryPool(size_t blockSize, size_t numBlocks);
    ~MemoryPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const { return blockSize; }
    size_t getTotalBlocks() const { return allBlocks.size(); }
    size_t getAllocatedBlocks() const { return allocatedBlocks; }
    size_t getFreeBlocks() const { return getTotalBlocks() - allocatedBlocks; }
    
    size_t getAllocatedBytes() const { return allocatedBytes; }
    size_t getTotalBytes() const { return poolSize; }
    
    double getUtilization() const;
    
    bool isValidPointer(void* ptr) const;
    void defragment();
    
private:
    void initializePool();
    MemoryBlock* findBlock(void* ptr) const;
};

class StackAllocator {
private:
    void* memory;
    size_t size;
    size_t offset;
    
    struct Marker {
        size_t position;
        const char* label;
    };
    
    std::vector<Marker> markers;
    
public:
    StackAllocator(size_t size);
    ~StackAllocator();
    
    void* allocate(size_t bytes, size_t alignment = sizeof(void*));
    void pushMarker(const char* label = nullptr);
    void popMarker();
    void popToMarker(const char* label);
    void clear();
    
    size_t getUsedBytes() const { return offset; }
    size_t getFreeBytes() const { return size - offset; }
    size_t getTotalBytes() const { return size; }
    
    double getUtilization() const;
    
private:
    size_t alignOffset(size_t offset, size_t alignment) const;
};

class ObjectPool {
private:
    struct ObjectNode {
        alignas(std::max_align_t) char data[1]; // Flexible array member
        ObjectNode* next;
    };
    
    size_t objectSize;
    size_t chunkSize;
    ObjectNode* freeList;
    std::vector<void*> chunks;
    
    mutable std::mutex poolMutex;
    std::atomic<size_t> allocatedObjects;
    
public:
    ObjectPool(size_t objectSize, size_t objectsPerChunk = 64);
    ~ObjectPool();
    
    void* allocateObject();
    void deallocateObject(void* obj);
    
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* memory = allocateObject();
        if (memory) {
            return new(memory) T(std::forward<Args>(args)...);
        }
        return nullptr;
    }
    
    template<typename T>
    void destroy(T* obj) {
        if (obj) {
            obj->~T();
            deallocateObject(obj);
        }
    }
    
    size_t getObjectSize() const { return objectSize; }
    size_t getAllocatedObjects() const { return allocatedObjects; }
    size_t getTotalObjects() const;
    
private:
    void allocateNewChunk();
};

class MemoryManager {
private:
    std::vector<std::unique_ptr<MemoryPool>> pools;
    std::vector<std::unique_ptr<ObjectPool>> objectPools;
    std::unique_ptr<StackAllocator> frameAllocator;
    
    mutable std::mutex managerMutex;
    std::atomic<size_t> totalAllocatedBytes;
    
    static MemoryManager* instance;
    MemoryManager();
    
public:
    static MemoryManager* getInstance();
    ~MemoryManager();
    
    void initialize(size_t frameAllocatorSize = 1024 * 1024); // 1MB default
    void shutdown();
    
    MemoryPool* createPool(const std::string& name, size_t blockSize, size_t numBlocks);
    MemoryPool* getPool(const std::string& name);
    
    ObjectPool* createObjectPool(const std::string& name, size_t objectSize, size_t objectsPerChunk = 64);
    ObjectPool* getObjectPool(const std::string& name);
    
    StackAllocator* getFrameAllocator() { return frameAllocator.get(); }
    
    void* allocate(size_t size, const std::string& poolName = "");
    void deallocate(void* ptr, const std::string& poolName = "");
    
    void resetFrameAllocator();
    
    void printStatistics() const;
    std::string getStatisticsReport() const;
    
    size_t getTotalAllocatedBytes() const { return totalAllocatedBytes; }
    
private:
    std::map<std::string, MemoryPool*> namedPools;
    std::map<std::string, ObjectPool*> namedObjectPools;
    
    MemoryPool* findBestFitPool(size_t size);
};

// RAII helper for frame allocations
class FrameAllocatorScope {
private:
    StackAllocator* allocator;
    
public:
    FrameAllocatorScope(const char* label = nullptr);
    ~FrameAllocatorScope();
    
    template<typename T>
    T* allocate(size_t count = 1) {
        return static_cast<T*>(allocator->allocate(sizeof(T) * count, alignof(T)));
    }
};

// Custom allocator for STL containers
template<typename T>
class PoolAllocator {
private:
    MemoryPool* pool;
    
public:
    using value_type = T;
    
    PoolAllocator(MemoryPool* pool) : pool(pool) {}
    
    template<typename U>
    PoolAllocator(const PoolAllocator<U>& other) : pool(other.pool) {}
    
    T* allocate(size_t n) {
        if (pool && sizeof(T) * n <= pool->getBlockSize()) {
            return static_cast<T*>(pool->allocate());
        }
        return static_cast<T*>(std::malloc(sizeof(T) * n));
    }
    
    void deallocate(T* p, size_t n) {
        if (pool && pool->isValidPointer(p)) {
            pool->deallocate(p);
        } else {
            std::free(p);
        }
    }
    
    template<typename U>
    bool operator==(const PoolAllocator<U>& other) const {
        return pool == other.pool;
    }
};

} // namespace Memory
} // namespace JJM

#endif // MEMORY_POOL_H