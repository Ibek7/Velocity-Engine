#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

#include "ecs/Component.h"
#include "ecs/Entity.h"

namespace JJM {
namespace ECS {

// Entity filter for queries
struct EntityFilter {
    std::vector<std::type_index> requiredComponents;
    std::vector<std::type_index> excludedComponents;
    std::function<bool(Entity*)> customFilter;
    std::string tag;
    bool activeOnly = true;

    bool operator==(const EntityFilter& other) const {
        return requiredComponents == other.requiredComponents &&
               excludedComponents == other.excludedComponents && tag == other.tag &&
               activeOnly == other.activeOnly;
        // Note: customFilter is not compared as std::function is not comparable
    }
};

// Entity group for cached queries
class EntityGroup {
   public:
    void addEntity(Entity* entity) { entities.insert(entity); }
    void removeEntity(Entity* entity) { entities.erase(entity); }
    bool contains(Entity* entity) const { return entities.count(entity) > 0; }
    const std::unordered_set<Entity*>& getEntities() const { return entities; }
    size_t size() const { return entities.size(); }
    void clear() { entities.clear(); }

   private:
    std::unordered_set<Entity*> entities;
};

// System base class for ECS systems
class System {
   public:
    virtual ~System() = default;
    virtual void update(float deltaTime) = 0;
    virtual void onEntityAdded(Entity* entity) {}
    virtual void onEntityRemoved(Entity* entity) {}

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    int getPriority() const { return m_priority; }
    void setPriority(int priority) { m_priority = priority; }

   protected:
    bool m_enabled = true;
    int m_priority = 0;
};

class EntityManager {
   private:
    std::vector<std::unique_ptr<Entity>> entities;
    EntityID nextID;

    // Entity groups for tag-based queries
    std::unordered_map<std::string, EntityGroup> tagGroups;

    // Query result caching
    struct QueryCache {
        EntityFilter filter;
        std::vector<Entity*> results;
        bool dirty;
        size_t lastEntityCount;

        QueryCache() : dirty(true), lastEntityCount(0) {}
    };
    std::vector<QueryCache> queryCaches;
    size_t cacheHits = 0;
    size_t cacheMisses = 0;

    // Systems
    std::vector<std::unique_ptr<System>> systems;
    bool systemsSorted = false;

    // Deferred operations
    std::vector<EntityID> entitiesToDestroy;
    bool processingUpdate = false;

    // Entity callbacks
    std::function<void(Entity*)> onEntityCreated;
    std::function<void(Entity*)> onEntityDestroyed;

   public:
    EntityManager();
    ~EntityManager();

    // Entity management
    Entity* createEntity();
    Entity* createEntity(const std::string& tag);
    void destroyEntity(EntityID id);
    void destroyEntity(Entity* entity);
    void destroyAllEntities();
    void destroyEntitiesWithTag(const std::string& tag);
    Entity* getEntity(EntityID id);

    // Deferred destruction
    void destroyEntityDeferred(EntityID id);
    void destroyEntityDeferred(Entity* entity);
    void processDeferred();

    // Entity tagging
    void setEntityTag(Entity* entity, const std::string& tag);
    void removeEntityTag(Entity* entity, const std::string& tag);
    bool entityHasTag(Entity* entity, const std::string& tag) const;

    // Query entities by component
    template <typename T>
    std::vector<Entity*> getEntitiesWithComponent();

    template <typename T, typename U, typename... Args>
    std::vector<Entity*> getEntitiesWithComponents();

    template <typename T>
    std::vector<Entity*> getEntitiesWithoutComponent();

    // Query entities by tag
    std::vector<Entity*> getEntitiesWithTag(const std::string& tag);
    Entity* getFirstEntityWithTag(const std::string& tag);

    // Advanced queries
    std::vector<Entity*> query(const EntityFilter& filter);
    Entity* queryFirst(const EntityFilter& filter);
    void forEach(const EntityFilter& filter, std::function<void(Entity*)> callback);

    template <typename T>
    void forEachWith(std::function<void(Entity*, T*)> callback);

    // Query caching
    std::vector<Entity*> queryCached(const EntityFilter& filter);
    void invalidateQueryCaches();
    void clearQueryCaches();
    void getCacheStatistics(size_t& hits, size_t& misses) const;

    std::vector<Entity*> getAllEntities();

    // System management
    template <typename T, typename... Args>
    T* addSystem(Args&&... args);

    template <typename T>
    T* getSystem();

    template <typename T>
    void removeSystem();

    void updateSystems(float deltaTime);
    void sortSystems();

    // Update all entities
    void update(float deltaTime);

    // Clear all entities
    void clear();

    // Entity callbacks
    void setOnEntityCreated(std::function<void(Entity*)> callback) { onEntityCreated = callback; }
    void setOnEntityDestroyed(std::function<void(Entity*)> callback) {
        onEntityDestroyed = callback;
    }

    // Statistics
    size_t getEntityCount() const { return entities.size(); }
    size_t getActiveEntityCount() const;
    size_t getSystemCount() const { return systems.size(); }
};

template <typename T>
std::vector<Entity*> EntityManager::getEntitiesWithComponent() {
    std::vector<Entity*> result;
    for (auto& entity : entities) {
        if (entity->isActive() && entity->hasComponent<T>()) {
            result.push_back(entity.get());
        }
    }
    return result;
}

template <typename T, typename U, typename... Args>
std::vector<Entity*> EntityManager::getEntitiesWithComponents() {
    std::vector<Entity*> result;
    for (auto& entity : entities) {
        if (entity->isActive() && entity->hasComponent<T>() && entity->hasComponent<U>()) {
            bool hasAll = true;
            if constexpr (sizeof...(Args) > 0) {
                hasAll = (entity->hasComponent<Args>() && ...);
            }
            if (hasAll) {
                result.push_back(entity.get());
            }
        }
    }
    return result;
}

template <typename T>
std::vector<Entity*> EntityManager::getEntitiesWithoutComponent() {
    std::vector<Entity*> result;
    for (auto& entity : entities) {
        if (entity->isActive() && !entity->hasComponent<T>()) {
            result.push_back(entity.get());
        }
    }
    return result;
}

template <typename T>
void EntityManager::forEachWith(std::function<void(Entity*, T*)> callback) {
    for (auto& entity : entities) {
        if (entity->isActive()) {
            T* component = entity->getComponent<T>();
            if (component) {
                callback(entity.get(), component);
            }
        }
    }
}

template <typename T, typename... Args>
T* EntityManager::addSystem(Args&&... args) {
    auto system = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = system.get();
    systems.push_back(std::move(system));
    systemsSorted = false;
    return ptr;
}

template <typename T>
T* EntityManager::getSystem() {
    for (auto& system : systems) {
        T* typed = dynamic_cast<T*>(system.get());
        if (typed) {
            return typed;
        }
    }
    return nullptr;
}

template <typename T>
void EntityManager::removeSystem() {
    systems.erase(std::remove_if(systems.begin(), systems.end(),
                                 [](const std::unique_ptr<System>& s) {
                                     return dynamic_cast<T*>(s.get()) != nullptr;
                                 }),
                  systems.end());
}

}  // namespace ECS
}  // namespace JJM

#endif  // ENTITY_MANAGER_H
