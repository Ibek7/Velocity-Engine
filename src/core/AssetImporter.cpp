#include "../../include/core/AssetImporter.h"
#include <algorithm>

namespace JJM {
namespace Core {

AssetPipeline::AssetPipeline() {
}

AssetPipeline::~AssetPipeline() {
}

void AssetPipeline::registerImporter(const std::string& extension, 
                                    std::shared_ptr<AssetImporter> importer) {
    m_importers[extension] = importer;
}

void* AssetPipeline::importAsset(const std::string& filepath, const ImportSettings& settings) {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) return nullptr;
    
    std::string ext = filepath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = m_importers.find(ext);
    if (it != m_importers.end()) {
        return it->second->import(filepath, settings);
    }
    
    return nullptr;
}

bool AssetPipeline::exportAsset(void* asset, AssetType type, const std::string& filepath) {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) return false;
    
    std::string ext = filepath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = m_importers.find(ext);
    if (it != m_importers.end()) {
        return it->second->exportAsset(asset, filepath);
    }
    
    return false;
}

AssetType AssetPipeline::detectType(const std::string& filepath) {
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos == std::string::npos) return AssetType::UNKNOWN;
    
    std::string ext = filepath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf") return AssetType::MESH;
    if (ext == ".png" || ext == ".jpg" || ext == ".tga") return AssetType::TEXTURE;
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg") return AssetType::AUDIO;
    if (ext == ".glsl" || ext == ".hlsl") return AssetType::SHADER;
    if (ext == ".ttf" || ext == ".otf") return AssetType::FONT;
    
    return AssetType::UNKNOWN;
}

} // namespace Core
} // namespace JJM
