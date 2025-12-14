#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <chrono>
#include <vector>

namespace JJM {
namespace Core {

enum class AssetType {
    Texture,
    Sound,
    Music,
    Font,
    Script,
    Data,
    Unknown
};

class Asset {
public:
    Asset(const std::string& path, AssetType type);
    virtual ~Asset();
    
    const std::string& getPath() const { return path; }
    AssetType getType() const { return type; }
    
    void setLastModified(std::chrono::system_clock::time_point time) { 
        lastModified = time; 
    }
    std::chrono::system_clock::time_point getLastModified() const { 
        return lastModified; 
    }
    
    virtual bool reload() = 0;
    
    void setLoaded(bool loaded) { this->loaded = loaded; }
    bool isLoaded() const { return loaded; }

protected:
    std::string path;
    AssetType type;
    std::chrono::system_clock::time_point lastModified;
    bool loaded;
};

class AssetWatcher {
public:
    AssetWatcher();
    ~AssetWatcher();
    
    void watch(const std::string& path);
    void unwatch(const std::string& path);
    
    void update();
    
    void setOnFileChanged(std::function<void(const std::string&)> callback) {
        onFileChanged = callback;
    }
    
    bool hasFileChanged(const std::string& path);

private:
    struct WatchedFile {
        std::string path;
        std::chrono::system_clock::time_point lastModified;
        
        WatchedFile() : path("") {}
        WatchedFile(const std::string& p) : path(p) {}
    };
    
    std::unordered_map<std::string, WatchedFile> watchedFiles;
    std::function<void(const std::string&)> onFileChanged;
    
    std::chrono::system_clock::time_point getFileModificationTime(const std::string& path);
};

class AssetHotReloader {
public:
    AssetHotReloader();
    ~AssetHotReloader();
    
    static AssetHotReloader& getInstance();
    
    void registerAsset(const std::string& path, std::shared_ptr<Asset> asset);
    void unregisterAsset(const std::string& path);
    
    void update();
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void setCheckInterval(float interval) { checkInterval = interval; }
    float getCheckInterval() const { return checkInterval; }
    
    void addWatchDirectory(const std::string& directory);
    void removeWatchDirectory(const std::string& directory);
    
    void setOnAssetReloaded(std::function<void(const std::string&, std::shared_ptr<Asset>)> callback) {
        onAssetReloaded = callback;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<Asset>> assets;
    AssetWatcher watcher;
    bool enabled;
    float checkInterval;
    float timeSinceLastCheck;
    std::vector<std::string> watchDirectories;
    
    std::function<void(const std::string&, std::shared_ptr<Asset>)> onAssetReloaded;
    
    void reloadAsset(const std::string& path);
    void scanWatchDirectories();
};

class TextureAsset : public Asset {
public:
    TextureAsset(const std::string& path);
    ~TextureAsset();
    
    bool reload() override;
    
    void* getTextureData() const { return textureData; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    void* textureData;
    int width;
    int height;
    
    bool loadFromFile();
};

class ScriptAsset : public Asset {
public:
    ScriptAsset(const std::string& path);
    ~ScriptAsset();
    
    bool reload() override;
    
    const std::string& getSource() const { return source; }

private:
    std::string source;
    
    bool loadFromFile();
};

class DataAsset : public Asset {
public:
    DataAsset(const std::string& path);
    ~DataAsset();
    
    bool reload() override;
    
    const std::vector<uint8_t>& getData() const { return data; }

private:
    std::vector<uint8_t> data;
    
    bool loadFromFile();
};

class AssetDependencyGraph {
public:
    AssetDependencyGraph();
    ~AssetDependencyGraph();
    
    void addDependency(const std::string& asset, const std::string& dependency);
    void removeDependency(const std::string& asset, const std::string& dependency);
    
    std::vector<std::string> getDependencies(const std::string& asset) const;
    std::vector<std::string> getDependents(const std::string& asset) const;
    
    std::vector<std::string> getReloadOrder(const std::string& asset) const;
    
    void clear();

private:
    std::unordered_map<std::string, std::vector<std::string>> dependencies;
    std::unordered_map<std::string, std::vector<std::string>> dependents;
    
    void topologicalSort(const std::string& asset, 
                        std::unordered_map<std::string, bool>& visited,
                        std::vector<std::string>& result) const;
};

} // namespace Core
} // namespace JJM
