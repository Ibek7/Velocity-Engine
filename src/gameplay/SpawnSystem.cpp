#include "gameplay/SpawnSystem.h"
#include <algorithm>
#include <cstdlib>

namespace JJM {
namespace Gameplay {

ObjectPool::ObjectPool(const std::string& prefabName, int initialSize)
    : m_prefabName(prefabName), m_activeCount(0) {
    prewarm(initialSize);
}

ObjectPool::~ObjectPool() {
    clear();
}

Entity* ObjectPool::acquire() {
    if (m_available.empty()) {
        // Grow pool
        Entity* entity = createEntity();
        m_pool.push_back(entity);
        m_activeCount++;
        return entity;
    }
    
    Entity* entity = m_available.front();
    m_available.pop();
    m_activeCount++;
    return entity;
}

void ObjectPool::release(Entity* entity) {
    m_available.push(entity);
    m_activeCount--;
}

void ObjectPool::prewarm(int count) {
    for (int i = 0; i < count; ++i) {
        Entity* entity = createEntity();
        m_pool.push_back(entity);
        m_available.push(entity);
    }
}

void ObjectPool::clear() {
    // Delete all entities
    for (Entity* entity : m_pool) {
        // delete entity;
    }
    m_pool.clear();
    while (!m_available.empty()) {
        m_available.pop();
    }
    m_activeCount = 0;
}

Entity* ObjectPool::createEntity() {
    // Create entity from prefab
    return nullptr;  // Stub
}

SpawnSystem::SpawnSystem()
    : m_waveActive(false), m_waveSpawned(0), m_waveTimer(0.0f) {
}

SpawnSystem::~SpawnSystem() {
    for (auto& pair : m_pools) {
        delete pair.second;
    }
}

void SpawnSystem::update(float deltaTime) {
    if (!m_waveActive) return;
    
    m_waveTimer += deltaTime;
    
    if (m_waveTimer >= m_currentWave.interval && m_waveSpawned < m_currentWave.count) {
        spawnAtTag(m_currentWave.prefabName, m_currentWave.spawnTag);
        m_waveSpawned++;
        m_waveTimer = 0.0f;
    }
    
    if (m_waveSpawned >= m_currentWave.count) {
        m_waveActive = false;
    }
}

void SpawnSystem::addSpawnPoint(const SpawnPoint& point) {
    m_spawnPoints.push_back(point);
}

void SpawnSystem::removeSpawnPoint(int index) {
    if (index >= 0 && index < static_cast<int>(m_spawnPoints.size())) {
        m_spawnPoints.erase(m_spawnPoints.begin() + index);
    }
}

SpawnPoint* SpawnSystem::findSpawnPoint(const std::string& tag) {
    for (auto& point : m_spawnPoints) {
        if (point.tag == tag && point.enabled) {
            return &point;
        }
    }
    return nullptr;
}

SpawnPoint* SpawnSystem::getRandomSpawnPoint() {
    if (m_spawnPoints.empty()) return nullptr;
    
    std::vector<SpawnPoint*> enabled;
    for (auto& point : m_spawnPoints) {
        if (point.enabled) {
            enabled.push_back(&point);
        }
    }
    
    if (enabled.empty()) return nullptr;
    
    int index = rand() % enabled.size();
    return enabled[index];
}

Entity* SpawnSystem::spawn(const std::string& prefabName, const float position[3]) {
    ObjectPool* pool = getPool(prefabName);
    Entity* entity = nullptr;
    
    if (pool) {
        entity = pool->acquire();
    } else {
        // Create without pooling
        // entity = createEntityFromPrefab(prefabName);
    }
    
    if (entity) {
        // Set position
    }
    
    return entity;
}

Entity* SpawnSystem::spawnAt(const std::string& prefabName, int spawnPointIndex) {
    if (spawnPointIndex < 0 || spawnPointIndex >= static_cast<int>(m_spawnPoints.size())) {
        return nullptr;
    }
    
    const SpawnPoint& point = m_spawnPoints[spawnPointIndex];
    return spawn(prefabName, point.position);
}

Entity* SpawnSystem::spawnAtTag(const std::string& prefabName, const std::string& tag) {
    SpawnPoint* point = findSpawnPoint(tag);
    if (!point) return nullptr;
    
    return spawn(prefabName, point->position);
}

void SpawnSystem::despawn(Entity* entity) {
    // Find which pool this belongs to and return it
    // For now, stub
}

void SpawnSystem::createPool(const std::string& prefabName, int initialSize) {
    if (m_pools.find(prefabName) != m_pools.end()) {
        return;  // Already exists
    }
    
    m_pools[prefabName] = new ObjectPool(prefabName, initialSize);
}

ObjectPool* SpawnSystem::getPool(const std::string& prefabName) {
    auto it = m_pools.find(prefabName);
    return it != m_pools.end() ? it->second : nullptr;
}

void SpawnSystem::startWave(const Wave& wave) {
    m_currentWave = wave;
    m_waveActive = true;
    m_waveSpawned = 0;
    m_waveTimer = 0.0f;
}

void SpawnSystem::stopWave() {
    m_waveActive = false;
}

} // namespace Gameplay
} // namespace JJM
