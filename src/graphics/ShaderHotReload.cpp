#include "graphics/ShaderHotReload.h"
#include <sys/stat.h>
#include <iostream>

namespace JJM {
namespace Graphics {

ShaderHotReload& ShaderHotReload::getInstance() {
    static ShaderHotReload instance;
    return instance;
}

ShaderHotReload::ShaderHotReload() {
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
}

void ShaderHotReload::update() {
    for (auto& pair : watchedFiles) {
        std::chrono::system_clock::time_point currentModTime;
        if (checkModified(pair.first, currentModTime)) {
            if (currentModTime > pair.second.lastModified) {
                std::cout << "Shader modified: " << pair.first << std::endl;
                reloadShader(pair.first, pair.second.shaderId);
                pair.second.lastModified = currentModTime;
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
