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
 * @brief Spatial partitioning performance hints
 */
enum class PartitioningHint {
    Static,          // Object rarely moves (buildings, terrain)
    Dynamic,         // Object moves frequently (players, enemies)
    Streaming,       // Object may be loaded/unloaded (distant content)
    HighPriority,    // Query this object frequently (important entities)
    LowPriority      // Query this object rarely (decorative elements)
};

/**
 * @brief Extended spatial object with performance hints
 */
class ISpatialObjectEx : public ISpatialObject {
public:
    virtual ~ISpatialObjectEx() {}
    virtual PartitioningHint getPartitioningHint() const { return PartitioningHint::Dynamic; }
    virtual bool isStatic() const { return getPartitioningHint() == PartitioningHint::Static; }
    virtual int getPriority() const { 
        switch (getPartitioningHint()) {
            case PartitioningHint::HighPriority: return 100;
            case PartitioningHint::Static: return 50;
            case PartitioningHint::Dynamic: return 25;
            case PartitioningHint::Streaming: return 10;
            case PartitioningHint::LowPriority: return 1;
            default: return 25;
        }
    }
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
    
    // Query with priority filtering
    std::vector<ISpatialObject*> queryPrioritized(const AABB& range, int minPriority = 0);
    
    // Separate static and dynamic objects for optimization
    void setStaticMode(bool enabled) { separateStatic = enabled; }
    void rebuildStatic();
    
    void clear();
    
    // Statistics
    size_t getObjectCount() const;
    size_t getStaticObjectCount() const { return staticObjects.size(); }
    size_t getDynamicObjectCount() const;

private:
    std::unique_ptr<QuadtreeNode> root;
    AABB bounds;
    int maxLevel;
    int maxObjects;
    
    // Static/dynamic separation
    bool separateStatic;
    std::vector<ISpatialObject*> staticObjects;
    std::unique_ptr<QuadtreeNode> staticRoot;
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
