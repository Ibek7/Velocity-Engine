#include "ecs/EntityManager.h"

#include <algorithm>

namespace JJM {
namespace ECS {

EntityManager::EntityManager() : nextID(1) {}

EntityManager::~EntityManager() { clear(); }

Entity* EntityManager::createEntity() {
    auto entity = std::make_unique<Entity>(nextID++, this);
    Entity* ptr = entity.get();
    entities.push_back(std::move(entity));
    invalidateQueryCaches();
    return ptr;
}

void EntityManager::destroyEntity(EntityID id) {
    auto it = std::find_if(entities.begin(), entities.end(),
                           [id](const std::unique_ptr<Entity>& e) { return e->getID() == id; });

    if (it != entities.end()) {
        entities.erase(it);
        invalidateQueryCaches();
    }
}

void EntityManager::destroyEntity(Entity* entity) {
    if (entity) {
        destroyEntity(entity->getID());
    }
}

Entity* EntityManager::getEntity(EntityID id) {
    auto it = std::find_if(entities.begin(), entities.end(),
                           [id](const std::unique_ptr<Entity>& e) { return e->getID() == id; });

    if (it != entities.end()) {
        return it->get();
    }

    return nullptr;
}

std::vector<Entity*> EntityManager::getAllEntities() {
    std::vector<Entity*> result;
    for (auto& entity : entities) {
        if (entity->isActive()) {
            result.push_back(entity.get());
        }
    }
    return result;
}

void EntityManager::update(float deltaTime) {
    // Remove inactive entities
    entities.erase(std::remove_if(entities.begin(), entities.end(),
                                  [](const std::unique_ptr<Entity>& e) { return !e->isActive(); }),
                   entities.end());

    // Update all active entities' components
    for (auto& entity : entities) {
        if (entity->isActive()) {
            for (auto& pair : entity->getComponents()) {
                if (pair.second && pair.second->isEnabled()) {
                    pair.second->update(deltaTime);
                }
            }
        }
    }
}

void EntityManager::clear() {
    entities.clear();
    nextID = 1;
    clearQueryCaches();
}

std::vector<Entity*> EntityManager::queryCached(const EntityFilter& filter) {
    // Check if we have a cached result for this filter
    for (auto& cache : queryCaches) {
        if (cache.filter == filter && !cache.dirty && cache.lastEntityCount == entities.size()) {
            cacheHits++;
            return cache.results;
        }
    }

    // Cache miss - perform the query
    cacheMisses++;
    std::vector<Entity*> results = query(filter);

    // Store in cache
    QueryCache newCache;
    newCache.filter = filter;
    newCache.results = results;
    newCache.dirty = false;
    newCache.lastEntityCount = entities.size();

    queryCaches.push_back(newCache);

    // Limit cache size to prevent unbounded growth
    if (queryCaches.size() > 32) {
        queryCaches.erase(queryCaches.begin());
    }

    return results;
}

void EntityManager::invalidateQueryCaches() {
    for (auto& cache : queryCaches) {
        cache.dirty = true;
    }
}

void EntityManager::clearQueryCaches() {
    queryCaches.clear();
    cacheHits = 0;
    cacheMisses = 0;
}

void EntityManager::getCacheStatistics(size_t& hits, size_t& misses) const {
    hits = cacheHits;
    misses = cacheMisses;
}

}  // namespace ECS
}  // namespace JJM
