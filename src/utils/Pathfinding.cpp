#include "utils/Pathfinding.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Utils {

// Pathfinding implementation
Pathfinding::Pathfinding(int width, int height, IsWalkableFunc walkableFunc, bool diagonal)
    : width(width), height(height), isWalkable(walkableFunc), allowDiagonal(diagonal) {
}

Pathfinding::~Pathfinding() {
}

std::vector<Math::Vector2D> Pathfinding::findPath(const Math::Vector2D& start, const Math::Vector2D& end) {
    return findPath(static_cast<int>(start.x), static_cast<int>(start.y),
                   static_cast<int>(end.x), static_cast<int>(end.y));
}

std::vector<Math::Vector2D> Pathfinding::findPath(int startX, int startY, int endX, int endY) {
    if (!isWalkable(startX, startY) || !isWalkable(endX, endY)) {
        return {};
    }
    
    std::priority_queue<PathNode*, std::vector<PathNode*>, PathNodeCompare> openSet;
    std::unordered_map<int, PathNode*> openMap;
    std::unordered_map<int, PathNode*> closedMap;
    
    auto getKey = [this](int x, int y) { return y * width + x; };
    
    PathNode* startNode = new PathNode(startX, startY);
    startNode->g = 0;
    startNode->h = heuristic(startX, startY, endX, endY);
    startNode->f = startNode->h;
    
    openSet.push(startNode);
    openMap[getKey(startX, startY)] = startNode;
    
    PathNode* endNode = nullptr;
    
    while (!openSet.empty()) {
        PathNode* current = openSet.top();
        openSet.pop();
        
        int currentKey = getKey(current->x, current->y);
        openMap.erase(currentKey);
        closedMap[currentKey] = current;
        
        if (current->x == endX && current->y == endY) {
            endNode = current;
            break;
        }
        
        std::vector<PathNode> neighbors = getNeighbors(*current);
        
        for (const auto& neighbor : neighbors) {
            int neighborKey = getKey(neighbor.x, neighbor.y);
            
            if (closedMap.find(neighborKey) != closedMap.end()) {
                continue;
            }
            
            float tentativeG = current->g + distance(current->x, current->y, neighbor.x, neighbor.y);
            
            auto openIt = openMap.find(neighborKey);
            if (openIt != openMap.end()) {
                if (tentativeG < openIt->second->g) {
                    openIt->second->g = tentativeG;
                    openIt->second->f = tentativeG + openIt->second->h;
                    openIt->second->parent = current;
                }
            } else {
                PathNode* newNode = new PathNode(neighbor.x, neighbor.y);
                newNode->g = tentativeG;
                newNode->h = heuristic(neighbor.x, neighbor.y, endX, endY);
                newNode->f = newNode->g + newNode->h;
                newNode->parent = current;
                
                openSet.push(newNode);
                openMap[neighborKey] = newNode;
            }
        }
    }
    
    std::vector<Math::Vector2D> path;
    if (endNode) {
        path = reconstructPath(endNode);
    }
    
    // Clean up
    for (auto& pair : openMap) {
        delete pair.second;
    }
    for (auto& pair : closedMap) {
        delete pair.second;
    }
    
    return path;
}

float Pathfinding::heuristic(int x1, int y1, int x2, int y2) {
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    return std::sqrt(dx * dx + dy * dy);
}

float Pathfinding::distance(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<PathNode> Pathfinding::getNeighbors(const PathNode& node) {
    std::vector<PathNode> neighbors;
    
    static const int dx[] = {0, 0, -1, 1, -1, -1, 1, 1};
    static const int dy[] = {-1, 1, 0, 0, -1, 1, -1, 1};
    
    int maxNeighbors = allowDiagonal ? 8 : 4;
    
    for (int i = 0; i < maxNeighbors; i++) {
        int nx = node.x + dx[i];
        int ny = node.y + dy[i];
        
        if (nx >= 0 && nx < width && ny >= 0 && ny < height && isWalkable(nx, ny)) {
            neighbors.push_back(PathNode(nx, ny));
        }
    }
    
    return neighbors;
}

std::vector<Math::Vector2D> Pathfinding::reconstructPath(PathNode* endNode) {
    std::vector<Math::Vector2D> path;
    
    PathNode* current = endNode;
    while (current) {
        path.push_back(Math::Vector2D(current->x, current->y));
        current = current->parent;
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}

// GridPathfinding implementation
GridPathfinding::GridPathfinding(int width, int height)
    : width(width), height(height) {
    grid.resize(height, std::vector<bool>(width, true));
    
    pathfinder = new Pathfinding(width, height,
        [this](int x, int y) { return this->isWalkable(x, y); });
}

GridPathfinding::~GridPathfinding() {
    delete pathfinder;
}

void GridPathfinding::setWalkable(int x, int y, bool walkable) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        grid[y][x] = walkable;
    }
}

bool GridPathfinding::isWalkable(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return grid[y][x];
    }
    return false;
}

void GridPathfinding::setAllWalkable(bool walkable) {
    for (auto& row : grid) {
        std::fill(row.begin(), row.end(), walkable);
    }
}

void GridPathfinding::clearGrid() {
    setAllWalkable(true);
}

std::vector<Math::Vector2D> GridPathfinding::findPath(const Math::Vector2D& start, const Math::Vector2D& end) {
    return pathfinder->findPath(start, end);
}

} // namespace Utils
} // namespace JJM
