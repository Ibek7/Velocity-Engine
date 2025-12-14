#include "scene/SceneSerialization.h"
#include <fstream>
#include <sstream>
#include <cstring>

namespace JJM {
namespace Scene {

// SceneSerializer implementation
SceneSerializer::SceneSerializer() {}

SceneSerializer::~SceneSerializer() {}

bool SceneSerializer::saveScene(Scene* scene, const std::string& path, SerializationFormat format) {
    std::string data = serializeScene(scene, format);
    
    std::ofstream file(path, format == SerializationFormat::Binary ? 
                       std::ios::binary : std::ios::out);
    if (!file.is_open()) {
        return false;
    }
    
    file << data;
    return true;
}

bool SceneSerializer::loadScene(Scene* scene, const std::string& path, SerializationFormat format) {
    std::ifstream file(path, format == SerializationFormat::Binary ?
                       std::ios::binary : std::ios::in);
    if (!file.is_open()) {
        return false;
    }
    
    std::string data((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    
    return deserializeScene(scene, data, format);
}

std::string SceneSerializer::serializeScene(Scene* scene, SerializationFormat format) {
    switch (format) {
        case SerializationFormat::JSON:
            return serializeToJSON(scene);
        case SerializationFormat::Binary:
            return serializeToBinary(scene);
        case SerializationFormat::XML:
            return serializeToXML(scene);
    }
    return "";
}

bool SceneSerializer::deserializeScene(Scene* scene, const std::string& data, SerializationFormat format) {
    switch (format) {
        case SerializationFormat::JSON:
            return deserializeFromJSON(scene, data);
        case SerializationFormat::Binary:
            return deserializeFromBinary(scene, data);
        case SerializationFormat::XML:
            return deserializeFromXML(scene, data);
    }
    return false;
}

std::string SceneSerializer::serializeToJSON(Scene* scene) {
    if (!scene) return "{}";
    
    JSONSceneWriter writer;
    return writer.writeScene(scene);
}

std::string SceneSerializer::serializeToBinary(Scene* scene) {
    if (!scene) return "";
    
    BinarySceneWriter writer;
    auto data = writer.writeScene(scene);
    return std::string(reinterpret_cast<const char*>(data.data()), data.size());
}

std::string SceneSerializer::serializeToXML(Scene* scene) {
    if (!scene) return "<scene/>";
    
    // Basic XML serialization
    return "<scene name=\"" + scene->getName() + "\"></scene>";
}

bool SceneSerializer::deserializeFromJSON(Scene* scene, const std::string& data) {
    if (!scene) return false;
    
    JSONSceneReader reader;
    return reader.readScene(scene, data);
}

bool SceneSerializer::deserializeFromBinary(Scene* scene, const std::string& data) {
    if (!scene) return false;
    
    BinarySceneReader reader;
    std::vector<uint8_t> bytes(data.begin(), data.end());
    return reader.readScene(scene, bytes);
}

bool SceneSerializer::deserializeFromXML(Scene* scene, const std::string& data) {
    // Basic XML deserialization
    return !data.empty() && scene != nullptr;
}

std::string SceneSerializer::serializeEntity(ECS::Entity* entity) {
    if (!entity) return "";
    
    EntitySerializer serializer;
    return serializer.serialize(entity);
}

ECS::Entity* SceneSerializer::deserializeEntity(Scene* scene, const std::string& data) {
    if (!scene || data.empty()) return nullptr;
    
    EntitySerializer serializer;
    return serializer.deserialize(data, nullptr);
}

// EntitySerializer implementation
EntitySerializer::EntitySerializer() {}

EntitySerializer::~EntitySerializer() {}

std::string EntitySerializer::serialize(ECS::Entity* entity) {
    if (!entity) return "";
    
    std::ostringstream oss;
    oss << "{\"id\":" << entity->getID() << "}";
    return oss.str();
}

ECS::Entity* EntitySerializer::deserialize(const std::string& data, ECS::EntityManager* manager) {
    if (data.empty() || !manager) return nullptr;
    
    // Parse entity data and create entity
    return nullptr;
}

void EntitySerializer::registerComponentSerializer(const std::string& componentType,
                                                   std::function<std::string(void*)> serializer,
                                                   std::function<void*(const std::string&)> deserializer) {
    ComponentSerializer cs;
    cs.serialize = serializer;
    cs.deserialize = deserializer;
    componentSerializers[componentType] = cs;
}

std::string EntitySerializer::serializeComponent(const std::string& type, void* component) {
    auto it = componentSerializers.find(type);
    if (it != componentSerializers.end() && component) {
        return it->second.serialize(component);
    }
    return "";
}

void* EntitySerializer::deserializeComponent(const std::string& type, const std::string& data) {
    auto it = componentSerializers.find(type);
    if (it != componentSerializers.end()) {
        return it->second.deserialize(data);
    }
    return nullptr;
}

// SceneAsset implementation
SceneAsset::SceneAsset() {}

SceneAsset::SceneAsset(const std::string& path) : path(path) {
    loadFromFile(path);
}

SceneAsset::~SceneAsset() {}

bool SceneAsset::loadFromFile(const std::string& path) {
    this->path = path;
    
    SceneSerializer serializer;
    scene = std::make_shared<Scene>("LoadedScene");
    return serializer.loadScene(scene.get(), path);
}

bool SceneAsset::saveToFile(const std::string& path) {
    if (!scene) return false;
    
    SceneSerializer serializer;
    return serializer.saveScene(scene.get(), path);
}

void SceneAsset::addDependency(const std::string& assetPath) {
    dependencies.push_back(assetPath);
}

// SceneBundle implementation
SceneBundle::SceneBundle() {}

SceneBundle::~SceneBundle() {}

void SceneBundle::addScene(const std::string& name, std::shared_ptr<Scene> scene) {
    scenes[name] = scene;
    sceneNames.push_back(name);
}

void SceneBundle::removeScene(const std::string& name) {
    scenes.erase(name);
    sceneNames.erase(std::remove(sceneNames.begin(), sceneNames.end(), name),
                    sceneNames.end());
}

std::shared_ptr<Scene> SceneBundle::getScene(const std::string& name) const {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        return it->second;
    }
    return nullptr;
}

bool SceneBundle::saveToFile(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    
    // Write bundle header and scenes
    return true;
}

bool SceneBundle::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    // Read bundle and load scenes
    return true;
}

// JSONSceneWriter implementation
JSONSceneWriter::JSONSceneWriter()
    : indentation(2), prettyPrint(true), currentIndentLevel(0) {}

JSONSceneWriter::~JSONSceneWriter() {}

std::string JSONSceneWriter::writeScene(Scene* scene) {
    if (!scene) return "{}";
    
    std::ostringstream oss;
    oss << "{" << (prettyPrint ? "\n" : "");
    currentIndentLevel++;
    
    oss << indent() << "\"name\": \"" << escapeString(scene->getName()) << "\"";
    
    currentIndentLevel--;
    oss << (prettyPrint ? "\n" : "") << "}";
    
    return oss.str();
}

std::string JSONSceneWriter::writeEntity(ECS::Entity* entity) {
    if (!entity) return "{}";
    
    std::ostringstream oss;
    oss << "{\"id\":" << entity->getID() << "}";
    return oss.str();
}

std::string JSONSceneWriter::indent() {
    if (!prettyPrint) return "";
    return std::string(currentIndentLevel * indentation, ' ');
}

std::string JSONSceneWriter::writeObject(const std::string& key, const std::string& value, bool isLast) {
    std::string result = indent() + "\"" + key + "\": " + value;
    if (!isLast) result += ",";
    if (prettyPrint) result += "\n";
    return result;
}

std::string JSONSceneWriter::writeArray(const std::string& key, const std::vector<std::string>& values) {
    std::ostringstream oss;
    oss << indent() << "\"" << key << "\": [";
    
    for (size_t i = 0; i < values.size(); ++i) {
        oss << values[i];
        if (i < values.size() - 1) oss << ",";
    }
    
    oss << "]";
    return oss.str();
}

std::string JSONSceneWriter::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

// JSONSceneReader implementation
JSONSceneReader::JSONSceneReader() {}

JSONSceneReader::~JSONSceneReader() {}

bool JSONSceneReader::readScene(Scene* scene, const std::string& json) {
    if (!scene || json.empty()) return false;
    
    // Scene name is read-only (set in constructor), skip name setting
    
    return true;
}

ECS::Entity* JSONSceneReader::readEntity(const std::string& json, ECS::EntityManager* manager) {
    if (json.empty() || !manager) return nullptr;
    
    return nullptr;
}

std::string JSONSceneReader::getValue(const std::string& json, const std::string& key) {
    size_t pos = findKey(json, key);
    if (pos == std::string::npos) return "";
    
    size_t start = json.find('"', pos + key.length() + 3);
    size_t end = json.find('"', start + 1);
    
    if (start != std::string::npos && end != std::string::npos) {
        return unescapeString(json.substr(start + 1, end - start - 1));
    }
    
    return "";
}

std::vector<std::string> JSONSceneReader::getArray(const std::string& json, const std::string& key) {
    std::vector<std::string> result;
    // Parse array values
    return result;
}

std::string JSONSceneReader::unescapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"': result += '"'; i++; break;
                case '\\': result += '\\'; i++; break;
                case 'n': result += '\n'; i++; break;
                case 'r': result += '\r'; i++; break;
                case 't': result += '\t'; i++; break;
                default: result += str[i];
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

size_t JSONSceneReader::findKey(const std::string& json, const std::string& key) {
    std::string searchStr = "\"" + key + "\"";
    return json.find(searchStr);
}

// BinarySceneWriter implementation
BinarySceneWriter::BinarySceneWriter() {}

BinarySceneWriter::~BinarySceneWriter() {}

std::vector<uint8_t> BinarySceneWriter::writeScene(Scene* scene) {
    std::vector<uint8_t> buffer;
    
    if (scene) {
        writeString(buffer, scene->getName());
    }
    
    return buffer;
}

std::vector<uint8_t> BinarySceneWriter::writeEntity(ECS::Entity* entity) {
    std::vector<uint8_t> buffer;
    
    if (entity) {
        writeInt32(buffer, static_cast<int32_t>(entity->getID()));
    }
    
    return buffer;
}

void BinarySceneWriter::writeInt32(std::vector<uint8_t>& buffer, int32_t value) {
    buffer.push_back((value >> 24) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void BinarySceneWriter::writeFloat(std::vector<uint8_t>& buffer, float value) {
    uint32_t intValue;
    std::memcpy(&intValue, &value, sizeof(float));
    writeInt32(buffer, static_cast<int32_t>(intValue));
}

void BinarySceneWriter::writeString(std::vector<uint8_t>& buffer, const std::string& value) {
    writeInt32(buffer, static_cast<int32_t>(value.length()));
    writeBytes(buffer, reinterpret_cast<const uint8_t*>(value.data()), value.length());
}

void BinarySceneWriter::writeBool(std::vector<uint8_t>& buffer, bool value) {
    buffer.push_back(value ? 1 : 0);
}

void BinarySceneWriter::writeBytes(std::vector<uint8_t>& buffer, const uint8_t* data, size_t size) {
    buffer.insert(buffer.end(), data, data + size);
}

// BinarySceneReader implementation
BinarySceneReader::BinarySceneReader() {}

BinarySceneReader::~BinarySceneReader() {}

bool BinarySceneReader::readScene(Scene* scene, const std::vector<uint8_t>& data) {
    if (!scene || data.empty()) return false;
    
    size_t offset = 0;
    std::string name = readString(data, offset);
    // Scene name is read-only (set in constructor)
    
    return true;
}

ECS::Entity* BinarySceneReader::readEntity(const std::vector<uint8_t>& data, ECS::EntityManager* manager) {
    if (data.empty() || !manager) return nullptr;
    
    return nullptr;
}

int32_t BinarySceneReader::readInt32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) return 0;
    
    int32_t value = (static_cast<int32_t>(data[offset]) << 24) |
                    (static_cast<int32_t>(data[offset + 1]) << 16) |
                    (static_cast<int32_t>(data[offset + 2]) << 8) |
                    static_cast<int32_t>(data[offset + 3]);
    offset += 4;
    return value;
}

float BinarySceneReader::readFloat(const std::vector<uint8_t>& data, size_t& offset) {
    int32_t intValue = readInt32(data, offset);
    float value;
    std::memcpy(&value, &intValue, sizeof(float));
    return value;
}

std::string BinarySceneReader::readString(const std::vector<uint8_t>& data, size_t& offset) {
    int32_t length = readInt32(data, offset);
    if (length <= 0 || offset + length > data.size()) return "";
    
    std::string value(reinterpret_cast<const char*>(&data[offset]), length);
    offset += length;
    return value;
}

bool BinarySceneReader::readBool(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset >= data.size()) return false;
    return data[offset++] != 0;
}

void BinarySceneReader::readBytes(const std::vector<uint8_t>& data, size_t& offset, uint8_t* buffer, size_t size) {
    if (offset + size > data.size()) return;
    std::memcpy(buffer, &data[offset], size);
    offset += size;
}

} // namespace Scene
} // namespace JJM
