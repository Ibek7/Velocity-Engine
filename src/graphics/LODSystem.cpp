#include "graphics/LODSystem.h"
#include <cmath>
#include <algorithm>

namespace Engine {

LODMesh::LODMesh()
    : m_currentLOD(0)
    , m_previousLOD(0)
    , m_transitionSpeed(2.0f)
    , m_crossFadeEnabled(true)
    , m_fadeRange(5.0f)
    , m_lodBias(0.0f)
{
    m_transition.fadeStartDistance = 0.0f;
    m_transition.fadeEndDistance = 0.0f;
    m_transition.currentFade = 0.0f;
    m_transition.isFading = false;
}

LODMesh::~LODMesh() {
}

void LODMesh::addLODLevel(int meshIndex, float distance, int triangleCount) {
    LODLevel level;
    level.meshIndex = meshIndex;
    level.distance = distance;
    level.triangleCount = triangleCount;
    level.screenCoverage = 0.0f; // Can be calculated based on distance
    
    m_levels.push_back(level);
    sortLevels();
}

void LODMesh::setLODLevel(int index, int meshIndex, float distance) {
    if (index >= 0 && index < static_cast<int>(m_levels.size())) {
        m_levels[index].meshIndex = meshIndex;
        m_levels[index].distance = distance;
        sortLevels();
    }
}

void LODMesh::removeLODLevel(int index) {
    if (index >= 0 && index < static_cast<int>(m_levels.size())) {
        m_levels.erase(m_levels.begin() + index);
    }
}

int LODMesh::selectLOD(float distanceToCamera) const {
    if (m_levels.empty()) {
        return 0;
    }
    
    float adjustedDistance = distanceToCamera * (1.0f + m_lodBias);
    
    for (size_t i = 0; i < m_levels.size(); ++i) {
        if (adjustedDistance < m_levels[i].distance) {
            return static_cast<int>(i);
        }
    }
    
    return static_cast<int>(m_levels.size()) - 1;
}

int LODMesh::selectLODByScreenCoverage(float screenCoverage) const {
    if (m_levels.empty()) {
        return 0;
    }
    
    for (size_t i = 0; i < m_levels.size(); ++i) {
        if (screenCoverage >= m_levels[i].screenCoverage) {
            return static_cast<int>(i);
        }
    }
    
    return static_cast<int>(m_levels.size()) - 1;
}

void LODMesh::update(float deltaTime, float currentDistance) {
    int newLOD = selectLOD(currentDistance);
    
    if (newLOD != m_currentLOD) {
        if (m_crossFadeEnabled) {
            m_transition.isFading = true;
            m_transition.fadeStartDistance = m_levels[m_currentLOD].distance;
            m_transition.fadeEndDistance = m_levels[newLOD].distance;
            m_transition.currentFade = 0.0f;
            m_previousLOD = m_currentLOD;
        }
        
        m_currentLOD = newLOD;
    }
    
    if (m_transition.isFading) {
        m_transition.currentFade += deltaTime * m_transitionSpeed;
        
        if (m_transition.currentFade >= 1.0f) {
            m_transition.isFading = false;
            m_transition.currentFade = 1.0f;
        }
    }
}

void LODMesh::sortLevels() {
    std::sort(m_levels.begin(), m_levels.end(),
        [](const LODLevel& a, const LODLevel& b) {
            return a.distance < b.distance;
        });
}

// LODGroup implementation
LODGroup::LODGroup(const std::string& name)
    : m_name(name)
    , m_posX(0.0f)
    , m_posY(0.0f)
    , m_posZ(0.0f)
    , m_boundsRadius(1.0f)
    , m_enabled(true)
    , m_distanceToCamera(0.0f)
    , m_activeLOD(0)
{
}

LODGroup::~LODGroup() {
}

void LODGroup::setPosition(float x, float y, float z) {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
}

void LODGroup::addMesh(LODMesh* mesh) {
    if (mesh) {
        m_meshes.push_back(mesh);
    }
}

void LODGroup::removeMesh(LODMesh* mesh) {
    auto it = std::find(m_meshes.begin(), m_meshes.end(), mesh);
    if (it != m_meshes.end()) {
        m_meshes.erase(it);
    }
}

LODMesh* LODGroup::getMesh(int index) {
    if (index >= 0 && index < static_cast<int>(m_meshes.size())) {
        return m_meshes[index];
    }
    return nullptr;
}

void LODGroup::update(float deltaTime, float cameraX, float cameraY, float cameraZ) {
    if (!m_enabled) {
        return;
    }
    
    // Calculate distance to camera
    float dx = m_posX - cameraX;
    float dy = m_posY - cameraY;
    float dz = m_posZ - cameraZ;
    m_distanceToCamera = std::sqrt(dx * dx + dy * dy + dz * dz);
    
    // Update all meshes in this group
    for (LODMesh* mesh : m_meshes) {
        if (mesh) {
            mesh->update(deltaTime, m_distanceToCamera);
            m_activeLOD = mesh->selectLOD(m_distanceToCamera);
        }
    }
}

// LODSystem implementation
LODSystem::LODSystem()
    : m_cameraX(0.0f)
    , m_cameraY(0.0f)
    , m_cameraZ(0.0f)
    , m_globalLODBias(0.0f)
    , m_maxLODLevel(3)
    , m_forceLODEnabled(false)
    , m_forceLOD(0)
    , m_debugVisualization(false)
{
    m_stats = LODStats();
}

LODSystem& LODSystem::getInstance() {
    static LODSystem instance;
    return instance;
}

void LODSystem::initialize() {
    // Initialize LOD system
}

void LODSystem::shutdown() {
    for (LODGroup* group : m_groups) {
        delete group;
    }
    m_groups.clear();
}

void LODSystem::update(float deltaTime) {
    resetStats();
    
    for (LODGroup* group : m_groups) {
        if (group && group->isEnabled()) {
            group->update(deltaTime, m_cameraX, m_cameraY, m_cameraZ);
            m_stats.visibleLODGroups++;
            
            // Count LOD levels
            int activeLOD = group->getActiveLOD();
            if (activeLOD == 0) m_stats.lod0Count++;
            else if (activeLOD == 1) m_stats.lod1Count++;
            else if (activeLOD == 2) m_stats.lod2Count++;
            else m_stats.lod3PlusCount++;
        }
    }
    
    m_stats.totalLODGroups = static_cast<int>(m_groups.size());
    updateStats();
}

void LODSystem::setCameraPosition(float x, float y, float z) {
    m_cameraX = x;
    m_cameraY = y;
    m_cameraZ = z;
}

void LODSystem::getCameraPosition(float& x, float& y, float& z) const {
    x = m_cameraX;
    y = m_cameraY;
    z = m_cameraZ;
}

LODGroup* LODSystem::createLODGroup(const std::string& name) {
    LODGroup* group = new LODGroup(name);
    m_groups.push_back(group);
    return group;
}

void LODSystem::destroyLODGroup(LODGroup* group) {
    auto it = std::find(m_groups.begin(), m_groups.end(), group);
    if (it != m_groups.end()) {
        delete *it;
        m_groups.erase(it);
    }
}

LODGroup* LODSystem::getLODGroup(const std::string& name) {
    for (LODGroup* group : m_groups) {
        if (group && group->getName() == name) {
            return group;
        }
    }
    return nullptr;
}

void LODSystem::setQuality(int quality) {
    switch (quality) {
        case 0: // Low
            m_globalLODBias = 1.0f;
            m_maxLODLevel = 1;
            break;
        case 1: // Medium
            m_globalLODBias = 0.5f;
            m_maxLODLevel = 2;
            break;
        case 2: // High
            m_globalLODBias = 0.0f;
            m_maxLODLevel = 3;
            break;
        case 3: // Ultra
            m_globalLODBias = -0.5f;
            m_maxLODLevel = 4;
            break;
    }
}

void LODSystem::resetStats() {
    m_stats.totalLODGroups = 0;
    m_stats.visibleLODGroups = 0;
    m_stats.lod0Count = 0;
    m_stats.lod1Count = 0;
    m_stats.lod2Count = 0;
    m_stats.lod3PlusCount = 0;
    m_stats.totalTriangles = 0;
    m_stats.savedTriangles = 0;
}

void LODSystem::updateStats() {
    // Calculate triangle counts and savings
}

// LODGenerator implementation
std::vector<int> LODGenerator::generateLODLevels(
    int highResMeshIndex,
    const std::vector<float>& distances,
    const std::vector<float>& reductionFactors
) {
    std::vector<int> lodMeshes;
    lodMeshes.push_back(highResMeshIndex);
    
    for (size_t i = 0; i < distances.size() && i < reductionFactors.size(); ++i) {
        int simplifiedMesh = simplifyMesh(highResMeshIndex, reductionFactors[i]);
        lodMeshes.push_back(simplifiedMesh);
    }
    
    return lodMeshes;
}

int LODGenerator::simplifyMesh(int sourceMeshIndex, float targetReduction) {
    // Implement mesh simplification algorithm
    // (e.g., quadric error metrics, edge collapse)
    return sourceMeshIndex;
}

std::vector<float> LODGenerator::calculateLODDistances(
    float objectSize,
    int lodCount,
    float minDistance,
    float maxDistance
) {
    std::vector<float> distances;
    
    if (lodCount <= 1) {
        distances.push_back(maxDistance);
        return distances;
    }
    
    // Logarithmic distribution of LOD distances
    float logMin = std::log(minDistance);
    float logMax = std::log(maxDistance);
    float logStep = (logMax - logMin) / (lodCount - 1);
    
    for (int i = 0; i < lodCount; ++i) {
        float dist = std::exp(logMin + i * logStep);
        distances.push_back(dist);
    }
    
    return distances;
}

} // namespace Engine
