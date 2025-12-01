#include "../../include/ai/NavigationMesh.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace JJM {
namespace AI {

// =============================================================================
// NavMeshNode Implementation
// =============================================================================

NavMeshNode::NavMeshNode(int id) : id(id), cost(1.0f) {}

void NavMeshNode::addVertex(const JJM::Math::Vector2D& vertex) {
    vertices.push_back(vertex);
}

void NavMeshNode::addEdge(int edgeId, int neighborId) {
    edgeIds.push_back(edgeId);
    neighborIds.push_back(neighborId);
}

JJM::Math::Vector2D NavMeshNode::getCenter() const {
    if (vertices.empty()) return JJM::Math::Vector2D(0, 0);
    
    JJM::Math::Vector2D center(0, 0);
    for (const auto& v : vertices) {
        center = center + v;
    }
    return center * (1.0f / vertices.size());
}

bool NavMeshNode::containsPoint(const JJM::Math::Vector2D& point) const {
    if (vertices.size() < 3) return false;
    
    // Ray casting algorithm
    bool inside = false;
    size_t j = vertices.size() - 1;
    
    for (size_t i = 0; i < vertices.size(); i++) {
        if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
            (point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) /
                      (vertices[j].y - vertices[i].y) + vertices[i].x)) {
            inside = !inside;
        }
        j = i;
    }
    
    return inside;
}

// =============================================================================
// NavMeshEdge Implementation
// =============================================================================

NavMeshEdge::NavMeshEdge(int id, const JJM::Math::Vector2D& start, 
                        const JJM::Math::Vector2D& end)
    : id(id), start(start), end(end) {}

// =============================================================================
// NavMesh Implementation
// =============================================================================

NavMesh::NavMesh() {}

NavMesh::~NavMesh() {
    clear();
}

int NavMesh::addNode(const std::vector<JJM::Math::Vector2D>& vertices) {
    int id = static_cast<int>(nodes.size());
    auto node = std::make_unique<NavMeshNode>(id);
    
    for (const auto& v : vertices) {
        node->addVertex(v);
    }
    
    nodes.push_back(std::move(node));
    return id;
}

NavMeshNode* NavMesh::getNode(int id) {
    if (id >= 0 && id < static_cast<int>(nodes.size())) {
        return nodes[id].get();
    }
    return nullptr;
}

const NavMeshNode* NavMesh::getNode(int id) const {
    if (id >= 0 && id < static_cast<int>(nodes.size())) {
        return nodes[id].get();
    }
    return nullptr;
}

int NavMesh::addEdge(const JJM::Math::Vector2D& start, const JJM::Math::Vector2D& end) {
    int id = static_cast<int>(edges.size());
    edges.push_back(std::make_unique<NavMeshEdge>(id, start, end));
    return id;
}

NavMeshEdge* NavMesh::getEdge(int id) {
    if (id >= 0 && id < static_cast<int>(edges.size())) {
        return edges[id].get();
    }
    return nullptr;
}

const NavMeshEdge* NavMesh::getEdge(int id) const {
    if (id >= 0 && id < static_cast<int>(edges.size())) {
        return edges[id].get();
    }
    return nullptr;
}

void NavMesh::connectNodes(int nodeId1, int nodeId2, int edgeId) {
    NavMeshNode* node1 = getNode(nodeId1);
    NavMeshNode* node2 = getNode(nodeId2);
    
    if (node1 && node2) {
        node1->addEdge(edgeId, nodeId2);
        node2->addEdge(edgeId, nodeId1);
    }
}

int NavMesh::findNodeContainingPoint(const JJM::Math::Vector2D& point) const {
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->containsPoint(point)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

NavMeshPath NavMesh::findPath(const JJM::Math::Vector2D& start, 
                              const JJM::Math::Vector2D& end) const {
    int startNode = findNodeContainingPoint(start);
    int endNode = findNodeContainingPoint(end);
    
    if (startNode < 0 || endNode < 0) {
        return NavMeshPath();
    }
    
    NavMeshPath path = findPath(startNode, endNode);
    
    if (path.valid) {
        // Add actual start and end points
        path.waypoints.insert(path.waypoints.begin(), start);
        path.waypoints.push_back(end);
    }
    
    return path;
}

NavMeshPath NavMesh::findPath(int startNodeId, int endNodeId) const {
    NavMeshPath result;
    
    if (startNodeId < 0 || endNodeId < 0 ||
        startNodeId >= static_cast<int>(nodes.size()) ||
        endNodeId >= static_cast<int>(nodes.size())) {
        return result;
    }
    
    if (startNodeId == endNodeId) {
        result.waypoints.push_back(nodes[startNodeId]->getCenter());
        result.valid = true;
        return result;
    }
    
    // A* pathfinding
    std::priority_queue<PathNode, std::vector<PathNode>, std::greater<PathNode>> openSet;
    std::unordered_map<int, float> gScores;
    std::unordered_map<int, int> cameFrom;
    
    openSet.emplace(startNodeId, 0, heuristic(startNodeId, endNodeId), -1);
    gScores[startNodeId] = 0;
    
    while (!openSet.empty()) {
        PathNode current = openSet.top();
        openSet.pop();
        
        if (current.nodeId == endNodeId) {
            reconstructPath(cameFrom, current.nodeId, result);
            result.valid = true;
            return result;
        }
        
        // Skip if disabled
        if (disabledNodes.find(current.nodeId) != disabledNodes.end()) {
            continue;
        }
        
        const NavMeshNode* node = getNode(current.nodeId);
        if (!node) continue;
        
        for (int neighborId : node->getNeighborIds()) {
            if (disabledNodes.find(neighborId) != disabledNodes.end()) {
                continue;
            }
            
            const NavMeshNode* neighbor = getNode(neighborId);
            if (!neighbor) continue;
            
            float edgeCost = (node->getCenter() - neighbor->getCenter()).magnitude();
            float tentativeG = current.gCost + edgeCost * neighbor->getCost();
            
            if (gScores.find(neighborId) == gScores.end() || 
                tentativeG < gScores[neighborId]) {
                gScores[neighborId] = tentativeG;
                cameFrom[neighborId] = current.nodeId;
                
                float h = heuristic(neighborId, endNodeId);
                openSet.emplace(neighborId, tentativeG, h, current.nodeId);
            }
        }
    }
    
    return result;
}

void NavMesh::smoothPath(NavMeshPath& path) const {
    if (path.waypoints.size() <= 2) return;
    
    // String pulling algorithm for smoother paths
    std::vector<JJM::Math::Vector2D> smoothed;
    smoothed.push_back(path.waypoints.front());
    
    size_t current = 0;
    while (current < path.waypoints.size() - 1) {
        size_t farthest = current + 1;
        
        // Find farthest visible waypoint
        for (size_t i = current + 2; i < path.waypoints.size(); i++) {
            // Simplified visibility check (would need proper line-of-sight test)
            farthest = i;
        }
        
        if (farthest != current + 1) {
            smoothed.push_back(path.waypoints[farthest]);
        }
        current = farthest;
    }
    
    smoothed.push_back(path.waypoints.back());
    path.waypoints = smoothed;
}

void NavMesh::setNodeCost(int nodeId, float cost) {
    NavMeshNode* node = getNode(nodeId);
    if (node) {
        node->setCost(cost);
    }
}

void NavMesh::disableNode(int nodeId) {
    disabledNodes[nodeId] = true;
}

void NavMesh::enableNode(int nodeId) {
    disabledNodes.erase(nodeId);
}

void NavMesh::clear() {
    nodes.clear();
    edges.clear();
    disabledNodes.clear();
}

float NavMesh::heuristic(int nodeId1, int nodeId2) const {
    const NavMeshNode* n1 = getNode(nodeId1);
    const NavMeshNode* n2 = getNode(nodeId2);
    
    if (!n1 || !n2) return 0.0f;
    
    return (n1->getCenter() - n2->getCenter()).magnitude();
}

void NavMesh::reconstructPath(const std::unordered_map<int, int>& cameFrom,
                             int current, NavMeshPath& path) const {
    std::vector<int> nodeIds;
    nodeIds.push_back(current);
    
    while (cameFrom.find(current) != cameFrom.end()) {
        current = cameFrom.at(current);
        nodeIds.push_back(current);
    }
    
    std::reverse(nodeIds.begin(), nodeIds.end());
    
    path.totalCost = 0.0f;
    for (size_t i = 0; i < nodeIds.size(); i++) {
        const NavMeshNode* node = getNode(nodeIds[i]);
        if (node) {
            path.waypoints.push_back(node->getCenter());
            
            if (i > 0) {
                const NavMeshNode* prev = getNode(nodeIds[i - 1]);
                if (prev) {
                    path.totalCost += (node->getCenter() - prev->getCenter()).magnitude();
                }
            }
        }
    }
}

// =============================================================================
// NavMeshBuilder Implementation
// =============================================================================

NavMeshBuilder::NavMeshBuilder()
    : maxSlope(45.0f), agentRadius(0.5f), stepHeight(0.5f), mergeRegions(true) {}

NavMesh NavMeshBuilder::buildFromGrid(int gridWidth, int gridHeight, float cellSize,
                                     const std::vector<bool>& walkable) {
    NavMesh mesh;
    
    if (walkable.size() != static_cast<size_t>(gridWidth * gridHeight)) {
        return mesh;
    }
    
    std::vector<Cell> grid(gridWidth * gridHeight);
    
    // Initialize cells
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            int idx = y * gridWidth + x;
            grid[idx] = Cell(x, y, walkable[idx]);
        }
    }
    
    // Build regions
    buildRegions(grid, gridWidth, gridHeight);
    
    // Create polygons
    createPolygons(mesh, grid, gridWidth, gridHeight, cellSize);
    
    // Connect neighbors
    connectNeighborNodes(mesh, grid, gridWidth, gridHeight);
    
    return mesh;
}

NavMesh NavMeshBuilder::buildFromObstacles(const JJM::Math::Vector2D& worldMin,
                                          const JJM::Math::Vector2D& worldMax,
                                          float cellSize,
                                          const std::vector<JJM::Math::Vector2D>& obstaclePoints,
                                          float obstacleRadius) {
    int gridWidth = static_cast<int>((worldMax.x - worldMin.x) / cellSize);
    int gridHeight = static_cast<int>((worldMax.y - worldMin.y) / cellSize);
    
    std::vector<bool> walkable(gridWidth * gridHeight, true);
    
    // Mark obstacle cells as unwalkable
    for (const auto& obstacle : obstaclePoints) {
        int gridX = static_cast<int>((obstacle.x - worldMin.x) / cellSize);
        int gridY = static_cast<int>((obstacle.y - worldMin.y) / cellSize);
        
        int radius = static_cast<int>(obstacleRadius / cellSize) + 1;
        
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                int x = gridX + dx;
                int y = gridY + dy;
                
                if (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight) {
                    float dist = std::sqrt(dx * dx + dy * dy) * cellSize;
                    if (dist <= obstacleRadius) {
                        walkable[y * gridWidth + x] = false;
                    }
                }
            }
        }
    }
    
    return buildFromGrid(gridWidth, gridHeight, cellSize, walkable);
}

void NavMeshBuilder::buildRegions(std::vector<Cell>& grid, int width, int height) {
    int regionId = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            if (grid[idx].walkable && grid[idx].regionId < 0) {
                // Flood fill region
                std::queue<int> queue;
                queue.push(idx);
                grid[idx].regionId = regionId;
                
                while (!queue.empty()) {
                    int current = queue.front();
                    queue.pop();
                    
                    int cx = current % width;
                    int cy = current / width;
                    
                    // Check 4-connected neighbors
                    int dx[] = {0, 1, 0, -1};
                    int dy[] = {-1, 0, 1, 0};
                    
                    for (int i = 0; i < 4; i++) {
                        int nx = cx + dx[i];
                        int ny = cy + dy[i];
                        
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            int nidx = ny * width + nx;
                            
                            if (grid[nidx].walkable && grid[nidx].regionId < 0) {
                                grid[nidx].regionId = regionId;
                                queue.push(nidx);
                            }
                        }
                    }
                }
                
                regionId++;
            }
        }
    }
}

void NavMeshBuilder::createPolygons(NavMesh& mesh, const std::vector<Cell>& grid,
                                   int width, int height, float cellSize) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            if (grid[idx].walkable) {
                std::vector<JJM::Math::Vector2D> vertices;
                
                // Create quad for this cell
                float x0 = x * cellSize;
                float y0 = y * cellSize;
                float x1 = (x + 1) * cellSize;
                float y1 = (y + 1) * cellSize;
                
                vertices.emplace_back(x0, y0);
                vertices.emplace_back(x1, y0);
                vertices.emplace_back(x1, y1);
                vertices.emplace_back(x0, y1);
                
                mesh.addNode(vertices);
            }
        }
    }
}

void NavMeshBuilder::connectNeighborNodes(NavMesh& mesh, const std::vector<Cell>& grid,
                                         int width, int height) {
    int nodeId = 0;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;
            
            if (grid[idx].walkable) {
                // Check right neighbor
                if (x + 1 < width && grid[idx + 1].walkable) {
                    NavMeshNode* current = mesh.getNode(nodeId);
                    if (current && current->getVertices().size() >= 2) {
                        int edgeId = mesh.addEdge(current->getVertices()[1], 
                                                 current->getVertices()[2]);
                        mesh.connectNodes(nodeId, nodeId + 1, edgeId);
                    }
                }
                
                // Check down neighbor
                if (y + 1 < height && grid[idx + width].walkable) {
                    NavMeshNode* current = mesh.getNode(nodeId);
                    if (current && current->getVertices().size() >= 3) {
                        int downId = nodeId + width;
                        int edgeId = mesh.addEdge(current->getVertices()[2],
                                                 current->getVertices()[3]);
                        mesh.connectNodes(nodeId, downId, edgeId);
                    }
                }
                
                nodeId++;
            }
        }
    }
}

// =============================================================================
// NavMeshAgent Implementation
// =============================================================================

NavMeshAgent::NavMeshAgent(const NavMesh* navMesh)
    : navMesh(navMesh), currentWaypointIndex(0), maxSpeed(5.0f),
      acceleration(10.0f), stoppingDistance(0.5f), autoRepath(true),
      repathInterval(1.0f), timeSinceLastPath(0.0f), paused(false) {}

void NavMeshAgent::setPosition(const JJM::Math::Vector2D& pos) {
    position = pos;
}

void NavMeshAgent::setDestination(const JJM::Math::Vector2D& dest) {
    destination = dest;
    calculatePath();
}

void NavMeshAgent::update(float deltaTime) {
    if (paused || !currentPath.valid) return;
    
    timeSinceLastPath += deltaTime;
    
    // Auto repath if enabled
    if (autoRepath && timeSinceLastPath >= repathInterval) {
        calculatePath();
        timeSinceLastPath = 0.0f;
    }
    
    followPath(deltaTime);
}

bool NavMeshAgent::isAtDestination() const {
    return (position - destination).magnitude() < stoppingDistance;
}

void NavMeshAgent::stop() {
    currentPath.clear();
    velocity = JJM::Math::Vector2D(0, 0);
}

void NavMeshAgent::pause() {
    paused = true;
}

void NavMeshAgent::resume() {
    paused = false;
}

void NavMeshAgent::calculatePath() {
    if (!navMesh) return;
    
    currentPath = navMesh->findPath(position, destination);
    currentWaypointIndex = 0;
}

void NavMeshAgent::followPath(float deltaTime) {
    if (currentPath.waypoints.empty() || 
        currentWaypointIndex >= static_cast<int>(currentPath.waypoints.size())) {
        velocity = JJM::Math::Vector2D(0, 0);
        return;
    }
    
    JJM::Math::Vector2D target = currentPath.waypoints[currentWaypointIndex];
    
    // Check if reached current waypoint
    if ((position - target).magnitude() < stoppingDistance) {
        currentWaypointIndex++;
        if (currentWaypointIndex >= static_cast<int>(currentPath.waypoints.size())) {
            velocity = JJM::Math::Vector2D(0, 0);
            return;
        }
        target = currentPath.waypoints[currentWaypointIndex];
    }
    
    // Steer towards target
    JJM::Math::Vector2D desired = steer(target, deltaTime);
    position = position + desired * deltaTime;
    velocity = desired;
}

JJM::Math::Vector2D NavMeshAgent::steer(const JJM::Math::Vector2D& target, float deltaTime) {
    JJM::Math::Vector2D desired = target - position;
    float distance = desired.magnitude();
    
    if (distance < 0.001f) {
        return JJM::Math::Vector2D(0, 0);
    }
    
    desired.normalize();
    desired = desired * maxSpeed;
    
    // Decelerate when approaching
    if (distance < stoppingDistance * 2.0f) {
        desired = desired * (distance / (stoppingDistance * 2.0f));
    }
    
    JJM::Math::Vector2D steer = desired - velocity;
    float maxAccel = acceleration * deltaTime;
    
    if (steer.magnitude() > maxAccel) {
        steer.normalize();
        steer = steer * maxAccel;
    }
    
    return velocity + steer;
}

// =============================================================================
// LocalAvoidance Implementation
// =============================================================================

LocalAvoidance::LocalAvoidance()
    : nextAgentId(0), timeHorizon(1.5f), maxNeighbors(10), neighborDistance(5.0f) {}

int LocalAvoidance::addAgent(const Agent& agent) {
    int id = nextAgentId++;
    agents[id] = agent;
    return id;
}

void LocalAvoidance::removeAgent(int id) {
    agents.erase(id);
}

void LocalAvoidance::updateAgent(int id, const JJM::Math::Vector2D& position,
                                const JJM::Math::Vector2D& velocity) {
    auto it = agents.find(id);
    if (it != agents.end()) {
        it->second.position = position;
        it->second.velocity = velocity;
    }
}

JJM::Math::Vector2D LocalAvoidance::computeAvoidanceVelocity(
    int agentId,
    const JJM::Math::Vector2D& preferredVelocity,
    float maxSpeed) {
    
    auto it = agents.find(agentId);
    if (it == agents.end()) {
        return preferredVelocity;
    }
    
    // Compute ORCA lines
    std::vector<Line> lines = computeORCALines(agentId, preferredVelocity);
    
    // Linear programming to find optimal velocity
    return linearProgram(lines, preferredVelocity, maxSpeed);
}

void LocalAvoidance::clear() {
    agents.clear();
    nextAgentId = 0;
}

std::vector<LocalAvoidance::Line> LocalAvoidance::computeORCALines(
    int agentId,
    const JJM::Math::Vector2D& preferredVelocity) {
    
    std::vector<Line> lines;
    
    const Agent& agent = agents[agentId];
    
    // Find nearby agents
    for (const auto& pair : agents) {
        if (pair.first == agentId) continue;
        
        const Agent& other = pair.second;
        float dist = (other.position - agent.position).magnitude();
        
        if (dist < neighborDistance) {
            // Simplified ORCA line computation
            JJM::Math::Vector2D relativePosition = other.position - agent.position;
            JJM::Math::Vector2D relativeVelocity = agent.velocity - other.velocity;
            
            Line line;
            line.point = agent.velocity;
            line.direction = relativePosition;
            line.direction.normalize();
            
            lines.push_back(line);
        }
    }
    
    return lines;
}

JJM::Math::Vector2D LocalAvoidance::linearProgram(
    const std::vector<Line>& lines,
    const JJM::Math::Vector2D& preferredVelocity,
    float maxSpeed) {
    
    // Simplified: just return preferred velocity clamped to max speed
    JJM::Math::Vector2D result = preferredVelocity;
    
    if (result.magnitude() > maxSpeed) {
        result.normalize();
        result = result * maxSpeed;
    }
    
    return result;
}

} // namespace AI
} // namespace JJM
