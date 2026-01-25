#ifndef AI_VISION_SYSTEM_H
#define AI_VISION_SYSTEM_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace JJM {
namespace AI {

// Forward declarations
class Entity;
class Transform;

// Vision sense types
enum class VisionSense {
    SIGHT,              // Normal vision
    PERIPHERAL,         // Wide angle, lower detail
    NIGHT_VISION,       // See in darkness
    THERMAL,            // Heat signatures
    MOTION_DETECTION    // Detect movement only
};

// Visibility level
enum class VisibilityLevel {
    INVISIBLE,          // Cannot be seen
    BARELY_VISIBLE,     // Edge of vision
    PARTIALLY_VISIBLE,  // Partially obscured
    CLEARLY_VISIBLE,    // Fully visible
    HIGHLIGHTED         // Extra visible (e.g., glowing)
};

// Vision target information
struct VisionTarget {
    Entity* entity;
    float position[3];
    float lastSeenPosition[3];
    float velocity[3];
    VisibilityLevel visibility;
    float visibilityScore;      // 0-1
    float distance;
    float angleFromForward;     // Degrees
    bool isInFieldOfView;
    bool isOccluded;
    float lastSeenTime;
    float firstSeenTime;
    float totalVisibleTime;
    int sightingCount;
    
    // Visual properties
    float size;                 // Apparent size
    float brightness;
    float contrast;
    bool isMoving;
    float movementSpeed;
    
    VisionTarget()
        : entity(nullptr), visibility(VisibilityLevel::INVISIBLE),
          visibilityScore(0), distance(0), angleFromForward(0),
          isInFieldOfView(false), isOccluded(false),
          lastSeenTime(0), firstSeenTime(0), totalVisibleTime(0),
          sightingCount(0), size(1.0f), brightness(1.0f), contrast(1.0f),
          isMoving(false), movementSpeed(0) {
        position[0] = position[1] = position[2] = 0;
        lastSeenPosition[0] = lastSeenPosition[1] = lastSeenPosition[2] = 0;
        velocity[0] = velocity[1] = velocity[2] = 0;
    }
};

// Vision cone configuration
struct VisionConeConfig {
    float fieldOfView;          // Degrees (e.g., 90)
    float peripheralFOV;        // Degrees for peripheral vision (e.g., 180)
    float viewDistance;         // Max distance
    float peripheralDistance;   // Max peripheral distance (usually shorter)
    float minVisibilityDistance; // Below this, always visible
    float eyeHeight;            // Height offset from entity position
    
    // Detail levels by distance
    float highDetailDistance;   // Full detail
    float mediumDetailDistance; // Reduced detail
    float lowDetailDistance;    // Minimal detail
    
    // Vision modifiers
    bool nightVisionEnabled;
    bool thermalVisionEnabled;
    float lightSensitivity;     // How much lighting affects vision
    float motionSensitivity;    // How easily motion is detected
    
    VisionConeConfig()
        : fieldOfView(90.0f), peripheralFOV(180.0f),
          viewDistance(50.0f), peripheralDistance(30.0f),
          minVisibilityDistance(2.0f), eyeHeight(1.7f),
          highDetailDistance(10.0f), mediumDetailDistance(25.0f),
          lowDetailDistance(50.0f), nightVisionEnabled(false),
          thermalVisionEnabled(false), lightSensitivity(1.0f),
          motionSensitivity(1.0f) {}
};

// Visual memory - remembered targets
struct VisualMemory {
    Entity* entity;
    float lastKnownPosition[3];
    float lastKnownVelocity[3];
    float confidence;           // 0-1, decays over time
    float timeSinceLastSeen;
    bool wasHostile;
    bool wasMoving;
    
    VisualMemory()
        : entity(nullptr), confidence(1.0f), timeSinceLastSeen(0),
          wasHostile(false), wasMoving(false) {
        lastKnownPosition[0] = lastKnownPosition[1] = lastKnownPosition[2] = 0;
        lastKnownVelocity[0] = lastKnownVelocity[1] = lastKnownVelocity[2] = 0;
    }
};

// Occlusion test result
struct OcclusionTest {
    bool isOccluded;
    float coveragePercent;      // How much is blocked (0-1)
    float nearestOccluderDistance;
    std::vector<Entity*> occluders;
};

// AI Vision System for a single entity
class AIVisionSystem {
public:
    AIVisionSystem(Entity* owner);
    ~AIVisionSystem();
    
    void update(float deltaTime);
    
    // Configuration
    void setConfig(const VisionConeConfig& config) { m_config = config; }
    VisionConeConfig& getConfig() { return m_config; }
    
    // Vision queries
    const std::vector<VisionTarget>& getVisibleTargets() const { return m_visibleTargets; }
    VisionTarget* getTarget(Entity* entity);
    bool canSee(Entity* entity) const;
    bool canSee(const float position[3]) const;
    
    // Get targets by visibility
    std::vector<VisionTarget*> getTargetsInFOV();
    std::vector<VisionTarget*> getClearlyVisibleTargets();
    VisionTarget* getNearestVisibleTarget();
    VisionTarget* getMostVisibleTarget();
    
    // Memory
    const std::vector<VisualMemory>& getVisualMemory() const { return m_visualMemory; }
    VisualMemory* recallMemory(Entity* entity);
    void forgetEntity(Entity* entity);
    void clearMemory();
    
    // Vision senses
    void enableSense(VisionSense sense, bool enable);
    bool isSenseEnabled(VisionSense sense) const;
    
    // Environmental factors
    void setAmbientLight(float level) { m_ambientLight = level; }
    void setFogDensity(float density) { m_fogDensity = density; }
    float getAmbientLight() const { return m_ambientLight; }
    
    // Debug visualization
    void setDebugVisualization(bool enable) { m_debugVisualization = enable; }
    void renderDebugVision();
    
    // Callbacks
    using TargetSpottedCallback = std::function<void(VisionTarget*)>;
    using TargetLostCallback = std::function<void(Entity*)>;
    
    void onTargetSpotted(TargetSpottedCallback callback) { m_onTargetSpotted = callback; }
    void onTargetLost(TargetLostCallback callback) { m_onTargetLost = callback; }
    
private:
    Entity* m_owner;
    VisionConeConfig m_config;
    
    std::vector<VisionTarget> m_visibleTargets;
    std::vector<VisualMemory> m_visualMemory;
    
    // Enabled senses
    std::unordered_map<VisionSense, bool> m_enabledSenses;
    
    // Environmental factors
    float m_ambientLight;
    float m_fogDensity;
    
    // Debug
    bool m_debugVisualization;
    
    // Callbacks
    TargetSpottedCallback m_onTargetSpotted;
    TargetLostCallback m_onTargetLost;
    
    // Internal methods
    void scanForTargets();
    bool isInFieldOfView(const float targetPos[3], bool usePeripheral) const;
    float calculateVisibilityScore(const float targetPos[3], Entity* target);
    OcclusionTest performOcclusionTest(const float targetPos[3]) const;
    void updateVisualMemory(float deltaTime);
    void updateTarget(VisionTarget& target, float deltaTime);
    
    // Vision calculations
    float getDistanceTo(const float pos[3]) const;
    float getAngleToTarget(const float targetPos[3]) const;
    void getEyePosition(float outPos[3]) const;
    void getForwardDirection(float outDir[3]) const;
    
    // Visual factors
    float calculateLightingFactor(const float pos[3]) const;
    float calculateSizeFactor(Entity* target, float distance) const;
    float calculateMotionFactor(const VisionTarget& target) const;
    float calculateContrastFactor(Entity* target) const;
};

// Global AI Vision Manager
class AIVisionManager {
public:
    AIVisionManager();
    ~AIVisionManager();
    
    void update(float deltaTime);
    
    // Register AI entities
    void registerVisionSystem(AIVisionSystem* system);
    void unregisterVisionSystem(AIVisionSystem* system);
    
    // Register potential targets
    void registerTarget(Entity* entity);
    void unregisterTarget(Entity* entity);
    
    // Get all registered targets
    const std::vector<Entity*>& getAllTargets() const { return m_targets; }
    
    // Spatial optimization
    void setUseSpacialPartitioning(bool use) { m_useSpacialPartitioning = use; }
    void setSectorSize(float size) { m_sectorSize = size; }
    
    // Global vision modifiers
    void setGlobalLightLevel(float level) { m_globalLightLevel = level; }
    void setGlobalFogDensity(float density) { m_globalFogDensity = density; }
    
    // Performance tuning
    void setUpdateBudget(float milliseconds) { m_updateBudgetMs = milliseconds; }
    void setMaxUpdatesPerFrame(int max) { m_maxUpdatesPerFrame = max; }
    
    // Statistics
    struct Stats {
        int totalVisionSystems;
        int totalTargets;
        int totalVisibilityTests;
        int totalOcclusionTests;
        float averageUpdateTime;
        int targetsVisible;
    };
    
    Stats getStatistics() const;
    
private:
    std::vector<AIVisionSystem*> m_visionSystems;
    std::vector<Entity*> m_targets;
    
    // Spatial partitioning
    bool m_useSpacialPartitioning;
    float m_sectorSize;
    struct Sector {
        std::vector<Entity*> targets;
    };
    std::unordered_map<uint64_t, Sector> m_sectors;
    
    // Global modifiers
    float m_globalLightLevel;
    float m_globalFogDensity;
    
    // Performance
    float m_updateBudgetMs;
    int m_maxUpdatesPerFrame;
    int m_currentUpdateIndex;
    
    // Stats
    mutable Stats m_stats;
    
    // Internal
    void updateSpatialPartitioning();
    uint64_t getSectorKey(float x, float z) const;
    std::vector<Entity*> getNearbyTargets(const float pos[3], float radius);
};

// Perception query helpers
namespace VisionQueries {
    // Find all entities visible to any AI
    std::vector<Entity*> findAllVisibleEntities();
    
    // Find all AIs that can see a specific entity
    std::vector<AIVisionSystem*> findObservers(Entity* target);
    
    // Check if entity is visible from a position
    bool isVisibleFrom(Entity* target, const float position[3],
                      const VisionConeConfig& config);
    
    // Predict if target will be visible in the future
    bool willBeVisible(AIVisionSystem* vision, Entity* target,
                      float timeInFuture);
    
    // Find best hiding spot from observers
    void findHidingSpot(const float currentPos[3],
                       const std::vector<AIVisionSystem*>& observers,
                       float searchRadius, float outPos[3]);
}

} // namespace AI
} // namespace JJM

#endif // AI_VISION_SYSTEM_H
