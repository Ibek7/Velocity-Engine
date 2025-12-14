#include "ecs/ComponentTemplate.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace JJM {
namespace ECS {

// ComponentTemplate implementation
ComponentTemplate::ComponentTemplate() {}

ComponentTemplate::ComponentTemplate(const std::string& name) : name(name) {}

ComponentTemplate::~ComponentTemplate() {}

void ComponentTemplate::addProperty(const std::string& key, const std::string& value) {
    properties[key] = value;
}

std::string ComponentTemplate::getProperty(const std::string& key) const {
    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }
    return "";
}

bool ComponentTemplate::hasProperty(const std::string& key) const {
    return properties.find(key) != properties.end();
}

// ComponentTemplateLibrary implementation
ComponentTemplateLibrary& ComponentTemplateLibrary::getInstance() {
    static ComponentTemplateLibrary instance;
    return instance;
}

void ComponentTemplateLibrary::registerTemplate(std::shared_ptr<ComponentTemplate> templ) {
    if (templ) {
        templates[templ->getName()] = templ;
    }
}

void ComponentTemplateLibrary::unregisterTemplate(const std::string& name) {
    templates.erase(name);
}

std::shared_ptr<ComponentTemplate> ComponentTemplateLibrary::getTemplate(const std::string& name) const {
    auto it = templates.find(name);
    if (it != templates.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> ComponentTemplateLibrary::getTemplateNames() const {
    std::vector<std::string> names;
    for (const auto& pair : templates) {
        names.push_back(pair.first);
    }
    return names;
}

std::vector<std::string> ComponentTemplateLibrary::getTemplatesByCategory(const std::string& category) const {
    std::vector<std::string> names;
    for (const auto& pair : templates) {
        if (pair.second->getCategory() == category) {
            names.push_back(pair.first);
        }
    }
    return names;
}

void ComponentTemplateLibrary::clear() {
    templates.clear();
}

bool ComponentTemplateLibrary::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Parse template file format
    return true;
}

bool ComponentTemplateLibrary::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Write templates to file
    for (const auto& pair : templates) {
        file << pair.first << "\n";
    }
    
    return true;
}

// ComponentTemplateBuilder implementation
ComponentTemplateBuilder::ComponentTemplateBuilder() {}

ComponentTemplateBuilder::ComponentTemplateBuilder(const std::string& name) : name(name) {}

ComponentTemplateBuilder& ComponentTemplateBuilder::setName(const std::string& name) {
    this->name = name;
    return *this;
}

ComponentTemplateBuilder& ComponentTemplateBuilder::setDescription(const std::string& desc) {
    this->description = desc;
    return *this;
}

ComponentTemplateBuilder& ComponentTemplateBuilder::setCategory(const std::string& category) {
    this->category = category;
    return *this;
}

ComponentTemplateBuilder& ComponentTemplateBuilder::addProperty(const std::string& key, const std::string& value) {
    properties[key] = value;
    return *this;
}

std::shared_ptr<ComponentTemplate> ComponentTemplateBuilder::build() {
    // Create a generic component template
    class GenericComponentTemplate : public ComponentTemplate {
    public:
        GenericComponentTemplate(const std::string& name,
                                const std::string& desc,
                                const std::string& cat,
                                const std::unordered_map<std::string, std::string>& props,
                                std::function<void(Component*)> init)
            : ComponentTemplate(name), initializer(init) {
            description = desc;
            category = cat;
            properties = props;
        }
        
        std::shared_ptr<Component> instantiate() override {
            return nullptr;
        }
        
        void applyToComponent(Component* component) override {
            if (component && initializer) {
                initializer(component);
            }
        }
        
    private:
        std::function<void(Component*)> initializer;
    };
    
    return std::make_shared<GenericComponentTemplate>(
        name, description, category, properties, initializer);
}

// ComponentPreset implementation
ComponentPreset::ComponentPreset() {}

ComponentPreset::ComponentPreset(const std::string& name) : name(name) {}

ComponentPreset::~ComponentPreset() {}

void ComponentPreset::addComponentTemplate(const std::string& templateName) {
    componentTemplates.push_back(templateName);
}

void ComponentPreset::removeComponentTemplate(const std::string& templateName) {
    componentTemplates.erase(
        std::remove(componentTemplates.begin(), componentTemplates.end(), templateName),
        componentTemplates.end());
}

void ComponentPreset::applyToEntity(Entity* entity) {
    if (!entity) return;
    
    auto& library = ComponentTemplateLibrary::getInstance();
    for (const auto& templateName : componentTemplates) {
        auto templ = library.getTemplate(templateName);
        if (templ) {
            // Template instantiation creates component
            // Entity's addComponent requires template args, so we skip direct add
            // This would need runtime type info to properly add components
        }
    }
}

// ComponentPresetLibrary implementation
ComponentPresetLibrary& ComponentPresetLibrary::getInstance() {
    static ComponentPresetLibrary instance;
    return instance;
}

void ComponentPresetLibrary::registerPreset(std::shared_ptr<ComponentPreset> preset) {
    if (preset) {
        presets[preset->getName()] = preset;
    }
}

void ComponentPresetLibrary::unregisterPreset(const std::string& name) {
    presets.erase(name);
}

std::shared_ptr<ComponentPreset> ComponentPresetLibrary::getPreset(const std::string& name) const {
    auto it = presets.find(name);
    if (it != presets.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> ComponentPresetLibrary::getPresetNames() const {
    std::vector<std::string> names;
    for (const auto& pair : presets) {
        names.push_back(pair.first);
    }
    return names;
}

void ComponentPresetLibrary::clear() {
    presets.clear();
}

bool ComponentPresetLibrary::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Parse preset file
    return true;
}

bool ComponentPresetLibrary::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Write presets to file
    for (const auto& pair : presets) {
        file << pair.first << "\n";
    }
    
    return true;
}

// EntityTemplate implementation
EntityTemplate::EntityTemplate() {}

EntityTemplate::EntityTemplate(const std::string& name) : name(name) {}

EntityTemplate::~EntityTemplate() {}

void EntityTemplate::addComponent(const std::string& templateName) {
    components.push_back(templateName);
}

void EntityTemplate::removeComponent(const std::string& templateName) {
    components.erase(
        std::remove(components.begin(), components.end(), templateName),
        components.end());
}

Entity* EntityTemplate::instantiate(EntityManager* manager) {
    if (!manager) return nullptr;
    
    Entity* entity = manager->createEntity();
    if (!entity) return nullptr;
    
    // Apply preset if specified
    if (!preset.empty()) {
        auto& presetLib = ComponentPresetLibrary::getInstance();
        auto presetObj = presetLib.getPreset(preset);
        if (presetObj) {
            presetObj->applyToEntity(entity);
        }
    }
    
    // Add individual components
    auto& library = ComponentTemplateLibrary::getInstance();
    for (const auto& componentName : components) {
        auto templ = library.getTemplate(componentName);
        if (templ) {
            // Template instantiation creates component
            // Entity's addComponent requires template args, so we skip direct add
            // This would need runtime type info or component factory to properly add components
        }
    }
    
    return entity;
}

// EntityTemplateLibrary implementation
EntityTemplateLibrary& EntityTemplateLibrary::getInstance() {
    static EntityTemplateLibrary instance;
    return instance;
}

void EntityTemplateLibrary::registerTemplate(std::shared_ptr<EntityTemplate> templ) {
    if (templ) {
        templates[templ->getName()] = templ;
    }
}

void EntityTemplateLibrary::unregisterTemplate(const std::string& name) {
    templates.erase(name);
}

std::shared_ptr<EntityTemplate> EntityTemplateLibrary::getTemplate(const std::string& name) const {
    auto it = templates.find(name);
    if (it != templates.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::string> EntityTemplateLibrary::getTemplateNames() const {
    std::vector<std::string> names;
    for (const auto& pair : templates) {
        names.push_back(pair.first);
    }
    return names;
}

void EntityTemplateLibrary::clear() {
    templates.clear();
}

bool EntityTemplateLibrary::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Parse entity templates
    return true;
}

bool EntityTemplateLibrary::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    // Write entity templates
    for (const auto& pair : templates) {
        file << pair.first << "\n";
    }
    
    return true;
}

} // namespace ECS
} // namespace JJM
