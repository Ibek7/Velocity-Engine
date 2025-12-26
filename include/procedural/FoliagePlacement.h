#pragma once

#include <string>
#include <vector>
#include <memory>

// Foliage placement system for vegetation
namespace Engine {

enum class FoliageType {
    Grass,
    Tree,
    Bush,
    Flower,
    Rock,
    Custom
};

struct FoliageInstance {
    float position[3];
    float rotation;
    float scale;
    FoliageType type;
    std::string modelPath;
};

class FoliagePlacement {
public:
    static FoliagePlacement& getInstance();

    // Placement
    void placeRandomly(FoliageType type, const std::string& modelPath,
                      float minX, float minZ, float maxX, float maxZ,
                      int count, float minScale = 0.8f, float maxScale = 1.2f);
    
    void placeOnTerrain(FoliageType type, const std::string& modelPath,
                       float centerX, float centerZ, float radius,
                       float density, float minScale = 0.8f, float maxScale = 1.2f);
    
    void placeManual(FoliageType type, const std::string& modelPath,
                    float x, float y, float z, float rotation, float scale);
    
    // Removal
    void removeInRadius(float x, float z, float radius);
    void removeByType(FoliageType type);
    void clearAll();
    
    // Query
    void getFoliageInRadius(float x, float z, float radius, std::vector<FoliageInstance*>& results);
    int getFoliageCount() const { return m_instances.size(); }
    int getFoliageCountByType(FoliageType type) const;
    
    // Rendering
    void render();
    void updateLOD(float cameraX, float cameraY, float cameraZ);
    
    // Configuration
    void setWindStrength(float strength) { m_windStrength = strength; }
    void setLODDistance(float distance) { m_lodDistance = distance; }

private:
    FoliagePlacement();
    FoliagePlacement(const FoliagePlacement&) = delete;
    FoliagePlacement& operator=(const FoliagePlacement&) = delete;

    float getRandomFloat(float min, float max) const;
    float getTerrainHeight(float x, float z) const;

    std::vector<FoliageInstance> m_instances;
    float m_windStrength;
    float m_lodDistance;
};

} // namespace Engine
