#include "graphics/ShaderCache.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace JJM {
namespace Graphics {

ShaderCache::ShaderCache()
    : m_enabled(true)
    , m_totalCacheSize(0)
    , m_maxCacheSize(256 * 1024 * 1024)  // 256 MB default
    , m_cacheHits(0)
    , m_cacheMisses(0)
{}

ShaderCache::~ShaderCache() {
    shutdown();
}

void ShaderCache::initialize(const std::filesystem::path& cacheDir, size_t maxSizeMB) {
    m_cacheDirectory = cacheDir;
    m_maxCacheSize = maxSizeMB * 1024 * 1024;
    
    // Create cache directory if it doesn't exist
    if (!std::filesystem::exists(m_cacheDirectory)) {
        std::filesystem::create_directories(m_cacheDirectory);
    }
    
    // Load existing cache
    load();
}

void ShaderCache::shutdown() {
    if (m_enabled && !m_entries.empty()) {
        save();
    }
    m_entries.clear();
}

bool ShaderCache::hasEntry(const std::string& shaderName, uint64_t sourceHash) const {
    if (!m_enabled) return false;
    
    auto it = m_entries.find(shaderName);
    if (it != m_entries.end()) {
        return it->second.sourceHash == sourceHash && validateCacheEntry(it->second);
    }
    return false;
}

const ShaderCacheEntry* ShaderCache::getEntry(const std::string& shaderName, uint64_t sourceHash) const {
    if (!m_enabled) return nullptr;
    
    auto it = m_entries.find(shaderName);
    if (it != m_entries.end() && it->second.sourceHash == sourceHash) {
        if (validateCacheEntry(it->second)) {
            const_cast<ShaderCache*>(this)->m_cacheHits++;
            return &it->second;
        }
    }
    
    const_cast<ShaderCache*>(this)->m_cacheMisses++;
    return nullptr;
}

void ShaderCache::addEntry(const std::string& shaderName, uint64_t sourceHash,
                           const std::vector<uint8_t>& binary, uint32_t format) {
    if (!m_enabled || binary.empty()) return;
    
    ShaderCacheEntry entry;
    entry.binary = binary;
    entry.sourceHash = sourceHash;
    entry.timestamp = std::chrono::system_clock::now();
    entry.format = format;
    entry.driverVersion = "1.0";  // TODO: Get actual driver version
    
    // Check if we need to evict old entries
    size_t entrySize = binary.size();
    if (m_totalCacheSize + entrySize > m_maxCacheSize) {
        evictOldestEntries(m_maxCacheSize - entrySize);
    }
    
    // Remove old entry if it exists
    auto it = m_entries.find(shaderName);
    if (it != m_entries.end()) {
        m_totalCacheSize -= it->second.binary.size();
    }
    
    m_entries[shaderName] = std::move(entry);
    m_totalCacheSize += entrySize;
}

void ShaderCache::removeEntry(const std::string& shaderName) {
    auto it = m_entries.find(shaderName);
    if (it != m_entries.end()) {
        m_totalCacheSize -= it->second.binary.size();
        
        // Delete file from disk
        std::string filePath = getCacheFilePath(shaderName);
        if (std::filesystem::exists(filePath)) {
            std::filesystem::remove(filePath);
        }
        
        m_entries.erase(it);
    }
}

void ShaderCache::clear() {
    // Delete all cache files
    if (std::filesystem::exists(m_cacheDirectory)) {
        for (const auto& entry : std::filesystem::directory_iterator(m_cacheDirectory)) {
            if (entry.path().extension() == ".cache") {
                std::filesystem::remove(entry.path());
            }
        }
    }
    
    m_entries.clear();
    m_totalCacheSize = 0;
    m_cacheHits = 0;
    m_cacheMisses = 0;
}

bool ShaderCache::load() {
    if (!std::filesystem::exists(m_cacheDirectory)) {
        return false;
    }
    
    m_entries.clear();
    m_totalCacheSize = 0;
    
    // Load all .cache files
    for (const auto& entry : std::filesystem::directory_iterator(m_cacheDirectory)) {
        if (entry.path().extension() == ".cache") {
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) continue;
            
            // Read header
            uint64_t sourceHash;
            uint64_t timestamp;
            uint32_t format;
            uint32_t binarySize;
            char driverVersion[64];
            
            file.read(reinterpret_cast<char*>(&sourceHash), sizeof(sourceHash));
            file.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
            file.read(reinterpret_cast<char*>(&format), sizeof(format));
            file.read(reinterpret_cast<char*>(&binarySize), sizeof(binarySize));
            file.read(driverVersion, sizeof(driverVersion));
            
            if (!file) continue;
            
            // Read binary data
            std::vector<uint8_t> binary(binarySize);
            file.read(reinterpret_cast<char*>(binary.data()), binarySize);
            
            if (!file) continue;
            
            // Create cache entry
            ShaderCacheEntry cacheEntry;
            cacheEntry.sourceHash = sourceHash;
            cacheEntry.timestamp = std::chrono::system_clock::from_time_t(timestamp);
            cacheEntry.format = format;
            cacheEntry.binary = std::move(binary);
            cacheEntry.driverVersion = driverVersion;
            
            std::string shaderName = entry.path().stem().string();
            m_entries[shaderName] = std::move(cacheEntry);
            m_totalCacheSize += binarySize;
        }
    }
    
    return !m_entries.empty();
}

bool ShaderCache::save() {
    if (!std::filesystem::exists(m_cacheDirectory)) {
        std::filesystem::create_directories(m_cacheDirectory);
    }
    
    for (const auto& [shaderName, entry] : m_entries) {
        std::string filePath = getCacheFilePath(shaderName);
        std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
        
        if (!file) continue;
        
        // Write header
        uint64_t timestamp = std::chrono::system_clock::to_time_t(entry.timestamp);
        uint32_t binarySize = static_cast<uint32_t>(entry.binary.size());
        char driverVersion[64] = {0};
        std::strncpy(driverVersion, entry.driverVersion.c_str(), sizeof(driverVersion) - 1);
        
        file.write(reinterpret_cast<const char*>(&entry.sourceHash), sizeof(entry.sourceHash));
        file.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        file.write(reinterpret_cast<const char*>(&entry.format), sizeof(entry.format));
        file.write(reinterpret_cast<const char*>(&binarySize), sizeof(binarySize));
        file.write(driverVersion, sizeof(driverVersion));
        
        // Write binary data
        file.write(reinterpret_cast<const char*>(entry.binary.data()), entry.binary.size());
        
        if (!file) return false;
    }
    
    return true;
}

ShaderCache::Stats ShaderCache::getStats() const {
    Stats stats;
    stats.hits = m_cacheHits;
    stats.misses = m_cacheMisses;
    stats.totalSize = m_totalCacheSize;
    stats.entryCount = m_entries.size();
    stats.hitRate = (m_cacheHits + m_cacheMisses > 0) 
        ? static_cast<float>(m_cacheHits) / (m_cacheHits + m_cacheMisses)
        : 0.0f;
    return stats;
}

uint64_t ShaderCache::computeHash(const std::string& source) {
    // Simple FNV-1a hash
    uint64_t hash = 14695981039346656037ULL;
    for (char c : source) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

void ShaderCache::cleanup() {
    // Remove entries older than 30 days
    auto now = std::chrono::system_clock::now();
    auto thirtyDaysAgo = now - std::chrono::hours(24 * 30);
    
    std::vector<std::string> toRemove;
    for (const auto& [name, entry] : m_entries) {
        if (entry.timestamp < thirtyDaysAgo) {
            toRemove.push_back(name);
        }
    }
    
    for (const auto& name : toRemove) {
        removeEntry(name);
    }
}

std::string ShaderCache::getCacheFilePath(const std::string& shaderName) const {
    return (m_cacheDirectory / (shaderName + ".cache")).string();
}

bool ShaderCache::validateCacheEntry(const ShaderCacheEntry& entry) const {
    return entry.isValid();
    // TODO: Add driver version validation if needed
}

void ShaderCache::evictOldestEntries(size_t targetSize) {
    if (m_totalCacheSize <= targetSize) return;
    
    // Sort entries by timestamp (oldest first)
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> entries;
    for (const auto& [name, entry] : m_entries) {
        entries.push_back({name, entry.timestamp});
    }
    
    std::sort(entries.begin(), entries.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Remove oldest entries until we reach target size
    for (const auto& [name, _] : entries) {
        if (m_totalCacheSize <= targetSize) break;
        removeEntry(name);
    }
}

} // namespace Graphics
} // namespace JJM
