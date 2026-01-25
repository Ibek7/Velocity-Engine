#include "core/ReflectionSystem.h"

namespace JJM {
namespace Core {

ReflectionRegistry& ReflectionRegistry::getInstance() {
    static ReflectionRegistry instance;
    return instance;
}

void ReflectionRegistry::registerType(const TypeInfo* type) {
    m_types[type->getName()] = type;
}

void ReflectionRegistry::registerClass(const ClassInfo* classInfo) {
    m_types[classInfo->getName()] = classInfo;
    m_classes.push_back(classInfo);
}

const TypeInfo* ReflectionRegistry::findType(const std::string& name) const {
    auto it = m_types.find(name);
    return it != m_types.end() ? it->second : nullptr;
}

const ClassInfo* ReflectionRegistry::findClass(const std::string& name) const {
    for (const auto* classInfo : m_classes) {
        if (classInfo->getName() == name) return classInfo;
    }
    return nullptr;
}

} // namespace Core
} // namespace JJM
