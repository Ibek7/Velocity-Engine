#ifndef JJM_TERRAIN_GENERATOR_H
#define JJM_TERRAIN_GENERATOR_H

#include "math/Vector2D.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <random>
#include <cmath>
#include <array>

namespace JJM {
namespace Procedural {

enum class TerrainType { Plains, Hills, Mountains, Water, Desert };

struct TerrainChunk {
    int x, z;
    std::vector<float> heightMap;
    std::vector<TerrainType> typeMap;
    int width, depth;
};

// =============================================================================
// Advanced Procedural Terrain Generation System
// =============================================================================

// Biome types for terrain classification
enum class BiomeType {
    Ocean,
    Beach,
    Desert,
    Savanna,
    Grassland,
    Forest,
    Taiga,
    Tundra,
    Snow,
    Jungle,
    Swamp,
    Mountains,
    Volcanic,
    Mesa,
    Count
};

// Biome parameters
struct BiomeParameters {
    BiomeType type;
    std::string name;
    
    // Height and moisture thresholds
    float minHeight, maxHeight;
    float minMoisture, maxMoisture;
    float minTemperature, maxTemperature;
    
    // Terrain characteristics
    float baseRoughness;
    float erosionResistance;
    float vegetationDensity;
    
    // Visual properties
    float colorR, colorG, colorB;
    std::string textureId;
    
    BiomeParameters()
        : type(BiomeType::Grassland)
        , minHeight(0), maxHeight(1)
        , minMoisture(0), maxMoisture(1)
        , minTemperature(0), maxTemperature(1)
        , baseRoughness(1.0f)
        , erosionResistance(0.5f)
        , vegetationDensity(0.5f)
        , colorR(0.3f), colorG(0.6f), colorB(0.2f)
    {}
};

// Advanced noise generator with multiple algorithms
class NoiseGenerator {
public:
    enum class NoiseType {
        Perlin,
        Simplex,
        Worley,
        Value,
        Ridged,
        Billowy
    };
    
private:
    int seed;
    std::vector<int> permutation;
    std::mt19937 rng;
    
public:
    NoiseGenerator(int seed = 0) : seed(seed), rng(seed) {
        generatePermutation();
    }
    
    void setSeed(int newSeed) {
        seed = newSeed;
        rng.seed(seed);
        generatePermutation();
    }
    
    // 2D noise functions
    float perlin2D(float x, float y) const {
        int xi = fastFloor(x) & 255;
        int yi = fastFloor(y) & 255;
        
        float xf = x - fastFloor(x);
        float yf = y - fastFloor(y);
        
        float u = fade(xf);
        float v = fade(yf);
        
        int aa = permutation[permutation[xi] + yi];
        int ab = permutation[permutation[xi] + yi + 1];
        int ba = permutation[permutation[xi + 1] + yi];
        int bb = permutation[permutation[xi + 1] + yi + 1];
        
        float x1 = lerp(grad2D(aa, xf, yf), grad2D(ba, xf - 1, yf), u);
        float x2 = lerp(grad2D(ab, xf, yf - 1), grad2D(bb, xf - 1, yf - 1), u);
        
        return lerp(x1, x2, v);
    }
    
    float simplex2D(float x, float y) const {
        const float F2 = 0.5f * (std::sqrt(3.0f) - 1.0f);
        const float G2 = (3.0f - std::sqrt(3.0f)) / 6.0f;
        
        float s = (x + y) * F2;
        int i = fastFloor(x + s);
        int j = fastFloor(y + s);
        
        float t = (i + j) * G2;
        float X0 = i - t;
        float Y0 = j - t;
        float x0 = x - X0;
        float y0 = y - Y0;
        
        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else { i1 = 0; j1 = 1; }
        
        float x1 = x0 - i1 + G2;
        float y1 = y0 - j1 + G2;
        float x2 = x0 - 1.0f + 2.0f * G2;
        float y2 = y0 - 1.0f + 2.0f * G2;
        
        int ii = i & 255;
        int jj = j & 255;
        
        float n0 = 0, n1 = 0, n2 = 0;
        
        float t0 = 0.5f - x0 * x0 - y0 * y0;
        if (t0 >= 0) {
            t0 *= t0;
            n0 = t0 * t0 * grad2D(permutation[ii + permutation[jj]], x0, y0);
        }
        
        float t1 = 0.5f - x1 * x1 - y1 * y1;
        if (t1 >= 0) {
            t1 *= t1;
            n1 = t1 * t1 * grad2D(permutation[ii + i1 + permutation[jj + j1]], x1, y1);
        }
        
        float t2 = 0.5f - x2 * x2 - y2 * y2;
        if (t2 >= 0) {
            t2 *= t2;
            n2 = t2 * t2 * grad2D(permutation[ii + 1 + permutation[jj + 1]], x2, y2);
        }
        
        return 70.0f * (n0 + n1 + n2);
    }
    
    float worley2D(float x, float y) const {
        int xi = fastFloor(x);
        int yi = fastFloor(y);
        
        float minDist = 1e10f;
        
        for (int ox = -1; ox <= 1; ++ox) {
            for (int oy = -1; oy <= 1; ++oy) {
                int cx = xi + ox;
                int cy = yi + oy;
                
                // Deterministic random point in cell
                float px = cx + hash2D(cx, cy) / 4294967295.0f;
                float py = cy + hash2D(cy, cx) / 4294967295.0f;
                
                float dx = x - px;
                float dy = y - py;
                float dist = dx * dx + dy * dy;
                
                minDist = std::min(minDist, dist);
            }
        }
        
        return std::sqrt(minDist);
    }
    
    float ridged2D(float x, float y) const {
        return 1.0f - std::abs(perlin2D(x, y));
    }
    
    float billowy2D(float x, float y) const {
        return std::abs(perlin2D(x, y));
    }
    
    // Fractal Brownian Motion
    float fbm(float x, float y, int octaves, float persistence, float lacunarity, NoiseType type = NoiseType::Perlin) const {
        float total = 0;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0;
        
        for (int i = 0; i < octaves; ++i) {
            float noiseValue = 0;
            
            switch (type) {
                case NoiseType::Perlin:
                    noiseValue = perlin2D(x * frequency, y * frequency);
                    break;
                case NoiseType::Simplex:
                    noiseValue = simplex2D(x * frequency, y * frequency);
                    break;
                case NoiseType::Worley:
                    noiseValue = worley2D(x * frequency, y * frequency);
                    break;
                case NoiseType::Ridged:
                    noiseValue = ridged2D(x * frequency, y * frequency);
                    break;
                case NoiseType::Billowy:
                    noiseValue = billowy2D(x * frequency, y * frequency);
                    break;
                default:
                    noiseValue = perlin2D(x * frequency, y * frequency);
            }
            
            total += noiseValue * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        
        return total / maxValue;
    }
    
    // Domain warping
    float domainWarp(float x, float y, float warpStrength, int warpOctaves = 3) const {
        float qx = fbm(x, y, warpOctaves, 0.5f, 2.0f);
        float qy = fbm(x + 5.2f, y + 1.3f, warpOctaves, 0.5f, 2.0f);
        return fbm(x + warpStrength * qx, y + warpStrength * qy, 4, 0.5f, 2.0f);
    }
    
private:
    void generatePermutation() {
        permutation.resize(512);
        std::vector<int> p(256);
        for (int i = 0; i < 256; ++i) p[i] = i;
        std::shuffle(p.begin(), p.end(), rng);
        for (int i = 0; i < 512; ++i) permutation[i] = p[i & 255];
    }
    
    static int fastFloor(float x) {
        return x > 0 ? (int)x : (int)x - 1;
    }
    
    static float fade(float t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    
    static float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }
    
    float grad2D(int hash, float x, float y) const {
        int h = hash & 7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
    }
    
    uint32_t hash2D(int x, int y) const {
        uint32_t h = seed;
        h ^= x * 374761393u;
        h = (h << 17) | (h >> 15);
        h *= 668265263u;
        h ^= y * 374761393u;
        h = (h << 17) | (h >> 15);
        h *= 668265263u;
        return h;
    }
};

// Hydraulic erosion simulation
class HydraulicErosion {
public:
    struct Parameters {
        int iterations;
        float inertia;
        float sedimentCapacity;
        float depositionRate;
        float erosionRate;
        float evaporationRate;
        float gravity;
        float minSlope;
        int dropletLifetime;
        float initialWater;
        float initialSpeed;
        int erosionRadius;
        
        Parameters()
            : iterations(50000)
            , inertia(0.05f)
            , sedimentCapacity(4.0f)
            , depositionRate(0.3f)
            , erosionRate(0.3f)
            , evaporationRate(0.01f)
            , gravity(4.0f)
            , minSlope(0.01f)
            , dropletLifetime(30)
            , initialWater(1.0f)
            , initialSpeed(1.0f)
            , erosionRadius(3)
        {}
    };
    
private:
    Parameters params;
    std::mt19937 rng;
    std::vector<std::vector<float>> erosionBrushWeights;
    std::vector<std::vector<int>> erosionBrushIndices;
    int mapSize;
    
public:
    HydraulicErosion(int seed = 0) : rng(seed), mapSize(0) {}
    
    void setParameters(const Parameters& p) { params = p; }
    const Parameters& getParameters() const { return params; }
    
    void erode(std::vector<float>& heightMap, int width, int height) {
        mapSize = width;
        initializeBrushes(width, height);
        
        std::uniform_real_distribution<float> distX(0, width - 1);
        std::uniform_real_distribution<float> distY(0, height - 1);
        
        for (int iter = 0; iter < params.iterations; ++iter) {
            float posX = distX(rng);
            float posY = distY(rng);
            float dirX = 0, dirY = 0;
            float speed = params.initialSpeed;
            float water = params.initialWater;
            float sediment = 0;
            
            for (int lifetime = 0; lifetime < params.dropletLifetime; ++lifetime) {
                int nodeX = static_cast<int>(posX);
                int nodeY = static_cast<int>(posY);
                int dropletIndex = nodeY * width + nodeX;
                
                float cellOffsetX = posX - nodeX;
                float cellOffsetY = posY - nodeY;
                
                // Calculate height and gradient
                auto [height, gradX, gradY] = calculateHeightAndGradient(heightMap, width, posX, posY);
                
                // Update direction
                dirX = dirX * params.inertia - gradX * (1 - params.inertia);
                dirY = dirY * params.inertia - gradY * (1 - params.inertia);
                
                float len = std::sqrt(dirX * dirX + dirY * dirY);
                if (len != 0) {
                    dirX /= len;
                    dirY /= len;
                }
                
                posX += dirX;
                posY += dirY;
                
                // Check bounds
                if (posX < 0 || posX >= width - 1 || posY < 0 || posY >= height - 1) break;
                
                float newHeight = calculateHeightAndGradient(heightMap, width, posX, posY).height;
                float deltaHeight = newHeight - height;
                
                // Calculate sediment capacity
                float capacity = std::max(-deltaHeight, params.minSlope) * speed * water * params.sedimentCapacity;
                
                if (sediment > capacity || deltaHeight > 0) {
                    // Deposit sediment
                    float depositAmount = (deltaHeight > 0) 
                        ? std::min(deltaHeight, sediment) 
                        : (sediment - capacity) * params.depositionRate;
                    
                    sediment -= depositAmount;
                    depositSediment(heightMap, width, height, nodeX, nodeY, cellOffsetX, cellOffsetY, depositAmount);
                } else {
                    // Erode terrain
                    float erodeAmount = std::min((capacity - sediment) * params.erosionRate, -deltaHeight);
                    
                    for (size_t i = 0; i < erosionBrushIndices[dropletIndex].size(); ++i) {
                        int erodeIndex = erosionBrushIndices[dropletIndex][i];
                        float weightedErode = erodeAmount * erosionBrushWeights[dropletIndex][i];
                        float currentHeight = heightMap[erodeIndex];
                        heightMap[erodeIndex] = std::max(0.0f, currentHeight - weightedErode);
                        sediment += currentHeight - heightMap[erodeIndex];
                    }
                }
                
                speed = std::sqrt(std::max(0.0f, speed * speed + deltaHeight * params.gravity));
                water *= (1 - params.evaporationRate);
            }
        }
    }
    
private:
    struct HeightGradient {
        float height;
        float gradX;
        float gradY;
    };
    
    HeightGradient calculateHeightAndGradient(const std::vector<float>& heightMap, int width, float x, float y) {
        int coordX = static_cast<int>(x);
        int coordY = static_cast<int>(y);
        
        float offsetX = x - coordX;
        float offsetY = y - coordY;
        
        int indexNW = coordY * width + coordX;
        float heightNW = heightMap[indexNW];
        float heightNE = heightMap[indexNW + 1];
        float heightSW = heightMap[indexNW + width];
        float heightSE = heightMap[indexNW + width + 1];
        
        float gradX = (heightNE - heightNW) * (1 - offsetY) + (heightSE - heightSW) * offsetY;
        float gradY = (heightSW - heightNW) * (1 - offsetX) + (heightSE - heightNE) * offsetX;
        float height = heightNW * (1 - offsetX) * (1 - offsetY)
                     + heightNE * offsetX * (1 - offsetY)
                     + heightSW * (1 - offsetX) * offsetY
                     + heightSE * offsetX * offsetY;
        
        return { height, gradX, gradY };
    }
    
    void depositSediment(std::vector<float>& heightMap, int width, int height, 
                        int nodeX, int nodeY, float cellOffsetX, float cellOffsetY, float amount) {
        heightMap[nodeY * width + nodeX] += amount * (1 - cellOffsetX) * (1 - cellOffsetY);
        heightMap[nodeY * width + nodeX + 1] += amount * cellOffsetX * (1 - cellOffsetY);
        heightMap[(nodeY + 1) * width + nodeX] += amount * (1 - cellOffsetX) * cellOffsetY;
        heightMap[(nodeY + 1) * width + nodeX + 1] += amount * cellOffsetX * cellOffsetY;
    }
    
    void initializeBrushes(int width, int height) {
        int mapSize = width * height;
        erosionBrushIndices.resize(mapSize);
        erosionBrushWeights.resize(mapSize);
        
        int radius = params.erosionRadius;
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int centerIndex = y * width + x;
                
                std::vector<int> indices;
                std::vector<float> weights;
                float weightSum = 0;
                
                for (int oy = -radius; oy <= radius; ++oy) {
                    for (int ox = -radius; ox <= radius; ++ox) {
                        int nx = x + ox;
                        int ny = y + oy;
                        
                        if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                            float dist = std::sqrt(static_cast<float>(ox * ox + oy * oy));
                            if (dist <= radius) {
                                float weight = 1 - dist / radius;
                                indices.push_back(ny * width + nx);
                                weights.push_back(weight);
                                weightSum += weight;
                            }
                        }
                    }
                }
                
                // Normalize weights
                for (float& w : weights) w /= weightSum;
                
                erosionBrushIndices[centerIndex] = std::move(indices);
                erosionBrushWeights[centerIndex] = std::move(weights);
            }
        }
    }
};

// Thermal erosion simulation
class ThermalErosion {
public:
    struct Parameters {
        int iterations;
        float talusAngle;       // Angle of repose in radians
        float erosionRate;
        
        Parameters()
            : iterations(50)
            , talusAngle(0.5f)   // ~30 degrees
            , erosionRate(0.5f)
        {}
    };
    
private:
    Parameters params;
    
public:
    void setParameters(const Parameters& p) { params = p; }
    
    void erode(std::vector<float>& heightMap, int width, int height) {
        float cellSize = 1.0f;
        float maxDiff = std::tan(params.talusAngle) * cellSize;
        
        for (int iter = 0; iter < params.iterations; ++iter) {
            std::vector<float> erosionMap(heightMap.size(), 0);
            
            for (int y = 1; y < height - 1; ++y) {
                for (int x = 1; x < width - 1; ++x) {
                    int idx = y * width + x;
                    float h = heightMap[idx];
                    
                    // Check 8 neighbors
                    float maxSlope = 0;
                    int steepestNeighbor = -1;
                    
                    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
                    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
                    
                    for (int i = 0; i < 8; ++i) {
                        int ni = (y + dy[i]) * width + (x + dx[i]);
                        float diff = h - heightMap[ni];
                        float dist = (dx[i] != 0 && dy[i] != 0) ? 1.414f : 1.0f;
                        float slope = diff / dist;
                        
                        if (slope > maxSlope) {
                            maxSlope = slope;
                            steepestNeighbor = ni;
                        }
                    }
                    
                    if (maxSlope > maxDiff && steepestNeighbor >= 0) {
                        float moveAmount = (maxSlope - maxDiff) * params.erosionRate * 0.5f;
                        erosionMap[idx] -= moveAmount;
                        erosionMap[steepestNeighbor] += moveAmount;
                    }
                }
            }
            
            // Apply erosion
            for (size_t i = 0; i < heightMap.size(); ++i) {
                heightMap[i] += erosionMap[i];
            }
        }
    }
};

// Biome system for terrain classification
class BiomeSystem {
private:
    std::vector<BiomeParameters> biomes;
    NoiseGenerator temperatureNoise;
    NoiseGenerator moistureNoise;
    float temperatureScale;
    float moistureScale;
    
public:
    BiomeSystem(int seed = 0)
        : temperatureNoise(seed)
        , moistureNoise(seed + 1000)
        , temperatureScale(0.003f)
        , moistureScale(0.004f)
    {
        initializeDefaultBiomes();
    }
    
    void initializeDefaultBiomes() {
        // Ocean
        BiomeParameters ocean;
        ocean.type = BiomeType::Ocean;
        ocean.name = "Ocean";
        ocean.minHeight = -1.0f; ocean.maxHeight = 0.0f;
        ocean.colorR = 0.1f; ocean.colorG = 0.3f; ocean.colorB = 0.7f;
        biomes.push_back(ocean);
        
        // Beach
        BiomeParameters beach;
        beach.type = BiomeType::Beach;
        beach.name = "Beach";
        beach.minHeight = 0.0f; beach.maxHeight = 0.1f;
        beach.colorR = 0.9f; beach.colorG = 0.85f; beach.colorB = 0.6f;
        biomes.push_back(beach);
        
        // Desert
        BiomeParameters desert;
        desert.type = BiomeType::Desert;
        desert.name = "Desert";
        desert.minHeight = 0.1f; desert.maxHeight = 0.5f;
        desert.minMoisture = 0.0f; desert.maxMoisture = 0.3f;
        desert.minTemperature = 0.5f; desert.maxTemperature = 1.0f;
        desert.colorR = 0.9f; desert.colorG = 0.8f; desert.colorB = 0.5f;
        biomes.push_back(desert);
        
        // Grassland
        BiomeParameters grassland;
        grassland.type = BiomeType::Grassland;
        grassland.name = "Grassland";
        grassland.minHeight = 0.1f; grassland.maxHeight = 0.5f;
        grassland.minMoisture = 0.3f; grassland.maxMoisture = 0.7f;
        grassland.colorR = 0.3f; grassland.colorG = 0.6f; grassland.colorB = 0.2f;
        biomes.push_back(grassland);
        
        // Forest
        BiomeParameters forest;
        forest.type = BiomeType::Forest;
        forest.name = "Forest";
        forest.minHeight = 0.1f; forest.maxHeight = 0.6f;
        forest.minMoisture = 0.5f; forest.maxMoisture = 1.0f;
        forest.colorR = 0.1f; forest.colorG = 0.4f; forest.colorB = 0.1f;
        biomes.push_back(forest);
        
        // Mountains
        BiomeParameters mountains;
        mountains.type = BiomeType::Mountains;
        mountains.name = "Mountains";
        mountains.minHeight = 0.6f; mountains.maxHeight = 1.0f;
        mountains.colorR = 0.5f; mountains.colorG = 0.5f; mountains.colorB = 0.5f;
        biomes.push_back(mountains);
        
        // Snow
        BiomeParameters snow;
        snow.type = BiomeType::Snow;
        snow.name = "Snow";
        snow.minHeight = 0.8f; snow.maxHeight = 1.0f;
        snow.minTemperature = 0.0f; snow.maxTemperature = 0.3f;
        snow.colorR = 0.95f; snow.colorG = 0.95f; snow.colorB = 1.0f;
        biomes.push_back(snow);
    }
    
    BiomeType getBiome(float x, float z, float height) const {
        float temperature = (temperatureNoise.perlin2D(x * temperatureScale, z * temperatureScale) + 1) * 0.5f;
        float moisture = (moistureNoise.perlin2D(x * moistureScale, z * moistureScale) + 1) * 0.5f;
        
        // Adjust temperature based on height (higher = colder)
        temperature -= height * 0.5f;
        
        float bestScore = -1e10f;
        BiomeType best = BiomeType::Grassland;
        
        for (const auto& biome : biomes) {
            float score = 0;
            
            // Height match
            if (height >= biome.minHeight && height <= biome.maxHeight) {
                score += 10.0f;
            } else {
                continue;
            }
            
            // Temperature match
            if (temperature >= biome.minTemperature && temperature <= biome.maxTemperature) {
                score += 5.0f;
            }
            
            // Moisture match
            if (moisture >= biome.minMoisture && moisture <= biome.maxMoisture) {
                score += 5.0f;
            }
            
            if (score > bestScore) {
                bestScore = score;
                best = biome.type;
            }
        }
        
        return best;
    }
    
    const BiomeParameters* getBiomeParameters(BiomeType type) const {
        for (const auto& biome : biomes) {
            if (biome.type == type) return &biome;
        }
        return nullptr;
    }
    
    void addBiome(const BiomeParameters& biome) {
        biomes.push_back(biome);
    }
};

// Multi-layer terrain generation
struct TerrainLayer {
    std::string name;
    NoiseGenerator::NoiseType noiseType;
    float scale;
    int octaves;
    float persistence;
    float lacunarity;
    float amplitude;
    float warpStrength;
    bool enabled;
    
    TerrainLayer()
        : noiseType(NoiseGenerator::NoiseType::Perlin)
        , scale(1.0f)
        , octaves(6)
        , persistence(0.5f)
        , lacunarity(2.0f)
        , amplitude(1.0f)
        , warpStrength(0.0f)
        , enabled(true)
    {}
};

// Advanced terrain chunk with additional data
struct AdvancedTerrainChunk {
    int chunkX, chunkZ;
    int width, depth;
    
    std::vector<float> heightMap;
    std::vector<float> moistureMap;
    std::vector<float> temperatureMap;
    std::vector<BiomeType> biomeMap;
    
    // Normal map for lighting
    std::vector<float> normalX;
    std::vector<float> normalY;
    std::vector<float> normalZ;
    
    // Additional data
    std::vector<uint8_t> splatMap;   // Texture blending weights
    std::vector<float> occlusionMap; // Ambient occlusion
    
    bool isGenerated;
    bool isEroded;
    
    AdvancedTerrainChunk() : chunkX(0), chunkZ(0), width(0), depth(0), isGenerated(false), isEroded(false) {}
};

// Main advanced terrain generator
class AdvancedTerrainGenerator {
private:
    int seed;
    NoiseGenerator noiseGen;
    BiomeSystem biomeSystem;
    HydraulicErosion hydraulicErosion;
    ThermalErosion thermalErosion;
    
    std::vector<TerrainLayer> layers;
    
    // Generation settings
    float baseScale;
    float heightMultiplier;
    int chunkSize;
    bool enableErosion;
    bool enableBiomes;
    
public:
    AdvancedTerrainGenerator(int seed = 0)
        : seed(seed)
        , noiseGen(seed)
        , biomeSystem(seed)
        , hydraulicErosion(seed)
        , baseScale(0.005f)
        , heightMultiplier(100.0f)
        , chunkSize(256)
        , enableErosion(true)
        , enableBiomes(true)
    {
        initializeDefaultLayers();
    }
    
    void initializeDefaultLayers() {
        // Base terrain
        TerrainLayer base;
        base.name = "Base";
        base.noiseType = NoiseGenerator::NoiseType::Simplex;
        base.scale = baseScale;
        base.octaves = 6;
        base.amplitude = 1.0f;
        layers.push_back(base);
        
        // Mountains
        TerrainLayer mountains;
        mountains.name = "Mountains";
        mountains.noiseType = NoiseGenerator::NoiseType::Ridged;
        mountains.scale = baseScale * 0.5f;
        mountains.octaves = 4;
        mountains.amplitude = 0.5f;
        layers.push_back(mountains);
        
        // Detail
        TerrainLayer detail;
        detail.name = "Detail";
        detail.noiseType = NoiseGenerator::NoiseType::Perlin;
        detail.scale = baseScale * 5.0f;
        detail.octaves = 3;
        detail.amplitude = 0.1f;
        layers.push_back(detail);
    }
    
    AdvancedTerrainChunk generateChunk(int chunkX, int chunkZ) {
        AdvancedTerrainChunk chunk;
        chunk.chunkX = chunkX;
        chunk.chunkZ = chunkZ;
        chunk.width = chunkSize;
        chunk.depth = chunkSize;
        
        int totalSize = chunkSize * chunkSize;
        chunk.heightMap.resize(totalSize);
        chunk.moistureMap.resize(totalSize);
        chunk.temperatureMap.resize(totalSize);
        chunk.biomeMap.resize(totalSize);
        chunk.normalX.resize(totalSize);
        chunk.normalY.resize(totalSize);
        chunk.normalZ.resize(totalSize);
        
        float worldOffsetX = chunkX * chunkSize;
        float worldOffsetZ = chunkZ * chunkSize;
        
        // Generate height map from layers
        for (int z = 0; z < chunkSize; ++z) {
            for (int x = 0; x < chunkSize; ++x) {
                float worldX = worldOffsetX + x;
                float worldZ = worldOffsetZ + z;
                
                float height = 0;
                for (const auto& layer : layers) {
                    if (!layer.enabled) continue;
                    
                    float layerValue;
                    if (layer.warpStrength > 0) {
                        layerValue = noiseGen.domainWarp(worldX * layer.scale, worldZ * layer.scale, layer.warpStrength);
                    } else {
                        layerValue = noiseGen.fbm(worldX * layer.scale, worldZ * layer.scale, 
                                                  layer.octaves, layer.persistence, layer.lacunarity, layer.noiseType);
                    }
                    height += layerValue * layer.amplitude;
                }
                
                // Normalize to 0-1 range
                height = (height + 1) * 0.5f;
                chunk.heightMap[z * chunkSize + x] = height;
            }
        }
        
        // Apply erosion
        if (enableErosion) {
            hydraulicErosion.erode(chunk.heightMap, chunkSize, chunkSize);
            thermalErosion.erode(chunk.heightMap, chunkSize, chunkSize);
            chunk.isEroded = true;
        }
        
        // Generate biomes
        if (enableBiomes) {
            for (int z = 0; z < chunkSize; ++z) {
                for (int x = 0; x < chunkSize; ++x) {
                    float worldX = worldOffsetX + x;
                    float worldZ = worldOffsetZ + z;
                    float height = chunk.heightMap[z * chunkSize + x];
                    
                    chunk.biomeMap[z * chunkSize + x] = biomeSystem.getBiome(worldX, worldZ, height);
                }
            }
        }
        
        // Calculate normals
        calculateNormals(chunk);
        
        chunk.isGenerated = true;
        return chunk;
    }
    
    void calculateNormals(AdvancedTerrainChunk& chunk) {
        for (int z = 1; z < chunk.depth - 1; ++z) {
            for (int x = 1; x < chunk.width - 1; ++x) {
                float hL = chunk.heightMap[z * chunk.width + (x - 1)];
                float hR = chunk.heightMap[z * chunk.width + (x + 1)];
                float hD = chunk.heightMap[(z - 1) * chunk.width + x];
                float hU = chunk.heightMap[(z + 1) * chunk.width + x];
                
                float nx = hL - hR;
                float nz = hD - hU;
                float ny = 2.0f;
                
                float len = std::sqrt(nx * nx + ny * ny + nz * nz);
                
                int idx = z * chunk.width + x;
                chunk.normalX[idx] = nx / len;
                chunk.normalY[idx] = ny / len;
                chunk.normalZ[idx] = nz / len;
            }
        }
    }
    
    // Configuration
    void setSeed(int newSeed) {
        seed = newSeed;
        noiseGen.setSeed(seed);
    }
    
    void setBaseScale(float scale) { baseScale = scale; }
    void setHeightMultiplier(float mult) { heightMultiplier = mult; }
    void setChunkSize(int size) { chunkSize = size; }
    void setEnableErosion(bool enable) { enableErosion = enable; }
    void setEnableBiomes(bool enable) { enableBiomes = enable; }
    
    void addLayer(const TerrainLayer& layer) { layers.push_back(layer); }
    void clearLayers() { layers.clear(); }
    std::vector<TerrainLayer>& getLayers() { return layers; }
    
    HydraulicErosion& getHydraulicErosion() { return hydraulicErosion; }
    ThermalErosion& getThermalErosion() { return thermalErosion; }
    BiomeSystem& getBiomeSystem() { return biomeSystem; }
};

class TerrainGenerator {
public:
    TerrainGenerator(int seed = 0);
    ~TerrainGenerator();
    
    TerrainChunk generateChunk(int chunkX, int chunkZ, int width, int depth);
    
    void setSeed(int seed);
    void setOctaves(int octaves);
    void setPersistence(float persistence);
    void setScale(float scale);
    void setHeightMultiplier(float multiplier);
    
    float getHeight(float x, float z) const;
    TerrainType getTerrainType(float height) const;

private:
    int seed;
    int octaves;
    float persistence;
    float scale;
    float heightMultiplier;
    
    float noise(float x, float z) const;
    float perlinNoise(float x, float z) const;
    float interpolate(float a, float b, float t) const;
};

} // namespace Procedural
} // namespace JJM

#endif
