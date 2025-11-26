#ifndef COMPONENT_FACTORY_H
#define COMPONENT_FACTORY_H

#include "ecs/Component.h"
#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

namespace JJM {
namespace ECS {

class ComponentFactory {
private:
    using CreatorFunc = std::function<Component*()>;
    std::unordered_map<std::string, CreatorFunc> creators;
    
    static ComponentFactory* instance;
    ComponentFactory();
    
public:
    static ComponentFactory* getInstance();
    ~ComponentFactory();
    
    template<typename T>
    void registerComponent(const std::string& name) {
        creators[name] = []() -> Component* {
            return new T();
        };
    }
    
    Component* createComponent(const std::string& name);
    
    bool isRegistered(const std::string& name) const;
    std::vector<std::string> getRegisteredComponents() const;
    
    void unregisterComponent(const std::string& name);
    void clear();
};

// Template specialization for registering components with parameters
template<typename T, typename... Args>
class ParameterizedComponentCreator {
public:
    static void registerComponent(ComponentFactory* factory, const std::string& name, Args... args) {
        factory->registerComponent<T>(name);
    }
};

} // namespace ECS
} // namespace JJM

#endif // COMPONENT_FACTORY_H
