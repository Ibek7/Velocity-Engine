/**
 * @file ComponentPool.h
 * @brief Component memory pooling system for improved cache efficiency
 * @version 1.0.0
 * @date 2026-01-16
 */

#ifndef COMPONENT_POOL_H
#define COMPONENT_POOL_H

#include <vector>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace JJM {
namespace ECS {

/**
 * @brief Pool allocator for component memory management
 * 
 * Provides contiguous memory allocation for components of the same type
 * to improve cache coherency and reduce memory fragmentation.
 */
template<typename T>
class ComponentPool {
private:
    struct Block {
        std::unique_ptr<T[]> data;
        std::vector<bool> occupied;
        size_t capacity;
        size_t used;
        
        Block(size_t cap) : capacity(cap), used(0) {
            data = std::make_unique<T[]>(capacity);
            occupied.resize(capacity, false);
        }
    };
    
    std::vector<std::unique_ptr<Block>> m_blocks;
    size_t m_blockSize;
    size_t m_totalAllocated;
    size_t m_totalUsed;
    
    // Free list for reusing slots
    std::vector<std::pair<size_t, size_t>> m_freeList; // (blockIndex, slotIndex)
    
public:
    /**
     * @brief Construct component pool with specified block size
     * @param blockSize Number of components per memory block
     */
    explicit ComponentPool(size_t blockSize = 1024) 
        : m_blockSize(blockSize)
        , m_totalAllocated(0)
        , m_totalUsed(0) 
    {
        // Pre-allocate first block
        allocateBlock();
    }
    
    ~ComponentPool() = default;
    
    /**
     * @brief Allocate component from pool
     * @param args Constructor arguments
     * @return Pointer to constructed component
     */
    template<typename... Args>
    T* allocate(Args&&... args) {
        size_t blockIdx, slotIdx;
        
        // Try to reuse from free list
        if (!m_freeList.empty()) {
            auto [bIdx, sIdx] = m_freeList.back();
            m_freeList.pop_back();
            blockIdx = bIdx;
            slotIdx = sIdx;
        } else {
            // Find block with available slot
            blockIdx = findAvailableBlock();
            slotIdx = findAvailableSlot(blockIdx);
        }
        
        Block* block = m_blocks[blockIdx].get();
        assert(!block->occupied[slotIdx] && "Slot should not be occupied");
        
        // Construct in place
        T* component = &block->data[slotIdx];
        new (component) T(std::forward<Args>(args)...);
        
        block->occupied[slotIdx] = true;
        block->used++;
        m_totalUsed++;
        
        return component;
    }
    
    /**
     * @brief Deallocate component and return to pool
     * @param component Pointer to component to deallocate
     */
    void deallocate(T* component) {
        if (!component) return;
        
        // Find which block contains this component
        for (size_t blockIdx = 0; blockIdx < m_blocks.size(); blockIdx++) {
            Block* block = m_blocks[blockIdx].get();
            T* blockStart = block->data.get();
            T* blockEnd = blockStart + block->capacity;
            
            if (component >= blockStart && component < blockEnd) {
                size_t slotIdx = component - blockStart;
                
                if (block->occupied[slotIdx]) {
                    // Call destructor
                    component->~T();
                    
                    block->occupied[slotIdx] = false;
                    block->used--;
                    m_totalUsed--;
                    
                    // Add to free list
                    m_freeList.emplace_back(blockIdx, slotIdx);
                }
                return;
            }
        }
    }
    
    /**
     * @brief Get total number of allocated components
     */
    size_t getAllocatedCount() const { return m_totalAllocated; }
    
    /**
     * @brief Get number of currently used components
     */
    size_t getUsedCount() const { return m_totalUsed; }
    
    /**
     * @brief Get memory usage in bytes
     */
    size_t getMemoryUsage() const {
        return m_totalAllocated * sizeof(T);
    }
    
    /**
     * @brief Get fragmentation ratio (0.0 = no fragmentation, 1.0 = fully fragmented)
     */
    float getFragmentation() const {
        if (m_totalAllocated == 0) return 0.0f;
        return 1.0f - (static_cast<float>(m_totalUsed) / m_totalAllocated);
    }
    
    /**
     * @brief Defragment pool by compacting used components
     */
    void defragment() {
        // TODO: Implement defragmentation by moving components to fill gaps
        // This would require updating pointers/references, which is complex
        // For now, just clear empty blocks
        compactBlocks();
    }
    
    /**
     * @brief Clear all components and reset pool
     */
    void clear() {
        for (auto& block : m_blocks) {
            for (size_t i = 0; i < block->capacity; i++) {
                if (block->occupied[i]) {
                    block->data[i].~T();
                }
            }
        }
        m_blocks.clear();
        m_freeList.clear();
        m_totalAllocated = 0;
        m_totalUsed = 0;
        
        // Allocate first block
        allocateBlock();
    }
    
private:
    void allocateBlock() {
        m_blocks.push_back(std::make_unique<Block>(m_blockSize));
        m_totalAllocated += m_blockSize;
    }
    
    size_t findAvailableBlock() {
        for (size_t i = 0; i < m_blocks.size(); i++) {
            if (m_blocks[i]->used < m_blocks[i]->capacity) {
                return i;
            }
        }
        
        // All blocks full, allocate new one
        allocateBlock();
        return m_blocks.size() - 1;
    }
    
    size_t findAvailableSlot(size_t blockIdx) {
        Block* block = m_blocks[blockIdx].get();
        for (size_t i = 0; i < block->capacity; i++) {
            if (!block->occupied[i]) {
                return i;
            }
        }
        assert(false && "Block should have available slot");
        return 0;
    }
    
    void compactBlocks() {
        // Remove completely empty blocks (except the first one)
        auto it = m_blocks.begin() + 1; // Keep first block
        while (it != m_blocks.end()) {
            if ((*it)->used == 0) {
                m_totalAllocated -= (*it)->capacity;
                it = m_blocks.erase(it);
            } else {
                ++it;
            }
        }
        
        // Update free list to remove references to deleted blocks
        m_freeList.erase(
            std::remove_if(m_freeList.begin(), m_freeList.end(),
                [this](const auto& pair) {
                    return pair.first >= m_blocks.size();
                }),
            m_freeList.end()
        );
    }
};

/**
 * @brief Component array with pool-based allocation
 * 
 * Provides array-like interface with contiguous memory for better cache performance
 */
template<typename T>
class ComponentArray {
private:
    std::vector<T> m_components;
    std::vector<size_t> m_entityToIndex;
    std::vector<size_t> m_indexToEntity;
    
public:
    /**
     * @brief Add component for entity
     */
    template<typename... Args>
    T* add(size_t entityId, Args&&... args) {
        if (entityId >= m_entityToIndex.size()) {
            m_entityToIndex.resize(entityId + 1, SIZE_MAX);
        }
        
        size_t index = m_components.size();
        m_components.emplace_back(std::forward<Args>(args)...);
        m_entityToIndex[entityId] = index;
        m_indexToEntity.push_back(entityId);
        
        return &m_components.back();
    }
    
    /**
     * @brief Remove component for entity
     */
    void remove(size_t entityId) {
        if (entityId >= m_entityToIndex.size()) return;
        
        size_t index = m_entityToIndex[entityId];
        if (index == SIZE_MAX) return;
        
        // Swap with last element
        size_t lastIndex = m_components.size() - 1;
        if (index != lastIndex) {
            m_components[index] = std::move(m_components[lastIndex]);
            size_t lastEntity = m_indexToEntity[lastIndex];
            m_entityToIndex[lastEntity] = index;
            m_indexToEntity[index] = lastEntity;
        }
        
        m_components.pop_back();
        m_indexToEntity.pop_back();
        m_entityToIndex[entityId] = SIZE_MAX;
    }
    
    /**
     * @brief Get component for entity
     */
    T* get(size_t entityId) {
        if (entityId >= m_entityToIndex.size()) return nullptr;
        size_t index = m_entityToIndex[entityId];
        return (index != SIZE_MAX) ? &m_components[index] : nullptr;
    }
    
    /**
     * @brief Check if entity has component
     */
    bool has(size_t entityId) const {
        return entityId < m_entityToIndex.size() && 
               m_entityToIndex[entityId] != SIZE_MAX;
    }
    
    /**
     * @brief Get all components
     */
    std::vector<T>& getComponents() { return m_components; }
    const std::vector<T>& getComponents() const { return m_components; }
    
    /**
     * @brief Get component count
     */
    size_t size() const { return m_components.size(); }
    
    /**
     * @brief Clear all components
     */
    void clear() {
        m_components.clear();
        m_entityToIndex.clear();
        m_indexToEntity.clear();
    }
};

} // namespace ECS
} // namespace JJM

#endif // COMPONENT_POOL_H
