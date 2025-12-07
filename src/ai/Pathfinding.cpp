#include "ai/Pathfinding.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace JJM {
namespace AI {

// Grid implementation
Grid::Grid(int width, int height)
    : width(width), height(height) {
    nodes.reserve(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            nodes.emplace_back(x, y);
        }
    }
}

void Grid::setWalkable(int x, int y, bool walkable) {
    if (inBounds(x, y)) {
        nodes[y * width + x].walkable = walkable;
    }
}

bool Grid::isWalkable(int x, int y) const {
    if (!inBounds(x, y)) return false;
    return nodes[y * width + x].walkable;
}

bool Grid::inBounds(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

GridNode* Grid::getNode(int x, int y) {
    if (!inBounds(x, y)) return nullptr;
    return &nodes[y * width + x];
}

std::vector<GridNode*> Grid::getNeighbors(GridNode* node) {
    std::vector<GridNode*> neighbors;
    
    static const int dx[] = {-1, 1, 0, 0, -1, 1, -1, 1};
    static const int dy[] = {0, 0, -1, 1, -1, -1, 1, 1};
    
    for (int i = 0; i < 8; ++i) {
        int nx = node->x + dx[i];
        int ny = node->y + dy[i];
        
        if (inBounds(nx, ny) && isWalkable(nx, ny)) {
            neighbors.push_back(getNode(nx, ny));
        }
    }
    
    return neighbors;
}

// AStar implementation
AStar::AStar(Grid* grid)
    : grid(grid), allowDiagonal(true), heuristic(manhattanDistance) {
}

float AStar::manhattanDistance(const GridNode& a, const GridNode& b) {
    return static_cast<float>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
}

float AStar::euclideanDistance(const GridNode& a, const GridNode& b) {
    float dx = static_cast<float>(a.x - b.x);
    float dy = static_cast<float>(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

float AStar::chebyshevDistance(const GridNode& a, const GridNode& b) {
    return static_cast<float>(std::max(std::abs(a.x - b.x), std::abs(a.y - b.y)));
}

std::vector<Math::Vector2D> AStar::findPath(const Math::Vector2D& start, const Math::Vector2D& goal) {
    GridNode* startNode = grid->getNode(static_cast<int>(start.x), static_cast<int>(start.y));
    GridNode* goalNode = grid->getNode(static_cast<int>(goal.x), static_cast<int>(goal.y));
    
    if (!startNode || !goalNode || !goalNode->walkable) {
        return {};
    }
    
    auto compare = [](GridNode* a, GridNode* b) { return a->fCost() > b->fCost(); };
    std::priority_queue<GridNode*, std::vector<GridNode*>, decltype(compare)> openSet(compare);
    std::unordered_map<GridNode*, bool> closedSet;
    
    startNode->gCost = 0;
    startNode->hCost = heuristic(*startNode, *goalNode);
    startNode->parent = nullptr;
    openSet.push(startNode);
    
    while (!openSet.empty()) {
        GridNode* current = openSet.top();
        openSet.pop();
        
        if (current == goalNode) {
            return reconstructPath(goalNode);
        }
        
        closedSet[current] = true;
        
        for (GridNode* neighbor : grid->getNeighbors(current)) {
            if (closedSet.find(neighbor) != closedSet.end()) continue;
            
            float tentativeG = current->gCost + 1.0f; // Assume uniform cost
            
            if (tentativeG < neighbor->gCost || neighbor->parent == nullptr) {
                neighbor->parent = current;
                neighbor->gCost = tentativeG;
                neighbor->hCost = heuristic(*neighbor, *goalNode);
                openSet.push(neighbor);
            }
        }
    }
    
    return {}; // No path found
}

std::vector<Math::Vector2D> AStar::reconstructPath(GridNode* endNode) {
    std::vector<GridNode*> nodePath;
    GridNode* current = endNode;
    
    while (current != nullptr) {
        nodePath.push_back(current);
        current = current->parent;
    }
    
    std::reverse(nodePath.begin(), nodePath.end());
    
    std::vector<Math::Vector2D> result;
    for (GridNode* node : nodePath) {
        result.emplace_back(static_cast<float>(node->x), static_cast<float>(node->y));
    }
    
    return result;
}

// Dijkstra implementation
Dijkstra::Dijkstra(Grid* grid)
    : grid(grid) {
}

std::vector<Math::Vector2D> Dijkstra::findPath(const Math::Vector2D& start, const Math::Vector2D& goal) {
    GridNode* startNode = grid->getNode(static_cast<int>(start.x), static_cast<int>(start.y));
    GridNode* goalNode = grid->getNode(static_cast<int>(goal.x), static_cast<int>(goal.y));
    
    if (!startNode || !goalNode) return {};
    
    auto distances = findDistances(start);
    
    std::vector<Math::Vector2D> path;
    GridNode* current = goalNode;
    
    while (current != startNode && current != nullptr) {
        path.emplace_back(static_cast<float>(current->x), static_cast<float>(current->y));
        current = current->parent;
    }
    
    if (current == startNode) {
        path.emplace_back(static_cast<float>(startNode->x), static_cast<float>(startNode->y));
        std::reverse(path.begin(), path.end());
    }
    
    return path;
}

std::unordered_map<GridNode*, float> Dijkstra::findDistances(const Math::Vector2D& start) {
    GridNode* startNode = grid->getNode(static_cast<int>(start.x), static_cast<int>(start.y));
    
    std::unordered_map<GridNode*, float> distances;
    auto compare = [&](GridNode* a, GridNode* b) { return distances[a] > distances[b]; };
    std::priority_queue<GridNode*, std::vector<GridNode*>, decltype(compare)> pq(compare);
    
    distances[startNode] = 0;
    pq.push(startNode);
    
    while (!pq.empty()) {
        GridNode* current = pq.top();
        pq.pop();
        
        for (GridNode* neighbor : grid->getNeighbors(current)) {
            float newDist = distances[current] + 1.0f;
            
            if (distances.find(neighbor) == distances.end() || newDist < distances[neighbor]) {
                distances[neighbor] = newDist;
                neighbor->parent = current;
                pq.push(neighbor);
            }
        }
    }
    
    return distances;
}

// FlowField implementation
FlowField::FlowField(Grid* grid)
    : grid(grid) {
    int size = grid->getWidth() * grid->getHeight();
    costField.resize(size, std::numeric_limits<float>::max());
    flowField.resize(size);
}

void FlowField::generateField(const Math::Vector2D& goal) {
    calculateCostField(goal);
    calculateFlowField();
}

void FlowField::calculateCostField(const Math::Vector2D& goal) {
    std::fill(costField.begin(), costField.end(), std::numeric_limits<float>::max());
    
    GridNode* goalNode = grid->getNode(static_cast<int>(goal.x), static_cast<int>(goal.y));
    if (!goalNode) return;
    
    int width = grid->getWidth();
    costField[goalNode->y * width + goalNode->x] = 0;
    
    std::queue<GridNode*> frontier;
    frontier.push(goalNode);
    
    while (!frontier.empty()) {
        GridNode* current = frontier.front();
        frontier.pop();
        
        float currentCost = costField[current->y * width + current->x];
        
        for (GridNode* neighbor : grid->getNeighbors(current)) {
            int idx = neighbor->y * width + neighbor->x;
            float newCost = currentCost + 1.0f;
            
            if (newCost < costField[idx]) {
                costField[idx] = newCost;
                frontier.push(neighbor);
            }
        }
    }
}

void FlowField::calculateFlowField() {
    int width = grid->getWidth();
    int height = grid->getHeight();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            GridNode* node = grid->getNode(x, y);
            if (!node || !node->walkable) continue;
            
            GridNode* bestNeighbor = nullptr;
            float lowestCost = costField[y * width + x];
            
            for (GridNode* neighbor : grid->getNeighbors(node)) {
                int idx = neighbor->y * width + neighbor->x;
                if (costField[idx] < lowestCost) {
                    lowestCost = costField[idx];
                    bestNeighbor = neighbor;
                }
            }
            
            if (bestNeighbor) {
                Math::Vector2D dir(
                    static_cast<float>(bestNeighbor->x - x),
                    static_cast<float>(bestNeighbor->y - y)
                );
                flowField[y * width + x] = dir.normalized();
            }
        }
    }
}

Math::Vector2D FlowField::getDirection(const Math::Vector2D& position) const {
    int x = static_cast<int>(position.x);
    int y = static_cast<int>(position.y);
    
    if (!grid->inBounds(x, y)) return Math::Vector2D(0, 0);
    
    return flowField[y * grid->getWidth() + x];
}

float FlowField::getCost(int x, int y) const {
    if (!grid->inBounds(x, y)) return std::numeric_limits<float>::max();
    return costField[y * grid->getWidth() + x];
}

} // namespace AI
} // namespace JJM
