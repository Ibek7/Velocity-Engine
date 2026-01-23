#include "physics/BroadPhase.h"
#include <algorithm>
#include <cmath>

namespace Engine {

// AABB Implementation
bool AABB::intersects(const AABB& other) const {
    return !(maxX < other.minX || minX > other.maxX ||
             maxY < other.minY || minY > other.maxY ||
             maxZ < other.minZ || minZ > other.maxZ);
}

float AABB::surfaceArea() const {
    float dx = maxX - minX;
    float dy = maxY - minY;
    float dz = maxZ - minZ;
    return 2.0f * (dx * dy + dy * dz + dz * dx);
}

AABB AABB::merge(const AABB& other) const {
    AABB result;
    result.minX = std::min(minX, other.minX);
    result.minY = std::min(minY, other.minY);
    result.minZ = std::min(minZ, other.minZ);
    result.maxX = std::max(maxX, other.maxX);
    result.maxY = std::max(maxY, other.maxY);
    result.maxZ = std::max(maxZ, other.maxZ);
    return result;
}

// SpatialHash Implementation
SpatialHash::SpatialHash(float cellSize, int gridWidth, int gridHeight)
    : m_cellSize(cellSize)
    , m_gridWidth(gridWidth)
    , m_gridHeight(gridHeight) {
    m_grid.resize(gridWidth * gridHeight);
}

SpatialHash::~SpatialHash() {
    clear();
}

void SpatialHash::insert(int id, const AABB& bounds) {
    ObjectEntry entry;
    entry.id = id;
    entry.bounds = bounds;
    getCellsForAABB(bounds, entry.cells);
    
    for (int cell : entry.cells) {
        m_grid[cell].push_back(id);
    }
    
    m_objects.push_back(entry);
}

void SpatialHash::remove(int id) {
    int index = findObjectIndex(id);
    if (index < 0) return;
    
    const ObjectEntry& entry = m_objects[index];
    for (int cell : entry.cells) {
        auto& cellObjects = m_grid[cell];
        cellObjects.erase(
            std::remove(cellObjects.begin(), cellObjects.end(), id),
            cellObjects.end()
        );
    }
    
    m_objects.erase(m_objects.begin() + index);
}

void SpatialHash::update(int id, const AABB& newBounds) {
    remove(id);
    insert(id, newBounds);
}

void SpatialHash::query(const AABB& bounds, std::function<void(int)> callback) const {
    std::vector<int> cells;
    getCellsForAABB(bounds, cells);
    
    std::vector<bool> visited(m_objects.size(), false);
    
    for (int cell : cells) {
        if (cell < 0 || cell >= static_cast<int>(m_grid.size())) continue;
        
        for (int id : m_grid[cell]) {
            int objIndex = findObjectIndex(id);
            if (objIndex >= 0 && !visited[objIndex]) {
                visited[objIndex] = true;
                if (m_objects[objIndex].bounds.intersects(bounds)) {
                    callback(id);
                }
            }
        }
    }
}

void SpatialHash::queryPairs(std::function<void(int, int)> callback) const {
    for (const auto& cell : m_grid) {
        for (size_t i = 0; i < cell.size(); ++i) {
            for (size_t j = i + 1; j < cell.size(); ++j) {
                callback(cell[i], cell[j]);
            }
        }
    }
}

void SpatialHash::clear() {
    for (auto& cell : m_grid) {
        cell.clear();
    }
    m_objects.clear();
}

int SpatialHash::hashPosition(float x, float y) const {
    int cellX = static_cast<int>(std::floor(x / m_cellSize));
    int cellY = static_cast<int>(std::floor(y / m_cellSize));
    
    cellX = std::max(0, std::min(cellX, m_gridWidth - 1));
    cellY = std::max(0, std::min(cellY, m_gridHeight - 1));
    
    return cellY * m_gridWidth + cellX;
}

void SpatialHash::getCellsForAABB(const AABB& bounds, std::vector<int>& cells) const {
    cells.clear();
    
    int minCellX = static_cast<int>(std::floor(bounds.minX / m_cellSize));
    int minCellY = static_cast<int>(std::floor(bounds.minY / m_cellSize));
    int maxCellX = static_cast<int>(std::floor(bounds.maxX / m_cellSize));
    int maxCellY = static_cast<int>(std::floor(bounds.maxY / m_cellSize));
    
    minCellX = std::max(0, std::min(minCellX, m_gridWidth - 1));
    minCellY = std::max(0, std::min(minCellY, m_gridHeight - 1));
    maxCellX = std::max(0, std::min(maxCellX, m_gridWidth - 1));
    maxCellY = std::max(0, std::min(maxCellY, m_gridHeight - 1));
    
    for (int y = minCellY; y <= maxCellY; ++y) {
        for (int x = minCellX; x <= maxCellX; ++x) {
            cells.push_back(y * m_gridWidth + x);
        }
    }
}

int SpatialHash::findObjectIndex(int id) const {
    for (size_t i = 0; i < m_objects.size(); ++i) {
        if (m_objects[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// DynamicAABBTree Implementation
DynamicAABBTree::DynamicAABBTree()
    : m_root(-1)
    , m_nodeCount(0)
    , m_nodeCapacity(16)
    , m_freeList(0) {
    m_nodes.resize(m_nodeCapacity);
    
    for (int i = 0; i < m_nodeCapacity - 1; ++i) {
        m_nodes[i].next = i + 1;
        m_nodes[i].height = -1;
    }
    m_nodes[m_nodeCapacity - 1].next = -1;
    m_nodes[m_nodeCapacity - 1].height = -1;
}

DynamicAABBTree::~DynamicAABBTree() {
}

int DynamicAABBTree::createProxy(const AABB& bounds, void* userData) {
    int proxyId = allocateNode();
    
    // Fatten the AABB
    const float extension = 0.1f;
    m_nodes[proxyId].aabb.minX = bounds.minX - extension;
    m_nodes[proxyId].aabb.minY = bounds.minY - extension;
    m_nodes[proxyId].aabb.minZ = bounds.minZ - extension;
    m_nodes[proxyId].aabb.maxX = bounds.maxX + extension;
    m_nodes[proxyId].aabb.maxY = bounds.maxY + extension;
    m_nodes[proxyId].aabb.maxZ = bounds.maxZ + extension;
    
    m_nodes[proxyId].userData = userData;
    m_nodes[proxyId].height = 0;
    m_nodes[proxyId].moved = true;
    
    insertLeaf(proxyId);
    
    return proxyId;
}

void DynamicAABBTree::destroyProxy(int proxyId) {
    removeLeaf(proxyId);
    freeNode(proxyId);
}

bool DynamicAABBTree::moveProxy(int proxyId, const AABB& bounds, const float* displacement) {
    if (m_nodes[proxyId].aabb.minX <= bounds.minX &&
        m_nodes[proxyId].aabb.minY <= bounds.minY &&
        m_nodes[proxyId].aabb.minZ <= bounds.minZ &&
        bounds.maxX <= m_nodes[proxyId].aabb.maxX &&
        bounds.maxY <= m_nodes[proxyId].aabb.maxY &&
        bounds.maxZ <= m_nodes[proxyId].aabb.maxZ) {
        return false;
    }
    
    removeLeaf(proxyId);
    
    const float extension = 0.1f;
    const float multiplier = 2.0f;
    
    AABB b = bounds;
    if (displacement) {
        float dx = multiplier * displacement[0];
        float dy = multiplier * displacement[1];
        float dz = multiplier * displacement[2];
        
        if (dx < 0.0f) b.minX += dx; else b.maxX += dx;
        if (dy < 0.0f) b.minY += dy; else b.maxY += dy;
        if (dz < 0.0f) b.minZ += dz; else b.maxZ += dz;
    }
    
    m_nodes[proxyId].aabb.minX = b.minX - extension;
    m_nodes[proxyId].aabb.minY = b.minY - extension;
    m_nodes[proxyId].aabb.minZ = b.minZ - extension;
    m_nodes[proxyId].aabb.maxX = b.maxX + extension;
    m_nodes[proxyId].aabb.maxY = b.maxY + extension;
    m_nodes[proxyId].aabb.maxZ = b.maxZ + extension;
    
    insertLeaf(proxyId);
    m_nodes[proxyId].moved = true;
    
    return true;
}

void* DynamicAABBTree::getUserData(int proxyId) const {
    return m_nodes[proxyId].userData;
}

AABB DynamicAABBTree::getAABB(int proxyId) const {
    return m_nodes[proxyId].aabb;
}

void DynamicAABBTree::query(const AABB& bounds, std::function<bool(int)> callback) const {
    std::vector<int> stack;
    stack.reserve(256);
    stack.push_back(m_root);
    
    while (!stack.empty()) {
        int nodeId = stack.back();
        stack.pop_back();
        
        if (nodeId == -1) continue;
        
        const Node& node = m_nodes[nodeId];
        if (node.aabb.intersects(bounds)) {
            if (node.isLeaf()) {
                bool proceed = callback(nodeId);
                if (!proceed) return;
            } else {
                stack.push_back(node.child1);
                stack.push_back(node.child2);
            }
        }
    }
}

int DynamicAABBTree::allocateNode() {
    if (m_freeList == -1) {
        int oldCapacity = m_nodeCapacity;
        m_nodeCapacity *= 2;
        m_nodes.resize(m_nodeCapacity);
        
        for (int i = oldCapacity; i < m_nodeCapacity - 1; ++i) {
            m_nodes[i].next = i + 1;
            m_nodes[i].height = -1;
        }
        m_nodes[m_nodeCapacity - 1].next = -1;
        m_nodes[m_nodeCapacity - 1].height = -1;
        m_freeList = oldCapacity;
    }
    
    int nodeId = m_freeList;
    m_freeList = m_nodes[nodeId].next;
    m_nodes[nodeId].parent = -1;
    m_nodes[nodeId].child1 = -1;
    m_nodes[nodeId].child2 = -1;
    m_nodes[nodeId].height = 0;
    m_nodes[nodeId].userData = nullptr;
    m_nodes[nodeId].moved = false;
    ++m_nodeCount;
    return nodeId;
}

void DynamicAABBTree::freeNode(int nodeId) {
    m_nodes[nodeId].next = m_freeList;
    m_nodes[nodeId].height = -1;
    m_freeList = nodeId;
    --m_nodeCount;
}

void DynamicAABBTree::insertLeaf(int leaf) {
    if (m_root == -1) {
        m_root = leaf;
        m_nodes[m_root].parent = -1;
        return;
    }
    
    AABB leafAABB = m_nodes[leaf].aabb;
    int index = m_root;
    
    while (!m_nodes[index].isLeaf()) {
        int child1 = m_nodes[index].child1;
        int child2 = m_nodes[index].child2;
        
        float area = m_nodes[index].aabb.surfaceArea();
        AABB combinedAABB = m_nodes[index].aabb.merge(leafAABB);
        float combinedArea = combinedAABB.surfaceArea();
        
        float cost = 2.0f * combinedArea;
        float inheritanceCost = 2.0f * (combinedArea - area);
        
        float cost1 = m_nodes[child1].aabb.merge(leafAABB).surfaceArea() + inheritanceCost;
        if (!m_nodes[child1].isLeaf()) {
            cost1 -= m_nodes[child1].aabb.surfaceArea();
        }
        
        float cost2 = m_nodes[child2].aabb.merge(leafAABB).surfaceArea() + inheritanceCost;
        if (!m_nodes[child2].isLeaf()) {
            cost2 -= m_nodes[child2].aabb.surfaceArea();
        }
        
        if (cost < cost1 && cost < cost2) break;
        
        index = (cost1 < cost2) ? child1 : child2;
    }
    
    int sibling = index;
    int oldParent = m_nodes[sibling].parent;
    int newParent = allocateNode();
    m_nodes[newParent].parent = oldParent;
    m_nodes[newParent].userData = nullptr;
    m_nodes[newParent].aabb = leafAABB.merge(m_nodes[sibling].aabb);
    m_nodes[newParent].height = m_nodes[sibling].height + 1;
    
    if (oldParent != -1) {
        if (m_nodes[oldParent].child1 == sibling) {
            m_nodes[oldParent].child1 = newParent;
        } else {
            m_nodes[oldParent].child2 = newParent;
        }
        
        m_nodes[newParent].child1 = sibling;
        m_nodes[newParent].child2 = leaf;
        m_nodes[sibling].parent = newParent;
        m_nodes[leaf].parent = newParent;
    } else {
        m_nodes[newParent].child1 = sibling;
        m_nodes[newParent].child2 = leaf;
        m_nodes[sibling].parent = newParent;
        m_nodes[leaf].parent = newParent;
        m_root = newParent;
    }
    
    index = m_nodes[leaf].parent;
    while (index != -1) {
        index = balance(index);
        
        int child1 = m_nodes[index].child1;
        int child2 = m_nodes[index].child2;
        
        m_nodes[index].height = 1 + std::max(m_nodes[child1].height, m_nodes[child2].height);
        m_nodes[index].aabb = m_nodes[child1].aabb.merge(m_nodes[child2].aabb);
        
        index = m_nodes[index].parent;
    }
}

void DynamicAABBTree::removeLeaf(int leaf) {
    if (leaf == m_root) {
        m_root = -1;
        return;
    }
    
    int parent = m_nodes[leaf].parent;
    int grandParent = m_nodes[parent].parent;
    int sibling = (m_nodes[parent].child1 == leaf) ? m_nodes[parent].child2 : m_nodes[parent].child1;
    
    if (grandParent != -1) {
        if (m_nodes[grandParent].child1 == parent) {
            m_nodes[grandParent].child1 = sibling;
        } else {
            m_nodes[grandParent].child2 = sibling;
        }
        m_nodes[sibling].parent = grandParent;
        freeNode(parent);
        
        int index = grandParent;
        while (index != -1) {
            index = balance(index);
            
            int child1 = m_nodes[index].child1;
            int child2 = m_nodes[index].child2;
            
            m_nodes[index].aabb = m_nodes[child1].aabb.merge(m_nodes[child2].aabb);
            m_nodes[index].height = 1 + std::max(m_nodes[child1].height, m_nodes[child2].height);
            
            index = m_nodes[index].parent;
        }
    } else {
        m_root = sibling;
        m_nodes[sibling].parent = -1;
        freeNode(parent);
    }
}

int DynamicAABBTree::balance(int iA) {
    Node& A = m_nodes[iA];
    if (A.isLeaf() || A.height < 2) {
        return iA;
    }
    
    int iB = A.child1;
    int iC = A.child2;
    
    Node& B = m_nodes[iB];
    Node& C = m_nodes[iC];
    
    int balance = C.height - B.height;
    
    if (balance > 1) {
        int iF = C.child1;
        int iG = C.child2;
        Node& F = m_nodes[iF];
        Node& G = m_nodes[iG];
        
        C.child1 = iA;
        C.parent = A.parent;
        A.parent = iC;
        
        if (C.parent != -1) {
            if (m_nodes[C.parent].child1 == iA) {
                m_nodes[C.parent].child1 = iC;
            } else {
                m_nodes[C.parent].child2 = iC;
            }
        } else {
            m_root = iC;
        }
        
        if (F.height > G.height) {
            C.child2 = iF;
            A.child2 = iG;
            G.parent = iA;
            A.aabb = B.aabb.merge(G.aabb);
            C.aabb = A.aabb.merge(F.aabb);
            
            A.height = 1 + std::max(B.height, G.height);
            C.height = 1 + std::max(A.height, F.height);
        } else {
            C.child2 = iG;
            A.child2 = iF;
            F.parent = iA;
            A.aabb = B.aabb.merge(F.aabb);
            C.aabb = A.aabb.merge(G.aabb);
            
            A.height = 1 + std::max(B.height, F.height);
            C.height = 1 + std::max(A.height, G.height);
        }
        
        return iC;
    }
    
    if (balance < -1) {
        int iD = B.child1;
        int iE = B.child2;
        Node& D = m_nodes[iD];
        Node& E = m_nodes[iE];
        
        B.child1 = iA;
        B.parent = A.parent;
        A.parent = iB;
        
        if (B.parent != -1) {
            if (m_nodes[B.parent].child1 == iA) {
                m_nodes[B.parent].child1 = iB;
            } else {
                m_nodes[B.parent].child2 = iB;
            }
        } else {
            m_root = iB;
        }
        
        if (D.height > E.height) {
            B.child2 = iD;
            A.child1 = iE;
            E.parent = iA;
            A.aabb = C.aabb.merge(E.aabb);
            B.aabb = A.aabb.merge(D.aabb);
            
            A.height = 1 + std::max(C.height, E.height);
            B.height = 1 + std::max(A.height, D.height);
        } else {
            B.child2 = iE;
            A.child1 = iD;
            D.parent = iA;
            A.aabb = C.aabb.merge(D.aabb);
            B.aabb = A.aabb.merge(E.aabb);
            
            A.height = 1 + std::max(C.height, D.height);
            B.height = 1 + std::max(A.height, E.height);
        }
        
        return iB;
    }
    
    return iA;
}

} // namespace Engine
