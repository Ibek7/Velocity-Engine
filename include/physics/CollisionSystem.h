#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "physics/Collider.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>

namespace JJM {
namespace Physics {

/**
 * @brief Spatial hash grid for broad-phase collision detection
 */
class SpatialHashGrid {
public:
    SpatialHashGrid(float cellSize = 10.0f);
    
    void insert(Collider* collider);
    void remove(Collider* collider);
    void clear();
    void update();
    
    std::vector<Collider*> query(const Math::Vector2D& position, float radius) const;
    std::vector<std::pair<Collider*, Collider*>> getPotentialCollisions() const;
    
    void setCellSize(float size) { cellSize = size; }
    float getCellSize() const { return cellSize; }
    
private:
    struct Cell {
        int x, y;
        bool operator==(const Cell& other) const {
            return x == other.x && y == other.y;
        }
    };
    
    struct CellHash {
        size_t operator()(const Cell& cell) const {
            return std::hash<int>()(cell.x) ^ (std::hash<int>()(cell.y) << 1);
        }
    };
    
    float cellSize;
    std::unordered_map<Cell, std::vector<Collider*>, CellHash> grid;
    
    Cell getCell(const Math::Vector2D& position) const;
    std::vector<Cell> getCells(Collider* collider) const;
};

/**
 * @brief Raycast hit information
 */
struct RaycastHit {
    Collider* collider;
    Math::Vector2D point;
    Math::Vector2D normal;
    float distance;
    
    RaycastHit() : collider(nullptr), distance(0) {}
};

/**
 * @brief Manages collision detection and response
 */
class CollisionSystem {
public:
    CollisionSystem();
    ~CollisionSystem() = default;
    
    void addCollider(Collider* collider);
    void removeCollider(Collider* collider);
    void update(float deltaTime);
    void clear();
    
    // Callbacks
    using CollisionCallback = std::function<void(Collider*, Collider*, const CollisionInfo&)>;
    void setOnCollisionEnter(CollisionCallback callback) { onCollisionEnter = callback; }
    void setOnCollisionStay(CollisionCallback callback) { onCollisionStay = callback; }
    void setOnCollisionExit(CollisionCallback callback) { onCollisionExit = callback; }
    
    // Queries
    std::vector<Collider*> queryRegion(const Math::Vector2D& center, float radius) const;
    Collider* raycast(const Math::Vector2D& origin, const Math::Vector2D& direction, 
                      float maxDistance, RaycastHit& hit) const;
    std::vector<Collider*> raycastAll(const Math::Vector2D& origin, const Math::Vector2D& direction,
                                       float maxDistance) const;
    
    // Spatial partitioning
    void setUseSpatialHash(bool use) { useSpatialHash = use; }
    void setSpatialHashCellSize(float size) { spatialHash.setCellSize(size); }
    
    // Statistics
    int getColliderCount() const { return static_cast<int>(colliders.size()); }
    int getCollisionCount() const { return collisionCount; }
    
private:
    std::vector<Collider*> colliders;
    SpatialHashGrid spatialHash;
    bool useSpatialHash;
    
    // Track collision pairs for enter/exit callbacks
    struct CollisionPair {
        Collider* a;
        Collider* b;
        bool operator==(const CollisionPair& other) const {
            return (a == other.a && b == other.b) || (a == other.b && b == other.a);
        }
    };
    
    struct CollisionPairHash {
        size_t operator()(const CollisionPair& pair) const {
            size_t h1 = std::hash<void*>()(pair.a);
            size_t h2 = std::hash<void*>()(pair.b);
            return h1 ^ (h2 << 1);
        }
    };
    
    std::unordered_map<CollisionPair, bool, CollisionPairHash> activeCollisions;
    int collisionCount;
    
    CollisionCallback onCollisionEnter;
    CollisionCallback onCollisionStay;
    CollisionCallback onCollisionExit;
    
    void detectCollisionsBruteForce();
    void detectCollisionsSpatialHash();
    void processCollisionPair(Collider* a, Collider* b);
};

} // namespace Physics
} // namespace JJM

#endif // COLLISION_SYSTEM_H
