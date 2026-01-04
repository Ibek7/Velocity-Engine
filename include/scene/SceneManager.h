#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "scene/Scene.h"
#include <memory>
#include <unordered_map>
#include <stack>
#include <future>
#include <functional>
#include <queue>
#include <mutex>

namespace JJM {
namespace Scene {

/**
 * @brief Scene loading state
 */
enum class SceneLoadState {
    NotLoaded,
    Loading,
    Loaded,
    Unloading,
    Failed
};

/**
 * @brief Scene load request for async loading
 */
struct SceneLoadRequest {
    std::string sceneName;
    std::string filePath;
    bool preload;
    int priority;
    std::function<void(bool)> callback;
    
    SceneLoadRequest()
        : preload(false)
        , priority(0)
    {}
};

/**
 * @brief Scene loading progress
 */
struct SceneLoadProgress {
    std::string sceneName;
    float progress;           // 0-1
    std::string currentTask;
    bool isComplete;
    bool hasError;
    std::string errorMessage;
    
    SceneLoadProgress()
        : progress(0.0f)
        , isComplete(false)
        , hasError(false)
    {}
};

/**
 * @brief Scene cache entry
 */
struct SceneCacheEntry {
    std::shared_ptr<Scene> scene;
    SceneLoadState state;
    std::chrono::steady_clock::time_point lastAccess;
    size_t memoryUsage;
    bool pinned;
    
    SceneCacheEntry()
        : state(SceneLoadState::NotLoaded)
        , memoryUsage(0)
        , pinned(false)
    {}
};

class SceneManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
    std::stack<std::shared_ptr<Scene>> sceneStack;
    std::shared_ptr<Scene> nextScene;
    bool shouldChange;
    
    // Async loading
    std::queue<SceneLoadRequest> loadQueue;
    std::unordered_map<std::string, SceneCacheEntry> sceneCache;
    std::unordered_map<std::string, SceneLoadProgress> loadProgress;
    std::mutex loadMutex;
    std::vector<std::future<void>> loadFutures;
    size_t maxConcurrentLoads;
    
    // Memory management
    size_t maxCacheSize;
    size_t currentCacheUsage;
    
    // Callbacks
    std::function<void(const std::string&)> onSceneLoadStart;
    std::function<void(const std::string&, bool)> onSceneLoadComplete;
    std::function<void(const std::string&, float)> onSceneLoadProgress;

public:
    SceneManager();
    ~SceneManager();

    // Scene management
    void addScene(const std::string& name, std::shared_ptr<Scene> scene);
    void removeScene(const std::string& name);
    void changeScene(const std::string& name);
    void pushScene(const std::string& name);
    void popScene();

    // Update and render
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);
    void handleInput(Input::InputManager* input);

    // Getters
    Scene* getCurrentScene();
    bool hasScene(const std::string& name) const;
    
    // Async scene loading
    void loadSceneAsync(const std::string& name, const std::string& filePath,
                       std::function<void(bool)> callback = nullptr);
    void loadSceneAsync(const SceneLoadRequest& request);
    void cancelSceneLoad(const std::string& name);
    void cancelAllLoads();
    
    // Scene preloading
    void preloadScene(const std::string& name, const std::string& filePath);
    void preloadScenes(const std::vector<std::pair<std::string, std::string>>& scenes);
    bool isScenePreloaded(const std::string& name) const;
    void unpreloadScene(const std::string& name);
    void unpreloadAllScenes();
    
    // Loading progress
    SceneLoadProgress getLoadProgress(const std::string& name) const;
    float getOverallLoadProgress() const;
    bool isLoading() const;
    bool isLoading(const std::string& name) const;
    SceneLoadState getSceneState(const std::string& name) const;
    
    // Cache management
    void setCacheSize(size_t maxBytes);
    size_t getCacheSize() const { return maxCacheSize; }
    size_t getCacheUsage() const { return currentCacheUsage; }
    void clearCache();
    void pinScene(const std::string& name);
    void unpinScene(const std::string& name);
    void evictLRU(size_t bytesToFree);
    
    // Scene dependencies
    void addSceneDependency(const std::string& scene, const std::string& dependency);
    void loadSceneWithDependencies(const std::string& name, const std::string& filePath);
    std::vector<std::string> getSceneDependencies(const std::string& name) const;
    
    // Loading callbacks
    void setOnSceneLoadStart(std::function<void(const std::string&)> callback);
    void setOnSceneLoadComplete(std::function<void(const std::string&, bool)> callback);
    void setOnSceneLoadProgress(std::function<void(const std::string&, float)> callback);
    
    // Configuration
    void setMaxConcurrentLoads(size_t maxLoads) { maxConcurrentLoads = maxLoads; }
    size_t getMaxConcurrentLoads() const { return maxConcurrentLoads; }

private:
    void performSceneChange();
    void processLoadQueue();
    void loadSceneInternal(const SceneLoadRequest& request);
    void updateLoadProgress(const std::string& name, float progress, const std::string& task);
    void cleanupCompletedLoads();
};

/**
 * @brief Scene loader interface for custom scene formats
 */
class ISceneLoader {
public:
    virtual ~ISceneLoader() = default;
    
    virtual bool canLoad(const std::string& filePath) const = 0;
    virtual std::shared_ptr<Scene> load(const std::string& filePath,
                                        std::function<void(float, const std::string&)> progressCallback = nullptr) = 0;
    virtual bool save(const Scene& scene, const std::string& filePath) = 0;
};

/**
 * @brief Scene loader registry
 */
class SceneLoaderRegistry {
public:
    static SceneLoaderRegistry& getInstance();
    
    void registerLoader(const std::string& format, std::unique_ptr<ISceneLoader> loader);
    void unregisterLoader(const std::string& format);
    ISceneLoader* getLoader(const std::string& filePath) const;
    ISceneLoader* getLoaderForFormat(const std::string& format) const;
    
    std::vector<std::string> getSupportedFormats() const;

private:
    SceneLoaderRegistry() = default;
    std::unordered_map<std::string, std::unique_ptr<ISceneLoader>> loaders;
};

} // namespace Scene
} // namespace JJM

#endif // SCENE_MANAGER_H
