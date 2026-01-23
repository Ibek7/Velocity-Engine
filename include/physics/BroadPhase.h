#pragma once

#include <vector>
#include <functional>

/**
 * @file BroadPhase.h
 * @brief Spatial partitioning and broad-phase collision detection
 * 
 * Implements various spatial data structures for efficient collision
 * detection broad-phase, including spatial hashing, quad-trees, and
 * sweep-and-prune algorithms.
 */

namespace Engine {

struct AABB {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    
    bool intersects(const AABB& other) const;
    float surfaceArea() const;
    AABB merge(const AABB& other) const;
};

/**
 * @class SpatialHash
 * @brief Spatial hash grid for broad-phase collision detection
 */
class SpatialHash {
public:
    SpatialHash(float cellSize, int gridWidth, int gridHeight);
    ~SpatialHash();
    
    /**
     * @brief Insert an object into the spatial hash
     * @param id Object identifier
     * @param bounds Object bounding box
     */
    void insert(int id, const AABB& bounds);
    
    /**
     * @brief Remove an object from the spatial hash
     * @param id Object identifier
     */
    void remove(int id);
    
    /**
     * @brief Update object position in the spatial hash
     * @param id Object identifier
     * @param newBounds New bounding box
     */
    void update(int id, const AABB& newBounds);
    
    /**
     * @brief Query objects in a region
     * @param bounds Query region
     * @param callback Function called for each potential collision
     */
    void query(const AABB& bounds, std::function<void(int)> callback) const;
    
    /**
     * @brief Query all potential collision pairs
     * @param callback Function called for each pair
     */
    void queryPairs(std::function<void(int, int)> callback) const;
    
    /**
     * @brief Clear all objects from the spatial hash
     */
    void clear();
    
    /**
     * @brief Get cell size
     */
    float getCellSize() const { return m_cellSize; }

private:
    struct ObjectEntry {
        int id;
        AABB bounds;
        std::vector<int> cells;
    };
    
    float m_cellSize;
    int m_gridWidth;
    int m_gridHeight;
    std::vector<std::vector<int>> m_grid;
    std::vector<ObjectEntry> m_objects;
    
    int hashPosition(float x, float y) const;
    void getCellsForAABB(const AABB& bounds, std::vector<int>& cells) const;
    int findObjectIndex(int id) const;
};

/**
 * @class DynamicAABBTree
 * @brief Dynamic AABB tree for hierarchical broad-phase detection
 */
class DynamicAABBTree {
public:
    DynamicAABBTree();
    ~DynamicAABBTree();
    
    /**
     * @brief Create a proxy for an object
     * @param bounds Object bounding box
     * @param userData User data associated with proxy
     * @return Proxy ID
     */
    int createProxy(const AABB& bounds, void* userData);
    
    /**
     * @brief Destroy a proxy
     * @param proxyId Proxy identifier
     */
    void destroyProxy(int proxyId);
    
    /**
     * @brief Move a proxy to a new position
     * @param proxyId Proxy identifier
     * @param bounds New bounding box
     * @param displacement Movement vector
     * @return True if proxy moved significantly
     */
    bool moveProxy(int proxyId, const AABB& bounds, const float* displacement);
    
    /**
     * @brief Get proxy user data
     * @param proxyId Proxy identifier
     * @return User data pointer
     */
    void* getUserData(int proxyId) const;
    
    /**
     * @brief Get proxy AABB
     * @param proxyId Proxy identifier
     * @return Bounding box
     */
    AABB getAABB(int proxyId) const;
    
    /**
     * @brief Query overlapping proxies
     * @param bounds Query region
     * @param callback Function called for each overlapping proxy
     */
    void query(const AABB& bounds, std::function<bool(int)> callback) const;
    
    /**
     * @brief Ray cast against the tree
     * @param origin Ray origin
     * @param direction Ray direction
     * @param maxDistance Maximum ray distance
     * @param callback Function called for each hit
     */
    void rayCast(const float* origin, const float* direction, float maxDistance,
                 std::function<float(int, const float*, const float*, float)> callback) const;
    
    /**
     * @brief Validate tree structure (debug)
     */
    bool validate() const;
    
    /**
     * @brief Get tree height
     */
    int getHeight() const;
    
    /**
     * @brief Get maximum balance of the tree
     */
    int getMaxBalance() const;
    
    /**
     * @brief Get surface area ratio (quality metric)
     */
    float getAreaRatio() const;
    
    /**
     * @brief Rebuild the tree for better quality
     */
    void rebuildBottomUp();

private:
    struct Node {
        AABB aabb;
        void* userData;
        
        union {
            int parent;
            int next;
        };
        
        int child1;
        int child2;
        
        int height;
        bool moved;
        
        bool isLeaf() const { return child1 == -1; }
    };
    
    int m_root;
    std::vector<Node> m_nodes;
    int m_nodeCount;
    int m_nodeCapacity;
    int m_freeList;
    
    int allocateNode();
    void freeNode(int node);
    void insertLeaf(int leaf);
    void removeLeaf(int leaf);
    int balance(int index);
    int computeHeight(int nodeId) const;
    void validateStructure(int index) const;
    void validateMetrics(int index) const;
};

/**
 * @class SweepAndPrune
 * @brief Sweep and prune broad-phase using sorted axis lists
 */
class SweepAndPrune {
public:
    SweepAndPrune();
    ~SweepAndPrune();
    
    /**
     * @brief Add an object to the system
     * @param id Object identifier
     * @param bounds Object bounding box
     * @param isStatic Whether object is static
     */
    void addObject(int id, const AABB& bounds, bool isStatic = false);
    
    /**
     * @brief Remove an object
     * @param id Object identifier
     */
    void removeObject(int id);
    
    /**
     * @brief Update object bounds
     * @param id Object identifier
     * @param newBounds New bounding box
     */
    void updateObject(int id, const AABB& newBounds);
    
    /**
     * @brief Get all overlapping pairs
     * @param outPairs Output vector of pairs
     */
    void computeOverlaps(std::vector<std::pair<int, int>>& outPairs);
    
    /**
     * @brief Query objects overlapping a region
     * @param bounds Query region
     * @param outObjects Output vector of object IDs
     */
    void queryRegion(const AABB& bounds, std::vector<int>& outObjects) const;
    
    /**
     * @brief Clear all objects
     */
    void clear();

private:
    struct Endpoint {
        float value;
        int id;
        bool isMin;
        
        bool operator<(const Endpoint& other) const {
            return value < other.value;
        }
    };
    
    struct ObjectData {
        int id;
        AABB bounds;
        bool isStatic;
        int minIndex[3];
        int maxIndex[3];
    };
    
    std::vector<Endpoint> m_axes[3];
    std::vector<ObjectData> m_objects;
    std::vector<std::pair<int, int>> m_pairs;
    
    void sortAxis(int axis);
    int findObject(int id) const;
};

} // namespace Engine
