#ifndef COMPONENT_TEMPLATES_H
#define COMPONENT_TEMPLATES_H

#include "ecs/Component.h"
#include "ecs/Entity.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace ECS {

class ComponentTemplate {
public:
    std::string name;
    std::map<std::string, std::string> properties;
    
    ComponentTemplate() = default;
    ComponentTemplate(const std::string& name) : name(name) {}
    
    void setProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key, const std::string& defaultValue = "") const;
    bool hasProperty(const std::string& key) const;
    
    void clear();
};

class EntityTemplate {
public:
    std::string name;
    std::vector<ComponentTemplate> components;
    
    EntityTemplate() = default;
    EntityTemplate(const std::string& name) : name(name) {}
    
    void addComponent(const ComponentTemplate& component);
    void removeComponent(const std::string& componentName);
    
    const std::vector<ComponentTemplate>& getComponents() const { return components; }
    int getComponentCount() const { return static_cast<int>(components.size()); }
    
    void clear();
};

class TemplateManager {
private:
    std::map<std::string, EntityTemplate> entityTemplates;
    std::map<std::string, ComponentTemplate> componentTemplates;
    
    using ComponentApplier = std::function<void(Entity*, const ComponentTemplate&)>;
    std::map<std::string, ComponentApplier> componentAppliers;
    
    static TemplateManager* instance;
    TemplateManager();
    
public:
    static TemplateManager* getInstance();
    ~TemplateManager();
    
    void registerEntityTemplate(const std::string& name, const EntityTemplate& entityTemplate);
    void registerComponentTemplate(const std::string& name, const ComponentTemplate& componentTemplate);
    
    void registerComponentApplier(const std::string& componentType, ComponentApplier applier);
    
    EntityTemplate* getEntityTemplate(const std::string& name);
    const EntityTemplate* getEntityTemplate(const std::string& name) const;
    
    ComponentTemplate* getComponentTemplate(const std::string& name);
    const ComponentTemplate* getComponentTemplate(const std::string& name) const;
    
    Entity* createEntityFromTemplate(const std::string& templateName);
    void applyTemplateToEntity(Entity* entity, const std::string& templateName);
    
    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;
    
    std::vector<std::string> getEntityTemplateNames() const;
    std::vector<std::string> getComponentTemplateNames() const;
    
    void clear();
    
private:
    void applyComponentTemplate(Entity* entity, const ComponentTemplate& componentTemplate);
};

// Helper templates for common component types
template<typename T>
ComponentTemplate createNumericPropertyTemplate(const std::string& name, T value) {
    ComponentTemplate temp(name);
    temp.setProperty("value", std::to_string(value));
    return temp;
}

template<typename T>
T getNumericProperty(const ComponentTemplate& temp, const std::string& key, T defaultValue) {
    std::string str = temp.getProperty(key);
    if (str.empty()) return defaultValue;
    
    if constexpr (std::is_same_v<T, int>) {
        return std::stoi(str);
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(str);
    } else if constexpr (std::is_same_v<T, double>) {
        return std::stod(str);
    }
    
    return defaultValue;
}

} // namespace ECS
} // namespace JJM

#endif // COMPONENT_TEMPLATES_H
