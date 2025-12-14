#ifndef JJM_PROCEDURAL_GENERATION_H
#define JJM_PROCEDURAL_GENERATION_H

#include <vector>
#include <memory>
#include <functional>
#include <random>
#include "math/Vector2D.h"

namespace JJM {
namespace Procedural {

/**
 * @brief Perlin noise generator for smooth random values
 */
class PerlinNoise {
public:
    PerlinNoise(unsigned int seed = 0);
    ~PerlinNoise();

    float noise(float x) const;
    float noise(float x, float y) const;
    float noise(float x, float y, float z) const;

    float octaveNoise(float x, int octaves, float persistence = 0.5f) const;
    float octaveNoise(float x, float y, int octaves, float persistence = 0.5f) const;

private:
    std::vector<int> permutation;
    float fade(float t) const;
    float lerp(float t, float a, float b) const;
    float grad(int hash, float x, float y, float z) const;
};

/**
 * @brief Tile types for procedural map generation
 */
enum class TileType {
    Empty = 0,
    Floor,
    Wall,
    Water,
    Grass,
    Stone,
    Door,
    Chest,
    Spawn
};

/**
 * @brief 2D grid for procedural map generation
 */
class ProceduralGrid {
public:
    ProceduralGrid(int width, int height);
    ~ProceduralGrid();

    void setTile(int x, int y, TileType type);
    TileType getTile(int x, int y) const;
    
    int getWidth() const;
    int getHeight() const;
    
    void fill(TileType type);
    void clear();

private:
    int width;
    int height;
    std::vector<TileType> tiles;
};

/**
 * @brief Rectangle for room generation
 */
struct Room {
    int x, y;
    int width, height;

    int getCenterX() const { return x + width / 2; }
    int getCenterY() const { return y + height / 2; }
    bool intersects(const Room& other) const;
};

/**
 * @brief Dungeon generator using rooms and corridors
 */
class DungeonGenerator {
public:
    DungeonGenerator(int width, int height, unsigned int seed = 0);
    ~DungeonGenerator();

    void generate();
    ProceduralGrid& getGrid();

    void setRoomCount(int minRooms, int maxRooms);
    void setRoomSize(int minSize, int maxSize);

private:
    ProceduralGrid grid;
    std::vector<Room> rooms;
    std::mt19937 rng;
    
    int minRooms, maxRooms;
    int minRoomSize, maxRoomSize;

    void generateRooms();
    void generateCorridors();
    void createRoom(const Room& room);
    void createHorizontalCorridor(int x1, int x2, int y);
    void createVerticalCorridor(int y1, int y2, int x);
};

/**
 * @brief Cave generator using cellular automata
 */
class CaveGenerator {
public:
    CaveGenerator(int width, int height, unsigned int seed = 0);
    ~CaveGenerator();

    void generate();
    ProceduralGrid& getGrid();

    void setFillProbability(float probability);
    void setIterations(int iterations);
    void setSmoothingRules(int birthLimit, int deathLimit);

private:
    ProceduralGrid grid;
    std::mt19937 rng;
    
    float fillProbability;
    int iterations;
    int birthLimit;
    int deathLimit;

    void randomFill();
    void smoothStep();
    int countNeighbors(int x, int y) const;
};

/**
 * @brief Maze generator using recursive backtracking
 */
class MazeGenerator {
public:
    MazeGenerator(int width, int height, unsigned int seed = 0);
    ~MazeGenerator();

    void generate();
    ProceduralGrid& getGrid();

private:
    ProceduralGrid grid;
    std::mt19937 rng;
    std::vector<std::vector<bool>> visited;

    void recursiveBacktrack(int x, int y);
    std::vector<std::pair<int, int>> getUnvisitedNeighbors(int x, int y);
    void removeWallBetween(int x1, int y1, int x2, int y2);
};

/**
 * @brief Height map for terrain generation
 */
class HeightMap {
public:
    HeightMap(int width, int height);
    ~HeightMap();

    void generate(const PerlinNoise& noise, float scale, int octaves);
    
    float getHeight(int x, int y) const;
    void setHeight(int x, int y, float height);
    
    int getWidth() const;
    int getHeight() const;
    
    void normalize();
    void smooth(int iterations);

private:
    int width;
    int height;
    std::vector<float> heights;
};

/**
 * @brief Terrain generator using height maps
 */
class TerrainGenerator {
public:
    TerrainGenerator(int width, int height, unsigned int seed = 0);
    ~TerrainGenerator();

    void generate();
    ProceduralGrid& getGrid();
    const HeightMap& getHeightMap() const;

    void setSeaLevel(float level);
    void setMountainLevel(float level);

private:
    ProceduralGrid grid;
    HeightMap heightMap;
    PerlinNoise noise;
    
    float seaLevel;
    float mountainLevel;

    void heightMapToTiles();
};

/**
 * @brief BSP (Binary Space Partitioning) tree for dungeon generation
 */
class BSPNode {
public:
    BSPNode(int x, int y, int width, int height);
    ~BSPNode();

    void split(int minSize);
    void createRooms();
    
    std::vector<Room> getRooms() const;

private:
    int x, y, width, height;
    std::unique_ptr<BSPNode> left;
    std::unique_ptr<BSPNode> right;
    Room* room;

    bool isLeaf() const;
    void collectRooms(std::vector<Room>& rooms) const;
};

/**
 * @brief BSP dungeon generator
 */
class BSPDungeonGenerator {
public:
    BSPDungeonGenerator(int width, int height, unsigned int seed = 0);
    ~BSPDungeonGenerator();

    void generate();
    ProceduralGrid& getGrid();

    void setMinRoomSize(int size);

private:
    ProceduralGrid grid;
    std::unique_ptr<BSPNode> root;
    std::mt19937 rng;
    
    int minRoomSize;

    void createRoomsAndCorridors();
    void connectRooms(const Room& room1, const Room& room2);
};

/**
 * @brief Wave Function Collapse generator
 */
class WaveFunctionCollapse {
public:
    WaveFunctionCollapse(int width, int height);
    ~WaveFunctionCollapse();

    void addRule(TileType tile, const std::vector<TileType>& allowedNeighbors);
    void generate(unsigned int seed = 0);
    
    ProceduralGrid& getGrid();

private:
    ProceduralGrid grid;
    std::unordered_map<TileType, std::vector<TileType>> rules;
    
    struct Cell {
        std::vector<TileType> possibleTypes;
        bool collapsed;
    };
    
    std::vector<std::vector<Cell>> cells;

    void collapse(int x, int y, TileType type);
    void propagate(int x, int y);
    std::pair<int, int> findLowestEntropy();
};

} // namespace Procedural
} // namespace JJM

#endif // JJM_PROCEDURAL_GENERATION_H
