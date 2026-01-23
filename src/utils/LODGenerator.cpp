#include "../../include/utils/LODGenerator.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <cstring>

namespace JJM {
namespace Utils {

// ============================================================================
// Vertex Implementation
// ============================================================================

bool Vertex::operator==(const Vertex& other) const {
    for (int i = 0; i < 3; i++) {
        if (std::abs(position[i] - other.position[i]) > 0.0001f) return false;
    }
    return true;
}

// ============================================================================
// Mesh Implementation
// ============================================================================

void Mesh::calculateNormals() {
    // Reset normals
    for (auto& v : vertices) {
        v.normal[0] = v.normal[1] = v.normal[2] = 0;
    }
    
    // Accumulate face normals
    for (size_t i = 0; i < indices.size(); i += 3) {
        const Vertex& v0 = vertices[indices[i]];
        const Vertex& v1 = vertices[indices[i + 1]];
        const Vertex& v2 = vertices[indices[i + 2]];
        
        float edge1[3] = {
            v1.position[0] - v0.position[0],
            v1.position[1] - v0.position[1],
            v1.position[2] - v0.position[2]
        };
        float edge2[3] = {
            v2.position[0] - v0.position[0],
            v2.position[1] - v0.position[1],
            v2.position[2] - v0.position[2]
        };
        
        float normal[3] = {
            edge1[1] * edge2[2] - edge1[2] * edge2[1],
            edge1[2] * edge2[0] - edge1[0] * edge2[2],
            edge1[0] * edge2[1] - edge1[1] * edge2[0]
        };
        
        for (int j = 0; j < 3; j++) {
            vertices[indices[i + j]].normal[0] += normal[0];
            vertices[indices[i + j]].normal[1] += normal[1];
            vertices[indices[i + j]].normal[2] += normal[2];
        }
    }
    
    // Normalize
    for (auto& v : vertices) {
        float len = std::sqrt(v.normal[0] * v.normal[0] + 
                            v.normal[1] * v.normal[1] + 
                            v.normal[2] * v.normal[2]);
        if (len > 0.0001f) {
            v.normal[0] /= len;
            v.normal[1] /= len;
            v.normal[2] /= len;
        }
    }
}

void Mesh::calculateBounds(float min[3], float max[3]) const {
    if (vertices.empty()) {
        min[0] = min[1] = min[2] = 0;
        max[0] = max[1] = max[2] = 0;
        return;
    }
    
    min[0] = max[0] = vertices[0].position[0];
    min[1] = max[1] = vertices[0].position[1];
    min[2] = max[2] = vertices[0].position[2];
    
    for (const auto& v : vertices) {
        for (int i = 0; i < 3; i++) {
            min[i] = std::min(min[i], v.position[i]);
            max[i] = std::max(max[i], v.position[i]);
        }
    }
}

float Mesh::calculateSurfaceArea() const {
    float totalArea = 0;
    for (size_t i = 0; i < indices.size(); i += 3) {
        const Vertex& v0 = vertices[indices[i]];
        const Vertex& v1 = vertices[indices[i + 1]];
        const Vertex& v2 = vertices[indices[i + 2]];
        
        float edge1[3] = {
            v1.position[0] - v0.position[0],
            v1.position[1] - v0.position[1],
            v1.position[2] - v0.position[2]
        };
        float edge2[3] = {
            v2.position[0] - v0.position[0],
            v2.position[1] - v0.position[1],
            v2.position[2] - v0.position[2]
        };
        
        float cross[3] = {
            edge1[1] * edge2[2] - edge1[2] * edge2[1],
            edge1[2] * edge2[0] - edge1[0] * edge2[2],
            edge1[0] * edge2[1] - edge1[1] * edge2[0]
        };
        
        float area = 0.5f * std::sqrt(cross[0] * cross[0] + 
                                     cross[1] * cross[1] + 
                                     cross[2] * cross[2]);
        totalArea += area;
    }
    return totalArea;
}

// ============================================================================
// QuadricMatrix Implementation
// ============================================================================

QuadricMatrix::QuadricMatrix() {
    std::memset(matrix, 0, sizeof(matrix));
}

void QuadricMatrix::addPlane(const float plane[4]) {
    // plane = [a, b, c, d] where ax + by + cz + d = 0
    // Q = [a²   ab   ac   ad]
    //     [ab   b²   bc   bd]
    //     [ac   bc   c²   cd]
    //     [ad   bd   cd   d²]
    // Stored as upper triangle: [a², ab, b², ac, bc, c², ad, bd, cd, d²]
    
    matrix[0] += plane[0] * plane[0];  // a²
    matrix[1] += plane[0] * plane[1];  // ab
    matrix[2] += plane[1] * plane[1];  // b²
    matrix[3] += plane[0] * plane[2];  // ac
    matrix[4] += plane[1] * plane[2];  // bc
    matrix[5] += plane[2] * plane[2];  // c²
    matrix[6] += plane[0] * plane[3];  // ad
    matrix[7] += plane[1] * plane[3];  // bd
    matrix[8] += plane[2] * plane[3];  // cd
    matrix[9] += plane[3] * plane[3];  // d²
}

void QuadricMatrix::add(const QuadricMatrix& other) {
    for (int i = 0; i < 10; i++) {
        matrix[i] += other.matrix[i];
    }
}

double QuadricMatrix::evaluateError(const float pos[3]) const {
    // v^T * Q * v where v = [x, y, z, 1]
    double x = pos[0], y = pos[1], z = pos[2];
    return matrix[0] * x * x + 2 * matrix[1] * x * y + matrix[2] * y * y +
           2 * matrix[3] * x * z + 2 * matrix[4] * y * z + matrix[5] * z * z +
           2 * matrix[6] * x + 2 * matrix[7] * y + 2 * matrix[8] * z + matrix[9];
}

bool QuadricMatrix::solveOptimalPosition(float result[3]) const {
    // Solve Q * v = 0 for optimal position
    // This is a simplified version - full implementation would use
    // matrix inversion or SVD for better numerical stability
    
    // For now, we'll use a simple approximation
    // In production, use Eigen or similar library
    result[0] = result[1] = result[2] = 0;
    return false;  // Indicate we're using approximation
}

// ============================================================================
// LODGenerator Implementation
// ============================================================================

LODGenerator::LODGenerator() {
}

LODGenerator::~LODGenerator() {
}

std::vector<LODLevel> LODGenerator::generateLODs(const Mesh& originalMesh,
                                                 const LODSettings& settings) {
    std::vector<LODLevel> levels;
    
    // Add original as LOD 0
    LODLevel lod0;
    lod0.mesh = originalMesh;
    lod0.distance = settings.baseLODDistance;
    lod0.reductionRatio = 1.0f;
    lod0.triangleCount = originalMesh.getTriangleCount();
    levels.push_back(lod0);
    
    reportProgress(0, 1.0f);
    
    // Generate subsequent LOD levels
    for (int i = 1; i < settings.numLevels; i++) {
        float ratio = std::pow(settings.reductionRate, i);
        ratio = std::max(ratio, settings.minReduction);
        
        LODLevel lod;
        lod.reductionRatio = ratio;
        lod.mesh = decimate(originalMesh, ratio, settings);
        lod.triangleCount = lod.mesh.getTriangleCount();
        
        levels.push_back(lod);
        reportProgress(i, 1.0f);
    }
    
    // Calculate distance thresholds
    calculateDistanceThresholds(levels, settings);
    
    return levels;
}

Mesh LODGenerator::decimate(const Mesh& mesh, float targetRatio,
                           const LODSettings& settings) {
    int targetTriangles = static_cast<int>(mesh.getTriangleCount() * targetRatio);
    targetTriangles = std::max(targetTriangles, 1);
    
    switch (settings.algorithm) {
        case DecimationAlgorithm::EDGE_COLLAPSE:
            return decimateEdgeCollapse(mesh, targetTriangles, settings);
        case DecimationAlgorithm::QUADRIC_ERROR:
            return decimateQuadricError(mesh, targetTriangles, settings);
        case DecimationAlgorithm::VERTEX_CLUSTERING:
            return decimateVertexClustering(mesh, targetTriangles, settings);
        default:
            return decimateQuadricError(mesh, targetTriangles, settings);
    }
}

Mesh LODGenerator::decimateQuadricError(const Mesh& mesh, int targetTriangles,
                                       const LODSettings& settings) {
    Mesh result = mesh;
    
    // Build quadric matrices for each vertex
    std::vector<QuadricMatrix> quadrics(mesh.vertices.size());
    buildQuadricMatrices(mesh, quadrics, settings);
    
    // Build edge collapse candidates
    std::vector<EdgeCollapse> collapses;
    buildEdgeCollapseList(mesh, collapses, settings);
    
    // Sort by error
    std::sort(collapses.begin(), collapses.end());
    
    // Perform collapses until target is reached
    int currentTriangles = mesh.getTriangleCount();
    for (const auto& collapse : collapses) {
        if (currentTriangles <= targetTriangles) break;
        
        if (isEdgeCollapsible(result, collapse.vertex0, collapse.vertex1, settings)) {
            performEdgeCollapse(result, collapse);
            currentTriangles--;
        }
    }
    
    // Post-processing
    if (settings.removeDegenerateFaces) {
        removeDegenerateFaces(result);
    }
    if (settings.weldVertices) {
        weldVertices(result, settings.weldThreshold);
    }
    if (settings.optimizeVertexCache) {
        optimizeVertexCache(result);
    }
    
    result.calculateNormals();
    return result;
}

Mesh LODGenerator::decimateEdgeCollapse(const Mesh& mesh, int targetTriangles,
                                       const LODSettings& settings) {
    // Simplified edge collapse - similar to quadric but without error metrics
    return decimateQuadricError(mesh, targetTriangles, settings);
}

Mesh LODGenerator::decimateVertexClustering(const Mesh& mesh, int targetTriangles,
                                           const LODSettings& settings) {
    Mesh result;
    
    // Build voxel grid
    VoxelGrid grid;
    int resolution = settings.gridResolution;
    buildVoxelGrid(mesh, grid, resolution, settings);
    
    // Cluster vertices in each voxel
    std::unordered_map<int, Vertex> clusterVertices;
    std::unordered_map<int, unsigned int> voxelToVertex;
    
    for (size_t i = 0; i < grid.cells.size(); i++) {
        if (!grid.cells[i].empty()) {
            Vertex clustered = clusterVertices(grid.cells[i], mesh);
            unsigned int newIndex = result.vertices.size();
            result.vertices.push_back(clustered);
            voxelToVertex[i] = newIndex;
        }
    }
    
    // Rebuild indices
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        int voxel0 = grid.getVoxelIndex(mesh.vertices[mesh.indices[i]].position);
        int voxel1 = grid.getVoxelIndex(mesh.vertices[mesh.indices[i+1]].position);
        int voxel2 = grid.getVoxelIndex(mesh.vertices[mesh.indices[i+2]].position);
        
        // Skip degenerate triangles
        if (voxel0 != voxel1 && voxel1 != voxel2 && voxel0 != voxel2) {
            result.indices.push_back(voxelToVertex[voxel0]);
            result.indices.push_back(voxelToVertex[voxel1]);
            result.indices.push_back(voxelToVertex[voxel2]);
        }
    }
    
    result.calculateNormals();
    return result;
}

void LODGenerator::buildQuadricMatrices(const Mesh& mesh,
                                       std::vector<QuadricMatrix>& quadrics,
                                       const LODSettings& settings) {
    // Initialize quadrics from face planes
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Vertex& v0 = mesh.vertices[mesh.indices[i]];
        const Vertex& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Vertex& v2 = mesh.vertices[mesh.indices[i + 2]];
        
        float normal[3];
        calculateFaceNormal(v0, v1, v2, normal);
        
        // Plane equation: ax + by + cz + d = 0
        float d = -(normal[0] * v0.position[0] + 
                   normal[1] * v0.position[1] + 
                   normal[2] * v0.position[2]);
        float plane[4] = { normal[0], normal[1], normal[2], d };
        
        // Add to vertex quadrics
        for (int j = 0; j < 3; j++) {
            quadrics[mesh.indices[i + j]].addPlane(plane);
        }
    }
}

void LODGenerator::buildEdgeCollapseList(const Mesh& mesh,
                                        std::vector<EdgeCollapse>& collapses,
                                        const LODSettings& settings) {
    // Build list of unique edges
    std::unordered_map<uint64_t, std::pair<unsigned int, unsigned int>> edges;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        for (int j = 0; j < 3; j++) {
            unsigned int v0 = mesh.indices[i + j];
            unsigned int v1 = mesh.indices[i + (j + 1) % 3];
            
            uint64_t key = (static_cast<uint64_t>(std::min(v0, v1)) << 32) | 
                          std::max(v0, v1);
            edges[key] = {v0, v1};
        }
    }
    
    // Create collapse candidates for each edge
    for (const auto& edge : edges) {
        EdgeCollapse collapse;
        collapse.vertex0 = edge.second.first;
        collapse.vertex1 = edge.second.second;
        
        // Simple midpoint for new vertex
        const Vertex& v0 = mesh.vertices[collapse.vertex0];
        const Vertex& v1 = mesh.vertices[collapse.vertex1];
        
        for (int i = 0; i < 3; i++) {
            collapse.newVertex.position[i] = (v0.position[i] + v1.position[i]) * 0.5f;
            collapse.newVertex.normal[i] = (v0.normal[i] + v1.normal[i]) * 0.5f;
        }
        for (int i = 0; i < 2; i++) {
            collapse.newVertex.texCoord[i] = (v0.texCoord[i] + v1.texCoord[i]) * 0.5f;
        }
        
        // Calculate error (simplified - would use quadric error in full impl)
        float dx = v1.position[0] - v0.position[0];
        float dy = v1.position[1] - v0.position[1];
        float dz = v1.position[2] - v0.position[2];
        collapse.error = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        collapses.push_back(collapse);
    }
}

bool LODGenerator::isEdgeCollapsible(const Mesh& mesh, unsigned int v0, unsigned int v1,
                                    const LODSettings& settings) {
    // Simplified check - full implementation would check topology,
    // boundaries, seams, etc.
    return true;
}

void LODGenerator::performEdgeCollapse(Mesh& mesh, const EdgeCollapse& collapse) {
    // Replace v0 with newVertex
    if (collapse.vertex0 < mesh.vertices.size()) {
        mesh.vertices[collapse.vertex0] = collapse.newVertex;
    }
    
    // Update indices that reference v1 to reference v0
    for (auto& idx : mesh.indices) {
        if (idx == collapse.vertex1) {
            idx = collapse.vertex0;
        }
    }
}

void LODGenerator::buildVoxelGrid(const Mesh& mesh, VoxelGrid& grid,
                                 int resolution, const LODSettings& settings) {
    float min[3], max[3];
    mesh.calculateBounds(min, max);
    
    for (int i = 0; i < 3; i++) {
        grid.origin[i] = min[i];
        float extent = max[i] - min[i];
        grid.resolution[i] = resolution;
        grid.cellSize[i] = extent / resolution;
    }
    
    grid.cells.resize(resolution * resolution * resolution);
    
    // Assign vertices to voxels
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        int voxelIndex = grid.getVoxelIndex(mesh.vertices[i].position);
        if (voxelIndex >= 0 && voxelIndex < static_cast<int>(grid.cells.size())) {
            grid.cells[voxelIndex].push_back(i);
        }
    }
}

int LODGenerator::VoxelGrid::getVoxelIndex(const float pos[3]) const {
    int x = static_cast<int>((pos[0] - origin[0]) / cellSize[0]);
    int y = static_cast<int>((pos[1] - origin[1]) / cellSize[1]);
    int z = static_cast<int>((pos[2] - origin[2]) / cellSize[2]);
    
    x = std::max(0, std::min(x, resolution[0] - 1));
    y = std::max(0, std::min(y, resolution[1] - 1));
    z = std::max(0, std::min(z, resolution[2] - 1));
    
    return x + y * resolution[0] + z * resolution[0] * resolution[1];
}

Vertex LODGenerator::clusterVertices(const std::vector<unsigned int>& vertices,
                                    const Mesh& mesh) {
    Vertex result;
    std::memset(&result, 0, sizeof(Vertex));
    
    for (unsigned int idx : vertices) {
        const Vertex& v = mesh.vertices[idx];
        for (int i = 0; i < 3; i++) {
            result.position[i] += v.position[i];
            result.normal[i] += v.normal[i];
        }
        for (int i = 0; i < 2; i++) {
            result.texCoord[i] += v.texCoord[i];
        }
    }
    
    float count = static_cast<float>(vertices.size());
    for (int i = 0; i < 3; i++) {
        result.position[i] /= count;
        result.normal[i] /= count;
    }
    for (int i = 0; i < 2; i++) {
        result.texCoord[i] /= count;
    }
    
    return result;
}

float LODGenerator::calculateTriangleArea(const Vertex& v0, const Vertex& v1,
                                         const Vertex& v2) {
    float edge1[3] = {
        v1.position[0] - v0.position[0],
        v1.position[1] - v0.position[1],
        v1.position[2] - v0.position[2]
    };
    float edge2[3] = {
        v2.position[0] - v0.position[0],
        v2.position[1] - v0.position[1],
        v2.position[2] - v0.position[2]
    };
    
    float cross[3] = {
        edge1[1] * edge2[2] - edge1[2] * edge2[1],
        edge1[2] * edge2[0] - edge1[0] * edge2[2],
        edge1[0] * edge2[1] - edge1[1] * edge2[0]
    };
    
    return 0.5f * std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
}

void LODGenerator::calculateFaceNormal(const Vertex& v0, const Vertex& v1,
                                      const Vertex& v2, float normal[3]) {
    float edge1[3] = {
        v1.position[0] - v0.position[0],
        v1.position[1] - v0.position[1],
        v1.position[2] - v0.position[2]
    };
    float edge2[3] = {
        v2.position[0] - v0.position[0],
        v2.position[1] - v0.position[1],
        v2.position[2] - v0.position[2]
    };
    
    normal[0] = edge1[1] * edge2[2] - edge1[2] * edge2[1];
    normal[1] = edge1[2] * edge2[0] - edge1[0] * edge2[2];
    normal[2] = edge1[0] * edge2[1] - edge1[1] * edge2[0];
    
    float len = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (len > 0.0001f) {
        normal[0] /= len;
        normal[1] /= len;
        normal[2] /= len;
    }
}

void LODGenerator::weldVertices(Mesh& mesh, float threshold) {
    // Simplified implementation
    mesh.calculateNormals();
}

void LODGenerator::removeDegenerateFaces(Mesh& mesh) {
    std::vector<unsigned int> newIndices;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        unsigned int i0 = mesh.indices[i];
        unsigned int i1 = mesh.indices[i + 1];
        unsigned int i2 = mesh.indices[i + 2];
        
        if (i0 != i1 && i1 != i2 && i0 != i2) {
            newIndices.push_back(i0);
            newIndices.push_back(i1);
            newIndices.push_back(i2);
        }
    }
    
    mesh.indices = newIndices;
}

void LODGenerator::optimizeVertexCache(Mesh& mesh) {
    // Tom Forsyth's vertex cache optimization would go here
    // Simplified for now
}

void LODGenerator::calculateDistanceThresholds(std::vector<LODLevel>& levels,
                                              const LODSettings& settings) {
    for (size_t i = 0; i < levels.size(); i++) {
        levels[i].distance = settings.baseLODDistance * 
                            std::pow(settings.distanceMultiplier, static_cast<float>(i));
    }
}

void LODGenerator::reportProgress(int level, float progress) {
    if (m_progressCallback) {
        m_progressCallback(level, progress);
    }
}

// ============================================================================
// LODManager Implementation
// ============================================================================

LODManager::LODManager()
    : m_lodBias(0.0f) {
    m_viewerPosition[0] = m_viewerPosition[1] = m_viewerPosition[2] = 0;
}

int LODManager::addObject(const std::vector<LODLevel>& levels,
                         const float position[3]) {
    LODObject obj;
    obj.levels = levels;
    if (position) {
        obj.position[0] = position[0];
        obj.position[1] = position[1];
        obj.position[2] = position[2];
    }
    
    int id = m_objects.size();
    m_objects.push_back(obj);
    return id;
}

void LODManager::removeObject(int objectId) {
    if (objectId >= 0 && objectId < static_cast<int>(m_objects.size())) {
        m_objects.erase(m_objects.begin() + objectId);
    }
}

void LODManager::updateObjectPosition(int objectId, const float position[3]) {
    if (objectId >= 0 && objectId < static_cast<int>(m_objects.size())) {
        m_objects[objectId].position[0] = position[0];
        m_objects[objectId].position[1] = position[1];
        m_objects[objectId].position[2] = position[2];
    }
}

void LODManager::setViewerPosition(const float position[3]) {
    m_viewerPosition[0] = position[0];
    m_viewerPosition[1] = position[1];
    m_viewerPosition[2] = position[2];
}

void LODManager::getViewerPosition(float position[3]) const {
    position[0] = m_viewerPosition[0];
    position[1] = m_viewerPosition[1];
    position[2] = m_viewerPosition[2];
}

void LODManager::updateLODs() {
    for (auto& obj : m_objects) {
        obj.currentLOD = selectLOD(obj);
    }
}

int LODManager::selectLOD(const LODObject& object) const {
    float dist = calculateDistance(m_viewerPosition, object.position);
    dist *= (1.0f - m_lodBias);  // Apply bias
    
    for (int i = object.levels.size() - 1; i >= 0; i--) {
        if (dist >= object.levels[i].distance) {
            return i;
        }
    }
    
    return 0;  // Highest detail
}

const LODManager::LODObject* LODManager::getObject(int objectId) const {
    if (objectId >= 0 && objectId < static_cast<int>(m_objects.size())) {
        return &m_objects[objectId];
    }
    return nullptr;
}

const Mesh* LODManager::getCurrentMesh(int objectId) const {
    const LODObject* obj = getObject(objectId);
    if (obj && obj->currentLOD >= 0 && 
        obj->currentLOD < static_cast<int>(obj->levels.size())) {
        return &obj->levels[obj->currentLOD].mesh;
    }
    return nullptr;
}

int LODManager::getCurrentLODLevel(int objectId) const {
    const LODObject* obj = getObject(objectId);
    return obj ? obj->currentLOD : -1;
}

float LODManager::calculateDistance(const float pos1[3], const float pos2[3]) const {
    float dx = pos2[0] - pos1[0];
    float dy = pos2[1] - pos1[1];
    float dz = pos2[2] - pos1[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace Utils
} // namespace JJM
