#ifndef TERRAIN_SYSTEM_H
#define TERRAIN_SYSTEM_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace JJM {
namespace Graphics {

/**
 * @brief Terrain LOD settings
 */
struct TerrainLODConfig {
    uint32_t lodLevels;          // Number of LOD levels
    float lodDistance;           // Distance multiplier for LOD transitions
    bool useGeomorphing;         // Smooth transitions between LODs
    float morphingRange;         // Distance range for morphing
    
    TerrainLODConfig()
        : lodLevels(4)
        , lodDistance(100.0f)
        , useGeomorphing(true)
        , morphingRange(20.0f) {}
};

/**
 * @brief Terrain chunk representing a section of terrain
 */
class TerrainChunk {
public:
    TerrainChunk(int32_t x, int32_t z, uint32_t size);
    ~TerrainChunk();
    
    int32_t getX() const { return m_x; }
    int32_t getZ() const { return m_z; }
    uint32_t getSize() const { return m_size; }
    uint32_t getCurrentLOD() const { return m_currentLOD; }
    
    void setCurrentLOD(uint32_t lod) { m_currentLOD = lod; }
    void generateMesh(const std::vector<float>& heightMap, uint32_t lod);
    void updateBuffers();
    
    uint32_t getVertexBuffer() const { return m_vbo; }
    uint32_t getIndexBuffer() const { return m_ibo; }
    uint32_t getIndexCount() const { return m_indexCount; }
    
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
private:
    int32_t m_x, m_z;
    uint32_t m_size;
    uint32_t m_currentLOD;
    uint32_t m_vbo, m_ibo;
    uint32_t m_indexCount;
    bool m_visible;
    
    std::vector<float> m_vertices;
    std::vector<uint32_t> m_indices;
};

/**
 * @brief Terrain material layer for texture splatting
 */
struct TerrainLayer {
    uint32_t diffuseTexture;
    uint32_t normalTexture;
    uint32_t roughnessTexture;
    float tilingScale;
    float heightMin;             // Min height for this layer
    float heightMax;             // Max height for this layer
    float slopeMin;              // Min slope (0-90 degrees)
    float slopeMax;              // Max slope
    
    TerrainLayer()
        : diffuseTexture(0)
        , normalTexture(0)
        , roughnessTexture(0)
        , tilingScale(1.0f)
        , heightMin(0.0f)
        , heightMax(1000.0f)
        , slopeMin(0.0f)
        , slopeMax(90.0f) {}
};

/**
 * @brief Terrain configuration
 */
struct TerrainConfig {
    uint32_t width;              // Terrain width in world units
    uint32_t depth;              // Terrain depth in world units
    uint32_t chunkSize;          // Size of each chunk
    uint32_t heightMapResolution;// Resolution of height map
    float heightScale;           // Scale factor for heights
    float baseHeight;            // Base terrain height
    bool enableCollision;        // Enable physics collision
    bool castShadows;            // Cast shadows
    
    TerrainConfig()
        : width(1024)
        , depth(1024)
        , chunkSize(64)
        , heightMapResolution(1024)
        , heightScale(100.0f)
        , baseHeight(0.0f)
        , enableCollision(true)
        , castShadows(true) {}
};

/**
 * @brief Main terrain system
 */
class TerrainSystem {
public:
    TerrainSystem();
    ~TerrainSystem();
    
    // Configuration
    void configure(const TerrainConfig& config);
    void setLODConfig(const TerrainLODConfig& lodConfig);
    const TerrainConfig& getConfig() const { return m_config; }
    
    // Heightmap
    bool loadHeightMap(const std::string& filepath);
    void generateHeightMap(uint32_t seed);
    void setHeight(uint32_t x, uint32_t z, float height);
    float getHeight(float x, float z) const;
    void smoothHeightMap(uint32_t iterations);
    
    // Terrain generation
    void generateTerrain();
    void updateChunks(const float* cameraPosition);
    
    // Layers and materials
    void addLayer(const TerrainLayer& layer);
    void removeLayer(uint32_t index);
    uint32_t getLayerCount() const { return static_cast<uint32_t>(m_layers.size()); }
    const TerrainLayer* getLayer(uint32_t index) const;
    
    // Splatmap for layer blending
    void generateSplatMap();
    uint32_t getSplatMapTexture() const { return m_splatMapTexture; }
    
    // Rendering
    void render(const float* viewMatrix, const float* projectionMatrix);
    void renderChunk(TerrainChunk* chunk);
    
    // Utilities
    void getNormal(float x, float z, float* normal) const;
    bool raycast(const float* origin, const float* direction, float* hitPoint) const;
    
    // Debug
    void enableWireframe(bool enable) { m_wireframe = enable; }
    bool isWireframeEnabled() const { return m_wireframe; }
    void visualizeLOD(bool enable) { m_visualizeLOD = enable; }
    
private:
    void createChunks();
    void updateChunkLODs(const float* cameraPosition);
    float calculateChunkDistance(const TerrainChunk* chunk, const float* cameraPosition) const;
    uint32_t selectLOD(float distance) const;
    void generateChunkGeometry(TerrainChunk* chunk, uint32_t lod);
    
    TerrainConfig m_config;
    TerrainLODConfig m_lodConfig;
    
    std::vector<float> m_heightMap;
    std::vector<std::unique_ptr<TerrainChunk>> m_chunks;
    std::vector<TerrainLayer> m_layers;
    
    uint32_t m_splatMapTexture;
    uint32_t m_terrainShader;
    
    bool m_wireframe;
    bool m_visualizeLOD;
};

/**
 * @brief Procedural terrain generator
 */
class ProceduralTerrainGenerator {
public:
    enum class NoiseType {
        Perlin,
        Simplex,
        Ridged,
        Billow,
        Voronoi
    };
    
    struct NoiseLayer {
        NoiseType type;
        float frequency;
        float amplitude;
        uint32_t octaves;
        float persistence;
        float lacunarity;
        
        NoiseLayer()
            : type(NoiseType::Perlin)
            , frequency(1.0f)
            , amplitude(1.0f)
            , octaves(4)
            , persistence(0.5f)
            , lacunarity(2.0f) {}
    };
    
    static void generateHeightMap(std::vector<float>& heightMap, 
                                  uint32_t width, uint32_t height,
                                  uint32_t seed,
                                  const std::vector<NoiseLayer>& layers);
    
    static void applyErosion(std::vector<float>& heightMap, 
                            uint32_t width, uint32_t height,
                            uint32_t iterations,
                            float erosionStrength);
    
    static void applyTerracing(std::vector<float>& heightMap, 
                              uint32_t levels,
                              float smoothness);
};

} // namespace Graphics
} // namespace JJM

#endif // TERRAIN_SYSTEM_H
