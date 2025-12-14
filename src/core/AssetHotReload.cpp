#include "core/AssetHotReload.h"
#include <fstream>
#include <sys/stat.h>
#include <algorithm>

namespace JJM {
namespace Core {

// Asset implementation
Asset::Asset(const std::string& path, AssetType type)
    : path(path), type(type), loaded(false) {}

Asset::~Asset() {}

// AssetWatcher implementation
AssetWatcher::AssetWatcher() {}

AssetWatcher::~AssetWatcher() {}

void AssetWatcher::watch(const std::string& path) {
    WatchedFile file(path);
    file.lastModified = getFileModificationTime(path);
    watchedFiles[path] = file;
}

void AssetWatcher::unwatch(const std::string& path) {
    watchedFiles.erase(path);
}

void AssetWatcher::update() {
    for (auto& pair : watchedFiles) {
        auto currentTime = getFileModificationTime(pair.first);
        
        if (currentTime > pair.second.lastModified) {
            pair.second.lastModified = currentTime;
            
            if (onFileChanged) {
                onFileChanged(pair.first);
            }
        }
    }
}

bool AssetWatcher::hasFileChanged(const std::string& path) {
    auto it = watchedFiles.find(path);
    if (it == watchedFiles.end()) {
        return false;
    }
    
    auto currentTime = getFileModificationTime(path);
    return currentTime > it->second.lastModified;
}

std::chrono::system_clock::time_point AssetWatcher::getFileModificationTime(const std::string& path) {
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) == 0) {
        return std::chrono::system_clock::from_time_t(fileInfo.st_mtime);
    }
    return std::chrono::system_clock::now();
}

// AssetHotReloader implementation
AssetHotReloader::AssetHotReloader()
    : enabled(true), checkInterval(1.0f), timeSinceLastCheck(0.0f) {}

AssetHotReloader::~AssetHotReloader() {}

AssetHotReloader& AssetHotReloader::getInstance() {
    static AssetHotReloader instance;
    return instance;
}

void AssetHotReloader::registerAsset(const std::string& path, std::shared_ptr<Asset> asset) {
    assets[path] = asset;
    watcher.watch(path);
}

void AssetHotReloader::unregisterAsset(const std::string& path) {
    assets.erase(path);
    watcher.unwatch(path);
}

void AssetHotReloader::update() {
    if (!enabled) {
        return;
    }
    
    timeSinceLastCheck += 0.016f; // Approximate frame time
    
    if (timeSinceLastCheck >= checkInterval) {
        timeSinceLastCheck = 0.0f;
        
        watcher.update();
        scanWatchDirectories();
    }
}

void AssetHotReloader::addWatchDirectory(const std::string& directory) {
    watchDirectories.push_back(directory);
}

void AssetHotReloader::removeWatchDirectory(const std::string& directory) {
    watchDirectories.erase(
        std::remove(watchDirectories.begin(), watchDirectories.end(), directory),
        watchDirectories.end()
    );
}

void AssetHotReloader::reloadAsset(const std::string& path) {
    auto it = assets.find(path);
    if (it != assets.end()) {
        if (it->second->reload()) {
            if (onAssetReloaded) {
                onAssetReloaded(path, it->second);
            }
        }
    }
}

void AssetHotReloader::scanWatchDirectories() {
    // Scan directories for file changes
    for (const auto& dir : watchDirectories) {
        // In a real implementation, this would recursively scan the directory
        // and check for modified files
    }
}

// TextureAsset implementation
TextureAsset::TextureAsset(const std::string& path)
    : Asset(path, AssetType::Texture), textureData(nullptr), 
      width(0), height(0) {
    loadFromFile();
}

TextureAsset::~TextureAsset() {
    if (textureData) {
        // Free texture data
        textureData = nullptr;
    }
}

bool TextureAsset::reload() {
    if (textureData) {
        textureData = nullptr;
    }
    return loadFromFile();
}

bool TextureAsset::loadFromFile() {
    // In a real implementation, this would load the texture using SDL_Image or similar
    // For now, just mark as loaded
    loaded = true;
    return true;
}

// ScriptAsset implementation
ScriptAsset::ScriptAsset(const std::string& path)
    : Asset(path, AssetType::Script) {
    loadFromFile();
}

ScriptAsset::~ScriptAsset() {}

bool ScriptAsset::reload() {
    source.clear();
    return loadFromFile();
}

bool ScriptAsset::loadFromFile() {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    source.assign((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
    
    loaded = true;
    return true;
}

// DataAsset implementation
DataAsset::DataAsset(const std::string& path)
    : Asset(path, AssetType::Data) {
    loadFromFile();
}

DataAsset::~DataAsset() {}

bool DataAsset::reload() {
    data.clear();
    return loadFromFile();
}

bool DataAsset::loadFromFile() {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    loaded = true;
    return true;
}

// AssetDependencyGraph implementation
AssetDependencyGraph::AssetDependencyGraph() {}

AssetDependencyGraph::~AssetDependencyGraph() {}

void AssetDependencyGraph::addDependency(const std::string& asset, const std::string& dependency) {
    dependencies[asset].push_back(dependency);
    dependents[dependency].push_back(asset);
}

void AssetDependencyGraph::removeDependency(const std::string& asset, const std::string& dependency) {
    auto& deps = dependencies[asset];
    deps.erase(std::remove(deps.begin(), deps.end(), dependency), deps.end());
    
    auto& depts = dependents[dependency];
    depts.erase(std::remove(depts.begin(), depts.end(), asset), depts.end());
}

std::vector<std::string> AssetDependencyGraph::getDependencies(const std::string& asset) const {
    auto it = dependencies.find(asset);
    if (it != dependencies.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> AssetDependencyGraph::getDependents(const std::string& asset) const {
    auto it = dependents.find(asset);
    if (it != dependents.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string> AssetDependencyGraph::getReloadOrder(const std::string& asset) const {
    std::vector<std::string> result;
    std::unordered_map<std::string, bool> visited;
    
    topologicalSort(asset, visited, result);
    
    return result;
}

void AssetDependencyGraph::clear() {
    dependencies.clear();
    dependents.clear();
}

void AssetDependencyGraph::topologicalSort(const std::string& asset,
                                          std::unordered_map<std::string, bool>& visited,
                                          std::vector<std::string>& result) const {
    if (visited[asset]) {
        return;
    }
    
    visited[asset] = true;
    
    auto deps = getDependencies(asset);
    for (const auto& dep : deps) {
        topologicalSort(dep, visited, result);
    }
    
    result.push_back(asset);
}

} // namespace Core
} // namespace JJM
