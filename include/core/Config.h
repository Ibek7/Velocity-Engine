#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>
#include <any>
#include <fstream>
#include <sstream>

namespace JJM {
namespace Core {

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
        values[key] = value;
    }
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const {
        auto it = values.find(key);
        if (it != values.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
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
