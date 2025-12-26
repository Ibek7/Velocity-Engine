#pragma once

#include <string>
#include <vector>
#include <map>

// Ragdoll physics for character animation
namespace Engine {

struct RagdollBone {
    std::string name;
    int boneId;
    int parentId;
    float mass;
    float length;
    float minAngle;
    float maxAngle;
    bool isActive;
};

struct RagdollJoint {
    int boneA;
    int boneB;
    float stiffness;
    float damping;
    bool isBreakable;
    float breakForce;
};

class Ragdoll {
public:
    Ragdoll(int entityId);
    ~Ragdoll();

    // Setup
    void addBone(const RagdollBone& bone);
    void addJoint(const RagdollJoint& joint);
    void build();
    
    // Activation
    void activate();
    void deactivate();
    bool isActive() const { return m_isActive; }
    
    // Physics
    void applyForce(int boneId, float forceX, float forceY, float forceZ);
    void applyImpulse(int boneId, float impX, float impY, float impZ);
    void setGravity(float x, float y, float z);
    
    // Bone manipulation
    void setBonePosition(int boneId, float x, float y, float z);
    void getBonePosition(int boneId, float& x, float& y, float& z) const;
    void setBoneRotation(int boneId, float x, float y, float z, float w);
    
    // Query
    const RagdollBone* getBone(int boneId) const;
    const std::vector<RagdollBone>& getBones() const { return m_bones; }
    const std::vector<RagdollJoint>& getJoints() const { return m_joints; }
    
    // Blend with animation
    void setBlendWeight(float weight);
    float getBlendWeight() const { return m_blendWeight; }
    void enableBlending(bool enable) { m_blendingEnabled = enable; }

private:
    void createPhysicsBodies();
    void createPhysicsConstraints();
    void syncWithPhysics();

    int m_entityId;
    std::vector<RagdollBone> m_bones;
    std::vector<RagdollJoint> m_joints;
    bool m_isActive;
    bool m_blendingEnabled;
    float m_blendWeight;
    float m_gravityX;
    float m_gravityY;
    float m_gravityZ;
};

class RagdollSystem {
public:
    static RagdollSystem& getInstance();

    // Ragdoll management
    Ragdoll* createRagdoll(int entityId);
    Ragdoll* getRagdoll(int entityId);
    void removeRagdoll(int entityId);
    
    // Update
    void update(float deltaTime);
    
    // Global settings
    void setGlobalDamping(float damping) { m_globalDamping = damping; }
    void setGlobalStiffness(float stiffness) { m_globalStiffness = stiffness; }
    void enableSelfCollision(bool enable) { m_selfCollisionEnabled = enable; }

private:
    RagdollSystem();
    RagdollSystem(const RagdollSystem&) = delete;
    RagdollSystem& operator=(const RagdollSystem&) = delete;

    std::map<int, Ragdoll*> m_ragdolls;
    float m_globalDamping;
    float m_globalStiffness;
    bool m_selfCollisionEnabled;
};

} // namespace Engine
