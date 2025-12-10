#include "ecs/ComponentSerialization.h"
#include <sstream>
#include <cstring>

namespace JJM {
namespace ECS {

// SerializationRegistry implementation
SerializationRegistry& SerializationRegistry::getInstance() {
    static SerializationRegistry instance;
    return instance;
}

void SerializationRegistry::registerSerializer(const std::string& typeName,
                                              std::shared_ptr<ComponentSerializer> serializer) {
    serializers[typeName] = serializer;
}

ComponentSerializer* SerializationRegistry::getSerializer(const std::string& typeName) const {
    auto it = serializers.find(typeName);
    if (it != serializers.end()) {
        return it->second.get();
    }
    return nullptr;
}

// EntitySerializer implementation
EntitySerializer::EntitySerializer() 
    : registry(&SerializationRegistry::getInstance()) {}

EntitySerializer::~EntitySerializer() {}

std::string EntitySerializer::serializeEntity(const Entity& entity) const {
    std::stringstream ss;
    
    ss << "Entity:" << entity.getID() << "\n";
    ss << "ComponentCount:0\n";
    
    return ss.str();
}

Entity EntitySerializer::deserializeEntity(const std::string& data) const {
    std::stringstream ss(data);
    std::string line;
    
    EntityID entityId = 0;
    
    while (std::getline(ss, line)) {
        if (line.find("Entity:") == 0) {
            entityId = std::stoi(line.substr(7));
        }
    }
    
    return Entity(entityId, nullptr);
}

std::string EntitySerializer::serializeComponent(const Component* component) const {
    if (!component || !registry) return "";
    
    // Placeholder - would need actual component type info
    return "Component";
}

Component* EntitySerializer::deserializeComponent(const std::string& typeName,
                                                  const std::string& data) const {
    if (!registry) return nullptr;
    
    ComponentSerializer* serializer = registry->getSerializer(typeName);
    
    if (!serializer) return nullptr;
    
    return serializer->deserialize(data);
}

// BinarySerializer implementation
BinarySerializer::BinarySerializer() {}

BinarySerializer::~BinarySerializer() {}

std::vector<uint8_t> BinarySerializer::serialize(const Entity& entity) const {
    std::vector<uint8_t> buffer;
    
    writeInt(buffer, static_cast<int>(entity.getID()));
    writeInt(buffer, 0);
    
    return buffer;
}

Entity BinarySerializer::deserialize(const std::vector<uint8_t>& data) const {
    size_t offset = 0;
    
    int entityId = readInt(data, offset);
    
    return Entity(static_cast<EntityID>(entityId), nullptr);
}

void BinarySerializer::writeInt(std::vector<uint8_t>& buffer, int value) const {
    buffer.push_back((value >> 24) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void BinarySerializer::writeFloat(std::vector<uint8_t>& buffer, float value) const {
    uint32_t intValue;
    std::memcpy(&intValue, &value, sizeof(float));
    writeInt(buffer, static_cast<int>(intValue));
}

void BinarySerializer::writeString(std::vector<uint8_t>& buffer, const std::string& value) const {
    writeInt(buffer, static_cast<int>(value.length()));
    for (char c : value) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
}

int BinarySerializer::readInt(const std::vector<uint8_t>& buffer, size_t& offset) const {
    if (offset + 4 > buffer.size()) return 0;
    
    int value = (static_cast<int>(buffer[offset]) << 24) |
                (static_cast<int>(buffer[offset + 1]) << 16) |
                (static_cast<int>(buffer[offset + 2]) << 8) |
                static_cast<int>(buffer[offset + 3]);
    
    offset += 4;
    return value;
}

float BinarySerializer::readFloat(const std::vector<uint8_t>& buffer, size_t& offset) const {
    int intValue = readInt(buffer, offset);
    float floatValue;
    std::memcpy(&floatValue, &intValue, sizeof(float));
    return floatValue;
}

std::string BinarySerializer::readString(const std::vector<uint8_t>& buffer, size_t& offset) const {
    int length = readInt(buffer, offset);
    
    if (offset + length > buffer.size()) return "";
    
    std::string value;
    value.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        value.push_back(static_cast<char>(buffer[offset + i]));
    }
    
    offset += length;
    return value;
}

// JSONSerializer implementation
JSONSerializer::JSONSerializer() {}

JSONSerializer::~JSONSerializer() {}

std::string JSONSerializer::serializeToJSON(const Entity& entity) const {
    std::stringstream ss;
    
    ss << "{\n";
    ss << "  \"id\": " << entity.getID() << ",\n";
    ss << "  \"components\": []\n";
    ss << "}";
    
    return ss.str();
}

Entity JSONSerializer::deserializeFromJSON(const std::string& json) const {
    size_t idPos = json.find("\"id\":");
    if (idPos == std::string::npos) return Entity(0, nullptr);
    
    size_t colonPos = json.find(':', idPos);
    size_t commaPos = json.find(',', colonPos);
    
    std::string idStr = json.substr(colonPos + 1, commaPos - colonPos - 1);
    
    EntityID id = 0;
    try {
        id = std::stoi(idStr);
    } catch (...) {
        id = 0;
    }
    
    return Entity(id, nullptr);
}

std::string JSONSerializer::componentToJSON(const Component* component) const {
    if (!component) return "{}";
    
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"type\": \"Component\"\n";
    ss << "}";
    
    return ss.str();
}

Component* JSONSerializer::componentFromJSON(const std::string& json) const {
    return nullptr;
}

std::string JSONSerializer::escapeString(const std::string& str) const {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    
    return result;
}

std::string JSONSerializer::unescapeString(const std::string& str) const {
    std::string result;
    result.reserve(str.length());
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            switch (str[i + 1]) {
                case '"':  result += '"'; ++i; break;
                case '\\': result += '\\'; ++i; break;
                case 'n':  result += '\n'; ++i; break;
                case 'r':  result += '\r'; ++i; break;
                case 't':  result += '\t'; ++i; break;
                default:   result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    
    return result;
}

// SerializationContext implementation
SerializationContext::SerializationContext() {}

SerializationContext::~SerializationContext() {}

void SerializationContext::setProperty(const std::string& key, const std::string& value) {
    properties[key] = value;
}

std::string SerializationContext::getProperty(const std::string& key) const {
    auto it = properties.find(key);
    if (it != properties.end()) {
        return it->second;
    }
    return "";
}

bool SerializationContext::hasProperty(const std::string& key) const {
    return properties.find(key) != properties.end();
}

void SerializationContext::clear() {
    properties.clear();
}

// ComponentFactory implementation
ComponentFactory& ComponentFactory::getInstance() {
    static ComponentFactory instance;
    return instance;
}

void ComponentFactory::registerComponent(const std::string& typeName, CreateFunc createFunc) {
    creators[typeName] = createFunc;
}

Component* ComponentFactory::createComponent(const std::string& typeName) const {
    auto it = creators.find(typeName);
    if (it != creators.end()) {
        return it->second();
    }
    return nullptr;
}

} // namespace ECS
} // namespace JJM
