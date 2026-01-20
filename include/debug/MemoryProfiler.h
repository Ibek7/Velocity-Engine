#ifndef JJM_MEMORY_PROFILER_H
#define JJM_MEMORY_PROFILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>

namespace JJM {
namespace Debug {

/**
 * @brief Memory allocation record
 */
struct AllocationRecord {
    void* address;
    size_t size;
    const char* file;
    int line;
    const char* function;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    size_t allocationId;
};

/**
 * @brief Memory statistics with detailed breakdown
 */
struct MemoryStats {
    size_t totalAllocations;
    size_t totalDeallocations;
    size_t currentAllocations;
    size_t peakAllocations;
    size_t totalBytesAllocated;
    size_t totalBytesFreed;
    size_t currentBytesAllocated;
    size_t peakBytesAllocated;
    
    // Allocation size distribution
    size_t smallAllocations;   // < 1KB
    size_t mediumAllocations;  // 1KB - 1MB
    size_t largeAllocations;   // > 1MB
    
    // Allocation age tracking
    size_t shortLivedAllocs;   // < 1 second
    size_t mediumLivedAllocs;  // 1-60 seconds
    size_t longLivedAllocs;    // > 60 seconds
    
    // Performance metrics
    double avgAllocationSize;
    double avgDeallocationTime;
    size_t fragmentationBytes;
    
    MemoryStats() 
        : totalAllocations(0), totalDeallocations(0)
        , currentAllocations(0), peakAllocations(0)
        , totalBytesAllocated(0), totalBytesFreed(0)
        , currentBytesAllocated(0), peakBytesAllocated(0)
        , smallAllocations(0), mediumAllocations(0), largeAllocations(0)
        , shortLivedAllocs(0), mediumLivedAllocs(0), longLivedAllocs(0)
        , avgAllocationSize(0.0), avgDeallocationTime(0.0)
        , fragmentationBytes(0)
    {}
};

/**
 * @brief Memory leak information
 */
struct MemoryLeak {
    void* address;
    size_t size;
    const char* file;
    int line;
    const char* function;
    float ageInSeconds;
};

/**
 * @brief Memory profiler for tracking allocations and leaks
 */
class MemoryProfiler {
public:
    static MemoryProfiler& getInstance();
    
    void enable();
    void disable();
    bool isEnabled() const;
    
    void recordAllocation(void* address, size_t size,
                         const char* file, int line, const char* function);
    void recordDeallocation(void* address);
    
    MemoryStats getStats() const;
    std::vector<MemoryLeak> detectLeaks() const;
    
    void reset();
    void dumpReport(const std::string& filePath) const;
    void printReport() const;
    
    size_t getAllocationCount() const;
    size_t getActiveAllocationCount() const;

private:
    MemoryProfiler();
    ~MemoryProfiler();
    
    MemoryProfiler(const MemoryProfiler&) = delete;
    MemoryProfiler& operator=(const MemoryProfiler&) = delete;
    
    bool enabled;
    mutable std::mutex mutex;
    std::unordered_map<void*, AllocationRecord> allocations;
    MemoryStats stats;
    size_t nextAllocationId;
    std::chrono::time_point<std::chrono::steady_clock> startTime;
};

/**
 * @brief Memory allocation tracker with scope
 */
class ScopedMemoryTracker {
public:
    ScopedMemoryTracker(const std::string& name);
    ~ScopedMemoryTracker();
    
    size_t getBytesAllocated() const;
    size_t getAllocationCount() const;

private:
    std::string name;
    size_t startBytes;
    size_t startAllocations;
};

/**
 * @brief Memory pool for efficient allocations
 */
class MemoryPool {
public:
    MemoryPool(size_t blockSize, size_t blockCount);
    ~MemoryPool();
    
    void* allocate();
    void deallocate(void* ptr);
    
    size_t getBlockSize() const;
    size_t getTotalBlocks() const;
    size_t getUsedBlocks() const;
    size_t getFreeBlocks() const;

private:
    size_t blockSize;
    size_t blockCount;
    std::vector<uint8_t> memory;
    std::vector<void*> freeList;
    std::mutex mutex;
};

/**
 * @brief Stack allocator for fast temporary allocations
 */
class StackAllocator {
public:
    StackAllocator(size_t size);
    ~StackAllocator();
    
    void* allocate(size_t size, size_t alignment = 8);
    void reset();
    
    size_t getSize() const;
    size_t getUsed() const;
    size_t getRemaining() const;

private:
    std::vector<uint8_t> memory;
    size_t offset;
    size_t totalSize;
};

/**
 * @brief Memory arena for grouped allocations
 */
class MemoryArena {
public:
    MemoryArena(size_t chunkSize = 65536);
    ~MemoryArena();
    
    void* allocate(size_t size);
    void reset();
    void clear();
    
    size_t getTotalAllocated() const;
    size_t getChunkCount() const;

private:
    struct Chunk {
        std::vector<uint8_t> memory;
        size_t offset;
    };
    
    size_t chunkSize;
    std::vector<std::unique_ptr<Chunk>> chunks;
    size_t currentChunk;
    size_t totalAllocated;
};

/**
 * @brief Allocation call stack tracker
 */
class AllocationCallStack {
public:
    AllocationCallStack();
    ~AllocationCallStack();
    
    void captureStack(int maxFrames = 32);
    std::vector<std::string> getStackTrace() const;
    void print() const;

private:
    std::vector<void*> frames;
};

/**
 * @brief Memory fragmentation analyzer
 */
class MemoryFragmentation {
public:
    struct FragmentationInfo {
        size_t totalFreeBlocks;
        size_t largestFreeBlock;
        size_t totalFreeMemory;
        float fragmentationRatio;
    };
    
    static FragmentationInfo analyze(const std::vector<void*>& allocations);
    static float calculateFragmentation(size_t largestFree, size_t totalFree);
};

/**
 * @brief Memory allocator interface
 */
class IAllocator {
public:
    virtual ~IAllocator() = default;
    virtual void* allocate(size_t size, size_t alignment = 8) = 0;
    virtual void deallocate(void* ptr) = 0;
    virtual void reset() = 0;
};

/**
 * @brief Linear allocator for sequential allocations
 */
class LinearAllocator : public IAllocator {
public:
    LinearAllocator(size_t size);
    ~LinearAllocator() override;
    
    void* allocate(size_t size, size_t alignment = 8) override;
    void deallocate(void* ptr) override;
    void reset() override;

private:
    std::vector<uint8_t> memory;
    size_t offset;
};

/**
 * @brief Free list allocator for variable-sized allocations
 */
class FreeListAllocator : public IAllocator {
public:
    FreeListAllocator(size_t size);
    ~FreeListAllocator() override;
    
    void* allocate(size_t size, size_t alignment = 8) override;
    void deallocate(void* ptr) override;
    void reset() override;

private:
    struct FreeBlock {
        size_t size;
        FreeBlock* next;
    };
    
    std::vector<uint8_t> memory;
    FreeBlock* freeList;
};

/**
 * @brief Memory profiler report generator
 */
class MemoryReport {
public:
    static void generateReport(const MemoryStats& stats,
                               const std::vector<MemoryLeak>& leaks,
                               const std::string& filePath);
    
    static void generateHTMLReport(const MemoryStats& stats,
                                   const std::vector<MemoryLeak>& leaks,
                                   const std::string& filePath);
    
    static std::string formatBytes(size_t bytes);
    static std::string formatMemoryLeak(const MemoryLeak& leak);
};

/**
 * @brief Memory snapshot for comparison
 */
class MemorySnapshot {
public:
    MemorySnapshot();
    ~MemorySnapshot();
    
    void capture();
    MemoryStats getStats() const;
    
    static MemorySnapshot diff(const MemorySnapshot& before,
                              const MemorySnapshot& after);

private:
    MemoryStats stats;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
};

/**
 * @brief Memory bounds checker
 */
class MemoryBoundsChecker {
public:
    static void* allocateWithGuards(size_t size);
    static void deallocateWithGuards(void* ptr);
    static bool checkGuards(void* ptr);

private:
    static constexpr uint32_t GUARD_VALUE = 0xDEADBEEF;
    static constexpr size_t GUARD_SIZE = sizeof(uint32_t);
};

} // namespace Debug
} // namespace JJM

// Memory profiling macros
#ifdef JJM_MEMORY_PROFILING
#define JJM_TRACK_ALLOC(ptr, size) \
    JJM::Debug::MemoryProfiler::getInstance().recordAllocation(ptr, size, __FILE__, __LINE__, __FUNCTION__)
#define JJM_TRACK_FREE(ptr) \
    JJM::Debug::MemoryProfiler::getInstance().recordDeallocation(ptr)
#else
#define JJM_TRACK_ALLOC(ptr, size) ((void)0)
#define JJM_TRACK_FREE(ptr) ((void)0)
#endif

#endif // JJM_MEMORY_PROFILER_H
