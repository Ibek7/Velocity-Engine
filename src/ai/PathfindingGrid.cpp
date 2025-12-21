#include "ai/PathfindingGrid.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace AI {

PathfindingGrid::PathfindingGrid(int w, int h, float cs)
    : width(w), height(h), cellSize(cs), allowDiagonal(true), heuristicWeight(1.0f) {
    grid.resize(height);
    for (int y = 0; y < height; ++y) {
        grid[y].resize(width);
        for (int x = 0; x < width; ++x) {
            grid[y][x].x = x;
            grid[y][x].y = y;
            grid[y][x].walkable = true;
            grid[y][x].parent = nullptr;
        }
    }
}

PathfindingGrid::~PathfindingGrid() {
}

void PathfindingGrid::setWalkable(int x, int y, bool walkable) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        grid[y][x].walkable = walkable;
    }
}

bool PathfindingGrid::isWalkable(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return grid[y][x].walkable;
    }
    return false;
}

std::vector<Math::Vector2D> PathfindingGrid::findPath(const Math::Vector2D& start,
                                                      const Math::Vector2D& end) {
    Math::Vector2D startGrid = worldToGrid(start);
    Math::Vector2D endGrid = worldToGrid(end);
    
    int startX = static_cast<int>(startGrid.x);
    int startY = static_cast<int>(startGrid.y);
    int endX = static_cast<int>(endGrid.x);
    int endY = static_cast<int>(endGrid.y);
    
    if (!isWalkable(startX, startY) || !isWalkable(endX, endY)) {
        return {};
    }
    
    resetGrid();
    
    std::priority_queue<GridNode*> openSet;
    std::vector<std::vector<bool>> closedSet(height, std::vector<bool>(width, false));
    
    GridNode* startNode = &grid[startY][startX];
    GridNode* endNode = &grid[endY][endX];
    
    startNode->gCost = 0;
    startNode->hCost = calculateHeuristic(*startNode, *endNode);
    openSet.push(startNode);
    
    while (!openSet.empty()) {
        GridNode* current = openSet.top();
        openSet.pop();
        
        if (current == endNode) {
            std::vector<Math::Vector2D> path;
            while (current != nullptr) {
                path.push_back(gridToWorld(current->x, current->y));
                current = current->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        closedSet[current->y][current->x] = true;
        
        for (GridNode* neighbor : getNeighbors(*current)) {
            if (closedSet[neighbor->y][neighbor->x]) continue;
            
            float tentativeG = current->gCost + calculateHeuristic(*current, *neighbor);
            
            if (tentativeG < neighbor->gCost || neighbor->parent == nullptr) {
                neighbor->gCost = tentativeG;
                neighbor->hCost = calculateHeuristic(*neighbor, *endNode);
                neighbor->parent = current;
                openSet.push(neighbor);
            }
        }
    }
    
    return {};
}

void PathfindingGrid::setDiagonalMovement(bool enabled) {
    allowDiagonal = enabled;
}

void PathfindingGrid::setHeuristicWeight(float weight) {
    heuristicWeight = weight;
}

float PathfindingGrid::calculateHeuristic(const GridNode& a, const GridNode& b) {
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    return heuristicWeight * std::sqrt(static_cast<float>(dx * dx + dy * dy));
}

std::vector<GridNode*> PathfindingGrid::getNeighbors(GridNode& node) {
    std::vector<GridNode*> neighbors;
    
    int dirs[][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    int numDirs = 4;
    
    if (allowDiagonal) {
        static int diagDirs[][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0},
                                     {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        numDirs = 8;
        for (int i = 0; i < numDirs; ++i) {
            dirs[i][0] = diagDirs[i][0];
            dirs[i][1] = diagDirs[i][1];
        }
    }
    
    for (int i = 0; i < numDirs; ++i) {
        int nx = node.x + dirs[i][0];
        int ny = node.y + dirs[i][1];
        
        if (isWalkable(nx, ny)) {
            neighbors.push_back(&grid[ny][nx]);
        }
    }
    
    return neighbors;
}

void PathfindingGrid::resetGrid() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            grid[y][x].gCost = INFINITY;
            grid[y][x].hCost = 0;
            grid[y][x].parent = nullptr;
        }
    }
}

Math::Vector2D PathfindingGrid::worldToGrid(const Math::Vector2D& world) {
    return Math::Vector2D(world.x / cellSize, world.y / cellSize);
}

Math::Vector2D PathfindingGrid::gridToWorld(int x, int y) {
    return Math::Vector2D((x + 0.5f) * cellSize, (y + 0.5f) * cellSize);
}

} // namespace AI
} // namespace JJM
