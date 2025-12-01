#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <typeindex>
#include <any>
#include <cstring>
#include <fstream>
#include <sstream>

namespace JJM {
namespace Serialization {

// Forward declarations
class SerializationContext;
class Serializer;

// Version information for data compatibility
struct VersionInfo {
    int major;
    int minor;
    int patch;
    
    VersionInfo(int maj = 1, int min = 0, int p = 0) 
        : major(maj), minor(min), patch(p) {}
    
    bool isCompatible(const VersionInfo& other) const {
        return major == other.major && minor >= other.minor;
    }
    
    std::string toString() const {
        return std::to_string(major) + "." + 
               std::to_string(minor) + "." + 
               std::to_string(patch);
    }
};

// Serializable interface
class ISerializable {
public:
    virtual ~ISerializable() = default;
    virtual void serialize(Serializer& serializer) const = 0;
    virtual void deserialize(Serializer& serializer) = 0;
    virtual std::string getTypeName() const = 0;
    virtual VersionInfo getVersion() const { return VersionInfo(1, 0, 0); }
};

// Type registration for serialization
struct TypeInfo {
    std::string name;
    std::type_index typeIndex;
    std::function<std::shared_ptr<ISerializable>()> factory;
    VersionInfo version;
    
    TypeInfo(const std::string& typeName, 
             const std::type_index& index,
             std::function<std::shared_ptr<ISerializable>()> factoryFunc,
             const VersionInfo& ver = VersionInfo())
        : name(typeName), typeIndex(index), factory(factoryFunc), version(ver) {}
};

// Serialization context manages type registry and circular references
class SerializationContext {
public:
    SerializationContext();
    ~SerializationContext();
    
    // Type registration
    template<typename T>
    void registerType(const std::string& name, const VersionInfo& version = VersionInfo()) {
        static_assert(std::is_base_of<ISerializable, T>::value, 
                     "Type must derive from ISerializable");
        
        auto factory = []() -> std::shared_ptr<ISerializable> {
            return std::make_shared<T>();
        };
        
        TypeInfo info(name, std::type_index(typeid(T)), factory, version);
        typeRegistry[name] = info;
        typeIndexToName[std::type_index(typeid(T))] = name;
    }
    
    // Get type info
    const TypeInfo* getTypeInfo(const std::string& name) const;
    const TypeInfo* getTypeInfo(const std::type_index& index) const;
    std::string getTypeName(const std::type_index& index) const;
    
    // Object tracking for circular references
    void trackObject(const void* ptr, int id);
    int getObjectId(const void* ptr) const;
    bool isTracked(const void* ptr) const;
    void clearTracking();
    
    // Version management
    void setVersion(const VersionInfo& version) { currentVersion = version; }
    const VersionInfo& getVersion() const { return currentVersion; }
    
private:
    std::unordered_map<std::string, TypeInfo> typeRegistry;
    std::unordered_map<std::type_index, std::string> typeIndexToName;
    std::unordered_map<const void*, int> objectToId;
    std::unordered_map<int, const void*> idToObject;
    int nextObjectId;
    VersionInfo currentVersion;
};

// Base serializer interface
class Serializer {
public:
    enum class Mode {
        Write,
        Read
    };
    
    Serializer(SerializationContext& ctx, Mode m) 
        : context(ctx), mode(m) {}
    virtual ~Serializer() = default;
    
    // Primitive types
    virtual void serialize(const std::string& name, bool& value) = 0;
    virtual void serialize(const std::string& name, int8_t& value) = 0;
    virtual void serialize(const std::string& name, uint8_t& value) = 0;
    virtual void serialize(const std::string& name, int16_t& value) = 0;
    virtual void serialize(const std::string& name, uint16_t& value) = 0;
    virtual void serialize(const std::string& name, int32_t& value) = 0;
    virtual void serialize(const std::string& name, uint32_t& value) = 0;
    virtual void serialize(const std::string& name, int64_t& value) = 0;
    virtual void serialize(const std::string& name, uint64_t& value) = 0;
    virtual void serialize(const std::string& name, float& value) = 0;
    virtual void serialize(const std::string& name, double& value) = 0;
    virtual void serialize(const std::string& name, std::string& value) = 0;
    
    // Container types
    template<typename T>
    void serialize(const std::string& name, std::vector<T>& vec) {
        beginArray(name, vec.size());
        for (size_t i = 0; i < vec.size(); i++) {
            serialize(name + "[" + std::to_string(i) + "]", vec[i]);
        }
        endArray();
    }
    
    // Object serialization
    virtual void serializeObject(const std::string& name, ISerializable* obj) = 0;
    
    // Array/object nesting
    virtual void beginArray(const std::string& name, size_t& size) = 0;
    virtual void endArray() = 0;
    virtual void beginObject(const std::string& name) = 0;
    virtual void endObject() = 0;
    
    // Access
    Mode getMode() const { return mode; }
    bool isReading() const { return mode == Mode::Read; }
    bool isWriting() const { return mode == Mode::Write; }
    SerializationContext& getContext() { return context; }
    const SerializationContext& getContext() const { return context; }
    
protected:
    SerializationContext& context;
    Mode mode;
};

// Binary serializer - compact binary format
class BinarySerializer : public Serializer {
public:
    BinarySerializer(SerializationContext& ctx, Mode m);
    ~BinarySerializer() override;
    
    // File operations
    bool openFile(const std::string& filename);
    void closeFile();
    
    // Buffer operations
    void setBuffer(std::vector<uint8_t>& buffer);
    const std::vector<uint8_t>& getBuffer() const { return *writeBuffer; }
    
    // Primitive types
    void serialize(const std::string& name, bool& value) override;
    void serialize(const std::string& name, int8_t& value) override;
    void serialize(const std::string& name, uint8_t& value) override;
    void serialize(const std::string& name, int16_t& value) override;
    void serialize(const std::string& name, uint16_t& value) override;
    void serialize(const std::string& name, int32_t& value) override;
    void serialize(const std::string& name, uint32_t& value) override;
    void serialize(const std::string& name, int64_t& value) override;
    void serialize(const std::string& name, uint64_t& value) override;
    void serialize(const std::string& name, float& value) override;
    void serialize(const std::string& name, double& value) override;
    void serialize(const std::string& name, std::string& value) override;
    
    void serializeObject(const std::string& name, ISerializable* obj) override;
    
    void beginArray(const std::string& name, size_t& size) override;
    void endArray() override;
    void beginObject(const std::string& name) override;
    void endObject() override;
    
private:
    template<typename T>
    void writeValue(const T& value);
    
    template<typename T>
    void readValue(T& value);
    
    void writeBytes(const void* data, size_t size);
    void readBytes(void* data, size_t size);
    
    std::fstream file;
    std::vector<uint8_t>* writeBuffer;
    size_t readPos;
    bool ownsBuffer;
};

// JSON serializer - human-readable text format
class JSONSerializer : public Serializer {
public:
    JSONSerializer(SerializationContext& ctx, Mode m);
    ~JSONSerializer() override;
    
    // File operations
    bool openFile(const std::string& filename);
    void closeFile();
    
    // String operations
    void setString(const std::string& str);
    std::string getString() const { return output.str(); }
    
    // Formatting
    void setPrettyPrint(bool pretty) { prettyPrint = pretty; }
    void setIndentation(int spaces) { indentSpaces = spaces; }
    
    // Primitive types
    void serialize(const std::string& name, bool& value) override;
    void serialize(const std::string& name, int8_t& value) override;
    void serialize(const std::string& name, uint8_t& value) override;
    void serialize(const std::string& name, int16_t& value) override;
    void serialize(const std::string& name, uint16_t& value) override;
    void serialize(const std::string& name, int32_t& value) override;
    void serialize(const std::string& name, uint32_t& value) override;
    void serialize(const std::string& name, int64_t& value) override;
    void serialize(const std::string& name, uint64_t& value) override;
    void serialize(const std::string& name, float& value) override;
    void serialize(const std::string& name, double& value) override;
    void serialize(const std::string& name, std::string& value) override;
    
    void serializeObject(const std::string& name, ISerializable* obj) override;
    
    void beginArray(const std::string& name, size_t& size) override;
    void endArray() override;
    void beginObject(const std::string& name) override;
    void endObject() override;
    
private:
    void writeIndent();
    void writeSeparator();
    std::string escapeString(const std::string& str);
    std::string unescapeString(const std::string& str);
    
    std::stringstream output;
    std::stringstream input;
    std::fstream file;
    
    int currentIndent;
    int indentSpaces;
    bool prettyPrint;
    bool needsSeparator;
    std::vector<bool> isFirstElement;
};

// Helper class for automatic serialization
class SerializableObject : public ISerializable {
public:
    ~SerializableObject() override = default;
    
    void serialize(Serializer& serializer) const override;
    void deserialize(Serializer& serializer) override;
    std::string getTypeName() const override { return typeName; }
    
protected:
    std::string typeName;
    
    // Derived classes should call these in their serialize/deserialize
    void serializeFields(Serializer& serializer) const;
    void deserializeFields(Serializer& serializer);
    
    // Field registration
    template<typename T>
    void registerField(const std::string& name, T* fieldPtr) {
        fields[name] = [fieldPtr](Serializer& s, const std::string& n) {
            s.serialize(n, *fieldPtr);
        };
    }
    
private:
    std::unordered_map<std::string, 
        std::function<void(Serializer&, const std::string&)>> fields;
};

// Save/Load helper functions
template<typename T>
bool saveToFile(const std::string& filename, const T& object, 
                SerializationContext& context) {
    static_assert(std::is_base_of<ISerializable, T>::value,
                 "Type must derive from ISerializable");
    
    BinarySerializer serializer(context, Serializer::Mode::Write);
    if (!serializer.openFile(filename)) {
        return false;
    }
    
    T* nonConstObj = const_cast<T*>(&object);
    serializer.serializeObject("root", nonConstObj);
    serializer.closeFile();
    return true;
}

template<typename T>
bool loadFromFile(const std::string& filename, T& object,
                  SerializationContext& context) {
    static_assert(std::is_base_of<ISerializable, T>::value,
                 "Type must derive from ISerializable");
    
    BinarySerializer serializer(context, Serializer::Mode::Read);
    if (!serializer.openFile(filename)) {
        return false;
    }
    
    serializer.serializeObject("root", &object);
    serializer.closeFile();
    return true;
}

template<typename T>
std::string toJSON(const T& object, SerializationContext& context, 
                   bool prettyPrint = true) {
    static_assert(std::is_base_of<ISerializable, T>::value,
                 "Type must derive from ISerializable");
    
    JSONSerializer serializer(context, Serializer::Mode::Write);
    serializer.setPrettyPrint(prettyPrint);
    
    T* nonConstObj = const_cast<T*>(&object);
    serializer.serializeObject("root", nonConstObj);
    return serializer.getString();
}

template<typename T>
bool fromJSON(const std::string& json, T& object, SerializationContext& context) {
    static_assert(std::is_base_of<ISerializable, T>::value,
                 "Type must derive from ISerializable");
    
    JSONSerializer serializer(context, Serializer::Mode::Read);
    serializer.setString(json);
    serializer.serializeObject("root", &object);
    return true;
}

} // namespace Serialization
} // namespace JJM

#endif // SERIALIZATION_H
