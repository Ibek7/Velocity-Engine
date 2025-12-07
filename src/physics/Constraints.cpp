#include "physics/Constraints.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Physics {

// DistanceConstraint
DistanceConstraint::DistanceConstraint(PhysicsBody* bodyA, PhysicsBody* bodyB, float distance)
    : bodyA(bodyA), bodyB(bodyB), stiffness(1.0f) {
    if (distance < 0) {
        Math::Vector2D diff = bodyB->position - bodyA->position;
        targetDistance = diff.magnitude();
    } else {
        targetDistance = distance;
    }
}

void DistanceConstraint::solve(float deltaTime) {
    if (!enabled || !bodyA || !bodyB) return;
    
    Math::Vector2D diff = bodyB->position - bodyA->position;
    float currentDistance = diff.magnitude();
    
    if (currentDistance < 0.0001f) return;
    
    float error = currentDistance - targetDistance;
    Math::Vector2D correction = (diff / currentDistance) * (error * stiffness * deltaTime);
    
    if (bodyA->type != BodyType::STATIC) {
        bodyA->position = bodyA->position + correction * 0.5f;
    }
    if (bodyB->type != BodyType::STATIC) {
        bodyB->position = bodyB->position - correction * 0.5f;
    }
}

// SpringConstraint
SpringConstraint::SpringConstraint(PhysicsBody* bodyA, PhysicsBody* bodyB,
                                   float restLength, float stiffness, float damping)
    : bodyA(bodyA), bodyB(bodyB), restLength(restLength), stiffness(stiffness), damping(damping) {
}

void SpringConstraint::solve(float deltaTime) {
    if (!enabled || !bodyA || !bodyB) return;
    
    Math::Vector2D diff = bodyB->position - bodyA->position;
    float distance = diff.magnitude();
    
    if (distance < 0.0001f) return;
    
    Math::Vector2D direction = diff / distance;
    float extension = distance - restLength;
    
    // Spring force
    Math::Vector2D springForce = direction * (extension * stiffness);
    
    // Damping force
    Math::Vector2D relativeVelocity = bodyB->velocity - bodyA->velocity;
    Math::Vector2D dampingForce = relativeVelocity * damping;
    
    Math::Vector2D totalForce = springForce + dampingForce;
    
    if (bodyA->type != BodyType::STATIC) {
        bodyA->applyForce(totalForce);
    }
    if (bodyB->type != BodyType::STATIC) {
        bodyB->applyForce(totalForce * -1.0f);
    }
}

// HingeConstraint
HingeConstraint::HingeConstraint(PhysicsBody* bodyA, PhysicsBody* bodyB, const Math::Vector2D& anchor)
    : bodyA(bodyA), bodyB(bodyB), anchor(anchor), useLimits(false), minAngle(0), maxAngle(0) {
}

void HingeConstraint::solve(float deltaTime) {
    if (!enabled || !bodyA || !bodyB) return;
    
    // Keep bodies connected at anchor point
    Math::Vector2D diff = bodyB->position - bodyA->position;
    Math::Vector2D targetDiff = anchor;
    Math::Vector2D correction = (targetDiff - diff) * 0.5f;
    
    if (bodyA->type != BodyType::STATIC) {
        bodyA->position = bodyA->position - correction;
    }
    if (bodyB->type != BodyType::STATIC) {
        bodyB->position = bodyB->position + correction;
    }
    
    (void)deltaTime; // Unused in simplified implementation
}

void HingeConstraint::setLimits(float minAngle, float maxAngle) {
    this->minAngle = minAngle;
    this->maxAngle = maxAngle;
}

// PositionConstraint
PositionConstraint::PositionConstraint(PhysicsBody* body, const Math::Vector2D& targetPos)
    : body(body), targetPosition(targetPos), stiffness(1.0f) {
}

void PositionConstraint::solve(float deltaTime) {
    if (!enabled || !body || body->type == BodyType::STATIC) return;
    
    Math::Vector2D diff = targetPosition - body->position;
    body->position = body->position + diff * stiffness * deltaTime;
}

// MotorConstraint
MotorConstraint::MotorConstraint(PhysicsBody* body)
    : body(body), targetVelocity(0, 0), maxForce(100.0f) {
}

void MotorConstraint::solve(float deltaTime) {
    if (!enabled || !body || body->type == BodyType::STATIC) return;
    
    Math::Vector2D velocityDiff = targetVelocity - body->velocity;
    Math::Vector2D force = velocityDiff * body->mass / deltaTime;
    
    // Clamp force
    float forceMag = force.magnitude();
    if (forceMag > maxForce) {
        force = (force / forceMag) * maxForce;
    }
    
    body->applyForce(force);
}

// ConstraintSolver
ConstraintSolver::ConstraintSolver() {
}

void ConstraintSolver::addConstraint(std::shared_ptr<Constraint> constraint) {
    if (constraint) {
        constraints.push_back(constraint);
    }
}

void ConstraintSolver::removeConstraint(std::shared_ptr<Constraint> constraint) {
    constraints.erase(
        std::remove(constraints.begin(), constraints.end(), constraint),
        constraints.end()
    );
}

void ConstraintSolver::clear() {
    constraints.clear();
}

void ConstraintSolver::solve(float deltaTime, int iterations) {
    // Pre-step
    for (auto& constraint : constraints) {
        if (constraint && constraint->isEnabled()) {
            constraint->preStep(deltaTime);
        }
    }
    
    // Iterative solving
    for (int i = 0; i < iterations; ++i) {
        for (auto& constraint : constraints) {
            if (constraint && constraint->isEnabled()) {
                constraint->solve(deltaTime / iterations);
            }
        }
    }
}

} // namespace Physics
} // namespace JJM
