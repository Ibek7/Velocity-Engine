#ifndef JJM_SPATIAL_PARTITIONING_H
#define JJM_SPATIAL_PARTITIONING_H

#include "math/Vector2D.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>

namespace JJM {
namespace Spatial {

/**
 * @brief AABB for spatial queries
 */
struct AABB {
    Math::Vector2D min;
    Math::Vector2D max;
    
    bool intersects(const AABB& other) const;
    bool contains(const Math::Vector2D& point) const;
    float area() const;
};

/**
 * @brief Spatial object interface
 */
class ISpatialObject {
public:
    virtual ~ISpatialObject() {}
    virtual AABB getBounds() const = 0;
    virtual uint32_t getId() const = 0;
};

/**
 * @brief Quadtree node
 */
class QuadtreeNode {
public:
    QuadtreeNode(const AABB& bounds, int level, int maxLevel, int maxObjects);
    ~QuadtreeNode();

    bool insert(ISpatialObject* object);
    void remove(ISpatialObject* object);
    void query(const AABB& range, std::vector<ISpatialObject*>& results);
    void clear();
    
    void subdivide();
    int getIndex(const AABB& bounds);

private:
    AABB bounds;
    int level;
    int maxLevel;
    int maxObjects;
    std::vector<ISpatialObject*> objects;
    std::unique_ptr<QuadtreeNode> children[4];
};

/**
 * @brief Quadtree for spatial partitioning
 */
class Quadtree {
public:
    Quadtree(const AABB& bounds, int maxLevel = 5, int maxObjects = 10);
    ~Quadtree();

    void insert(ISpatialObject* object);
    void remove(ISpatialObject* object);
    void update(ISpatialObject* object);
    
    std::vector<ISpatialObject*> query(const AABB& range);
    std::vector<ISpatialObject*> queryRadius(const Math::Vector2D& center, float radius);
    
    void clear();

private:
    std::unique_ptr<QuadtreeNode> root;
    AABB bounds;
    int maxLevel;
    int maxObjects;
};

/**
 * @brief Spatial hash grid
 */
class SpatialHashGrid {
public:
    SpatialHashGrid(float cellSize);
    ~SpatialHashGrid();

    void insert(ISpatialObject* object);
    void remove(ISpatialObject* object);
    void update(ISpatialObject* object);
    
    std::vector<ISpatialObject*> query(const AABB& range);
    std::vector<ISpatialObject*> queryRadius(const Math::Vector2D& center, float radius);
    
    void clear();

private:
    float cellSize;
    std::unordered_map<uint64_t, std::vector<ISpatialObject*>> grid;
    
    uint64_t hashCell(int x, int y) const;
    void getCells(const AABB& bounds, std::vector<uint64_t>& cells);
};

/**
 * @brief BVH node
 */
struct BVHNode {
    AABB bounds;
    ISpatialObject* object;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
};

/**
 * @brief Bounding Volume Hierarchy
 */
class BVH {
public:
    BVH();
    ~BVH();

    void build(std::vector<ISpatialObject*>& objects);
    void query(const AABB& range, std::vector<ISpatialObject*>& results);
    void clear();

private:
    std::unique_ptr<BVHNode> root;
    
    std::unique_ptr<BVHNode> buildRecursive(std::vector<ISpatialObject*>& objects, int start, int end);
    void queryRecursive(BVHNode* node, const AABB& range, std::vector<ISpatialObject*>& results);
};

} // namespace Spatial
} // namespace JJM

#endif // JJM_SPATIAL_PARTITIONING_H
