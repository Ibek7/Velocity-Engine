#include "ai/AIVisionSystem.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace JJM {
namespace AI {

// Helper functions
static float vectorLength(const float v[3]) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

static float dotProduct(const float a[3], const float b[3]) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static void normalize(float v[3]) {
    float len = vectorLength(v);
    if (len > 0.0001f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
}

// AIVisionSystem implementation
AIVisionSystem::AIVisionSystem(Entity* owner)
    : m_owner(owner), m_ambientLight(1.0f), m_fogDensity(0.0f),
      m_debugVisualization(false) {
    
    // Enable sight by default
    m_enabledSenses[VisionSense::SIGHT] = true;
    m_enabledSenses[VisionSense::PERIPHERAL] = false;
    m_enabledSenses[VisionSense::NIGHT_VISION] = false;
    m_enabledSenses[VisionSense::THERMAL] = false;
    m_enabledSenses[VisionSense::MOTION_DETECTION] = false;
}

AIVisionSystem::~AIVisionSystem() {
}

void AIVisionSystem::update(float deltaTime) {
    // Scan for new targets
    scanForTargets();
    
    // Update existing targets
    for (auto& target : m_visibleTargets) {
        updateTarget(target, deltaTime);
    }
    
    // Remove targets that are no longer visible
    m_visibleTargets.erase(
        std::remove_if(m_visibleTargets.begin(), m_visibleTargets.end(),
                      [this](VisionTarget& target) {
                          if (target.visibility == VisibilityLevel::INVISIBLE) {
                              // Move to memory
                              VisualMemory memory;
                              memory.entity = target.entity;
                              memory.lastKnownPosition[0] = target.lastSeenPosition[0];
                              memory.lastKnownPosition[1] = target.lastSeenPosition[1];
                              memory.lastKnownPosition[2] = target.lastSeenPosition[2];
                              memory.lastKnownVelocity[0] = target.velocity[0];
                              memory.lastKnownVelocity[1] = target.velocity[1];
                              memory.lastKnownVelocity[2] = target.velocity[2];
                              memory.wasMoving = target.isMoving;
                              m_visualMemory.push_back(memory);
                              
                              // Trigger callback
                              if (m_onTargetLost) {
                                  m_onTargetLost(target.entity);
                              }
                              
                              return true;
                          }
                          return false;
                      }),
        m_visibleTargets.end()
    );
    
    // Update memory
    updateVisualMemory(deltaTime);
    
    // Debug visualization
    if (m_debugVisualization) {
        renderDebugVision();
    }
}

void AIVisionSystem::scanForTargets() {
    // Get nearby entities from vision manager
    // For now, this is a stub
    // AIVisionManager would provide potential targets
}

bool AIVisionSystem::isInFieldOfView(const float targetPos[3], bool usePeripheral) const {
    float eyePos[3];
    getEyePosition(eyePos);
    
    float toTarget[3] = {
        targetPos[0] - eyePos[0],
        targetPos[1] - eyePos[1],
        targetPos[2] - eyePos[2]
    };
    normalize(toTarget);
    
    float forward[3];
    getForwardDirection(forward);
    
    float dot = dotProduct(forward, toTarget);
    float angle = std::acos(dot) * 180.0f / 3.14159f;
    
    float fov = usePeripheral ? m_config.peripheralFOV : m_config.fieldOfView;
    return angle <= fov * 0.5f;
}

float AIVisionSystem::calculateVisibilityScore(const float targetPos[3], Entity* target) {
    float score = 1.0f;
    
    // Distance factor
    float distance = getDistanceTo(targetPos);
    if (distance > m_config.viewDistance) return 0.0f;
    float distanceFactor = 1.0f - (distance / m_config.viewDistance);
    score *= distanceFactor;
    
    // Angle factor (center of vision = 1.0, edge = 0.0)
    float angle = getAngleToTarget(targetPos);
    float angleFactor = 1.0f - (angle / (m_config.fieldOfView * 0.5f));
    score *= std::max(0.0f, angleFactor);
    
    // Lighting factor
    float lightFactor = calculateLightingFactor(targetPos);
    score *= lightFactor;
    
    // Size factor (larger = easier to see)
    float sizeFactor = calculateSizeFactor(target, distance);
    score *= sizeFactor;
    
    // Motion factor (moving = easier to spot)
    VisionTarget* existingTarget = const_cast<AIVisionSystem*>(this)->getTarget(target);
    if (existingTarget) {
        float motionFactor = calculateMotionFactor(*existingTarget);
        score *= motionFactor;
    }
    
    // Fog factor
    if (m_fogDensity > 0.0f) {
        float fogFactor = std::exp(-m_fogDensity * distance);
        score *= fogFactor;
    }
    
    return std::max(0.0f, std::min(1.0f, score));
}

OcclusionTest AIVisionSystem::performOcclusionTest(const float targetPos[3]) const {
    OcclusionTest result;
    result.isOccluded = false;
    result.coveragePercent = 0.0f;
    result.nearestOccluderDistance = 1000000.0f;
    
    // TODO: Implement raycast occlusion test
    // This would ray trace from eye position to target
    // and check for blocking geometry
    
    return result;
}

void AIVisionSystem::updateVisualMemory(float deltaTime) {
    for (auto& memory : m_visualMemory) {
        memory.timeSinceLastSeen += deltaTime;
        
        // Decay confidence over time
        memory.confidence -= deltaTime * 0.1f;  // Decay rate
        memory.confidence = std::max(0.0f, memory.confidence);
    }
    
    // Remove very old memories
    m_visualMemory.erase(
        std::remove_if(m_visualMemory.begin(), m_visualMemory.end(),
                      [](const VisualMemory& m) {
                          return m.confidence < 0.1f || m.timeSinceLastSeen > 30.0f;
                      }),
        m_visualMemory.end()
    );
}

void AIVisionSystem::updateTarget(VisionTarget& target, float deltaTime) {
    // Update position
    // target.position would be updated from entity's current position
    
    // Update velocity
    float dx = target.position[0] - target.lastSeenPosition[0];
    float dy = target.position[1] - target.lastSeenPosition[1];
    float dz = target.position[2] - target.lastSeenPosition[2];
    
    target.velocity[0] = dx / deltaTime;
    target.velocity[1] = dy / deltaTime;
    target.velocity[2] = dz / deltaTime;
    
    target.movementSpeed = vectorLength(target.velocity);
    target.isMoving = target.movementSpeed > 0.1f;
    
    // Recalculate visibility
    target.visibilityScore = calculateVisibilityScore(target.position, target.entity);
    
    // Update distance and angle
    target.distance = getDistanceTo(target.position);
    target.angleFromForward = getAngleToTarget(target.position);
    target.isInFieldOfView = isInFieldOfView(target.position, false);
    
    // Occlusion test
    OcclusionTest occlusionTest = performOcclusionTest(target.position);
    target.isOccluded = occlusionTest.isOccluded;
    
    // Determine visibility level
    if (target.isOccluded || target.visibilityScore < 0.1f) {
        target.visibility = VisibilityLevel::INVISIBLE;
    } else if (target.visibilityScore < 0.3f) {
        target.visibility = VisibilityLevel::BARELY_VISIBLE;
    } else if (target.visibilityScore < 0.6f) {
        target.visibility = VisibilityLevel::PARTIALLY_VISIBLE;
    } else {
        target.visibility = VisibilityLevel::CLEARLY_VISIBLE;
    }
    
    // Update timing
    if (target.visibility != VisibilityLevel::INVISIBLE) {
        target.totalVisibleTime += deltaTime;
        target.lastSeenTime += deltaTime;
        target.lastSeenPosition[0] = target.position[0];
        target.lastSeenPosition[1] = target.position[1];
        target.lastSeenPosition[2] = target.position[2];
    }
}

VisionTarget* AIVisionSystem::getTarget(Entity* entity) {
    for (auto& target : m_visibleTargets) {
        if (target.entity == entity) {
            return &target;
        }
    }
    return nullptr;
}

bool AIVisionSystem::canSee(Entity* entity) const {
    for (const auto& target : m_visibleTargets) {
        if (target.entity == entity && target.visibility != VisibilityLevel::INVISIBLE) {
            return true;
        }
    }
    return false;
}

bool AIVisionSystem::canSee(const float position[3]) const {
    if (!isInFieldOfView(position, false)) return false;
    
    float distance = getDistanceTo(position);
    if (distance > m_config.viewDistance) return false;
    
    // Would also check occlusion
    return true;
}

std::vector<VisionTarget*> AIVisionSystem::getTargetsInFOV() {
    std::vector<VisionTarget*> result;
    for (auto& target : m_visibleTargets) {
        if (target.isInFieldOfView) {
            result.push_back(&target);
        }
    }
    return result;
}

std::vector<VisionTarget*> AIVisionSystem::getClearlyVisibleTargets() {
    std::vector<VisionTarget*> result;
    for (auto& target : m_visibleTargets) {
        if (target.visibility == VisibilityLevel::CLEARLY_VISIBLE ||
            target.visibility == VisibilityLevel::HIGHLIGHTED) {
            result.push_back(&target);
        }
    }
    return result;
}

VisionTarget* AIVisionSystem::getNearestVisibleTarget() {
    VisionTarget* nearest = nullptr;
    float minDist = 1000000.0f;
    
    for (auto& target : m_visibleTargets) {
        if (target.visibility != VisibilityLevel::INVISIBLE && target.distance < minDist) {
            nearest = &target;
            minDist = target.distance;
        }
    }
    
    return nearest;
}

VisionTarget* AIVisionSystem::getMostVisibleTarget() {
    VisionTarget* most = nullptr;
    float maxScore = 0.0f;
    
    for (auto& target : m_visibleTargets) {
        if (target.visibilityScore > maxScore) {
            most = &target;
            maxScore = target.visibilityScore;
        }
    }
    
    return most;
}

VisualMemory* AIVisionSystem::recallMemory(Entity* entity) {
    for (auto& memory : m_visualMemory) {
        if (memory.entity == entity) {
            return &memory;
        }
    }
    return nullptr;
}

void AIVisionSystem::forgetEntity(Entity* entity) {
    m_visualMemory.erase(
        std::remove_if(m_visualMemory.begin(), m_visualMemory.end(),
                      [entity](const VisualMemory& m) {
                          return m.entity == entity;
                      }),
        m_visualMemory.end()
    );
}

void AIVisionSystem::clearMemory() {
    m_visualMemory.clear();
}

void AIVisionSystem::enableSense(VisionSense sense, bool enable) {
    m_enabledSenses[sense] = enable;
}

bool AIVisionSystem::isSenseEnabled(VisionSense sense) const {
    auto it = m_enabledSenses.find(sense);
    return it != m_enabledSenses.end() && it->second;
}

void AIVisionSystem::renderDebugVision() {
    // TODO: Implement debug visualization
    // Would render vision cone, visible targets, etc.
}

float AIVisionSystem::getDistanceTo(const float pos[3]) const {
    float eyePos[3];
    getEyePosition(eyePos);
    
    float dx = pos[0] - eyePos[0];
    float dy = pos[1] - eyePos[1];
    float dz = pos[2] - eyePos[2];
    
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

float AIVisionSystem::getAngleToTarget(const float targetPos[3]) const {
    float eyePos[3];
    getEyePosition(eyePos);
    
    float toTarget[3] = {
        targetPos[0] - eyePos[0],
        targetPos[1] - eyePos[1],
        targetPos[2] - eyePos[2]
    };
    normalize(toTarget);
    
    float forward[3];
    getForwardDirection(forward);
    
    float dot = dotProduct(forward, toTarget);
    return std::acos(std::max(-1.0f, std::min(1.0f, dot))) * 180.0f / 3.14159f;
}

void AIVisionSystem::getEyePosition(float outPos[3]) const {
    // Get owner position and add eye height
    // outPos[0] = ownerPos[0];
    // outPos[1] = ownerPos[1] + m_config.eyeHeight;
    // outPos[2] = ownerPos[2];
    outPos[0] = outPos[1] = outPos[2] = 0;  // Stub
}

void AIVisionSystem::getForwardDirection(float outDir[3]) const {
    // Get owner's forward direction
    // This would come from the entity's transform
    outDir[0] = 0;
    outDir[1] = 0;
    outDir[2] = 1;  // Stub: facing +Z
}

float AIVisionSystem::calculateLightingFactor(const float pos[3]) const {
    float lightFactor = m_ambientLight;
    
    // Adjust based on vision type
    if (isSenseEnabled(VisionSense::NIGHT_VISION)) {
        lightFactor = std::max(lightFactor, 0.8f);  // Can see in dark
    }
    
    lightFactor = std::pow(lightFactor, 1.0f / m_config.lightSensitivity);
    
    return std::max(0.1f, std::min(1.0f, lightFactor));
}

float AIVisionSystem::calculateSizeFactor(Entity* target, float distance) const {
    // Larger objects are easier to see
    float apparentSize = 1.0f / std::max(1.0f, distance);
    return std::min(1.0f, apparentSize * 5.0f);
}

float AIVisionSystem::calculateMotionFactor(const VisionTarget& target) const {
    if (!target.isMoving) return 1.0f;
    
    // Moving targets are easier to spot
    float motionBonus = 1.0f + (target.movementSpeed * 0.1f * m_config.motionSensitivity);
    return std::min(2.0f, motionBonus);
}

float AIVisionSystem::calculateContrastFactor(Entity* target) const {
    // TODO: Calculate based on target color vs background
    return 1.0f;
}

// AIVisionManager implementation
AIVisionManager::AIVisionManager()
    : m_useSpacialPartitioning(true), m_sectorSize(50.0f),
      m_globalLightLevel(1.0f), m_globalFogDensity(0.0f),
      m_updateBudgetMs(5.0f), m_maxUpdatesPerFrame(10),
      m_currentUpdateIndex(0) {
    m_stats = Stats();
}

AIVisionManager::~AIVisionManager() {
}

void AIVisionManager::update(float deltaTime) {
    // Update spatial partitioning
    if (m_useSpacialPartitioning) {
        updateSpatialPartitioning();
    }
    
    // Update vision systems (time-sliced)
    int updatesThisFrame = 0;
    int systemCount = static_cast<int>(m_visionSystems.size());
    
    if (systemCount > 0) {
        while (updatesThisFrame < m_maxUpdatesPerFrame && updatesThisFrame < systemCount) {
            int index = m_currentUpdateIndex % systemCount;
            m_visionSystems[index]->update(deltaTime);
            
            m_currentUpdateIndex++;
            updatesThisFrame++;
        }
    }
    
    // Update stats
    m_stats.totalVisionSystems = systemCount;
    m_stats.totalTargets = static_cast<int>(m_targets.size());
}

void AIVisionManager::registerVisionSystem(AIVisionSystem* system) {
    m_visionSystems.push_back(system);
}

void AIVisionManager::unregisterVisionSystem(AIVisionSystem* system) {
    m_visionSystems.erase(
        std::remove(m_visionSystems.begin(), m_visionSystems.end(), system),
        m_visionSystems.end()
    );
}

void AIVisionManager::registerTarget(Entity* entity) {
    m_targets.push_back(entity);
}

void AIVisionManager::unregisterTarget(Entity* entity) {
    m_targets.erase(
        std::remove(m_targets.begin(), m_targets.end(), entity),
        m_targets.end()
    );
}

AIVisionManager::Stats AIVisionManager::getStatistics() const {
    return m_stats;
}

void AIVisionManager::updateSpatialPartitioning() {
    m_sectors.clear();
    
    for (Entity* target : m_targets) {
        // Get target position
        float pos[3] = {0, 0, 0};  // Would get from entity
        uint64_t key = getSectorKey(pos[0], pos[2]);
        m_sectors[key].targets.push_back(target);
    }
}

uint64_t AIVisionManager::getSectorKey(float x, float z) const {
    int32_t sectorX = static_cast<int32_t>(std::floor(x / m_sectorSize));
    int32_t sectorZ = static_cast<int32_t>(std::floor(z / m_sectorSize));
    return (static_cast<uint64_t>(sectorX) << 32) | static_cast<uint64_t>(sectorZ);
}

std::vector<Entity*> AIVisionManager::getNearbyTargets(const float pos[3], float radius) {
    if (!m_useSpacialPartitioning) {
        return m_targets;
    }
    
    std::vector<Entity*> nearby;
    
    // Get sectors in radius
    int sectorRadius = static_cast<int>(std::ceil(radius / m_sectorSize));
    uint64_t centerKey = getSectorKey(pos[0], pos[2]);
    
    // Check neighboring sectors
    for (int dx = -sectorRadius; dx <= sectorRadius; ++dx) {
        for (int dz = -sectorRadius; dz <= sectorRadius; ++dz) {
            uint64_t key = centerKey + (static_cast<uint64_t>(dx) << 32) + dz;
            auto it = m_sectors.find(key);
            if (it != m_sectors.end()) {
                nearby.insert(nearby.end(), it->second.targets.begin(), it->second.targets.end());
            }
        }
    }
    
    return nearby;
}

// VisionQueries implementation
namespace VisionQueries {

std::vector<Entity*> findAllVisibleEntities() {
    // TODO: Query all vision systems
    return std::vector<Entity*>();
}

std::vector<AIVisionSystem*> findObservers(Entity* target) {
    // TODO: Query which systems can see target
    return std::vector<AIVisionSystem*>();
}

bool isVisibleFrom(Entity* target, const float position[3],
                  const VisionConeConfig& config) {
    // TODO: Test visibility from position
    return false;
}

bool willBeVisible(AIVisionSystem* vision, Entity* target, float timeInFuture) {
    // TODO: Predict future visibility
    return false;
}

void findHidingSpot(const float currentPos[3],
                   const std::vector<AIVisionSystem*>& observers,
                   float searchRadius, float outPos[3]) {
    // TODO: Find position not visible to observers
    outPos[0] = currentPos[0];
    outPos[1] = currentPos[1];
    outPos[2] = currentPos[2];
}

} // namespace VisionQueries

} // namespace AI
} // namespace JJM
