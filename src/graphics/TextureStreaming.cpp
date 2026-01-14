#include "graphics/TextureStreaming.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace JJM {
namespace Graphics {

// StreamedTexture Implementation
StreamedTexture::StreamedTexture(uint32_t id, const std::string& path)
    : m_id(id)
    , m_path(path)
    , m_state(StreamState::Unloaded)
    , m_loadedMipLevel(0)
    , m_lastAccessFrame(0)
    , m_memoryUsage(0) {
    // Initialize mip levels (placeholder - would load from file)
    // For now, create a simple mip chain
    uint32_t width = 1024;
    uint32_t height = 1024;
    uint32_t mipCount = static_cast<uint32_t>(std::log2(std::max(width, height))) + 1;
    
    m_mipLevels.resize(mipCount);
    for (uint32_t i = 0; i < mipCount; ++i) {
        m_mipLevels[i].width = std::max(1u, width >> i);
        m_mipLevels[i].height = std::max(1u, height >> i);
        m_mipLevels[i].dataSize = m_mipLevels[i].width * m_mipLevels[i].height * 4; // RGBA
        m_mipLevels[i].loaded = false;
    }
}

StreamedTexture::~StreamedTexture() {
    unloadAll();
}

bool StreamedTexture::loadMipLevel(uint32_t level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (level >= m_mipLevels.size()) {
        return false;
    }
    
    MipLevel& mip = m_mipLevels[level];
    if (mip.loaded) {
        return true;
    }
    
    // Allocate and load data (placeholder - would load from disk)
    mip.data.resize(mip.dataSize);
    // TODO: Load actual texture data from file
    
    mip.loaded = true;
    m_memoryUsage += mip.dataSize;
    
    return true;
}

void StreamedTexture::unloadMipLevel(uint32_t level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (level >= m_mipLevels.size()) {
        return;
    }
    
    MipLevel& mip = m_mipLevels[level];
    if (!mip.loaded) {
        return;
    }
    
    m_memoryUsage -= mip.dataSize;
    mip.data.clear();
    mip.data.shrink_to_fit();
    mip.loaded = false;
}

void StreamedTexture::unloadAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& mip : m_mipLevels) {
        if (mip.loaded) {
            mip.data.clear();
            mip.data.shrink_to_fit();
            mip.loaded = false;
        }
    }
    
    m_memoryUsage = 0;
    m_state = StreamState::Unloaded;
}

const MipLevel* StreamedTexture::getMipLevel(uint32_t level) const {
    if (level >= m_mipLevels.size()) {
        return nullptr;
    }
    return &m_mipLevels[level];
}

MipLevel* StreamedTexture::getMipLevel(uint32_t level) {
    if (level >= m_mipLevels.size()) {
        return nullptr;
    }
    return &m_mipLevels[level];
}

// TextureStreamingSystem Implementation
TextureStreamingSystem::TextureStreamingSystem()
    : m_nextTextureId(1)
    , m_currentFrame(0)
    , m_isProcessing(false) {
}

TextureStreamingSystem::~TextureStreamingSystem() {
    m_textures.clear();
}

void TextureStreamingSystem::configure(const StreamingConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

uint32_t TextureStreamingSystem::registerTexture(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint32_t id = m_nextTextureId++;
    auto texture = std::make_unique<StreamedTexture>(id, path);
    m_textures[id] = std::move(texture);
    
    return id;
}

void TextureStreamingSystem::unregisterTexture(uint32_t textureId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_textures.find(textureId);
    if (it != m_textures.end()) {
        size_t memoryFreed = it->second->getMemoryUsage();
        m_stats.currentMemoryUsage -= memoryFreed;
        m_textures.erase(it);
    }
}

void TextureStreamingSystem::requestTexture(uint32_t textureId, StreamPriority priority, float distance) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    
    StreamRequest request;
    request.textureId = textureId;
    request.priority = priority;
    request.distance = distance;
    request.frameRequested = m_currentFrame;
    request.mipLevel = calculateMipLevelFromDistance(distance, 10); // Assume max 10 mips
    
    m_streamQueue.push(request);
    m_stats.pendingStreams = static_cast<uint32_t>(m_streamQueue.size());
}

void TextureStreamingSystem::update(float deltaTime, const float* cameraPosition) {
    m_currentFrame++;
    
    // Process streaming queue
    processStreamQueue();
    
    // Evict textures if needed
    evictIfNeeded();
    
    // Predictive loading
    if (m_config.enablePredictiveLoading && cameraPosition) {
        updatePredictiveLoading(cameraPosition);
    }
    
    // Update stats
    m_stats.activeStreams = 0;
    for (const auto& pair : m_textures) {
        if (pair.second->getState() == StreamState::Loading) {
            m_stats.activeStreams++;
        }
    }
}

void TextureStreamingSystem::flushPendingRequests() {
    while (!m_streamQueue.empty()) {
        processStreamQueue();
    }
}

bool TextureStreamingSystem::isMemoryAvailable(size_t required) const {
    return (m_stats.currentMemoryUsage + required) <= m_config.maxMemoryBudget;
}

void TextureStreamingSystem::evictLeastRecentlyUsed(size_t targetBytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Build list of eviction candidates
    std::vector<std::pair<uint64_t, uint32_t>> candidates;
    for (const auto& pair : m_textures) {
        if (pair.second->getState() != StreamState::Unloaded) {
            candidates.push_back({pair.second->getLastAccessFrame(), pair.first});
        }
    }
    
    // Sort by least recently used
    std::sort(candidates.begin(), candidates.end());
    
    size_t freedMemory = 0;
    for (const auto& candidate : candidates) {
        if (freedMemory >= targetBytes) {
            break;
        }
        
        auto it = m_textures.find(candidate.second);
        if (it != m_textures.end()) {
            size_t memoryUsage = it->second->getMemoryUsage();
            it->second->unloadAll();
            it->second->setState(StreamState::Evicted);
            
            freedMemory += memoryUsage;
            m_stats.currentMemoryUsage -= memoryUsage;
            m_stats.texturesEvicted++;
        }
    }
}

StreamedTexture* TextureStreamingSystem::getTexture(uint32_t textureId) {
    auto it = m_textures.find(textureId);
    if (it != m_textures.end()) {
        return it->second.get();
    }
    return nullptr;
}

const StreamedTexture* TextureStreamingSystem::getTexture(uint32_t textureId) const {
    auto it = m_textures.find(textureId);
    if (it != m_textures.end()) {
        return it->second.get();
    }
    return nullptr;
}

StreamState TextureStreamingSystem::getTextureState(uint32_t textureId) const {
    auto texture = getTexture(textureId);
    return texture ? texture->getState() : StreamState::Unloaded;
}

uint32_t TextureStreamingSystem::getOptimalMipLevel(uint32_t textureId, float distance) const {
    auto texture = getTexture(textureId);
    if (!texture) {
        return 0;
    }
    
    return calculateMipLevelFromDistance(distance, texture->getMaxMipLevels());
}

void TextureStreamingSystem::resetStats() {
    m_stats = StreamingStats();
}

void TextureStreamingSystem::dumpStatus() const {
    std::cout << "=== Texture Streaming Status ===" << std::endl;
    std::cout << "Memory Usage: " << (m_stats.currentMemoryUsage / 1024 / 1024) << " MB / "
              << (m_config.maxMemoryBudget / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Peak Memory: " << (m_stats.peakMemoryUsage / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Textures Loaded: " << m_stats.texturesLoaded << std::endl;
    std::cout << "Textures Evicted: " << m_stats.texturesEvicted << std::endl;
    std::cout << "Active Streams: " << m_stats.activeStreams << std::endl;
    std::cout << "Pending Streams: " << m_stats.pendingStreams << std::endl;
    std::cout << "Cache Misses: " << m_stats.cacheMisses << std::endl;
}

std::vector<uint32_t> TextureStreamingSystem::getLoadedTextures() const {
    std::vector<uint32_t> loaded;
    for (const auto& pair : m_textures) {
        if (pair.second->getState() != StreamState::Unloaded) {
            loaded.push_back(pair.first);
        }
    }
    return loaded;
}

void TextureStreamingSystem::processStreamQueue() {
    std::lock_guard<std::mutex> queueLock(m_queueMutex);
    std::lock_guard<std::mutex> lock(m_mutex);
    
    uint32_t processed = 0;
    while (!m_streamQueue.empty() && processed < m_config.maxConcurrentLoads) {
        StreamRequest request = m_streamQueue.top();
        m_streamQueue.pop();
        
        auto it = m_textures.find(request.textureId);
        if (it == m_textures.end()) {
            m_stats.cacheMisses++;
            continue;
        }
        
        StreamedTexture* texture = it->second.get();
        texture->updateLastAccessFrame(m_currentFrame);
        
        // Check if we need to load this mip level
        if (request.mipLevel < texture->getLoadedMipLevel() || 
            texture->getState() == StreamState::Unloaded) {
            
            // Check memory availability
            const MipLevel* mip = texture->getMipLevel(request.mipLevel);
            if (mip && !mip->loaded) {
                if (isMemoryAvailable(mip->dataSize)) {
                    texture->setState(StreamState::Loading);
                    
                    if (texture->loadMipLevel(request.mipLevel)) {
                        texture->setLoadedMipLevel(request.mipLevel);
                        texture->setState(StreamState::PartiallyLoaded);
                        
                        m_stats.currentMemoryUsage += mip->dataSize;
                        if (m_stats.currentMemoryUsage > m_stats.peakMemoryUsage) {
                            m_stats.peakMemoryUsage = m_stats.currentMemoryUsage;
                        }
                        m_stats.texturesLoaded++;
                    }
                } else {
                    // Not enough memory, put back in queue
                    m_streamQueue.push(request);
                    break;
                }
            }
        }
        
        processed++;
    }
    
    m_stats.pendingStreams = static_cast<uint32_t>(m_streamQueue.size());
}

void TextureStreamingSystem::evictIfNeeded() {
    if (m_stats.currentMemoryUsage > (m_config.maxMemoryBudget - m_config.minMemoryThreshold)) {
        size_t targetFree = m_config.minMemoryThreshold;
        evictLeastRecentlyUsed(targetFree);
    }
}

void TextureStreamingSystem::updatePredictiveLoading(const float* cameraPosition) {
    // Placeholder for predictive loading logic
    // Would analyze camera movement and preload textures in the direction of travel
}

uint32_t TextureStreamingSystem::calculateMipLevelFromDistance(float distance, uint32_t maxMips) const {
    if (distance <= 0.0f) {
        return 0;
    }
    
    // Calculate mip level based on distance
    // Closer = lower mip level (higher detail)
    float mipLevel = std::log2(distance + 1.0f) + m_config.lodBias;
    mipLevel = std::clamp(mipLevel, 0.0f, static_cast<float>(maxMips - 1));
    
    return static_cast<uint32_t>(mipLevel);
}

} // namespace Graphics
} // namespace JJM
