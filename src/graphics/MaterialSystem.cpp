#include "graphics/MaterialSystem.h"
#include <algorithm>
#include <cstring>

namespace JJM {
namespace Graphics {

// MaterialProperty implementation
MaterialProperty::MaterialProperty() : type(PropertyType::Float) {
    data.floatValue = 0.0f;
}

MaterialProperty::MaterialProperty(PropertyType t) : type(t) {
    std::memset(&data, 0, sizeof(data));
}

MaterialProperty::~MaterialProperty() {}

PropertyType MaterialProperty::getType() const { return type; }

void MaterialProperty::setFloat(float value) {
    type = PropertyType::Float;
    data.floatValue = value;
}

void MaterialProperty::setInt(int value) {
    type = PropertyType::Int;
    data.intValue = value;
}

void MaterialProperty::setBool(bool value) {
    type = PropertyType::Bool;
    data.boolValue = value;
}

void MaterialProperty::setColor(const Color& value) {
    type = PropertyType::Color;
    colorValue = value;
}

void MaterialProperty::setVector2(float x, float y) {
    type = PropertyType::Vector2;
    data.vector4[0] = x;
    data.vector4[1] = y;
}

void MaterialProperty::setVector3(float x, float y, float z) {
    type = PropertyType::Vector3;
    data.vector4[0] = x;
    data.vector4[1] = y;
    data.vector4[2] = z;
}

void MaterialProperty::setVector4(float x, float y, float z, float w) {
    type = PropertyType::Vector4;
    data.vector4[0] = x;
    data.vector4[1] = y;
    data.vector4[2] = z;
    data.vector4[3] = w;
}

void MaterialProperty::setTexture(std::shared_ptr<Texture> texture) {
    type = PropertyType::Texture;
    textureValue = texture;
}

float MaterialProperty::getFloat() const {
    return type == PropertyType::Float ? data.floatValue : 0.0f;
}

int MaterialProperty::getInt() const {
    return type == PropertyType::Int ? data.intValue : 0;
}

bool MaterialProperty::getBool() const {
    return type == PropertyType::Bool ? data.boolValue : false;
}

Color MaterialProperty::getColor() const {
    return type == PropertyType::Color ? colorValue : Color(255, 255, 255, 255);
}

void MaterialProperty::getVector2(float& x, float& y) const {
    if (type == PropertyType::Vector2) {
        x = data.vector4[0];
        y = data.vector4[1];
    } else {
        x = y = 0.0f;
    }
}

void MaterialProperty::getVector3(float& x, float& y, float& z) const {
    if (type == PropertyType::Vector3) {
        x = data.vector4[0];
        y = data.vector4[1];
        z = data.vector4[2];
    } else {
        x = y = z = 0.0f;
    }
}

void MaterialProperty::getVector4(float& x, float& y, float& z, float& w) const {
    if (type == PropertyType::Vector4) {
        x = data.vector4[0];
        y = data.vector4[1];
        z = data.vector4[2];
        w = data.vector4[3];
    } else {
        x = y = z = w = 0.0f;
    }
}

std::shared_ptr<Texture> MaterialProperty::getTexture() const {
    return type == PropertyType::Texture ? textureValue : nullptr;
}

// RenderState implementation
RenderState::RenderState()
    : blendMode(BlendMode::Opaque), cullMode(CullMode::Back),
      depthFunc(DepthFunc::Less), depthWrite(true), depthTest(true),
      alphaTest(false), alphaThreshold(0.5f), wireframe(false), renderQueue(2000) {}

// Material implementation
Material::Material(const std::string& name)
    : name(name), shader(nullptr) {}

Material::Material(std::shared_ptr<Shader> shader)
    : name("Material"), shader(shader) {}

Material::~Material() {}

void Material::setName(const std::string& n) { name = n; }
std::string Material::getName() const { return name; }

void Material::setShader(std::shared_ptr<Shader> s) { shader = s; }
std::shared_ptr<Shader> Material::getShader() const { return shader; }

void Material::setProperty(const std::string& propName, const MaterialProperty& property) {
    properties[propName] = property;
}

void Material::setFloat(const std::string& propName, float value) {
    MaterialProperty prop(PropertyType::Float);
    prop.setFloat(value);
    properties[propName] = prop;
}

void Material::setInt(const std::string& propName, int value) {
    MaterialProperty prop(PropertyType::Int);
    prop.setInt(value);
    properties[propName] = prop;
}

void Material::setBool(const std::string& propName, bool value) {
    MaterialProperty prop(PropertyType::Bool);
    prop.setBool(value);
    properties[propName] = prop;
}

void Material::setColor(const std::string& propName, const Color& value) {
    MaterialProperty prop(PropertyType::Color);
    prop.setColor(value);
    properties[propName] = prop;
}

void Material::setVector2(const std::string& propName, float x, float y) {
    MaterialProperty prop(PropertyType::Vector2);
    prop.setVector2(x, y);
    properties[propName] = prop;
}

void Material::setVector3(const std::string& propName, float x, float y, float z) {
    MaterialProperty prop(PropertyType::Vector3);
    prop.setVector3(x, y, z);
    properties[propName] = prop;
}

void Material::setVector4(const std::string& propName, float x, float y, float z, float w) {
    MaterialProperty prop(PropertyType::Vector4);
    prop.setVector4(x, y, z, w);
    properties[propName] = prop;
}

void Material::setTexture(const std::string& propName, std::shared_ptr<Texture> texture) {
    MaterialProperty prop(PropertyType::Texture);
    prop.setTexture(texture);
    properties[propName] = prop;
}

MaterialProperty* Material::getProperty(const std::string& propName) {
    auto it = properties.find(propName);
    return it != properties.end() ? &it->second : nullptr;
}

bool Material::hasProperty(const std::string& propName) const {
    return properties.find(propName) != properties.end();
}

void Material::setRenderState(const RenderState& state) { renderState = state; }
RenderState& Material::getRenderState() { return renderState; }
const RenderState& Material::getRenderState() const { return renderState; }

void Material::setBlendMode(BlendMode mode) { renderState.blendMode = mode; }
BlendMode Material::getBlendMode() const { return renderState.blendMode; }

void Material::setRenderQueue(int queue) { renderState.renderQueue = queue; }
int Material::getRenderQueue() const { return renderState.renderQueue; }

void Material::setMainTexture(std::shared_ptr<Texture> texture) {
    mainTexture = texture;
    setTexture("_MainTex", texture);
}

std::shared_ptr<Texture> Material::getMainTexture() const { return mainTexture; }

void Material::setNormalMap(std::shared_ptr<Texture> texture) {
    normalMap = texture;
    setTexture("_NormalMap", texture);
}

std::shared_ptr<Texture> Material::getNormalMap() const { return normalMap; }

void Material::setSpecularMap(std::shared_ptr<Texture> texture) {
    specularMap = texture;
    setTexture("_SpecularMap", texture);
}

std::shared_ptr<Texture> Material::getSpecularMap() const { return specularMap; }

void Material::setEmissionMap(std::shared_ptr<Texture> texture) {
    emissionMap = texture;
    setTexture("_EmissionMap", texture);
}

std::shared_ptr<Texture> Material::getEmissionMap() const { return emissionMap; }

void Material::apply() {
    if (shader) {
        bind();
    }
}

void Material::bind() {
    if (!shader) return;
    
    // Apply render state and shader properties
    // This would interact with the rendering backend
}

void Material::unbind() {
    // Unbind shader and restore state
}

void Material::enableKeyword(const std::string& keyword) {
    if (!isKeywordEnabled(keyword)) {
        keywords.push_back(keyword);
    }
}

void Material::disableKeyword(const std::string& keyword) {
    keywords.erase(std::remove(keywords.begin(), keywords.end(), keyword), keywords.end());
}

bool Material::isKeywordEnabled(const std::string& keyword) const {
    return std::find(keywords.begin(), keywords.end(), keyword) != keywords.end();
}

Material* Material::clone() const {
    Material* mat = new Material(name + "_Clone");
    mat->copyPropertiesFrom(*this);
    return mat;
}

void Material::copyPropertiesFrom(const Material& other) {
    shader = other.shader;
    properties = other.properties;
    renderState = other.renderState;
    keywords = other.keywords;
    mainTexture = other.mainTexture;
    normalMap = other.normalMap;
    specularMap = other.specularMap;
    emissionMap = other.emissionMap;
}

// StandardMaterial implementation
StandardMaterial::StandardMaterial() : Material("StandardMaterial") {
    setColor("_Albedo", Color(255, 255, 255, 255));
    setFloat("_Metallic", 0.0f);
    setFloat("_Roughness", 0.5f);
    setFloat("_AO", 1.0f);
    setColor("_Emission", Color(0, 0, 0, 255));
    setFloat("_EmissionIntensity", 1.0f);
}

StandardMaterial::~StandardMaterial() {}

void StandardMaterial::setAlbedo(const Color& color) {
    setColor("_Albedo", color);
}

Color StandardMaterial::getAlbedo() const {
    auto* prop = const_cast<StandardMaterial*>(this)->getProperty("_Albedo");
    return prop ? prop->getColor() : Color(255, 255, 255, 255);
}

void StandardMaterial::setMetallic(float value) {
    setFloat("_Metallic", value);
}

float StandardMaterial::getMetallic() const {
    auto* prop = const_cast<StandardMaterial*>(this)->getProperty("_Metallic");
    return prop ? prop->getFloat() : 0.0f;
}

void StandardMaterial::setRoughness(float value) {
    setFloat("_Roughness", value);
}

float StandardMaterial::getRoughness() const {
    auto* prop = const_cast<StandardMaterial*>(this)->getProperty("_Roughness");
    return prop ? prop->getFloat() : 0.5f;
}

void StandardMaterial::setAO(float value) {
    setFloat("_AO", value);
}

float StandardMaterial::getAO() const {
    auto* prop = const_cast<StandardMaterial*>(this)->getProperty("_AO");
    return prop ? prop->getFloat() : 1.0f;
}

void StandardMaterial::setEmission(const Color& color) {
    setColor("_Emission", color);
}

Color StandardMaterial::getEmission() const {
    auto* prop = const_cast<StandardMaterial*>(this)->getProperty("_Emission");
    return prop ? prop->getColor() : Color(0, 0, 0, 255);
}

void StandardMaterial::setEmissionIntensity(float intensity) {
    setFloat("_EmissionIntensity", intensity);
}

float StandardMaterial::getEmissionIntensity() const {
    auto* prop = const_cast<StandardMaterial*>(this)->getProperty("_EmissionIntensity");
    return prop ? prop->getFloat() : 1.0f;
}

// UnlitMaterial implementation
UnlitMaterial::UnlitMaterial() : Material("UnlitMaterial") {
    Material::setColor("_Color", Color(255, 255, 255, 255));
}

UnlitMaterial::~UnlitMaterial() {}

void UnlitMaterial::setColor(const Color& color) {
    Material::setColor("_Color", color);
}

Color UnlitMaterial::getColor() const {
    auto* prop = const_cast<UnlitMaterial*>(this)->getProperty("_Color");
    return prop ? prop->getColor() : Color(255, 255, 255, 255);
}

void UnlitMaterial::setMainTexture(std::shared_ptr<Texture> texture) {
    Material::setMainTexture(texture);
}

// MaterialLibrary implementation
MaterialLibrary::MaterialLibrary() {
    createBuiltInMaterials();
}

MaterialLibrary::~MaterialLibrary() {}

MaterialLibrary& MaterialLibrary::getInstance() {
    static MaterialLibrary instance;
    return instance;
}

void MaterialLibrary::addMaterial(const std::string& name, std::shared_ptr<Material> material) {
    materials[name] = material;
}

void MaterialLibrary::removeMaterial(const std::string& name) {
    materials.erase(name);
}

std::shared_ptr<Material> MaterialLibrary::getMaterial(const std::string& name) {
    auto it = materials.find(name);
    return it != materials.end() ? it->second : nullptr;
}

bool MaterialLibrary::hasMaterial(const std::string& name) const {
    return materials.find(name) != materials.end();
}

std::vector<std::string> MaterialLibrary::getMaterialNames() const {
    std::vector<std::string> names;
    for (const auto& pair : materials) {
        names.push_back(pair.first);
    }
    return names;
}

void MaterialLibrary::clear() {
    materials.clear();
    createBuiltInMaterials();
}

std::shared_ptr<Material> MaterialLibrary::getDefaultMaterial() {
    return defaultMaterial;
}

std::shared_ptr<Material> MaterialLibrary::getErrorMaterial() {
    return errorMaterial;
}

void MaterialLibrary::createBuiltInMaterials() {
    defaultMaterial = std::make_shared<StandardMaterial>();
    defaultMaterial->setName("Default");
    
    auto error = std::make_shared<UnlitMaterial>();
    error->setName("Error");
    error->setColor(Color(255, 0, 255, 255));
    errorMaterial = error;
}

// MaterialInstance implementation
MaterialInstance::MaterialInstance(std::shared_ptr<Material> baseMaterial)
    : baseMaterial(baseMaterial) {}

MaterialInstance::~MaterialInstance() {}

std::shared_ptr<Material> MaterialInstance::getBaseMaterial() const {
    return baseMaterial;
}

void MaterialInstance::setProperty(const std::string& name, const MaterialProperty& property) {
    overrides[name] = property;
}

void MaterialInstance::setFloat(const std::string& name, float value) {
    MaterialProperty prop(PropertyType::Float);
    prop.setFloat(value);
    overrides[name] = prop;
}

void MaterialInstance::setInt(const std::string& name, int value) {
    MaterialProperty prop(PropertyType::Int);
    prop.setInt(value);
    overrides[name] = prop;
}

void MaterialInstance::setColor(const std::string& name, const Color& value) {
    MaterialProperty prop(PropertyType::Color);
    prop.setColor(value);
    overrides[name] = prop;
}

void MaterialInstance::setTexture(const std::string& name, std::shared_ptr<Texture> texture) {
    MaterialProperty prop(PropertyType::Texture);
    prop.setTexture(texture);
    overrides[name] = prop;
}

MaterialProperty* MaterialInstance::getProperty(const std::string& name) {
    auto it = overrides.find(name);
    if (it != overrides.end()) {
        return &it->second;
    }
    return baseMaterial ? baseMaterial->getProperty(name) : nullptr;
}

bool MaterialInstance::hasOverride(const std::string& name) const {
    return overrides.find(name) != overrides.end();
}

void MaterialInstance::clearOverrides() {
    overrides.clear();
}

void MaterialInstance::apply() {
    if (baseMaterial) {
        baseMaterial->apply();
        // Apply overrides on top
    }
}

// MaterialPass implementation
MaterialPass::MaterialPass(const std::string& name) : name(name) {}
MaterialPass::~MaterialPass() {}

void MaterialPass::setName(const std::string& n) { name = n; }
std::string MaterialPass::getName() const { return name; }

void MaterialPass::setShader(std::shared_ptr<Shader> s) { shader = s; }
std::shared_ptr<Shader> MaterialPass::getShader() const { return shader; }

void MaterialPass::setRenderState(const RenderState& state) { renderState = state; }
RenderState& MaterialPass::getRenderState() { return renderState; }

void MaterialPass::apply() {
    if (shader) {
        // Apply shader and render state
    }
}

// MultiPassMaterial implementation
MultiPassMaterial::MultiPassMaterial(const std::string& name) : Material(name) {}
MultiPassMaterial::~MultiPassMaterial() {}

void MultiPassMaterial::addPass(std::shared_ptr<MaterialPass> pass) {
    passes.push_back(pass);
}

void MultiPassMaterial::removePass(size_t index) {
    if (index < passes.size()) {
        passes.erase(passes.begin() + index);
    }
}

void MultiPassMaterial::clearPasses() {
    passes.clear();
}

MaterialPass* MultiPassMaterial::getPass(size_t index) {
    return index < passes.size() ? passes[index].get() : nullptr;
}

size_t MultiPassMaterial::getPassCount() const {
    return passes.size();
}

void MultiPassMaterial::applyPass(size_t index) {
    if (index < passes.size()) {
        passes[index]->apply();
    }
}

// MaterialUtils implementation
std::unordered_map<std::string, MaterialProperty> MaterialUtils::globalProperties;

std::shared_ptr<Material> MaterialUtils::createStandardMaterial(const Color& albedo) {
    auto mat = std::make_shared<StandardMaterial>();
    mat->setAlbedo(albedo);
    return mat;
}

std::shared_ptr<Material> MaterialUtils::createUnlitMaterial(const Color& color) {
    auto mat = std::make_shared<UnlitMaterial>();
    mat->setColor(color);
    return mat;
}

std::shared_ptr<Material> MaterialUtils::createWireframeMaterial(const Color& color) {
    auto mat = std::make_shared<UnlitMaterial>();
    mat->setColor(color);
    auto& state = mat->getRenderState();
    state.wireframe = true;
    return mat;
}

void MaterialUtils::setGlobalFloat(const std::string& name, float value) {
    MaterialProperty prop(PropertyType::Float);
    prop.setFloat(value);
    globalProperties[name] = prop;
}

void MaterialUtils::setGlobalColor(const std::string& name, const Color& value) {
    MaterialProperty prop(PropertyType::Color);
    prop.setColor(value);
    globalProperties[name] = prop;
}

void MaterialUtils::setGlobalTexture(const std::string& name, std::shared_ptr<Texture> texture) {
    MaterialProperty prop(PropertyType::Texture);
    prop.setTexture(texture);
    globalProperties[name] = prop;
}

float MaterialUtils::getGlobalFloat(const std::string& name) {
    auto it = globalProperties.find(name);
    return it != globalProperties.end() ? it->second.getFloat() : 0.0f;
}

Color MaterialUtils::getGlobalColor(const std::string& name) {
    auto it = globalProperties.find(name);
    return it != globalProperties.end() ? it->second.getColor() : Color(255, 255, 255, 255);
}

std::shared_ptr<Texture> MaterialUtils::getGlobalTexture(const std::string& name) {
    auto it = globalProperties.find(name);
    return it != globalProperties.end() ? it->second.getTexture() : nullptr;
}

} // namespace Graphics
} // namespace JJM
