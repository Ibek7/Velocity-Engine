#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <any>

namespace JJM {
namespace Events {

class Event {
private:
    std::string type;
    std::unordered_map<std::string, std::any> data;
    bool handled;
    
public:
    Event(const std::string& eventType);
    
    template<typename T>
    void setData(const std::string& key, const T& value) {
        data[key] = value;
    }
    
    template<typename T>
    T getData(const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) {
            return std::any_cast<T>(it->second);
        }
        throw std::runtime_error("Event data key not found: " + key);
    }
    
    template<typename T>
    T getData(const std::string& key, const T& defaultValue) const {
        auto it = data.find(key);
        if (it != data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return defaultValue;
            }
        }
        return defaultValue;
    }
    
    const std::string& getType() const { return type; }
    void markHandled() { handled = true; }
    bool isHandled() const { return handled; }
};

using EventHandler = std::function<void(const Event&)>;

class EventDispatcher {
private:
    struct ListenerInfo {
        int id;
        EventHandler handler;
    };
    
    std::unordered_map<std::string, std::vector<ListenerInfo>> listeners;
    int nextListenerId;
    
    static EventDispatcher* instance;
    EventDispatcher();
    
public:
    ~EventDispatcher();
    
    static EventDispatcher* getInstance();
    static void destroy();
    
    int addEventListener(const std::string& eventType, const EventHandler& handler);
    void removeEventListener(const std::string& eventType, int listenerId);
    void removeAllListeners(const std::string& eventType);
    void removeAllListeners();
    
    void dispatchEvent(const Event& event);
    void dispatchEvent(const std::string& eventType);
    
    // Delete copy constructor and assignment operator
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
};

// Common event types
namespace EventTypes {
    const std::string COLLISION = "collision";
    const std::string ENTITY_CREATED = "entity_created";
    const std::string ENTITY_DESTROYED = "entity_destroyed";
    const std::string SCENE_LOADED = "scene_loaded";
    const std::string SCENE_UNLOADED = "scene_unloaded";
    const std::string PLAYER_DEATH = "player_death";
    const std::string GAME_OVER = "game_over";
    const std::string LEVEL_COMPLETE = "level_complete";
    const std::string BUTTON_CLICKED = "button_clicked";
    const std::string VALUE_CHANGED = "value_changed";
}

} // namespace Events
} // namespace JJM

#endif // EVENT_SYSTEM_H
