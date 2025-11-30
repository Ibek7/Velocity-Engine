#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <any>
#include <typeindex>

namespace JJM {
namespace Plugin {

// Forward declarations
class IPlugin;
class PluginManager;
class PluginAPI;

// Plugin version structure
struct PluginVersion {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    
    PluginVersion(uint32_t maj = 0, uint32_t min = 0, uint32_t p = 0)
        : major(maj), minor(min), patch(p) {}
    
    bool isCompatible(const PluginVersion& required) const;
    std::string toString() const;
    
    bool operator<(const PluginVersion& other) const;
    bool operator==(const PluginVersion& other) const;
};

// Plugin metadata
struct PluginMetadata {
    std::string name;
    std::string description;
    std::string author;
    PluginVersion version;
    PluginVersion engineVersionRequired;
    std::vector<std::string> dependencies;
    std::unordered_map<std::string, std::string> customProperties;
    
    // Security information
    std::string signature;
    std::string hash;
    bool trusted;
    
    PluginMetadata()
        : version(), engineVersionRequired(), trusted(false) {}
};

// Plugin state enumeration
enum class PluginState {
    Unloaded,
    Loading,
    Loaded,
    Initializing,
    Ready,
    Error,
    Unloading
};

// Plugin type enumeration
enum class PluginType {
    Gameplay,       // Game logic plugins
    Rendering,      // Graphics and rendering
    Audio,          // Audio processing
    Input,          // Input handling
    Network,        // Network functionality
    Tools,          // Development tools
    Custom          // User-defined
};

// Plugin API interface for exposing engine functionality
class PluginAPI {
public:
    virtual ~PluginAPI() = default;
    
    // Core engine access
    template<typename T>
    T* getSystem();
    
    template<typename T>
    void registerService(const std::string& name, T* service);
    
    // Event system integration
    template<typename EventType>
    void subscribeToEvent(std::function<void(const EventType&)> callback);
    
    template<typename EventType>
    void publishEvent(const EventType& event);
    
    // Resource management
    bool loadResource(const std::string& path, const std::string& type);
    void unloadResource(const std::string& path);
    
    // Configuration access
    std::any getConfig(const std::string& key, const std::any& defaultValue = {});
    void setConfig(const std::string& key, const std::any& value);
    
    // Logging
    void logInfo(const std::string& message);
    void logWarning(const std::string& message);
    void logError(const std::string& message);
    
    // Utility functions
    std::string getPluginDataPath(const std::string& pluginName) const;
    std::chrono::milliseconds getGameTime() const;
    
private:
    std::unordered_map<std::type_index, std::any> systems;
    std::unordered_map<std::string, std::any> services;
    std::unordered_map<std::string, std::any> config;
};

// Base plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    // Plugin lifecycle
    virtual bool initialize(PluginAPI* api) = 0;
    virtual void shutdown() = 0;
    virtual void update(float deltaTime) {}
    
    // Plugin information
    virtual PluginMetadata getMetadata() const = 0;
    virtual PluginType getType() const = 0;
    
    // Hot-reload support
    virtual void onBeforeReload() {}
    virtual void onAfterReload() {}
    
    // Dependency management
    virtual std::vector<std::string> getDependencies() const { return {}; }
    virtual bool checkCompatibility(const PluginVersion& engineVersion) const;
    
    // Configuration
    virtual void loadConfiguration(const std::string& configPath) {}
    virtual void saveConfiguration(const std::string& configPath) {}
};

// Plugin loader interface
class IPluginLoader {
public:
    virtual ~IPluginLoader() = default;
    
    virtual std::shared_ptr<IPlugin> loadPlugin(const std::filesystem::path& path) = 0;
    virtual void unloadPlugin(std::shared_ptr<IPlugin> plugin) = 0;
    virtual bool canLoad(const std::filesystem::path& path) const = 0;
    virtual std::vector<std::string> getSupportedExtensions() const = 0;
};

// Dynamic library loader implementation
class DynamicLibraryLoader : public IPluginLoader {
public:
    DynamicLibraryLoader();
    ~DynamicLibraryLoader();
    
    std::shared_ptr<IPlugin> loadPlugin(const std::filesystem::path& path) override;
    void unloadPlugin(std::shared_ptr<IPlugin> plugin) override;
    bool canLoad(const std::filesystem::path& path) const override;
    std::vector<std::string> getSupportedExtensions() const override;
    
private:
    struct LibraryHandle {
        void* handle;
        std::filesystem::path path;
        std::shared_ptr<IPlugin> plugin;
    };
    
    std::unordered_map<std::string, LibraryHandle> loadedLibraries;
    std::mutex libraryMutex;
};

// Plugin container - manages a single plugin instance
class PluginContainer {
public:
    PluginContainer(const std::string& name, std::shared_ptr<IPlugin> plugin);
    ~PluginContainer();
    
    // Plugin management
    bool initialize(PluginAPI* api);
    void shutdown();
    void update(float deltaTime);
    
    // State management
    PluginState getState() const { return state; }
    bool isReady() const { return state == PluginState::Ready; }
    bool hasError() const { return state == PluginState::Error; }
    
    // Metadata access
    const PluginMetadata& getMetadata() const;
    PluginType getType() const;
    
    // Hot-reload support
    bool reload(std::shared_ptr<IPlugin> newPlugin);
    void setReloadCallback(std::function<void(const std::string&)> callback);
    
    // Dependency management
    void addDependency(const std::string& dependencyName);
    void removeDependency(const std::string& dependencyName);
    const std::vector<std::string>& getDependencies() const;
    
    // Error handling
    const std::string& getLastError() const { return lastError; }
    void clearError();
    
    // Plugin access
    std::shared_ptr<IPlugin> getPlugin() const { return plugin; }
    const std::string& getName() const { return name; }
    
private:
    std::string name;
    std::shared_ptr<IPlugin> plugin;
    std::atomic<PluginState> state;
    std::string lastError;
    std::vector<std::string> dependencies;
    std::function<void(const std::string&)> reloadCallback;
    mutable std::mutex containerMutex;
};

// Security manager for plugin validation
class PluginSecurityManager {
public:
    PluginSecurityManager();
    ~PluginSecurityManager();
    
    // Security validation
    bool validatePlugin(const std::filesystem::path& path);
    bool verifySignature(const PluginMetadata& metadata, const std::string& pluginData);
    bool isTrusted(const std::string& pluginName) const;
    
    // Sandbox management
    void enableSandbox(const std::string& pluginName);
    void disableSandbox(const std::string& pluginName);
    bool isSandboxEnabled(const std::string& pluginName) const;
    
    // Permission management
    void grantPermission(const std::string& pluginName, const std::string& permission);
    void revokePermission(const std::string& pluginName, const std::string& permission);
    bool hasPermission(const std::string& pluginName, const std::string& permission) const;
    
    // Trusted plugin management
    void addTrustedPlugin(const std::string& pluginName, const std::string& signature);
    void removeTrustedPlugin(const std::string& pluginName);
    
private:
    std::unordered_map<std::string, bool> trustedPlugins;
    std::unordered_map<std::string, bool> sandboxEnabled;
    std::unordered_map<std::string, std::vector<std::string>> pluginPermissions;
    mutable std::mutex securityMutex;
    
    std::string calculateHash(const std::string& data) const;
    bool validateSignature(const std::string& data, const std::string& signature) const;
};

// Hot-reload watcher for development
class HotReloadWatcher {
public:
    HotReloadWatcher();
    ~HotReloadWatcher();
    
    // Watch management
    void startWatching(const std::filesystem::path& directory);
    void stopWatching();
    bool isWatching() const { return watching; }
    
    // Callbacks
    void setFileChangedCallback(std::function<void(const std::filesystem::path&)> callback);
    void setDirectoryChangedCallback(std::function<void(const std::filesystem::path&)> callback);
    
    // File filtering
    void addWatchExtension(const std::string& extension);
    void removeWatchExtension(const std::string& extension);
    void setWatchExtensions(const std::vector<std::string>& extensions);
    
private:
    std::atomic<bool> watching;
    std::thread watchThread;
    std::filesystem::path watchDirectory;
    std::vector<std::string> watchExtensions;
    std::function<void(const std::filesystem::path&)> fileChangedCallback;
    std::function<void(const std::filesystem::path&)> directoryChangedCallback;
    
    void watchLoop();
    bool shouldWatchFile(const std::filesystem::path& file) const;
};

// Main plugin manager class
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    // Initialization and cleanup
    bool initialize();
    void shutdown();
    
    // Plugin directory management
    void addPluginDirectory(const std::filesystem::path& directory);
    void removePluginDirectory(const std::filesystem::path& directory);
    void setPluginDirectories(const std::vector<std::filesystem::path>& directories);
    const std::vector<std::filesystem::path>& getPluginDirectories() const;
    
    // Plugin loading and management
    bool loadPlugin(const std::filesystem::path& path);
    bool loadPlugin(const std::string& name, const std::filesystem::path& path);
    bool unloadPlugin(const std::string& name);
    bool reloadPlugin(const std::string& name);
    
    // Plugin discovery
    std::vector<std::filesystem::path> discoverPlugins() const;
    bool loadAllPlugins();
    bool loadPluginsFromDirectory(const std::filesystem::path& directory);
    
    // Plugin access
    PluginContainer* getPlugin(const std::string& name);
    const PluginContainer* getPlugin(const std::string& name) const;
    std::vector<std::string> getLoadedPluginNames() const;
    std::vector<PluginContainer*> getPluginsByType(PluginType type);
    
    // Plugin state management
    bool isPluginLoaded(const std::string& name) const;
    PluginState getPluginState(const std::string& name) const;
    size_t getLoadedPluginCount() const;
    
    // Dependency management
    bool resolveDependencies();
    std::vector<std::string> getDependencyOrder() const;
    bool validateDependencies(const std::string& pluginName) const;
    
    // Update cycle
    void updatePlugins(float deltaTime);
    
    // Hot-reload support
    void enableHotReload(bool enable = true);
    bool isHotReloadEnabled() const { return hotReloadEnabled; }
    void setHotReloadDelay(std::chrono::milliseconds delay) { hotReloadDelay = delay; }
    
    // Event system
    void setPluginLoadedCallback(std::function<void(const std::string&)> callback);
    void setPluginUnloadedCallback(std::function<void(const std::string&)> callback);
    void setPluginErrorCallback(std::function<void(const std::string&, const std::string&)> callback);
    
    // Configuration
    void setConfiguration(const std::string& key, const std::any& value);
    std::any getConfiguration(const std::string& key, const std::any& defaultValue = {}) const;
    bool loadConfiguration(const std::filesystem::path& configPath);
    bool saveConfiguration(const std::filesystem::path& configPath) const;
    
    // Loader management
    void registerLoader(std::shared_ptr<IPluginLoader> loader);
    void unregisterLoader(std::shared_ptr<IPluginLoader> loader);
    
    // Security
    PluginSecurityManager& getSecurityManager() { return securityManager; }
    const PluginSecurityManager& getSecurityManager() const { return securityManager; }
    
    // API access
    PluginAPI& getAPI() { return api; }
    const PluginAPI& getAPI() const { return api; }
    
    // Statistics and diagnostics
    struct Statistics {
        size_t totalPluginsLoaded;
        size_t totalPluginsFailed;
        size_t totalReloads;
        std::chrono::milliseconds totalLoadTime;
        std::chrono::milliseconds averageLoadTime;
        std::unordered_map<PluginType, size_t> pluginsByType;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
private:
    // Core components
    PluginAPI api;
    PluginSecurityManager securityManager;
    HotReloadWatcher hotReloadWatcher;
    
    // Plugin storage
    std::unordered_map<std::string, std::unique_ptr<PluginContainer>> plugins;
    std::vector<std::filesystem::path> pluginDirectories;
    std::vector<std::shared_ptr<IPluginLoader>> loaders;
    
    // State management
    std::atomic<bool> initialized;
    std::atomic<bool> hotReloadEnabled;
    std::chrono::milliseconds hotReloadDelay;
    mutable std::mutex pluginsMutex;
    
    // Dependency resolution
    std::vector<std::string> dependencyOrder;
    std::unordered_map<std::string, std::vector<std::string>> dependencyGraph;
    
    // Callbacks
    std::function<void(const std::string&)> pluginLoadedCallback;
    std::function<void(const std::string&)> pluginUnloadedCallback;
    std::function<void(const std::string&, const std::string&)> pluginErrorCallback;
    
    // Configuration
    std::unordered_map<std::string, std::any> configuration;
    mutable std::mutex configMutex;
    mutable std::mutex loadersMutex;
    
    // Statistics
    mutable Statistics statistics;
    mutable std::mutex statsMutex;
    
    // Internal methods
    IPluginLoader* findCompatibleLoader(const std::filesystem::path& path) const;
    bool loadPluginInternal(const std::string& name, const std::filesystem::path& path);
    bool unloadPluginInternal(const std::string& name);
    
    void buildDependencyGraph();
    bool topologicalSort();
    bool hasCyclicDependencies() const;
    
    void onFileChanged(const std::filesystem::path& path);
    void onHotReload(const std::string& pluginName);
    
    void notifyPluginLoaded(const std::string& name);
    void notifyPluginUnloaded(const std::string& name);
    void notifyPluginError(const std::string& name, const std::string& error);
};

// Utility macros for plugin development
#define DECLARE_PLUGIN_INTERFACE(ClassName) \
    extern "C" { \
        JJM::Plugin::IPlugin* createPlugin(); \
        void destroyPlugin(JJM::Plugin::IPlugin* plugin); \
        const char* getPluginName(); \
        const char* getPluginVersion(); \
    }

#define IMPLEMENT_PLUGIN_INTERFACE(ClassName) \
    extern "C" { \
        JJM::Plugin::IPlugin* createPlugin() { return new ClassName(); } \
        void destroyPlugin(JJM::Plugin::IPlugin* plugin) { delete plugin; } \
        const char* getPluginName() { return #ClassName; } \
        const char* getPluginVersion() { return "1.0.0"; } \
    }

// Plugin factory for easier plugin creation
template<typename PluginType>
class PluginFactory {
public:
    static std::shared_ptr<PluginType> create() {
        static_assert(std::is_base_of_v<IPlugin, PluginType>, "PluginType must inherit from IPlugin");
        return std::make_shared<PluginType>();
    }
    
    static bool registerPlugin(PluginManager& manager, const std::string& name) {
        auto plugin = create();
        return manager.loadPlugin(name, std::filesystem::path{});  // Custom registration
    }
};

} // namespace Plugin
} // namespace JJM