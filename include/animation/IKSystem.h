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

} // namespace Engine
