#ifndef SPATIAL_HASH_H
#define SPATIAL_HASH_H

#include "math/Vector2D.h"
#include "physics/Collider.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace JJM {
namespace Physics {

struct GridCell {
    int x;
    int y;
    
    GridCell(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool operator==(const GridCell& other) const {
        return x == other.x && y == other.y;
    }
};

struct GridCellHash {
    std::size_t operator()(const GridCell& cell) const {
        return std::hash<int>()(cell.x) ^ (std::hash<int>()(cell.y) << 1);
    }
};

class SpatialHash {
private:
    float cellSize;
    std::unordered_map<GridCell, std::vector<Collider*>, GridCellHash> grid;
    std::unordered_map<Collider*, std::vector<GridCell>> colliderCells;
    
public:
    SpatialHash(float cellSize = 64.0f);
    ~SpatialHash();
    
    void insert(Collider* collider);
    void remove(Collider* collider);
    void update(Collider* collider);
    void clear();
    
    std::vector<Collider*> query(const Math::Vector2D& position, float radius);
    std::vector<Collider*> queryRect(const Math::Vector2D& min, const Math::Vector2D& max);
    std::vector<Collider*> getNearby(Collider* collider);
    
    int getObjectCount() const;
    int getCellCount() const;
    
private:
    GridCell positionToCell(const Math::Vector2D& position) const;
    std::vector<GridCell> getColliderCells(Collider* collider);
    void insertIntoCell(const GridCell& cell, Collider* collider);
    void removeFromCell(const GridCell& cell, Collider* collider);
};

} // namespace Physics
} // namespace JJM

#endif // SPATIAL_HASH_H
