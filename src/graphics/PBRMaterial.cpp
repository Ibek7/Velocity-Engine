#include "graphics/PBRMaterial.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Graphics {

// PBRMaterial Implementation
PBRMaterial::PBRMaterial(const std::string& name)
    : m_name(name)
    , m_workflow(PBRWorkflow::MetallicRoughness) {
}

PBRMaterial::~PBRMaterial() {
}

void PBRMaterial::setTexture(PBRTextureType type, uint32_t textureId) {
    m_textures[type] = textureId;
}

uint32_t PBRMaterial::getTexture(PBRTextureType type) const {
    auto it = m_textures.find(type);
    return (it != m_textures.end()) ? it->second : 0;
}

bool PBRMaterial::hasTexture(PBRTextureType type) const {
    return m_textures.find(type) != m_textures.end();
}

void PBRMaterial::removeTexture(PBRTextureType type) {
    m_textures.erase(type);
}

void PBRMaterial::bindUniforms(uint32_t shader) const {
    // TODO: Bind material properties to shader uniforms
    // This would use OpenGL or other graphics API calls
}

void PBRMaterial::bindTextures() const {
    // TODO: Bind all textures to their respective slots
}

PBRMaterial PBRMaterial::createMetalPreset(const std::string& name) {
    PBRMaterial material(name);
    material.m_properties.metallic = 1.0f;
    material.m_properties.roughness = 0.3f;
    material.m_properties.albedo[0] = 0.8f;
    material.m_properties.albedo[1] = 0.8f;
    material.m_properties.albedo[2] = 0.85f;
    return material;
}

PBRMaterial PBRMaterial::createPlasticPreset(const std::string& name) {
    PBRMaterial material(name);
    material.m_properties.metallic = 0.0f;
    material.m_properties.roughness = 0.5f;
    material.m_properties.albedo[0] = 0.2f;
    material.m_properties.albedo[1] = 0.5f;
    material.m_properties.albedo[2] = 0.8f;
    return material;
}

PBRMaterial PBRMaterial::createWoodPreset(const std::string& name) {
    PBRMaterial material(name);
    material.m_properties.metallic = 0.0f;
    material.m_properties.roughness = 0.8f;
    material.m_properties.albedo[0] = 0.6f;
    material.m_properties.albedo[1] = 0.4f;
    material.m_properties.albedo[2] = 0.2f;
    return material;
}

PBRMaterial PBRMaterial::createStonePreset(const std::string& name) {
    PBRMaterial material(name);
    material.m_properties.metallic = 0.0f;
    material.m_properties.roughness = 0.9f;
    material.m_properties.albedo[0] = 0.5f;
    material.m_properties.albedo[1] = 0.5f;
    material.m_properties.albedo[2] = 0.5f;
    return material;
}

PBRMaterial PBRMaterial::createGlassPreset(const std::string& name) {
    PBRMaterial material(name);
    material.m_properties.metallic = 0.0f;
    material.m_properties.roughness = 0.0f;
    material.m_properties.useTransmission = true;
    material.m_properties.transmission = 0.9f;
    material.m_properties.ior = 1.5f;
    material.m_properties.albedo[0] = 1.0f;
    material.m_properties.albedo[1] = 1.0f;
    material.m_properties.albedo[2] = 1.0f;
    return material;
}

// PBRMaterialManager Implementation
PBRMaterialManager::PBRMaterialManager()
    : m_nextMaterialId(1)
    , m_defaultShader(0) {
}

PBRMaterialManager::~PBRMaterialManager() {
    m_materials.clear();
}

uint32_t PBRMaterialManager::createMaterial(const std::string& name) {
    uint32_t id = m_nextMaterialId++;
    auto material = std::make_unique<PBRMaterial>(name);
    m_materials[id] = std::move(material);
    m_materialNameMap[name] = id;
    return id;
}

void PBRMaterialManager::destroyMaterial(uint32_t materialId) {
    auto it = m_materials.find(materialId);
    if (it != m_materials.end()) {
        m_materialNameMap.erase(it->second->getName());
        m_materials.erase(it);
    }
}

PBRMaterial* PBRMaterialManager::getMaterial(uint32_t materialId) {
    auto it = m_materials.find(materialId);
    return (it != m_materials.end()) ? it->second.get() : nullptr;
}

const PBRMaterial* PBRMaterialManager::getMaterial(uint32_t materialId) const {
    auto it = m_materials.find(materialId);
    return (it != m_materials.end()) ? it->second.get() : nullptr;
}

uint32_t PBRMaterialManager::findMaterial(const std::string& name) const {
    auto it = m_materialNameMap.find(name);
    return (it != m_materialNameMap.end()) ? it->second : 0;
}

bool PBRMaterialManager::loadMaterial(const std::string& filepath) {
    // TODO: Load material from file (JSON, XML, or custom format)
    return false;
}

bool PBRMaterialManager::saveMaterial(uint32_t materialId, const std::string& filepath) const {
    // TODO: Save material to file
    return false;
}

uint32_t PBRMaterialManager::createPreset(const std::string& name, const std::string& presetType) {
    uint32_t id = m_nextMaterialId++;
    std::unique_ptr<PBRMaterial> material;
    
    if (presetType == "metal") {
        material = std::make_unique<PBRMaterial>(PBRMaterial::createMetalPreset(name));
    } else if (presetType == "plastic") {
        material = std::make_unique<PBRMaterial>(PBRMaterial::createPlasticPreset(name));
    } else if (presetType == "wood") {
        material = std::make_unique<PBRMaterial>(PBRMaterial::createWoodPreset(name));
    } else if (presetType == "stone") {
        material = std::make_unique<PBRMaterial>(PBRMaterial::createStonePreset(name));
    } else if (presetType == "glass") {
        material = std::make_unique<PBRMaterial>(PBRMaterial::createGlassPreset(name));
    } else {
        material = std::make_unique<PBRMaterial>(name);
    }
    
    m_materials[id] = std::move(material);
    m_materialNameMap[name] = id;
    return id;
}

// PBRRenderer Implementation
uint32_t PBRRenderer::generateIrradianceMap(uint32_t environmentMap, uint32_t resolution) {
    // TODO: Generate irradiance map from environment map using convolution
    return 0;
}

uint32_t PBRRenderer::generatePrefilterMap(uint32_t environmentMap, uint32_t resolution) {
    // TODO: Generate prefiltered environment map for different roughness levels
    return 0;
}

uint32_t PBRRenderer::generateBRDFLUT(uint32_t resolution) {
    // TODO: Generate BRDF integration lookup table
    return 0;
}

void PBRRenderer::setupIBL(uint32_t irradianceMap, uint32_t prefilterMap, uint32_t brdfLUT) {
    // TODO: Bind IBL maps for rendering
}

void PBRRenderer::renderPBR(const PBRMaterial* material,
                           uint32_t mesh,
                           const float* modelMatrix,
                           const float* viewMatrix,
                           const float* projectionMatrix) {
    if (!material) return;
    
    // TODO: Render mesh with PBR material
    // 1. Bind PBR shader
    // 2. Set uniforms (matrices, material properties)
    // 3. Bind textures
    // 4. Bind IBL maps
    // 5. Draw mesh
}

// BRDFHelper Implementation
float BRDFHelper::distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265359f * denom * denom;
    
    return nom / denom;
}

float BRDFHelper::geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return nom / denom;
}

float BRDFHelper::geometrySmith(float NdotV, float NdotL, float roughness) {
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

void BRDFHelper::fresnelSchlick(float cosTheta, const float* F0, float* result) {
    float power = std::pow(1.0f - cosTheta, 5.0f);
    result[0] = F0[0] + (1.0f - F0[0]) * power;
    result[1] = F0[1] + (1.0f - F0[1]) * power;
    result[2] = F0[2] + (1.0f - F0[2]) * power;
}

void BRDFHelper::fresnelSchlickRoughness(float cosTheta, const float* F0,
                                        float roughness, float* result) {
    float power = std::pow(1.0f - cosTheta, 5.0f);
    float oneMinusRoughness = 1.0f - roughness;
    
    result[0] = F0[0] + (std::max(oneMinusRoughness, F0[0]) - F0[0]) * power;
    result[1] = F0[1] + (std::max(oneMinusRoughness, F0[1]) - F0[1]) * power;
    result[2] = F0[2] + (std::max(oneMinusRoughness, F0[2]) - F0[2]) * power;
}

void BRDFHelper::metallicRoughnessToSpecularGlossiness(
    const float* albedo, float metallic, float roughness,
    float* specular, float* glossiness) {
    
    const float dielectricSpecular = 0.04f;
    
    // Lerp between dielectric and metal specular
    for (int i = 0; i < 3; ++i) {
        specular[i] = dielectricSpecular * (1.0f - metallic) + albedo[i] * metallic;
    }
    
    *glossiness = 1.0f - roughness;
}

void BRDFHelper::specularGlossinessToMetallicRoughness(
    const float* specular, float glossiness,
    float* albedo, float* metallic, float* roughness) {
    
    const float dielectricSpecular = 0.04f;
    
    // Calculate perceived brightness of specular color
    float specularBrightness = (specular[0] + specular[1] + specular[2]) / 3.0f;
    
    // Estimate metallic
    *metallic = std::clamp((specularBrightness - dielectricSpecular) / 
                          (1.0f - dielectricSpecular), 0.0f, 1.0f);
    
    // Calculate albedo
    if (*metallic > 0.0f) {
        for (int i = 0; i < 3; ++i) {
            albedo[i] = specular[i] / *metallic;
        }
    } else {
        albedo[0] = albedo[1] = albedo[2] = 1.0f;
    }
    
    *roughness = 1.0f - glossiness;
}

} // namespace Graphics
} // namespace JJM
