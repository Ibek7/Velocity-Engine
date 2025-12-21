#ifndef JJM_PATHFINDING_GRID_H
#define JJM_PATHFINDING_GRID_H

#include "math/Vector2D.h"
#include <vector>
#include <queue>
#include <functional>

namespace JJM {
namespace AI {

struct GridNode {
    int x, y;
    bool walkable;
    float gCost, hCost;
    GridNode* parent;
    
    float fCost() const { return gCost + hCost; }
    
    bool operator<(const GridNode& other) const {
        return fCost() > other.fCost();
    }
};

class PathfindingGrid {
public:
    PathfindingGrid(int width, int height, float cellSize);
    ~PathfindingGrid();
    
    void setWalkable(int x, int y, bool walkable);
    bool isWalkable(int x, int y) const;
    
    std::vector<Math::Vector2D> findPath(const Math::Vector2D& start, 
                                         const Math::Vector2D& end);
    
    void setDiagonalMovement(bool enabled);
    void setHeuristicWeight(float weight);

private:
    int width, height;
    float cellSize;
    std::vector<std::vector<GridNode>> grid;
    bool allowDiagonal;
    float heuristicWeight;
    
    float calculateHeuristic(const GridNode& a, const GridNode& b);
    std::vector<GridNode*> getNeighbors(GridNode& node);
    void resetGrid();
    
    Math::Vector2D worldToGrid(const Math::Vector2D& world);
    Math::Vector2D gridToWorld(int x, int y);
};

} // namespace AI
} // namespace JJM

#endif
