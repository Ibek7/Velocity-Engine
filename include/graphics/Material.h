/**
 * @file Material.h
 * @brief Advanced material system with property management
 * @version 1.0.0
 * @date 2026-01-12
 */

#ifndef MATERIAL_H
#define MATERIAL_H

#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <vector>

namespace JJM {
namespace Graphics {

class Shader;
class Texture;

/**
 * @brief Material property value types
 */
using MaterialPropertyValue = std::variant<
    int,
    float,
    bool,
    std::vector<float>,  // For vectors and matrices
    std::shared_ptr<Texture>
>;

/**
 * @brief Material property descriptor
 */
struct MaterialProperty {
    std::string name;
    MaterialPropertyValue value;
    std::string uniformName;  // Shader uniform binding
    bool isDirty{true};
};

/**
 * @brief Material rendering configuration
 */
class Material {
public:
    Material(const std::string& name = "");
    ~Material();
    
    // Shader management
    void setShader(std::shared_ptr<Shader> shader);
    std::shared_ptr<Shader> getShader() const { return m_shader; }
    
    // Property setters
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setBool(const std::string& name, bool value);
    void setVector2(const std::string& name, float x, float y);
    void setVector3(const std::string& name, float x, float y, float z);
    void setVector4(const std::string& name, float x, float y, float z, float w);
    void setColor(const std::string& name, float r, float g, float b, float a = 1.0f);
    void setTexture(const std::string& name, std::shared_ptr<Texture> texture);
    void setMatrix(const std::string& name, const float* matrix);
    
    // Property getters
    int getInt(const std::string& name, int defaultValue = 0) const;
    float getFloat(const std::string& name, float defaultValue = 0.0f) const;
    bool getBool(const std::string& name, bool defaultValue = false) const;
    std::shared_ptr<Texture> getTexture(const std::string& name) const;
    
    // Property management
    bool hasProperty(const std::string& name) const;
    void removeProperty(const std::string& name);
    void clearProperties();
    std::vector<std::string> getPropertyNames() const;
    
    // Material application
    void bind();
    void unbind();
    void applyProperties();
    
    // Serialization
    std::string serialize() const;
    bool deserialize(const std::string& data);
    
    // Cloning
    std::shared_ptr<Material> clone() const;
    
    // Metadata
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
private:
    std::string m_name;
    std::shared_ptr<Shader> m_shader;
    std::unordered_map<std::string, MaterialProperty> m_properties;
    
    void markDirty(const std::string& name);
};

/**
 * @brief Material library for managing material instances
 */
class MaterialLibrary {
public:
    static MaterialLibrary& getInstance();
    
    // Material management
    std::shared_ptr<Material> createMaterial(const std::string& name);
    std::shared_ptr<Material> getMaterial(const std::string& name);
    bool hasMaterial(const std::string& name) const;
    void removeMaterial(const std::string& name);
    void clear();
    
    // Batch operations
    std::vector<std::string> getMaterialNames() const;
    size_t getMaterialCount() const { return m_materials.size(); }
    
    // Loading
    bool loadMaterial(const std::string& name, const std::string& filePath);
    bool saveMaterial(const std::string& name, const std::string& filePath);
    
private:
    MaterialLibrary() = default;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
};

} // namespace Graphics
} // namespace JJM

#endif // MATERIAL_H
