/**
 * @file SerializationHelpers.h
 * @brief Utility functions for JSON and binary serialization
 * @version 1.0.0
 * @date 2026-01-16
 */

#ifndef SERIALIZATION_HELPERS_H
#define SERIALIZATION_HELPERS_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <cstring>
#include <fstream>

namespace JJM {
namespace Serialization {

// =============================================================================
// Binary Serialization Helpers
// =============================================================================

/**
 * @brief Binary writer for efficient serialization
 */
class BinaryWriter {
private:
    std::vector<uint8_t> m_buffer;
    size_t m_position;
    
public:
    BinaryWriter() : m_position(0) {
        m_buffer.reserve(1024);
    }
    
    // Primitive types
    void write(bool value) {
        writePrimitive(static_cast<uint8_t>(value ? 1 : 0));
    }
    
    void write(int8_t value) { writePrimitive(value); }
    void write(uint8_t value) { writePrimitive(value); }
    void write(int16_t value) { writePrimitive(value); }
    void write(uint16_t value) { writePrimitive(value); }
    void write(int32_t value) { writePrimitive(value); }
    void write(uint32_t value) { writePrimitive(value); }
    void write(int64_t value) { writePrimitive(value); }
    void write(uint64_t value) { writePrimitive(value); }
    void write(float value) { writePrimitive(value); }
    void write(double value) { writePrimitive(value); }
    
    // String
    void write(const std::string& value) {
        write(static_cast<uint32_t>(value.length()));
        writeBytes(value.data(), value.length());
    }
    
    // Vectors
    void write(const Math::Vector2D& vec) {
        write(vec.x);
        write(vec.y);
    }
    
    // Color
    void write(const Graphics::Color& color) {
        write(color.r);
        write(color.g);
        write(color.b);
        write(color.a);
    }
    
    // Array/Vector
    template<typename T>
    void write(const std::vector<T>& vec) {
        write(static_cast<uint32_t>(vec.size()));
        for (const auto& item : vec) {
            write(item);
        }
    }
    
    // Map
    template<typename K, typename V>
    void write(const std::map<K, V>& map) {
        write(static_cast<uint32_t>(map.size()));
        for (const auto& [key, value] : map) {
            write(key);
            write(value);
        }
    }
    
    // Raw bytes
    void writeBytes(const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        m_buffer.insert(m_buffer.end(), bytes, bytes + size);
        m_position += size;
    }
    
    // Get serialized data
    const std::vector<uint8_t>& getBuffer() const { return m_buffer; }
    size_t getSize() const { return m_buffer.size(); }
    
    // Save to file
    bool saveToFile(const std::string& filePath) const {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) return false;
        file.write(reinterpret_cast<const char*>(m_buffer.data()), m_buffer.size());
        return file.good();
    }
    
    void clear() {
        m_buffer.clear();
        m_position = 0;
    }
    
private:
    template<typename T>
    void writePrimitive(const T& value) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
        m_buffer.insert(m_buffer.end(), bytes, bytes + sizeof(T));
        m_position += sizeof(T);
    }
};

/**
 * @brief Binary reader for efficient deserialization
 */
class BinaryReader {
private:
    const uint8_t* m_data;
    size_t m_size;
    size_t m_position;
    bool m_error;
    
public:
    BinaryReader(const void* data, size_t size)
        : m_data(static_cast<const uint8_t*>(data))
        , m_size(size)
        , m_position(0)
        , m_error(false)
    {}
    
    BinaryReader(const std::vector<uint8_t>& buffer)
        : m_data(buffer.data())
        , m_size(buffer.size())
        , m_position(0)
        , m_error(false)
    {}
    
    // Primitive types
    bool readBool() {
        return static_cast<bool>(readPrimitive<uint8_t>());
    }
    
    int8_t readInt8() { return readPrimitive<int8_t>(); }
    uint8_t readUInt8() { return readPrimitive<uint8_t>(); }
    int16_t readInt16() { return readPrimitive<int16_t>(); }
    uint16_t readUInt16() { return readPrimitive<uint16_t>(); }
    int32_t readInt32() { return readPrimitive<int32_t>(); }
    uint32_t readUInt32() { return readPrimitive<uint32_t>(); }
    int64_t readInt64() { return readPrimitive<int64_t>(); }
    uint64_t readUInt64() { return readPrimitive<uint64_t>(); }
    float readFloat() { return readPrimitive<float>(); }
    double readDouble() { return readPrimitive<double>(); }
    
    // String
    std::string readString() {
        uint32_t length = readUInt32();
        if (m_error || m_position + length > m_size) {
            m_error = true;
            return "";
        }
        std::string result(reinterpret_cast<const char*>(m_data + m_position), length);
        m_position += length;
        return result;
    }
    
    // Vectors
    Math::Vector2D readVector2D() {
        Math::Vector2D vec;
        vec.x = readFloat();
        vec.y = readFloat();
        return vec;
    }
    
    // Color
    Graphics::Color readColor() {
        Graphics::Color color;
        color.r = readUInt8();
        color.g = readUInt8();
        color.b = readUInt8();
        color.a = readUInt8();
        return color;
    }
    
    // Array/Vector
    template<typename T>
    std::vector<T> readVector() {
        uint32_t size = readUInt32();
        std::vector<T> result;
        result.reserve(size);
        for (uint32_t i = 0; i < size && !m_error; i++) {
            result.push_back(read<T>());
        }
        return result;
    }
    
    // Map
    template<typename K, typename V>
    std::map<K, V> readMap() {
        uint32_t size = readUInt32();
        std::map<K, V> result;
        for (uint32_t i = 0; i < size && !m_error; i++) {
            K key = read<K>();
            V value = read<V>();
            result[key] = value;
        }
        return result;
    }
    
    // Raw bytes
    void readBytes(void* dest, size_t size) {
        if (m_position + size > m_size) {
            m_error = true;
            return;
        }
        std::memcpy(dest, m_data + m_position, size);
        m_position += size;
    }
    
    // Load from file
    static bool loadFromFile(const std::string& filePath, std::vector<uint8_t>& buffer) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        buffer.resize(size);
        file.read(reinterpret_cast<char*>(buffer.data()), size);
        return file.good();
    }
    
    // State
    bool hasError() const { return m_error; }
    size_t getPosition() const { return m_position; }
    size_t getSize() const { return m_size; }
    size_t remaining() const { return m_size - m_position; }
    bool atEnd() const { return m_position >= m_size; }
    
private:
    template<typename T>
    T readPrimitive() {
        if (m_position + sizeof(T) > m_size) {
            m_error = true;
            return T{};
        }
        T value;
        std::memcpy(&value, m_data + m_position, sizeof(T));
        m_position += sizeof(T);
        return value;
    }
    
    template<typename T>
    T read();
};

// Template specializations for read()
template<> inline bool BinaryReader::read<bool>() { return readBool(); }
template<> inline int8_t BinaryReader::read<int8_t>() { return readInt8(); }
template<> inline uint8_t BinaryReader::read<uint8_t>() { return readUInt8(); }
template<> inline int16_t BinaryReader::read<int16_t>() { return readInt16(); }
template<> inline uint16_t BinaryReader::read<uint16_t>() { return readUInt16(); }
template<> inline int32_t BinaryReader::read<int32_t>() { return readInt32(); }
template<> inline uint32_t BinaryReader::read<uint32_t>() { return readUInt32(); }
template<> inline int64_t BinaryReader::read<int64_t>() { return readInt64(); }
template<> inline uint64_t BinaryReader::read<uint64_t>() { return readUInt64(); }
template<> inline float BinaryReader::read<float>() { return readFloat(); }
template<> inline double BinaryReader::read<double>() { return readDouble(); }
template<> inline std::string BinaryReader::read<std::string>() { return readString(); }
template<> inline Math::Vector2D BinaryReader::read<Math::Vector2D>() { return readVector2D(); }
template<> inline Graphics::Color BinaryReader::read<Graphics::Color>() { return readColor(); }

// =============================================================================
// JSON Helpers (Simple key-value format)
// =============================================================================

/**
 * @brief Simple JSON-like writer (not full JSON spec)
 */
class JSONWriter {
private:
    std::string m_output;
    std::vector<bool> m_firstInScope;
    int m_indent;
    
public:
    JSONWriter() : m_indent(0) {
        m_output = "{\n";
        m_indent++;
        m_firstInScope.push_back(true);
    }
    
    void writeInt(const std::string& key, int value) {
        writeComma();
        addIndent();
        m_output += "\"" + key + "\": " + std::to_string(value);
    }
    
    void writeFloat(const std::string& key, float value) {
        writeComma();
        addIndent();
        m_output += "\"" + key + "\": " + std::to_string(value);
    }
    
    void writeString(const std::string& key, const std::string& value) {
        writeComma();
        addIndent();
        m_output += "\"" + key + "\": \"" + value + "\"";
    }
    
    void writeBool(const std::string& key, bool value) {
        writeComma();
        addIndent();
        m_output += "\"" + key + "\": " + (value ? "true" : "false");
    }
    
    void writeVector2D(const std::string& key, const Math::Vector2D& vec) {
        writeComma();
        addIndent();
        m_output += "\"" + key + "\": {\"x\": " + std::to_string(vec.x) + 
                    ", \"y\": " + std::to_string(vec.y) + "}";
    }
    
    void startObject(const std::string& key) {
        writeComma();
        addIndent();
        m_output += "\"" + key + "\": {\n";
        m_indent++;
        m_firstInScope.push_back(true);
    }
    
    void endObject() {
        m_output += "\n";
        m_indent--;
        m_firstInScope.pop_back();
        addIndent();
        m_output += "}";
    }
    
    std::string toString() {
        std::string result = m_output;
        result += "\n}";
        return result;
    }
    
private:
    void addIndent() {
        for (int i = 0; i < m_indent; i++) {
            m_output += "  ";
        }
    }
    
    void writeComma() {
        if (!m_firstInScope.back()) {
            m_output += ",\n";
        } else {
            m_firstInScope.back() = false;
        }
    }
};

} // namespace Serialization
} // namespace JJM

#endif // SERIALIZATION_HELPERS_H
