#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <string>
#include <array>
#include <bitset>
#include <chrono>
#include <unordered_map>

namespace JJM {
namespace Memory {

// =============================================================================
// Memory Block
// =============================================================================

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

// =============================================================================
// Advanced Memory Pool Types
// =============================================================================

/**
 * @brief Pool allocation strategy
 */
enum class PoolAllocationStrategy {
    FirstFit,           // First free block that fits
    BestFit,            // Smallest free block that fits
    NextFit,            // Search from last allocation point
    Buddy,              // Power-of-2 buddy allocation
    Slab                // Fixed-size object caching
};

/**
 * @brief Memory pool statistics
 */
struct PoolStatistics {
    size_t totalBytes;
    size_t allocatedBytes;
    size_t peakAllocatedBytes;
    size_t freeBytes;
    size_t allocationCount;
    size_t deallocationCount;
    size_t failedAllocations;
    size_t fragmentedBlocks;
    float fragmentation;            // 0.0 - 1.0
    float utiliziation;             // 0.0 - 1.0
    std::chrono::nanoseconds totalAllocationTime;
    std::chrono::nanoseconds totalDeallocationTime;
    
    PoolStatistics()
        : totalBytes(0)
        , allocatedBytes(0)
        , peakAllocatedBytes(0)
        , freeBytes(0)
        , allocationCount(0)
        , deallocationCount(0)
        , failedAllocations(0)
        , fragmentedBlocks(0)
        , fragmentation(0.0f)
        , utiliziation(0.0f)
    {}
};

/**
 * @brief Memory allocation info for debugging
 */
struct AllocationInfo {
    void* address;
    size_t size;
    std::string file;
    int line;
    std::string function;
    std::chrono::steady_clock::time_point timestamp;
    std::string tag;
    
    AllocationInfo()
        : address(nullptr)
        , size(0)
        , line(0)
    {}
};

/**
 * @brief Memory leak detection entry
 */
struct LeakInfo {
    AllocationInfo allocation;
    size_t leakSize;
    bool isReported;
};

// =============================================================================
// Buddy Allocator - Power-of-2 allocation
// =============================================================================

/**
 * @brief Buddy system allocator for power-of-2 allocations
 */
class BuddyAllocator {
private:
    static constexpr int MAX_ORDER = 20;  // Up to 1MB blocks
    
    void* m_memory;
    size_t m_totalSize;
    int m_maxOrder;
    
    // Free lists for each order
    std::array<MemoryBlock*, MAX_ORDER + 1> m_freeLists;
    
    // Bitmap to track allocated blocks
    std::vector<std::bitset<(1 << MAX_ORDER)>> m_splitBitmaps;
    
    mutable std::mutex m_mutex;
    PoolStatistics m_stats;
    
public:
    BuddyAllocator(size_t totalSize);
    ~BuddyAllocator();
    
    void* allocate(size_t size);
    void deallocate(void* ptr, size_t size);
    
    // Query
    size_t getTotalSize() const { return m_totalSize; }
    size_t getFreeSize() const;
    const PoolStatistics& getStats() const { return m_stats; }
    
    // Utility
    bool isValidPointer(void* ptr) const;
    static int sizeToOrder(size_t size);
    static size_t orderToSize(int order);
    
private:
    void* allocateBlock(int order);
    void freeBlock(void* ptr, int order);
    int findBuddyIndex(void* ptr, int order) const;
    void* getBuddy(void* ptr, int order) const;
    void splitBlock(int order);
    bool mergeBlocks(void* ptr, int order);
};

// =============================================================================
// Slab Allocator - Object caching
// =============================================================================

/**
 * @brief Slab cache for fixed-size objects
 */
class SlabCache {
private:
    struct Slab {
        void* memory;
        std::vector<bool> freeMap;
        size_t freeCount;
        Slab* next;
        Slab* prev;
    };
    
    std::string m_name;
    size_t m_objectSize;
    size_t m_objectsPerSlab;
    size_t m_alignment;
    
    Slab* m_partialSlabs;       // Slabs with some free objects
    Slab* m_fullSlabs;          // Completely allocated slabs
    Slab* m_emptySlabs;         // Completely free slabs
    
    size_t m_maxEmptySlabs;     // Maximum cached empty slabs
    size_t m_emptySlabCount;
    
    // Constructor/destructor for objects
    std::function<void(void*)> m_constructor;
    std::function<void(void*)> m_destructor;
    
    mutable std::mutex m_mutex;
    PoolStatistics m_stats;
    
public:
    SlabCache(const std::string& name, size_t objectSize, size_t objectsPerSlab = 64,
              size_t alignment = alignof(std::max_align_t));
    ~SlabCache();
    
    // Object allocation
    void* allocate();
    void deallocate(void* ptr);
    
    // Batch operations
    std::vector<void*> allocateBatch(size_t count);
    void deallocateBatch(const std::vector<void*>& ptrs);
    
    // Constructor/destructor
    void setConstructor(std::function<void(void*)> ctor) { m_constructor = ctor; }
    void setDestructor(std::function<void(void*)> dtor) { m_destructor = dtor; }
    
    // Cache management
    void shrink();                  // Free empty slabs
    void grow(size_t slabCount);    // Pre-allocate slabs
    void reap();                    // Aggressive memory reclamation
    
    // Query
    const std::string& getName() const { return m_name; }
    size_t getObjectSize() const { return m_objectSize; }
    const PoolStatistics& getStats() const { return m_stats; }
    size_t getAllocatedObjects() const;
    size_t getTotalObjects() const;
    
private:
    Slab* createSlab();
    void destroySlab(Slab* slab);
    void* allocateFromSlab(Slab* slab);
    void deallocateFromSlab(Slab* slab, void* ptr);
    void moveSlabToList(Slab* slab, Slab** fromList, Slab** toList);
};

// =============================================================================
// Ring Buffer Allocator - FIFO allocation
// =============================================================================

/**
 * @brief Ring buffer allocator for FIFO allocations
 */
class RingBufferAllocator {
private:
    void* m_memory;
    size_t m_size;
    size_t m_head;
    size_t m_tail;
    size_t m_usedBytes;
    
    mutable std::mutex m_mutex;
    PoolStatistics m_stats;
    
public:
    RingBufferAllocator(size_t size);
    ~RingBufferAllocator();
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    void deallocateOldest();
    void clear();
    
    size_t getUsedBytes() const { return m_usedBytes; }
    size_t getFreeBytes() const { return m_size - m_usedBytes; }
    size_t getTotalBytes() const { return m_size; }
    
    bool isEmpty() const { return m_usedBytes == 0; }
    bool isFull() const { return m_usedBytes >= m_size; }
    
private:
    size_t alignUp(size_t value, size_t alignment) const;
};

// =============================================================================
// Thread-Local Pool - Per-thread allocation
// =============================================================================

/**
 * @brief Thread-local memory pool to avoid contention
 */
class ThreadLocalPool {
private:
    static thread_local std::unique_ptr<MemoryPool> tl_pool;
    
    size_t m_blockSize;
    size_t m_blocksPerThread;
    
public:
    ThreadLocalPool(size_t blockSize, size_t blocksPerThread);
    ~ThreadLocalPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
    static void initializeThread();
    static void cleanupThread();
};

// =============================================================================
// Memory Arena - Linear allocation with reset
// =============================================================================

/**
 * @brief Memory arena for temporary allocations
 */
class MemoryArena {
private:
    struct Chunk {
        void* memory;
        size_t size;
        size_t used;
        Chunk* next;
    };
    
    size_t m_defaultChunkSize;
    Chunk* m_currentChunk;
    Chunk* m_firstChunk;
    
    size_t m_totalAllocated;
    size_t m_totalUsed;
    
    mutable std::mutex m_mutex;
    
public:
    MemoryArena(size_t defaultChunkSize = 64 * 1024);
    ~MemoryArena();
    
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* memory = allocate(sizeof(T), alignof(T));
        return new(memory) T(std::forward<Args>(args)...);
    }
    
    void reset();                   // Reset for reuse (keeps memory)
    void clear();                   // Free all memory
    
    size_t getTotalAllocated() const { return m_totalAllocated; }
    size_t getTotalUsed() const { return m_totalUsed; }
    
private:
    void allocateNewChunk(size_t minSize);
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

/**
 * @brief Component-optimized memory pool with fixed-size blocks
 * Designed for ECS component allocation with minimal fragmentation
 */
class ComponentPool {
public:
    ComponentPool(size_t componentSize, size_t componentsPerBlock = 256);
    ~ComponentPool();
    
    void* allocateComponent();
    void deallocateComponent(void* component);
    
    // Bulk operations for efficient batch processing
    void* allocateBatch(size_t count, std::vector<void*>& outPointers);
    void deallocateBatch(const std::vector<void*>& pointers);
    
    // Defragmentation for improved cache coherency
    void compact();
    void* getComponentAtIndex(size_t index) const;
    size_t getComponentIndex(void* component) const;
    
    // Statistics
    size_t getComponentSize() const { return m_componentSize; }
    size_t getAllocatedCount() const { return m_allocatedCount; }
    size_t getCapacity() const { return m_capacity; }
    float getFragmentation() const;
    
private:
    size_t m_componentSize;
    size_t m_componentsPerBlock;
    size_t m_capacity;
    std::atomic<size_t> m_allocatedCount;
    
    struct Block {
        void* memory;
        std::bitset<256> allocationBitmap;
        size_t freeCount;
    };
    
    std::vector<Block> m_blocks;
    std::vector<size_t> m_freeBlockIndices;
    mutable std::mutex m_mutex;
    
    void allocateNewBlock();
    size_t findFreeSlotInBlock(Block& block);
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