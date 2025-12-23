#ifndef JJM_TERRAIN_GENERATOR_H
#define JJM_TERRAIN_GENERATOR_H

#include "math/Vector2D.h"
#include <vector>
#include <functional>

namespace JJM {
namespace Procedural {

enum class TerrainType { Plains, Hills, Mountains, Water, Desert };

struct TerrainChunk {
    int x, z;
    std::vector<float> heightMap;
    std::vector<TerrainType> typeMap;
    int width, depth;
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
