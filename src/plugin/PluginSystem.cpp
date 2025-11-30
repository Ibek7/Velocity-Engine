#include "plugin/PluginSystem.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <dlfcn.h>
#include <filesystem>
#include <thread>
#include <queue>

namespace JJM {
namespace Plugin {

// -- PluginVersion implementation
bool PluginVersion::isCompatible(const PluginVersion& required) const {
    if (major != required.major) return major > required.major;
    if (minor != required.minor) return minor >= required.minor;
    return patch >= required.patch;
}

std::string PluginVersion::toString() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

bool PluginVersion::operator<(const PluginVersion& other) const {
    if (major != other.major) return major < other.major;
    if (minor != other.minor) return minor < other.minor;
    return patch < other.patch;
}

bool PluginVersion::operator==(const PluginVersion& other) const {
    return major == other.major && minor == other.minor && patch == other.patch;
}

// -- PluginAPI implementation
template<typename T>
T* PluginAPI::getSystem() {
    auto it = systems.find(std::type_index(typeid(T)));
    if (it != systems.end()) {
        return std::any_cast<T*>(it->second);
    }
    return nullptr;
}

template<typename T>
void PluginAPI::registerService(const std::string& name, T* service) {
    services[name] = service;
}

template<typename EventType>
void PluginAPI::subscribeToEvent(std::function<void(const EventType&)> callback) {
    // Event system integration would be implemented here
    // For now, this is a placeholder
}

template<typename EventType>
void PluginAPI::publishEvent(const EventType& event) {
    // Event system integration would be implemented here
    // For now, this is a placeholder
}

bool PluginAPI::loadResource(const std::string& path, const std::string& type) {
    // Resource loading would be integrated with the engine's asset loader
    return false;  // Placeholder
}

void PluginAPI::unloadResource(const std::string& path) {
    // Resource unloading implementation
}

std::any PluginAPI::getConfig(const std::string& key, const std::any& defaultValue) {
    auto it = config.find(key);
    return (it != config.end()) ? it->second : defaultValue;
}

void PluginAPI::setConfig(const std::string& key, const std::any& value) {
    config[key] = value;
}

void PluginAPI::logInfo(const std::string& message) {
    std::cout << "[PLUGIN INFO] " << message << std::endl;
}

void PluginAPI::logWarning(const std::string& message) {
    std::cout << "[PLUGIN WARNING] " << message << std::endl;
}

void PluginAPI::logError(const std::string& message) {
    std::cerr << "[PLUGIN ERROR] " << message << std::endl;
}

std::string PluginAPI::getPluginDataPath(const std::string& pluginName) const {
    return "./plugins/data/" + pluginName;
}

std::chrono::milliseconds PluginAPI::getGameTime() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    );
}

// -- IPlugin implementation
bool IPlugin::checkCompatibility(const PluginVersion& engineVersion) const {
    auto metadata = getMetadata();
    return engineVersion.isCompatible(metadata.engineVersionRequired);
}

// -- DynamicLibraryLoader implementation
DynamicLibraryLoader::DynamicLibraryLoader() = default;

DynamicLibraryLoader::~DynamicLibraryLoader() {
    std::lock_guard<std::mutex> lock(libraryMutex);
    for (auto& [name, handle] : loadedLibraries) {
        if (handle.handle) {
            dlclose(handle.handle);
        }
    }
    loadedLibraries.clear();
}

std::shared_ptr<IPlugin> DynamicLibraryLoader::loadPlugin(const std::filesystem::path& path) {
    std::lock_guard<std::mutex> lock(libraryMutex);
    
    // Check if already loaded
    std::string pathStr = path.string();
    auto it = loadedLibraries.find(pathStr);
    if (it != loadedLibraries.end()) {
        return it->second.plugin;
    }
    
    // Load the dynamic library
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << dlerror() << std::endl;
        return nullptr;
    }
    
    // Clear any existing errors
    dlerror();
    
    // Get the plugin creation function
    typedef IPlugin* (*CreatePluginFunc)();
    CreatePluginFunc createPlugin = (CreatePluginFunc) dlsym(handle, "createPlugin");
    
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        std::cerr << "Cannot load symbol 'createPlugin': " << dlsym_error << std::endl;
        dlclose(handle);
        return nullptr;
    }
    
    // Create the plugin instance
    IPlugin* pluginRawPtr = createPlugin();
    if (!pluginRawPtr) {
        std::cerr << "Failed to create plugin instance" << std::endl;
        dlclose(handle);
        return nullptr;
    }
    
    auto plugin = std::shared_ptr<IPlugin>(pluginRawPtr);
    
    // Store the handle for later cleanup
    LibraryHandle libHandle = { handle, path, plugin };
    loadedLibraries[pathStr] = libHandle;
    
    return plugin;
}

void DynamicLibraryLoader::unloadPlugin(std::shared_ptr<IPlugin> plugin) {
    std::lock_guard<std::mutex> lock(libraryMutex);
    
    for (auto it = loadedLibraries.begin(); it != loadedLibraries.end(); ++it) {
        if (it->second.plugin == plugin) {
            // Get the destroy function
            typedef void (*DestroyPluginFunc)(IPlugin*);
            DestroyPluginFunc destroyPlugin = (DestroyPluginFunc) dlsym(it->second.handle, "destroyPlugin");
            
            // Call destroy if available
            if (destroyPlugin) {
                destroyPlugin(plugin.get());
            }
            
            // Close the library
            dlclose(it->second.handle);
            
            // Remove from map
            loadedLibraries.erase(it);
            break;
        }
    }
}

bool DynamicLibraryLoader::canLoad(const std::filesystem::path& path) const {
    std::string extension = path.extension().string();
    return extension == ".so" || extension == ".dylib" || extension == ".dll";
}

std::vector<std::string> DynamicLibraryLoader::getSupportedExtensions() const {
#ifdef __APPLE__
    return {".dylib", ".so"};
#elif defined(_WIN32)
    return {".dll"};
#else
    return {".so"};
#endif
}

// -- PluginContainer implementation
PluginContainer::PluginContainer(const std::string& name, std::shared_ptr<IPlugin> plugin)
    : name(name), plugin(plugin), state(PluginState::Loaded) {}

PluginContainer::~PluginContainer() {
    if (state == PluginState::Ready) {
        shutdown();
    }
}

bool PluginContainer::initialize(PluginAPI* api) {
    std::lock_guard<std::mutex> lock(containerMutex);
    
    if (state != PluginState::Loaded) {
        lastError = "Plugin is not in loaded state";
        return false;
    }
    
    state = PluginState::Initializing;
    
    try {
        if (plugin->initialize(api)) {
            state = PluginState::Ready;
            lastError.clear();
            return true;
        } else {
            state = PluginState::Error;
            lastError = "Plugin initialization failed";
            return false;
        }
    } catch (const std::exception& e) {
        state = PluginState::Error;
        lastError = "Exception during initialization: " + std::string(e.what());
        return false;
    }
}

void PluginContainer::shutdown() {
    std::lock_guard<std::mutex> lock(containerMutex);
    
    if (state == PluginState::Ready) {
        state = PluginState::Unloading;
        try {
            plugin->shutdown();
        } catch (const std::exception& e) {
            lastError = "Exception during shutdown: " + std::string(e.what());
        }
    }
    
    state = PluginState::Unloaded;
}

void PluginContainer::update(float deltaTime) {
    std::lock_guard<std::mutex> lock(containerMutex);
    
    if (state == PluginState::Ready) {
        try {
            plugin->update(deltaTime);
        } catch (const std::exception& e) {
            state = PluginState::Error;
            lastError = "Exception during update: " + std::string(e.what());
        }
    }
}

const PluginMetadata& PluginContainer::getMetadata() const {
    if (plugin) {
        return plugin->getMetadata();
    }
    static PluginMetadata emptyMetadata;
    return emptyMetadata;
}

PluginType PluginContainer::getType() const {
    return plugin->getType();
}

bool PluginContainer::reload(std::shared_ptr<IPlugin> newPlugin) {
    std::lock_guard<std::mutex> lock(containerMutex);
    
    if (state == PluginState::Ready) {
        // Notify old plugin of reload
        try {
            plugin->onBeforeReload();
        } catch (const std::exception& e) {
            lastError = "Exception during before reload: " + std::string(e.what());
        }
        
        // Shutdown old plugin
        plugin->shutdown();
    }
    
    // Replace plugin
    plugin = newPlugin;
    state = PluginState::Loaded;
    
    // Notify new plugin
    try {
        plugin->onAfterReload();
    } catch (const std::exception& e) {
        lastError = "Exception during after reload: " + std::string(e.what());
    }
    
    // Trigger reload callback
    if (reloadCallback) {
        reloadCallback(name);
    }
    
    return true;
}

void PluginContainer::setReloadCallback(std::function<void(const std::string&)> callback) {
    reloadCallback = callback;
}

void PluginContainer::addDependency(const std::string& dependencyName) {
    auto it = std::find(dependencies.begin(), dependencies.end(), dependencyName);
    if (it == dependencies.end()) {
        dependencies.push_back(dependencyName);
    }
}

void PluginContainer::removeDependency(const std::string& dependencyName) {
    auto it = std::find(dependencies.begin(), dependencies.end(), dependencyName);
    if (it != dependencies.end()) {
        dependencies.erase(it);
    }
}

const std::vector<std::string>& PluginContainer::getDependencies() const {
    return dependencies;
}

void PluginContainer::clearError() {
    lastError.clear();
    if (state == PluginState::Error) {
        state = PluginState::Loaded;
    }
}

// -- PluginSecurityManager implementation
PluginSecurityManager::PluginSecurityManager() = default;
PluginSecurityManager::~PluginSecurityManager() = default;

bool PluginSecurityManager::validatePlugin(const std::filesystem::path& path) {
    // Basic file existence check
    if (!std::filesystem::exists(path)) {
        return false;
    }
    
    // Check file size (avoid extremely large files)
    auto fileSize = std::filesystem::file_size(path);
    if (fileSize > 100 * 1024 * 1024) {  // 100MB limit
        return false;
    }
    
    // Read file for hash validation
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Calculate hash
    std::string hash = calculateHash(content);
    
    // For now, just return true - in a real implementation,
    // this would check against a whitelist or signature database
    return true;
}

bool PluginSecurityManager::verifySignature(const PluginMetadata& metadata, const std::string& pluginData) {
    // Basic signature verification
    return validateSignature(pluginData, metadata.signature);
}

bool PluginSecurityManager::isTrusted(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(securityMutex);
    auto it = trustedPlugins.find(pluginName);
    return it != trustedPlugins.end() && it->second;
}

void PluginSecurityManager::enableSandbox(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(securityMutex);
    sandboxEnabled[pluginName] = true;
}

void PluginSecurityManager::disableSandbox(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(securityMutex);
    sandboxEnabled[pluginName] = false;
}

bool PluginSecurityManager::isSandboxEnabled(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(securityMutex);
    auto it = sandboxEnabled.find(pluginName);
    return it != sandboxEnabled.end() && it->second;
}

void PluginSecurityManager::grantPermission(const std::string& pluginName, const std::string& permission) {
    std::lock_guard<std::mutex> lock(securityMutex);
    pluginPermissions[pluginName].push_back(permission);
}

void PluginSecurityManager::revokePermission(const std::string& pluginName, const std::string& permission) {
    std::lock_guard<std::mutex> lock(securityMutex);
    auto it = pluginPermissions.find(pluginName);
    if (it != pluginPermissions.end()) {
        auto permIt = std::find(it->second.begin(), it->second.end(), permission);
        if (permIt != it->second.end()) {
            it->second.erase(permIt);
        }
    }
}

bool PluginSecurityManager::hasPermission(const std::string& pluginName, const std::string& permission) const {
    std::lock_guard<std::mutex> lock(securityMutex);
    auto it = pluginPermissions.find(pluginName);
    if (it != pluginPermissions.end()) {
        return std::find(it->second.begin(), it->second.end(), permission) != it->second.end();
    }
    return false;
}

void PluginSecurityManager::addTrustedPlugin(const std::string& pluginName, const std::string& signature) {
    std::lock_guard<std::mutex> lock(securityMutex);
    trustedPlugins[pluginName] = true;
}

void PluginSecurityManager::removeTrustedPlugin(const std::string& pluginName) {
    std::lock_guard<std::mutex> lock(securityMutex);
    trustedPlugins.erase(pluginName);
}

std::string PluginSecurityManager::calculateHash(const std::string& data) const {
    // Simple hash calculation (in real implementation, use a proper hash like SHA-256)
    std::hash<std::string> hasher;
    return std::to_string(hasher(data));
}

bool PluginSecurityManager::validateSignature(const std::string& data, const std::string& signature) const {
    // Basic signature validation - in real implementation, use proper cryptographic verification
    return !signature.empty() && signature.length() > 10;
}

// -- HotReloadWatcher implementation
HotReloadWatcher::HotReloadWatcher() : watching(false) {}

HotReloadWatcher::~HotReloadWatcher() {
    stopWatching();
}

void HotReloadWatcher::startWatching(const std::filesystem::path& directory) {
    if (watching.load()) {
        stopWatching();
    }
    
    watchDirectory = directory;
    watching = true;
    watchThread = std::thread(&HotReloadWatcher::watchLoop, this);
}

void HotReloadWatcher::stopWatching() {
    if (watching.load()) {
        watching = false;
        if (watchThread.joinable()) {
            watchThread.join();
        }
    }
}

void HotReloadWatcher::setFileChangedCallback(std::function<void(const std::filesystem::path&)> callback) {
    fileChangedCallback = callback;
}

void HotReloadWatcher::setDirectoryChangedCallback(std::function<void(const std::filesystem::path&)> callback) {
    directoryChangedCallback = callback;
}

void HotReloadWatcher::addWatchExtension(const std::string& extension) {
    auto it = std::find(watchExtensions.begin(), watchExtensions.end(), extension);
    if (it == watchExtensions.end()) {
        watchExtensions.push_back(extension);
    }
}

void HotReloadWatcher::removeWatchExtension(const std::string& extension) {
    auto it = std::find(watchExtensions.begin(), watchExtensions.end(), extension);
    if (it != watchExtensions.end()) {
        watchExtensions.erase(it);
    }
}

void HotReloadWatcher::setWatchExtensions(const std::vector<std::string>& extensions) {
    watchExtensions = extensions;
}

void HotReloadWatcher::watchLoop() {
    std::unordered_map<std::string, std::filesystem::file_time_type> fileModTimes;
    
    while (watching.load()) {
        try {
            if (!std::filesystem::exists(watchDirectory)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }
            
            for (const auto& entry : std::filesystem::recursive_directory_iterator(watchDirectory)) {
                if (entry.is_regular_file() && shouldWatchFile(entry.path())) {
                    auto modTime = entry.last_write_time();
                    std::string pathStr = entry.path().string();
                    
                    auto it = fileModTimes.find(pathStr);
                    if (it == fileModTimes.end()) {
                        // New file
                        fileModTimes[pathStr] = modTime;
                    } else if (it->second != modTime) {
                        // File modified
                        it->second = modTime;
                        if (fileChangedCallback) {
                            fileChangedCallback(entry.path());
                        }
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            // Handle filesystem errors gracefully
            std::cerr << "Filesystem error in hot reload watcher: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

bool HotReloadWatcher::shouldWatchFile(const std::filesystem::path& file) const {
    if (watchExtensions.empty()) {
        return true;  // Watch all files if no extensions specified
    }
    
    std::string extension = file.extension().string();
    return std::find(watchExtensions.begin(), watchExtensions.end(), extension) != watchExtensions.end();
}

// -- PluginManager implementation
PluginManager::PluginManager()
    : initialized(false), hotReloadEnabled(false), hotReloadDelay(std::chrono::milliseconds(1000)) {
    
    // Register default loader
    registerLoader(std::make_shared<DynamicLibraryLoader>());
}

PluginManager::~PluginManager() {
    shutdown();
}

bool PluginManager::initialize() {
    if (initialized.load()) {
        return true;
    }
    
    // Setup hot reload watcher
    hotReloadWatcher.setFileChangedCallback([this](const std::filesystem::path& path) {
        onFileChanged(path);
    });
    
    initialized = true;
    return true;
}

void PluginManager::shutdown() {
    if (!initialized.load()) {
        return;
    }
    
    // Stop hot reload
    hotReloadWatcher.stopWatching();
    
    // Unload all plugins in reverse dependency order
    auto order = getDependencyOrder();
    std::reverse(order.begin(), order.end());
    
    for (const auto& pluginName : order) {
        unloadPlugin(pluginName);
    }
    
    {
        std::lock_guard<std::mutex> lock(pluginsMutex);
        plugins.clear();
    }
    
    initialized = false;
}

void PluginManager::addPluginDirectory(const std::filesystem::path& directory) {
    auto it = std::find(pluginDirectories.begin(), pluginDirectories.end(), directory);
    if (it == pluginDirectories.end()) {
        pluginDirectories.push_back(directory);
    }
}

void PluginManager::removePluginDirectory(const std::filesystem::path& directory) {
    auto it = std::find(pluginDirectories.begin(), pluginDirectories.end(), directory);
    if (it != pluginDirectories.end()) {
        pluginDirectories.erase(it);
    }
}

void PluginManager::setPluginDirectories(const std::vector<std::filesystem::path>& directories) {
    pluginDirectories = directories;
}

const std::vector<std::filesystem::path>& PluginManager::getPluginDirectories() const {
    return pluginDirectories;
}

bool PluginManager::loadPlugin(const std::filesystem::path& path) {
    return loadPlugin(path.stem().string(), path);
}

bool PluginManager::loadPlugin(const std::string& name, const std::filesystem::path& path) {
    return loadPluginInternal(name, path);
}

bool PluginManager::unloadPlugin(const std::string& name) {
    return unloadPluginInternal(name);
}

bool PluginManager::reloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    
    auto it = plugins.find(name);
    if (it == plugins.end()) {
        return false;
    }
    
    // Get the original path
    auto originalPath = it->second->getMetadata().name;  // Simplified for this example
    
    // Find compatible loader
    std::filesystem::path pluginPath;  // This would be stored in the container
    IPluginLoader* loader = findCompatibleLoader(pluginPath);
    if (!loader) {
        return false;
    }
    
    // Load new plugin instance
    auto newPlugin = loader->loadPlugin(pluginPath);
    if (!newPlugin) {
        return false;
    }
    
    // Reload the container
    bool success = it->second->reload(newPlugin);
    
    // Re-initialize if successful
    if (success) {
        success = it->second->initialize(&api);
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        statistics.totalReloads++;
    }
    
    return success;
}

std::vector<std::filesystem::path> PluginManager::discoverPlugins() const {
    std::vector<std::filesystem::path> discoveredPlugins;
    
    for (const auto& directory : pluginDirectories) {
        if (!std::filesystem::exists(directory)) {
            continue;
        }
        
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    IPluginLoader* loader = findCompatibleLoader(entry.path());
                    if (loader) {
                        discoveredPlugins.push_back(entry.path());
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error discovering plugins in " << directory << ": " << e.what() << std::endl;
        }
    }
    
    return discoveredPlugins;
}

bool PluginManager::loadAllPlugins() {
    auto discoveredPlugins = discoverPlugins();
    bool allSuccess = true;
    
    for (const auto& pluginPath : discoveredPlugins) {
        if (!loadPlugin(pluginPath)) {
            allSuccess = false;
        }
    }
    
    // Resolve dependencies after loading all plugins
    return resolveDependencies() && allSuccess;
}

bool PluginManager::loadPluginsFromDirectory(const std::filesystem::path& directory) {
    if (!std::filesystem::exists(directory)) {
        return false;
    }
    
    bool allSuccess = true;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                IPluginLoader* loader = findCompatibleLoader(entry.path());
                if (loader) {
                    if (!loadPlugin(entry.path())) {
                        allSuccess = false;
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error loading plugins from " << directory << ": " << e.what() << std::endl;
        return false;
    }
    
    return allSuccess;
}

PluginContainer* PluginManager::getPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    auto it = plugins.find(name);
    return (it != plugins.end()) ? it->second.get() : nullptr;
}

const PluginContainer* PluginManager::getPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    auto it = plugins.find(name);
    return (it != plugins.end()) ? it->second.get() : nullptr;
}

std::vector<std::string> PluginManager::getLoadedPluginNames() const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    std::vector<std::string> names;
    names.reserve(plugins.size());
    
    for (const auto& [name, plugin] : plugins) {
        names.push_back(name);
    }
    
    return names;
}

std::vector<PluginContainer*> PluginManager::getPluginsByType(PluginType type) {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    std::vector<PluginContainer*> result;
    
    for (const auto& [name, plugin] : plugins) {
        if (plugin->getType() == type) {
            result.push_back(plugin.get());
        }
    }
    
    return result;
}

bool PluginManager::isPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    return plugins.find(name) != plugins.end();
}

PluginState PluginManager::getPluginState(const std::string& name) const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    auto it = plugins.find(name);
    return (it != plugins.end()) ? it->second->getState() : PluginState::Unloaded;
}

size_t PluginManager::getLoadedPluginCount() const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    return plugins.size();
}

bool PluginManager::resolveDependencies() {
    buildDependencyGraph();
    return topologicalSort();
}

std::vector<std::string> PluginManager::getDependencyOrder() const {
    return dependencyOrder;
}

bool PluginManager::validateDependencies(const std::string& pluginName) const {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    
    auto it = plugins.find(pluginName);
    if (it == plugins.end()) {
        return false;
    }
    
    const auto& dependencies = it->second->getDependencies();
    for (const auto& dep : dependencies) {
        if (plugins.find(dep) == plugins.end()) {
            return false;  // Dependency not found
        }
    }
    
    return true;
}

void PluginManager::updatePlugins(float deltaTime) {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    
    // Update plugins in dependency order
    for (const auto& pluginName : dependencyOrder) {
        auto it = plugins.find(pluginName);
        if (it != plugins.end() && it->second->isReady()) {
            it->second->update(deltaTime);
        }
    }
}

void PluginManager::enableHotReload(bool enable) {
    hotReloadEnabled = enable;
    
    if (enable && !pluginDirectories.empty()) {
        // Start watching the first directory
        hotReloadWatcher.startWatching(pluginDirectories[0]);
    } else {
        hotReloadWatcher.stopWatching();
    }
}

// Internal implementation methods continue...
IPluginLoader* PluginManager::findCompatibleLoader(const std::filesystem::path& path) const {
    std::lock_guard<std::mutex> lock(loadersMutex);
    
    for (const auto& loader : loaders) {
        if (loader->canLoad(path)) {
            return loader.get();
        }
    }
    
    return nullptr;
}

bool PluginManager::loadPluginInternal(const std::string& name, const std::filesystem::path& path) {
    // Security validation
    if (!securityManager.validatePlugin(path)) {
        notifyPluginError(name, "Plugin failed security validation");
        return false;
    }
    
    // Find compatible loader
    IPluginLoader* loader = findCompatibleLoader(path);
    if (!loader) {
        notifyPluginError(name, "No compatible loader found for plugin");
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Load the plugin
    auto plugin = loader->loadPlugin(path);
    if (!plugin) {
        notifyPluginError(name, "Failed to load plugin");
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            statistics.totalPluginsFailed++;
        }
        return false;
    }
    
    // Create container
    auto container = std::make_unique<PluginContainer>(name, plugin);
    
    // Set up reload callback
    container->setReloadCallback([this](const std::string& pluginName) {
        onHotReload(pluginName);
    });
    
    // Initialize the plugin
    if (!container->initialize(&api)) {
        notifyPluginError(name, container->getLastError());
        {
            std::lock_guard<std::mutex> statsLock(statsMutex);
            statistics.totalPluginsFailed++;
        }
        return false;
    }
    
    // Add to plugins map
    {
        std::lock_guard<std::mutex> lock(pluginsMutex);
        plugins[name] = std::move(container);
    }
    
    // Update statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    {
        std::lock_guard<std::mutex> statsLock(statsMutex);
        statistics.totalPluginsLoaded++;
        statistics.totalLoadTime += loadTime;
        statistics.averageLoadTime = statistics.totalLoadTime / statistics.totalPluginsLoaded;
        statistics.pluginsByType[plugin->getType()]++;
    }
    
    notifyPluginLoaded(name);
    return true;
}

bool PluginManager::unloadPluginInternal(const std::string& name) {
    std::lock_guard<std::mutex> lock(pluginsMutex);
    
    auto it = plugins.find(name);
    if (it == plugins.end()) {
        return false;
    }
    
    // Shutdown the plugin
    it->second->shutdown();
    
    // Remove from plugins map
    plugins.erase(it);
    
    notifyPluginUnloaded(name);
    return true;
}

void PluginManager::buildDependencyGraph() {
    dependencyGraph.clear();
    
    std::lock_guard<std::mutex> lock(pluginsMutex);
    for (const auto& [name, plugin] : plugins) {
        dependencyGraph[name] = plugin->getDependencies();
    }
}

bool PluginManager::topologicalSort() {
    dependencyOrder.clear();
    
    std::unordered_map<std::string, int> inDegree;
    
    // Calculate in-degrees
    for (const auto& [plugin, deps] : dependencyGraph) {
        if (inDegree.find(plugin) == inDegree.end()) {
            inDegree[plugin] = 0;
        }
        for (const auto& dep : deps) {
            inDegree[dep]++;
        }
    }
    
    // Find nodes with zero in-degree
    std::queue<std::string> queue;
    for (const auto& [plugin, degree] : inDegree) {
        if (degree == 0) {
            queue.push(plugin);
        }
    }
    
    // Process nodes
    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();
        dependencyOrder.push_back(current);
        
        auto it = dependencyGraph.find(current);
        if (it != dependencyGraph.end()) {
            for (const auto& neighbor : it->second) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    queue.push(neighbor);
                }
            }
        }
    }
    
    // Check for cycles
    return dependencyOrder.size() == dependencyGraph.size();
}

bool PluginManager::hasCyclicDependencies() const {
    // This would be implemented with a proper cycle detection algorithm
    return false;  // Simplified for this implementation
}

void PluginManager::onFileChanged(const std::filesystem::path& path) {
    // Delayed hot reload to avoid multiple rapid reloads
    std::this_thread::sleep_for(hotReloadDelay);
    
    // Find plugin by path and trigger reload
    std::string pluginName = path.stem().string();
    if (isPluginLoaded(pluginName)) {
        reloadPlugin(pluginName);
    }
}

void PluginManager::onHotReload(const std::string& pluginName) {
    // Plugin-specific hot reload handling
    std::cout << "Plugin hot-reloaded: " << pluginName << std::endl;
}

void PluginManager::notifyPluginLoaded(const std::string& name) {
    if (pluginLoadedCallback) {
        pluginLoadedCallback(name);
    }
}

void PluginManager::notifyPluginUnloaded(const std::string& name) {
    if (pluginUnloadedCallback) {
        pluginUnloadedCallback(name);
    }
}

void PluginManager::notifyPluginError(const std::string& name, const std::string& error) {
    if (pluginErrorCallback) {
        pluginErrorCallback(name, error);
    }
}

// Additional method implementations continue...
void PluginManager::setPluginLoadedCallback(std::function<void(const std::string&)> callback) {
    pluginLoadedCallback = callback;
}

void PluginManager::setPluginUnloadedCallback(std::function<void(const std::string&)> callback) {
    pluginUnloadedCallback = callback;
}

void PluginManager::setPluginErrorCallback(std::function<void(const std::string&, const std::string&)> callback) {
    pluginErrorCallback = callback;
}

void PluginManager::setConfiguration(const std::string& key, const std::any& value) {
    std::lock_guard<std::mutex> lock(configMutex);
    configuration[key] = value;
}

std::any PluginManager::getConfiguration(const std::string& key, const std::any& defaultValue) const {
    std::lock_guard<std::mutex> lock(configMutex);
    auto it = configuration.find(key);
    return (it != configuration.end()) ? it->second : defaultValue;
}

bool PluginManager::loadConfiguration(const std::filesystem::path& configPath) {
    // Configuration loading would be implemented here
    return true;  // Placeholder
}

bool PluginManager::saveConfiguration(const std::filesystem::path& configPath) const {
    // Configuration saving would be implemented here
    return true;  // Placeholder
}

void PluginManager::registerLoader(std::shared_ptr<IPluginLoader> loader) {
    std::lock_guard<std::mutex> lock(loadersMutex);
    loaders.push_back(loader);
}

void PluginManager::unregisterLoader(std::shared_ptr<IPluginLoader> loader) {
    std::lock_guard<std::mutex> lock(loadersMutex);
    auto it = std::find(loaders.begin(), loaders.end(), loader);
    if (it != loaders.end()) {
        loaders.erase(it);
    }
}

PluginManager::Statistics PluginManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex);
    return statistics;
}

void PluginManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(statsMutex);
    statistics = Statistics{};
}

} // namespace Plugin
} // namespace JJM