#include "graphics/MaterialInstance.h"
#include <GL/glew.h>
#include <algorithm>
#include <functional>

namespace JJM {
namespace Graphics {

// =============================================================================
// MaterialInstance Implementation
// =============================================================================

MaterialInstance::MaterialInstance(std::shared_ptr<Material> baseMaterial)
    : m_baseMaterial(baseMaterial)
    , m_dirty(true)
    , m_instanceHash(0)
{
    updateHash();
}

MaterialInstance::~MaterialInstance() = default;

void MaterialInstance::setTexture(const std::string& name, unsigned int textureHandle) {
    m_textureOverrides[name] = textureHandle;
    m_dirty = true;
    updateHash();
}

unsigned int MaterialInstance::getTexture(const std::string& name) const {
    auto it = m_textureOverrides.find(name);
    if (it != m_textureOverrides.end()) {
        return it->second;
    }
    
    // Fall back to base material
    if (m_baseMaterial) {
        return m_baseMaterial->getTexture(name);
    }
    
    return 0;
}

bool MaterialInstance::hasOverride(const std::string& name) const {
    return m_parameterOverrides.count(name) > 0 || m_textureOverrides.count(name) > 0;
}

void MaterialInstance::removeOverride(const std::string& name) {
    m_parameterOverrides.erase(name);
    m_textureOverrides.erase(name);
    m_dirty = true;
    updateHash();
}

void MaterialInstance::clearOverrides() {
    m_parameterOverrides.clear();
    m_textureOverrides.clear();
    m_dirty = true;
    updateHash();
}

void MaterialInstance::bind() {
    if (!m_baseMaterial) return;
    
    // Bind base material first
    m_baseMaterial->bind();
    
    // Apply parameter overrides to shader
    auto shader = m_baseMaterial->getShader();
    if (!shader) return;
    
    // TODO: Apply overrides to shader uniforms
    // This requires shader uniform setting functionality
    
    // Apply texture overrides
    int textureUnit = 0;
    for (const auto& [name, handle] : m_textureOverrides) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, handle);
        // TODO: Set uniform for texture unit
        textureUnit++;
    }
    
    m_dirty = false;
}

void MaterialInstance::updateHash() {
    // Compute hash based on base material and overrides
    std::hash<std::string> hasher;
    m_instanceHash = 0;
    
    if (m_baseMaterial) {
        m_instanceHash ^= hasher(m_baseMaterial->getName());
    }
    
    // Hash parameter overrides
    for (const auto& [name, value] : m_parameterOverrides) {
        m_instanceHash ^= hasher(name);
        // Hash value based on type
        // TODO: Implement proper variant hashing
    }
    
    // Hash texture overrides
    for (const auto& [name, handle] : m_textureOverrides) {
        m_instanceHash ^= hasher(name) ^ handle;
    }
}

// =============================================================================
// Material Implementation
// =============================================================================

Material::Material(const std::string& name)
    : m_name(name)
    , m_depthTest(true)
    , m_depthWrite(true)
    , m_alphaBlend(false)
    , m_blendSrc(GL_SRC_ALPHA)
    , m_blendDst(GL_ONE_MINUS_SRC_ALPHA)
    , m_cullMode(GL_BACK)
    , m_supportsInstancing(true)
{}

Material::~Material() = default;

void Material::setShader(std::shared_ptr<Shader> shader) {
    m_shader = shader;
}

void Material::addParameter(const std::string& name, const MaterialParameterValue& defaultValue, bool isDynamic) {
    MaterialParameter param;
    param.name = name;
    param.defaultValue = defaultValue;
    param.isDynamic = isDynamic;
    m_parameters[name] = param;
}

void Material::setTexture(const std::string& name, unsigned int textureHandle) {
    m_textures[name] = textureHandle;
}

unsigned int Material::getTexture(const std::string& name) const {
    auto it = m_textures.find(name);
    if (it != m_textures.end()) {
        return it->second;
    }
    return 0;
}

std::shared_ptr<MaterialInstance> Material::createInstance() {
    auto instance = std::make_shared<MaterialInstance>(shared_from_this());
    m_instances.push_back(instance);
    cleanupInstances();
    return instance;
}

void Material::bind() {
    applyRenderState();
    
    // Bind shader
    if (m_shader) {
        // TODO: Bind shader program
    }
    
    // Set default parameters
    // TODO: Set shader uniforms based on m_parameters
    
    // Bind textures
    int textureUnit = 0;
    for (const auto& [name, handle] : m_textures) {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, handle);
        // TODO: Set shader uniform for texture unit
        textureUnit++;
    }
}

void Material::applyRenderState() {
    // Apply depth testing
    if (m_depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    glDepthMask(m_depthWrite ? GL_TRUE : GL_FALSE);
    
    // Apply blending
    if (m_alphaBlend) {
        glEnable(GL_BLEND);
        glBlendFunc(m_blendSrc, m_blendDst);
    } else {
        glDisable(GL_BLEND);
    }
    
    // Apply culling
    if (m_cullMode != GL_NONE) {
        glEnable(GL_CULL_FACE);
        glCullFace(m_cullMode);
    } else {
        glDisable(GL_CULL_FACE);
    }
}

void Material::cleanupInstances() {
    // Remove expired weak pointers
    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(),
            [](const std::weak_ptr<MaterialInstance>& wp) { return wp.expired(); }),
        m_instances.end()
    );
}

// =============================================================================
// MaterialSystem Implementation
// =============================================================================

MaterialSystem::MaterialSystem() = default;

MaterialSystem::~MaterialSystem() {
    clear();
}

std::shared_ptr<Material> MaterialSystem::createMaterial(const std::string& name) {
    auto material = std::make_shared<Material>(name);
    m_materials[name] = material;
    return material;
}

std::shared_ptr<Material> MaterialSystem::getMaterial(const std::string& name) {
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        return it->second;
    }
    return nullptr;
}

void MaterialSystem::removeMaterial(const std::string& name) {
    m_materials.erase(name);
}

std::shared_ptr<Material> MaterialSystem::loadMaterial(const std::string& filepath) {
    // TODO: Implement material loading from file (JSON/XML format)
    return nullptr;
}

void MaterialSystem::sortForBatching(std::vector<MaterialInstance*>& instances) {
    // Sort by material instance hash to group similar instances together
    std::sort(instances.begin(), instances.end(),
        [](const MaterialInstance* a, const MaterialInstance* b) {
            return a->getHash() < b->getHash();
        });
}

void MaterialSystem::buildBatches(const std::vector<MaterialInstance*>& instances) {
    m_instanceBatches.clear();
    
    // Group instances by hash
    for (MaterialInstance* instance : instances) {
        m_instanceBatches[instance->getHash()].push_back(instance);
    }
}

void MaterialSystem::clear() {
    m_materials.clear();
    m_instanceBatches.clear();
}

// Template specializations for MaterialInstance::getParameter
template<>
int MaterialInstance::getParameter<int>(const std::string& name) const {
    auto it = m_parameterOverrides.find(name);
    if (it != m_parameterOverrides.end()) {
        return std::get<int>(it->second);
    }
    
    if (m_baseMaterial) {
        return m_baseMaterial->getParameter<int>(name);
    }
    
    return 0;
}

template<>
float MaterialInstance::getParameter<float>(const std::string& name) const {
    auto it = m_parameterOverrides.find(name);
    if (it != m_parameterOverrides.end()) {
        return std::get<float>(it->second);
    }
    
    if (m_baseMaterial) {
        return m_baseMaterial->getParameter<float>(name);
    }
    
    return 0.0f;
}

// Template specializations for Material::getParameter
template<>
int Material::getParameter<int>(const std::string& name) const {
    auto it = m_parameters.find(name);
    if (it != m_parameters.end()) {
        return std::get<int>(it->second.defaultValue);
    }
    return 0;
}

template<>
float Material::getParameter<float>(const std::string& name) const {
    auto it = m_parameters.find(name);
    if (it != m_parameters.end()) {
        return std::get<float>(it->second.defaultValue);
    }
    return 0.0f;
}

} // namespace Graphics
} // namespace JJM
