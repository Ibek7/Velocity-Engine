#include "graphics/ShaderHotReload.h"
#include <sys/stat.h>
#include <iostream>

namespace JJM {
namespace Graphics {

ShaderHotReload& ShaderHotReload::getInstance() {
    static ShaderHotReload instance;
    return instance;
}

ShaderHotReload::ShaderHotReload() : isPaused(false) {
}

ShaderHotReload::~ShaderHotReload() {
}

void ShaderHotReload::watch(const std::string& path, unsigned int shaderId) {
    std::chrono::system_clock::time_point modTime;
    if (checkModified(path, modTime)) {
        ShaderFile file;
        file.path = path;
        file.lastModified = modTime;
        file.shaderId = shaderId;
        watchedFiles[path] = file;
    }
}

void ShaderHotReload::unwatch(const std::string& path) {
    watchedFiles.erase(path);
    dependencyCache.erase(path);
}

void ShaderHotReload::update() {
    if (isPaused) {
        return;
    }
    
    for (auto& pair : watchedFiles) {
        std::chrono::system_clock::time_point currentModTime;
        if (checkModified(pair.first, currentModTime)) {
            if (currentModTime > pair.second.lastModified) {
                std::cout << "Shader modified: " << pair.first << std::endl;
                reloadShader(pair.first, pair.second.shaderId);
                pair.second.lastModified = currentModTime;
                
                // Check if any other shaders depend on this file
                checkDependencies(pair.first);
            }
        }
        
        // Check dependencies
        for (const auto& dep : pair.second.dependencies) {
            std::chrono::system_clock::time_point depModTime;
            if (checkModified(dep, depModTime)) {
                auto it = dependencyCache.find(dep);
                if (it == dependencyCache.end() || depModTime > it->second) {
                    std::cout << "Shader dependency modified: " << dep << " affecting " << pair.first << std::endl;
                    reloadShader(pair.first, pair.second.shaderId);
                    pair.second.lastModified = std::chrono::system_clock::now();
                    dependencyCache[dep] = depModTime;
                }
            }
        }
    }
}

void ShaderHotReload::addDependency(const std::string& shaderPath, const std::string& dependencyPath) {
    auto it = watchedFiles.find(shaderPath);
    if (it != watchedFiles.end()) {
        it->second.dependencies.push_back(dependencyPath);
        
        // Cache the dependency's modification time
        std::chrono::system_clock::time_point modTime;
        if (checkModified(dependencyPath, modTime)) {
            dependencyCache[dependencyPath] = modTime;
        }
    }
}

void ShaderHotReload::clearDependencies(const std::string& shaderPath) {
    auto it = watchedFiles.find(shaderPath);
    if (it != watchedFiles.end()) {
        it->second.dependencies.clear();
    }
}

std::vector<std::string> ShaderHotReload::getDependencies(const std::string& shaderPath) const {
    auto it = watchedFiles.find(shaderPath);
    if (it != watchedFiles.end()) {
        return it->second.dependencies;
    }
    return {};
}

void ShaderHotReload::pauseWatching() {
    isPaused = true;
}

void ShaderHotReload::resumeWatching() {
    isPaused = false;
}

bool ShaderHotReload::isWatching() const {
    return !isPaused;
}

void ShaderHotReload::checkDependencies(const std::string& changedPath) {
    // Find all shaders that depend on the changed file
    for (auto& pair : watchedFiles) {
        for (const auto& dep : pair.second.dependencies) {
            if (dep == changedPath) {
                std::cout << "Reloading " << pair.first << " due to dependency change: " << changedPath << std::endl;
                reloadShader(pair.first, pair.second.shaderId);
                pair.second.lastModified = std::chrono::system_clock::now();
                break;
            }
        }
    }
}

void ShaderHotReload::setReloadCallback(ReloadCallback callback) {
    reloadCallback = callback;
}

bool ShaderHotReload::checkModified(const std::string& path, 
                                   std::chrono::system_clock::time_point& outTime) {
    struct stat result;
    if (stat(path.c_str(), &result) == 0) {
        outTime = std::chrono::system_clock::from_time_t(result.st_mtime);
        return true;
    }
    return false;
}

void ShaderHotReload::reloadShader(const std::string& path, unsigned int shaderId) {
    if (reloadCallback) {
        reloadCallback(path, shaderId);
    }
}

} // namespace Graphics
} // namespace JJM
