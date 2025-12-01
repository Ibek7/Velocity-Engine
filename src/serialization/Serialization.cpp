#include "../../include/serialization/Serialization.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace JJM {
namespace Serialization {

// =============================================================================
// SerializationContext Implementation
// =============================================================================

SerializationContext::SerializationContext() : nextObjectId(1) {}

SerializationContext::~SerializationContext() {}

const TypeInfo* SerializationContext::getTypeInfo(const std::string& name) const {
    auto it = typeRegistry.find(name);
    return (it != typeRegistry.end()) ? &it->second : nullptr;
}

const TypeInfo* SerializationContext::getTypeInfo(const std::type_index& index) const {
    auto it = typeIndexToName.find(index);
    if (it != typeIndexToName.end()) {
        return getTypeInfo(it->second);
    }
    return nullptr;
}

std::string SerializationContext::getTypeName(const std::type_index& index) const {
    auto it = typeIndexToName.find(index);
    return (it != typeIndexToName.end()) ? it->second : "";
}

void SerializationContext::trackObject(const void* ptr, int id) {
    objectToId[ptr] = id;
    idToObject[id] = ptr;
}

int SerializationContext::getObjectId(const void* ptr) const {
    auto it = objectToId.find(ptr);
    return (it != objectToId.end()) ? it->second : -1;
}

bool SerializationContext::isTracked(const void* ptr) const {
    return objectToId.find(ptr) != objectToId.end();
}

void SerializationContext::clearTracking() {
    objectToId.clear();
    idToObject.clear();
    nextObjectId = 1;
}

// =============================================================================
// BinarySerializer Implementation
// =============================================================================

BinarySerializer::BinarySerializer(SerializationContext& ctx, Mode m)
    : Serializer(ctx, m), writeBuffer(nullptr), readPos(0), ownsBuffer(false) {
    if (mode == Mode::Write) {
        writeBuffer = new std::vector<uint8_t>();
        ownsBuffer = true;
    }
}

BinarySerializer::~BinarySerializer() {
    closeFile();
    if (ownsBuffer && writeBuffer) {
        delete writeBuffer;
    }
}

bool BinarySerializer::openFile(const std::string& filename) {
    if (mode == Mode::Write) {
        file.open(filename, std::ios::out | std::ios::binary);
    } else {
        file.open(filename, std::ios::in | std::ios::binary);
        if (file.is_open()) {
            // Read entire file into buffer
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            
            if (ownsBuffer && writeBuffer) {
                delete writeBuffer;
            }
            writeBuffer = new std::vector<uint8_t>(size);
            ownsBuffer = true;
            
            file.read(reinterpret_cast<char*>(writeBuffer->data()), size);
            readPos = 0;
        }
    }
    return file.is_open();
}

void BinarySerializer::closeFile() {
    if (file.is_open()) {
        if (mode == Mode::Write && writeBuffer) {
            file.write(reinterpret_cast<const char*>(writeBuffer->data()), 
                      writeBuffer->size());
        }
        file.close();
    }
}

void BinarySerializer::setBuffer(std::vector<uint8_t>& buffer) {
    if (ownsBuffer && writeBuffer) {
        delete writeBuffer;
    }
    writeBuffer = &buffer;
    ownsBuffer = false;
    readPos = 0;
}

template<typename T>
void BinarySerializer::writeValue(const T& value) {
    if (!writeBuffer) return;
    
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    writeBuffer->insert(writeBuffer->end(), bytes, bytes + sizeof(T));
}

template<typename T>
void BinarySerializer::readValue(T& value) {
    if (!writeBuffer || readPos + sizeof(T) > writeBuffer->size()) {
        return;
    }
    
    std::memcpy(&value, writeBuffer->data() + readPos, sizeof(T));
    readPos += sizeof(T);
}

void BinarySerializer::writeBytes(const void* data, size_t size) {
    if (!writeBuffer) return;
    
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(data);
    writeBuffer->insert(writeBuffer->end(), bytes, bytes + size);
}

void BinarySerializer::readBytes(void* data, size_t size) {
    if (!writeBuffer || readPos + size > writeBuffer->size()) {
        return;
    }
    
    std::memcpy(data, writeBuffer->data() + readPos, size);
    readPos += size;
}

void BinarySerializer::serialize(const std::string&, bool& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, int8_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, uint8_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, int16_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, uint16_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, int32_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, uint32_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, int64_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, uint64_t& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, float& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, double& value) {
    if (isWriting()) {
        writeValue(value);
    } else {
        readValue(value);
    }
}

void BinarySerializer::serialize(const std::string&, std::string& value) {
    if (isWriting()) {
        uint32_t length = static_cast<uint32_t>(value.length());
        writeValue(length);
        writeBytes(value.data(), length);
    } else {
        uint32_t length = 0;
        readValue(length);
        value.resize(length);
        readBytes(&value[0], length);
    }
}

void BinarySerializer::serializeObject(const std::string&, ISerializable* obj) {
    if (!obj) return;
    
    if (isWriting()) {
        // Write type name
        std::string typeName = obj->getTypeName();
        serialize("__type", typeName);
        
        // Write version
        VersionInfo version = obj->getVersion();
        serialize("__version_major", version.major);
        serialize("__version_minor", version.minor);
        serialize("__version_patch", version.patch);
        
        // Serialize object
        obj->serialize(*this);
    } else {
        // Read type name and version
        std::string typeName;
        serialize("__type", typeName);
        
        VersionInfo version;
        serialize("__version_major", version.major);
        serialize("__version_minor", version.minor);
        serialize("__version_patch", version.patch);
        
        // Deserialize object
        obj->deserialize(*this);
    }
}

void BinarySerializer::beginArray(const std::string&, size_t& size) {
    if (isWriting()) {
        uint32_t sz = static_cast<uint32_t>(size);
        writeValue(sz);
    } else {
        uint32_t sz = 0;
        readValue(sz);
        size = sz;
    }
}

void BinarySerializer::endArray() {
    // No-op for binary format
}

void BinarySerializer::beginObject(const std::string&) {
    // No-op for binary format
}

void BinarySerializer::endObject() {
    // No-op for binary format
}

// =============================================================================
// JSONSerializer Implementation
// =============================================================================

JSONSerializer::JSONSerializer(SerializationContext& ctx, Mode m)
    : Serializer(ctx, m), currentIndent(0), indentSpaces(2), 
      prettyPrint(true), needsSeparator(false) {}

JSONSerializer::~JSONSerializer() {
    closeFile();
}

bool JSONSerializer::openFile(const std::string& filename) {
    if (mode == Mode::Write) {
        file.open(filename, std::ios::out);
    } else {
        file.open(filename, std::ios::in);
        if (file.is_open()) {
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            input.str(content);
        }
    }
    return file.is_open();
}

void JSONSerializer::closeFile() {
    if (file.is_open()) {
        if (mode == Mode::Write) {
            file << output.str();
        }
        file.close();
    }
}

void JSONSerializer::setString(const std::string& str) {
    input.str(str);
}

void JSONSerializer::writeIndent() {
    if (prettyPrint) {
        output << std::string(currentIndent * indentSpaces, ' ');
    }
}

void JSONSerializer::writeSeparator() {
    if (needsSeparator) {
        output << ",";
        if (prettyPrint) output << "\n";
    }
    needsSeparator = true;
}

std::string JSONSerializer::escapeString(const std::string& str) {
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

std::string JSONSerializer::unescapeString(const std::string& str) {
    std::string result;
    bool escape = false;
    for (char c : str) {
        if (escape) {
            switch (c) {
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                default: result += c;
            }
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else {
            result += c;
        }
    }
    return result;
}

void JSONSerializer::serialize(const std::string& name, bool& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << (value ? "true" : "false");
    }
    // Reading would parse JSON - simplified for this implementation
}

void JSONSerializer::serialize(const std::string& name, int8_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << static_cast<int>(value);
    }
}

void JSONSerializer::serialize(const std::string& name, uint8_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << static_cast<unsigned>(value);
    }
}

void JSONSerializer::serialize(const std::string& name, int16_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, uint16_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, int32_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, uint32_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, int64_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, uint64_t& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, float& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, double& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": " << value;
    }
}

void JSONSerializer::serialize(const std::string& name, std::string& value) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": \"" << escapeString(value) << "\"";
    }
}

void JSONSerializer::serializeObject(const std::string& name, ISerializable* obj) {
    if (!obj) return;
    
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": {";
        if (prettyPrint) output << "\n";
        
        currentIndent++;
        needsSeparator = false;
        
        // Write type info
        std::string typeName = obj->getTypeName();
        serialize("__type", typeName);
        
        VersionInfo version = obj->getVersion();
        std::string versionStr = version.toString();
        serialize("__version", versionStr);
        
        // Serialize object - cast to non-const for calling serialize
        const_cast<ISerializable*>(obj)->serialize(*this);
        
        currentIndent--;
        if (prettyPrint) output << "\n";
        writeIndent();
        output << "}";
        needsSeparator = true;
    } else {
        // Reading would parse JSON - simplified
        obj->deserialize(*this);
    }
}

void JSONSerializer::beginArray(const std::string& name, size_t& size) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": [";
        if (prettyPrint) output << "\n";
        currentIndent++;
        needsSeparator = false;
        isFirstElement.push_back(true);
    }
}

void JSONSerializer::endArray() {
    if (isWriting()) {
        currentIndent--;
        if (prettyPrint) output << "\n";
        writeIndent();
        output << "]";
        needsSeparator = true;
        isFirstElement.pop_back();
    }
}

void JSONSerializer::beginObject(const std::string& name) {
    if (isWriting()) {
        writeSeparator();
        writeIndent();
        output << "\"" << name << "\": {";
        if (prettyPrint) output << "\n";
        currentIndent++;
        needsSeparator = false;
    }
}

void JSONSerializer::endObject() {
    if (isWriting()) {
        currentIndent--;
        if (prettyPrint) output << "\n";
        writeIndent();
        output << "}";
        needsSeparator = true;
    }
}

// =============================================================================
// SerializableObject Implementation
// =============================================================================

void SerializableObject::serialize(Serializer& serializer) const {
    serializeFields(serializer);
}

void SerializableObject::deserialize(Serializer& serializer) {
    deserializeFields(serializer);
}

void SerializableObject::serializeFields(Serializer& serializer) const {
    for (const auto& field : fields) {
        const_cast<std::function<void(Serializer&, const std::string&)>&>(field.second)
            (serializer, field.first);
    }
}

void SerializableObject::deserializeFields(Serializer& serializer) {
    for (auto& field : fields) {
        field.second(serializer, field.first);
    }
}

} // namespace Serialization
} // namespace JJM
