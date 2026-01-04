#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// IK (Inverse Kinematics) system for procedural animation
namespace Engine {

struct IKJoint {
    std::string name;
    int jointId;
    int parentId;
    
    float x, y, z;          // Current position
    float length;           // Bone length
    float minAngle;         // Min rotation angle (degrees)
    float maxAngle;         // Max rotation angle (degrees)
    
    bool isEndEffector;
};

enum class IKSolverType {
    CCD,            // Cyclic Coordinate Descent
    FABRIK,         // Forward And Backward Reaching IK
    TwoJoint,       // Analytic two-joint solver
    Jacobian        // Jacobian-based solver
};

class IKChain {
public:
    IKChain();
    ~IKChain();

    // Chain setup
    void addJoint(const IKJoint& joint);
    void setEndEffector(int jointId);
    void setRootJoint(int jointId);
    
    // Constraints
    void setJointLimits(int jointId, float minAngle, float maxAngle);
    void setPoleTarget(float x, float y, float z);
    void enablePoleTarget(bool enable) { m_usePoleTarget = enable; }
    
    // Solver configuration
    void setSolverType(IKSolverType type) { m_solverType = type; }
    void setMaxIterations(int iterations) { m_maxIterations = iterations; }
    void setTolerance(float tolerance) { m_tolerance = tolerance; }
    void setWeight(float weight) { m_weight = weight; }
    
    // Solving
    bool solve(float targetX, float targetY, float targetZ);
    bool solveWithHint(float targetX, float targetY, float targetZ, 
                       float hintX, float hintY, float hintZ);
    
    // Query
    const IKJoint& getJoint(int index) const { return m_joints[index]; }
    int getJointCount() const { return static_cast<int>(m_joints.size()); }
    int getEndEffectorId() const { return m_endEffectorId; }
    
    // Results
    void getJointPosition(int jointId, float& x, float& y, float& z) const;
    void getJointRotation(int jointId, float& angle) const;

private:
    bool solveCCD(float targetX, float targetY, float targetZ);
    bool solveFABRIK(float targetX, float targetY, float targetZ);
    bool solveTwoJoint(float targetX, float targetY, float targetZ);
    bool solveJacobian(float targetX, float targetY, float targetZ);
    
    void forwardReach(float targetX, float targetY, float targetZ);
    void backwardReach();
    
    void applyJointLimits(int jointId);
    float calculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const;
    
    std::vector<IKJoint> m_joints;
    int m_rootJointId;
    int m_endEffectorId;
    
    IKSolverType m_solverType;
    int m_maxIterations;
    float m_tolerance;
    float m_weight;
    
    bool m_usePoleTarget;
    float m_poleTargetX, m_poleTargetY, m_poleTargetZ;
};

class IKSystem {
public:
    IKSystem();
    ~IKSystem();

    // Chain management
    int createChain(const std::string& name);
    void destroyChain(int chainId);
    IKChain* getChain(int chainId);
    IKChain* getChainByName(const std::string& name);
    
    // Common IK setups
    int createLegChain(const std::string& name, float thighLength, float shinLength);
    int createArmChain(const std::string& name, float upperArmLength, float forearmLength);
    int createSpineChain(const std::string& name, int vertebraeCount, float segmentLength);
    
    // Update
    void update(float deltaTime);
    
    // Global settings
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

private:
    std::map<int, IKChain*> m_chains;
    std::map<std::string, int> m_chainNames;
    int m_nextChainId;
    bool m_enabled;
};

// Helper class for foot IK (keeping feet planted on ground)
class FootIK {
public:
    FootIK();
    
    // Setup
    void setLegChain(IKChain* chain) { m_legChain = chain; }
    void setFootHeight(float height) { m_footHeight = height; }
    void setMaxReach(float reach) { m_maxReach = reach; }
    
    // Ground detection
    void setGroundHeight(float height) { m_groundHeight = height; }
    void enableRaycast(bool enable) { m_useRaycast = enable; }
    
    // Update
    void update(float hipX, float hipY, float hipZ, float deltaTime);
    
    // Query
    void getFootPosition(float& x, float& y, float& z) const;
    bool isPlanted() const { return m_isPlanted; }

private:
    IKChain* m_legChain;
    
    float m_footHeight;
    float m_maxReach;
    float m_groundHeight;
    
    bool m_useRaycast;
    bool m_isPlanted;
    
    float m_footX, m_footY, m_footZ;
    float m_targetX, m_targetY, m_targetZ;
};

// Helper for look-at IK (head/eyes tracking targets)
class LookAtIK {
public:
    LookAtIK();
    
    // Setup
    void setHeadJoint(int jointId) { m_headJointId = jointId; }
    void setNeckJoint(int jointId) { m_neckJointId = jointId; }
    void setEyeJoints(int leftEyeId, int rightEyeId);
    
    // Configuration
    void setMaxAngle(float degrees) { m_maxAngle = degrees; }
    void setWeight(float weight) { m_weight = weight; }
    void setSmoothTime(float time) { m_smoothTime = time; }
    
    // Target
    void setTarget(float x, float y, float z);
    void clearTarget();
    
    // Update
    void update(float deltaTime);
    
    // Query
    bool hasTarget() const { return m_hasTarget; }
    void getTargetDirection(float& x, float& y, float& z) const;

private:
    int m_headJointId;
    int m_neckJointId;
    int m_leftEyeId;
    int m_rightEyeId;
    
    float m_maxAngle;
    float m_weight;
    float m_smoothTime;
    
    bool m_hasTarget;
    float m_targetX, m_targetY, m_targetZ;
    float m_currentX, m_currentY, m_currentZ;
};

// Two-bone IK solver (common for arms/legs)
class TwoBoneIK {
public:
    static bool solve(
        float rootX, float rootY, float rootZ,
        float midX, float midY, float midZ,
        float endX, float endY, float endZ,
        float targetX, float targetY, float targetZ,
        float poleX, float poleY, float poleZ,
        float& outMidX, float& outMidY, float& outMidZ,
        float& outEndX, float& outEndY, float& outEndZ
    );
    
    static bool solveAnalytic(
        float upperLength,
        float lowerLength,
        float targetDistance,
        float& upperAngle,
        float& lowerAngle
    );
};

// =============================================================================
// ADVANCED IK CONSTRAINT SYSTEM
// =============================================================================

/**
 * @brief Joint constraint types
 */
enum class IKConstraintType {
    None,
    Hinge,          // Single axis rotation (elbow, knee)
    BallAndSocket,  // Spherical rotation with cone limits (shoulder, hip)
    Twist,          // Rotation around bone axis
    Planar,         // Movement restricted to a plane
    Distance,       // Distance constraint between joints
    Custom
};

/**
 * @brief Rotation limit types
 */
struct RotationLimit {
    float minX{-180.0f}, maxX{180.0f};
    float minY{-180.0f}, maxY{180.0f};
    float minZ{-180.0f}, maxZ{180.0f};
    
    static RotationLimit Hinge(float minAngle, float maxAngle) {
        return {minAngle, maxAngle, 0.0f, 0.0f, 0.0f, 0.0f};
    }
    
    static RotationLimit Cone(float coneAngle) {
        return {-coneAngle, coneAngle, -coneAngle, coneAngle, -180.0f, 180.0f};
    }
    
    static RotationLimit Ellipse(float xAngle, float yAngle) {
        return {-xAngle, xAngle, -yAngle, yAngle, -180.0f, 180.0f};
    }
};

/**
 * @brief Advanced joint constraint
 */
struct IKConstraint {
    int jointId;
    IKConstraintType type{IKConstraintType::None};
    RotationLimit rotationLimit;
    
    // Hinge constraint
    float hingeAxis[3]{0.0f, 1.0f, 0.0f};    // Axis of rotation
    
    // Ball and socket constraint
    float coneAxis[3]{0.0f, 0.0f, 1.0f};     // Primary direction
    float coneAngle{45.0f};                   // Cone half-angle
    float twistMin{-90.0f}, twistMax{90.0f}; // Twist limits
    
    // Soft constraints
    float stiffness{1.0f};          // 0-1, how strongly to enforce
    float damping{0.1f};            // Damping factor
    
    // Preferred rotation (rest pose)
    float preferredRotation[4]{0.0f, 0.0f, 0.0f, 1.0f};  // Quaternion
    float preferredWeight{0.0f};    // How much to prefer rest pose
};

/**
 * @brief Constraint solver for IK
 */
class IKConstraintSolver {
public:
    IKConstraintSolver() = default;
    
    void addConstraint(const IKConstraint& constraint);
    void removeConstraint(int jointId);
    void clearConstraints();
    
    // Apply constraints to joint rotation
    void applyConstraints(int jointId, float& rotX, float& rotY, float& rotZ) const;
    void applyConstraintQuaternion(int jointId, float* quaternion) const;
    
    // Soft constraint solving
    void solveIterative(std::vector<IKJoint>& joints, int iterations = 10);
    
    // Preset constraint profiles
    void applyHumanoidConstraints();
    void applyQuadrupedConstraints();
    
    // Query
    const IKConstraint* getConstraint(int jointId) const;
    bool hasConstraint(int jointId) const;

private:
    std::map<int, IKConstraint> m_constraints;
    
    void projectToConeSurface(float* direction, const float* axis, float coneAngle) const;
    void clampTwist(float* rotation, float minTwist, float maxTwist) const;
};

// =============================================================================
// MULTI-TARGET IK SYSTEM
// =============================================================================

/**
 * @brief IK target with priority and blending
 */
struct IKTarget {
    std::string name;
    int chainId{-1};
    
    // Target position
    float positionX{0.0f}, positionY{0.0f}, positionZ{0.0f};
    bool hasPosition{true};
    
    // Target rotation (optional)
    float rotationX{0.0f}, rotationY{0.0f}, rotationZ{0.0f}, rotationW{1.0f};
    bool hasRotation{false};
    
    // Priority and blending
    float weight{1.0f};             // 0-1 blend weight
    int priority{0};                // Higher = solved first
    
    // Interpolation
    float blendSpeed{10.0f};        // Units per second for interpolation
    bool instantMove{false};        // Skip interpolation
    
    // Active state
    bool enabled{true};
    float activeWeight{0.0f};       // Current interpolated weight
};

/**
 * @brief Multi-target IK coordinator
 */
class MultiTargetIK {
public:
    MultiTargetIK(IKSystem& ikSystem);
    
    // Target management
    int addTarget(const IKTarget& target);
    void removeTarget(int targetId);
    void removeTarget(const std::string& name);
    IKTarget* getTarget(int targetId);
    IKTarget* getTarget(const std::string& name);
    
    // Batch target setting
    void setTargetPosition(int targetId, float x, float y, float z);
    void setTargetRotation(int targetId, float x, float y, float z, float w);
    void setTargetWeight(int targetId, float weight);
    void setTargetEnabled(int targetId, bool enabled);
    
    // Solving
    void solve(float deltaTime);
    void solveImmediate();   // Solve without interpolation
    
    // Priority handling
    void setPrioritySorting(bool enabled) { m_usePriority = enabled; }
    
    // Statistics
    struct SolveStats {
        int targetsProcessed;
        int iterationsUsed;
        float averageError;
        float solveTime;
    };
    SolveStats getLastSolveStats() const { return m_lastStats; }
    
private:
    IKSystem& m_ikSystem;
    std::map<int, IKTarget> m_targets;
    int m_nextTargetId{0};
    bool m_usePriority{true};
    SolveStats m_lastStats{};
    
    void interpolateTarget(IKTarget& target, float deltaTime);
    void sortByPriority(std::vector<int>& targetIds);
};

// =============================================================================
// FULL-BODY IK SYSTEM
// =============================================================================

/**
 * @brief Body part identifiers for full-body IK
 */
enum class BodyPart {
    Root,
    Spine,
    Head,
    LeftArm,
    RightArm,
    LeftHand,
    RightHand,
    LeftLeg,
    RightLeg,
    LeftFoot,
    RightFoot
};

/**
 * @brief Full-body IK goal
 */
struct FBIKGoal {
    BodyPart bodyPart;
    float positionX{0.0f}, positionY{0.0f}, positionZ{0.0f};
    float rotationX{0.0f}, rotationY{0.0f}, rotationZ{0.0f}, rotationW{1.0f};
    float positionWeight{1.0f};
    float rotationWeight{1.0f};
    bool usePosition{true};
    bool useRotation{false};
};

/**
 * @brief Full-body IK configuration
 */
struct FBIKConfig {
    // Spine configuration
    int spineIterations{3};
    float spineBendWeight{0.5f};
    float spineTwistWeight{0.5f};
    
    // Arm/leg pull
    float armPull{0.5f};
    float legPull{0.5f};
    
    // Root motion
    bool allowRootMotion{true};
    float rootPositionWeight{0.5f};
    float rootRotationWeight{0.5f};
    
    // Quality settings
    int iterations{10};
    float tolerance{0.001f};
    float damping{0.95f};
};

/**
 * @brief Full-body inverse kinematics system
 */
class FullBodyIK {
public:
    FullBodyIK();
    ~FullBodyIK();
    
    // Setup
    void setSkeleton(void* skeletonPtr);  // Link to skeletal animation
    void setJointMapping(BodyPart part, int jointId);
    void setJointMapping(BodyPart part, const std::string& jointName);
    void autoDetectMapping();   // Try to auto-map common bone names
    
    // Configuration
    void setConfig(const FBIKConfig& config);
    const FBIKConfig& getConfig() const { return m_config; }
    
    // Goals
    void setGoal(const FBIKGoal& goal);
    void clearGoal(BodyPart bodyPart);
    void clearAllGoals();
    
    // Convenience methods
    void setHandTarget(bool isLeft, float x, float y, float z, float weight = 1.0f);
    void setFootTarget(bool isLeft, float x, float y, float z, float weight = 1.0f);
    void setHeadTarget(float lookX, float lookY, float lookZ, float weight = 1.0f);
    void setRootPosition(float x, float y, float z);
    
    // Solving
    void solve(float deltaTime);
    void solveImmediate();
    
    // Constraints
    void setConstraintSolver(IKConstraintSolver* solver) { m_constraintSolver = solver; }
    
    // Quality vs performance
    void setQualityMode(int iterations) { m_config.iterations = iterations; }
    void setLowQualityMode() { m_config.iterations = 3; }
    void setHighQualityMode() { m_config.iterations = 15; }
    
    // Results
    void getJointTransform(int jointId, float* position, float* rotation) const;
    void getBodyPartTransform(BodyPart part, float* position, float* rotation) const;
    
    // Debug
    bool isValid() const { return m_isValid; }
    std::string getLastError() const { return m_lastError; }

private:
    struct JointState {
        float position[3];
        float rotation[4];  // Quaternion
        float localPosition[3];
        float localRotation[4];
        int parentIndex;
    };
    
    // Skeleton data
    void* m_skeleton{nullptr};
    std::map<BodyPart, int> m_jointMapping;
    std::vector<JointState> m_jointStates;
    
    // IK chains
    IKChain m_leftArmChain;
    IKChain m_rightArmChain;
    IKChain m_leftLegChain;
    IKChain m_rightLegChain;
    IKChain m_spineChain;
    
    // Goals and config
    std::map<BodyPart, FBIKGoal> m_goals;
    FBIKConfig m_config;
    
    // Constraints
    IKConstraintSolver* m_constraintSolver{nullptr};
    
    // State
    bool m_isValid{false};
    std::string m_lastError;
    
    // Internal methods
    void buildChains();
    void solveLimbs();
    void solveSpine();
    void solveRoot();
    void applyResults();
};

// =============================================================================
// IK BLENDING AND LAYERS
// =============================================================================

/**
 * @brief IK blend mode
 */
enum class IKBlendMode {
    Override,       // IK completely overrides animation
    Additive,       // IK adds to animation
    Weighted        // Weighted blend between IK and animation
};

/**
 * @brief IK layer for layered IK solving
 */
struct IKLayer {
    std::string name;
    int priority{0};
    float weight{1.0f};
    IKBlendMode blendMode{IKBlendMode::Override};
    bool enabled{true};
    
    // Which body parts this layer affects
    std::vector<BodyPart> affectedParts;
    
    // Layer-specific goals
    std::vector<FBIKGoal> goals;
};

/**
 * @brief IK layer manager for complex blending
 */
class IKLayerManager {
public:
    IKLayerManager(FullBodyIK& fbik);
    
    // Layer management
    int addLayer(const std::string& name, int priority = 0);
    void removeLayer(int layerId);
    void removeLayer(const std::string& name);
    IKLayer* getLayer(int layerId);
    IKLayer* getLayer(const std::string& name);
    
    // Layer configuration
    void setLayerWeight(int layerId, float weight);
    void setLayerEnabled(int layerId, bool enabled);
    void setLayerBlendMode(int layerId, IKBlendMode mode);
    void setLayerAffectedParts(int layerId, const std::vector<BodyPart>& parts);
    
    // Goals per layer
    void setLayerGoal(int layerId, const FBIKGoal& goal);
    void clearLayerGoal(int layerId, BodyPart part);
    void clearLayerGoals(int layerId);
    
    // Blending
    void setGlobalBlendSpeed(float speed) { m_blendSpeed = speed; }
    void blendToLayer(int layerId, float targetWeight, float duration);
    void crossFadeLayers(int fromLayerId, int toLayerId, float duration);
    
    // Solving
    void solve(float deltaTime);
    
private:
    FullBodyIK& m_fbik;
    std::vector<IKLayer> m_layers;
    float m_blendSpeed{5.0f};
    
    void sortLayers();
    void blendGoals();
};

// =============================================================================
// PROCEDURAL ANIMATION HELPERS
// =============================================================================

/**
 * @brief Procedural hand placement for holding objects
 */
class HandPlacementIK {
public:
    HandPlacementIK(FullBodyIK& fbik);
    
    // Object to hold
    void setHoldTarget(float x, float y, float z, float rotX, float rotY, float rotZ, float rotW);
    void setTwoHandedGrip(float spacing, float rotationOffset);
    
    // Grip types
    enum class GripType { Default, Pistol, Rifle, Tool, Custom };
    void setGripType(GripType type);
    void setGripOffset(bool isLeft, float x, float y, float z);
    
    // Update
    void update(float deltaTime);
    
    // State
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void setWeight(float weight) { m_weight = weight; }
    
private:
    FullBodyIK& m_fbik;
    bool m_enabled{true};
    float m_weight{1.0f};
    float m_holdPosition[3];
    float m_holdRotation[4];
    float m_leftGripOffset[3];
    float m_rightGripOffset[3];
    bool m_twoHanded{false};
    GripType m_gripType{GripType::Default};
};

/**
 * @brief Procedural foot placement for uneven terrain
 */
class TerrainFootIK {
public:
    TerrainFootIK(FullBodyIK& fbik);
    
    // Terrain interface
    using RaycastFunc = std::function<bool(float x, float y, float z, float dirX, float dirY, float dirZ, 
                                            float& hitX, float& hitY, float& hitZ, float& normalX, float& normalY, float& normalZ)>;
    void setRaycastFunction(RaycastFunc func);
    
    // Configuration
    void setMaxStepHeight(float height) { m_maxStepHeight = height; }
    void setFootOffset(float offset) { m_footOffset = offset; }
    void setBlendSpeed(float speed) { m_blendSpeed = speed; }
    void setRaycastHeight(float height) { m_raycastHeight = height; }
    
    // Body adjustment
    void setAdjustBody(bool adjust) { m_adjustBody = adjust; }
    void setMaxBodyAdjust(float amount) { m_maxBodyAdjust = amount; }
    
    // Update
    void update(float deltaTime);
    
    // Query
    float getLeftFootHeight() const { return m_leftFootHeight; }
    float getRightFootHeight() const { return m_rightFootHeight; }
    bool isLeftFootPlanted() const { return m_leftPlanted; }
    bool isRightFootPlanted() const { return m_rightPlanted; }
    
private:
    FullBodyIK& m_fbik;
    RaycastFunc m_raycast;
    
    float m_maxStepHeight{0.5f};
    float m_footOffset{0.1f};
    float m_blendSpeed{10.0f};
    float m_raycastHeight{1.0f};
    bool m_adjustBody{true};
    float m_maxBodyAdjust{0.3f};
    
    float m_leftFootHeight{0.0f};
    float m_rightFootHeight{0.0f};
    bool m_leftPlanted{false};
    bool m_rightPlanted{false};
};

/**
 * @brief Aim IK for weapons and pointing
 */
class AimIK {
public:
    AimIK(FullBodyIK& fbik);
    
    // Target
    void setAimTarget(float x, float y, float z);
    void clearTarget();
    
    // Configuration
    void setAimJoint(int jointId) { m_aimJointId = jointId; }
    void setAimAxis(float x, float y, float z);
    void setWeight(float weight) { m_weight = weight; }
    void setMaxAngle(float degrees) { m_maxAngle = degrees; }
    
    // Spine involvement
    void setSpineInvolvement(float weight) { m_spineWeight = weight; }
    
    // Update
    void update(float deltaTime);
    
    // Query
    bool canReachTarget() const { return m_canReach; }
    float getAngleToTarget() const { return m_angleToTarget; }
    
private:
    FullBodyIK& m_fbik;
    
    int m_aimJointId{-1};
    float m_aimAxis[3]{0.0f, 0.0f, 1.0f};
    float m_targetPosition[3];
    bool m_hasTarget{false};
    float m_weight{1.0f};
    float m_maxAngle{90.0f};
    float m_spineWeight{0.5f};
    bool m_canReach{false};
    float m_angleToTarget{0.0f};
};

} // namespace Engine
