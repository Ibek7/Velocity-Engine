#include "procedural/TerrainGenerator.h"
#include <cmath>
#include <random>

namespace JJM {
namespace Procedural {

TerrainGenerator::TerrainGenerator(int s)
    : seed(s), octaves(4), persistence(0.5f), scale(50.0f), heightMultiplier(10.0f) {
}

TerrainGenerator::~TerrainGenerator() {
}

TerrainChunk TerrainGenerator::generateChunk(int chunkX, int chunkZ, int width, int depth) {
    TerrainChunk chunk;
    chunk.x = chunkX;
    chunk.z = chunkZ;
    chunk.width = width;
    chunk.depth = depth;
    
    chunk.heightMap.resize(width * depth);
    chunk.typeMap.resize(width * depth);
    
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            float worldX = (chunkX * width + x) / scale;
            float worldZ = (chunkZ * depth + z) / scale;
            
            float height = perlinNoise(worldX, worldZ) * heightMultiplier;
            
            int index = z * width + x;
            chunk.heightMap[index] = height;
            chunk.typeMap[index] = getTerrainType(height);
        }
    }
    
    return chunk;
}

void TerrainGenerator::setSeed(int s) {
    seed = s;
}

void TerrainGenerator::setOctaves(int o) {
    octaves = o;
}

void TerrainGenerator::setPersistence(float p) {
    persistence = p;
}

void TerrainGenerator::setScale(float s) {
    scale = s;
}

void TerrainGenerator::setHeightMultiplier(float m) {
    heightMultiplier = m;
}

float TerrainGenerator::getHeight(float x, float z) const {
    return perlinNoise(x / scale, z / scale) * heightMultiplier;
}

TerrainType TerrainGenerator::getTerrainType(float height) const {
    if (height < 0.0f) return TerrainType::Water;
    if (height < 2.0f) return TerrainType::Plains;
    if (height < 5.0f) return TerrainType::Hills;
    if (height < 8.0f) return TerrainType::Mountains;
    return TerrainType::Desert;
}

float TerrainGenerator::noise(float x, float z) const {
    int n = static_cast<int>(x + z * 57 + seed);
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float TerrainGenerator::perlinNoise(float x, float z) const {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        total += noise(x * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

float TerrainGenerator::interpolate(float a, float b, float t) const {
    float ft = t * 3.1415927f;
    float f = (1.0f - std::cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

} // namespace Procedural
} // namespace JJM
