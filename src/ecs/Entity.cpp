#include "ecs/Entity.h"
#include "ecs/EntityManager.h"
#include "ecs/Component.h"

namespace JJM {
namespace ECS {

Entity::Entity(EntityID id, EntityManager* manager) 
    : id(id), manager(manager), active(true) {}

Entity::~Entity() {
    for (auto& pair : components) {
        if (pair.second) {
            pair.second->destroy();
        }
    }
    components.clear();
}

void Entity::destroy() {
    active = false;
    if (manager) {
        manager->destroyEntity(this);
    }
}

template<typename T, typename... Args>
T* Entity::addComponent(Args&&... args) {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    
    std::type_index typeIndex(typeid(T));
    
    // Check if component already exists
    if (components.find(typeIndex) != components.end()) {
        return static_cast<T*>(components[typeIndex].get());
    }
    
    // Create new component
    auto component = std::make_shared<T>(std::forward<Args>(args)...);
    component->setOwner(this);
    components[typeIndex] = component;
    component->init();
    
    return component.get();
}

template<typename T>
T* Entity::getComponent() {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    
    std::type_index typeIndex(typeid(T));
    auto it = components.find(typeIndex);
    
    if (it != components.end()) {
        return static_cast<T*>(it->second.get());
    }
    
    return nullptr;
}

template<typename T>
const T* Entity::getComponent() const {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    
    std::type_index typeIndex(typeid(T));
    auto it = components.find(typeIndex);
    
    if (it != components.end()) {
        return static_cast<const T*>(it->second.get());
    }
    
    return nullptr;
}

template<typename T>
bool Entity::hasComponent() const {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    
    std::type_index typeIndex(typeid(T));
    return components.find(typeIndex) != components.end();
}

template<typename T>
void Entity::removeComponent() {
    static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
    
    std::type_index typeIndex(typeid(T));
    auto it = components.find(typeIndex);
    
    if (it != components.end()) {
        it->second->destroy();
        components.erase(it);
    }
}

// Explicit template instantiations will be in the implementation files that use them

} // namespace ECS
} // namespace JJM
