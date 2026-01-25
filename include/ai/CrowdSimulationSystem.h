#ifndef CROWD_SIMULATION_SYSTEM_H
#define CROWD_SIMULATION_SYSTEM_H

#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace AI {

// Forward declarations
class Entity;
class NavigationMesh;

// Crowd agent properties
struct CrowdAgentProperties {
    float radius;               // Agent radius for collision
    float height;               // Agent height
    float maxSpeed;             // Maximum movement speed
    float maxAcceleration;      // Maximum acceleration
    float separationWeight;     // Weight for separation behavior
    float alignmentWeight;      // Weight for alignment behavior
    float cohesionWeight;       // Weight for cohesion behavior
    float avoidanceRadius;      // Radius to avoid other agents
    float neighborRadius;       // Radius to consider neighbors
    int maxNeighbors;           // Maximum neighbors to consider
    float pathOptimizationRange; // Range for path optimization
    bool useLocalAvoidance;
    bool usePathFollowing;
    
    CrowdAgentProperties()
        : radius(0.5f), height(2.0f), maxSpeed(3.5f), maxAcceleration(8.0f),
          separationWeight(1.0f), alignmentWeight(0.5f), cohesionWeight(0.5f),
          avoidanceRadius(2.0f), neighborRadius(5.0f), maxNeighbors(6),
          pathOptimizationRange(2.0f), useLocalAvoidance(true), usePathFollowing(true) {}
};

// Crowd agent state
enum class CrowdAgentState {
    IDLE,
    WALKING,
    RUNNING,
    WAITING,
    PANICKED,
    FOLLOWING_PATH
};

// Crowd agent
class CrowdAgent {
public:
    CrowdAgent(Entity* entity);
    ~CrowdAgent();
    
    void setProperties(const CrowdAgentProperties& props) { m_properties = props; }
    const CrowdAgentProperties& getProperties() const { return m_properties; }
    
    void setPosition(const float pos[3]);
    void getPosition(float outPos[3]) const;
    
    void setVelocity(const float vel[3]);
    void getVelocity(float outVel[3]) const;
    
    void setTarget(const float target[3]);
    bool hasTarget() const { return m_hasTarget; }
    void clearTarget();
    
    void setState(CrowdAgentState state) { m_state = state; }
    CrowdAgentState getState() const { return m_state; }
    
    Entity* getEntity() { return m_entity; }
    const Entity* getEntity() const { return m_entity; }
    
    // Path following
    void setPath(const std::vector<float>& waypoints);
    const std::vector<float>& getPath() const { return m_path; }
    int getCurrentWaypointIndex() const { return m_currentWaypoint; }
    
    // Neighbors
    void addNeighbor(CrowdAgent* agent);
    void clearNeighbors();
    const std::vector<CrowdAgent*>& getNeighbors() const { return m_neighbors; }
    
    // Update
    void calculateSteering(float outSteering[3]);
    void update(float deltaTime);
    
    // Enable/disable
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    Entity* m_entity;
    CrowdAgentProperties m_properties;
    CrowdAgentState m_state;
    
    float m_position[3];
    float m_velocity[3];
    float m_target[3];
    bool m_hasTarget;
    
    std::vector<float> m_path;  // Series of waypoints (x,y,z, x,y,z, ...)
    int m_currentWaypoint;
    
    std::vector<CrowdAgent*> m_neighbors;
    bool m_enabled;
    
    // Steering behaviors
    void calculateSeparation(float outForce[3]);
    void calculateAlignment(float outForce[3]);
    void calculateCohesion(float outForce[3]);
    void calculateAvoidance(float outForce[3]);
    void calculatePathFollowing(float outForce[3]);
};

// Crowd formation
enum class FormationType {
    NONE,
    LINE,
    COLUMN,
    WEDGE,
    CIRCLE,
    GRID
};

struct Formation {
    FormationType type;
    float spacing;
    float position[3];
    float direction[3];
    std::vector<CrowdAgent*> agents;
    
    Formation() : type(FormationType::NONE), spacing(2.0f) {
        position[0] = position[1] = position[2] = 0;
        direction[0] = 0; direction[1] = 0; direction[2] = 1;
    }
};

// Spatial partitioning grid for optimization
class CrowdGrid {
public:
    CrowdGrid(float cellSize);
    ~CrowdGrid();
    
    void clear();
    void insert(CrowdAgent* agent);
    std::vector<CrowdAgent*> queryRadius(const float position[3], float radius);
    
private:
    float m_cellSize;
    struct Cell {
        std::vector<CrowdAgent*> agents;
    };
    std::unordered_map<uint64_t, Cell> m_cells;
    
    uint64_t getCellKey(float x, float z) const;
};

// Main crowd simulation system
class CrowdSimulationSystem {
public:
    CrowdSimulationSystem();
    ~CrowdSimulationSystem();
    
    void initialize(NavigationMesh* navmesh = nullptr);
    void shutdown();
    
    // Agent management
    CrowdAgent* addAgent(Entity* entity, const CrowdAgentProperties& props);
    void removeAgent(CrowdAgent* agent);
    void removeAllAgents();
    
    const std::vector<CrowdAgent*>& getAllAgents() const { return m_agents; }
    CrowdAgent* getAgent(Entity* entity);
    
    // Update
    void update(float deltaTime);
    
    // Formation management
    Formation* createFormation(FormationType type, const float position[3]);
    void assignAgentToFormation(CrowdAgent* agent, Formation* formation);
    void updateFormation(Formation* formation);
    void dissolveFormation(Formation* formation);
    
    // Group behaviors
    void setGroupTarget(const std::vector<CrowdAgent*>& agents, const float target[3]);
    void makeGroupFlee(const std::vector<CrowdAgent*>& agents, const float fleeFrom[3]);
    void makeGroupFollow(const std::vector<CrowdAgent*>& agents, Entity* leader);
    
    // Navigation
    void setNavigationMesh(NavigationMesh* navmesh) { m_navmesh = navmesh; }
    NavigationMesh* getNavigationMesh() { return m_navmesh; }
    
    // Configuration
    void setMaxAgents(int max) { m_maxAgents = max; }
    int getMaxAgents() const { return m_maxAgents; }
    
    void setUpdateFrequency(float hz) { m_updateFrequency = hz; }
    float getUpdateFrequency() const { return m_updateFrequency; }
    
    void setUseMultithreading(bool use) { m_useMultithreading = use; }
    bool usesMultithreading() const { return m_useMultithreading; }
    
    void setSpatialGridSize(float size);
    
    // Quality settings
    void setMaxNeighborChecks(int max) { m_maxNeighborChecks = max; }
    void setNeighborCheckRadius(float radius) { m_neighborCheckRadius = radius; }
    
    // Callbacks
    using AgentCollisionCallback = std::function<void(CrowdAgent*, CrowdAgent*)>;
    void setAgentCollisionCallback(AgentCollisionCallback callback) {
        m_onAgentCollision = callback;
    }
    
    // Statistics
    struct Stats {
        int totalAgents;
        int activeAgents;
        int formationCount;
        float averageSpeed;
        float updateTime;
        int neighborChecks;
    };
    
    Stats getStatistics() const;
    
    // Debug
    void setDebugVisualization(bool enable) { m_debugVisualization = enable; }
    void renderDebug();
    
private:
    std::vector<CrowdAgent*> m_agents;
    std::vector<Formation*> m_formations;
    
    CrowdGrid* m_grid;
    NavigationMesh* m_navmesh;
    
    // Configuration
    int m_maxAgents;
    float m_updateFrequency;
    float m_timeSinceLastUpdate;
    bool m_useMultithreading;
    int m_maxNeighborChecks;
    float m_neighborCheckRadius;
    
    // Callbacks
    AgentCollisionCallback m_onAgentCollision;
    
    // Debug
    bool m_debugVisualization;
    
    // Statistics
    mutable Stats m_stats;
    
    // Internal methods
    void updateNeighbors();
    void updateAgentSteering(CrowdAgent* agent, float deltaTime);
    void resolveCollisions();
    void updateFormations();
    bool raycast(const float from[3], const float to[3], float outHit[3]);
    
    // Formation helpers
    void calculateFormationPositions(Formation* formation, std::vector<float>& positions);
};

// Crowd presets
namespace CrowdPresets {
    CrowdAgentProperties getNormalCitizen();
    CrowdAgentProperties getPanickedCitizen();
    CrowdAgentProperties getSoldier();
    CrowdAgentProperties getZombie();
}

} // namespace AI
} // namespace JJM

#endif // CROWD_SIMULATION_SYSTEM_H
