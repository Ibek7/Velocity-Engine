#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <any>
#include <queue>
#include <mutex>
#include <chrono>
#include <optional>
#include <type_traits>

namespace JJM {
namespace Events {

/**
 * @brief Base class for strongly-typed events
 * 
 * Derive from this to create type-safe event classes:
 * @code
 * struct PlayerDiedEvent : public TypedEvent<PlayerDiedEvent> {
 *     int playerId;
 *     std::string cause;
 * };
 * @endcode
 */
template<typename Derived>
struct TypedEvent {
    static const char* getEventType() {
        static const std::string typeName = typeid(Derived).name();
        return typeName.c_str();
    }
};

/**
 * @brief Type-safe event handler wrapper
 */
template<typename EventType>
using TypedEventHandler = std::function<void(const EventType&)>;

/**
 * @brief Event priority levels
 */
enum class EventPriority {
    Low = 0,
    Normal = 100,
    High = 200,
    Critical = 300,
    Immediate = 400  // Bypass queue, process immediately
};

/**
 * @brief Event propagation behavior
 */
enum class EventPropagation {
    Continue,       // Continue to next handler
    Stop,           // Stop propagation to remaining handlers
    StopImmediate   // Stop immediately, don't call any more handlers
};

class Event {
private:
    std::string type;
    std::unordered_map<std::string, std::any> data;
    bool handled;
    EventPriority priority;
    EventPropagation propagation;
    std::chrono::steady_clock::time_point timestamp;
    std::string source;
    uint64_t eventId;
    static std::atomic<uint64_t> nextEventId;
    
public:
    Event(const std::string& eventType, EventPriority priority = EventPriority::Normal);
    
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
    
    bool hasData(const std::string& key) const {
        return data.find(key) != data.end();
    }
    
    const std::string& getType() const { return type; }
    void markHandled() { handled = true; }
    bool isHandled() const { return handled; }
    
    EventPriority getPriority() const { return priority; }
    void setPriority(EventPriority p) { priority = p; }
    
    EventPropagation getPropagation() const { return propagation; }
    void stopPropagation() { propagation = EventPropagation::Stop; }
    void stopImmediatePropagation() { propagation = EventPropagation::StopImmediate; }
    
    void setSource(const std::string& src) { source = src; }
    const std::string& getSource() const { return source; }
    
    uint64_t getId() const { return eventId; }
    std::chrono::steady_clock::time_point getTimestamp() const { return timestamp; }
};

using EventHandler = std::function<void(const Event&)>;
using EventFilter = std::function<bool(const Event&)>;

/**
 * @brief Queued event entry with scheduling
 */
struct QueuedEvent {
    Event event;
    std::chrono::steady_clock::time_point dispatchTime;
    float delay;
    bool repeating;
    float repeatInterval;
    int repeatCount;         // -1 for infinite
    int currentRepeat;
    
    QueuedEvent(const Event& e, float delaySeconds = 0.0f)
        : event(e)
        , delay(delaySeconds)
        , repeating(false)
        , repeatInterval(0.0f)
        , repeatCount(0)
        , currentRepeat(0)
    {
        dispatchTime = std::chrono::steady_clock::now() + 
                       std::chrono::milliseconds(static_cast<int>(delaySeconds * 1000));
    }
    
    bool operator<(const QueuedEvent& other) const {
        // Lower priority value = lower priority, but we want high priority first
        if (event.getPriority() != other.event.getPriority()) {
            return static_cast<int>(event.getPriority()) < static_cast<int>(other.event.getPriority());
        }
        // Earlier dispatch time = higher priority
        return dispatchTime > other.dispatchTime;
    }
};

/**
 * @brief Event subscription with priority and filtering
 */
struct EventSubscription {
    int id;
    EventHandler handler;
    EventPriority priority;
    EventFilter filter;
    bool once;              // Auto-remove after first call
    bool enabled;
    
    EventSubscription()
        : id(0)
        , priority(EventPriority::Normal)
        , once(false)
        , enabled(true)
    {}
    
    bool operator<(const EventSubscription& other) const {
        return static_cast<int>(priority) < static_cast<int>(other.priority);
    }
};

/**
 * @brief Event channel for isolated event streams
 */
class EventChannel {
private:
    std::string name;
    std::vector<EventSubscription> subscriptions;
    int nextSubscriptionId;
    bool enabled;
    
public:
    EventChannel(const std::string& channelName);
    
    int subscribe(const EventHandler& handler, EventPriority priority = EventPriority::Normal);
    int subscribeOnce(const EventHandler& handler, EventPriority priority = EventPriority::Normal);
    int subscribeFiltered(const EventHandler& handler, const EventFilter& filter, 
                          EventPriority priority = EventPriority::Normal);
    void unsubscribe(int subscriptionId);
    void unsubscribeAll();
    
    void dispatch(const Event& event);
    
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool isEnabled() const { return enabled; }
    
    const std::string& getName() const { return name; }
    size_t getSubscriptionCount() const { return subscriptions.size(); }
};

/**
 * @brief Listener info for legacy API compatibility
 */
struct ListenerInfo {
    int id;
    EventHandler handler;
    EventPriority priority;
    EventFilter filter;
    bool once;
};

class EventDispatcher {
private:
    std::unordered_map<std::string, std::vector<ListenerInfo>> listeners;
    int nextListenerId;
    
    // Event queue
    std::priority_queue<QueuedEvent> eventQueue;
    std::mutex queueMutex;
    bool processingQueue;
    size_t maxQueueSize;
    
    // Event channels
    std::unordered_map<std::string, std::unique_ptr<EventChannel>> channels;
    
    // Event history (for debugging/replay)
    std::vector<Event> eventHistory;
    bool recordHistory;
    size_t maxHistorySize;
    
    // Deferred events (for next frame)
    std::vector<Event> deferredEvents;
    std::mutex deferredMutex;
    
    // Statistics
    struct EventStats {
        uint64_t totalDispatched;
        uint64_t totalQueued;
        uint64_t totalFiltered;
        std::unordered_map<std::string, uint64_t> eventCounts;
    };
    EventStats stats;
    
    static EventDispatcher* instance;
    EventDispatcher();
    
public:
    ~EventDispatcher();
    
    static EventDispatcher* getInstance();
    static void destroy();
    
    // Legacy API (kept for compatibility)
    int addEventListener(const std::string& eventType, const EventHandler& handler);
    void removeEventListener(const std::string& eventType, int listenerId);
    void removeAllListeners(const std::string& eventType);
    void removeAllListeners();
    
    // Enhanced subscription API
    int subscribe(const std::string& eventType, const EventHandler& handler, 
                  EventPriority priority = EventPriority::Normal);
    int subscribeOnce(const std::string& eventType, const EventHandler& handler,
                      EventPriority priority = EventPriority::Normal);
    int subscribeFiltered(const std::string& eventType, const EventHandler& handler,
                          const EventFilter& filter, EventPriority priority = EventPriority::Normal);
    void unsubscribe(const std::string& eventType, int subscriptionId);
    
    // Type-safe event API
    template<typename EventType>
    int subscribeTyped(const TypedEventHandler<EventType>& handler,
                      EventPriority priority = EventPriority::Normal) {
        static_assert(std::is_base_of<TypedEvent<EventType>, EventType>::value,
                     "EventType must inherit from TypedEvent<EventType>");
        
        auto wrapper = [handler](const Event& event) {
            try {
                const EventType& typedEvent = std::any_cast<const EventType&>(
                    event.getData<std::any>("_typed_event_data"));
                handler(typedEvent);
            } catch (const std::bad_any_cast&) {
                // Event wasn't properly typed, ignore
            }
        };
        
        return subscribe(EventType::getEventType(), wrapper, priority);
    }
    
    template<typename EventType>
    void dispatchTyped(const EventType& typedEvent) {
        static_assert(std::is_base_of<TypedEvent<EventType>, EventType>::value,
                     "EventType must inherit from TypedEvent<EventType>");
        
        Event event(EventType::getEventType());
        event.setData<std::any>("_typed_event_data", typedEvent);
        dispatchEvent(event);
    }
    
    // Immediate dispatch
    void dispatchEvent(const Event& event);
    void dispatchEvent(const std::string& eventType);
    
    // Queued dispatch
    void queueEvent(const Event& event, float delay = 0.0f);
    void queueRepeatingEvent(const Event& event, float interval, int count = -1);
    void processQueue();
    void clearQueue();
    size_t getQueueSize() const;
    void setMaxQueueSize(size_t size) { maxQueueSize = size; }
    
    // Deferred dispatch (next frame)
    void deferEvent(const Event& event);
    void processDeferred();
    
    // Event channels
    EventChannel* createChannel(const std::string& name);
    EventChannel* getChannel(const std::string& name);
    void destroyChannel(const std::string& name);
    
    // Event history
    void enableHistory(bool enable, size_t maxSize = 1000);
    const std::vector<Event>& getHistory() const { return eventHistory; }
    void clearHistory();
    void replayHistory();
    
    // Statistics
    const EventStats& getStats() const { return stats; }
    void resetStats();
    
    // Delete copy constructor and assignment operator
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
};

/**
 * @brief RAII event subscription scope guard
 */
class ScopedEventSubscription {
private:
    std::string eventType;
    int subscriptionId;
    
public:
    ScopedEventSubscription(const std::string& type, const EventHandler& handler,
                            EventPriority priority = EventPriority::Normal);
    ~ScopedEventSubscription();
    
    ScopedEventSubscription(const ScopedEventSubscription&) = delete;
    ScopedEventSubscription& operator=(const ScopedEventSubscription&) = delete;
    
    ScopedEventSubscription(ScopedEventSubscription&& other) noexcept;
    ScopedEventSubscription& operator=(ScopedEventSubscription&& other) noexcept;
    
    void release();
};

/**
 * @brief Event builder for fluent event creation
 */
class EventBuilder {
private:
    Event event;
    
public:
    EventBuilder(const std::string& type);
    
    EventBuilder& withPriority(EventPriority priority);
    EventBuilder& withSource(const std::string& source);
    
    template<typename T>
    EventBuilder& withData(const std::string& key, const T& value) {
        event.setData(key, value);
        return *this;
    }
    
    Event build() const { return event; }
    void dispatch();
    void queue(float delay = 0.0f);
    void defer();
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
    
    // Additional system events
    const std::string FRAME_START = "frame_start";
    const std::string FRAME_END = "frame_end";
    const std::string INPUT_RECEIVED = "input_received";
    const std::string WINDOW_RESIZE = "window_resize";
    const std::string WINDOW_FOCUS = "window_focus";
    const std::string AUDIO_COMPLETE = "audio_complete";
    const std::string ANIMATION_COMPLETE = "animation_complete";
    const std::string NETWORK_CONNECTED = "network_connected";
    const std::string NETWORK_DISCONNECTED = "network_disconnected";
}

} // namespace Events
} // namespace JJM

#endif // EVENT_SYSTEM_H
