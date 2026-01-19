#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H

#include "math/Vector2D.h"
#include "physics/PhysicsBody.h"
#include "physics/Collider.h"
#include "ecs/EntityManager.h"
#include <vector>
#include <functional>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace JJM {
namespace Physics {

// Raycast hit result
struct RaycastHit {
    ECS::Entity* entity;
    Math::Vector2D point;
    Math::Vector2D normal;
    float distance;
    Collider* collider;
};

// Layer mask for collision filtering
using LayerMask = uint32_t;

// Collision filter callback
using CollisionFilter = std::function<bool(ECS::Entity*, ECS::Entity*)>;

// Physics configuration
struct PhysicsConfig {
    Math::Vector2D gravity;
    float fixedTimeStep;
    int velocityIterations;
    int positionIterations;
    bool continuousCollision;
    float sleepThreshold;
    
    // Constraint limits
    int maxConstraints;
    float maxConstraintForce;
    float constraintBias;
    float constraintSlop;
    bool warmStarting;
    
    PhysicsConfig()
        : gravity(0.0f, -9.81f)
        , fixedTimeStep(1.0f / 60.0f)
        , velocityIterations(8)
        , positionIterations(3)
        , continuousCollision(true)
        , sleepThreshold(0.01f)
        , maxConstraints(2048)
        , maxConstraintForce(10000.0f)
        , constraintBias(0.2f)
        , constraintSlop(0.005f)
        , warmStarting(true)
    {}
};

// =============================================================================
// Spatial Hashing - Broad Phase Collision Detection
// =============================================================================

/**
 * @brief Cell key for spatial hash grid
 */
struct SpatialCell {
    int x;
    int y;
    
    bool operator==(const SpatialCell& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Hash function for spatial cells
 */
struct SpatialCellHash {
    size_t operator()(const SpatialCell& cell) const {
        // Combine hashes using a prime multiplier
        return std::hash<int>()(cell.x) ^ (std::hash<int>()(cell.y) << 1);
    }
};

/**
 * @brief AABB for spatial hashing
 */
struct SpatialAABB {
    Math::Vector2D min;
    Math::Vector2D max;
    
    SpatialAABB() = default;
    SpatialAABB(const Math::Vector2D& minPoint, const Math::Vector2D& maxPoint)
        : min(minPoint), max(maxPoint) {}
    
    Math::Vector2D getCenter() const {
        return Math::Vector2D((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
    }
    
    Math::Vector2D getExtents() const {
        return Math::Vector2D((max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f);
    }
    
    bool intersects(const SpatialAABB& other) const {
        return !(max.x < other.min.x || min.x > other.max.x ||
                 max.y < other.min.y || min.y > other.max.y);
    }
    
    bool contains(const Math::Vector2D& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y;
    }
    
    void expand(float amount) {
        min.x -= amount;
        min.y -= amount;
        max.x += amount;
        max.y += amount;
    }
    
    static SpatialAABB merge(const SpatialAABB& a, const SpatialAABB& b) {
        return SpatialAABB(
            Math::Vector2D(std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y)),
            Math::Vector2D(std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y))
        );
    }
};

/**
 * @brief Entity entry in spatial hash
 */
struct SpatialEntry {
    ECS::Entity* entity;
    SpatialAABB bounds;
    LayerMask layer;
    bool isStatic;
    
    SpatialEntry()
        : entity(nullptr)
        , layer(0xFFFFFFFF)
        , isStatic(false)
    {}
};

/**
 * @brief Spatial hash grid for broad phase collision detection
 */
class SpatialHashGrid {
private:
    float m_cellSize;
    std::unordered_map<SpatialCell, std::vector<ECS::Entity*>, SpatialCellHash> m_cells;
    std::unordered_map<ECS::Entity*, SpatialEntry> m_entries;
    std::unordered_map<ECS::Entity*, std::vector<SpatialCell>> m_entityCells;
    
    // Statistics
    struct Stats {
        size_t totalEntities;
        size_t totalCells;
        size_t maxEntitiesPerCell;
        size_t queriesPerFrame;
        size_t pairsChecked;
        size_t pairsPassedBroadPhase;
    };
    mutable Stats m_stats;
    
public:
    SpatialHashGrid(float cellSize = 100.0f);
    ~SpatialHashGrid();
    
    // Configuration
    void setCellSize(float size);
    float getCellSize() const { return m_cellSize; }
    
    // Entity management
    void insert(ECS::Entity* entity, const SpatialAABB& bounds, LayerMask layer = 0xFFFFFFFF, bool isStatic = false);
    void remove(ECS::Entity* entity);
    void update(ECS::Entity* entity, const SpatialAABB& newBounds);
    void clear();
    
    // Bulk operations
    void rebuild();
    void optimizeCellSize();
    
    // Queries
    std::vector<ECS::Entity*> query(const SpatialAABB& bounds) const;
    std::vector<ECS::Entity*> queryPoint(const Math::Vector2D& point) const;
    std::vector<ECS::Entity*> queryCircle(const Math::Vector2D& center, float radius) const;
    std::vector<ECS::Entity*> queryRay(const Math::Vector2D& origin, const Math::Vector2D& direction, float maxDistance) const;
    
    // Collision pairs
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs() const;
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs(LayerMask mask) const;
    
    // Neighbor queries
    std::vector<ECS::Entity*> getNeighbors(ECS::Entity* entity) const;
    std::vector<ECS::Entity*> getNearestEntities(const Math::Vector2D& point, int count) const;
    ECS::Entity* getNearestEntity(const Math::Vector2D& point, LayerMask mask = 0xFFFFFFFF) const;
    
    // Statistics
    const Stats& getStats() const { return m_stats; }
    void resetStats();
    size_t getEntityCount() const { return m_entries.size(); }
    size_t getCellCount() const { return m_cells.size(); }
    
    // Debug
    std::vector<std::pair<SpatialCell, size_t>> getCellOccupancy() const;
    
private:
    SpatialCell worldToCell(const Math::Vector2D& position) const;
    std::vector<SpatialCell> getCellsForBounds(const SpatialAABB& bounds) const;
    void addToCell(const SpatialCell& cell, ECS::Entity* entity);
    void removeFromCell(const SpatialCell& cell, ECS::Entity* entity);
};

/**
 * @brief Hierarchical grid for multi-resolution spatial hashing
 */
class HierarchicalSpatialGrid {
private:
    std::vector<std::unique_ptr<SpatialHashGrid>> m_levels;
    std::vector<float> m_levelSizes;
    
public:
    HierarchicalSpatialGrid(const std::vector<float>& levelSizes = {25.0f, 100.0f, 400.0f});
    
    void insert(ECS::Entity* entity, const SpatialAABB& bounds, LayerMask layer = 0xFFFFFFFF);
    void remove(ECS::Entity* entity);
    void update(ECS::Entity* entity, const SpatialAABB& newBounds);
    void clear();
    
    std::vector<ECS::Entity*> query(const SpatialAABB& bounds) const;
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs() const;
    
private:
    int selectLevel(const SpatialAABB& bounds) const;
};

/**
 * @brief Sweep and prune algorithm for broad phase
 */
class SweepAndPrune {
private:
    struct Endpoint {
        float value;
        ECS::Entity* entity;
        bool isMin;
        int axis;
    };
    
    std::vector<Endpoint> m_xEndpoints;
    std::vector<Endpoint> m_yEndpoints;
    std::unordered_map<ECS::Entity*, SpatialEntry> m_entries;
    std::unordered_set<std::pair<ECS::Entity*, ECS::Entity*>*> m_overlappingPairs;
    
public:
    SweepAndPrune();
    
    void insert(ECS::Entity* entity, const SpatialAABB& bounds);
    void remove(ECS::Entity* entity);
    void update(ECS::Entity* entity, const SpatialAABB& newBounds);
    
    std::vector<std::pair<ECS::Entity*, ECS::Entity*>> getPotentialPairs() const;
    
private:
    void sortEndpoints();
    void updateOverlaps();
};

class PhysicsWorld {
private:
    Math::Vector2D gravity;
    ECS::EntityManager* entityManager;
    std::vector<CollisionInfo> collisions;
    PhysicsConfig config;
    
    // Layer collision matrix
    std::vector<std::vector<bool>> layerMatrix;
    static constexpr int MAX_LAYERS = 32;
    
    // Collision callbacks
    std::function<void(const CollisionInfo&)> onCollisionEnter;
    std::function<void(const CollisionInfo&)> onCollisionStay;
    std::function<void(const CollisionInfo&)> onCollisionExit;
    
    // Time accumulator for fixed timestep
    float timeAccumulator;
    
    // Spatial partitioning
    std::unique_ptr<SpatialHashGrid> m_spatialHash;
    bool m_useSpatialHashing;

public:
    PhysicsWorld(ECS::EntityManager* em);
    ~PhysicsWorld();

    // Configuration
    void setConfig(const PhysicsConfig& cfg);
    const PhysicsConfig& getConfig() const { return config; }

    // Update physics
    void update(float deltaTime);
    void fixedUpdate();
    
    // Gravity
    void setGravity(const Math::Vector2D& g) { gravity = g; config.gravity = g; }
    Math::Vector2D getGravity() const { return gravity; }
    
    // Spatial hashing
    void enableSpatialHashing(bool enable, float cellSize = 100.0f);
    bool isSpatialHashingEnabled() const { return m_useSpatialHashing; }
    SpatialHashGrid* getSpatialHash() { return m_spatialHash.get(); }
    void rebuildSpatialHash();
    
    // Collision detection
    void detectCollisions();
    void resolveCollisions();
    
    // Broadphase optimization statistics
    struct BroadphaseStats {
        int totalBodies;
        int activeCells;
        int potentialPairs;
        int actualCollisions;
        float broadphaseTimeMs;
        float narrowphaseTimeMs;
        
        BroadphaseStats() 
            : totalBodies(0), activeCells(0), potentialPairs(0), 
              actualCollisions(0), broadphaseTimeMs(0.0f), narrowphaseTimeMs(0.0f) {}
    };
    
    BroadphaseStats getBroadphaseStats() const { return m_broadphaseStats; }
    void resetBroadphaseStats() { m_broadphaseStats = BroadphaseStats(); }
    
    // Raycasting
    std::optional<RaycastHit> raycast(const Math::Vector2D& origin, 
                                       const Math::Vector2D& direction,
                                       float maxDistance,
                                       LayerMask layerMask = 0xFFFFFFFF) const;
    std::vector<RaycastHit> raycastAll(const Math::Vector2D& origin,
                                        const Math::Vector2D& direction,
                                        float maxDistance,
                                        LayerMask layerMask = 0xFFFFFFFF) const;
    bool linecast(const Math::Vector2D& start, 
                  const Math::Vector2D& end,
                  LayerMask layerMask = 0xFFFFFFFF) const;
    
    // Shape queries
    std::vector<ECS::Entity*> queryPoint(const Math::Vector2D& point);
    std::vector<ECS::Entity*> queryArea(const Math::Vector2D& min, const Math::Vector2D& max);
    std::vector<ECS::Entity*> queryCircle(const Math::Vector2D& center, float radius);
    std::vector<ECS::Entity*> overlapBox(const Math::Vector2D& center, 
                                          const Math::Vector2D& halfExtents,
                                          float angle = 0.0f);
    
    // Layer management
    void setLayerCollision(int layer1, int layer2, bool shouldCollide);
    bool getLayerCollision(int layer1, int layer2) const;
    void resetLayerMatrix();
    
    // Collision callbacks
    void setOnCollisionEnter(std::function<void(const CollisionInfo&)> callback);
    void setOnCollisionStay(std::function<void(const CollisionInfo&)> callback);
    void setOnCollisionExit(std::function<void(const CollisionInfo&)> callback);
    
    // Physics queries
    Math::Vector2D getClosestPoint(const Math::Vector2D& point, const Collider& collider) const;
    float getDistance(ECS::Entity* entityA, ECS::Entity* entityB) const;
    bool checkCollision(ECS::Entity* entityA, ECS::Entity* entityB) const;
    
    // Force application
    void applyExplosionForce(const Math::Vector2D& center, float force, float radius);
    void applyForceInArea(const Math::Vector2D& min, const Math::Vector2D& max, 
                          const Math::Vector2D& force);
    
private:
    void applyGravity(float deltaTime);
    void resolveCollision(const CollisionInfo& info);
    bool rayIntersectsCollider(const Math::Vector2D& origin,
                               const Math::Vector2D& direction,
                               const Collider& collider,
                               RaycastHit& outHit) const;
    
    // Broadphase statistics tracking
    BroadphaseStats m_broadphaseStats;
};

// =============================================================================
// PHYSICS JOINTS AND CONSTRAINTS
// =============================================================================

/**
 * @brief Joint types
 */
enum class JointType {
    Distance,       // Maintains fixed distance between bodies
    Revolute,       // Hinge joint with rotation around anchor
    Prismatic,      // Slider joint with linear motion
    Spring,         // Spring connection with damping
    Rope,           // Maximum distance constraint (slack allowed)
    Weld,           // Rigidly connects two bodies
    Wheel,          // For vehicle wheels
    Pulley,         // Pulley system between bodies
    Gear,           // Links rotation of two revolute joints
    Motor           // Applies angular velocity
};

/**
 * @brief Joint limit state
 */
enum class JointLimitState {
    Inactive,
    AtLower,
    AtUpper,
    Equal
};

/**
 * @brief Base joint configuration
 */
struct JointConfig {
    ECS::Entity* bodyA{nullptr};
    ECS::Entity* bodyB{nullptr};
    Math::Vector2D anchorA;         // Local anchor on body A
    Math::Vector2D anchorB;         // Local anchor on body B
    bool collideConnected{false};   // Allow connected bodies to collide
    float breakForce{0.0f};         // 0 = unbreakable
    float breakTorque{0.0f};
    void* userData{nullptr};
};

/**
 * @brief Distance joint keeps bodies at fixed distance
 */
struct DistanceJointConfig : public JointConfig {
    float length{-1.0f};            // -1 = auto-compute from initial positions
    float minLength{0.0f};          // 0 = no min
    float maxLength{0.0f};          // 0 = no max (uses length)
    float stiffness{0.0f};          // 0 = rigid, >0 = spring-like
    float damping{0.0f};
};

/**
 * @brief Revolute (hinge) joint allows rotation around anchor
 */
struct RevoluteJointConfig : public JointConfig {
    float referenceAngle{0.0f};
    bool enableLimit{false};
    float lowerAngle{0.0f};
    float upperAngle{0.0f};
    bool enableMotor{false};
    float motorSpeed{0.0f};
    float maxMotorTorque{0.0f};
};

/**
 * @brief Prismatic (slider) joint allows linear motion along axis
 */
struct PrismaticJointConfig : public JointConfig {
    Math::Vector2D axis{1.0f, 0.0f}; // Local axis on body A
    float referenceAngle{0.0f};
    bool enableLimit{false};
    float lowerTranslation{0.0f};
    float upperTranslation{0.0f};
    bool enableMotor{false};
    float motorSpeed{0.0f};
    float maxMotorForce{0.0f};
};

/**
 * @brief Spring joint with stiffness and damping
 */
struct SpringJointConfig : public JointConfig {
    float restLength{-1.0f};        // -1 = auto-compute
    float stiffness{100.0f};        // Spring constant (N/m)
    float damping{1.0f};            // Damping coefficient
    float minLength{0.0f};
    float maxLength{0.0f};          // 0 = unlimited
};

/**
 * @brief Wheel joint for vehicles
 */
struct WheelJointConfig : public JointConfig {
    Math::Vector2D axis{0.0f, 1.0f}; // Suspension axis
    float suspensionStiffness{50.0f};
    float suspensionDamping{5.0f};
    float maxSuspensionForce{1000.0f};
    bool enableMotor{false};
    float motorSpeed{0.0f};
    float maxMotorTorque{100.0f};
};

/**
 * @brief Abstract joint base class
 */
class Joint {
public:
    virtual ~Joint() = default;
    
    JointType getType() const { return m_type; }
    ECS::Entity* getBodyA() const { return m_bodyA; }
    ECS::Entity* getBodyB() const { return m_bodyB; }
    
    // Forces and reaction
    virtual Math::Vector2D getReactionForce(float invDt) const = 0;
    virtual float getReactionTorque(float invDt) const = 0;
    
    // Breaking
    void setBreakForce(float force) { m_breakForce = force; }
    void setBreakTorque(float torque) { m_breakTorque = torque; }
    bool isBroken() const { return m_broken; }
    
    // State
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    // User data
    void* getUserData() const { return m_userData; }
    void setUserData(void* data) { m_userData = data; }
    
protected:
    Joint(JointType type, ECS::Entity* bodyA, ECS::Entity* bodyB)
        : m_type(type), m_bodyA(bodyA), m_bodyB(bodyB) {}
    
    JointType m_type;
    ECS::Entity* m_bodyA;
    ECS::Entity* m_bodyB;
    float m_breakForce{0.0f};
    float m_breakTorque{0.0f};
    bool m_broken{false};
    bool m_enabled{true};
    void* m_userData{nullptr};
};

/**
 * @brief Distance joint implementation
 */
class DistanceJoint : public Joint {
public:
    DistanceJoint(const DistanceJointConfig& config);
    
    float getLength() const { return m_length; }
    void setLength(float length) { m_length = length; }
    
    float getMinLength() const { return m_minLength; }
    void setMinLength(float length) { m_minLength = length; }
    float getMaxLength() const { return m_maxLength; }
    void setMaxLength(float length) { m_maxLength = length; }
    
    float getStiffness() const { return m_stiffness; }
    void setStiffness(float stiffness) { m_stiffness = stiffness; }
    float getDamping() const { return m_damping; }
    void setDamping(float damping) { m_damping = damping; }
    
    float getCurrentLength() const;
    Math::Vector2D getReactionForce(float invDt) const override;
    float getReactionTorque(float invDt) const override { return 0.0f; }
    
private:
    Math::Vector2D m_anchorA, m_anchorB;
    float m_length;
    float m_minLength, m_maxLength;
    float m_stiffness, m_damping;
    float m_impulse{0.0f};
};

/**
 * @brief Revolute joint implementation
 */
class RevoluteJoint : public Joint {
public:
    RevoluteJoint(const RevoluteJointConfig& config);
    
    float getJointAngle() const;
    float getJointSpeed() const;
    
    bool isLimitEnabled() const { return m_enableLimit; }
    void enableLimit(bool enable) { m_enableLimit = enable; }
    void setLimits(float lower, float upper);
    float getLowerLimit() const { return m_lowerAngle; }
    float getUpperLimit() const { return m_upperAngle; }
    
    bool isMotorEnabled() const { return m_enableMotor; }
    void enableMotor(bool enable) { m_enableMotor = enable; }
    void setMotorSpeed(float speed) { m_motorSpeed = speed; }
    float getMotorSpeed() const { return m_motorSpeed; }
    void setMaxMotorTorque(float torque) { m_maxMotorTorque = torque; }
    float getMotorTorque(float invDt) const;
    
    Math::Vector2D getReactionForce(float invDt) const override;
    float getReactionTorque(float invDt) const override;
    
private:
    Math::Vector2D m_anchorA, m_anchorB;
    float m_referenceAngle;
    bool m_enableLimit;
    float m_lowerAngle, m_upperAngle;
    bool m_enableMotor;
    float m_motorSpeed;
    float m_maxMotorTorque;
    JointLimitState m_limitState{JointLimitState::Inactive};
};

/**
 * @brief Spring joint implementation
 */
class SpringJoint : public Joint {
public:
    SpringJoint(const SpringJointConfig& config);
    
    float getRestLength() const { return m_restLength; }
    void setRestLength(float length) { m_restLength = length; }
    
    float getStiffness() const { return m_stiffness; }
    void setStiffness(float k) { m_stiffness = k; }
    
    float getDamping() const { return m_damping; }
    void setDamping(float b) { m_damping = b; }
    
    float getCurrentLength() const;
    float getCurrentForce() const;
    
    Math::Vector2D getReactionForce(float invDt) const override;
    float getReactionTorque(float invDt) const override { return 0.0f; }
    
private:
    Math::Vector2D m_anchorA, m_anchorB;
    float m_restLength;
    float m_stiffness;
    float m_damping;
    float m_minLength, m_maxLength;
};

/**
 * @brief Joint manager
 */
class JointManager {
public:
    JointManager(PhysicsWorld& world);
    ~JointManager();
    
    // Create joints
    DistanceJoint* createDistanceJoint(const DistanceJointConfig& config);
    RevoluteJoint* createRevoluteJoint(const RevoluteJointConfig& config);
    SpringJoint* createSpringJoint(const SpringJointConfig& config);
    
    // Destroy joints
    void destroyJoint(Joint* joint);
    void destroyAllJoints();
    void destroyJointsForBody(ECS::Entity* body);
    
    // Query
    std::vector<Joint*> getJointsForBody(ECS::Entity* body) const;
    size_t getJointCount() const { return m_joints.size(); }
    
    // Update
    void solveVelocityConstraints(float dt);
    void solvePositionConstraints();
    void checkBreakage();
    
    // Callbacks
    using JointBreakCallback = std::function<void(Joint*)>;
    void setOnJointBreak(JointBreakCallback callback) { m_onBreak = callback; }
    
private:
    PhysicsWorld& m_world;
    std::vector<std::unique_ptr<Joint>> m_joints;
    JointBreakCallback m_onBreak;
};

// =============================================================================
// CONTINUOUS COLLISION DETECTION
// =============================================================================

/**
 * @brief Time of impact result
 */
struct TOIResult {
    bool hit{false};
    float t{1.0f};                  // Time of impact (0-1 of timestep)
    Math::Vector2D point;
    Math::Vector2D normal;
    ECS::Entity* entityA{nullptr};
    ECS::Entity* entityB{nullptr};
};

/**
 * @brief Swept shape for CCD
 */
struct SweptShape {
    ECS::Entity* entity;
    Math::Vector2D startPos;
    Math::Vector2D endPos;
    float startAngle;
    float endAngle;
    SpatialAABB sweptBounds;
};

/**
 * @brief Continuous collision detector
 */
class CCDSolver {
public:
    CCDSolver();
    
    // Time of impact calculation
    TOIResult calculateTOI(const SweptShape& shapeA, const SweptShape& shapeB);
    
    // Swept collision test
    bool sweptAABBTest(const SpatialAABB& aStart, const SpatialAABB& aEnd,
                       const SpatialAABB& bStart, const SpatialAABB& bEnd,
                       float& outT);
    
    bool sweptCircleCircle(const Math::Vector2D& aStart, const Math::Vector2D& aEnd, float radiusA,
                           const Math::Vector2D& bStart, const Math::Vector2D& bEnd, float radiusB,
                           float& outT, Math::Vector2D& outPoint, Math::Vector2D& outNormal);
    
    bool sweptCircleAABB(const Math::Vector2D& circleStart, const Math::Vector2D& circleEnd, float radius,
                         const SpatialAABB& box,
                         float& outT, Math::Vector2D& outPoint, Math::Vector2D& outNormal);
    
    // Configuration
    void setMaxIterations(int iterations) { m_maxIterations = iterations; }
    void setTolerance(float tolerance) { m_tolerance = tolerance; }
    
private:
    int m_maxIterations{20};
    float m_tolerance{0.0001f};
};

// =============================================================================
// TRIGGER VOLUMES
// =============================================================================

/**
 * @brief Trigger volume shape type
 */
enum class TriggerShape {
    Box,
    Circle,
    Polygon
};

/**
 * @brief Trigger volume for detection without physics response
 */
class TriggerVolume {
public:
    TriggerVolume(const std::string& name = "");
    ~TriggerVolume();
    
    // Shape setup
    void setAsBox(const Math::Vector2D& center, const Math::Vector2D& halfExtents, float angle = 0.0f);
    void setAsCircle(const Math::Vector2D& center, float radius);
    void setAsPolygon(const std::vector<Math::Vector2D>& vertices);
    
    // Transform
    void setPosition(const Math::Vector2D& pos);
    void setRotation(float angle);
    void setScale(float scale);
    Math::Vector2D getPosition() const { return m_position; }
    float getRotation() const { return m_rotation; }
    
    // Layer filtering
    void setLayerMask(LayerMask mask) { m_layerMask = mask; }
    LayerMask getLayerMask() const { return m_layerMask; }
    
    // State
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Collision checks
    bool containsPoint(const Math::Vector2D& point) const;
    bool intersects(const SpatialAABB& bounds) const;
    bool intersects(const TriggerVolume& other) const;
    
    // Entity tracking
    const std::unordered_set<ECS::Entity*>& getEntitiesInside() const { return m_entitiesInside; }
    bool isEntityInside(ECS::Entity* entity) const;
    
    // Callbacks
    using TriggerCallback = std::function<void(TriggerVolume*, ECS::Entity*)>;
    void setOnEnter(TriggerCallback callback) { m_onEnter = callback; }
    void setOnStay(TriggerCallback callback) { m_onStay = callback; }
    void setOnExit(TriggerCallback callback) { m_onExit = callback; }
    
    // User data
    const std::string& getName() const { return m_name; }
    void setUserData(void* data) { m_userData = data; }
    void* getUserData() const { return m_userData; }
    
    // Internal update
    void updateEntityTracking(ECS::Entity* entity, bool isInside);
    
private:
    std::string m_name;
    TriggerShape m_shape{TriggerShape::Box};
    Math::Vector2D m_position;
    float m_rotation{0.0f};
    float m_scale{1.0f};
    
    // Shape data
    Math::Vector2D m_halfExtents;   // For box
    float m_radius{0.0f};           // For circle
    std::vector<Math::Vector2D> m_vertices;  // For polygon
    
    LayerMask m_layerMask{0xFFFFFFFF};
    bool m_enabled{true};
    
    std::unordered_set<ECS::Entity*> m_entitiesInside;
    
    TriggerCallback m_onEnter;
    TriggerCallback m_onStay;
    TriggerCallback m_onExit;
    
    void* m_userData{nullptr};
};

/**
 * @brief Trigger volume manager
 */
class TriggerManager {
public:
    TriggerManager(PhysicsWorld& world);
    ~TriggerManager();
    
    // Create/destroy triggers
    TriggerVolume* createTrigger(const std::string& name = "");
    void destroyTrigger(TriggerVolume* trigger);
    void destroyTrigger(const std::string& name);
    void destroyAllTriggers();
    
    // Query
    TriggerVolume* getTrigger(const std::string& name);
    std::vector<TriggerVolume*> getTriggersAtPoint(const Math::Vector2D& point);
    std::vector<TriggerVolume*> getTriggersInArea(const SpatialAABB& area);
    size_t getTriggerCount() const { return m_triggers.size(); }
    
    // Update (call each physics step)
    void update();
    
    // Batch callbacks
    using GlobalTriggerCallback = std::function<void(TriggerVolume*, ECS::Entity*, bool enter)>;
    void setGlobalCallback(GlobalTriggerCallback callback) { m_globalCallback = callback; }
    
private:
    PhysicsWorld& m_world;
    std::vector<std::unique_ptr<TriggerVolume>> m_triggers;
    std::unordered_map<std::string, TriggerVolume*> m_namedTriggers;
    GlobalTriggerCallback m_globalCallback;
};

} // namespace Physics
} // namespace JJM

#endif // PHYSICS_WORLD_H
