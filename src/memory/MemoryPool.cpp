#include "memory/MemoryPool.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

namespace JJM {
namespace Memory {

// MemoryBlock implementation
MemoryBlock::MemoryBlock(size_t size) : size(size), allocated(false), next(nullptr) {
    data = std::aligned_alloc(sizeof(void*), size);
    if (!data) {
        throw std::bad_alloc();
    }
}

MemoryBlock::~MemoryBlock() {
    if (data) {
        std::free(data);
    }
}

// MemoryPool implementation
MemoryPool::MemoryPool(size_t blockSize, size_t numBlocks)
    : blockSize(blockSize), poolSize(blockSize * numBlocks),
      poolMemory(nullptr), freeList(nullptr),
      allocatedBytes(0), allocatedBlocks(0) {
    initializePool();
}

MemoryPool::~MemoryPool() {
    for (auto* block : allBlocks) {
        delete block;
    }
    
    if (poolMemory) {
        std::free(poolMemory);
    }
}

void* MemoryPool::allocate() {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (!freeList) {
        return nullptr; // Pool exhausted
    }
    
    MemoryBlock* block = freeList;
    freeList = freeList->getNext();
    
    block->setAllocated(true);
    block->setNext(nullptr);
    
    allocatedBlocks++;
    allocatedBytes += blockSize;
    
    return block->getData();
}

void MemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    std::lock_guard<std::mutex> lock(poolMutex);
    
    MemoryBlock* block = findBlock(ptr);
    if (block && block->isAllocated()) {
        block->setAllocated(false);
        block->setNext(freeList);
        freeList = block;
        
        allocatedBlocks--;
        allocatedBytes -= blockSize;
    }
}

double MemoryPool::getUtilization() const {
    if (allBlocks.empty()) return 0.0;
    return static_cast<double>(allocatedBlocks) / static_cast<double>(allBlocks.size());
}

bool MemoryPool::isValidPointer(void* ptr) const {
    std::lock_guard<std::mutex> lock(poolMutex);
    return findBlock(ptr) != nullptr;
}

void MemoryPool::defragment() {
    // For a simple pool allocator, defragmentation doesn't apply
    // This would be more relevant for a heap allocator
}

void MemoryPool::initializePool() {
    poolMemory = std::aligned_alloc(sizeof(void*), poolSize);
    if (!poolMemory) {
        throw std::bad_alloc();
    }
    
    size_t numBlocks = poolSize / blockSize;
    allBlocks.reserve(numBlocks);
    
    char* current = static_cast<char*>(poolMemory);
    
    for (size_t i = 0; i < numBlocks; i++) {
        MemoryBlock* block = new MemoryBlock(0); // Don't allocate memory in block
        block->data = current;
        block->size = blockSize;
        
        allBlocks.push_back(block);
        
        // Add to free list
        block->setNext(freeList);
        freeList = block;
        
        current += blockSize;
    }
}

MemoryBlock* MemoryPool::findBlock(void* ptr) const {
    for (auto* block : allBlocks) {
        if (block->getData() == ptr) {
            return block;
        }
    }
    return nullptr;
}

// StackAllocator implementation
StackAllocator::StackAllocator(size_t size) : size(size), offset(0) {
    memory = std::aligned_alloc(sizeof(void*), size);
    if (!memory) {
        throw std::bad_alloc();
    }
}

StackAllocator::~StackAllocator() {
    if (memory) {
        std::free(memory);
    }
}

void* StackAllocator::allocate(size_t bytes, size_t alignment) {
    size_t alignedOffset = alignOffset(offset, alignment);
    
    if (alignedOffset + bytes > size) {
        return nullptr; // Not enough space
    }
    
    void* result = static_cast<char*>(memory) + alignedOffset;
    offset = alignedOffset + bytes;
    
    return result;
}

void StackAllocator::pushMarker(const char* label) {
    markers.push_back({offset, label});
}

void StackAllocator::popMarker() {
    if (!markers.empty()) {
        offset = markers.back().position;
        markers.pop_back();
    }
}

void StackAllocator::popToMarker(const char* label) {
    for (int i = static_cast<int>(markers.size()) - 1; i >= 0; i--) {
        if (markers[i].label && std::strcmp(markers[i].label, label) == 0) {
            offset = markers[i].position;
            markers.erase(markers.begin() + i, markers.end());
            return;
        }
    }
}

void StackAllocator::clear() {
    offset = 0;
    markers.clear();
}

double StackAllocator::getUtilization() const {
    return static_cast<double>(offset) / static_cast<double>(size);
}

size_t StackAllocator::alignOffset(size_t offset, size_t alignment) const {
    return (offset + alignment - 1) & ~(alignment - 1);
}

// ObjectPool implementation
ObjectPool::ObjectPool(size_t objectSize, size_t objectsPerChunk)
    : objectSize(objectSize), chunkSize(objectsPerChunk),
      freeList(nullptr), allocatedObjects(0) {
    
    // Ensure object size includes overhead for linking
    this->objectSize = std::max(objectSize, sizeof(ObjectNode*));
    
    allocateNewChunk();
}

ObjectPool::~ObjectPool() {
    for (void* chunk : chunks) {
        std::free(chunk);
    }
}

void* ObjectPool::allocateObject() {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (!freeList) {
        allocateNewChunk();
        if (!freeList) {
            return nullptr;
        }
    }
    
    ObjectNode* node = freeList;
    freeList = freeList->next;
    
    allocatedObjects++;
    
    return node;
}

void ObjectPool::deallocateObject(void* obj) {
    if (!obj) return;
    
    std::lock_guard<std::mutex> lock(poolMutex);
    
    ObjectNode* node = static_cast<ObjectNode*>(obj);
    node->next = freeList;
    freeList = node;
    
    allocatedObjects--;
}

size_t ObjectPool::getTotalObjects() const {
    return chunks.size() * chunkSize;
}

void ObjectPool::allocateNewChunk() {
    size_t chunkBytes = objectSize * chunkSize;
    void* chunk = std::aligned_alloc(alignof(ObjectNode), chunkBytes);
    
    if (!chunk) {
        return;
    }
    
    chunks.push_back(chunk);
    
    // Link all objects in the chunk to the free list
    char* current = static_cast<char*>(chunk);
    
    for (size_t i = 0; i < chunkSize; i++) {
        ObjectNode* node = reinterpret_cast<ObjectNode*>(current);
        node->next = freeList;
        freeList = node;
        current += objectSize;
    }
}

// MemoryManager implementation
MemoryManager* MemoryManager::instance = nullptr;

MemoryManager::MemoryManager() : totalAllocatedBytes(0) {}

MemoryManager* MemoryManager::getInstance() {
    if (!instance) {
        instance = new MemoryManager();
    }
    return instance;
}

MemoryManager::~MemoryManager() {
    shutdown();
}

void MemoryManager::initialize(size_t frameAllocatorSize) {
    frameAllocator = std::make_unique<StackAllocator>(frameAllocatorSize);
}

void MemoryManager::shutdown() {
    pools.clear();
    objectPools.clear();
    frameAllocator.reset();
    namedPools.clear();
    namedObjectPools.clear();
}

MemoryPool* MemoryManager::createPool(const std::string& name, size_t blockSize, size_t numBlocks) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto pool = std::make_unique<MemoryPool>(blockSize, numBlocks);
    MemoryPool* ptr = pool.get();
    
    pools.push_back(std::move(pool));
    namedPools[name] = ptr;
    
    return ptr;
}

MemoryPool* MemoryManager::getPool(const std::string& name) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = namedPools.find(name);
    if (it != namedPools.end()) {
        return it->second;
    }
    
    return nullptr;
}

ObjectPool* MemoryManager::createObjectPool(const std::string& name, size_t objectSize, size_t objectsPerChunk) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto pool = std::make_unique<ObjectPool>(objectSize, objectsPerChunk);
    ObjectPool* ptr = pool.get();
    
    objectPools.push_back(std::move(pool));
    namedObjectPools[name] = ptr;
    
    return ptr;
}

ObjectPool* MemoryManager::getObjectPool(const std::string& name) {
    std::lock_guard<std::mutex> lock(managerMutex);
    
    auto it = namedObjectPools.find(name);
    if (it != namedObjectPools.end()) {
        return it->second;
    }
    
    return nullptr;
}

void* MemoryManager::allocate(size_t size, const std::string& poolName) {
    if (!poolName.empty()) {
        MemoryPool* pool = getPool(poolName);
        if (pool) {
            return pool->allocate();
        }
    }
    
    MemoryPool* pool = findBestFitPool(size);
    if (pool) {
        return pool->allocate();
    }
    
    // Fallback to regular malloc
    totalAllocatedBytes += size;
    return std::malloc(size);
}

void MemoryManager::deallocate(void* ptr, const std::string& poolName) {
    if (!ptr) return;
    
    if (!poolName.empty()) {
        MemoryPool* pool = getPool(poolName);
        if (pool && pool->isValidPointer(ptr)) {
            pool->deallocate(ptr);
            return;
        }
    }
    
    // Try to find in any pool
    for (const auto& pool : pools) {
        if (pool->isValidPointer(ptr)) {
            pool->deallocate(ptr);
            return;
        }
    }
    
    // Fallback to regular free
    std::free(ptr);
}

void MemoryManager::resetFrameAllocator() {
    if (frameAllocator) {
        frameAllocator->clear();
    }
}

void MemoryManager::printStatistics() const {
    std::cout << getStatisticsReport() << std::endl;
}

std::string MemoryManager::getStatisticsReport() const {
    std::stringstream ss;
    ss << "=== Memory Manager Statistics ===" << std::endl;
    ss << "Total Allocated Bytes: " << totalAllocatedBytes << std::endl;
    
    ss << "\n--- Memory Pools ---" << std::endl;
    for (const auto& pair : namedPools) {
        const auto& pool = pair.second;
        ss << pair.first << ": " << pool->getAllocatedBlocks() << "/" << pool->getTotalBlocks()
           << " blocks (" << (pool->getUtilization() * 100.0) << "% util)" << std::endl;
    }
    
    ss << "\n--- Object Pools ---" << std::endl;
    for (const auto& pair : namedObjectPools) {
        const auto& pool = pair.second;
        ss << pair.first << ": " << pool->getAllocatedObjects() << "/" << pool->getTotalObjects()
           << " objects" << std::endl;
    }
    
    if (frameAllocator) {
        ss << "\n--- Frame Allocator ---" << std::endl;
        ss << "Used: " << frameAllocator->getUsedBytes() << "/" << frameAllocator->getTotalBytes()
           << " bytes (" << (frameAllocator->getUtilization() * 100.0) << "% util)" << std::endl;
    }
    
    return ss.str();
}

MemoryPool* MemoryManager::findBestFitPool(size_t size) {
    MemoryPool* bestFit = nullptr;
    size_t smallestFit = SIZE_MAX;
    
    for (const auto& pool : pools) {
        if (pool->getBlockSize() >= size && pool->getBlockSize() < smallestFit && pool->getFreeBlocks() > 0) {
            bestFit = pool.get();
            smallestFit = pool->getBlockSize();
        }
    }
    
    return bestFit;
}

// FrameAllocatorScope implementation
FrameAllocatorScope::FrameAllocatorScope(const char* label)
    : allocator(MemoryManager::getInstance()->getFrameAllocator()) {
    if (allocator) {
        allocator->pushMarker(label);
    }
}

FrameAllocatorScope::~FrameAllocatorScope() {
    if (allocator) {
        allocator->popMarker();
    }
}

} // namespace Memory
} // namespace JJM