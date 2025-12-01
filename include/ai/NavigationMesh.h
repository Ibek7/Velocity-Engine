#ifndef NAVIGATION_MESH_H
#define NAVIGATION_MESH_H

#include "../math/Vector2D.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <queue>
#include <functional>

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
    NavMeshPath findPath(const JJM::Math::Vector2D& start, 
                         const JJM::Math::Vector2D& end) const;
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
        
        bool operator>(const PathNode& other) const {
            return fCost() > other.fCost();
        }
    };
    
    std::vector<std::unique_ptr<NavMeshNode>> nodes;
    std::vector<std::unique_ptr<NavMeshEdge>> edges;
    std::unordered_map<int, bool> disabledNodes;
    
    float heuristic(int nodeId1, int nodeId2) const;
    void reconstructPath(const std::unordered_map<int, int>& cameFrom,
                        int current, NavMeshPath& path) const;
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
                              const JJM::Math::Vector2D& worldMax,
                              float cellSize,
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
        
        Cell(int _x = 0, int _y = 0, bool w = true)
            : x(_x), y(_y), walkable(w), regionId(-1) {}
    };
    
    float maxSlope;
    float agentRadius;
    float stepHeight;
    bool mergeRegions;
    
    void markWalkableGrid(std::vector<Cell>& grid, int width, int height,
                         const std::vector<bool>& walkable);
    void buildRegions(std::vector<Cell>& grid, int width, int height);
    void createPolygons(NavMesh& mesh, const std::vector<Cell>& grid,
                       int width, int height, float cellSize);
    void connectNeighborNodes(NavMesh& mesh, const std::vector<Cell>& grid,
                             int width, int height);
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
              const JJM::Math::Vector2D& vel = JJM::Math::Vector2D(0, 0),
              float r = 1.0f, int p = 0)
            : position(pos), velocity(vel), radius(r), priority(p) {}
    };
    
    LocalAvoidance();
    
    // Add/remove agents
    int addAgent(const Agent& agent);
    void removeAgent(int id);
    void updateAgent(int id, const JJM::Math::Vector2D& position,
                    const JJM::Math::Vector2D& velocity);
    
    // Calculate avoidance velocity
    JJM::Math::Vector2D computeAvoidanceVelocity(
        int agentId,
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
    
    std::vector<Line> computeORCALines(int agentId,
                                       const JJM::Math::Vector2D& preferredVelocity);
    JJM::Math::Vector2D linearProgram(const std::vector<Line>& lines,
                                     const JJM::Math::Vector2D& preferredVelocity,
                                     float maxSpeed);
};

} // namespace AI
} // namespace JJM

#endif // NAVIGATION_MESH_H
