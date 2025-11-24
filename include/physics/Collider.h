#ifndef COLLIDER_H
#define COLLIDER_H

#include "math/Vector2D.h"
#include "ecs/Component.h"
#include <vector>

namespace JJM {
namespace Physics {

enum class ColliderType {
    CIRCLE,
    BOX,
    POLYGON
};

struct CollisionInfo {
    bool colliding;
    Math::Vector2D normal;
    float penetration;
    ECS::Entity* other;
    
    CollisionInfo() : colliding(false), normal(0, 0), penetration(0), other(nullptr) {}
};

class Collider : public ECS::Component {
public:
    ColliderType type;
    Math::Vector2D offset;
    bool isTrigger;
    
    Collider(ColliderType t = ColliderType::BOX);
    virtual ~Collider() = default;
    
    virtual bool checkCollision(const Collider* other, CollisionInfo& info) const = 0;
    virtual Math::Vector2D getPosition() const;
};

class CircleCollider : public Collider {
public:
    float radius;
    
    CircleCollider(float r = 1.0f);
    
    bool checkCollision(const Collider* other, CollisionInfo& info) const override;
    
private:
    bool checkCircleCircle(const CircleCollider* other, CollisionInfo& info) const;
    bool checkCircleBox(const class BoxCollider* other, CollisionInfo& info) const;
};

class BoxCollider : public Collider {
public:
    Math::Vector2D size;
    
    BoxCollider(const Math::Vector2D& s = Math::Vector2D(1, 1));
    BoxCollider(float width, float height);
    
    bool checkCollision(const Collider* other, CollisionInfo& info) const override;
    
    Math::Vector2D getMin() const;
    Math::Vector2D getMax() const;
    
private:
    bool checkBoxBox(const BoxCollider* other, CollisionInfo& info) const;
    bool checkBoxCircle(const CircleCollider* other, CollisionInfo& info) const;
};

} // namespace Physics
} // namespace JJM

#endif // COLLIDER_H
