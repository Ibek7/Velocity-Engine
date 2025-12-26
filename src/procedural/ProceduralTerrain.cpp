#include "procedural/ProceduralTerrain.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <fstream>

namespace Engine {

ProceduralTerrain::ProceduralTerrain()
    : m_width(256)
    , m_height(256)
    , m_algorithm(TerrainAlgorithm::PerlinNoise)
{
    m_settings.width = 256;
    m_settings.height = 256;
    m_settings.scale = 50.0f;
    m_settings.heightMultiplier = 10.0f;
    m_settings.octaves = 4;
    m_settings.persistence = 0.5f;
    m_settings.lacunarity = 2.0f;
    m_settings.seed = 0;
    m_settings.temperatureVariation = 1.0f;
    m_settings.moistureVariation = 1.0f;
    m_settings.biomeBlendDistance = 5.0f;
}

ProceduralTerrain::~ProceduralTerrain() {
}

void ProceduralTerrain::generate(const TerrainSettings& settings) {
    m_settings = settings;
    m_width = settings.width;
    m_height = settings.height;
    
    m_heightMap.resize(m_width * m_height, 0.0f);
    m_normalMap.resize(m_width * m_height * 3, 0.0f);
    m_biomeMap.resize(m_width * m_height, 0);
    
    generateWithAlgorithm(m_algorithm, settings);
    calculateNormals();
}

void ProceduralTerrain::generateWithAlgorithm(TerrainAlgorithm algorithm, const TerrainSettings& settings) {
    m_algorithm = algorithm;
    
    switch (algorithm) {
        case TerrainAlgorithm::PerlinNoise:
            generatePerlinNoise(settings);
            break;
        case TerrainAlgorithm::SimplexNoise:
            generateSimplexNoise(settings);
            break;
        case TerrainAlgorithm::DiamondSquare:
            generateDiamondSquare(settings);
            break;
        case TerrainAlgorithm::MidpointDisplacement:
            generateMidpointDisplacement(settings);
            break;
        case TerrainAlgorithm::VoronoiDiagram:
            generateVoronoi(settings);
            break;
        default:
            generatePerlinNoise(settings);
            break;
    }
}

void ProceduralTerrain::regenerate() {
    generate(m_settings);
}

float ProceduralTerrain::getHeight(int x, int y) const {
    if (!isValidCoordinate(x, y)) {
        return 0.0f;
    }
    return m_heightMap[getIndex(x, y)];
}

void ProceduralTerrain::setHeight(int x, int y, float height) {
    if (isValidCoordinate(x, y)) {
        m_heightMap[getIndex(x, y)] = height;
    }
}

float ProceduralTerrain::getInterpolatedHeight(float x, float y) const {
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    float fx = x - x0;
    float fy = y - y0;
    
    float h00 = getHeight(x0, y0);
    float h10 = getHeight(x1, y0);
    float h01 = getHeight(x0, y1);
    float h11 = getHeight(x1, y1);
    
    float h0 = interpolate(h00, h10, fx);
    float h1 = interpolate(h01, h11, fx);
    
    return interpolate(h0, h1, fy);
}

void ProceduralTerrain::calculateNormals() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float left = getHeight(x - 1, y);
            float right = getHeight(x + 1, y);
            float up = getHeight(x, y - 1);
            float down = getHeight(x, y + 1);
            
            float nx = left - right;
            float ny = 2.0f;
            float nz = down - up;
            
            float length = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (length > 0.0f) {
                nx /= length;
                ny /= length;
                nz /= length;
            }
            
            int index = getIndex(x, y) * 3;
            m_normalMap[index] = nx;
            m_normalMap[index + 1] = ny;
            m_normalMap[index + 2] = nz;
        }
    }
}

void ProceduralTerrain::getNormal(int x, int y, float& nx, float& ny, float& nz) const {
    if (!isValidCoordinate(x, y)) {
        nx = 0.0f;
        ny = 1.0f;
        nz = 0.0f;
        return;
    }
    
    int index = getIndex(x, y) * 3;
    nx = m_normalMap[index];
    ny = m_normalMap[index + 1];
    nz = m_normalMap[index + 2];
}

void ProceduralTerrain::applyHydraulicErosion(int iterations, float rainAmount, float evaporationRate, float sedimentCapacity) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> disX(0, m_width - 1);
    std::uniform_int_distribution<> disY(0, m_height - 1);
    
    for (int iter = 0; iter < iterations; ++iter) {
        int x = disX(gen);
        int y = disY(gen);
        
        float water = rainAmount;
        float sediment = 0.0f;
        float posX = static_cast<float>(x);
        float posY = static_cast<float>(y);
        
        // Simulate water droplet
        for (int step = 0; step < 100 && water > 0.01f; ++step) {
            float height = getInterpolatedHeight(posX, posY);
            
            // Find steepest descent
            float minHeight = height;
            float dirX = 0.0f;
            float dirY = 0.0f;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    
                    float nx = posX + dx;
                    float ny = posY + dy;
                    float nh = getInterpolatedHeight(nx, ny);
                    
                    if (nh < minHeight) {
                        minHeight = nh;
                        dirX = static_cast<float>(dx);
                        dirY = static_cast<float>(dy);
                    }
                }
            }
            
            // Deposit/erode
            float heightDiff = height - minHeight;
            if (heightDiff > 0) {
                float capacity = heightDiff * water * sedimentCapacity;
                if (sediment > capacity) {
                    float deposit = (sediment - capacity) * 0.5f;
                    setHeight(static_cast<int>(posX), static_cast<int>(posY), height + deposit);
                    sediment -= deposit;
                } else {
                    float erode = std::min(heightDiff, capacity - sediment) * 0.5f;
                    setHeight(static_cast<int>(posX), static_cast<int>(posY), height - erode);
                    sediment += erode;
                }
            }
            
            // Move droplet
            posX += dirX * 0.5f;
            posY += dirY * 0.5f;
            
            // Evaporate
            water *= (1.0f - evaporationRate);
            
            // Boundary check
            if (posX < 0 || posX >= m_width - 1 || posY < 0 || posY >= m_height - 1) {
                break;
            }
        }
    }
    
    calculateNormals();
}

void ProceduralTerrain::applyThermalErosion(int iterations, float talusAngle, float erosionRate) {
    float talusThreshold = std::tan(talusAngle * 3.14159f / 180.0f);
    
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> newHeightMap = m_heightMap;
        
        for (int y = 1; y < m_height - 1; ++y) {
            for (int x = 1; x < m_width - 1; ++x) {
                float height = getHeight(x, y);
                float maxDiff = 0.0f;
                
                // Find steepest neighbor
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        
                        float neighborHeight = getHeight(x + dx, y + dy);
                        float diff = height - neighborHeight;
                        maxDiff = std::max(maxDiff, diff);
                    }
                }
                
                // Apply erosion if slope exceeds talus angle
                if (maxDiff > talusThreshold) {
                    float erode = (maxDiff - talusThreshold) * erosionRate;
                    newHeightMap[getIndex(x, y)] -= erode;
                }
            }
        }
        
        m_heightMap = newHeightMap;
    }
    
    calculateNormals();
}

void ProceduralTerrain::applyWindErosion(int iterations, float windStrength, float windDirectionX, float windDirectionY) {
    // Normalize wind direction
    float length = std::sqrt(windDirectionX * windDirectionX + windDirectionY * windDirectionY);
    if (length > 0.0f) {
        windDirectionX /= length;
        windDirectionY /= length;
    }
    
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> newHeightMap = m_heightMap;
        
        for (int y = 1; y < m_height - 1; ++y) {
            for (int x = 1; x < m_width - 1; ++x) {
                float height = getHeight(x, y);
                
                // Sample in wind direction
                int windX = x + static_cast<int>(windDirectionX);
                int windY = y + static_cast<int>(windDirectionY);
                
                if (isValidCoordinate(windX, windY)) {
                    float windHeight = getHeight(windX, windY);
                    if (height > windHeight) {
                        float erode = (height - windHeight) * windStrength * 0.1f;
                        newHeightMap[getIndex(x, y)] -= erode;
                        newHeightMap[getIndex(windX, windY)] += erode * 0.5f;
                    }
                }
            }
        }
        
        m_heightMap = newHeightMap;
    }
    
    calculateNormals();
}

void ProceduralTerrain::smooth(int iterations, float strength) {
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> newHeightMap = m_heightMap;
        
        for (int y = 1; y < m_height - 1; ++y) {
            for (int x = 1; x < m_width - 1; ++x) {
                float sum = 0.0f;
                int count = 0;
                
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        sum += getHeight(x + dx, y + dy);
                        count++;
                    }
                }
                
                float average = sum / count;
                float current = getHeight(x, y);
                newHeightMap[getIndex(x, y)] = current + (average - current) * strength;
            }
        }
        
        m_heightMap = newHeightMap;
    }
    
    calculateNormals();
}

void ProceduralTerrain::gaussianBlur(float sigma) {
    // Simplified Gaussian blur
    int kernelSize = static_cast<int>(sigma * 3);
    std::vector<float> kernel(kernelSize);
    float sum = 0.0f;
    
    for (int i = 0; i < kernelSize; ++i) {
        float x = i - kernelSize / 2;
        kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += kernel[i];
    }
    
    for (int i = 0; i < kernelSize; ++i) {
        kernel[i] /= sum;
    }
    
    // Apply horizontal pass
    std::vector<float> temp = m_heightMap;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float value = 0.0f;
            for (int k = 0; k < kernelSize; ++k) {
                int sx = x + k - kernelSize / 2;
                if (sx >= 0 && sx < m_width) {
                    value += temp[y * m_width + sx] * kernel[k];
                }
            }
            m_heightMap[getIndex(x, y)] = value;
        }
    }
    
    // Apply vertical pass
    temp = m_heightMap;
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float value = 0.0f;
            for (int k = 0; k < kernelSize; ++k) {
                int sy = y + k - kernelSize / 2;
                if (sy >= 0 && sy < m_height) {
                    value += temp[sy * m_width + x] * kernel[k];
                }
            }
            m_heightMap[getIndex(x, y)] = value;
        }
    }
    
    calculateNormals();
}

void ProceduralTerrain::addMountainRange(float startX, float startY, float endX, float endY, float height, float width) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            // Calculate distance to line segment
            float dx = endX - startX;
            float dy = endY - startY;
            float t = ((x - startX) * dx + (y - startY) * dy) / (dx * dx + dy * dy);
            t = std::clamp(t, 0.0f, 1.0f);
            
            float closestX = startX + t * dx;
            float closestY = startY + t * dy;
            
            float dist = std::sqrt((x - closestX) * (x - closestX) + (y - closestY) * (y - closestY));
            
            if (dist < width) {
                float factor = 1.0f - (dist / width);
                float addition = height * factor * factor;
                setHeight(x, y, getHeight(x, y) + addition);
            }
        }
    }
    
    calculateNormals();
}

void ProceduralTerrain::addValley(float centerX, float centerY, float radius, float depth) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist < radius) {
                float factor = 1.0f - (dist / radius);
                float subtraction = depth * factor;
                setHeight(x, y, getHeight(x, y) - subtraction);
            }
        }
    }
    
    calculateNormals();
}

void ProceduralTerrain::addPlateau(float centerX, float centerY, float radius, float height) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            if (dist < radius) {
                float currentHeight = getHeight(x, y);
                float targetHeight = height;
                float factor = 1.0f - (dist / radius);
                setHeight(x, y, currentHeight + (targetHeight - currentHeight) * factor);
            }
        }
    }
    
    calculateNormals();
}

void ProceduralTerrain::addRiver(const std::vector<std::pair<float, float>>& path, float width, float depth) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
        float x1 = path[i].first;
        float y1 = path[i].second;
        float x2 = path[i + 1].first;
        float y2 = path[i + 1].second;
        
        // Carve river segment
        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                float dx = x2 - x1;
                float dy = y2 - y1;
                float t = ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy);
                t = std::clamp(t, 0.0f, 1.0f);
                
                float closestX = x1 + t * dx;
                float closestY = y1 + t * dy;
                
                float dist = std::sqrt((x - closestX) * (x - closestX) + (y - closestY) * (y - closestY));
                
                if (dist < width) {
                    float factor = 1.0f - (dist / width);
                    float carve = depth * factor;
                    setHeight(x, y, getHeight(x, y) - carve);
                }
            }
        }
    }
    
    calculateNormals();
}

void ProceduralTerrain::generateBiomeMap() {
    m_temperatureMap.resize(m_width * m_height);
    m_moistureMap.resize(m_width * m_height);
    
    // Generate temperature and moisture maps
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float temp = fbmNoise(x * 0.01f, y * 0.01f, 3, 0.5f, 2.0f);
            float moisture = fbmNoise(x * 0.01f + 1000.0f, y * 0.01f, 3, 0.5f, 2.0f);
            
            m_temperatureMap[getIndex(x, y)] = temp;
            m_moistureMap[getIndex(x, y)] = moisture;
        }
    }
}

int ProceduralTerrain::getBiome(int x, int y) const {
    if (!isValidCoordinate(x, y)) {
        return 0;
    }
    return m_biomeMap[getIndex(x, y)];
}

void ProceduralTerrain::addBiome(const BiomeType& biome) {
    m_biomes.push_back(biome);
}

void ProceduralTerrain::generateTextureWeights() {
    // Generate blending weights for multi-texturing
}

float ProceduralTerrain::getTextureWeight(int x, int y, int textureIndex) const {
    if (textureIndex >= static_cast<int>(m_textureWeights.size())) {
        return 0.0f;
    }
    
    if (!isValidCoordinate(x, y)) {
        return 0.0f;
    }
    
    return m_textureWeights[textureIndex][getIndex(x, y)];
}

void ProceduralTerrain::exportToHeightmap(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        for (float height : m_heightMap) {
            unsigned char value = static_cast<unsigned char>(std::clamp(height * 255.0f, 0.0f, 255.0f));
            file.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
        file.close();
    }
}

void ProceduralTerrain::exportToMesh(const std::string& filename) {
    // Export as OBJ or similar format
}

void ProceduralTerrain::exportToRAW(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(m_heightMap.data()), m_heightMap.size() * sizeof(float));
        file.close();
    }
}

void ProceduralTerrain::generatePerlinNoise(const TerrainSettings& settings) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            float nx = x / settings.scale;
            float ny = y / settings.scale;
            
            float height = fbmNoise(nx, ny, settings.octaves, settings.persistence, settings.lacunarity);
            height *= settings.heightMultiplier;
            
            m_heightMap[getIndex(x, y)] = height;
        }
    }
}

void ProceduralTerrain::generateSimplexNoise(const TerrainSettings& settings) {
    // Similar to Perlin but uses simplex noise
    generatePerlinNoise(settings);
}

void ProceduralTerrain::generateDiamondSquare(const TerrainSettings& settings) {
    // Diamond-square algorithm implementation
    generatePerlinNoise(settings);
}

void ProceduralTerrain::generateMidpointDisplacement(const TerrainSettings& settings) {
    // Midpoint displacement algorithm
    generatePerlinNoise(settings);
}

void ProceduralTerrain::generateVoronoi(const TerrainSettings& settings) {
    // Voronoi diagram based terrain
    generatePerlinNoise(settings);
}

float ProceduralTerrain::perlinNoise(float x, float y) const {
    // Simplified Perlin noise
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    
    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    
    float u = xf * xf * (3.0f - 2.0f * xf);
    float v = yf * yf * (3.0f - 2.0f * yf);
    
    return interpolate(
        interpolate(static_cast<float>(xi), static_cast<float>(xi + 1), u),
        interpolate(static_cast<float>(yi), static_cast<float>(yi + 1), u),
        v
    );
}

float ProceduralTerrain::simplexNoise(float x, float y) const {
    return perlinNoise(x, y);
}

float ProceduralTerrain::fbmNoise(float x, float y, int octaves, float persistence, float lacunarity) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        total += perlinNoise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}

float ProceduralTerrain::interpolate(float a, float b, float t) const {
    return a + t * (b - a);
}

float ProceduralTerrain::cosineInterpolate(float a, float b, float t) const {
    float ft = t * 3.14159f;
    float f = (1.0f - std::cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

bool ProceduralTerrain::isValidCoordinate(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

int ProceduralTerrain::getIndex(int x, int y) const {
    return y * m_width + x;
}

// TerrainGenerator implementation
TerrainGenerator::TerrainGenerator() {
}

TerrainGenerator& TerrainGenerator::getInstance() {
    static TerrainGenerator instance;
    return instance;
}

ProceduralTerrain* TerrainGenerator::generateFlatTerrain(int width, int height) {
    ProceduralTerrain* terrain = new ProceduralTerrain();
    TerrainSettings settings;
    settings.width = width;
    settings.height = height;
    settings.scale = 1.0f;
    settings.heightMultiplier = 0.0f;
    terrain->generate(settings);
    return terrain;
}

ProceduralTerrain* TerrainGenerator::generateHillyTerrain(int width, int height, int seed) {
    ProceduralTerrain* terrain = new ProceduralTerrain();
    TerrainSettings settings;
    settings.width = width;
    settings.height = height;
    settings.scale = 50.0f;
    settings.heightMultiplier = 5.0f;
    settings.octaves = 4;
    settings.persistence = 0.5f;
    settings.lacunarity = 2.0f;
    settings.seed = seed;
    terrain->generate(settings);
    return terrain;
}

ProceduralTerrain* TerrainGenerator::generateMountainousTerrain(int width, int height, int seed) {
    ProceduralTerrain* terrain = new ProceduralTerrain();
    TerrainSettings settings;
    settings.width = width;
    settings.height = height;
    settings.scale = 80.0f;
    settings.heightMultiplier = 20.0f;
    settings.octaves = 6;
    settings.persistence = 0.6f;
    settings.lacunarity = 2.5f;
    settings.seed = seed;
    terrain->generate(settings);
    return terrain;
}

ProceduralTerrain* TerrainGenerator::generateIslandTerrain(int width, int height, int seed) {
    ProceduralTerrain* terrain = generateHillyTerrain(width, height, seed);
    
    // Apply island mask
    float centerX = width * 0.5f;
    float centerY = height * 0.5f;
    float maxDist = std::min(width, height) * 0.5f;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float dx = x - centerX;
            float dy = y - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);
            float mask = std::max(0.0f, 1.0f - dist / maxDist);
            
            float currentHeight = terrain->getHeight(x, y);
            terrain->setHeight(x, y, currentHeight * mask * mask);
        }
    }
    
    terrain->calculateNormals();
    return terrain;
}

ProceduralTerrain* TerrainGenerator::generateCanyonTerrain(int width, int height, int seed) {
    ProceduralTerrain* terrain = generateMountainousTerrain(width, height, seed);
    terrain->applyHydraulicErosion(1000, 0.1f, 0.05f, 0.5f);
    return terrain;
}

ProceduralTerrain* TerrainGenerator::generateCustomTerrain(const TerrainSettings& settings, TerrainAlgorithm algorithm) {
    ProceduralTerrain* terrain = new ProceduralTerrain();
    terrain->generateWithAlgorithm(algorithm, settings);
    return terrain;
}

void TerrainGenerator::destroyTerrain(ProceduralTerrain* terrain) {
    delete terrain;
}

} // namespace Engine
