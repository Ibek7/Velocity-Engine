#ifndef COMPONENT_H
#define COMPONENT_H

namespace JJM {
namespace ECS {

// Forward declaration
class Entity;

class Component {
protected:
    Entity* owner;
    bool enabled;

public:
    Component() : owner(nullptr), enabled(true) {}
    virtual ~Component() = default;

    virtual void init() {}
    virtual void update(float deltaTime) {}
    virtual void destroy() {}

    void setOwner(Entity* entity) { owner = entity; }
    Entity* getOwner() const { return owner; }

    bool isEnabled() const { return enabled; }
    void setEnabled(bool value) { enabled = value; }
};

} // namespace ECS
} // namespace JJM

#endif // COMPONENT_H
