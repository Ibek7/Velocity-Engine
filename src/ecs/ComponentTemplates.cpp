#include "ecs/ComponentTemplates.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace JJM {
namespace ECS {

// ComponentTemplate implementation
void ComponentTemplate::setProperty(const std::string& key, const std::string& value) {
    properties[key] = value;
}

std::string ComponentTemplate::getProperty(const std::string& key, const std::string& defaultValue) const {
    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }
    return defaultValue;
}

bool ComponentTemplate::hasProperty(const std::string& key) const {
    return properties.find(key) != properties.end();
}

void ComponentTemplate::clear() {
    properties.clear();
}

// EntityTemplate implementation
void EntityTemplate::addComponent(const ComponentTemplate& component) {
    components.push_back(component);
}

void EntityTemplate::removeComponent(const std::string& componentName) {
    components.erase(
        std::remove_if(components.begin(), components.end(),
            [&componentName](const ComponentTemplate& c) {
                return c.name == componentName;
            }),
        components.end()
    );
}

void EntityTemplate::clear() {
    components.clear();
}

// TemplateManager implementation
TemplateManager* TemplateManager::instance = nullptr;

TemplateManager::TemplateManager() {
}

TemplateManager* TemplateManager::getInstance() {
    if (!instance) {
        instance = new TemplateManager();
    }
    return instance;
}

TemplateManager::~TemplateManager() {
}

void TemplateManager::registerEntityTemplate(const std::string& name, const EntityTemplate& entityTemplate) {
    entityTemplates[name] = entityTemplate;
}

void TemplateManager::registerComponentTemplate(const std::string& name, const ComponentTemplate& componentTemplate) {
    componentTemplates[name] = componentTemplate;
}

void TemplateManager::registerComponentApplier(const std::string& componentType, ComponentApplier applier) {
    componentAppliers[componentType] = applier;
}

EntityTemplate* TemplateManager::getEntityTemplate(const std::string& name) {
    auto it = entityTemplates.find(name);
    if (it != entityTemplates.end()) {
        return &it->second;
    }
    return nullptr;
}

const EntityTemplate* TemplateManager::getEntityTemplate(const std::string& name) const {
    auto it = entityTemplates.find(name);
    if (it != entityTemplates.end()) {
        return &it->second;
    }
    return nullptr;
}

ComponentTemplate* TemplateManager::getComponentTemplate(const std::string& name) {
    auto it = componentTemplates.find(name);
    if (it != componentTemplates.end()) {
        return &it->second;
    }
    return nullptr;
}

const ComponentTemplate* TemplateManager::getComponentTemplate(const std::string& name) const {
    auto it = componentTemplates.find(name);
    if (it != componentTemplates.end()) {
        return &it->second;
    }
    return nullptr;
}

Entity* TemplateManager::createEntityFromTemplate(const std::string& templateName) {
    auto* entityTemplate = getEntityTemplate(templateName);
    if (!entityTemplate) {
        std::cerr << "Entity template not found: " << templateName << std::endl;
        return nullptr;
    }
    
    Entity* entity = new Entity();
    applyTemplateToEntity(entity, templateName);
    return entity;
}

void TemplateManager::applyTemplateToEntity(Entity* entity, const std::string& templateName) {
    if (!entity) return;
    
    auto* entityTemplate = getEntityTemplate(templateName);
    if (!entityTemplate) {
        std::cerr << "Entity template not found: " << templateName << std::endl;
        return;
    }
    
    for (const auto& componentTemplate : entityTemplate->components) {
        applyComponentTemplate(entity, componentTemplate);
    }
}

bool TemplateManager::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open template file: " << filepath << std::endl;
        return false;
    }
    
    std::string line;
    EntityTemplate* currentEntityTemplate = nullptr;
    ComponentTemplate* currentComponentTemplate = nullptr;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "entity") {
            std::string name;
            iss >> name;
            
            EntityTemplate temp(name);
            entityTemplates[name] = temp;
            currentEntityTemplate = &entityTemplates[name];
            currentComponentTemplate = nullptr;
        }
        else if (command == "component" && currentEntityTemplate) {
            std::string name;
            iss >> name;
            
            ComponentTemplate comp(name);
            currentEntityTemplate->addComponent(comp);
            currentComponentTemplate = &currentEntityTemplate->components.back();
        }
        else if (command == "property" && currentComponentTemplate) {
            std::string key, value;
            iss >> key;
            std::getline(iss, value);
            
            // Trim leading whitespace from value
            size_t start = value.find_first_not_of(" \t");
            if (start != std::string::npos) {
                value = value.substr(start);
            }
            
            currentComponentTemplate->setProperty(key, value);
        }
    }
    
    file.close();
    return true;
}

bool TemplateManager::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to create template file: " << filepath << std::endl;
        return false;
    }
    
    for (const auto& pair : entityTemplates) {
        file << "entity " << pair.first << "\n";
        
        for (const auto& component : pair.second.components) {
            file << "  component " << component.name << "\n";
            
            for (const auto& prop : component.properties) {
                file << "    property " << prop.first << " " << prop.second << "\n";
            }
        }
        
        file << "\n";
    }
    
    file.close();
    return true;
}

std::vector<std::string> TemplateManager::getEntityTemplateNames() const {
    std::vector<std::string> names;
    for (const auto& pair : entityTemplates) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<std::string> TemplateManager::getComponentTemplateNames() const {
    std::vector<std::string> names;
    for (const auto& pair : componentTemplates) {
        names.push_back(pair.first);
    }
    return names;
}

void TemplateManager::clear() {
    entityTemplates.clear();
    componentTemplates.clear();
}

void TemplateManager::applyComponentTemplate(Entity* entity, const ComponentTemplate& componentTemplate) {
    auto it = componentAppliers.find(componentTemplate.name);
    if (it != componentAppliers.end()) {
        it->second(entity, componentTemplate);
    } else {
        std::cerr << "No applier registered for component type: "
                  << componentTemplate.name << std::endl;
    }
}

} // namespace ECS
} // namespace JJM
