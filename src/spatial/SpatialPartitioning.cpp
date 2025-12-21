#include "spatial/SpatialPartitioning.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Spatial {

// AABB implementation
bool AABB::intersects(const AABB& other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y);
}

bool AABB::contains(const Math::Vector2D& point) const {
    return point.x >= min.x && point.x <= max.x &&
           point.y >= min.y && point.y <= max.y;
}

float AABB::area() const {
    return (max.x - min.x) * (max.y - min.y);
}

// QuadtreeNode implementation
QuadtreeNode::QuadtreeNode(const AABB& bounds, int level, int maxLevel, int maxObjects)
    : bounds(bounds), level(level), maxLevel(maxLevel), maxObjects(maxObjects) {
}

QuadtreeNode::~QuadtreeNode() {
}

bool QuadtreeNode::insert(ISpatialObject* object) {
    if (!bounds.intersects(object->getBounds())) {
        return false;
    }
    
    if (objects.size() < static_cast<size_t>(maxObjects) || level >= maxLevel) {
        objects.push_back(object);
        return true;
    }
    
    if (children[0] == nullptr) {
        subdivide();
    }
    
    int index = getIndex(object->getBounds());
    if (index != -1) {
        return children[index]->insert(object);
    }
    
    objects.push_back(object);
    return true;
}

void QuadtreeNode::remove(ISpatialObject* object) {
    objects.erase(std::remove(objects.begin(), objects.end(), object), objects.end());
    
    for (int i = 0; i < 4; i++) {
        if (children[i]) {
            children[i]->remove(object);
        }
    }
}

void QuadtreeNode::query(const AABB& range, std::vector<ISpatialObject*>& results) {
    if (!bounds.intersects(range)) {
        return;
    }
    
    for (auto* obj : objects) {
        if (obj->getBounds().intersects(range)) {
            results.push_back(obj);
        }
    }
    
    if (children[0]) {
        for (int i = 0; i < 4; i++) {
            children[i]->query(range, results);
        }
    }
}

void QuadtreeNode::clear() {
    objects.clear();
    for (int i = 0; i < 4; i++) {
        children[i].reset();
    }
}

void QuadtreeNode::subdivide() {
    float midX = (bounds.min.x + bounds.max.x) / 2.0f;
    float midY = (bounds.min.y + bounds.max.y) / 2.0f;
    
    children[0] = std::make_unique<QuadtreeNode>(
        AABB{bounds.min, Math::Vector2D(midX, midY)},
        level + 1, maxLevel, maxObjects);
    children[1] = std::make_unique<QuadtreeNode>(
        AABB{Math::Vector2D(midX, bounds.min.y), Math::Vector2D(bounds.max.x, midY)},
        level + 1, maxLevel, maxObjects);
    children[2] = std::make_unique<QuadtreeNode>(
        AABB{Math::Vector2D(bounds.min.x, midY), Math::Vector2D(midX, bounds.max.y)},
        level + 1, maxLevel, maxObjects);
    children[3] = std::make_unique<QuadtreeNode>(
        AABB{Math::Vector2D(midX, midY), bounds.max},
        level + 1, maxLevel, maxObjects);
}

int QuadtreeNode::getIndex(const AABB& objBounds) {
    float midX = (bounds.min.x + bounds.max.x) / 2.0f;
    float midY = (bounds.min.y + bounds.max.y) / 2.0f;
    
    bool top = objBounds.min.y > midY;
    bool bottom = objBounds.max.y < midY;
    bool left = objBounds.max.x < midX;
    bool right = objBounds.min.x > midX;
    
    if (left) {
        if (top) return 2;
        if (bottom) return 0;
    } else if (right) {
        if (top) return 3;
        if (bottom) return 1;
    }
    
    return -1;
}

// Quadtree implementation
Quadtree::Quadtree(const AABB& bounds, int maxLevel, int maxObjects)
    : bounds(bounds), maxLevel(maxLevel), maxObjects(maxObjects) {
    root = std::make_unique<QuadtreeNode>(bounds, 0, maxLevel, maxObjects);
}

Quadtree::~Quadtree() {
}

void Quadtree::insert(ISpatialObject* object) {
    root->insert(object);
}

void Quadtree::remove(ISpatialObject* object) {
    root->remove(object);
}

void Quadtree::update(ISpatialObject* object) {
    remove(object);
    insert(object);
}

std::vector<ISpatialObject*> Quadtree::query(const AABB& range) {
    std::vector<ISpatialObject*> results;
    root->query(range, results);
    return results;
}

std::vector<ISpatialObject*> Quadtree::queryRadius(const Math::Vector2D& center, float radius) {
    AABB range{
        Math::Vector2D(center.x - radius, center.y - radius),
        Math::Vector2D(center.x + radius, center.y + radius)
    };
    return query(range);
}

void Quadtree::clear() {
    root->clear();
}

// SpatialHashGrid implementation
SpatialHashGrid::SpatialHashGrid(float cellSize) : cellSize(cellSize) {
}

SpatialHashGrid::~SpatialHashGrid() {
}

void SpatialHashGrid::insert(ISpatialObject* object) {
    std::vector<uint64_t> cells;
    getCells(object->getBounds(), cells);
    
    for (uint64_t cell : cells) {
        grid[cell].push_back(object);
    }
}

void SpatialHashGrid::remove(ISpatialObject* object) {
    std::vector<uint64_t> cells;
    getCells(object->getBounds(), cells);
    
    for (uint64_t cell : cells) {
        auto& objects = grid[cell];
        objects.erase(std::remove(objects.begin(), objects.end(), object), objects.end());
    }
}

void SpatialHashGrid::update(ISpatialObject* object) {
    remove(object);
    insert(object);
}

std::vector<ISpatialObject*> SpatialHashGrid::query(const AABB& range) {
    std::vector<uint64_t> cells;
    getCells(range, cells);
    
    std::unordered_set<ISpatialObject*> results;
    for (uint64_t cell : cells) {
        auto it = grid.find(cell);
        if (it != grid.end()) {
            for (auto* obj : it->second) {
                if (obj->getBounds().intersects(range)) {
                    results.insert(obj);
                }
            }
        }
    }
    
    return std::vector<ISpatialObject*>(results.begin(), results.end());
}

std::vector<ISpatialObject*> SpatialHashGrid::queryRadius(const Math::Vector2D& center, float radius) {
    AABB range{
        Math::Vector2D(center.x - radius, center.y - radius),
        Math::Vector2D(center.x + radius, center.y + radius)
    };
    return query(range);
}

void SpatialHashGrid::clear() {
    grid.clear();
}

uint64_t SpatialHashGrid::hashCell(int x, int y) const {
    return (static_cast<uint64_t>(x) << 32) | static_cast<uint64_t>(y);
}

void SpatialHashGrid::getCells(const AABB& bounds, std::vector<uint64_t>& cells) {
    int minX = static_cast<int>(std::floor(bounds.min.x / cellSize));
    int minY = static_cast<int>(std::floor(bounds.min.y / cellSize));
    int maxX = static_cast<int>(std::floor(bounds.max.x / cellSize));
    int maxY = static_cast<int>(std::floor(bounds.max.y / cellSize));
    
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            cells.push_back(hashCell(x, y));
        }
    }
}

// BVH implementation
BVH::BVH() {
}

BVH::~BVH() {
}

void BVH::build(std::vector<ISpatialObject*>& objects) {
    if (objects.empty()) return;
    root = buildRecursive(objects, 0, objects.size());
}

void BVH::query(const AABB& range, std::vector<ISpatialObject*>& results) {
    if (root) {
        queryRecursive(root.get(), range, results);
    }
}

void BVH::clear() {
    root.reset();
}

std::unique_ptr<BVHNode> BVH::buildRecursive(std::vector<ISpatialObject*>& objects, int start, int end) {
    auto node = std::make_unique<BVHNode>();
    
    if (end - start == 1) {
        node->object = objects[start];
        node->bounds = objects[start]->getBounds();
        return node;
    }
    
    // Compute bounds
    node->bounds = objects[start]->getBounds();
    for (int i = start + 1; i < end; i++) {
        AABB objBounds = objects[i]->getBounds();
        node->bounds.min.x = std::min(node->bounds.min.x, objBounds.min.x);
        node->bounds.min.y = std::min(node->bounds.min.y, objBounds.min.y);
        node->bounds.max.x = std::max(node->bounds.max.x, objBounds.max.x);
        node->bounds.max.y = std::max(node->bounds.max.y, objBounds.max.y);
    }
    
    int mid = (start + end) / 2;
    node->left = buildRecursive(objects, start, mid);
    node->right = buildRecursive(objects, mid, end);
    
    return node;
}

void BVH::queryRecursive(BVHNode* node, const AABB& range, std::vector<ISpatialObject*>& results) {
    if (!node || !node->bounds.intersects(range)) {
        return;
    }
    
    if (node->object) {
        if (node->object->getBounds().intersects(range)) {
            results.push_back(node->object);
        }
        return;
    }
    
    if (node->left) queryRecursive(node->left.get(), range, results);
    if (node->right) queryRecursive(node->right.get(), range, results);
}

} // namespace Spatial
} // namespace JJM
