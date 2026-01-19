#ifndef JJM_SHADER_HOT_RELOAD_H
#define JJM_SHADER_HOT_RELOAD_H

#include <string>
#include <map>
#include <functional>
#include <chrono>

namespace JJM {
namespace Graphics {

class Shader;

struct ShaderFile {
    std::string path;
    std::chrono::system_clock::time_point lastModified;
    unsigned int shaderId;
    std::vector<std::string> dependencies;  // Include files this shader depends on
};

class ShaderHotReload {
public:
    static ShaderHotReload& getInstance();
    
    void watch(const std::string& path, unsigned int shaderId);
    void unwatch(const std::string& path);
    void update();
    
    // Dependency tracking
    void addDependency(const std::string& shaderPath, const std::string& dependencyPath);
    void clearDependencies(const std::string& shaderPath);
    std::vector<std::string> getDependencies(const std::string& shaderPath) const;
    
    // Batch operations
    void pauseWatching();
    void resumeWatching();
    bool isWatching() const;
    
    using ReloadCallback = std::function<void(const std::string&, unsigned int)>;
    void setReloadCallback(ReloadCallback callback);

private:
    ShaderHotReload();
    ~ShaderHotReload();
    
    bool checkModified(const std::string& path, std::chrono::system_clock::time_point& outTime);
    void reloadShader(const std::string& path, unsigned int shaderId);
    void checkDependencies(const std::string& changedPath);
    
    std::map<std::string, ShaderFile> watchedFiles;
    std::map<std::string, std::chrono::system_clock::time_point> dependencyCache;
    ReloadCallback reloadCallback;
    bool isPaused;
};

} // namespace Graphics
} // namespace JJM

#endif
