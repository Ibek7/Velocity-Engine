#ifndef LOD_GENERATOR_H
#define LOD_GENERATOR_H

#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace Utils {

// Forward declarations
struct Vertex;
struct Triangle;
struct Mesh;
struct LODLevel;
class LODGenerator;

// Basic vertex structure
struct Vertex {
    float position[3];      // Position
    float normal[3];        // Normal
    float texCoord[2];      // UV coordinates
    float color[4];         // Vertex color (optional)
    
    bool operator==(const Vertex& other) const;
    bool operator!=(const Vertex& other) const { return !(*this == other); }
};

// Triangle structure
struct Triangle {
    unsigned int indices[3];  // Vertex indices
    float normal[3];          // Face normal
    float area;               // Triangle area
    
    Triangle() : area(0) {
        indices[0] = indices[1] = indices[2] = 0;
        normal[0] = normal[1] = normal[2] = 0;
    }
};

// Mesh data structure
struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Triangle> triangles;
    
    void calculateNormals();
    void calculateBounds(float min[3], float max[3]) const;
    float calculateSurfaceArea() const;
    int getTriangleCount() const { return indices.size() / 3; }
};

// LOD level descriptor
struct LODLevel {
    Mesh mesh;
    float distance;          // Distance threshold for this LOD
    float reductionRatio;    // Ratio of triangles vs original (0-1)
    int triangleCount;       // Number of triangles in this LOD
    
    LODLevel() : distance(0), reductionRatio(1.0f), triangleCount(0) {}
};

// Decimation algorithm types
enum class DecimationAlgorithm {
    EDGE_COLLAPSE,           // Iterative edge collapse (Hoppe)
    QUADRIC_ERROR,           // Quadric error metrics (Garland-Heckbert)
    VERTEX_CLUSTERING,       // Vertex clustering/grid-based
    PROGRESSIVE_MESH         // Progressive mesh (Hoppe '96)
};

// LOD generation settings
struct LODSettings {
    // General settings
    int numLevels = 4;                           // Number of LOD levels to generate
    DecimationAlgorithm algorithm = DecimationAlgorithm::QUADRIC_ERROR;
    
    // Distance-based LOD
    float baseLODDistance = 10.0f;               // Distance for LOD 0
    float distanceMultiplier = 2.5f;             // Distance multiplier between levels
    
    // Reduction settings
    float minReduction = 0.1f;                   // Minimum triangles (10% of original)
    float reductionRate = 0.5f;                  // Reduction per level (50%)
    
    // Quality settings
    bool preserveTopology = true;                // Maintain mesh topology
    bool preserveBoundaries = true;              // Don't collapse boundary edges
    bool preserveUVSeams = true;                 // Preserve UV seam edges
    bool preserveNormalSeams = true;             // Preserve normal discontinuities
    float maxErrorThreshold = 0.01f;             // Maximum allowed geometric error
    
    // Feature preservation
    bool preserveSharpFeatures = true;           // Preserve sharp edges/corners
    float sharpFeatureAngle = 60.0f;             // Angle threshold (degrees)
    bool lockBoundaryVertices = true;            // Lock boundary vertices
    
    // Quadric error settings
    bool useQuadricWeighting = true;             // Weight errors by triangle area
    float boundaryWeight = 1000.0f;              // Weight for boundary edges
    float seamWeight = 100.0f;                   // Weight for seam edges
    
    // Vertex clustering settings (for VERTEX_CLUSTERING algorithm)
    int gridResolution = 64;                     // Grid resolution
    bool adaptiveGrid = true;                    // Use adaptive grid sizing
    
    // Progressive mesh settings
    bool generateCollapseSequence = false;       // Store collapse order
    
    // Post-processing
    bool optimizeVertexCache = true;             // Optimize for GPU cache
    bool removeDegenerateFaces = true;           // Remove zero-area triangles
    bool weldVertices = true;                    // Merge duplicate vertices
    float weldThreshold = 0.0001f;               // Distance threshold for welding
};

// Edge collapse candidate
struct EdgeCollapse {
    unsigned int vertex0;     // First vertex index
    unsigned int vertex1;     // Second vertex index
    Vertex newVertex;         // Resulting merged vertex
    float error;              // Quadric error for this collapse
    
    bool operator<(const EdgeCollapse& other) const {
        return error < other.error;
    }
};

// Quadric error matrix (for quadric error metric decimation)
struct QuadricMatrix {
    double matrix[10];  // Symmetric 4x4 matrix stored as upper triangle
    
    QuadricMatrix();
    void addPlane(const float plane[4]);
    void add(const QuadricMatrix& other);
    double evaluateError(const float pos[3]) const;
    bool solveOptimalPosition(float result[3]) const;
};

// LOD Generator class
class LODGenerator {
public:
    LODGenerator();
    ~LODGenerator();
    
    // Main LOD generation
    std::vector<LODLevel> generateLODs(const Mesh& originalMesh, 
                                       const LODSettings& settings = LODSettings());
    
    // Single level decimation
    Mesh decimate(const Mesh& mesh, float targetRatio, 
                  const LODSettings& settings = LODSettings());
    
    // Specific algorithms
    Mesh decimateEdgeCollapse(const Mesh& mesh, int targetTriangles,
                             const LODSettings& settings);
    Mesh decimateQuadricError(const Mesh& mesh, int targetTriangles,
                             const LODSettings& settings);
    Mesh decimateVertexClustering(const Mesh& mesh, int targetTriangles,
                                  const LODSettings& settings);
    
    // Progressive mesh
    struct CollapseRecord {
        unsigned int vertex0, vertex1;
        Vertex newVertex;
    };
    std::vector<CollapseRecord> generateProgressiveMesh(const Mesh& mesh,
                                                       const LODSettings& settings);
    
    // Utility functions
    static float calculateGeometricError(const Mesh& original, const Mesh& simplified);
    static void calculateDistanceThresholds(std::vector<LODLevel>& levels,
                                           const LODSettings& settings);
    static void optimizeVertexCache(Mesh& mesh);
    
    // Vertex operations
    static void weldVertices(Mesh& mesh, float threshold = 0.0001f);
    static void removeDegenerateFaces(Mesh& mesh);
    
    // Feature detection
    static std::vector<bool> detectBoundaryVertices(const Mesh& mesh);
    static std::vector<bool> detectSharpEdges(const Mesh& mesh, float angleThreshold);
    static std::vector<bool> detectUVSeams(const Mesh& mesh);
    
    // Callbacks for progress
    using ProgressCallback = std::function<void(int level, float progress)>;
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    
private:
    ProgressCallback m_progressCallback;
    
    // Edge collapse helpers
    void buildEdgeCollapseList(const Mesh& mesh, 
                              std::vector<EdgeCollapse>& collapses,
                              const LODSettings& settings);
    bool isEdgeCollapsible(const Mesh& mesh, unsigned int v0, unsigned int v1,
                          const LODSettings& settings);
    void performEdgeCollapse(Mesh& mesh, const EdgeCollapse& collapse);
    
    // Quadric error helpers
    void buildQuadricMatrices(const Mesh& mesh, 
                            std::vector<QuadricMatrix>& quadrics,
                            const LODSettings& settings);
    float computeCollapseError(const QuadricMatrix& q0, const QuadricMatrix& q1,
                              const Vertex& newVertex);
    
    // Vertex clustering helpers
    struct VoxelGrid {
        int resolution[3];
        float cellSize[3];
        float origin[3];
        std::vector<std::vector<unsigned int>> cells;
        
        int getVoxelIndex(const float pos[3]) const;
    };
    void buildVoxelGrid(const Mesh& mesh, VoxelGrid& grid, 
                       int resolution, const LODSettings& settings);
    Vertex clusterVertices(const std::vector<unsigned int>& vertices, 
                          const Mesh& mesh);
    
    // Topology helpers
    struct HalfEdge {
        unsigned int vertex;
        unsigned int oppositeHalfEdge;
        unsigned int nextHalfEdge;
        unsigned int face;
    };
    void buildHalfEdgeStructure(const Mesh& mesh, 
                               std::vector<HalfEdge>& halfEdges);
    
    // Utility helpers
    float calculateTriangleArea(const Vertex& v0, const Vertex& v1, 
                               const Vertex& v2);
    void calculateFaceNormal(const Vertex& v0, const Vertex& v1, 
                           const Vertex& v2, float normal[3]);
    bool areVerticesSimilar(const Vertex& v0, const Vertex& v1, 
                          float threshold);
    
    // Progress reporting
    void reportProgress(int level, float progress);
};

// Helper class for automatic LOD management at runtime
class LODManager {
public:
    struct LODObject {
        std::vector<LODLevel> levels;
        float position[3];           // Object position for distance calculation
        int currentLOD;              // Currently active LOD index
        void* userData;              // User data
        
        LODObject() : currentLOD(0), userData(nullptr) {
            position[0] = position[1] = position[2] = 0;
        }
    };
    
    LODManager();
    
    // Object management
    int addObject(const std::vector<LODLevel>& levels, 
                 const float position[3] = nullptr);
    void removeObject(int objectId);
    void updateObjectPosition(int objectId, const float position[3]);
    
    // Camera/viewer position
    void setViewerPosition(const float position[3]);
    void getViewerPosition(float position[3]) const;
    
    // LOD selection
    void updateLODs();  // Update all objects based on viewer position
    int selectLOD(const LODObject& object) const;
    
    // Settings
    void setLODBias(float bias) { m_lodBias = bias; }  // Bias LOD selection
    float getLODBias() const { return m_lodBias; }
    
    // Query
    const LODObject* getObject(int objectId) const;
    const Mesh* getCurrentMesh(int objectId) const;
    int getCurrentLODLevel(int objectId) const;
    
private:
    std::vector<LODObject> m_objects;
    float m_viewerPosition[3];
    float m_lodBias;  // Positive = higher detail, negative = lower detail
    
    float calculateDistance(const float pos1[3], const float pos2[3]) const;
};

} // namespace Utils
} // namespace JJM

#endif // LOD_GENERATOR_H
