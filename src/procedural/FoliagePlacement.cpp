#include "procedural/FoliagePlacement.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace Engine {

FoliagePlacement::FoliagePlacement()
    : m_windStrength(0.5f)
    , m_lodDistance(100.0f)
{
}

FoliagePlacement& FoliagePlacement::getInstance() {
    static FoliagePlacement instance;
    return instance;
}

void FoliagePlacement::placeRandomly(FoliageType type, const std::string& modelPath,
                                    float minX, float minZ, float maxX, float maxZ,
                                    int count, float minScale, float maxScale) {
    for (int i = 0; i < count; ++i) {
        float x = getRandomFloat(minX, maxX);
        float z = getRandomFloat(minZ, maxZ);
        float y = getTerrainHeight(x, z);
        float rotation = getRandomFloat(0.0f, 360.0f);
        float scale = getRandomFloat(minScale, maxScale);
        
        FoliageInstance instance;
        instance.position[0] = x;
        instance.position[1] = y;
        instance.position[2] = z;
        instance.rotation = rotation;
        instance.scale = scale;
        instance.type = type;
        instance.modelPath = modelPath;
        
        m_instances.push_back(instance);
    }
}

void FoliagePlacement::placeOnTerrain(FoliageType type, const std::string& modelPath,
                                     float centerX, float centerZ, float radius,
                                     float density, float minScale, float maxScale) {
    // Calculate number of instances based on density
    float area = 3.14159f * radius * radius;
    int count = static_cast<int>(area * density);
    
    for (int i = 0; i < count; ++i) {
        // Random point in circle
        float angle = getRandomFloat(0.0f, 6.28318f);
        float r = std::sqrt(getRandomFloat(0.0f, 1.0f)) * radius;
        
        float x = centerX + r * std::cos(angle);
        float z = centerZ + r * std::sin(angle);
        float y = getTerrainHeight(x, z);
        float rotation = getRandomFloat(0.0f, 360.0f);
        float scale = getRandomFloat(minScale, maxScale);
        
        FoliageInstance instance;
        instance.position[0] = x;
        instance.position[1] = y;
        instance.position[2] = z;
        instance.rotation = rotation;
        instance.scale = scale;
        instance.type = type;
        instance.modelPath = modelPath;
        
        m_instances.push_back(instance);
    }
}

void FoliagePlacement::placeManual(FoliageType type, const std::string& modelPath,
                                  float x, float y, float z, float rotation, float scale) {
    FoliageInstance instance;
    instance.position[0] = x;
    instance.position[1] = y;
    instance.position[2] = z;
    instance.rotation = rotation;
    instance.scale = scale;
    instance.type = type;
    instance.modelPath = modelPath;
    
    m_instances.push_back(instance);
}

void FoliagePlacement::removeInRadius(float x, float z, float radius) {
    float radiusSq = radius * radius;
    
    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(),
            [x, z, radiusSq](const FoliageInstance& instance) {
                float dx = instance.position[0] - x;
                float dz = instance.position[2] - z;
                return (dx*dx + dz*dz) <= radiusSq;
            }),
        m_instances.end()
    );
}

void FoliagePlacement::removeByType(FoliageType type) {
    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(),
            [type](const FoliageInstance& instance) {
                return instance.type == type;
            }),
        m_instances.end()
    );
}

void FoliagePlacement::clearAll() {
    m_instances.clear();
}

void FoliagePlacement::getFoliageInRadius(float x, float z, float radius, std::vector<FoliageInstance*>& results) {
    results.clear();
    float radiusSq = radius * radius;
    
    for (auto& instance : m_instances) {
        float dx = instance.position[0] - x;
        float dz = instance.position[2] - z;
        
        if (dx*dx + dz*dz <= radiusSq) {
            results.push_back(&instance);
        }
    }
}

int FoliagePlacement::getFoliageCountByType(FoliageType type) const {
    int count = 0;
    for (const auto& instance : m_instances) {
        if (instance.type == type) {
            ++count;
        }
    }
    return count;
}

void FoliagePlacement::render() {
    // TODO: Actual rendering with instancing
    for (const auto& instance : m_instances) {
        (void)instance; // Placeholder
    }
}

void FoliagePlacement::updateLOD(float cameraX, float cameraY, float cameraZ) {
    // TODO: Calculate LOD levels based on distance from camera
    (void)cameraX;
    (void)cameraY;
    (void)cameraZ;
}

float FoliagePlacement::getRandomFloat(float min, float max) const {
    float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    return min + random * (max - min);
}

float FoliagePlacement::getTerrainHeight(float x, float z) const {
    // TODO: Query actual terrain height
    (void)x;
    (void)z;
    return 0.0f;
}

} // namespace Engine
