#include "graphics/MeshLOD.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Graphics {

// =============================================================================
// MeshLOD Implementation
// =============================================================================

MeshLOD::MeshLOD(const std::string& name)
    : m_name(name)
    , m_currentLOD(0)
    , m_transitionMode(LODTransitionMode::Instant)
    , m_transitionSpeed(1.0f)
    , m_transitionProgress(1.0f)
    , m_useForcedLOD(false)
    , m_forcedLOD(-1)
    , m_lodBias(0.0f)
    , m_minScreenSize(0.01f)
{}

MeshLOD::~MeshLOD() = default;

void MeshLOD::addLODLevel(const LODLevel& level) {
    m_levels.push_back(level);
    sortLODLevels();
}

void MeshLOD::addLODLevel(float distance, std::shared_ptr<Mesh> mesh) {
    LODLevel level;
    level.distance = distance;
    level.mesh = mesh;
    // TODO: Calculate triangle count from mesh
    level.triangleCount = 0;
    m_levels.push_back(level);
    sortLODLevels();
}

const LODLevel& MeshLOD::getLODLevel(int index) const {
    static LODLevel emptyLevel;
    if (index >= 0 && index < static_cast<int>(m_levels.size())) {
        return m_levels[index];
    }
    return emptyLevel;
}

bool MeshLOD::updateDistance(float distance, float deltaTime) {
    if (m_useForcedLOD) {
        return false;
    }
    
    // Apply LOD bias
    float biasedDistance = distance * (1.0f + m_lodBias);
    
    int targetLOD = selectLODByDistance(biasedDistance);
    
    if (targetLOD != m_currentLOD) {
        if (m_transitionMode == LODTransitionMode::Instant) {
            m_currentLOD = targetLOD;
            m_transitionProgress = 1.0f;
        } else {
            updateTransition(targetLOD, deltaTime);
        }
        return true;
    }
    
    return false;
}

bool MeshLOD::updateScreenCoverage(float screenCoverage, float deltaTime) {
    if (m_useForcedLOD) {
        return false;
    }
    
    int targetLOD = selectLODByScreenCoverage(screenCoverage);
    
    if (targetLOD != m_currentLOD) {
        if (m_transitionMode == LODTransitionMode::Instant) {
            m_currentLOD = targetLOD;
            m_transitionProgress = 1.0f;
        } else {
            updateTransition(targetLOD, deltaTime);
        }
        return true;
    }
    
    return false;
}

std::shared_ptr<Mesh> MeshLOD::getCurrentMesh() const {
    if (m_currentLOD >= 0 && m_currentLOD < static_cast<int>(m_levels.size())) {
        return m_levels[m_currentLOD].mesh;
    }
    return nullptr;
}

void MeshLOD::setForcedLOD(int lodLevel) {
    if (lodLevel < 0) {
        m_useForcedLOD = false;
        m_forcedLOD = -1;
    } else if (lodLevel < static_cast<int>(m_levels.size())) {
        m_useForcedLOD = true;
        m_forcedLOD = lodLevel;
        m_currentLOD = lodLevel;
    }
}

bool MeshLOD::isTransitioning() const {
    return m_transitionProgress < 1.0f;
}

bool MeshLOD::shouldCull(float screenCoverage) const {
    return screenCoverage < m_minScreenSize;
}

int MeshLOD::getTotalTriangleCount() const {
    int total = 0;
    for (const auto& level : m_levels) {
        total += level.triangleCount;
    }
    return total;
}

void MeshLOD::sortLODLevels() {
    std::sort(m_levels.begin(), m_levels.end(),
        [](const LODLevel& a, const LODLevel& b) {
            return a.distance < b.distance;
        });
}

int MeshLOD::selectLODByDistance(float distance) const {
    if (m_levels.empty()) return 0;
    
    // Find appropriate LOD level based on distance
    for (int i = static_cast<int>(m_levels.size()) - 1; i >= 0; --i) {
        if (distance >= m_levels[i].distance) {
            return i;
        }
    }
    
    return 0;  // Return highest quality if within all thresholds
}

int MeshLOD::selectLODByScreenCoverage(float coverage) const {
    if (m_levels.empty()) return 0;
    
    // Find appropriate LOD level based on screen coverage
    for (int i = 0; i < static_cast<int>(m_levels.size()); ++i) {
        if (coverage >= m_levels[i].screenCoverage) {
            return i;
        }
    }
    
    return static_cast<int>(m_levels.size()) - 1;  // Return lowest quality
}

void MeshLOD::updateTransition(int targetLOD, float deltaTime) {
    // Update transition progress
    m_transitionProgress += m_transitionSpeed * deltaTime;
    
    if (m_transitionProgress >= 1.0f) {
        m_currentLOD = targetLOD;
        m_transitionProgress = 1.0f;
    }
}

// =============================================================================
// LODSystem Implementation
// =============================================================================

LODSystem::LODSystem()
    : m_globalLODBias(0.0f)
    , m_lodDistanceScale(1.0f)
    , m_enableLOD(true)
{}

LODSystem::~LODSystem() = default;

void LODSystem::registerLODGroup(MeshLOD* lodGroup) {
    if (lodGroup && std::find(m_lodGroups.begin(), m_lodGroups.end(), lodGroup) == m_lodGroups.end()) {
        m_lodGroups.push_back(lodGroup);
    }
}

void LODSystem::unregisterLODGroup(MeshLOD* lodGroup) {
    m_lodGroups.erase(
        std::remove(m_lodGroups.begin(), m_lodGroups.end(), lodGroup),
        m_lodGroups.end()
    );
}

void LODSystem::update(const float* cameraPosition, float deltaTime) {
    if (!m_enableLOD) return;
    
    clearStats();
    
    for (MeshLOD* lodGroup : m_lodGroups) {
        if (!lodGroup) continue;
        
        // Apply global bias
        float combinedBias = lodGroup->getLODBias() + m_globalLODBias;
        lodGroup->setLODBias(combinedBias);
        
        // TODO: Calculate distance from camera to object
        // For now, use placeholder distance calculation
        float distance = 10.0f * m_lodDistanceScale;
        
        // Update LOD based on distance
        lodGroup->updateDistance(distance, deltaTime);
    }
    
    updateStats();
}

void LODSystem::clearStats() {
    m_stats = Stats();
}

void LODSystem::updateStats() {
    m_stats.totalLODGroups = static_cast<int>(m_lodGroups.size());
    
    for (const MeshLOD* lodGroup : m_lodGroups) {
        if (!lodGroup) continue;
        
        int currentLOD = lodGroup->getCurrentLOD();
        
        if (currentLOD == 0) {
            m_stats.lod0Count++;
        } else if (currentLOD == 1) {
            m_stats.lod1Count++;
        } else if (currentLOD == 2) {
            m_stats.lod2Count++;
        } else {
            m_stats.lod3PlusCount++;
        }
        
        m_stats.visibleObjects++;
    }
}

// =============================================================================
// LODGenerator Implementation
// =============================================================================

std::vector<std::shared_ptr<Mesh>> LODGenerator::generateLODs(
    const Mesh& baseMesh,
    int numLevels,
    const std::vector<float>& reductionRates)
{
    std::vector<std::shared_ptr<Mesh>> lodMeshes;
    
    // Add base mesh as LOD 0
    // TODO: Clone mesh
    // lodMeshes.push_back(std::make_shared<Mesh>(baseMesh));
    
    // Generate simplified versions
    for (int i = 0; i < numLevels - 1 && i < static_cast<int>(reductionRates.size()); ++i) {
        // TODO: Implement mesh simplification
        // For now, return empty vector
    }
    
    return lodMeshes;
}

std::vector<float> LODGenerator::calculateLODDistances(float objectSize, int numLevels) {
    std::vector<float> distances;
    
    // Use exponential scaling for LOD distances
    float baseDistance = objectSize * 2.0f;
    
    for (int i = 0; i < numLevels; ++i) {
        float distance = baseDistance * std::pow(2.0f, static_cast<float>(i));
        distances.push_back(distance);
    }
    
    return distances;
}

std::shared_ptr<Mesh> LODGenerator::simplifyMesh(const Mesh& mesh, int targetTriangleCount) {
    // TODO: Implement mesh simplification algorithm (e.g., edge collapse, quadric error metrics)
    // This is a complex algorithm that would require significant implementation
    return nullptr;
}

} // namespace Graphics
} // namespace JJM
