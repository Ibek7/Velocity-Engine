#ifndef REFLECTION_SYSTEM_H
#define REFLECTION_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <typeinfo>
#include <functional>

namespace JJM {
namespace Core {

// Type information
enum class TypeKind {
    PRIMITIVE,
    CLASS,
    STRUCT,
    ENUM,
    POINTER,
    ARRAY,
    UNKNOWN
};

// Field flags
enum FieldFlags {
    NONE = 0,
    SERIALIZABLE = 1 << 0,
    EDITOR_VISIBLE = 1 << 1,
    READONLY = 1 << 2,
    TRANSIENT = 1 << 3    // Don't save
};

// Type metadata
class TypeInfo {
public:
    TypeInfo(const std::string& name, TypeKind kind, size_t size)
        : m_name(name), m_kind(kind), m_size(size) {}
    
    const std::string& getName() const { return m_name; }
    TypeKind getKind() const { return m_kind; }
    size_t getSize() const { return m_size; }
    
private:
    std::string m_name;
    TypeKind m_kind;
    size_t m_size;
};

// Field metadata
class FieldInfo {
public:
    FieldInfo(const std::string& name, const TypeInfo* type, size_t offset, int flags)
        : m_name(name), m_type(type), m_offset(offset), m_flags(flags) {}
    
    const std::string& getName() const { return m_name; }
    const TypeInfo* getType() const { return m_type; }
    size_t getOffset() const { return m_offset; }
    int getFlags() const { return m_flags; }
    
    template<typename T>
    T& get(void* instance) const {
        return *reinterpret_cast<T*>(static_cast<char*>(instance) + m_offset);
    }
    
    template<typename T>
    void set(void* instance, const T& value) const {
        get<T>(instance) = value;
    }
    
private:
    std::string m_name;
    const TypeInfo* m_type;
    size_t m_offset;
    int m_flags;
};

// Class metadata
class ClassInfo : public TypeInfo {
public:
    ClassInfo(const std::string& name, size_t size)
        : TypeInfo(name, TypeKind::CLASS, size), m_baseClass(nullptr) {}
    
    void setBaseClass(const ClassInfo* base) { m_baseClass = base; }
    const ClassInfo* getBaseClass() const { return m_baseClass; }
    
    void addField(const FieldInfo& field) { m_fields.push_back(field); }
    const std::vector<FieldInfo>& getFields() const { return m_fields; }
    
    const FieldInfo* findField(const std::string& name) const {
        for (const auto& field : m_fields) {
            if (field.getName() == name) return &field;
        }
        if (m_baseClass) return m_baseClass->findField(name);
        return nullptr;
    }
    
    void* createInstance() const {
        if (m_constructor) return m_constructor();
        return nullptr;
    }
    
    void setConstructor(std::function<void*()> ctor) { m_constructor = ctor; }
    
private:
    const ClassInfo* m_baseClass;
    std::vector<FieldInfo> m_fields;
    std::function<void*()> m_constructor;
};

// Reflection registry
class ReflectionRegistry {
public:
    static ReflectionRegistry& getInstance();
    
    void registerType(const TypeInfo* type);
    void registerClass(const ClassInfo* classInfo);
    
    const TypeInfo* findType(const std::string& name) const;
    const ClassInfo* findClass(const std::string& name) const;
    
    const std::vector<const ClassInfo*>& getAllClasses() const { return m_classes; }
    
private:
    ReflectionRegistry() = default;
    std::unordered_map<std::string, const TypeInfo*> m_types;
    std::vector<const ClassInfo*> m_classes;
};

// Reflection macros
#define JJM_REFLECT_CLASS(ClassName) \
    public: \
    static const JJM::Core::ClassInfo* getClassInfoStatic(); \
    virtual const JJM::Core::ClassInfo* getClassInfo() const { return getClassInfoStatic(); }

#define JJM_IMPLEMENT_REFLECTION(ClassName) \
    const JJM::Core::ClassInfo* ClassName::getClassInfoStatic() { \
        static JJM::Core::ClassInfo* info = nullptr; \
        if (!info) { \
            info = new JJM::Core::ClassInfo(#ClassName, sizeof(ClassName)); \
            info->setConstructor([]() -> void* { return new ClassName(); }); \
            JJM::Core::ReflectionRegistry::getInstance().registerClass(info);

#define JJM_FIELD(FieldName, Flags) \
    info->addField(JJM::Core::FieldInfo( \
        #FieldName, \
        nullptr, \
        offsetof(ClassName, FieldName), \
        Flags \
    ));

#define JJM_END_REFLECTION() \
        } \
        return info; \
    }

} // namespace Core
} // namespace JJM

#endif
