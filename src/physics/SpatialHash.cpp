#include "physics/SpatialHash.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Physics {

SpatialHash::SpatialHash(float cellSize) : cellSize(cellSize) {
}

SpatialHash::~SpatialHash() {
    clear();
}

void SpatialHash::insert(Collider* collider) {
    if (!collider) return;
    
    auto cells = getColliderCells(collider);
    colliderCells[collider] = cells;
    
    for (const auto& cell : cells) {
        insertIntoCell(cell, collider);
    }
}

void SpatialHash::remove(Collider* collider) {
    if (!collider) return;
    
    auto it = colliderCells.find(collider);
    if (it != colliderCells.end()) {
        for (const auto& cell : it->second) {
            removeFromCell(cell, collider);
        }
        colliderCells.erase(it);
    }
}

void SpatialHash::update(Collider* collider) {
    if (!collider) return;
    
    auto it = colliderCells.find(collider);
    if (it != colliderCells.end()) {
        auto newCells = getColliderCells(collider);
        auto& oldCells = it->second;
        
        // Remove from old cells that are not in new cells
        for (const auto& oldCell : oldCells) {
            if (std::find(newCells.begin(), newCells.end(), oldCell) == newCells.end()) {
                removeFromCell(oldCell, collider);
            }
        }
        
        // Add to new cells that are not in old cells
        for (const auto& newCell : newCells) {
            if (std::find(oldCells.begin(), oldCells.end(), newCell) == oldCells.end()) {
                insertIntoCell(newCell, collider);
            }
        }
        
        it->second = newCells;
    } else {
        insert(collider);
    }
}

void SpatialHash::clear() {
    grid.clear();
    colliderCells.clear();
}

std::vector<Collider*> SpatialHash::query(const Math::Vector2D& position, float radius) {
    std::unordered_set<Collider*> result;
    
    // Calculate grid bounds for the query
    int minX = static_cast<int>(std::floor((position.x - radius) / cellSize));
    int maxX = static_cast<int>(std::floor((position.x + radius) / cellSize));
    int minY = static_cast<int>(std::floor((position.y - radius) / cellSize));
    int maxY = static_cast<int>(std::floor((position.y + radius) / cellSize));
    
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            GridCell cell(x, y);
            auto it = grid.find(cell);
            if (it != grid.end()) {
                for (auto* collider : it->second) {
                    result.insert(collider);
                }
            }
        }
    }
    
    return std::vector<Collider*>(result.begin(), result.end());
}

std::vector<Collider*> SpatialHash::queryRect(const Math::Vector2D& min, const Math::Vector2D& max) {
    std::unordered_set<Collider*> result;
    
    int minX = static_cast<int>(std::floor(min.x / cellSize));
    int maxX = static_cast<int>(std::floor(max.x / cellSize));
    int minY = static_cast<int>(std::floor(min.y / cellSize));
    int maxY = static_cast<int>(std::floor(max.y / cellSize));
    
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            GridCell cell(x, y);
            auto it = grid.find(cell);
            if (it != grid.end()) {
                for (auto* collider : it->second) {
                    result.insert(collider);
                }
            }
        }
    }
    
    return std::vector<Collider*>(result.begin(), result.end());
}

std::vector<Collider*> SpatialHash::getNearby(Collider* collider) {
    if (!collider) return {};
    
    std::unordered_set<Collider*> result;
    
    auto it = colliderCells.find(collider);
    if (it != colliderCells.end()) {
        for (const auto& cell : it->second) {
            auto cellIt = grid.find(cell);
            if (cellIt != grid.end()) {
                for (auto* other : cellIt->second) {
                    if (other != collider) {
                        result.insert(other);
                    }
                }
            }
        }
    }
    
    return std::vector<Collider*>(result.begin(), result.end());
}

int SpatialHash::getObjectCount() const {
    return static_cast<int>(colliderCells.size());
}

int SpatialHash::getCellCount() const {
    return static_cast<int>(grid.size());
}

GridCell SpatialHash::positionToCell(const Math::Vector2D& position) const {
    return GridCell(
        static_cast<int>(std::floor(position.x / cellSize)),
        static_cast<int>(std::floor(position.y / cellSize))
    );
}

std::vector<GridCell> SpatialHash::getColliderCells(Collider* collider) {
    std::vector<GridCell> cells;
    
    if (!collider) return cells;
    
    Math::Vector2D pos = collider->getPosition();
    
    if (collider->type == ColliderType::CIRCLE) {
        auto* circle = static_cast<CircleCollider*>(collider);
        float r = circle->radius;
        
        int minX = static_cast<int>(std::floor((pos.x - r) / cellSize));
        int maxX = static_cast<int>(std::floor((pos.x + r) / cellSize));
        int minY = static_cast<int>(std::floor((pos.y - r) / cellSize));
        int maxY = static_cast<int>(std::floor((pos.y + r) / cellSize));
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                cells.push_back(GridCell(x, y));
            }
        }
    } else if (collider->type == ColliderType::BOX) {
        auto* box = static_cast<BoxCollider*>(collider);
        Math::Vector2D min = box->getMin();
        Math::Vector2D max = box->getMax();
        
        int minX = static_cast<int>(std::floor(min.x / cellSize));
        int maxX = static_cast<int>(std::floor(max.x / cellSize));
        int minY = static_cast<int>(std::floor(min.y / cellSize));
        int maxY = static_cast<int>(std::floor(max.y / cellSize));
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                cells.push_back(GridCell(x, y));
            }
        }
    }
    
    return cells;
}

void SpatialHash::insertIntoCell(const GridCell& cell, Collider* collider) {
    grid[cell].push_back(collider);
}

void SpatialHash::removeFromCell(const GridCell& cell, Collider* collider) {
    auto it = grid.find(cell);
    if (it != grid.end()) {
        auto& colliders = it->second;
        colliders.erase(
            std::remove(colliders.begin(), colliders.end(), collider),
            colliders.end()
        );
        
        if (colliders.empty()) {
            grid.erase(it);
        }
    }
}

} // namespace Physics
} // namespace JJM
