#include "animation/IKSystem.h"
#include <cmath>
#include <algorithm>

namespace Engine {

// IKChain implementation
IKChain::IKChain()
    : m_rootJointId(0)
    , m_endEffectorId(-1)
    , m_solverType(IKSolverType::FABRIK)
    , m_maxIterations(10)
    , m_tolerance(0.01f)
    , m_weight(1.0f)
    , m_usePoleTarget(false)
    , m_poleTargetX(0.0f)
    , m_poleTargetY(1.0f)
    , m_poleTargetZ(0.0f)
{
}

IKChain::~IKChain() {
}

void IKChain::addJoint(const IKJoint& joint) {
    m_joints.push_back(joint);
}

void IKChain::setEndEffector(int jointId) {
    m_endEffectorId = jointId;
    if (jointId >= 0 && jointId < static_cast<int>(m_joints.size())) {
        m_joints[jointId].isEndEffector = true;
    }
}

void IKChain::setRootJoint(int jointId) {
    m_rootJointId = jointId;
}

void IKChain::setJointLimits(int jointId, float minAngle, float maxAngle) {
    if (jointId >= 0 && jointId < static_cast<int>(m_joints.size())) {
        m_joints[jointId].minAngle = minAngle;
        m_joints[jointId].maxAngle = maxAngle;
    }
}

void IKChain::setPoleTarget(float x, float y, float z) {
    m_poleTargetX = x;
    m_poleTargetY = y;
    m_poleTargetZ = z;
}

bool IKChain::solve(float targetX, float targetY, float targetZ) {
    switch (m_solverType) {
        case IKSolverType::CCD:
            return solveCCD(targetX, targetY, targetZ);
        case IKSolverType::FABRIK:
            return solveFABRIK(targetX, targetY, targetZ);
        case IKSolverType::TwoJoint:
            return solveTwoJoint(targetX, targetY, targetZ);
        case IKSolverType::Jacobian:
            return solveJacobian(targetX, targetY, targetZ);
    }
    return false;
}

bool IKChain::solveWithHint(float targetX, float targetY, float targetZ,
                             float hintX, float hintY, float hintZ) {
    setPoleTarget(hintX, hintY, hintZ);
    enablePoleTarget(true);
    return solve(targetX, targetY, targetZ);
}

void IKChain::getJointPosition(int jointId, float& x, float& y, float& z) const {
    if (jointId >= 0 && jointId < static_cast<int>(m_joints.size())) {
        x = m_joints[jointId].x;
        y = m_joints[jointId].y;
        z = m_joints[jointId].z;
    }
}

void IKChain::getJointRotation(int jointId, float& angle) const {
    if (jointId >= 0 && jointId < static_cast<int>(m_joints.size())) {
        // Calculate angle based on joint position relative to parent
        angle = 0.0f; // Simplified
    }
}

bool IKChain::solveCCD(float targetX, float targetY, float targetZ) {
    if (m_endEffectorId < 0) return false;
    
    for (int iteration = 0; iteration < m_maxIterations; ++iteration) {
        // Get end effector position
        float endX = m_joints[m_endEffectorId].x;
        float endY = m_joints[m_endEffectorId].y;
        float endZ = m_joints[m_endEffectorId].z;
        
        // Check if close enough
        float dist = calculateDistance(endX, endY, endZ, targetX, targetY, targetZ);
        if (dist < m_tolerance) {
            return true;
        }
        
        // Iterate through joints backwards (from end to root)
        for (int i = m_endEffectorId; i >= m_rootJointId; --i) {
            IKJoint& joint = m_joints[i];
            
            // Vector from joint to end effector
            float toEndX = endX - joint.x;
            float toEndY = endY - joint.y;
            float toEndZ = endZ - joint.z;
            
            // Vector from joint to target
            float toTargetX = targetX - joint.x;
            float toTargetY = targetY - joint.y;
            float toTargetZ = targetZ - joint.z;
            
            // Normalize vectors
            float toEndLen = std::sqrt(toEndX * toEndX + toEndY * toEndY + toEndZ * toEndZ);
            float toTargetLen = std::sqrt(toTargetX * toTargetX + toTargetY * toTargetY + toTargetZ * toTargetZ);
            
            if (toEndLen > 0.0f && toTargetLen > 0.0f) {
                toEndX /= toEndLen;
                toEndY /= toEndLen;
                toEndZ /= toEndLen;
                toTargetX /= toTargetLen;
                toTargetY /= toTargetLen;
                toTargetZ /= toTargetLen;
                
                // Calculate rotation angle
                float dot = toEndX * toTargetX + toEndY * toTargetY + toEndZ * toTargetZ;
                float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));
                
                // Apply rotation (simplified - would need full rotation matrix)
                if (angle > 0.001f) {
                    applyJointLimits(i);
                }
            }
        }
    }
    
    return false;
}

bool IKChain::solveFABRIK(float targetX, float targetY, float targetZ) {
    if (m_endEffectorId < 0) return false;
    
    // Store original root position
    float rootX = m_joints[m_rootJointId].x;
    float rootY = m_joints[m_rootJointId].y;
    float rootZ = m_joints[m_rootJointId].z;
    
    for (int iteration = 0; iteration < m_maxIterations; ++iteration) {
        // Forward reach
        forwardReach(targetX, targetY, targetZ);
        
        // Backward reach
        m_joints[m_rootJointId].x = rootX;
        m_joints[m_rootJointId].y = rootY;
        m_joints[m_rootJointId].z = rootZ;
        backwardReach();
        
        // Check convergence
        float endX = m_joints[m_endEffectorId].x;
        float endY = m_joints[m_endEffectorId].y;
        float endZ = m_joints[m_endEffectorId].z;
        float dist = calculateDistance(endX, endY, endZ, targetX, targetY, targetZ);
        
        if (dist < m_tolerance) {
            return true;
        }
    }
    
    return false;
}

void IKChain::forwardReach(float targetX, float targetY, float targetZ) {
    // Start from end effector
    m_joints[m_endEffectorId].x = targetX;
    m_joints[m_endEffectorId].y = targetY;
    m_joints[m_endEffectorId].z = targetZ;
    
    // Move towards root
    for (int i = m_endEffectorId - 1; i >= m_rootJointId; --i) {
        IKJoint& current = m_joints[i];
        IKJoint& next = m_joints[i + 1];
        
        // Direction from next to current
        float dx = current.x - next.x;
        float dy = current.y - next.y;
        float dz = current.z - next.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        if (dist > 0.0f) {
            float lambda = next.length / dist;
            current.x = next.x + dx * lambda;
            current.y = next.y + dy * lambda;
            current.z = next.z + dz * lambda;
        }
    }
}

void IKChain::backwardReach() {
    // Move from root to end effector
    for (int i = m_rootJointId; i < m_endEffectorId; ++i) {
        IKJoint& current = m_joints[i];
        IKJoint& next = m_joints[i + 1];
        
        // Direction from current to next
        float dx = next.x - current.x;
        float dy = next.y - current.y;
        float dz = next.z - current.z;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        if (dist > 0.0f) {
            float lambda = next.length / dist;
            next.x = current.x + dx * lambda;
            next.y = current.y + dy * lambda;
            next.z = current.z + dz * lambda;
        }
        
        applyJointLimits(i);
    }
}

bool IKChain::solveTwoJoint(float targetX, float targetY, float targetZ) {
    if (m_joints.size() < 3) return false;
    
    // Use analytic two-bone solver
    float outMidX, outMidY, outMidZ;
    float outEndX, outEndY, outEndZ;
    
    return TwoBoneIK::solve(
        m_joints[0].x, m_joints[0].y, m_joints[0].z,
        m_joints[1].x, m_joints[1].y, m_joints[1].z,
        m_joints[2].x, m_joints[2].y, m_joints[2].z,
        targetX, targetY, targetZ,
        m_poleTargetX, m_poleTargetY, m_poleTargetZ,
        outMidX, outMidY, outMidZ,
        outEndX, outEndY, outEndZ
    );
}

bool IKChain::solveJacobian(float targetX, float targetY, float targetZ) {
    // Jacobian-based solver (more complex, simplified here)
    return solveCCD(targetX, targetY, targetZ);
}

void IKChain::applyJointLimits(int jointId) {
    if (jointId < 0 || jointId >= static_cast<int>(m_joints.size())) return;
    
    IKJoint& joint = m_joints[jointId];
    // Apply angle constraints (simplified)
}

float IKChain::calculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// IKSystem implementation
IKSystem::IKSystem()
    : m_nextChainId(1)
    , m_enabled(true)
{
}

IKSystem::~IKSystem() {
    for (auto& pair : m_chains) {
        delete pair.second;
    }
}

int IKSystem::createChain(const std::string& name) {
    int chainId = m_nextChainId++;
    m_chains[chainId] = new IKChain();
    m_chainNames[name] = chainId;
    return chainId;
}

void IKSystem::destroyChain(int chainId) {
    auto it = m_chains.find(chainId);
    if (it != m_chains.end()) {
        delete it->second;
        m_chains.erase(it);
    }
}

IKChain* IKSystem::getChain(int chainId) {
    auto it = m_chains.find(chainId);
    return it != m_chains.end() ? it->second : nullptr;
}

IKChain* IKSystem::getChainByName(const std::string& name) {
    auto it = m_chainNames.find(name);
    if (it != m_chainNames.end()) {
        return getChain(it->second);
    }
    return nullptr;
}

int IKSystem::createLegChain(const std::string& name, float thighLength, float shinLength) {
    int chainId = createChain(name);
    IKChain* chain = getChain(chainId);
    
    if (chain) {
        // Hip joint
        IKJoint hip = {"hip", 0, -1, 0.0f, 0.0f, 0.0f, thighLength, -90.0f, 90.0f, false};
        chain->addJoint(hip);
        
        // Knee joint
        IKJoint knee = {"knee", 1, 0, 0.0f, -thighLength, 0.0f, shinLength, 0.0f, 160.0f, false};
        chain->addJoint(knee);
        
        // Ankle joint (end effector)
        IKJoint ankle = {"ankle", 2, 1, 0.0f, -thighLength - shinLength, 0.0f, 0.0f, -45.0f, 45.0f, true};
        chain->addJoint(ankle);
        
        chain->setRootJoint(0);
        chain->setEndEffector(2);
    }
    
    return chainId;
}

int IKSystem::createArmChain(const std::string& name, float upperArmLength, float forearmLength) {
    int chainId = createChain(name);
    IKChain* chain = getChain(chainId);
    
    if (chain) {
        // Shoulder joint
        IKJoint shoulder = {"shoulder", 0, -1, 0.0f, 0.0f, 0.0f, upperArmLength, -180.0f, 180.0f, false};
        chain->addJoint(shoulder);
        
        // Elbow joint
        IKJoint elbow = {"elbow", 1, 0, upperArmLength, 0.0f, 0.0f, forearmLength, 0.0f, 150.0f, false};
        chain->addJoint(elbow);
        
        // Wrist joint (end effector)
        IKJoint wrist = {"wrist", 2, 1, upperArmLength + forearmLength, 0.0f, 0.0f, 0.0f, -90.0f, 90.0f, true};
        chain->addJoint(wrist);
        
        chain->setRootJoint(0);
        chain->setEndEffector(2);
    }
    
    return chainId;
}

int IKSystem::createSpineChain(const std::string& name, int vertebraeCount, float segmentLength) {
    int chainId = createChain(name);
    IKChain* chain = getChain(chainId);
    
    if (chain) {
        for (int i = 0; i < vertebraeCount; ++i) {
            IKJoint vertebra;
            vertebra.name = "vertebra_" + std::to_string(i);
            vertebra.jointId = i;
            vertebra.parentId = i - 1;
            vertebra.x = 0.0f;
            vertebra.y = i * segmentLength;
            vertebra.z = 0.0f;
            vertebra.length = segmentLength;
            vertebra.minAngle = -30.0f;
            vertebra.maxAngle = 30.0f;
            vertebra.isEndEffector = (i == vertebraeCount - 1);
            
            chain->addJoint(vertebra);
        }
        
        chain->setRootJoint(0);
        chain->setEndEffector(vertebraeCount - 1);
    }
    
    return chainId;
}

void IKSystem::update(float deltaTime) {
    if (!m_enabled) return;
    
    // Update all chains (if needed)
}

// FootIK implementation
FootIK::FootIK()
    : m_legChain(nullptr)
    , m_footHeight(0.1f)
    , m_maxReach(2.0f)
    , m_groundHeight(0.0f)
    , m_useRaycast(false)
    , m_isPlanted(false)
    , m_footX(0.0f), m_footY(0.0f), m_footZ(0.0f)
    , m_targetX(0.0f), m_targetY(0.0f), m_targetZ(0.0f)
{
}

void FootIK::update(float hipX, float hipY, float hipZ, float deltaTime) {
    if (!m_legChain) return;
    
    // Calculate foot target position
    m_targetX = hipX;
    m_targetY = m_groundHeight + m_footHeight;
    m_targetZ = hipZ;
    
    // Check if within reach
    float dx = m_targetX - hipX;
    float dy = m_targetY - hipY;
    float dz = m_targetZ - hipZ;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    
    if (dist <= m_maxReach) {
        m_legChain->solve(m_targetX, m_targetY, m_targetZ);
        m_isPlanted = true;
    } else {
        m_isPlanted = false;
    }
}

void FootIK::getFootPosition(float& x, float& y, float& z) const {
    x = m_footX;
    y = m_footY;
    z = m_footZ;
}

// LookAtIK implementation
LookAtIK::LookAtIK()
    : m_headJointId(-1)
    , m_neckJointId(-1)
    , m_leftEyeId(-1)
    , m_rightEyeId(-1)
    , m_maxAngle(90.0f)
    , m_weight(1.0f)
    , m_smoothTime(0.1f)
    , m_hasTarget(false)
    , m_targetX(0.0f), m_targetY(0.0f), m_targetZ(0.0f)
    , m_currentX(0.0f), m_currentY(0.0f), m_currentZ(0.0f)
{
}

void LookAtIK::setEyeJoints(int leftEyeId, int rightEyeId) {
    m_leftEyeId = leftEyeId;
    m_rightEyeId = rightEyeId;
}

void LookAtIK::setTarget(float x, float y, float z) {
    m_targetX = x;
    m_targetY = y;
    m_targetZ = z;
    m_hasTarget = true;
}

void LookAtIK::clearTarget() {
    m_hasTarget = false;
}

void LookAtIK::update(float deltaTime) {
    if (!m_hasTarget) return;
    
    // Smooth interpolation to target
    float t = std::min(deltaTime / m_smoothTime, 1.0f);
    m_currentX += (m_targetX - m_currentX) * t;
    m_currentY += (m_targetY - m_currentY) * t;
    m_currentZ += (m_targetZ - m_currentZ) * t;
}

void LookAtIK::getTargetDirection(float& x, float& y, float& z) const {
    x = m_currentX;
    y = m_currentY;
    z = m_currentZ;
}

// TwoBoneIK implementation
bool TwoBoneIK::solve(
    float rootX, float rootY, float rootZ,
    float midX, float midY, float midZ,
    float endX, float endY, float endZ,
    float targetX, float targetY, float targetZ,
    float poleX, float poleY, float poleZ,
    float& outMidX, float& outMidY, float& outMidZ,
    float& outEndX, float& outEndY, float& outEndZ
) {
    // Calculate bone lengths
    float upperLength = std::sqrt((midX - rootX) * (midX - rootX) +
                                  (midY - rootY) * (midY - rootY) +
                                  (midZ - rootZ) * (midZ - rootZ));
    
    float lowerLength = std::sqrt((endX - midX) * (endX - midX) +
                                  (endY - midY) * (endY - midY) +
                                  (endZ - midZ) * (endZ - midZ));
    
    // Distance to target
    float dx = targetX - rootX;
    float dy = targetY - rootY;
    float dz = targetZ - rootZ;
    float targetDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
    
    // Solve angles
    float upperAngle, lowerAngle;
    if (solveAnalytic(upperLength, lowerLength, targetDistance, upperAngle, lowerAngle)) {
        // Apply angles to calculate joint positions (simplified)
        outEndX = targetX;
        outEndY = targetY;
        outEndZ = targetZ;
        
        // Calculate mid joint position
        outMidX = (rootX + targetX) * 0.5f;
        outMidY = (rootY + targetY) * 0.5f;
        outMidZ = (rootZ + targetZ) * 0.5f;
        
        return true;
    }
    
    return false;
}

bool TwoBoneIK::solveAnalytic(
    float upperLength,
    float lowerLength,
    float targetDistance,
    float& upperAngle,
    float& lowerAngle
) {
    float totalLength = upperLength + lowerLength;
    
    // Check if target is reachable
    if (targetDistance > totalLength) {
        // Fully extended
        upperAngle = 0.0f;
        lowerAngle = 0.0f;
        return true;
    }
    
    if (targetDistance < std::abs(upperLength - lowerLength)) {
        // Too close
        return false;
    }
    
    // Law of cosines
    float cosLower = (upperLength * upperLength + lowerLength * lowerLength - targetDistance * targetDistance) /
                     (2.0f * upperLength * lowerLength);
    lowerAngle = std::acos(std::clamp(cosLower, -1.0f, 1.0f));
    
    float cosUpper = (upperLength * upperLength + targetDistance * targetDistance - lowerLength * lowerLength) /
                     (2.0f * upperLength * targetDistance);
    upperAngle = std::acos(std::clamp(cosUpper, -1.0f, 1.0f));
    
    return true;
}

} // namespace Engine
