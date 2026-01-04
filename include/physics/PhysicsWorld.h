#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H

#include "math/Vector2D.h"
#include "physics/PhysicsBody.h"
#include "physics/Collider.h"
#include "ecs/EntityManager.h"
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace JJM {
namespace Physics {

// Raycast hit result
struct RaycastHit {
    ECS::Entity* entity;
    Math::Vector2D point;
    Math::Vector2D normal;
    float distance;
    Collider* collider;
};

// Layer mask for collision filtering
using LayerMask = uint32_t;

// Collision filter callback
using CollisionFilter = std::function<bool(ECS::Entity*, ECS::Entity*)>;

// Physics configuration
struct PhysicsConfig {
    Math::Vector2D gravity;
    float fixedTimeStep;
    int velocityIterations;
    int positionIterations;
    bool continuousCollision;
    float sleepThreshold;
    
    PhysicsConfig()
        : gravity(0.0f, -9.81f)
        , fixedTimeStep(1.0f / 60.0f)
        , velocityIterations(8)
        , positionIterations(3)
        , continuousCollision(true)
        , sleepThreshold(0.01f)
    {}
};

// =============================================================================
// Spatial Hashing - Broad Phase Collision Detection
// =============================================================================

/**
 * @brief Cell key for spatial hash grid
 */
struct SpatialCell {
    int x;
    int y;
    
    bool operator==(const SpatialCell& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Hash function for spatial cells
 */
struct SpatialCellHash {
    size_t operator()(const SpatialCell& cell) const {
        // Combine hashes using a prime multiplier
        return std::hash<int>()(cell.x) ^ (std::hash<int>()(cell.y) << 1);
    }
};

/**
 * @brief AABB for spatial hashing
 */
struct SpatialAABB {
    Math::Vector2D min;
    Math::Vector2D max;
    
    SpatialAABB() = default;
    SpatialAABB(const Math::Vector2D& minPoint, const Math::Vector2D& maxPoint)
        : min(minPoint), max(maxPoint) {}
    
    Math::Vector2D getCenter() const {
        return Math::Vector2D((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    }
    
    Math::Vector2D getExtents() const {
        return Math::Vector2D((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f);
    }
    
    bool intersects(const SpatialAABB& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y);
    }
    
    bool contains(const Math::Vector2D& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }
    
    void expand(float amount) {
        min.x -= amount;
        min.y -= amount;
        max.x += amount;
        max.y += amount;
    }
    
    static SpatialAABB merge(const SpatialAABB& a, const SpatialAABB& b) {
        return SpatialAABB(
            Math::Vector2D(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y)),
            Math::Vector2D(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y))
        );
    }
};

/**
 * @brief Entity entry in spatial hash
 */
struct SpatialEntry {
    ECS::Entity* entity;
    SpatialAABB bounds;
    LayerMask layer;
    bool isStatic;
    
    SpatialEntry()
        : entity(nullptr)
        , layer(0xFFFFFFFF)
        , isStatic(false)
    {}
};

/**
 * @brief Spatial hash grid for broad phase collision detection
 */
class SpatialHashGrid {
private:
    float m_cellSize;
    std::unordered_map<SpatialCell, std::vector<ECS::Entity*>, SpatialCellHash> m_cells;
    std::unordered_map<ECS::Entity*, SpatialEntry> m_entries;
    std::unordered_map<ECS::Entity*, std::vector<SpatialCell>> m_entityCells;
    
    // Statistics
    struct Stats {
        size_t totalEntities;
        size_t totalCells;
        size_t maxEntitiesPerCell;
        size_t queriesPerFrame;
        size_t pairsChecked;
        size_t pairsPassedBroadPhase;
    };
    mutable Stats m_stats;
    
public:
    SpatialHashGrid(float cellSize = 100.0f);
    ~SpatialHashGrid();
    
    // Configuration
    void setCellSize(float size);
    float getCellSize() const { return m_cellSize; }
    
    // Entity management
    void insert(ECS::Entity* entity, const SpatialAABB& bounds, LayerMask layer = 0xFFFFFFFF, bool isStatic = false);
    void remove(ECS::Entity* entity);
    void update(ECS::Entity* entity, const SpatialAABB& newBounds);
    void clear();
    
    // Bulk operations
    void rebuild();
    void optimizeCellSize();
    
    // Queries
    std::vector<ECS::Entity*> query(const SpatialAABB& bounds) const;
    std::vector<ECS::Entity*> queryPoint(const Math::Vector2D& point) const;
    std::vector<ECS::Entity*> queryCircle(const Math::Vector2D& center, float radius) const;
    std::vector<ECS::Entity*> queryRay(const Math::Vector2D& origin, const Math::Vector2D& direction, float maxDistance) const;
    
    // Collision pairs
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs() const;
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs(LayerMask mask) const;
    
    // Neighbor queries
    std::vector<ECS::Entity*> getNeighbors(ECS::Entity* entity) const;
    std::vector<ECS::Entity*> getNearestEntities(const Math::Vector2D& point, int count) const;
    ECS::Entity* getNearestEntity(const Math::Vector2D& point, LayerMask mask = 0xFFFFFFFF) const;
    
    // Statistics
    const Stats& getStats() const { return m_stats; }
    void resetStats();
    size_t getEntityCount() const { return m_entries.size(); }
    size_t getCellCount() const { return m_cells.size(); }
    
    // Debug
    std::vector<std::pair<SpatialCell, size_t>> getCellOccupancy() const;
    
private:
    SpatialCell worldToCell(const Math::Vector2D& position) const;
    std::vector<SpatialCell> getCellsForBounds(const SpatialAABB& bounds) const;
    void addToCell(const SpatialCell& cell, ECS::Entity* entity);
    void removeFromCell(const SpatialCell& cell, ECS::Entity* entity);
};

/**
 * @brief Hierarchical grid for multi-resolution spatial hashing
 */
class HierarchicalSpatialGrid {
private:
    std::vector<std::unique_ptr<SpatialHashGrid>> m_levels;
    std::vector<float> m_levelSizes;
    
public:
    HierarchicalSpatialGrid(const std::vector<float>& levelSizes = {25.0f, 100.0f, 400.0f});
    
    void insert(ECS::Entity* entity, const SpatialAABB& bounds, LayerMask layer = 0xFFFFFFFF);
    void remove(ECS::Entity* entity);
    void update(ECS::Entity* entity, const SpatialAABB& newBounds);
    void clear();
    
    std::vector<ECS::Entity*> query(const SpatialAABB& bounds) const;
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs() const;
    
private:
    int selectLevel(const SpatialAABB& bounds) const;
};

/**
 * @brief Sweep and prune algorithm for broad phase
 */
class SweepAndPrune {
private:
    struct Endpoint {
        float value;
        ECS::Entity* entity;
        bool isMin;
        int axis;
    };
    
    std::vector<Endpoint> m_xEndpoints;
    std::vector<Endpoint> m_yEndpoints;
    std::unordered_map<ECS::Entity*, SpatialEntry> m_entries;
    std::unordered_set<std::pair<ECS::Entity*, ECS::Entity*>*> m_overlappingPairs;
    
public:
    SweepAndPrune();
    
    void insert(ECS::Entity* entity, const SpatialAABB& bounds);
    void remove(ECS::Entity* entity);
    void update(ECS::Entity* entity, const SpatialAABB& newBounds);
    
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs() const;
    
private:
    void sortEndpoints();
    void updateOverlaps();
};

class PhysicsWorld {
private:
    Math::Vector2D gravity;
    ECS::EntityManager* entityManager;
    std::vector<CollisionInfo> collisions;
    PhysicsConfig config;
    
    // Layer collision matrix
    std::vector<std::vector<bool>> layerMatrix;
    static constexpr int MAX_LAYERS = 32;
    
    // Collision callbacks
    std::function<void(const CollisionInfo&)> onCollisionEnter;
    std::function<void(const CollisionInfo&)> onCollisionStay;
    std::function<void(const CollisionInfo&)> onCollisionExit;
    
    // Time accumulator for fixed timestep
    float timeAccumulator;
    
    // Spatial partitioning
    std::unique_ptr<SpatialHashGrid> m_spatialHash;
    bool m_useSpatialHashing;

public:
    PhysicsWorld(ECS::EntityManager* em);
    ~PhysicsWorld();

    // Configuration
    void setConfig(const PhysicsConfig& cfg);
    const PhysicsConfig& getConfig() const { return config; }

    // Update physics
    void update(float deltaTime);
    void fixedUpdate();
    
    // Gravity
    void setGravity(const Math::Vector2D& g) { gravity = g; config.gravity = g; }
    Math::Vector2D getGravity() const { return gravity; }
    
    // Spatial hashing
    void enableSpatialHashing(bool enable, float cellSize = 100.0f);
    bool isSpatialHashingEnabled() const { return m_useSpatialHashing; }
    SpatialHashGrid* getSpatialHash() { return m_spatialHash.get(); }
    void rebuildSpatialHash();
    
    // Collision detection
    void detectCollisions();
    void resolveCollisions();
    
    // Raycasting
    std::optional<RaycastHit> raycast(const Math::Vector2D& origin, 
                                       const Math::Vector2D& direction,
                                       float maxDistance,
                                       LayerMask layerMask = 0xFFFFFFFF) const;
    std::vector<RaycastHit> raycastAll(const Math::Vector2D& origin,
                                        const Math::Vector2D& direction,
                                        float maxDistance,
                                        LayerMask layerMask = 0xFFFFFFFF) const;
    bool linecast(const Math::Vector2D& start, 
                  const Math::Vector2D& end,
                  LayerMask layerMask = 0xFFFFFFFF) const;
    
    // Shape queries
    std::vector<ECS::Entity*> queryPoint(const Math::Vector2D& point);
    std::vector<ECS::Entity*> queryArea(const Math::Vector2D& min, const Math::Vector2D& max);
    std::vector<ECS::Entity*> queryCircle(const Math::Vector2D& center, float radius);
    std::vector<ECS::Entity*> overlapBox(const Math::Vector2D& center, 
                                          const Math::Vector2D& halfExtents,
                                          float angle = 0.0f);
    
    // Layer management
    void setLayerCollision(int layer1, int layer2, bool shouldCollide);
    bool getLayerCollision(int layer1, int layer2) const;
    void resetLayerMatrix();
    
    // Collision callbacks
    void setOnCollisionEnter(std::function<void(const CollisionInfo&)> callback);
    void setOnCollisionStay(std::function<void(const CollisionInfo&)> callback);
    void setOnCollisionExit(std::function<void(const CollisionInfo&)> callback);
    
    // Physics queries
    Math::Vector2D getClosestPoint(const Math::Vector2D& point, const Collider& collider) const;
    float getDistance(ECS::Entity* entityA, ECS::Entity* entityB) const;
    bool checkCollision(ECS::Entity* entityA, ECS::Entity* entityB) const;
    
    // Force application
    void applyExplosionForce(const Math::Vector2D& center, float force, float radius);
    void applyForceInArea(const Math::Vector2D& min, const Math::Vector2D& max, 
                          const Math::Vector2D& force);
    
private:
    void applyGravity(float deltaTime);
    void resolveCollision(const CollisionInfo& info);
    bool rayIntersectsCollider(const Math::Vector2D& origin,
                               const Math::Vector2D& direction,
                               const Collider& collider,
                               RaycastHit& outHit) const;
};

} // namespace Physics
} // namespace JJM

#endif // PHYSICS_WORLD_H
