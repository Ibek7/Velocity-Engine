#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <any>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace JJM {
namespace Core {

// Custom exception for configuration errors
class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message) 
        : std::runtime_error("Config Error: " + message) {}
};

class Config {
private:
    std::unordered_map<std::string, std::any> values;
    static Config* instance;
    
    Config();
    
public:
    ~Config();
    
    static Config* getInstance();
    static void destroy();
    
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    
    template<typename T>
    void set(const std::string& key, const T& value) {
        if (key.empty()) {
            throw ConfigException("Configuration key cannot be empty");
        }
        values[key] = value;
    }
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        if (key.empty()) {
            throw ConfigException("Configuration key cannot be empty");
        }
        auto it = values.find(key);
        if (it != values.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                throw ConfigException("Type mismatch for key '" + key + "'");
            }
        }
        return defaultValue;
    }
    
    // Throws if key not found (for required configs)
    template<typename T>
    T getRequired(const std::string& key) const {
        if (key.empty()) {
            throw ConfigException("Configuration key cannot be empty");
        }
        auto it = values.find(key);
        if (it == values.end()) {
            throw ConfigException("Required configuration key '" + key + "' not found");
        }
        try {
            return std::any_cast<T>(it->second);
        } catch (const std::bad_any_cast&) {
            throw ConfigException("Type mismatch for key '" + key + "'");
        }
    }
    
    bool has(const std::string& key) const;
    void remove(const std::string& key);
    void clear();
    
    // Common configuration getters/setters
    int getWindowWidth() const;
    int getWindowHeight() const;
    bool isFullscreen() const;
    std::string getWindowTitle() const;
    int getTargetFPS() const;
    
    void setWindowWidth(int width);
    void setWindowHeight(int height);
    void setFullscreen(bool fullscreen);
    void setWindowTitle(const std::string& title);
    void setTargetFPS(int fps);
    
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
};

} // namespace Core
} // namespace JJM

#endif // CONFIG_H
