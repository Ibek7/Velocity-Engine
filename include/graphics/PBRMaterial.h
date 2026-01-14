#ifndef PBR_MATERIAL_H
#define PBR_MATERIAL_H

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Graphics {

/**
 * @brief PBR workflow type
 */
enum class PBRWorkflow {
    MetallicRoughness,  // Metallic/Roughness workflow (glTF standard)
    SpecularGlossiness  // Specular/Glossiness workflow
};

/**
 * @brief PBR texture slots
 */
enum class PBRTextureType {
    Albedo,           // Base color
    Normal,           // Normal map
    Metallic,         // Metallic map
    Roughness,        // Roughness map
    AO,              // Ambient occlusion
    Emissive,        // Emissive/glow
    Height,          // Height/displacement
    MetallicRoughness, // Combined metallic-roughness (glTF)
    SpecularGlossiness // Combined specular-glossiness
};

/**
 * @brief PBR material properties
 */
struct PBRProperties {
    // Base properties
    float albedo[4];          // RGBA base color
    float metallic;           // Metallic factor (0-1)
    float roughness;          // Roughness factor (0-1)
    float ao;                 // Ambient occlusion factor (0-1)
    
    // Emissive
    float emissive[3];        // RGB emissive color
    float emissiveStrength;   // Emissive intensity
    
    // Advanced properties
    float normalScale;        // Normal map intensity
    float heightScale;        // Parallax height scale
    float alphaCutoff;        // Alpha test threshold
    bool doubleSided;         // Render both sides
    
    // Clear coat (optional)
    bool useClearCoat;
    float clearCoat;          // Clear coat intensity
    float clearCoatRoughness; // Clear coat roughness
    
    // Sheen (optional, for cloth)
    bool useSheen;
    float sheen[3];           // RGB sheen color
    float sheenRoughness;
    
    // Transmission (optional, for glass)
    bool useTransmission;
    float transmission;       // Transmission factor
    float ior;               // Index of refraction
    
    PBRProperties()
        : metallic(0.0f)
        , roughness(0.5f)
        , ao(1.0f)
        , emissiveStrength(0.0f)
        , normalScale(1.0f)
        , heightScale(0.1f)
        , alphaCutoff(0.5f)
        , doubleSided(false)
        , useClearCoat(false)
        , clearCoat(0.0f)
        , clearCoatRoughness(0.0f)
        , useSheen(false)
        , sheenRoughness(0.0f)
        , useTransmission(false)
        , transmission(0.0f)
        , ior(1.5f) {
        
        // Default white albedo
        albedo[0] = albedo[1] = albedo[2] = albedo[3] = 1.0f;
        
        // Default black emissive
        emissive[0] = emissive[1] = emissive[2] = 0.0f;
        
        // Default sheen
        sheen[0] = sheen[1] = sheen[2] = 0.0f;
    }
};

/**
 * @brief PBR Material
 */
class PBRMaterial {
public:
    PBRMaterial(const std::string& name);
    ~PBRMaterial();
    
    const std::string& getName() const { return m_name; }
    
    // Workflow
    void setWorkflow(PBRWorkflow workflow) { m_workflow = workflow; }
    PBRWorkflow getWorkflow() const { return m_workflow; }
    
    // Properties
    void setProperties(const PBRProperties& props) { m_properties = props; }
    PBRProperties& getProperties() { return m_properties; }
    const PBRProperties& getProperties() const { return m_properties; }
    
    // Textures
    void setTexture(PBRTextureType type, uint32_t textureId);
    uint32_t getTexture(PBRTextureType type) const;
    bool hasTexture(PBRTextureType type) const;
    void removeTexture(PBRTextureType type);
    
    // Shader uniforms
    void bindUniforms(uint32_t shader) const;
    void bindTextures() const;
    
    // Presets
    static PBRMaterial createMetalPreset(const std::string& name);
    static PBRMaterial createPlasticPreset(const std::string& name);
    static PBRMaterial createWoodPreset(const std::string& name);
    static PBRMaterial createStonePreset(const std::string& name);
    static PBRMaterial createGlassPreset(const std::string& name);
    
private:
    std::string m_name;
    PBRWorkflow m_workflow;
    PBRProperties m_properties;
    std::unordered_map<PBRTextureType, uint32_t> m_textures;
};

/**
 * @brief PBR Material manager
 */
class PBRMaterialManager {
public:
    PBRMaterialManager();
    ~PBRMaterialManager();
    
    // Material management
    uint32_t createMaterial(const std::string& name);
    void destroyMaterial(uint32_t materialId);
    PBRMaterial* getMaterial(uint32_t materialId);
    const PBRMaterial* getMaterial(uint32_t materialId) const;
    
    // Material lookup by name
    uint32_t findMaterial(const std::string& name) const;
    
    // Loading
    bool loadMaterial(const std::string& filepath);
    bool saveMaterial(uint32_t materialId, const std::string& filepath) const;
    
    // Presets
    uint32_t createPreset(const std::string& name, const std::string& presetType);
    
    // Shader management
    void setDefaultPBRShader(uint32_t shader) { m_defaultShader = shader; }
    uint32_t getDefaultPBRShader() const { return m_defaultShader; }
    
    // Statistics
    uint32_t getMaterialCount() const { return static_cast<uint32_t>(m_materials.size()); }
    
private:
    std::unordered_map<uint32_t, std::unique_ptr<PBRMaterial>> m_materials;
    std::unordered_map<std::string, uint32_t> m_materialNameMap;
    uint32_t m_nextMaterialId;
    uint32_t m_defaultShader;
};

/**
 * @brief PBR rendering utilities
 */
class PBRRenderer {
public:
    // Environment maps
    static uint32_t generateIrradianceMap(uint32_t environmentMap, uint32_t resolution);
    static uint32_t generatePrefilterMap(uint32_t environmentMap, uint32_t resolution);
    static uint32_t generateBRDFLUT(uint32_t resolution);
    
    // IBL (Image-Based Lighting)
    static void setupIBL(uint32_t irradianceMap, uint32_t prefilterMap, uint32_t brdfLUT);
    
    // Rendering
    static void renderPBR(const PBRMaterial* material,
                         uint32_t mesh,
                         const float* modelMatrix,
                         const float* viewMatrix,
                         const float* projectionMatrix);
};

/**
 * @brief BRDF (Bidirectional Reflectance Distribution Function) helper
 */
class BRDFHelper {
public:
    // Distribution functions
    static float distributionGGX(float NdotH, float roughness);
    static float geometrySchlickGGX(float NdotV, float roughness);
    static float geometrySmith(float NdotV, float NdotL, float roughness);
    
    // Fresnel
    static void fresnelSchlick(float cosTheta, const float* F0, float* result);
    static void fresnelSchlickRoughness(float cosTheta, const float* F0, 
                                       float roughness, float* result);
    
    // Convert between workflows
    static void metallicRoughnessToSpecularGlossiness(
        const float* albedo, float metallic, float roughness,
        float* specular, float* glossiness);
    
    static void specularGlossinessToMetallicRoughness(
        const float* specular, float glossiness,
        float* albedo, float* metallic, float* roughness);
};

} // namespace Graphics
} // namespace JJM

#endif // PBR_MATERIAL_H
