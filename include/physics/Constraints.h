#ifndef PHYSICS_CONSTRAINTS_H
#define PHYSICS_CONSTRAINTS_H

#include "math/Vector2D.h"
#include "physics/PhysicsBody.h"
#include <memory>

namespace JJM {
namespace Physics {

/**
 * @brief Base class for physics constraints
 */
class Constraint {
public:
    virtual ~Constraint() = default;
    
    virtual void solve(float deltaTime) = 0;
    virtual void preStep(float deltaTime) { (void)deltaTime; }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
protected:
    bool enabled = true;
};

/**
 * @brief Distance constraint keeps two bodies at fixed distance
 */
class DistanceConstraint : public Constraint {
public:
    DistanceConstraint(PhysicsBody* bodyA, PhysicsBody* bodyB, float distance = -1.0f);
    
    void solve(float deltaTime) override;
    
    void setDistance(float dist) { targetDistance = dist; }
    float getDistance() const { return targetDistance; }
    void setStiffness(float s) { stiffness = s; }
    
private:
    PhysicsBody* bodyA;
    PhysicsBody* bodyB;
    float targetDistance;
    float stiffness;
};

/**
 * @brief Spring constraint applies spring force between bodies
 */
class SpringConstraint : public Constraint {
public:
    SpringConstraint(PhysicsBody* bodyA, PhysicsBody* bodyB, 
                     float restLength = 1.0f, float stiffness = 100.0f, float damping = 10.0f);
    
    void solve(float deltaTime) override;
    
    void setRestLength(float length) { restLength = length; }
    void setStiffness(float k) { stiffness = k; }
    void setDamping(float d) { damping = d; }
    
private:
    PhysicsBody* bodyA;
    PhysicsBody* bodyB;
    float restLength;
    float stiffness;
    float damping;
};

/**
 * @brief Hinge constraint creates rotational joint
 */
class HingeConstraint : public Constraint {
public:
    HingeConstraint(PhysicsBody* bodyA, PhysicsBody* bodyB, const Math::Vector2D& anchor);
    
    void solve(float deltaTime) override;
    
    void setAnchor(const Math::Vector2D& pos) { anchor = pos; }
    void setEnableLimits(bool enable) { useLimits = enable; }
    void setLimits(float minAngle, float maxAngle);
    
private:
    PhysicsBody* bodyA;
    PhysicsBody* bodyB;
    Math::Vector2D anchor;
    bool useLimits;
    float minAngle;
    float maxAngle;
};

/**
 * @brief Position constraint locks body to specific position
 */
class PositionConstraint : public Constraint {
public:
    PositionConstraint(PhysicsBody* body, const Math::Vector2D& targetPos);
    
    void solve(float deltaTime) override;
    
    void setTargetPosition(const Math::Vector2D& pos) { targetPosition = pos; }
    void setStiffness(float s) { stiffness = s; }
    
private:
    PhysicsBody* body;
    Math::Vector2D targetPosition;
    float stiffness;
};

/**
 * @brief Motor constraint applies continuous force/torque
 */
class MotorConstraint : public Constraint {
public:
    MotorConstraint(PhysicsBody* body);
    
    void solve(float deltaTime) override;
    
    void setTargetVelocity(const Math::Vector2D& vel) { targetVelocity = vel; }
    void setMaxForce(float force) { maxForce = force; }
    
private:
    PhysicsBody* body;
    Math::Vector2D targetVelocity;
    float maxForce;
};

/**
 * @brief Manages all physics constraints
 */
class ConstraintSolver {
public:
    ConstraintSolver();
    
    void addConstraint(std::shared_ptr<Constraint> constraint);
    void removeConstraint(std::shared_ptr<Constraint> constraint);
    void clear();
    
    void solve(float deltaTime, int iterations = 10);
    
    int getConstraintCount() const { return static_cast<int>(constraints.size()); }
    
private:
    std::vector<std::shared_ptr<Constraint>> constraints;
};

} // namespace Physics
} // namespace JJM

#endif // PHYSICS_CONSTRAINTS_H
