#ifndef JJM_MATERIAL_SYSTEM_H
#define JJM_MATERIAL_SYSTEM_H

#include "graphics/Color.h"
#include "graphics/Texture.h"
#include "graphics/ShaderSystem.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace JJM {
namespace Graphics {

/**
 * @brief Material property types
 */
enum class PropertyType {
    Float,
    Int,
    Bool,
    Color,
    Vector2,
    Vector3,
    Vector4,
    Texture,
    Matrix3x3,
    Matrix4x4
};

/**
 * @brief Material property value
 */
class MaterialProperty {
public:
    MaterialProperty();
    MaterialProperty(PropertyType type);
    ~MaterialProperty();

    PropertyType getType() const;
    
    void setFloat(float value);
    void setInt(int value);
    void setBool(bool value);
    void setColor(const Color& value);
    void setVector2(float x, float y);
    void setVector3(float x, float y, float z);
    void setVector4(float x, float y, float z, float w);
    void setTexture(std::shared_ptr<Texture> texture);
    
    float getFloat() const;
    int getInt() const;
    bool getBool() const;
    Color getColor() const;
    void getVector2(float& x, float& y) const;
    void getVector3(float& x, float& y, float& z) const;
    void getVector4(float& x, float& y, float& z, float& w) const;
    std::shared_ptr<Texture> getTexture() const;

private:
    PropertyType type;
    union {
        float floatValue;
        int intValue;
        bool boolValue;
        float vector4[4];
    } data;
    Color colorValue;
    std::shared_ptr<Texture> textureValue;
};

/**
 * @brief Blending modes for materials
 */
enum class BlendMode {
    Opaque,
    AlphaBlend,
    Additive,
    Multiplicative,
    Premultiplied,
    Custom
};

/**
 * @brief Culling modes
 */
enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

/**
 * @brief Depth test functions
 */
enum class DepthFunc {
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

/**
 * @brief Material rendering states
 */
struct RenderState {
    BlendMode blendMode;
    CullMode cullMode;
    DepthFunc depthFunc;
    bool depthWrite;
    bool depthTest;
    bool alphaTest;
    float alphaThreshold;
    bool wireframe;
    int renderQueue;
    
    RenderState();
};

/**
 * @brief Material class
 */
class Material {
public:
    Material(const std::string& name = "Material");
    Material(std::shared_ptr<Shader> shader);
    ~Material();

    void setName(const std::string& name);
    std::string getName() const;
    
    void setShader(std::shared_ptr<Shader> shader);
    std::shared_ptr<Shader> getShader() const;
    
    // Property management
    void setProperty(const std::string& name, const MaterialProperty& property);
    void setFloat(const std::string& name, float value);
    void setInt(const std::string& name, int value);
    void setBool(const std::string& name, bool value);
    void setColor(const std::string& name, const Color& value);
    void setVector2(const std::string& name, float x, float y);
    void setVector3(const std::string& name, float x, float y, float z);
    void setVector4(const std::string& name, float x, float y, float z, float w);
    void setTexture(const std::string& name, std::shared_ptr<Texture> texture);
    
    MaterialProperty* getProperty(const std::string& name);
    bool hasProperty(const std::string& name) const;
    
    // Render state
    void setRenderState(const RenderState& state);
    RenderState& getRenderState();
    const RenderState& getRenderState() const;
    
    void setBlendMode(BlendMode mode);
    BlendMode getBlendMode() const;
    
    void setRenderQueue(int queue);
    int getRenderQueue() const;
    
    // Texture slots
    void setMainTexture(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> getMainTexture() const;
    
    void setNormalMap(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> getNormalMap() const;
    
    void setSpecularMap(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> getSpecularMap() const;
    
    void setEmissionMap(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> getEmissionMap() const;
    
    // Material application
    void apply();
    void bind();
    void unbind();
    
    // Keywords (shader variants)
    void enableKeyword(const std::string& keyword);
    void disableKeyword(const std::string& keyword);
    bool isKeywordEnabled(const std::string& keyword) const;
    
    // Instancing
    Material* clone() const;
    void copyPropertiesFrom(const Material& other);

private:
    std::string name;
    std::shared_ptr<Shader> shader;
    std::unordered_map<std::string, MaterialProperty> properties;
    RenderState renderState;
    std::vector<std::string> keywords;
    
    std::shared_ptr<Texture> mainTexture;
    std::shared_ptr<Texture> normalMap;
    std::shared_ptr<Texture> specularMap;
    std::shared_ptr<Texture> emissionMap;
};

/**
 * @brief Standard material with PBR properties
 */
class StandardMaterial : public Material {
public:
    StandardMaterial();
    ~StandardMaterial();

    void setAlbedo(const Color& color);
    Color getAlbedo() const;
    
    void setMetallic(float value);
    float getMetallic() const;
    
    void setRoughness(float value);
    float getRoughness() const;
    
    void setAO(float value);
    float getAO() const;
    
    void setEmission(const Color& color);
    Color getEmission() const;
    
    void setEmissionIntensity(float intensity);
    float getEmissionIntensity() const;
};

/**
 * @brief Unlit material
 */
class UnlitMaterial : public Material {
public:
    UnlitMaterial();
    ~UnlitMaterial();

    void setColor(const Color& color);
    Color getColor() const;
    
    void setMainTexture(std::shared_ptr<Texture> texture);
};

/**
 * @brief Material library for managing materials
 */
class MaterialLibrary {
public:
    static MaterialLibrary& getInstance();
    
    MaterialLibrary(const MaterialLibrary&) = delete;
    MaterialLibrary& operator=(const MaterialLibrary&) = delete;

    void addMaterial(const std::string& name, std::shared_ptr<Material> material);
    void removeMaterial(const std::string& name);
    
    std::shared_ptr<Material> getMaterial(const std::string& name);
    bool hasMaterial(const std::string& name) const;
    
    std::vector<std::string> getMaterialNames() const;
    
    void clear();
    
    // Built-in materials
    std::shared_ptr<Material> getDefaultMaterial();
    std::shared_ptr<Material> getErrorMaterial();

private:
    MaterialLibrary();
    ~MaterialLibrary();
    
    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    std::shared_ptr<Material> defaultMaterial;
    std::shared_ptr<Material> errorMaterial;
    
    void createBuiltInMaterials();
};

/**
 * @brief Material instance for per-object material properties
 */
class MaterialInstance {
public:
    MaterialInstance(std::shared_ptr<Material> baseMaterial);
    ~MaterialInstance();

    std::shared_ptr<Material> getBaseMaterial() const;
    
    void setProperty(const std::string& name, const MaterialProperty& property);
    void setFloat(const std::string& name, float value);
    void setInt(const std::string& name, int value);
    void setColor(const std::string& name, const Color& value);
    void setTexture(const std::string& name, std::shared_ptr<Texture> texture);
    
    MaterialProperty* getProperty(const std::string& name);
    bool hasOverride(const std::string& name) const;
    
    void clearOverrides();
    
    void apply();

private:
    std::shared_ptr<Material> baseMaterial;
    std::unordered_map<std::string, MaterialProperty> overrides;
};

/**
 * @brief Material pass for multi-pass rendering
 */
class MaterialPass {
public:
    MaterialPass(const std::string& name);
    ~MaterialPass();

    void setName(const std::string& name);
    std::string getName() const;
    
    void setShader(std::shared_ptr<Shader> shader);
    std::shared_ptr<Shader> getShader() const;
    
    void setRenderState(const RenderState& state);
    RenderState& getRenderState();
    
    void apply();

private:
    std::string name;
    std::shared_ptr<Shader> shader;
    RenderState renderState;
};

/**
 * @brief Multi-pass material
 */
class MultiPassMaterial : public Material {
public:
    MultiPassMaterial(const std::string& name = "MultiPassMaterial");
    ~MultiPassMaterial();

    void addPass(std::shared_ptr<MaterialPass> pass);
    void removePass(size_t index);
    void clearPasses();
    
    MaterialPass* getPass(size_t index);
    size_t getPassCount() const;
    
    void applyPass(size_t index);

private:
    std::vector<std::shared_ptr<MaterialPass>> passes;
};

/**
 * @brief Material utilities
 */
class MaterialUtils {
public:
    static std::shared_ptr<Material> createStandardMaterial(const Color& albedo);
    static std::shared_ptr<Material> createUnlitMaterial(const Color& color);
    static std::shared_ptr<Material> createWireframeMaterial(const Color& color);
    
    static void setGlobalFloat(const std::string& name, float value);
    static void setGlobalColor(const std::string& name, const Color& value);
    static void setGlobalTexture(const std::string& name, std::shared_ptr<Texture> texture);
    
    static float getGlobalFloat(const std::string& name);
    static Color getGlobalColor(const std::string& name);
    static std::shared_ptr<Texture> getGlobalTexture(const std::string& name);

private:
    static std::unordered_map<std::string, MaterialProperty> globalProperties;
};

} // namespace Graphics
} // namespace JJM

#endif // JJM_MATERIAL_SYSTEM_H
