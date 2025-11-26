#include "ecs/ComponentFactory.h"
#include <iostream>

namespace JJM {
namespace ECS {

ComponentFactory* ComponentFactory::instance = nullptr;

ComponentFactory::ComponentFactory() {
}

ComponentFactory* ComponentFactory::getInstance() {
    if (!instance) {
        instance = new ComponentFactory();
    }
    return instance;
}

ComponentFactory::~ComponentFactory() {
    clear();
}

Component* ComponentFactory::createComponent(const std::string& name) {
    auto it = creators.find(name);
    if (it != creators.end()) {
        return it->second();
    }
    
    std::cerr << "Component type '" << name << "' not registered in factory" << std::endl;
    return nullptr;
}

bool ComponentFactory::isRegistered(const std::string& name) const {
    return creators.find(name) != creators.end();
}

std::vector<std::string> ComponentFactory::getRegisteredComponents() const {
    std::vector<std::string> names;
    for (const auto& pair : creators) {
        names.push_back(pair.first);
    }
    return names;
}

void ComponentFactory::unregisterComponent(const std::string& name) {
    creators.erase(name);
}

void ComponentFactory::clear() {
    creators.clear();
}

} // namespace ECS
} // namespace JJM
