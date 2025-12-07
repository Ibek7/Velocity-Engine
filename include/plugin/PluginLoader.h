#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace JJM {
namespace Plugin {

class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    virtual const char* getName() const = 0;
    virtual const char* getVersion() const = 0;
    virtual const char* getAuthor() const = 0;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) = 0;
};

struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string path;
    void* handle;
    IPlugin* instance;
    bool loaded;
};

class PluginLoader {
public:
    PluginLoader();
    ~PluginLoader();
    
    bool loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& name);
    void unloadAllPlugins();
    
    bool reloadPlugin(const std::string& name);
    
    IPlugin* getPlugin(const std::string& name);
    const PluginInfo* getPluginInfo(const std::string& name) const;
    
    std::vector<std::string> getLoadedPlugins() const;
    
    void updateAllPlugins(float deltaTime);
    
    void setPluginDirectory(const std::string& directory) { pluginDirectory = directory; }
    const std::string& getPluginDirectory() const { return pluginDirectory; }

private:
    std::vector<PluginInfo> plugins;
    std::string pluginDirectory;
    
    void* loadLibrary(const std::string& path);
    void unloadLibrary(void* handle);
    void* getSymbol(void* handle, const std::string& name);
    
    PluginInfo* findPlugin(const std::string& name);
};

using CreatePluginFunc = IPlugin* (*)();
using DestroyPluginFunc = void (*)(IPlugin*);

#define EXPORT_PLUGIN(PluginClass) \
    extern "C" { \
        IPlugin* createPlugin() { return new PluginClass(); } \
        void destroyPlugin(IPlugin* plugin) { delete plugin; } \
    }

} // namespace Plugin
} // namespace JJM
