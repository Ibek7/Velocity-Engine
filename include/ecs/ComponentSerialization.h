#pragma once

#include "ecs/Entity.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace ECS {

class Component;

class ComponentSerializer {
public:
    virtual ~ComponentSerializer() = default;
    
    virtual std::string serialize(const Component* component) const = 0;
    virtual Component* deserialize(const std::string& data) const = 0;
    
    virtual std::string getComponentType() const = 0;
};

template<typename T>
class TypedComponentSerializer : public ComponentSerializer {
public:
    virtual std::string serialize(const Component* component) const override {
        const T* typedComponent = dynamic_cast<const T*>(component);
        if (!typedComponent) return "";
        
        return serializeImpl(typedComponent);
    }
    
    virtual Component* deserialize(const std::string& data) const override {
        return deserializeImpl(data);
    }
    
    virtual std::string getComponentType() const override {
        return typeid(T).name();
    }

protected:
    virtual std::string serializeImpl(const T* component) const = 0;
    virtual T* deserializeImpl(const std::string& data) const = 0;
};

class SerializationRegistry {
public:
    static SerializationRegistry& getInstance();
    
    void registerSerializer(const std::string& typeName, 
                           std::shared_ptr<ComponentSerializer> serializer);
    
    ComponentSerializer* getSerializer(const std::string& typeName) const;
    
    template<typename T>
    void registerSerializer(std::shared_ptr<TypedComponentSerializer<T>> serializer) {
        registerSerializer(typeid(T).name(), serializer);
    }

private:
    SerializationRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<ComponentSerializer>> serializers;
};

class EntitySerializer {
public:
    EntitySerializer();
    ~EntitySerializer();
    
    std::string serializeEntity(const Entity& entity) const;
    Entity deserializeEntity(const std::string& data) const;
    
    std::string serializeComponent(const Component* component) const;
    Component* deserializeComponent(const std::string& typeName, 
                                    const std::string& data) const;
    
    void setRegistry(SerializationRegistry* registry) { this->registry = registry; }

private:
    SerializationRegistry* registry;
};

class BinarySerializer {
public:
    BinarySerializer();
    ~BinarySerializer();
    
    std::vector<uint8_t> serialize(const Entity& entity) const;
    Entity deserialize(const std::vector<uint8_t>& data) const;
    
    void writeInt(std::vector<uint8_t>& buffer, int value) const;
    void writeFloat(std::vector<uint8_t>& buffer, float value) const;
    void writeString(std::vector<uint8_t>& buffer, const std::string& value) const;
    
    int readInt(const std::vector<uint8_t>& buffer, size_t& offset) const;
    float readFloat(const std::vector<uint8_t>& buffer, size_t& offset) const;
    std::string readString(const std::vector<uint8_t>& buffer, size_t& offset) const;
};

class JSONSerializer {
public:
    JSONSerializer();
    ~JSONSerializer();
    
    std::string serializeToJSON(const Entity& entity) const;
    Entity deserializeFromJSON(const std::string& json) const;
    
    std::string componentToJSON(const Component* component) const;
    Component* componentFromJSON(const std::string& json) const;

private:
    std::string escapeString(const std::string& str) const;
    std::string unescapeString(const std::string& str) const;
};

class SerializationContext {
public:
    SerializationContext();
    ~SerializationContext();
    
    void setProperty(const std::string& key, const std::string& value);
    std::string getProperty(const std::string& key) const;
    bool hasProperty(const std::string& key) const;
    
    void clear();

private:
    std::unordered_map<std::string, std::string> properties;
};

class ComponentFactory {
public:
    using CreateFunc = std::function<Component*()>;
    
    static ComponentFactory& getInstance();
    
    void registerComponent(const std::string& typeName, CreateFunc createFunc);
    Component* createComponent(const std::string& typeName) const;
    
    template<typename T>
    void registerComponent() {
        registerComponent(typeid(T).name(), []() -> Component* {
            return new T();
        });
    }

private:
    ComponentFactory() = default;
    std::unordered_map<std::string, CreateFunc> creators;
};

} // namespace ECS
} // namespace JJM
