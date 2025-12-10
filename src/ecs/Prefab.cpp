#include "ecs/Prefab.h"
#include "ecs/EntityManager.h"
#include <fstream>
#include <sstream>

namespace JJM {
namespace ECS {

// Prefab implementation
Prefab::Prefab(const std::string& name)
    : name(name), templateData(""), parent(nullptr) {}

Prefab::~Prefab() {}

void Prefab::setTemplate(const std::string& templateData) {
    this->templateData = templateData;
}

Entity Prefab::instantiate(EntityManager* manager) const {
    if (!manager) {
        return Entity(0, nullptr);
    }
    
    Entity* entity = manager->createEntity();
    
    return entity ? *entity : Entity(0, nullptr);
}

void Prefab::addProperty(const std::string& key, const std::string& value) {
    properties[key] = value;
}

std::string Prefab::getProperty(const std::string& key) const {
    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }
    
    if (parent) {
        return parent->getProperty(key);
    }
    
    return "";
}

// PrefabManager implementation
PrefabManager::PrefabManager() {}

PrefabManager::~PrefabManager() {}

void PrefabManager::registerPrefab(const std::string& name, std::shared_ptr<Prefab> prefab) {
    prefabs[name] = prefab;
}

std::shared_ptr<Prefab> PrefabManager::getPrefab(const std::string& name) const {
    auto it = prefabs.find(name);
    if (it != prefabs.end()) {
        return it->second;
    }
    return nullptr;
}

bool PrefabManager::hasPrefab(const std::string& name) const {
    return prefabs.find(name) != prefabs.end();
}

void PrefabManager::removePrefab(const std::string& name) {
    prefabs.erase(name);
}

Entity PrefabManager::instantiate(const std::string& name, EntityManager* manager) const {
    auto prefab = getPrefab(name);
    if (prefab) {
        return prefab->instantiate(manager);
    }
    return Entity(0, nullptr);
}

std::shared_ptr<Prefab> PrefabManager::createPrefab(const std::string& name) {
    auto prefab = std::make_shared<Prefab>(name);
    registerPrefab(name, prefab);
    return prefab;
}

std::shared_ptr<Prefab> PrefabManager::createFromEntity(const std::string& name, const Entity& entity) {
    auto prefab = std::make_shared<Prefab>(name);
    
    JSONSerializer serializer;
    std::string entityData = serializer.serializeToJSON(entity);
    prefab->setTemplate(entityData);
    
    registerPrefab(name, prefab);
    return prefab;
}

void PrefabManager::clear() {
    prefabs.clear();
}

std::vector<std::string> PrefabManager::getPrefabNames() const {
    std::vector<std::string> names;
    names.reserve(prefabs.size());
    
    for (const auto& pair : prefabs) {
        names.push_back(pair.first);
    }
    
    return names;
}

// PrefabLoader implementation
PrefabLoader::PrefabLoader() {}

PrefabLoader::~PrefabLoader() {}

std::shared_ptr<Prefab> PrefabLoader::loadFromFile(const std::string& filepath) {
    std::string content = readFile(filepath);
    if (content.empty()) {
        return nullptr;
    }
    
    return loadFromJSON(content);
}

bool PrefabLoader::saveToFile(const Prefab& prefab, const std::string& filepath) {
    std::string json = saveToJSON(prefab);
    return writeFile(filepath, json);
}

std::shared_ptr<Prefab> PrefabLoader::loadFromJSON(const std::string& json) {
    size_t namePos = json.find("\"name\":");
    if (namePos == std::string::npos) {
        return nullptr;
    }
    
    size_t nameStart = json.find('"', namePos + 7);
    size_t nameEnd = json.find('"', nameStart + 1);
    
    std::string name = json.substr(nameStart + 1, nameEnd - nameStart - 1);
    
    auto prefab = std::make_shared<Prefab>(name);
    
    size_t templatePos = json.find("\"template\":");
    if (templatePos != std::string::npos) {
        size_t templateStart = json.find('"', templatePos + 11);
        size_t templateEnd = json.find('"', templateStart + 1);
        
        std::string templateData = json.substr(templateStart + 1, 
                                               templateEnd - templateStart - 1);
        prefab->setTemplate(templateData);
    }
    
    return prefab;
}

std::string PrefabLoader::saveToJSON(const Prefab& prefab) {
    std::stringstream ss;
    
    ss << "{\n";
    ss << "  \"name\": \"" << prefab.getName() << "\",\n";
    ss << "  \"template\": \"" << prefab.getTemplate() << "\"\n";
    ss << "}";
    
    return ss.str();
}

std::string PrefabLoader::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return buffer.str();
}

bool PrefabLoader::writeFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }
    
    file << content;
    return true;
}

// PrefabVariant implementation
PrefabVariant::PrefabVariant(const std::string& name, Prefab* basePrefab)
    : name(name), basePrefab(basePrefab) {}

PrefabVariant::~PrefabVariant() {}

void PrefabVariant::overrideProperty(const std::string& key, const std::string& value) {
    overrides[key] = value;
}

std::string PrefabVariant::getProperty(const std::string& key) const {
    auto it = overrides.find(key);
    if (it != overrides.end()) {
        return it->second;
    }
    
    if (basePrefab) {
        return basePrefab->getProperty(key);
    }
    
    return "";
}

Entity PrefabVariant::instantiate(EntityManager* manager) const {
    if (!basePrefab) {
        return Entity(0, nullptr);
    }
    
    Entity entity = basePrefab->instantiate(manager);
    
    return entity;
}

// PrefabInstance implementation
PrefabInstance::PrefabInstance(const Entity& entity, const std::string& prefabName)
    : entity(entity), prefabName(prefabName), modified(false) {}

PrefabInstance::~PrefabInstance() {}

void PrefabInstance::revertToPrefab() {
    modified = false;
}

void PrefabInstance::applyModifications() {
    modified = true;
}

// PrefabInstanceManager implementation
PrefabInstanceManager::PrefabInstanceManager() {}

PrefabInstanceManager::~PrefabInstanceManager() {}

void PrefabInstanceManager::registerInstance(const Entity& entity, const std::string& prefabName) {
    instances[entity.getID()] = std::make_unique<PrefabInstance>(entity, prefabName);
}

void PrefabInstanceManager::unregisterInstance(const Entity& entity) {
    instances.erase(entity.getID());
}

PrefabInstance* PrefabInstanceManager::getInstance(const Entity& entity) {
    auto it = instances.find(entity.getID());
    if (it != instances.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool PrefabInstanceManager::isInstance(const Entity& entity) const {
    return instances.find(entity.getID()) != instances.end();
}

std::vector<PrefabInstance*> PrefabInstanceManager::getInstancesOfPrefab(const std::string& prefabName) {
    std::vector<PrefabInstance*> result;
    
    for (auto& pair : instances) {
        if (pair.second->getPrefabName() == prefabName) {
            result.push_back(pair.second.get());
        }
    }
    
    return result;
}

void PrefabInstanceManager::updateAllInstances(const std::string& prefabName) {
    auto instanceList = getInstancesOfPrefab(prefabName);
    
    for (auto* instance : instanceList) {
        if (!instance->isModified()) {
            instance->revertToPrefab();
        }
    }
}

} // namespace ECS
} // namespace JJM
