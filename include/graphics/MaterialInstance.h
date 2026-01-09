#ifndef MATERIAL_INSTANCE_H
#define MATERIAL_INSTANCE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <vector>

namespace JJM {
namespace Graphics {

// Forward declarations
class Material;
class Shader;

/**
 * @brief Material parameter value types
 */
using MaterialParameterValue = std::variant<
    int,
    float,
    double,
    bool,
    std::string,
    std::vector<float>,  // For vectors (vec2, vec3, vec4)
    std::vector<int>
>;

/**
 * @brief Material parameter descriptor
 */
struct MaterialParameter {
    std::string name;
    MaterialParameterValue defaultValue;
    bool isDynamic;  // Can be changed per-instance
    
    MaterialParameter()
        : isDynamic(true)
    {}
};

/**
 * @brief Material instance with per-instance parameter overrides
 * 
 * Allows multiple objects to share the same base material while
 * having different parameter values (e.g., different colors, textures).
 */
class MaterialInstance {
private:
    std::shared_ptr<Material> m_baseMaterial;
    std::unordered_map<std::string, MaterialParameterValue> m_parameterOverrides;
    std::unordered_map<std::string, unsigned int> m_textureOverrides;
    bool m_dirty;
    
    // Instance hash for batching
    size_t m_instanceHash;
    
public:
    explicit MaterialInstance(std::shared_ptr<Material> baseMaterial);
    ~MaterialInstance();
    
    /**
     * @brief Set a parameter override
     * @param name Parameter name
     * @param value Parameter value
     */
    template<typename T>
    void setParameter(const std::string& name, const T& value) {
        m_parameterOverrides[name] = value;
        m_dirty = true;
        updateHash();
    }
    
    /**
     * @brief Get parameter value (override or default)
     * @param name Parameter name
     * @return Parameter value
     */
    template<typename T>
    T getParameter(const std::string& name) const;
    
    /**
     * @brief Set texture override
     * @param name Texture parameter name
     * @param textureHandle Texture GPU handle
     */
    void setTexture(const std::string& name, unsigned int textureHandle);
    
    /**
     * @brief Get texture handle
     * @param name Texture parameter name
     * @return Texture GPU handle
     */
    unsigned int getTexture(const std::string& name) const;
    
    /**
     * @brief Check if parameter has override
     */
    bool hasOverride(const std::string& name) const;
    
    /**
     * @brief Remove parameter override
     */
    void removeOverride(const std::string& name);
    
    /**
     * @brief Clear all overrides
     */
    void clearOverrides();
    
    /**
     * @brief Apply material and its parameters to shader
     */
    void bind();
    
    /**
     * @brief Get base material
     */
    std::shared_ptr<Material> getBaseMaterial() const { return m_baseMaterial; }
    
    /**
     * @brief Get instance hash for batching/sorting
     */
    size_t getHash() const { return m_instanceHash; }
    
    /**
     * @brief Check if instance needs update
     */
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }
    
private:
    void updateHash();
};

/**
 * @brief Material with support for instancing
 */
class Material {
private:
    std::string m_name;
    std::shared_ptr<Shader> m_shader;
    std::unordered_map<std::string, MaterialParameter> m_parameters;
    std::unordered_map<std::string, unsigned int> m_textures;
    
    // Rendering state
    bool m_depthTest;
    bool m_depthWrite;
    bool m_alphaBlend;
    int m_blendSrc;
    int m_blendDst;
    int m_cullMode;
    
    // Instancing support
    bool m_supportsInstancing;
    std::vector<std::weak_ptr<MaterialInstance>> m_instances;
    
public:
    Material(const std::string& name);
    ~Material();
    
    /**
     * @brief Set shader for this material
     */
    void setShader(std::shared_ptr<Shader> shader);
    std::shared_ptr<Shader> getShader() const { return m_shader; }
    
    /**
     * @brief Add parameter definition
     */
    void addParameter(const std::string& name, const MaterialParameterValue& defaultValue, bool isDynamic = true);
    
    /**
     * @brief Set default parameter value
     */
    template<typename T>
    void setParameter(const std::string& name, const T& value) {
        if (m_parameters.count(name) > 0) {
            m_parameters[name].defaultValue = value;
        }
    }
    
    /**
     * @brief Get parameter default value
     */
    template<typename T>
    T getParameter(const std::string& name) const;
    
    /**
     * @brief Set texture
     */
    void setTexture(const std::string& name, unsigned int textureHandle);
    unsigned int getTexture(const std::string& name) const;
    
    /**
     * @brief Create material instance
     * @return New material instance with this material as base
     */
    std::shared_ptr<MaterialInstance> createInstance();
    
    /**
     * @brief Set rendering state
     */
    void setDepthTest(bool enable) { m_depthTest = enable; }
    void setDepthWrite(bool enable) { m_depthWrite = enable; }
    void setAlphaBlend(bool enable) { m_alphaBlend = enable; }
    void setBlendMode(int src, int dst) { m_blendSrc = src; m_blendDst = dst; }
    void setCullMode(int mode) { m_cullMode = mode; }
    
    bool getDepthTest() const { return m_depthTest; }
    bool getDepthWrite() const { return m_depthWrite; }
    bool getAlphaBlend() const { return m_alphaBlend; }
    
    /**
     * @brief Enable/disable instancing support
     */
    void setSupportsInstancing(bool supports) { m_supportsInstancing = supports; }
    bool supportsInstancing() const { return m_supportsInstancing; }
    
    /**
     * @brief Apply material state
     */
    void bind();
    
    /**
     * @brief Get material name
     */
    const std::string& getName() const { return m_name; }
    
    /**
     * @brief Get all parameters
     */
    const std::unordered_map<std::string, MaterialParameter>& getParameters() const {
        return m_parameters;
    }
    
private:
    void applyRenderState();
    void cleanupInstances();
};

/**
 * @brief Material system for managing materials and instances
 */
class MaterialSystem {
private:
    std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
    std::unordered_map<size_t, std::vector<MaterialInstance*>> m_instanceBatches;
    
public:
    MaterialSystem();
    ~MaterialSystem();
    
    /**
     * @brief Create a new material
     */
    std::shared_ptr<Material> createMaterial(const std::string& name);
    
    /**
     * @brief Get material by name
     */
    std::shared_ptr<Material> getMaterial(const std::string& name);
    
    /**
     * @brief Remove material
     */
    void removeMaterial(const std::string& name);
    
    /**
     * @brief Load material from file
     */
    std::shared_ptr<Material> loadMaterial(const std::string& filepath);
    
    /**
     * @brief Sort instances for batching
     * @param instances List of material instances to sort
     */
    void sortForBatching(std::vector<MaterialInstance*>& instances);
    
    /**
     * @brief Build instance batches for efficient rendering
     */
    void buildBatches(const std::vector<MaterialInstance*>& instances);
    
    /**
     * @brief Clear all materials
     */
    void clear();
};

} // namespace Graphics
} // namespace JJM

#endif // MATERIAL_INSTANCE_H
