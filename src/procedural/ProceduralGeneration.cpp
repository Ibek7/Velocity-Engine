#include "procedural/ProceduralGeneration.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Procedural {

// PerlinNoise implementation
PerlinNoise::PerlinNoise(unsigned int seed) {
    permutation.resize(256);
    for (int i = 0; i < 256; ++i) {
        permutation[i] = i;
    }
    
    std::mt19937 rng(seed);
    std::shuffle(permutation.begin(), permutation.end(), rng);
    
    permutation.insert(permutation.end(), permutation.begin(), permutation.end());
}

PerlinNoise::~PerlinNoise() {}

float PerlinNoise::noise(float x) const {
    return noise(x, 0.0f, 0.0f);
}

float PerlinNoise::noise(float x, float y) const {
    return noise(x, y, 0.0f);
}

float PerlinNoise::noise(float x, float y, float z) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    
    float u = fade(x);
    float v = fade(y);
    float w = fade(z);
    
    int A = permutation[X] + Y;
    int AA = permutation[A] + Z;
    int AB = permutation[A + 1] + Z;
    int B = permutation[X + 1] + Y;
    int BA = permutation[B] + Z;
    int BB = permutation[B + 1] + Z;
    
    return lerp(w,
        lerp(v,
            lerp(u, grad(permutation[AA], x, y, z),
                    grad(permutation[BA], x - 1, y, z)),
            lerp(u, grad(permutation[AB], x, y - 1, z),
                    grad(permutation[BB], x - 1, y - 1, z))),
        lerp(v,
            lerp(u, grad(permutation[AA + 1], x, y, z - 1),
                    grad(permutation[BA + 1], x - 1, y, z - 1)),
            lerp(u, grad(permutation[AB + 1], x, y - 1, z - 1),
                    grad(permutation[BB + 1], x - 1, y - 1, z - 1))));
}

float PerlinNoise::octaveNoise(float x, int octaves, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        total += noise(x * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

float PerlinNoise::octaveNoise(float x, float y, int octaves, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; ++i) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    
    return total / maxValue;
}

float PerlinNoise::fade(float t) const {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float t, float a, float b) const {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y, float z) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// ProceduralGrid implementation
ProceduralGrid::ProceduralGrid(int width, int height)
    : width(width), height(height), tiles(width * height, TileType::Empty) {}

ProceduralGrid::~ProceduralGrid() {}

void ProceduralGrid::setTile(int x, int y, TileType type) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y * width + x] = type;
    }
}

TileType ProceduralGrid::getTile(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return tiles[y * width + x];
    }
    return TileType::Empty;
}

int ProceduralGrid::getWidth() const { return width; }
int ProceduralGrid::getHeight() const { return height; }

void ProceduralGrid::fill(TileType type) {
    std::fill(tiles.begin(), tiles.end(), type);
}

void ProceduralGrid::clear() {
    fill(TileType::Empty);
}

// Room implementation
bool Room::intersects(const Room& other) const {
    return !(x + width < other.x || other.x + other.width < x ||
             y + height < other.y || other.y + other.height < y);
}

// DungeonGenerator implementation
DungeonGenerator::DungeonGenerator(int width, int height, unsigned int seed)
    : grid(width, height), rng(seed),
      minRooms(5), maxRooms(10),
      minRoomSize(5), maxRoomSize(10) {}

DungeonGenerator::~DungeonGenerator() {}

void DungeonGenerator::generate() {
    grid.fill(TileType::Wall);
    rooms.clear();
    
    generateRooms();
    generateCorridors();
}

ProceduralGrid& DungeonGenerator::getGrid() {
    return grid;
}

void DungeonGenerator::setRoomCount(int min, int max) {
    minRooms = min;
    maxRooms = max;
}

void DungeonGenerator::setRoomSize(int min, int max) {
    minRoomSize = min;
    maxRoomSize = max;
}

void DungeonGenerator::generateRooms() {
    std::uniform_int_distribution<int> roomCountDist(minRooms, maxRooms);
    int roomCount = roomCountDist(rng);
    
    for (int i = 0; i < roomCount * 10 && static_cast<int>(rooms.size()) < roomCount; ++i) {
        std::uniform_int_distribution<int> sizeDist(minRoomSize, maxRoomSize);
        Room room;
        room.width = sizeDist(rng);
        room.height = sizeDist(rng);
        
        std::uniform_int_distribution<int> xDist(1, grid.getWidth() - room.width - 1);
        std::uniform_int_distribution<int> yDist(1, grid.getHeight() - room.height - 1);
        room.x = xDist(rng);
        room.y = yDist(rng);
        
        bool intersects = false;
        for (const auto& existingRoom : rooms) {
            if (room.intersects(existingRoom)) {
                intersects = true;
                break;
            }
        }
        
        if (!intersects) {
            createRoom(room);
            rooms.push_back(room);
        }
    }
}

void DungeonGenerator::generateCorridors() {
    for (size_t i = 1; i < rooms.size(); ++i) {
        const Room& prev = rooms[i - 1];
        const Room& curr = rooms[i];
        
        int prevCenterX = prev.getCenterX();
        int prevCenterY = prev.getCenterY();
        int currCenterX = curr.getCenterX();
        int currCenterY = curr.getCenterY();
        
        if (rng() % 2 == 0) {
            createHorizontalCorridor(prevCenterX, currCenterX, prevCenterY);
            createVerticalCorridor(prevCenterY, currCenterY, currCenterX);
        } else {
            createVerticalCorridor(prevCenterY, currCenterY, prevCenterX);
            createHorizontalCorridor(prevCenterX, currCenterX, currCenterY);
        }
    }
}

void DungeonGenerator::createRoom(const Room& room) {
    for (int y = room.y; y < room.y + room.height; ++y) {
        for (int x = room.x; x < room.x + room.width; ++x) {
            grid.setTile(x, y, TileType::Floor);
        }
    }
}

void DungeonGenerator::createHorizontalCorridor(int x1, int x2, int y) {
    int minX = std::min(x1, x2);
    int maxX = std::max(x1, x2);
    for (int x = minX; x <= maxX; ++x) {
        grid.setTile(x, y, TileType::Floor);
    }
}

void DungeonGenerator::createVerticalCorridor(int y1, int y2, int x) {
    int minY = std::min(y1, y2);
    int maxY = std::max(y1, y2);
    for (int y = minY; y <= maxY; ++y) {
        grid.setTile(x, y, TileType::Floor);
    }
}

// CaveGenerator implementation
CaveGenerator::CaveGenerator(int width, int height, unsigned int seed)
    : grid(width, height), rng(seed),
      fillProbability(0.45f), iterations(4),
      birthLimit(4), deathLimit(3) {}

CaveGenerator::~CaveGenerator() {}

void CaveGenerator::generate() {
    randomFill();
    for (int i = 0; i < iterations; ++i) {
        smoothStep();
    }
}

ProceduralGrid& CaveGenerator::getGrid() {
    return grid;
}

void CaveGenerator::setFillProbability(float probability) {
    fillProbability = probability;
}

void CaveGenerator::setIterations(int iter) {
    iterations = iter;
}

void CaveGenerator::setSmoothingRules(int birth, int death) {
    birthLimit = birth;
    deathLimit = death;
}

void CaveGenerator::randomFill() {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            if (x == 0 || y == 0 || x == grid.getWidth() - 1 || y == grid.getHeight() - 1) {
                grid.setTile(x, y, TileType::Wall);
            } else {
                grid.setTile(x, y, dist(rng) < fillProbability ? TileType::Wall : TileType::Floor);
            }
        }
    }
}

void CaveGenerator::smoothStep() {
    ProceduralGrid newGrid(grid.getWidth(), grid.getHeight());
    
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            int neighbors = countNeighbors(x, y);
            
            if (grid.getTile(x, y) == TileType::Wall) {
                newGrid.setTile(x, y, neighbors >= deathLimit ? TileType::Wall : TileType::Floor);
            } else {
                newGrid.setTile(x, y, neighbors >= birthLimit ? TileType::Wall : TileType::Floor);
            }
        }
    }
    
    grid = newGrid;
}

int CaveGenerator::countNeighbors(int x, int y) const {
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (nx < 0 || ny < 0 || nx >= grid.getWidth() || ny >= grid.getHeight() ||
                grid.getTile(nx, ny) == TileType::Wall) {
                ++count;
            }
        }
    }
    return count;
}

// MazeGenerator implementation
MazeGenerator::MazeGenerator(int width, int height, unsigned int seed)
    : grid(width, height), rng(seed) {
    visited.resize(height, std::vector<bool>(width, false));
}

MazeGenerator::~MazeGenerator() {}

void MazeGenerator::generate() {
    grid.fill(TileType::Wall);
    recursiveBacktrack(0, 0);
}

ProceduralGrid& MazeGenerator::getGrid() {
    return grid;
}

void MazeGenerator::recursiveBacktrack(int x, int y) {
    visited[y][x] = true;
    grid.setTile(x, y, TileType::Floor);
    
    auto neighbors = getUnvisitedNeighbors(x, y);
    std::shuffle(neighbors.begin(), neighbors.end(), rng);
    
    for (const auto& neighbor : neighbors) {
        int nx = neighbor.first;
        int ny = neighbor.second;
        
        if (!visited[ny][nx]) {
            removeWallBetween(x, y, nx, ny);
            recursiveBacktrack(nx, ny);
        }
    }
}

std::vector<std::pair<int, int>> MazeGenerator::getUnvisitedNeighbors(int x, int y) {
    std::vector<std::pair<int, int>> neighbors;
    
    if (x > 1 && !visited[y][x - 2]) neighbors.push_back({x - 2, y});
    if (x < grid.getWidth() - 2 && !visited[y][x + 2]) neighbors.push_back({x + 2, y});
    if (y > 1 && !visited[y - 2][x]) neighbors.push_back({x, y - 2});
    if (y < grid.getHeight() - 2 && !visited[y + 2][x]) neighbors.push_back({x, y + 2});
    
    return neighbors;
}

void MazeGenerator::removeWallBetween(int x1, int y1, int x2, int y2) {
    int wallX = (x1 + x2) / 2;
    int wallY = (y1 + y2) / 2;
    grid.setTile(wallX, wallY, TileType::Floor);
}

// HeightMap implementation
HeightMap::HeightMap(int width, int height)
    : width(width), height(height), heights(width * height, 0.0f) {}

HeightMap::~HeightMap() {}

void HeightMap::generate(const PerlinNoise& noise, float scale, int octaves) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float nx = static_cast<float>(x) / width * scale;
            float ny = static_cast<float>(y) / height * scale;
            float value = noise.octaveNoise(nx, ny, octaves);
            setHeight(x, y, value);
        }
    }
}

float HeightMap::getHeight(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return heights[y * width + x];
    }
    return 0.0f;
}

void HeightMap::setHeight(int x, int y, float height) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        heights[y * width + x] = height;
    }
}

int HeightMap::getWidth() const { return width; }
int HeightMap::getHeight() const { return height; }

void HeightMap::normalize() {
    float minHeight = *std::min_element(heights.begin(), heights.end());
    float maxHeight = *std::max_element(heights.begin(), heights.end());
    float range = maxHeight - minHeight;
    
    if (range > 0.0f) {
        for (auto& h : heights) {
            h = (h - minHeight) / range;
        }
    }
}

void HeightMap::smooth(int iterations) {
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<float> newHeights = heights;
        
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                float sum = 0.0f;
                int count = 0;
                
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        sum += getHeight(x + dx, y + dy);
                        ++count;
                    }
                }
                
                newHeights[y * width + x] = sum / count;
            }
        }
        
        heights = newHeights;
    }
}

// TerrainGenerator implementation
TerrainGenerator::TerrainGenerator(int width, int height, unsigned int seed)
    : grid(width, height), heightMap(width, height), noise(seed),
      seaLevel(0.3f), mountainLevel(0.7f) {}

TerrainGenerator::~TerrainGenerator() {}

void TerrainGenerator::generate() {
    heightMap.generate(noise, 4.0f, 6);
    heightMap.normalize();
    heightMapToTiles();
}

ProceduralGrid& TerrainGenerator::getGrid() {
    return grid;
}

const HeightMap& TerrainGenerator::getHeightMap() const {
    return heightMap;
}

void TerrainGenerator::setSeaLevel(float level) {
    seaLevel = level;
}

void TerrainGenerator::setMountainLevel(float level) {
    mountainLevel = level;
}

void TerrainGenerator::heightMapToTiles() {
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            float h = heightMap.getHeight(x, y);
            
            if (h < seaLevel) {
                grid.setTile(x, y, TileType::Water);
            } else if (h < seaLevel + 0.1f) {
                grid.setTile(x, y, TileType::Grass);
            } else if (h < mountainLevel) {
                grid.setTile(x, y, TileType::Floor);
            } else {
                grid.setTile(x, y, TileType::Stone);
            }
        }
    }
}

// BSPNode implementation
BSPNode::BSPNode(int x, int y, int width, int height)
    : x(x), y(y), width(width), height(height), room(nullptr) {}

BSPNode::~BSPNode() {
    delete room;
}

void BSPNode::split(int minSize) {
    if (width < minSize * 2 || height < minSize * 2) {
        return;
    }
    
    bool splitH = (rand() % 2) == 0;
    
    if (width > height && width / height >= 1.25) {
        splitH = false;
    } else if (height > width && height / width >= 1.25) {
        splitH = true;
    }
    
    int max = (splitH ? height : width) - minSize;
    if (max <= minSize) return;
    
    int split = minSize + rand() % (max - minSize);
    
    if (splitH) {
        left = std::make_unique<BSPNode>(x, y, width, split);
        right = std::make_unique<BSPNode>(x, y + split, width, height - split);
    } else {
        left = std::make_unique<BSPNode>(x, y, split, height);
        right = std::make_unique<BSPNode>(x + split, y, width - split, height);
    }
    
    left->split(minSize);
    right->split(minSize);
}

void BSPNode::createRooms() {
    if (left || right) {
        if (left) left->createRooms();
        if (right) right->createRooms();
    } else {
        int roomWidth = width / 2 + rand() % (width / 2);
        int roomHeight = height / 2 + rand() % (height / 2);
        int roomX = x + rand() % (width - roomWidth);
        int roomY = y + rand() % (height - roomHeight);
        
        room = new Room{roomX, roomY, roomWidth, roomHeight};
    }
}

std::vector<Room> BSPNode::getRooms() const {
    std::vector<Room> rooms;
    collectRooms(rooms);
    return rooms;
}

bool BSPNode::isLeaf() const {
    return !left && !right;
}

void BSPNode::collectRooms(std::vector<Room>& rooms) const {
    if (room) {
        rooms.push_back(*room);
    }
    if (left) left->collectRooms(rooms);
    if (right) right->collectRooms(rooms);
}

// BSPDungeonGenerator implementation
BSPDungeonGenerator::BSPDungeonGenerator(int width, int height, unsigned int seed)
    : grid(width, height), rng(seed), minRoomSize(6) {
    srand(seed);
}

BSPDungeonGenerator::~BSPDungeonGenerator() {}

void BSPDungeonGenerator::generate() {
    grid.fill(TileType::Wall);
    root = std::make_unique<BSPNode>(0, 0, grid.getWidth(), grid.getHeight());
    root->split(minRoomSize);
    root->createRooms();
    createRoomsAndCorridors();
}

ProceduralGrid& BSPDungeonGenerator::getGrid() {
    return grid;
}

void BSPDungeonGenerator::setMinRoomSize(int size) {
    minRoomSize = size;
}

void BSPDungeonGenerator::createRoomsAndCorridors() {
    auto rooms = root->getRooms();
    
    for (const auto& room : rooms) {
        for (int y = room.y; y < room.y + room.height; ++y) {
            for (int x = room.x; x < room.x + room.width; ++x) {
                grid.setTile(x, y, TileType::Floor);
            }
        }
    }
    
    for (size_t i = 1; i < rooms.size(); ++i) {
        connectRooms(rooms[i - 1], rooms[i]);
    }
}

void BSPDungeonGenerator::connectRooms(const Room& room1, const Room& room2) {
    int x1 = room1.getCenterX();
    int y1 = room1.getCenterY();
    int x2 = room2.getCenterX();
    int y2 = room2.getCenterY();
    
    for (int x = std::min(x1, x2); x <= std::max(x1, x2); ++x) {
        grid.setTile(x, y1, TileType::Floor);
    }
    
    for (int y = std::min(y1, y2); y <= std::max(y1, y2); ++y) {
        grid.setTile(x2, y, TileType::Floor);
    }
}

// WaveFunctionCollapse implementation
WaveFunctionCollapse::WaveFunctionCollapse(int width, int height)
    : grid(width, height) {}

WaveFunctionCollapse::~WaveFunctionCollapse() {}

void WaveFunctionCollapse::addRule(TileType tile, const std::vector<TileType>& allowedNeighbors) {
    rules[tile] = allowedNeighbors;
}

void WaveFunctionCollapse::generate(unsigned int seed) {
    std::mt19937 rng(seed);
    
    cells.resize(grid.getHeight(), std::vector<Cell>(grid.getWidth()));
    
    for (auto& row : cells) {
        for (auto& cell : row) {
            for (const auto& rule : rules) {
                cell.possibleTypes.push_back(rule.first);
            }
            cell.collapsed = false;
        }
    }
    
    while (true) {
        auto [x, y] = findLowestEntropy();
        if (x == -1) break;
        
        std::uniform_int_distribution<size_t> dist(0, cells[y][x].possibleTypes.size() - 1);
        TileType chosenType = cells[y][x].possibleTypes[dist(rng)];
        
        collapse(x, y, chosenType);
        propagate(x, y);
    }
}

ProceduralGrid& WaveFunctionCollapse::getGrid() {
    return grid;
}

void WaveFunctionCollapse::collapse(int x, int y, TileType type) {
    cells[y][x].possibleTypes = {type};
    cells[y][x].collapsed = true;
    grid.setTile(x, y, type);
}

void WaveFunctionCollapse::propagate(int x, int y) {
    // Simplified propagation
    (void)x;
    (void)y;
}

std::pair<int, int> WaveFunctionCollapse::findLowestEntropy() {
    int minEntropy = INT_MAX;
    int minX = -1, minY = -1;
    
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            if (!cells[y][x].collapsed) {
                int entropy = static_cast<int>(cells[y][x].possibleTypes.size());
                if (entropy < minEntropy) {
                    minEntropy = entropy;
                    minX = x;
                    minY = y;
                }
            }
        }
    }
    
    return {minX, minY};
}

} // namespace Procedural
} // namespace JJM
