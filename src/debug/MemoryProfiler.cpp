#include "debug/MemoryProfiler.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iostream>

namespace JJM {
namespace Debug {

// MemoryProfiler implementation
MemoryProfiler& MemoryProfiler::getInstance() {
    static MemoryProfiler instance;
    return instance;
}

MemoryProfiler::MemoryProfiler()
    : enabled(false), nextAllocationId(0) {
    std::memset(&stats, 0, sizeof(stats));
    startTime = std::chrono::steady_clock::now();
}

MemoryProfiler::~MemoryProfiler() {}

void MemoryProfiler::enable() {
    std::lock_guard<std::mutex> lock(mutex);
    enabled = true;
}

void MemoryProfiler::disable() {
    std::lock_guard<std::mutex> lock(mutex);
    enabled = false;
}

bool MemoryProfiler::isEnabled() const {
    return enabled;
}

void MemoryProfiler::recordAllocation(void* address, size_t size,
                                     const char* file, int line, const char* function) {
    if (!enabled || !address) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    AllocationRecord record;
    record.address = address;
    record.size = size;
    record.file = file;
    record.line = line;
    record.function = function;
    record.timestamp = std::chrono::steady_clock::now();
    record.allocationId = nextAllocationId++;
    
    allocations[address] = record;
    
    ++stats.totalAllocations;
    ++stats.currentAllocations;
    stats.totalBytesAllocated += size;
    stats.currentBytesAllocated += size;
    
    if (stats.currentAllocations > stats.peakAllocations) {
        stats.peakAllocations = stats.currentAllocations;
    }
    if (stats.currentBytesAllocated > stats.peakBytesAllocated) {
        stats.peakBytesAllocated = stats.currentBytesAllocated;
    }
}

void MemoryProfiler::recordDeallocation(void* address) {
    if (!enabled || !address) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = allocations.find(address);
    if (it != allocations.end()) {
        ++stats.totalDeallocations;
        --stats.currentAllocations;
        stats.totalBytesFreed += it->second.size;
        stats.currentBytesAllocated -= it->second.size;
        
        allocations.erase(it);
    }
}

MemoryStats MemoryProfiler::getStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats;
}

std::vector<MemoryLeak> MemoryProfiler::detectLeaks() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<MemoryLeak> leaks;
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& pair : allocations) {
        MemoryLeak leak;
        leak.address = pair.second.address;
        leak.size = pair.second.size;
        leak.file = pair.second.file;
        leak.line = pair.second.line;
        leak.function = pair.second.function;
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pair.second.timestamp);
        leak.ageInSeconds = duration.count() / 1000.0f;
        
        leaks.push_back(leak);
    }
    
    return leaks;
}

void MemoryProfiler::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    allocations.clear();
    std::memset(&stats, 0, sizeof(stats));
    nextAllocationId = 0;
    startTime = std::chrono::steady_clock::now();
}

void MemoryProfiler::dumpReport(const std::string& filePath) const {
    auto leaks = detectLeaks();
    MemoryReport::generateReport(stats, leaks, filePath);
}

void MemoryProfiler::printReport() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::cout << "=== Memory Profiler Report ===" << std::endl;
    std::cout << "Total Allocations: " << stats.totalAllocations << std::endl;
    std::cout << "Total Deallocations: " << stats.totalDeallocations << std::endl;
    std::cout << "Current Allocations: " << stats.currentAllocations << std::endl;
    std::cout << "Peak Allocations: " << stats.peakAllocations << std::endl;
    std::cout << "Total Bytes Allocated: " << MemoryReport::formatBytes(stats.totalBytesAllocated) << std::endl;
    std::cout << "Current Bytes Allocated: " << MemoryReport::formatBytes(stats.currentBytesAllocated) << std::endl;
    std::cout << "Peak Bytes Allocated: " << MemoryReport::formatBytes(stats.peakBytesAllocated) << std::endl;
    
    if (stats.currentAllocations > 0) {
        std::cout << "\n=== Potential Memory Leaks ===" << std::endl;
        auto leaks = detectLeaks();
        for (const auto& leak : leaks) {
            std::cout << MemoryReport::formatMemoryLeak(leak) << std::endl;
        }
    }
}

size_t MemoryProfiler::getAllocationCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats.totalAllocations;
}

size_t MemoryProfiler::getActiveAllocationCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats.currentAllocations;
}

// ScopedMemoryTracker implementation
ScopedMemoryTracker::ScopedMemoryTracker(const std::string& name)
    : name(name) {
    auto& profiler = MemoryProfiler::getInstance();
    auto stats = profiler.getStats();
    startBytes = stats.currentBytesAllocated;
    startAllocations = stats.currentAllocations;
}

ScopedMemoryTracker::~ScopedMemoryTracker() {
    auto& profiler = MemoryProfiler::getInstance();
    auto stats = profiler.getStats();
    size_t bytesAllocated = stats.currentBytesAllocated - startBytes;
    size_t allocations = stats.currentAllocations - startAllocations;
    
    std::cout << "[" << name << "] Allocated: " 
              << MemoryReport::formatBytes(bytesAllocated)
              << " (" << allocations << " allocations)" << std::endl;
}

size_t ScopedMemoryTracker::getBytesAllocated() const {
    auto& profiler = MemoryProfiler::getInstance();
    auto stats = profiler.getStats();
    return stats.currentBytesAllocated - startBytes;
}

size_t ScopedMemoryTracker::getAllocationCount() const {
    auto& profiler = MemoryProfiler::getInstance();
    auto stats = profiler.getStats();
    return stats.currentAllocations - startAllocations;
}

// MemoryPool implementation
MemoryPool::MemoryPool(size_t blockSize, size_t blockCount)
    : blockSize(blockSize), blockCount(blockCount) {
    memory.resize(blockSize * blockCount);
    
    for (size_t i = 0; i < blockCount; ++i) {
        freeList.push_back(&memory[i * blockSize]);
    }
}

MemoryPool::~MemoryPool() {}

void* MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (freeList.empty()) return nullptr;
    
    void* ptr = freeList.back();
    freeList.pop_back();
    return ptr;
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    freeList.push_back(ptr);
}

size_t MemoryPool::getBlockSize() const { return blockSize; }
size_t MemoryPool::getTotalBlocks() const { return blockCount; }
size_t MemoryPool::getUsedBlocks() const { return blockCount - freeList.size(); }
size_t MemoryPool::getFreeBlocks() const { return freeList.size(); }

// StackAllocator implementation
StackAllocator::StackAllocator(size_t size)
    : memory(size), offset(0), totalSize(size) {}

StackAllocator::~StackAllocator() {}

void* StackAllocator::allocate(size_t size, size_t alignment) {
    size_t padding = (alignment - (offset % alignment)) % alignment;
    size_t alignedOffset = offset + padding;
    
    if (alignedOffset + size > totalSize) return nullptr;
    
    void* ptr = &memory[alignedOffset];
    offset = alignedOffset + size;
    return ptr;
}

void StackAllocator::reset() {
    offset = 0;
}

size_t StackAllocator::getSize() const { return totalSize; }
size_t StackAllocator::getUsed() const { return offset; }
size_t StackAllocator::getRemaining() const { return totalSize - offset; }

// MemoryArena implementation
MemoryArena::MemoryArena(size_t chunkSize)
    : chunkSize(chunkSize), currentChunk(0), totalAllocated(0) {}

MemoryArena::~MemoryArena() {}

void* MemoryArena::allocate(size_t size) {
    if (chunks.empty() || chunks[currentChunk]->offset + size > chunkSize) {
        auto chunk = std::make_unique<Chunk>();
        chunk->memory.resize(chunkSize);
        chunk->offset = 0;
        chunks.push_back(std::move(chunk));
        currentChunk = chunks.size() - 1;
    }
    
    auto& chunk = chunks[currentChunk];
    void* ptr = &chunk->memory[chunk->offset];
    chunk->offset += size;
    totalAllocated += size;
    
    return ptr;
}

void MemoryArena::reset() {
    for (auto& chunk : chunks) {
        chunk->offset = 0;
    }
    currentChunk = 0;
    totalAllocated = 0;
}

void MemoryArena::clear() {
    chunks.clear();
    currentChunk = 0;
    totalAllocated = 0;
}

size_t MemoryArena::getTotalAllocated() const { return totalAllocated; }
size_t MemoryArena::getChunkCount() const { return chunks.size(); }

// AllocationCallStack implementation
AllocationCallStack::AllocationCallStack() {}
AllocationCallStack::~AllocationCallStack() {}

void AllocationCallStack::captureStack(int maxFrames) {
    (void)maxFrames; // Stub - would use platform-specific stack capture
}

std::vector<std::string> AllocationCallStack::getStackTrace() const {
    return {}; // Stub
}

void AllocationCallStack::print() const {
    // Stub
}

// MemoryFragmentation implementation
MemoryFragmentation::FragmentationInfo MemoryFragmentation::analyze(
    const std::vector<void*>& allocations) {
    (void)allocations; // Stub
    return {};
}

float MemoryFragmentation::calculateFragmentation(size_t largestFree, size_t totalFree) {
    if (totalFree == 0) return 0.0f;
    return 1.0f - (static_cast<float>(largestFree) / totalFree);
}

// LinearAllocator implementation
LinearAllocator::LinearAllocator(size_t size)
    : memory(size), offset(0) {}

LinearAllocator::~LinearAllocator() {}

void* LinearAllocator::allocate(size_t size, size_t alignment) {
    size_t padding = (alignment - (offset % alignment)) % alignment;
    size_t alignedOffset = offset + padding;
    
    if (alignedOffset + size > memory.size()) return nullptr;
    
    void* ptr = &memory[alignedOffset];
    offset = alignedOffset + size;
    return ptr;
}

void LinearAllocator::deallocate(void* ptr) {
    (void)ptr; // Linear allocator doesn't support individual deallocation
}

void LinearAllocator::reset() {
    offset = 0;
}

// FreeListAllocator implementation
FreeListAllocator::FreeListAllocator(size_t size)
    : memory(size) {
    freeList = reinterpret_cast<FreeBlock*>(memory.data());
    freeList->size = size;
    freeList->next = nullptr;
}

FreeListAllocator::~FreeListAllocator() {}

void* FreeListAllocator::allocate(size_t size, size_t alignment) {
    FreeBlock* prev = nullptr;
    FreeBlock* current = freeList;
    
    while (current) {
        uintptr_t address = reinterpret_cast<uintptr_t>(current);
        size_t padding = (alignment - (address % alignment)) % alignment;
        size_t requiredSize = size + padding;
        
        if (current->size >= requiredSize) {
            if (current->size > requiredSize + sizeof(FreeBlock)) {
                FreeBlock* newBlock = reinterpret_cast<FreeBlock*>(
                    reinterpret_cast<uint8_t*>(current) + requiredSize);
                newBlock->size = current->size - requiredSize;
                newBlock->next = current->next;
                
                if (prev) prev->next = newBlock;
                else freeList = newBlock;
            } else {
                if (prev) prev->next = current->next;
                else freeList = current->next;
            }
            
            return reinterpret_cast<void*>(address + padding);
        }
        
        prev = current;
        current = current->next;
    }
    
    return nullptr;
}

void FreeListAllocator::deallocate(void* ptr) {
    if (!ptr) return;
    
    // Simplified - would need to track allocation sizes
    FreeBlock* block = static_cast<FreeBlock*>(ptr);
    block->next = freeList;
    freeList = block;
}

void FreeListAllocator::reset() {
    freeList = reinterpret_cast<FreeBlock*>(memory.data());
    freeList->size = memory.size();
    freeList->next = nullptr;
}

// MemoryReport implementation
void MemoryReport::generateReport(const MemoryStats& stats,
                                  const std::vector<MemoryLeak>& leaks,
                                  const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return;
    
    file << "=== Memory Profiler Report ===" << std::endl;
    file << "Total Allocations: " << stats.totalAllocations << std::endl;
    file << "Total Deallocations: " << stats.totalDeallocations << std::endl;
    file << "Current Allocations: " << stats.currentAllocations << std::endl;
    file << "Peak Allocations: " << stats.peakAllocations << std::endl;
    file << "Total Bytes Allocated: " << formatBytes(stats.totalBytesAllocated) << std::endl;
    file << "Current Bytes: " << formatBytes(stats.currentBytesAllocated) << std::endl;
    file << "Peak Bytes: " << formatBytes(stats.peakBytesAllocated) << std::endl;
    
    if (!leaks.empty()) {
        file << "\n=== Memory Leaks ===" << std::endl;
        for (const auto& leak : leaks) {
            file << formatMemoryLeak(leak) << std::endl;
        }
    }
}

void MemoryReport::generateHTMLReport(const MemoryStats& stats,
                                     const std::vector<MemoryLeak>& leaks,
                                     const std::string& filePath) {
    std::ofstream file(filePath);
    if (!file.is_open()) return;
    
    file << "<html><head><title>Memory Report</title></head><body>" << std::endl;
    file << "<h1>Memory Profiler Report</h1>" << std::endl;
    file << "<h2>Statistics</h2>" << std::endl;
    file << "<p>Total Allocations: " << stats.totalAllocations << "</p>" << std::endl;
    file << "<p>Current Allocations: " << stats.currentAllocations << "</p>" << std::endl;
    file << "<p>Peak Allocations: " << stats.peakAllocations << "</p>" << std::endl;
    
    if (!leaks.empty()) {
        file << "<h2>Memory Leaks</h2>" << std::endl;
        file << "<ul>" << std::endl;
        for (const auto& leak : leaks) {
            file << "<li>" << formatMemoryLeak(leak) << "</li>" << std::endl;
        }
        file << "</ul>" << std::endl;
    }
    
    file << "</body></html>" << std::endl;
}

std::string MemoryReport::formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        ++unitIndex;
    }
    
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << size << " " << units[unitIndex];
    return oss.str();
}

std::string MemoryReport::formatMemoryLeak(const MemoryLeak& leak) {
    std::ostringstream oss;
    oss << "Leak at " << leak.address << " (" << formatBytes(leak.size) << ") "
        << "from " << leak.file << ":" << leak.line 
        << " (" << leak.function << ") "
        << "age: " << leak.ageInSeconds << "s";
    return oss.str();
}

// MemorySnapshot implementation
MemorySnapshot::MemorySnapshot() {
    std::memset(&stats, 0, sizeof(stats));
}

MemorySnapshot::~MemorySnapshot() {}

void MemorySnapshot::capture() {
    stats = MemoryProfiler::getInstance().getStats();
    timestamp = std::chrono::steady_clock::now();
}

MemoryStats MemorySnapshot::getStats() const {
    return stats;
}

MemorySnapshot MemorySnapshot::diff(const MemorySnapshot& before,
                                    const MemorySnapshot& after) {
    MemorySnapshot result;
    result.stats.totalAllocations = after.stats.totalAllocations - before.stats.totalAllocations;
    result.stats.totalDeallocations = after.stats.totalDeallocations - before.stats.totalDeallocations;
    result.stats.currentAllocations = after.stats.currentAllocations - before.stats.currentAllocations;
    result.stats.totalBytesAllocated = after.stats.totalBytesAllocated - before.stats.totalBytesAllocated;
    result.stats.totalBytesFreed = after.stats.totalBytesFreed - before.stats.totalBytesFreed;
    result.stats.currentBytesAllocated = after.stats.currentBytesAllocated - before.stats.currentBytesAllocated;
    return result;
}

// MemoryBoundsChecker implementation
void* MemoryBoundsChecker::allocateWithGuards(size_t size) {
    size_t totalSize = size + GUARD_SIZE * 2;
    uint8_t* memory = new uint8_t[totalSize];
    
    *reinterpret_cast<uint32_t*>(memory) = GUARD_VALUE;
    *reinterpret_cast<uint32_t*>(memory + GUARD_SIZE + size) = GUARD_VALUE;
    
    return memory + GUARD_SIZE;
}

void MemoryBoundsChecker::deallocateWithGuards(void* ptr) {
    if (!ptr) return;
    
    uint8_t* memory = static_cast<uint8_t*>(ptr) - GUARD_SIZE;
    delete[] memory;
}

bool MemoryBoundsChecker::checkGuards(void* ptr) {
    if (!ptr) return false;
    
    uint8_t* memory = static_cast<uint8_t*>(ptr) - GUARD_SIZE;
    uint32_t frontGuard = *reinterpret_cast<uint32_t*>(memory);
    
    return frontGuard == GUARD_VALUE;
}

} // namespace Debug
} // namespace JJM
