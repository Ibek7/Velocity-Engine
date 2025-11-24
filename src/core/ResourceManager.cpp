#include "core/ResourceManager.h"
#include <iostream>

namespace JJM {
namespace Core {

ResourceManager* ResourceManager::instance = nullptr;

ResourceManager::ResourceManager() : renderer(nullptr) {}

ResourceManager::~ResourceManager() {
    clear();
}

ResourceManager* ResourceManager::getInstance() {
    if (!instance) {
        instance = new ResourceManager();
    }
    return instance;
}

void ResourceManager::destroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

std::shared_ptr<Graphics::Texture> ResourceManager::loadTexture(const std::string& id, const std::string& filePath) {
    if (!renderer) {
        std::cerr << "ResourceManager: Renderer not set!" << std::endl;
        return nullptr;
    }
    
    // Check if already loaded
    auto it = textures.find(id);
    if (it != textures.end()) {
        return it->second;
    }
    
    // Load new texture
    auto texture = std::make_shared<Graphics::Texture>();
    if (texture->loadFromFile(filePath, renderer)) {
        textures[id] = texture;
        std::cout << "Loaded texture: " << id << " from " << filePath << std::endl;
        return texture;
    }
    
    std::cerr << "Failed to load texture: " << filePath << std::endl;
    return nullptr;
}

std::shared_ptr<Graphics::Texture> ResourceManager::getTexture(const std::string& id) {
    auto it = textures.find(id);
    if (it != textures.end()) {
        return it->second;
    }
    return nullptr;
}

void ResourceManager::unloadTexture(const std::string& id) {
    auto it = textures.find(id);
    if (it != textures.end()) {
        textures.erase(it);
        std::cout << "Unloaded texture: " << id << std::endl;
    }
}

void ResourceManager::unloadAllTextures() {
    textures.clear();
    std::cout << "All textures unloaded" << std::endl;
}

bool ResourceManager::hasTexture(const std::string& id) const {
    return textures.find(id) != textures.end();
}

void ResourceManager::clear() {
    unloadAllTextures();
}

} // namespace Core
} // namespace JJM
