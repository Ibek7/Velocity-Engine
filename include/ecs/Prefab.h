#pragma once

#include "ecs/Entity.h"
#include "ecs/ComponentSerialization.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace JJM {
namespace ECS {

class EntityManager;

class Prefab {
public:
    Prefab(const std::string& name);
    ~Prefab();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void setTemplate(const std::string& templateData);
    const std::string& getTemplate() const { return templateData; }
    
    Entity instantiate(EntityManager* manager) const;
    
    void addProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key) const;
    
    void setParent(Prefab* parent) { this->parent = parent; }
    Prefab* getParent() const { return parent; }

private:
    std::string name;
    std::string templateData;
    std::unordered_map<std::string, std::string> properties;
    Prefab* parent;
};

class PrefabManager {
public:
    PrefabManager();
    ~PrefabManager();
    
    void registerPrefab(const std::string& name, std::shared_ptr<Prefab> prefab);
    std::shared_ptr<Prefab> getPrefab(const std::string& name) const;
    
    bool hasPrefab(const std::string& name) const;
    void removePrefab(const std::string& name);
    
    Entity instantiate(const std::string& name, EntityManager* manager) const;
    
    std::shared_ptr<Prefab> createPrefab(const std::string& name);
    std::shared_ptr<Prefab> createFromEntity(const std::string& name, const Entity& entity);
    
    void clear();
    
    size_t getPrefabCount() const { return prefabs.size(); }
    std::vector<std::string> getPrefabNames() const;

private:
    std::unordered_map<std::string, std::shared_ptr<Prefab>> prefabs;
};

class PrefabLoader {
public:
    PrefabLoader();
    ~PrefabLoader();
    
    std::shared_ptr<Prefab> loadFromFile(const std::string& filepath);
    bool saveToFile(const Prefab& prefab, const std::string& filepath);
    
    std::shared_ptr<Prefab> loadFromJSON(const std::string& json);
    std::string saveToJSON(const Prefab& prefab);

private:
    std::string readFile(const std::string& filepath);
    bool writeFile(const std::string& filepath, const std::string& content);
};

class PrefabVariant {
public:
    PrefabVariant(const std::string& name, Prefab* basePrefab);
    ~PrefabVariant();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void overrideProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key) const;
    
    Entity instantiate(EntityManager* manager) const;

private:
    std::string name;
    Prefab* basePrefab;
    std::unordered_map<std::string, std::string> overrides;
};

class PrefabInstance {
public:
    PrefabInstance(const Entity& entity, const std::string& prefabName);
    ~PrefabInstance();
    
    const Entity& getEntity() const { return entity; }
    const std::string& getPrefabName() const { return prefabName; }
    
    void setPrefabName(const std::string& name) { prefabName = name; }
    
    bool isModified() const { return modified; }
    void setModified(bool modified) { this->modified = modified; }
    
    void revertToPrefab();
    void applyModifications();

private:
    Entity entity;
    std::string prefabName;
    bool modified;
};

class PrefabInstanceManager {
public:
    PrefabInstanceManager();
    ~PrefabInstanceManager();
    
    void registerInstance(const Entity& entity, const std::string& prefabName);
    void unregisterInstance(const Entity& entity);
    
    PrefabInstance* getInstance(const Entity& entity);
    bool isInstance(const Entity& entity) const;
    
    std::vector<PrefabInstance*> getInstancesOfPrefab(const std::string& prefabName);
    
    void updateAllInstances(const std::string& prefabName);

private:
    std::unordered_map<EntityID, std::unique_ptr<PrefabInstance>> instances;
};

} // namespace ECS
} // namespace JJM
