#include "plugin/PluginLoader.h"
#include <dlfcn.h>
#include <algorithm>

namespace JJM {
namespace Plugin {

PluginLoader::PluginLoader() : pluginDirectory("./plugins") {}

PluginLoader::~PluginLoader() {
    unloadAllPlugins();
}

bool PluginLoader::loadPlugin(const std::string& path) {
    std::string fullPath = path;
    if (path.find('/') == std::string::npos) {
        fullPath = pluginDirectory + "/" + path;
    }
    
    void* handle = loadLibrary(fullPath);
    if (!handle) {
        return false;
    }
    
    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(getSymbol(handle, "createPlugin"));
    if (!createFunc) {
        unloadLibrary(handle);
        return false;
    }
    
    IPlugin* plugin = createFunc();
    if (!plugin) {
        unloadLibrary(handle);
        return false;
    }
    
    if (!plugin->initialize()) {
        DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(getSymbol(handle, "destroyPlugin"));
        if (destroyFunc) {
            destroyFunc(plugin);
        }
        unloadLibrary(handle);
        return false;
    }
    
    PluginInfo info;
    info.name = plugin->getName();
    info.version = plugin->getVersion();
    info.author = plugin->getAuthor();
    info.path = fullPath;
    info.handle = handle;
    info.instance = plugin;
    info.loaded = true;
    
    plugins.push_back(info);
    
    return true;
}

bool PluginLoader::unloadPlugin(const std::string& name) {
    PluginInfo* info = findPlugin(name);
    if (!info) return false;
    
    if (info->instance) {
        info->instance->shutdown();
        
        DestroyPluginFunc destroyFunc = reinterpret_cast<DestroyPluginFunc>(
            getSymbol(info->handle, "destroyPlugin"));
        if (destroyFunc) {
            destroyFunc(info->instance);
        }
        info->instance = nullptr;
    }
    
    if (info->handle) {
        unloadLibrary(info->handle);
        info->handle = nullptr;
    }
    
    auto it = std::find_if(plugins.begin(), plugins.end(),
        [&name](const PluginInfo& p) { return p.name == name; });
    
    if (it != plugins.end()) {
        plugins.erase(it);
    }
    
    return true;
}

void PluginLoader::unloadAllPlugins() {
    while (!plugins.empty()) {
        unloadPlugin(plugins.front().name);
    }
}

bool PluginLoader::reloadPlugin(const std::string& name) {
    PluginInfo* info = findPlugin(name);
    if (!info) return false;
    
    std::string path = info->path;
    
    if (!unloadPlugin(name)) return false;
    
    return loadPlugin(path);
}

IPlugin* PluginLoader::getPlugin(const std::string& name) {
    PluginInfo* info = findPlugin(name);
    return info ? info->instance : nullptr;
}

const PluginInfo* PluginLoader::getPluginInfo(const std::string& name) const {
    auto it = std::find_if(plugins.begin(), plugins.end(),
        [&name](const PluginInfo& p) { return p.name == name; });
    
    return (it != plugins.end()) ? &(*it) : nullptr;
}

std::vector<std::string> PluginLoader::getLoadedPlugins() const {
    std::vector<std::string> names;
    for (const auto& plugin : plugins) {
        names.push_back(plugin.name);
    }
    return names;
}

void PluginLoader::updateAllPlugins(float deltaTime) {
    for (auto& plugin : plugins) {
        if (plugin.loaded && plugin.instance) {
            plugin.instance->update(deltaTime);
        }
    }
}

void* PluginLoader::loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
}

void PluginLoader::unloadLibrary(void* handle) {
    if (handle) {
        dlclose(handle);
    }
}

void* PluginLoader::getSymbol(void* handle, const std::string& name) {
    if (!handle) return nullptr;
    return dlsym(handle, name.c_str());
}

PluginInfo* PluginLoader::findPlugin(const std::string& name) {
    auto it = std::find_if(plugins.begin(), plugins.end(),
        [&name](const PluginInfo& p) { return p.name == name; });
    
    return (it != plugins.end()) ? &(*it) : nullptr;
}

} // namespace Plugin
} // namespace JJM
