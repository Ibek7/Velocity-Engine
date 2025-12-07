#include "physics/CollisionSystem.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Physics {

// SpatialHashGrid implementation
SpatialHashGrid::SpatialHashGrid(float cellSize)
    : cellSize(cellSize) {
}

SpatialHashGrid::Cell SpatialHashGrid::getCell(const Math::Vector2D& position) const {
    return Cell{
        static_cast<int>(std::floor(position.x / cellSize)),
        static_cast<int>(std::floor(position.y / cellSize))
    };
}

std::vector<SpatialHashGrid::Cell> SpatialHashGrid::getCells(Collider* collider) const {
    std::vector<Cell> cells;
    Math::Vector2D pos = collider->getPosition();
    
    // For simplicity, just use collider's position cell
    // A more robust implementation would cover all cells the collider overlaps
    cells.push_back(getCell(pos));
    
    return cells;
}

void SpatialHashGrid::insert(Collider* collider) {
    auto cells = getCells(collider);
    for (const auto& cell : cells) {
        grid[cell].push_back(collider);
    }
}

void SpatialHashGrid::remove(Collider* collider) {
    auto cells = getCells(collider);
    for (const auto& cell : cells) {
        auto& cellColliders = grid[cell];
        cellColliders.erase(
            std::remove(cellColliders.begin(), cellColliders.end(), collider),
            cellColliders.end()
        );
    }
}

void SpatialHashGrid::clear() {
    grid.clear();
}

void SpatialHashGrid::update() {
    // Rebuild grid
    clear();
}

std::vector<Collider*> SpatialHashGrid::query(const Math::Vector2D& position, float radius) const {
    std::vector<Collider*> result;
    
    // Get cells in radius
    int cellRadius = static_cast<int>(std::ceil(radius / cellSize));
    Cell center = getCell(position);
    
    for (int x = center.x - cellRadius; x <= center.x + cellRadius; ++x) {
        for (int y = center.y - cellRadius; y <= center.y + cellRadius; ++y) {
            Cell cell{x, y};
            auto it = grid.find(cell);
            if (it != grid.end()) {
                for (Collider* collider : it->second) {
                    Math::Vector2D diff = collider->getPosition() - position;
                    if (diff.magnitude() <= radius) {
                        result.push_back(collider);
                    }
                }
            }
        }
    }
    
    return result;
}

std::vector<std::pair<Collider*, Collider*>> SpatialHashGrid::getPotentialCollisions() const {
    std::vector<std::pair<Collider*, Collider*>> pairs;
    
    for (const auto& [cell, colliders] : grid) {
        // Check colliders within same cell
        for (size_t i = 0; i < colliders.size(); ++i) {
            for (size_t j = i + 1; j < colliders.size(); ++j) {
                pairs.push_back({colliders[i], colliders[j]});
            }
        }
    }
    
    return pairs;
}

// CollisionSystem implementation
CollisionSystem::CollisionSystem()
    : useSpatialHash(true), collisionCount(0) {
}

void CollisionSystem::addCollider(Collider* collider) {
    if (!collider) return;
    
    colliders.push_back(collider);
    
    if (useSpatialHash) {
        spatialHash.insert(collider);
    }
}

void CollisionSystem::removeCollider(Collider* collider) {
    if (!collider) return;
    
    colliders.erase(
        std::remove(colliders.begin(), colliders.end(), collider),
        colliders.end()
    );
    
    if (useSpatialHash) {
        spatialHash.remove(collider);
    }
}

void CollisionSystem::update(float deltaTime) {
    (void)deltaTime; // Unused in current implementation
    
    collisionCount = 0;
    
    // Track previous collisions
    std::unordered_map<CollisionPair, bool, CollisionPairHash> previousCollisions = activeCollisions;
    activeCollisions.clear();
    
    // Detect collisions
    if (useSpatialHash) {
        detectCollisionsSpatialHash();
    } else {
        detectCollisionsBruteForce();
    }
    
    // Process exit callbacks for collisions that ended
    for (const auto& [pair, _] : previousCollisions) {
        if (activeCollisions.find(pair) == activeCollisions.end()) {
            if (onCollisionExit) {
                CollisionInfo info;
                onCollisionExit(pair.a, pair.b, info);
            }
        }
    }
}

void CollisionSystem::detectCollisionsBruteForce() {
    for (size_t i = 0; i < colliders.size(); ++i) {
        for (size_t j = i + 1; j < colliders.size(); ++j) {
            processCollisionPair(colliders[i], colliders[j]);
        }
    }
}

void CollisionSystem::detectCollisionsSpatialHash() {
    auto pairs = spatialHash.getPotentialCollisions();
    for (const auto& [a, b] : pairs) {
        processCollisionPair(a, b);
    }
}

void CollisionSystem::processCollisionPair(Collider* a, Collider* b) {
    CollisionInfo info;
    
    if (a->checkCollision(b, info)) {
        collisionCount++;
        
        CollisionPair pair{a, b};
        bool wasColliding = activeCollisions.find(pair) != activeCollisions.end();
        activeCollisions[pair] = true;
        
        if (wasColliding) {
            if (onCollisionStay) {
                onCollisionStay(a, b, info);
            }
        } else {
            if (onCollisionEnter) {
                onCollisionEnter(a, b, info);
            }
        }
    }
}

void CollisionSystem::clear() {
    colliders.clear();
    spatialHash.clear();
    activeCollisions.clear();
    collisionCount = 0;
}

std::vector<Collider*> CollisionSystem::queryRegion(const Math::Vector2D& center, float radius) const {
    if (useSpatialHash) {
        return spatialHash.query(center, radius);
    }
    
    // Brute force query
    std::vector<Collider*> result;
    for (Collider* collider : colliders) {
        Math::Vector2D diff = collider->getPosition() - center;
        if (diff.magnitude() <= radius) {
            result.push_back(collider);
        }
    }
    return result;
}

Collider* CollisionSystem::raycast(const Math::Vector2D& origin, const Math::Vector2D& direction,
                                    float maxDistance, RaycastHit& hit) const {
    Collider* closestHit = nullptr;
    float closestDist = maxDistance;
    
    Math::Vector2D dir = direction;
    dir.normalize();
    
    for (Collider* collider : colliders) {
        // Simplified raycast - proper implementation would vary by collider type
        Math::Vector2D toCollider = collider->getPosition() - origin;
        float projection = toCollider.dot(dir);
        
        if (projection > 0 && projection < closestDist) {
            Math::Vector2D closestPoint = origin + dir * projection;
            float dist = (collider->getPosition() - closestPoint).magnitude();
            
            // Check if ray intersects (simplified - would need proper shape testing)
            float threshold = 2.0f; // Rough approximation
            if (dist < threshold && projection < closestDist) {
                closestHit = collider;
                closestDist = projection;
                hit.collider = collider;
                hit.point = closestPoint;
                hit.distance = projection;
                hit.normal = (closestPoint - collider->getPosition()).normalized();
            }
        }
    }
    
    return closestHit;
}

std::vector<Collider*> CollisionSystem::raycastAll(const Math::Vector2D& origin, const Math::Vector2D& direction,
                                                     float maxDistance) const {
    std::vector<Collider*> hits;
    
    Math::Vector2D dir = direction;
    dir.normalize();
    
    for (Collider* collider : colliders) {
        Math::Vector2D toCollider = collider->getPosition() - origin;
        float projection = toCollider.dot(dir);
        
        if (projection > 0 && projection < maxDistance) {
            Math::Vector2D closestPoint = origin + dir * projection;
            float dist = (collider->getPosition() - closestPoint).magnitude();
            
            float threshold = 2.0f;
            if (dist < threshold) {
                hits.push_back(collider);
            }
        }
    }
    
    return hits;
}

} // namespace Physics
} // namespace JJM
