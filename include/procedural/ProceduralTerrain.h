#pragma once

#include <vector>
#include <string>
#include <functional>

// Procedural terrain generation with multiple algorithms
namespace Engine {

enum class TerrainAlgorithm {
    PerlinNoise,
    SimplexNoise,
    DiamondSquare,
    MidpointDisplacement,
    VoronoiDiagram,
    Hydraulic,           // Erosion simulation
    Thermal              // Thermal erosion
};

struct TerrainSettings {
    int width;
    int height;
    float scale;
    float heightMultiplier;
    int octaves;
    float persistence;
    float lacunarity;
    int seed;
    
    // Biome settings
    float temperatureVariation;
    float moistureVariation;
    float biomeBlendDistance;
};

struct BiomeType {
    std::string name;
    float minHeight;
    float maxHeight;
    float minTemperature;
    float maxTemperature;
    float minMoisture;
    float maxMoisture;
    
    float terrainRoughness;
    float vegetationDensity;
};

class ProceduralTerrain {
public:
    ProceduralTerrain();
    ~ProceduralTerrain();

    // Generation
    void generate(const TerrainSettings& settings);
    void generateWithAlgorithm(TerrainAlgorithm algorithm, const TerrainSettings& settings);
    void regenerate();
    
    // Height manipulation
    float getHeight(int x, int y) const;
    void setHeight(int x, int y, float height);
    float getInterpolatedHeight(float x, float y) const;
    
    // Normal calculation
    void calculateNormals();
    void getNormal(int x, int y, float& nx, float& ny, float& nz) const;
    
    // Erosion
    void applyHydraulicErosion(int iterations, float rainAmount, float evaporationRate, float sedimentCapacity);
    void applyThermalErosion(int iterations, float talusAngle, float erosionRate);
    void applyWindErosion(int iterations, float windStrength, float windDirectionX, float windDirectionY);
    
    // Smoothing
    void smooth(int iterations, float strength);
    void gaussianBlur(float sigma);
    
    // Features
    void addMountainRange(float startX, float startY, float endX, float endY, float height, float width);
    void addValley(float centerX, float centerY, float radius, float depth);
    void addPlateau(float centerX, float centerY, float radius, float height);
    void addRiver(const std::vector<std::pair<float, float>>& path, float width, float depth);
    
    // Biomes
    void generateBiomeMap();
    int getBiome(int x, int y) const;
    void addBiome(const BiomeType& biome);
    
    // Texturing
    void generateTextureWeights();
    float getTextureWeight(int x, int y, int textureIndex) const;
    
    // Query
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    const std::vector<float>& getHeightMap() const { return m_heightMap; }
    const std::vector<float>& getNormalMap() const { return m_normalMap; }
    
    // Export
    void exportToHeightmap(const std::string& filename);
    void exportToMesh(const std::string& filename);
    void exportToRAW(const std::string& filename);

private:
    // Generation algorithms
    void generatePerlinNoise(const TerrainSettings& settings);
    void generateSimplexNoise(const TerrainSettings& settings);
    void generateDiamondSquare(const TerrainSettings& settings);
    void generateMidpointDisplacement(const TerrainSettings& settings);
    void generateVoronoi(const TerrainSettings& settings);
    
    // Noise functions
    float perlinNoise(float x, float y) const;
    float simplexNoise(float x, float y) const;
    float fbmNoise(float x, float y, int octaves, float persistence, float lacunarity) const;
    
    // Utilities
    float interpolate(float a, float b, float t) const;
    float cosineInterpolate(float a, float b, float t) const;
    bool isValidCoordinate(int x, int y) const;
    int getIndex(int x, int y) const;

    int m_width;
    int m_height;
    TerrainSettings m_settings;
    TerrainAlgorithm m_algorithm;
    
    std::vector<float> m_heightMap;
    std::vector<float> m_normalMap;
    std::vector<int> m_biomeMap;
    std::vector<float> m_temperatureMap;
    std::vector<float> m_moistureMap;
    std::vector<std::vector<float>> m_textureWeights;
    
    std::vector<BiomeType> m_biomes;
};

class TerrainGenerator {
public:
    static TerrainGenerator& getInstance();

    // Preset terrains
    ProceduralTerrain* generateFlatTerrain(int width, int height);
    ProceduralTerrain* generateHillyTerrain(int width, int height, int seed = 0);
    ProceduralTerrain* generateMountainousTerrain(int width, int height, int seed = 0);
    ProceduralTerrain* generateIslandTerrain(int width, int height, int seed = 0);
    ProceduralTerrain* generateCanyonTerrain(int width, int height, int seed = 0);
    
    // Custom generation
    ProceduralTerrain* generateCustomTerrain(const TerrainSettings& settings, TerrainAlgorithm algorithm);
    
    // Terrain management
    void destroyTerrain(ProceduralTerrain* terrain);

private:
    TerrainGenerator();
    TerrainGenerator(const TerrainGenerator&) = delete;
    TerrainGenerator& operator=(const TerrainGenerator&) = delete;
};

} // namespace Engine
