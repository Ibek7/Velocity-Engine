#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "math/Vector2D.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <queue>

namespace JJM {
namespace AI {

struct GridNode {
    int x, y;
    float gCost, hCost;
    GridNode* parent;
    bool walkable;
    
    GridNode() : x(0), y(0), gCost(0), hCost(0), parent(nullptr), walkable(true) {}
    GridNode(int x, int y) : x(x), y(y), gCost(0), hCost(0), parent(nullptr), walkable(true) {}
    
    float fCost() const { return gCost + hCost; }
    
    bool operator==(const GridNode& other) const {
        return x == other.x && y == other.y;
    }
};

struct GridNodeHash {
    size_t operator()(const GridNode& node) const {
        return std::hash<int>()(node.x) ^ (std::hash<int>()(node.y) << 1);
    }
};

class Grid {
public:
    Grid(int width, int height);
    
    void setWalkable(int x, int y, bool walkable);
    bool isWalkable(int x, int y) const;
    bool inBounds(int x, int y) const;
    
    GridNode* getNode(int x, int y);
    std::vector<GridNode*> getNeighbors(GridNode* node);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width, height;
    std::vector<GridNode> nodes;
};

class AStar {
public:
    AStar(Grid* grid);
    
    std::vector<Math::Vector2D> findPath(const Math::Vector2D& start, const Math::Vector2D& goal);
    
    void setHeuristic(std::function<float(const GridNode&, const GridNode&)> heuristic) {
        this->heuristic = heuristic;
    }
    
    void setAllowDiagonal(bool allow) { allowDiagonal = allow; }
    
    static float manhattanDistance(const GridNode& a, const GridNode& b);
    static float euclideanDistance(const GridNode& a, const GridNode& b);
    static float chebyshevDistance(const GridNode& a, const GridNode& b);
    
private:
    Grid* grid;
    bool allowDiagonal;
    std::function<float(const GridNode&, const GridNode&)> heuristic;
    
    std::vector<Math::Vector2D> reconstructPath(GridNode* endNode);
};

class Dijkstra {
public:
    Dijkstra(Grid* grid);
    
    std::vector<Math::Vector2D> findPath(const Math::Vector2D& start, const Math::Vector2D& goal);
    std::unordered_map<GridNode*, float> findDistances(const Math::Vector2D& start);
    
private:
    Grid* grid;
};

class FlowField {
public:
    FlowField(Grid* grid);
    
    void generateField(const Math::Vector2D& goal);
    Math::Vector2D getDirection(const Math::Vector2D& position) const;
    float getCost(int x, int y) const;
    
private:
    Grid* grid;
    std::vector<float> costField;
    std::vector<Math::Vector2D> flowField;
    
    void calculateCostField(const Math::Vector2D& goal);
    void calculateFlowField();
};

} // namespace AI
} // namespace JJM

#endif // PATHFINDING_H
