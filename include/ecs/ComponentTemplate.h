#pragma once

#include "ecs/Component.h"
#include "ecs/Entity.h"
#include "ecs/EntityManager.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>

namespace JJM {
namespace ECS {

class ComponentTemplate {
public:
    ComponentTemplate();
    explicit ComponentTemplate(const std::string& name);
    virtual ~ComponentTemplate();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void setDescription(const std::string& desc) { description = desc; }
    const std::string& getDescription() const { return description; }
    
    void setCategory(const std::string& cat) { category = cat; }
    const std::string& getCategory() const { return category; }
    
    virtual std::shared_ptr<Component> instantiate() = 0;
    virtual void applyToComponent(Component* component) = 0;
    
    void addProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key) const;
    bool hasProperty(const std::string& key) const;
    
    const std::unordered_map<std::string, std::string>& getProperties() const {
        return properties;
    }

protected:
    std::string name;
    std::string description;
    std::string category;
    std::unordered_map<std::string, std::string> properties;
};

class ComponentTemplateLibrary {
public:
    static ComponentTemplateLibrary& getInstance();
    
    void registerTemplate(std::shared_ptr<ComponentTemplate> templ);
    void unregisterTemplate(const std::string& name);
    
    std::shared_ptr<ComponentTemplate> getTemplate(const std::string& name) const;
    std::vector<std::string> getTemplateNames() const;
    std::vector<std::string> getTemplatesByCategory(const std::string& category) const;
    
    void clear();
    
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);

private:
    ComponentTemplateLibrary() {}
    ~ComponentTemplateLibrary() {}
    ComponentTemplateLibrary(const ComponentTemplateLibrary&) = delete;
    ComponentTemplateLibrary& operator=(const ComponentTemplateLibrary&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<ComponentTemplate>> templates;
};

template<typename T>
class TypedComponentTemplate : public ComponentTemplate {
public:
    TypedComponentTemplate() {}
    explicit TypedComponentTemplate(const std::string& name) : ComponentTemplate(name) {}
    virtual ~TypedComponentTemplate() {}
    
    void setInitializer(std::function<void(T*)> init) {
        initializer = init;
    }
    
    std::shared_ptr<Component> instantiate() override {
        auto component = std::make_shared<T>();
        if (initializer) {
            initializer(component.get());
        }
        applyToComponent(component.get());
        return component;
    }
    
    void applyToComponent(Component* component) override {
        T* typedComponent = dynamic_cast<T*>(component);
        if (typedComponent && initializer) {
            initializer(typedComponent);
        }
    }

private:
    std::function<void(T*)> initializer;
};

class ComponentTemplateBuilder {
public:
    ComponentTemplateBuilder();
    explicit ComponentTemplateBuilder(const std::string& name);
    
    ComponentTemplateBuilder& setName(const std::string& name);
    ComponentTemplateBuilder& setDescription(const std::string& desc);
    ComponentTemplateBuilder& setCategory(const std::string& category);
    ComponentTemplateBuilder& addProperty(const std::string& key, const std::string& value);
    
    template<typename T>
    ComponentTemplateBuilder& setInitializer(std::function<void(T*)> init) {
        initializer = [init](Component* component) {
            T* typed = dynamic_cast<T*>(component);
            if (typed) init(typed);
        };
        return *this;
    }
    
    std::shared_ptr<ComponentTemplate> build();

private:
    std::string name;
    std::string description;
    std::string category;
    std::unordered_map<std::string, std::string> properties;
    std::function<void(Component*)> initializer;
};

class ComponentPreset {
public:
    ComponentPreset();
    explicit ComponentPreset(const std::string& name);
    ~ComponentPreset();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void addComponentTemplate(const std::string& templateName);
    void removeComponentTemplate(const std::string& templateName);
    
    const std::vector<std::string>& getComponentTemplates() const {
        return componentTemplates;
    }
    
    void applyToEntity(Entity* entity);

private:
    std::string name;
    std::vector<std::string> componentTemplates;
};

class ComponentPresetLibrary {
public:
    static ComponentPresetLibrary& getInstance();
    
    void registerPreset(std::shared_ptr<ComponentPreset> preset);
    void unregisterPreset(const std::string& name);
    
    std::shared_ptr<ComponentPreset> getPreset(const std::string& name) const;
    std::vector<std::string> getPresetNames() const;
    
    void clear();
    
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);

private:
    ComponentPresetLibrary() {}
    ~ComponentPresetLibrary() {}
    ComponentPresetLibrary(const ComponentPresetLibrary&) = delete;
    ComponentPresetLibrary& operator=(const ComponentPresetLibrary&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<ComponentPreset>> presets;
};

class EntityTemplate {
public:
    EntityTemplate();
    explicit EntityTemplate(const std::string& name);
    ~EntityTemplate();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void setPreset(const std::string& presetName) { preset = presetName; }
    const std::string& getPreset() const { return preset; }
    
    void addComponent(const std::string& templateName);
    void removeComponent(const std::string& templateName);
    
    const std::vector<std::string>& getComponents() const {
        return components;
    }
    
    Entity* instantiate(EntityManager* manager);

private:
    std::string name;
    std::string preset;
    std::vector<std::string> components;
};

class EntityTemplateLibrary {
public:
    static EntityTemplateLibrary& getInstance();
    
    void registerTemplate(std::shared_ptr<EntityTemplate> templ);
    void unregisterTemplate(const std::string& name);
    
    std::shared_ptr<EntityTemplate> getTemplate(const std::string& name) const;
    std::vector<std::string> getTemplateNames() const;
    
    void clear();
    
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);

private:
    EntityTemplateLibrary() {}
    ~EntityTemplateLibrary() {}
    EntityTemplateLibrary(const EntityTemplateLibrary&) = delete;
    EntityTemplateLibrary& operator=(const EntityTemplateLibrary&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<EntityTemplate>> templates;
};

} // namespace ECS
} // namespace JJM
