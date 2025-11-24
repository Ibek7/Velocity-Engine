#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include "ecs/Entity.h"
#include "ecs/Component.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace JJM {
namespace ECS {

class EntityManager {
private:
    std::vector<std::unique_ptr<Entity>> entities;
    EntityID nextID;

public:
    EntityManager();
    ~EntityManager();

    // Entity management
    Entity* createEntity();
    void destroyEntity(EntityID id);
    void destroyEntity(Entity* entity);
    Entity* getEntity(EntityID id);
    
    // Query entities
    template<typename T>
    std::vector<Entity*> getEntitiesWithComponent();
    
    std::vector<Entity*> getAllEntities();
    
    // Update all entities
    void update(float deltaTime);
    
    // Clear all entities
    void clear();
    
    size_t getEntityCount() const { return entities.size(); }
};

template<typename T>
std::vector<Entity*> EntityManager::getEntitiesWithComponent() {
    std::vector<Entity*> result;
    for (auto& entity : entities) {
        if (entity->isActive() && entity->hasComponent<T>()) {
            result.push_back(entity.get());
        }
    }
    return result;
}

} // namespace ECS
} // namespace JJM

#endif // ENTITY_MANAGER_H
