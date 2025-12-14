#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "scene/Scene.h"
#include "ecs/Entity.h"

namespace JJM {
namespace Scene {

enum class SerializationFormat {
    JSON,
    Binary,
    XML
};

class SceneSerializer {
public:
    SceneSerializer();
    ~SceneSerializer();
    
    bool saveScene(Scene* scene, const std::string& path, 
                   SerializationFormat format = SerializationFormat::JSON);
    
    bool loadScene(Scene* scene, const std::string& path,
                   SerializationFormat format = SerializationFormat::JSON);
    
    std::string serializeScene(Scene* scene, SerializationFormat format);
    bool deserializeScene(Scene* scene, const std::string& data, SerializationFormat format);
    
    void setOnEntitySerialized(std::function<void(ECS::Entity*)> callback) {
        onEntitySerialized = callback;
    }
    
    void setOnEntityDeserialized(std::function<void(ECS::Entity*)> callback) {
        onEntityDeserialized = callback;
    }

private:
    std::function<void(ECS::Entity*)> onEntitySerialized;
    std::function<void(ECS::Entity*)> onEntityDeserialized;
    
    std::string serializeToJSON(Scene* scene);
    std::string serializeToBinary(Scene* scene);
    std::string serializeToXML(Scene* scene);
    
    bool deserializeFromJSON(Scene* scene, const std::string& data);
    bool deserializeFromBinary(Scene* scene, const std::string& data);
    bool deserializeFromXML(Scene* scene, const std::string& data);
    
    std::string serializeEntity(ECS::Entity* entity);
    ECS::Entity* deserializeEntity(Scene* scene, const std::string& data);
};

class EntitySerializer {
public:
    EntitySerializer();
    ~EntitySerializer();
    
    std::string serialize(ECS::Entity* entity);
    ECS::Entity* deserialize(const std::string& data, ECS::EntityManager* manager);
    
    void registerComponentSerializer(const std::string& componentType,
                                     std::function<std::string(void*)> serializer,
                                     std::function<void*(const std::string&)> deserializer);

private:
    struct ComponentSerializer {
        std::function<std::string(void*)> serialize;
        std::function<void*(const std::string&)> deserialize;
    };
    
    std::unordered_map<std::string, ComponentSerializer> componentSerializers;
    
    std::string serializeComponent(const std::string& type, void* component);
    void* deserializeComponent(const std::string& type, const std::string& data);
};

class SceneAsset {
public:
    SceneAsset();
    explicit SceneAsset(const std::string& path);
    ~SceneAsset();
    
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    
    void setScene(std::shared_ptr<Scene> scene) { this->scene = scene; }
    std::shared_ptr<Scene> getScene() const { return scene; }
    
    const std::string& getPath() const { return path; }
    
    void addDependency(const std::string& assetPath);
    const std::vector<std::string>& getDependencies() const { return dependencies; }

private:
    std::string path;
    std::shared_ptr<Scene> scene;
    std::vector<std::string> dependencies;
};

class SceneBundle {
public:
    SceneBundle();
    ~SceneBundle();
    
    void addScene(const std::string& name, std::shared_ptr<Scene> scene);
    void removeScene(const std::string& name);
    
    std::shared_ptr<Scene> getScene(const std::string& name) const;
    
    bool saveToFile(const std::string& path);
    bool loadFromFile(const std::string& path);
    
    const std::vector<std::string>& getSceneNames() const { return sceneNames; }

private:
    std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
    std::vector<std::string> sceneNames;
};

class JSONSceneWriter {
public:
    JSONSceneWriter();
    ~JSONSceneWriter();
    
    std::string writeScene(Scene* scene);
    std::string writeEntity(ECS::Entity* entity);
    
    void setIndentation(int spaces) { indentation = spaces; }
    void setPrettyPrint(bool enabled) { prettyPrint = enabled; }

private:
    int indentation;
    bool prettyPrint;
    int currentIndentLevel;
    
    std::string indent();
    std::string writeObject(const std::string& key, const std::string& value, bool isLast = false);
    std::string writeArray(const std::string& key, const std::vector<std::string>& values);
    std::string escapeString(const std::string& str);
};

class JSONSceneReader {
public:
    JSONSceneReader();
    ~JSONSceneReader();
    
    bool readScene(Scene* scene, const std::string& json);
    ECS::Entity* readEntity(const std::string& json, ECS::EntityManager* manager);
    
    std::string getValue(const std::string& json, const std::string& key);
    std::vector<std::string> getArray(const std::string& json, const std::string& key);

private:
    std::string unescapeString(const std::string& str);
    size_t findKey(const std::string& json, const std::string& key);
};

class BinarySceneWriter {
public:
    BinarySceneWriter();
    ~BinarySceneWriter();
    
    std::vector<uint8_t> writeScene(Scene* scene);
    std::vector<uint8_t> writeEntity(ECS::Entity* entity);
    
    void writeInt32(std::vector<uint8_t>& buffer, int32_t value);
    void writeFloat(std::vector<uint8_t>& buffer, float value);
    void writeString(std::vector<uint8_t>& buffer, const std::string& value);
    void writeBool(std::vector<uint8_t>& buffer, bool value);

private:
    void writeBytes(std::vector<uint8_t>& buffer, const uint8_t* data, size_t size);
};

class BinarySceneReader {
public:
    BinarySceneReader();
    ~BinarySceneReader();
    
    bool readScene(Scene* scene, const std::vector<uint8_t>& data);
    ECS::Entity* readEntity(const std::vector<uint8_t>& data, ECS::EntityManager* manager);
    
    int32_t readInt32(const std::vector<uint8_t>& data, size_t& offset);
    float readFloat(const std::vector<uint8_t>& data, size_t& offset);
    std::string readString(const std::vector<uint8_t>& data, size_t& offset);
    bool readBool(const std::vector<uint8_t>& data, size_t& offset);

private:
    void readBytes(const std::vector<uint8_t>& data, size_t& offset, uint8_t* buffer, size_t size);
};

} // namespace Scene
} // namespace JJM
