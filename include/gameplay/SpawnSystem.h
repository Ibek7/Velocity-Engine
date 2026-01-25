#ifndef SPAWN_SYSTEM_H
#define SPAWN_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <queue>

namespace JJM {
namespace Gameplay {

class Entity;

struct SpawnPoint {
    float position[3];
    float rotation[3];
    bool enabled;
    std::string tag;
    
    SpawnPoint() : enabled(true) {
        position[0] = position[1] = position[2] = 0;
        rotation[0] = rotation[1] = rotation[2] = 0;
    }
};

class ObjectPool {
public:
    ObjectPool(const std::string& prefabName, int initialSize);
    ~ObjectPool();
    
    Entity* acquire();
    void release(Entity* entity);
    
    int getActiveCount() const { return m_activeCount; }
    int getPoolSize() const { return static_cast<int>(m_pool.size()); }
    
    void prewarm(int count);
    void clear();
    
private:
    std::string m_prefabName;
    std::vector<Entity*> m_pool;
    std::queue<Entity*> m_available;
    int m_activeCount;
    
    Entity* createEntity();
};

class SpawnSystem {
public:
    SpawnSystem();
    ~SpawnSystem();
    
    void update(float deltaTime);
    
    // Spawn points
    void addSpawnPoint(const SpawnPoint& point);
    void removeSpawnPoint(int index);
    const std::vector<SpawnPoint>& getSpawnPoints() const { return m_spawnPoints; }
    
    SpawnPoint* findSpawnPoint(const std::string& tag);
    SpawnPoint* getRandomSpawnPoint();
    
    // Spawning
    Entity* spawn(const std::string& prefabName, const float position[3]);
    Entity* spawnAt(const std::string& prefabName, int spawnPointIndex);
    Entity* spawnAtTag(const std::string& prefabName, const std::string& tag);
    
    void despawn(Entity* entity);
    
    // Object pooling
    void createPool(const std::string& prefabName, int initialSize);
    ObjectPool* getPool(const std::string& prefabName);
    
    // Wave spawning
    struct Wave {
        std::string prefabName;
        int count;
        float interval;
        std::string spawnTag;
    };
    
    void startWave(const Wave& wave);
    void stopWave();
    bool isWaveActive() const { return m_waveActive; }
    
private:
    std::vector<SpawnPoint> m_spawnPoints;
    std::unordered_map<std::string, ObjectPool*> m_pools;
    
    // Wave spawning
    bool m_waveActive;
    Wave m_currentWave;
    int m_waveSpawned;
    float m_waveTimer;
};

} // namespace Gameplay
} // namespace JJM

#endif
