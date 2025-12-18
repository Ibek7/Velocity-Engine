#include "core/AssetLoader.h"
#include <iostream>

namespace JJM {
namespace Core {

AssetLoader::AssetLoader(ResourceManager* rm, Audio::AudioManager* am, Graphics::Renderer* r)
    : resourceManager(rm), audioManager(am), renderer(r),
      loadedCount(0), totalCount(0), loading(false) {
}

AssetLoader::~AssetLoader() {
    unloadAll();
}

void AssetLoader::addTexture(const std::string& id, const std::string& path) {
    std::lock_guard<std::mutex> lock(loadMutex);
    
    AssetInfo info(id, path, AssetType::TEXTURE);
    assetMap[id] = assets.size();
    assets.push_back(info);
    totalCount = static_cast<int>(assets.size());
}

void AssetLoader::addSound(const std::string& id, const std::string& path) {
    std::lock_guard<std::mutex> lock(loadMutex);
    
    AssetInfo info(id, path, AssetType::SOUND);
    assetMap[id] = assets.size();
    assets.push_back(info);
    totalCount = static_cast<int>(assets.size());
}

void AssetLoader::addMusic(const std::string& id, const std::string& path) {
    std::lock_guard<std::mutex> lock(loadMutex);
    
    AssetInfo info(id, path, AssetType::MUSIC);
    assetMap[id] = assets.size();
    assets.push_back(info);
    totalCount = static_cast<int>(assets.size());
}

void AssetLoader::loadAll() {
    loading = true;
    loadedCount = 0;
    
    loadAllInternal();
    
    loading = false;
    if (completeCallback) {
        completeCallback();
    }
}

void AssetLoader::loadAllAsync() {
    loading = true;
    loadedCount = 0;
    
    std::thread loadThread([this]() {
        loadAllInternal();
        
        loading = false;
        if (completeCallback) {
            completeCallback();
        }
    });
    
    loadThread.detach();
}

void AssetLoader::loadGroup(const std::vector<std::string>& ids) {
    for (const auto& id : ids) {
        auto it = assetMap.find(id);
        if (it != assetMap.end()) {
            loadAsset(it->second);
        }
    }
}

void AssetLoader::unloadAll() {
    std::lock_guard<std::mutex> lock(loadMutex);
    
    for (auto& asset : assets) {
        if (asset.loaded) {
            switch (asset.type) {
                case AssetType::TEXTURE:
                    if (resourceManager) {
                        resourceManager->unloadTexture(asset.id);
                    }
                    break;
                case AssetType::SOUND:
                    if (audioManager) {
                        audioManager->unloadSoundEffect(asset.id);
                    }
                    break;
                case AssetType::MUSIC:
                    if (audioManager) {
                        audioManager->unloadMusic(asset.id);
                    }
                    break;
                default:
                    break;
            }
            asset.loaded = false;
        }
    }
    
    loadedCount = 0;
}

void AssetLoader::unloadGroup(const std::vector<std::string>& ids) {
    std::lock_guard<std::mutex> lock(loadMutex);
    
    for (const auto& id : ids) {
        auto it = assetMap.find(id);
        if (it != assetMap.end() && assets[it->second].loaded) {
            auto& asset = assets[it->second];
            
            switch (asset.type) {
                case AssetType::TEXTURE:
                    if (resourceManager) {
                        resourceManager->unloadTexture(asset.id);
                    }
                    break;
                case AssetType::SOUND:
                    if (audioManager) {
                        audioManager->unloadSoundEffect(asset.id);
                    }
                    break;
                case AssetType::MUSIC:
                    if (audioManager) {
                        audioManager->unloadMusic(asset.id);
                    }
                    break;
                default:
                    break;
            }
            
            asset.loaded = false;
            loadedCount--;
        }
    }
}

bool AssetLoader::isLoaded(const std::string& id) const {
    auto it = assetMap.find(id);
    if (it != assetMap.end()) {
        return assets[it->second].loaded;
    }
    return false;
}

float AssetLoader::getProgress() const {
    if (totalCount == 0) return 1.0f;
    return static_cast<float>(loadedCount) / static_cast<float>(totalCount);
}

void AssetLoader::setProgressCallback(ProgressCallback callback) {
    progressCallback = callback;
}

void AssetLoader::setCompleteCallback(CompleteCallback callback) {
    completeCallback = callback;
}

void AssetLoader::loadAsset(size_t index) {
    if (index >= assets.size()) return;
    
    auto& asset = assets[index];
    if (asset.loaded) return;
    
    bool success = false;
    
    switch (asset.type) {
        case AssetType::TEXTURE:
            if (resourceManager) {
                auto texture = resourceManager->loadTexture(asset.id, asset.path);
                success = (texture != nullptr);
            }
            break;
        case AssetType::SOUND:
            if (audioManager) {
                success = audioManager->loadSoundEffect(asset.id, asset.path);
            }
            break;
        case AssetType::MUSIC:
            if (audioManager) {
                success = audioManager->loadMusic(asset.id, asset.path);
            }
            break;
        default:
            break;
    }
    
    if (success) {
        asset.loaded = true;
        loadedCount++;
        
        if (progressCallback) {
            progressCallback(loadedCount, totalCount, asset.id);
        }
    }
}

void AssetLoader::loadAllInternal() {
    for (size_t i = 0; i < assets.size(); i++) {
        loadAsset(i);
    }
}

} // namespace Core
} // namespace JJM
