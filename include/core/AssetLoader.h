#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include "core/ResourceManager.h"
#include "graphics/Texture.h"
#include "audio/AudioManager.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace JJM {
namespace Core {

enum class AssetType {
    TEXTURE,
    SOUND,
    MUSIC,
    FONT,
    DATA
};

struct AssetInfo {
    std::string id;
    std::string path;
    AssetType type;
    bool loaded;
    
    AssetInfo() : loaded(false) {}
    AssetInfo(const std::string& id, const std::string& path, AssetType type)
        : id(id), path(path), type(type), loaded(false) {}
};

class AssetLoader {
private:
    ResourceManager* resourceManager;
    Audio::AudioManager* audioManager;
    Graphics::Renderer* renderer;
    
    std::vector<AssetInfo> assets;
    std::map<std::string, size_t> assetMap;
    
    std::atomic<int> loadedCount;
    std::atomic<int> totalCount;
    std::atomic<bool> loading;
    std::mutex loadMutex;
    
    using ProgressCallback = std::function<void(int, int, const std::string&)>;
    ProgressCallback progressCallback;
    
    using CompleteCallback = std::function<void()>;
    CompleteCallback completeCallback;
    
public:
    AssetLoader(ResourceManager* rm, Audio::AudioManager* am, Graphics::Renderer* r);
    ~AssetLoader();
    
    void addTexture(const std::string& id, const std::string& path);
    void addSound(const std::string& id, const std::string& path);
    void addMusic(const std::string& id, const std::string& path);
    
    void loadAll();
    void loadAllAsync();
    void loadGroup(const std::vector<std::string>& ids);
    
    void unloadAll();
    void unloadGroup(const std::vector<std::string>& ids);
    
    bool isLoaded(const std::string& id) const;
    bool isLoading() const { return loading; }
    float getProgress() const;
    
    void setProgressCallback(ProgressCallback callback);
    void setCompleteCallback(CompleteCallback callback);
    
    int getLoadedCount() const { return loadedCount; }
    int getTotalCount() const { return totalCount; }
    
private:
    void loadAsset(size_t index);
    void loadAllInternal();
};

} // namespace Core
} // namespace JJM

#endif // ASSET_LOADER_H
