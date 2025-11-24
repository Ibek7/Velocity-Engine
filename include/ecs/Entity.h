#ifndef ENTITY_H
#define ENTITY_H

#include <cstdint>
#include <memory>
#include <vector>
#include <typeindex>
#include <unordered_map>

namespace JJM {
namespace ECS {

// Forward declarations
class Component;
class EntityManager;

using EntityID = uint64_t;

class Entity {
private:
    EntityID id;
    EntityManager* manager;
    bool active;
    std::unordered_map<std::type_index, std::shared_ptr<Component>> components;

public:
    Entity(EntityID id, EntityManager* manager);
    ~Entity();

    // Component management
    template<typename T, typename... Args>
    T* addComponent(Args&&... args);
    
    template<typename T>
    T* getComponent();
    
    template<typename T>
    const T* getComponent() const;
    
    template<typename T>
    bool hasComponent() const;
    
    template<typename T>
    void removeComponent();

    // Entity management
    EntityID getID() const { return id; }
    bool isActive() const { return active; }
    void setActive(bool isActive) { active = isActive; }
    void destroy();

    // Get all components
    const std::unordered_map<std::type_index, std::shared_ptr<Component>>& getComponents() const {
        return components;
    }
};

} // namespace ECS
} // namespace JJM

#endif // ENTITY_H
