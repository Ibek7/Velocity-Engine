#ifndef COLLIDER_H
#define COLLIDER_H

#include "math/Vector2D.h"
#include "ecs/Component.h"
#include <vector>
#include <functional>

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

using CollisionCallback = std::function<void(const CollisionInfo&)>;
using TriggerCallback = std::function<void(ECS::Entity*)>;

class Collider : public ECS::Component {
public:
    ColliderType type;
    Math::Vector2D offset;
    bool isTrigger;
    int layer;
    int layerMask;
    
    Collider(ColliderType t = ColliderType::BOX);
    virtual ~Collider() = default;
    
    virtual bool checkCollision(const Collider* other, CollisionInfo& info) const = 0;
    virtual Math::Vector2D getPosition() const;
    
    bool canCollideWith(const Collider* other) const;
    void setLayer(int l);
    void setLayerMask(int mask);
    
    // Collision callbacks
    void setOnCollisionEnter(CollisionCallback callback) { onCollisionEnter = callback; }
    void setOnCollisionStay(CollisionCallback callback) { onCollisionStay = callback; }
    void setOnCollisionExit(CollisionCallback callback) { onCollisionExit = callback; }
    
    void setOnTriggerEnter(TriggerCallback callback) { onTriggerEnter = callback; }
    void setOnTriggerExit(TriggerCallback callback) { onTriggerExit = callback; }
    
    void invokeCollisionEnter(const CollisionInfo& info) { if (onCollisionEnter) onCollisionEnter(info); }
    void invokeCollisionStay(const CollisionInfo& info) { if (onCollisionStay) onCollisionStay(info); }
    void invokeCollisionExit(const CollisionInfo& info) { if (onCollisionExit) onCollisionExit(info); }
    
    void invokeTriggerEnter(ECS::Entity* other) { if (onTriggerEnter) onTriggerEnter(other); }
    void invokeTriggerExit(ECS::Entity* other) { if (onTriggerExit) onTriggerExit(other); }

protected:
    CollisionCallback onCollisionEnter;
    CollisionCallback onCollisionStay;
    CollisionCallback onCollisionExit;
    
    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
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
