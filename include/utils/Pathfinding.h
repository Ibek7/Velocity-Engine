#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "math/Vector2D.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <queue>

namespace JJM {
namespace Utils {

struct PathNode {
    int x, y;
    float g, h, f;
    PathNode* parent;
    
    PathNode(int x = 0, int y = 0) 
        : x(x), y(y), g(0), h(0), f(0), parent(nullptr) {}
    
    bool operator==(const PathNode& other) const {
        return x == other.x && y == other.y;
    }
};

struct PathNodeHash {
    std::size_t operator()(const PathNode& node) const {
        return std::hash<int>()(node.x) ^ (std::hash<int>()(node.y) << 1);
    }
};

struct PathNodeCompare {
    bool operator()(const PathNode* a, const PathNode* b) const {
        return a->f > b->f;
    }
};

class Pathfinding {
public:
    using IsWalkableFunc = std::function<bool(int, int)>;
    
private:
    int width;
    int height;
    IsWalkableFunc isWalkable;
    bool allowDiagonal;
    
public:
    Pathfinding(int width, int height, IsWalkableFunc walkableFunc, bool diagonal = true);
    ~Pathfinding();
    
    std::vector<Math::Vector2D> findPath(const Math::Vector2D& start, const Math::Vector2D& end);
    std::vector<Math::Vector2D> findPath(int startX, int startY, int endX, int endY);
    
    void setAllowDiagonal(bool allow) { allowDiagonal = allow; }
    bool getAllowDiagonal() const { return allowDiagonal; }
    
    static float heuristic(int x1, int y1, int x2, int y2);
    static float distance(int x1, int y1, int x2, int y2);
    
private:
    std::vector<PathNode> getNeighbors(const PathNode& node);
    std::vector<Math::Vector2D> reconstructPath(PathNode* endNode);
};

class GridPathfinding {
private:
    std::vector<std::vector<bool>> grid;
    int width;
    int height;
    Pathfinding* pathfinder;
    
public:
    GridPathfinding(int width, int height);
    ~GridPathfinding();
    
    void setWalkable(int x, int y, bool walkable);
    bool isWalkable(int x, int y) const;
    
    void setAllWalkable(bool walkable);
    void clearGrid();
    
    std::vector<Math::Vector2D> findPath(const Math::Vector2D& start, const Math::Vector2D& end);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
};

} // namespace Utils
} // namespace JJM

#endif // PATHFINDING_H
