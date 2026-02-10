#ifndef NAVIGATION_MESH_H
#define NAVIGATION_MESH_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../math/Vector2D.h"

namespace JJM {
namespace AI {

// Forward declarations
class NavMeshNode;
class NavMeshEdge;
class NavMesh;
class NavMeshBuilder;

// Navigation mesh node (polygon)
class NavMeshNode {
   public:
    NavMeshNode(int id);

    void addVertex(const JJM::Math::Vector2D& vertex);
    void addEdge(int edgeId, int neighborId);

    int getId() const { return id; }
    const std::vector<JJM::Math::Vector2D>& getVertices() const { return vertices; }
    const std::vector<int>& getEdgeIds() const { return edgeIds; }
    const std::vector<int>& getNeighborIds() const { return neighborIds; }

    JJM::Math::Vector2D getCenter() const;
    bool containsPoint(const JJM::Math::Vector2D& point) const;
    float getCost() const { return cost; }
    void setCost(float c) { cost = c; }

   private:
    int id;
    std::vector<JJM::Math::Vector2D> vertices;
    std::vector<int> edgeIds;
    std::vector<int> neighborIds;
    float cost;
};

// Navigation mesh edge (portal between nodes)
class NavMeshEdge {
   public:
    NavMeshEdge(int id, const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end);

    int getId() const { return id; }
    JJM::Math::Vector2D getStart() const { return start; }
    JJM::Math::Vector2D getEnd() const { return end; }
    JJM::Math::Vector2D getCenter() const { return (start + end) * 0.5f; }
    float getWidth() const { return (end - start).magnitude(); }

   private:
    int id;
    JJM::Math::Vector2D start;
    JJM::Math::Vector2D end;
};

// Navigation mesh path
struct NavMeshPath {
    std::vector<JJM::Math::Vector2D> waypoints;
    float totalCost;
    bool valid;

    NavMeshPath() : totalCost(0.0f), valid(false) {}

    void clear() {
        waypoints.clear();
        totalCost = 0.0f;
        valid = false;
    }

    bool isEmpty() const { return waypoints.empty(); }
    size_t size() const { return waypoints.size(); }
};

// Navigation mesh for pathfinding
class NavMesh {
   public:
    NavMesh();
    ~NavMesh();

    // Delete copy, allow move
    NavMesh(const NavMesh&) = delete;
    NavMesh& operator=(const NavMesh&) = delete;
    NavMesh(NavMesh&&) = default;
    NavMesh& operator=(NavMesh&&) = default;

    // Node management
    int addNode(const std::vector<JJM::Math::Vector2D>& vertices);
    NavMeshNode* getNode(int id);
    const NavMeshNode* getNode(int id) const;

    // Edge management
    int addEdge(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end);
    NavMeshEdge* getEdge(int id);
    const NavMeshEdge* getEdge(int id) const;

    // Connect two nodes via an edge
    void connectNodes(int nodeId1, int nodeId2, int edgeId);

    // Find node containing a point
    int findNodeContainingPoint(const JJM::Math::Vector2D& point) const;

    // Pathfinding
    NavMeshPath findPath(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end) const;
    NavMeshPath findPath(int startNodeId, int endNodeId) const;

    // String pulling for path smoothing
    void smoothPath(NavMeshPath& path) const;

    // Dynamic updates
    void setNodeCost(int nodeId, float cost);
    void disableNode(int nodeId);
    void enableNode(int nodeId);

    // Query
    size_t getNodeCount() const { return nodes.size(); }
    size_t getEdgeCount() const { return edges.size(); }

    // Clear all data
    void clear();

   private:
    struct PathNode {
        int nodeId;
        float gCost;  // Cost from start
        float hCost;  // Heuristic to end
        float fCost() const { return gCost + hCost; }
        int parent;

        PathNode(int id = -1, float g = 0, float h = 0, int p = -1)
            : nodeId(id), gCost(g), hCost(h), parent(p) {}

        bool operator>(const PathNode& other) const { return fCost() > other.fCost(); }
    };

    std::vector<std::unique_ptr<NavMeshNode>> nodes;
    std::vector<std::unique_ptr<NavMeshEdge>> edges;
    std::unordered_map<int, bool> disabledNodes;

    float heuristic(int nodeId1, int nodeId2) const;
    void reconstructPath(const std::unordered_map<int, int>& cameFrom, int current,
                         NavMeshPath& path) const;
};

// Navigation mesh builder for automatic generation
class NavMeshBuilder {
   public:
    NavMeshBuilder();

    // Build from grid
    NavMesh buildFromGrid(int gridWidth, int gridHeight, float cellSize,
                          const std::vector<bool>& walkable);

    // Build from obstacles
    NavMesh buildFromObstacles(const JJM::Math::Vector2D& worldMin,
                               const JJM::Math::Vector2D& worldMax, float cellSize,
                               const std::vector<JJM::Math::Vector2D>& obstaclePoints,
                               float obstacleRadius);

    // Configuration
    void setMaxSlope(float angle) { maxSlope = angle; }
    void setAgentRadius(float radius) { agentRadius = radius; }
    void setStepHeight(float height) { stepHeight = height; }

    // Region growing for polygon formation
    void setMergeRegions(bool merge) { mergeRegions = merge; }

   private:
    struct Cell {
        int x, y;
        bool walkable;
        int regionId;

        Cell(int _x = 0, int _y = 0, bool w = true) : x(_x), y(_y), walkable(w), regionId(-1) {}
    };

    float maxSlope;
    float agentRadius;
    float stepHeight;
    bool mergeRegions;

    void markWalkableGrid(std::vector<Cell>& grid, int width, int height,
                          const std::vector<bool>& walkable);
    void buildRegions(std::vector<Cell>& grid, int width, int height);
    void createPolygons(NavMesh& mesh, const std::vector<Cell>& grid, int width, int height,
                        float cellSize);
    void connectNeighborNodes(NavMesh& mesh, const std::vector<Cell>& grid, int width, int height);
};

// Agent using navigation mesh
class NavMeshAgent {
   public:
    NavMeshAgent(const NavMesh* navMesh);

    void setPosition(const JJM::Math::Vector2D& pos);
    void setDestination(const JJM::Math::Vector2D& dest);

    void update(float deltaTime);

    JJM::Math::Vector2D getPosition() const { return position; }
    JJM::Math::Vector2D getVelocity() const { return velocity; }
    bool hasPath() const { return currentPath.valid; }
    bool isAtDestination() const;

    // Configuration
    void setSpeed(float speed) { maxSpeed = speed; }
    void setAcceleration(float accel) { acceleration = accel; }
    void setStoppingDistance(float dist) { stoppingDistance = dist; }
    void setAutoRepath(bool enable) { autoRepath = enable; }
    void setRepathInterval(float interval) { repathInterval = interval; }

    // Query
    const NavMeshPath& getCurrentPath() const { return currentPath; }
    int getCurrentWaypoint() const { return currentWaypointIndex; }

    // Control
    void stop();
    void pause();
    void resume();
    bool isPaused() const { return paused; }

   private:
    const NavMesh* navMesh;

    JJM::Math::Vector2D position;
    JJM::Math::Vector2D velocity;
    JJM::Math::Vector2D destination;

    NavMeshPath currentPath;
    int currentWaypointIndex;

    float maxSpeed;
    float acceleration;
    float stoppingDistance;

    bool autoRepath;
    float repathInterval;
    float timeSinceLastPath;

    bool paused;

    void calculatePath();
    void followPath(float deltaTime);
    JJM::Math::Vector2D steer(const JJM::Math::Vector2D& target, float deltaTime);
};

// Local avoidance for multiple agents
class LocalAvoidance {
   public:
    struct Agent {
        JJM::Math::Vector2D position;
        JJM::Math::Vector2D velocity;
        float radius;
        int priority;

        Agent(const JJM::Math::Vector2D& pos = JJM::Math::Vector2D(0, 0),
              const JJM::Math::Vector2D& vel = JJM::Math::Vector2D(0, 0), float r = 1.0f, int p = 0)
            : position(pos), velocity(vel), radius(r), priority(p) {}
    };

    LocalAvoidance();

    // Add/remove agents
    int addAgent(const Agent& agent);
    void removeAgent(int id);
    void updateAgent(int id, const JJM::Math::Vector2D& position,
                     const JJM::Math::Vector2D& velocity);

    // Calculate avoidance velocity
    JJM::Math::Vector2D computeAvoidanceVelocity(int agentId,
                                                 const JJM::Math::Vector2D& preferredVelocity,
                                                 float maxSpeed);

    // Configuration
    void setTimeHorizon(float horizon) { timeHorizon = horizon; }
    void setMaxNeighbors(int max) { maxNeighbors = max; }
    void setNeighborDistance(float dist) { neighborDistance = dist; }

    void clear();

   private:
    std::unordered_map<int, Agent> agents;
    int nextAgentId;

    float timeHorizon;
    int maxNeighbors;
    float neighborDistance;

    struct Line {
        JJM::Math::Vector2D point;
        JJM::Math::Vector2D direction;
    };

    std::vector<Line> computeORCALines(int agentId, const JJM::Math::Vector2D& preferredVelocity);
    JJM::Math::Vector2D linearProgram(const std::vector<Line>& lines,
                                      const JJM::Math::Vector2D& preferredVelocity, float maxSpeed);
};

// =============================================================================
// HIERARCHICAL PATHFINDING (HPA*)
// =============================================================================

/**
 * @brief Cluster for hierarchical navigation
 */
struct NavMeshCluster {
    int clusterId;
    std::vector<int> nodeIds;  // Nodes in this cluster
    JJM::Math::Vector2D center;
    float radius;

    // Inter-cluster connections
    struct BorderNode {
        int nodeId;
        int edgeId;  // Edge connecting to other cluster
        int neighborClusterId;
        float distanceToCenter;
    };
    std::vector<BorderNode> borderNodes;
};

/**
 * @brief Pre-computed path between clusters
 */
struct ClusterPath {
    int fromClusterId;
    int toClusterId;
    std::vector<int> borderNodeSequence;
    float totalCost;
    bool valid{true};
};

/**
 * @brief Hierarchical navigation mesh for large worlds
 */
class HierarchicalNavMesh {
   public:
    HierarchicalNavMesh(NavMesh& baseMesh);
    ~HierarchicalNavMesh();

    // Build hierarchy
    void buildHierarchy(int clusterSize = 10);
    void rebuildHierarchy();
    void clear();

    // Hierarchical pathfinding
    NavMeshPath findPathHierarchical(const JJM::Math::Vector2D& start,
                                     const JJM::Math::Vector2D& end);

    // Path refinement
    NavMeshPath refineClusterPath(const std::vector<int>& clusterPath);
    void smoothHierarchicalPath(NavMeshPath& path);

    // Dynamic updates
    void invalidateCluster(int clusterId);
    void invalidateNode(int nodeId);
    void rebuildCluster(int clusterId);

    // Query
    int findClusterContainingNode(int nodeId) const;
    int findClusterContainingPoint(const JJM::Math::Vector2D& point) const;
    size_t getClusterCount() const { return clusters.size(); }
    const NavMeshCluster* getCluster(int clusterId) const;

    // Pre-computation
    void precomputeClusterPaths();
    const ClusterPath* getPrecomputedPath(int fromCluster, int toCluster) const;

    // Statistics
    struct HierarchyStats {
        size_t clusterCount;
        size_t totalBorderNodes;
        size_t precomputedPaths;
        float averageClusterSize;
        float hierarchyBuildTime;
    };
    HierarchyStats getStatistics() const;

   private:
    NavMesh& baseMesh;
    std::vector<NavMeshCluster> clusters;
    std::unordered_map<int, int> nodeToCluster;  // Node ID -> Cluster ID
    std::map<std::pair<int, int>, ClusterPath> precomputedPaths;

    void clusterNodes(int clusterSize);
    void findBorderNodes();
    void buildClusterGraph();
    std::vector<int> findClusterPath(int startCluster, int endCluster);
};

// =============================================================================
// PATH CACHING AND POOLING
// =============================================================================

/**
 * @brief Cache key for path lookup
 */
struct PathCacheKey {
    int startNodeId;
    int endNodeId;

    bool operator==(const PathCacheKey& other) const {
        return startNodeId == other.startNodeId && endNodeId == other.endNodeId;
    }
};

// Hash function for PathCacheKey
struct PathCacheKeyHash {
    size_t operator()(const PathCacheKey& key) const {
        return std::hash<int>()(key.startNodeId) ^ (std::hash<int>()(key.endNodeId) << 16);
    }
};

/**
 * @brief Cached path entry
 */
struct CachedPath {
    NavMeshPath path;
    uint64_t timestamp;
    uint32_t useCount;
    bool valid{true};
};

/**
 * @brief LRU path cache for frequently used routes
 */
class PathCache {
   public:
    PathCache(size_t maxSize = 1000);

    // Cache operations
    void addPath(const PathCacheKey& key, const NavMeshPath& path);
    const NavMeshPath* getPath(const PathCacheKey& key);
    void invalidatePath(const PathCacheKey& key);
    void invalidatePathsContaining(int nodeId);
    void clear();

    // Configuration
    void setMaxSize(size_t size);
    void setExpirationTime(float seconds) { expirationTime = seconds; }

    // Statistics
    struct CacheStats {
        size_t cacheSize;
        size_t hits;
        size_t misses;
        float hitRate;
        size_t evictions;
    };
    CacheStats getStatistics() const;
    void resetStatistics();

   private:
    std::unordered_map<PathCacheKey, CachedPath, PathCacheKeyHash> cache;
    std::list<PathCacheKey> lruList;  // Most recent at front
    std::unordered_map<PathCacheKey, std::list<PathCacheKey>::iterator, PathCacheKeyHash> lruMap;

    size_t maxSize;
    float expirationTime{60.0f};  // Seconds before path expires

    mutable CacheStats stats{};

    void evictOldest();
    void moveToFront(const PathCacheKey& key);
    bool isExpired(const CachedPath& cached) const;
};

// =============================================================================
// THREADED PATHFINDING
// =============================================================================

/**
 * @brief Path request for async processing
 */
struct PathRequest {
    int requestId;
    JJM::Math::Vector2D start;
    JJM::Math::Vector2D end;
    int priority;
    std::function<void(const NavMeshPath&)> callback;
    bool cancelled{false};

    // Request options
    bool useHierarchical{true};
    bool smoothPath{true};
    float maxSearchTime{0.1f};  // Max time to spend on this request
};

/**
 * @brief Threaded pathfinding system
 */
class ThreadedPathfinder {
   public:
    ThreadedPathfinder(NavMesh& mesh, int threadCount = 2);
    ~ThreadedPathfinder();

    // Request management
    int requestPath(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end,
                    std::function<void(const NavMeshPath&)> callback, int priority = 0);
    void cancelRequest(int requestId);
    void cancelAllRequests();

    // Hierarchical support
    void setHierarchicalMesh(HierarchicalNavMesh* hierarchical) { hierMesh = hierarchical; }

    // Path caching
    void setPathCache(PathCache* cache) { pathCache = cache; }

    // Control
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const { return running; }

    // Process completed paths on main thread
    void processCompletedPaths();

    // Statistics
    struct ThreadStats {
        size_t pendingRequests;
        size_t completedRequests;
        size_t cancelledRequests;
        float averagePathTime;
        size_t cacheHits;
        size_t cacheMisses;
    };
    ThreadStats getStatistics() const;

    // Configuration
    void setMaxRequestsPerFrame(int max) { maxRequestsPerFrame = max; }

   private:
    NavMesh& navMesh;
    HierarchicalNavMesh* hierMesh{nullptr};
    PathCache* pathCache{nullptr};

    std::vector<std::thread> workers;
    std::priority_queue<PathRequest, std::vector<PathRequest>,
                        std::function<bool(const PathRequest&, const PathRequest&)>>
        requestQueue;
    std::vector<std::pair<int, NavMeshPath>> completedPaths;
    std::unordered_map<int, std::function<void(const NavMeshPath&)>> callbacks;

    std::mutex queueMutex;
    std::mutex completedMutex;
    std::condition_variable queueCondition;

    std::atomic<bool> running{false};
    std::atomic<bool> paused{false};
    int nextRequestId{0};
    int maxRequestsPerFrame{10};

    mutable ThreadStats stats{};

    void workerThread();
    void processRequest(PathRequest& request);
};

// =============================================================================
// DYNAMIC OBSTACLES
// =============================================================================

/**
 * @brief Dynamic obstacle shape
 */
enum class ObstacleShape { Circle, Rectangle, Polygon, Capsule };

/**
 * @brief Dynamic obstacle definition
 */
struct DynamicObstacle {
    int obstacleId;
    ObstacleShape shape;
    JJM::Math::Vector2D position;
    float rotation{0.0f};

    // Shape parameters
    union {
        struct {
            float radius;
        } circle;
        struct {
            float width;
            float height;
        } rectangle;
        struct {
            float radius;
            float length;
        } capsule;
    };
    std::vector<JJM::Math::Vector2D> polygonVertices;  // For polygon shape

    // Properties
    float costMultiplier{-1.0f};  // -1 = impassable, >1 = higher cost
    bool enabled{true};
    int priority{0};  // Higher priority obstacles override lower

    // Velocity for prediction
    JJM::Math::Vector2D velocity;
    float angularVelocity{0.0f};
};

/**
 * @brief Dynamic obstacle manager with nav mesh integration
 */
class DynamicObstacleManager {
   public:
    DynamicObstacleManager(NavMesh& mesh);
    ~DynamicObstacleManager();

    // Obstacle management
    int addObstacle(const DynamicObstacle& obstacle);
    void removeObstacle(int obstacleId);
    void updateObstacle(int obstacleId, const JJM::Math::Vector2D& position, float rotation = 0.0f);
    void updateObstacleVelocity(int obstacleId, const JJM::Math::Vector2D& velocity);
    DynamicObstacle* getObstacle(int obstacleId);

    // Batch updates
    void updateAllObstacles(float deltaTime);

    // Query
    bool isPointBlocked(const JJM::Math::Vector2D& point) const;
    bool isLineBlocked(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end) const;
    std::vector<int> getObstaclesInArea(const JJM::Math::Vector2D& center, float radius) const;
    std::vector<int> getObstaclesOnPath(const NavMeshPath& path) const;

    // Nav mesh integration
    void updateAffectedNodes();
    void setNodeCostCallback(std::function<void(int nodeId, float cost)> callback);

    // Prediction
    JJM::Math::Vector2D predictObstaclePosition(int obstacleId, float time) const;
    bool willPathBeBlocked(const NavMeshPath& path, float travelTime) const;

    // Spatial partitioning for efficient queries
    void rebuildSpatialIndex();
    void setCellSize(float size) { spatialCellSize = size; }

    // Statistics
    struct ObstacleStats {
        size_t obstacleCount;
        size_t affectedNodes;
        float updateTime;
    };
    ObstacleStats getStatistics() const;

   private:
    NavMesh& navMesh;
    std::unordered_map<int, DynamicObstacle> obstacles;
    int nextObstacleId{0};

    // Spatial hash for fast queries
    float spatialCellSize{10.0f};
    std::unordered_map<int64_t, std::vector<int>> spatialHash;

    std::function<void(int, float)> nodeCostCallback;

    bool pointInObstacle(const JJM::Math::Vector2D& point, const DynamicObstacle& obstacle) const;
    bool lineIntersectsObstacle(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end,
                                const DynamicObstacle& obstacle) const;
    int64_t getSpatialKey(const JJM::Math::Vector2D& point) const;
    void updateSpatialHash(int obstacleId);
};

// =============================================================================
// PATH SMOOTHING AND OPTIMIZATION
// =============================================================================

/**
 * @brief Path optimizer for smoother navigation
 */
class PathOptimizer {
   public:
    PathOptimizer(const NavMesh& mesh);

    // Smoothing algorithms
    NavMeshPath smoothFunnel(const NavMeshPath& path);  // String pulling
    NavMeshPath smoothCatmullRom(const NavMeshPath& path, int segments = 3);
    NavMeshPath smoothBezier(const NavMeshPath& path, int segments = 3);

    // Path simplification
    NavMeshPath simplifyRDP(const NavMeshPath& path,
                            float epsilon = 0.1f);  // Ramer-Douglas-Peucker
    NavMeshPath removeRedundantPoints(const NavMeshPath& path, float angleThreshold = 0.1f);

    // Path shortcutting
    NavMeshPath shortcutPath(const NavMeshPath& path);
    NavMeshPath shortcutWithRaycasts(const NavMeshPath& path);

    // Configuration
    void setMaxShortcutDistance(float distance) { maxShortcutDist = distance; }
    void setRaycastStepSize(float step) { raycastStep = step; }

   private:
    const NavMesh& navMesh;
    float maxShortcutDist{50.0f};
    float raycastStep{0.5f};

    bool canShortcut(const JJM::Math::Vector2D& from, const JJM::Math::Vector2D& to) const;
};

/**
 * @brief Jump Point Search optimization for grid-based regions
 */
class JumpPointSearch {
   public:
    JumpPointSearch();

    // Initialize with walkable grid
    void initialize(int width, int height, const std::vector<bool>& walkable);

    // Find path using JPS
    std::vector<JJM::Math::Vector2D> findPath(int startX, int startY, int endX, int endY);

    // Configuration
    void setDiagonalMovement(bool allow) { allowDiagonal = allow; }
    void setCornerCutting(bool allow) { allowCornerCut = allow; }

    // Statistics
    struct JPSStats {
        int nodesExpanded;
        int jumpPointsFound;
        float searchTime;
    };
    JPSStats getLastSearchStats() const { return lastStats; }

   private:
    int gridWidth, gridHeight;
    std::vector<bool> walkableGrid;
    bool allowDiagonal{true};
    bool allowCornerCut{false};
    JPSStats lastStats{};

    // Jump point detection
    bool isJumpPoint(int x, int y, int dx, int dy) const;
    std::pair<int, int> jump(int x, int y, int dx, int dy, int endX, int endY) const;
    std::vector<std::pair<int, int>> findSuccessors(int x, int y, int px, int py, int endX,
                                                    int endY) const;
    std::vector<std::pair<int, int>> pruneNeighbors(int x, int y, int px, int py) const;

    bool isWalkable(int x, int y) const;
    bool isBlocked(int x, int y) const;
};

/**
 * @brief Theta* algorithm for any-angle pathfinding
 */
class ThetaStarPathfinder {
   public:
    ThetaStarPathfinder(const NavMesh& mesh);

    NavMeshPath findPath(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end);

    // Line of sight checking
    void setLineOfSightChecker(
        std::function<bool(const JJM::Math::Vector2D&, const JJM::Math::Vector2D&)> checker);

    // Configuration
    void setMaxIterations(int iterations) { maxIterations = iterations; }

   private:
    const NavMesh& navMesh;
    std::function<bool(const JJM::Math::Vector2D&, const JJM::Math::Vector2D&)> lineOfSight;
    int maxIterations{10000};

    bool defaultLineOfSight(const JJM::Math::Vector2D& from, const JJM::Math::Vector2D& to) const;
};

}  // namespace AI
}  // namespace JJM

#endif  // NAVIGATION_MESH_H
