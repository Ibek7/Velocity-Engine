#pragma once

#include "math/Vector2D.h"
#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace Spatial {

struct AABB {
    Math::Vector2D min;
    Math::Vector2D max;
    
    AABB() : min(0, 0), max(0, 0) {}
    AABB(const Math::Vector2D& min, const Math::Vector2D& max) : min(min), max(max) {}
    
    bool contains(const Math::Vector2D& point) const;
    bool intersects(const AABB& other) const;
    Math::Vector2D getCenter() const;
    Math::Vector2D getSize() const;
};

template<typename T>
class QuadTree {
public:
    QuadTree(const AABB& bounds, int maxDepth = 8, int maxObjects = 4);
    ~QuadTree();
    
    void insert(const Math::Vector2D& position, T* object);
    void remove(T* object);
    
    std::vector<T*> query(const AABB& region) const;
    std::vector<T*> queryRadius(const Math::Vector2D& center, float radius) const;
    
    void clear();
    
    int getObjectCount() const { return objects.size(); }
    int getDepth() const { return depth; }

private:
    struct ObjectEntry {
        Math::Vector2D position;
        T* object;
        
        ObjectEntry(const Math::Vector2D& pos, T* obj) : position(pos), object(obj) {}
    };
    
    AABB bounds;
    int maxDepth;
    int maxObjects;
    int depth;
    
    std::vector<ObjectEntry> objects;
    std::unique_ptr<QuadTree<T>> children[4];
    
    bool isDivided;
    
    void subdivide();
    int getQuadrant(const Math::Vector2D& position) const;
    void queryNode(const AABB& region, std::vector<T*>& results) const;
};

template<typename T>
QuadTree<T>::QuadTree(const AABB& bounds, int maxDepth, int maxObjects)
    : bounds(bounds), maxDepth(maxDepth), maxObjects(maxObjects), depth(0), isDivided(false) {}

template<typename T>
QuadTree<T>::~QuadTree() {
    clear();
}

template<typename T>
void QuadTree<T>::insert(const Math::Vector2D& position, T* object) {
    if (!bounds.contains(position)) {
        return;
    }
    
    if (!isDivided && objects.size() < static_cast<size_t>(maxObjects)) {
        objects.emplace_back(position, object);
        return;
    }
    
    if (!isDivided && depth < maxDepth) {
        subdivide();
    }
    
    if (isDivided) {
        int quadrant = getQuadrant(position);
        if (quadrant >= 0 && quadrant < 4 && children[quadrant]) {
            children[quadrant]->insert(position, object);
        }
    } else {
        objects.emplace_back(position, object);
    }
}

template<typename T>
void QuadTree<T>::remove(T* object) {
    objects.erase(
        std::remove_if(objects.begin(), objects.end(),
            [object](const ObjectEntry& entry) {
                return entry.object == object;
            }),
        objects.end()
    );
    
    if (isDivided) {
        for (int i = 0; i < 4; ++i) {
            if (children[i]) {
                children[i]->remove(object);
            }
        }
    }
}

template<typename T>
std::vector<T*> QuadTree<T>::query(const AABB& region) const {
    std::vector<T*> results;
    queryNode(region, results);
    return results;
}

template<typename T>
std::vector<T*> QuadTree<T>::queryRadius(const Math::Vector2D& center, float radius) const {
    AABB region(
        Math::Vector2D(center.x - radius, center.y - radius),
        Math::Vector2D(center.x + radius, center.y + radius)
    );
    
    std::vector<T*> candidates = query(region);
    std::vector<T*> results;
    
    float radiusSq = radius * radius;
    for (T* obj : candidates) {
        (void)obj;
        results.push_back(obj);
    }
    
    return results;
}

template<typename T>
void QuadTree<T>::clear() {
    objects.clear();
    
    if (isDivided) {
        for (int i = 0; i < 4; ++i) {
            children[i].reset();
        }
        isDivided = false;
    }
}

template<typename T>
void QuadTree<T>::subdivide() {
    Math::Vector2D center = bounds.getCenter();
    Math::Vector2D halfSize = bounds.getSize() * 0.5f;
    
    AABB quadrants[4] = {
        AABB(bounds.min, center),
        AABB(Math::Vector2D(center.x, bounds.min.y), Math::Vector2D(bounds.max.x, center.y)),
        AABB(Math::Vector2D(bounds.min.x, center.y), Math::Vector2D(center.x, bounds.max.y)),
        AABB(center, bounds.max)
    };
    
    for (int i = 0; i < 4; ++i) {
        children[i] = std::make_unique<QuadTree<T>>(quadrants[i], maxDepth, maxObjects);
        children[i]->depth = depth + 1;
    }
    
    std::vector<ObjectEntry> oldObjects = std::move(objects);
    objects.clear();
    
    for (const auto& entry : oldObjects) {
        int quadrant = getQuadrant(entry.position);
        if (quadrant >= 0 && quadrant < 4) {
            children[quadrant]->insert(entry.position, entry.object);
        }
    }
    
    isDivided = true;
}

template<typename T>
int QuadTree<T>::getQuadrant(const Math::Vector2D& position) const {
    Math::Vector2D center = bounds.getCenter();
    
    bool left = position.x < center.x;
    bool top = position.y < center.y;
    
    if (left && top) return 0;
    if (!left && top) return 1;
    if (left && !top) return 2;
    return 3;
}

template<typename T>
void QuadTree<T>::queryNode(const AABB& region, std::vector<T*>& results) const {
    if (!bounds.intersects(region)) {
        return;
    }
    
    for (const auto& entry : objects) {
        if (region.contains(entry.position)) {
            results.push_back(entry.object);
        }
    }
    
    if (isDivided) {
        for (int i = 0; i < 4; ++i) {
            if (children[i]) {
                children[i]->queryNode(region, results);
            }
        }
    }
}

} // namespace Spatial
} // namespace JJM
