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
};

class ShaderHotReload {
public:
    static ShaderHotReload& getInstance();
    
    void watch(const std::string& path, unsigned int shaderId);
    void unwatch(const std::string& path);
    void update();
    
    using ReloadCallback = std::function<void(const std::string&, unsigned int)>;
    void setReloadCallback(ReloadCallback callback);

private:
    ShaderHotReload();
    ~ShaderHotReload();
    
    bool checkModified(const std::string& path, std::chrono::system_clock::time_point& outTime);
    void reloadShader(const std::string& path, unsigned int shaderId);
    
    std::map<std::string, ShaderFile> watchedFiles;
    ReloadCallback reloadCallback;
};

} // namespace Graphics
} // namespace JJM

#endif
