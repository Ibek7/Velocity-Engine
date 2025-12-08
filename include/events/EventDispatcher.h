#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <algorithm>

namespace JJM {
namespace Events {

class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index getType() const = 0;
};

template<typename T>
class TypedEvent : public Event {
public:
    TypedEvent(const T& data) : data(data) {}
    
    std::type_index getType() const override {
        return std::type_index(typeid(T));
    }
    
    const T& getData() const { return data; }

private:
    T data;
};

using EventListener = std::function<void(const Event&)>;

class EventDispatcher {
public:
    EventDispatcher();
    ~EventDispatcher();
    
    template<typename T>
    int addEventListener(std::function<void(const T&)> listener);
    
    void removeEventListener(int listenerId);
    
    template<typename T>
    void removeEventListeners();
    
    template<typename T>
    void dispatch(const T& eventData);
    
    void dispatchQueued();
    
    template<typename T>
    void queueEvent(const T& eventData);
    
    void clear();
    
    size_t getListenerCount() const { return listeners.size(); }

private:
    struct ListenerInfo {
        int id;
        std::type_index type;
        EventListener callback;
        
        ListenerInfo(int id, std::type_index type, EventListener callback)
            : id(id), type(type), callback(callback) {}
    };
    
    std::vector<ListenerInfo> listeners;
    std::vector<std::unique_ptr<Event>> eventQueue;
    
    int nextListenerId;
};

template<typename T>
int EventDispatcher::addEventListener(std::function<void(const T&)> listener) {
    int id = nextListenerId++;
    
    auto wrapper = [listener](const Event& event) {
        const TypedEvent<T>* typedEvent = dynamic_cast<const TypedEvent<T>*>(&event);
        if (typedEvent) {
            listener(typedEvent->getData());
        }
    };
    
    listeners.emplace_back(id, std::type_index(typeid(T)), wrapper);
    
    return id;
}

template<typename T>
void EventDispatcher::removeEventListeners() {
    std::type_index targetType = std::type_index(typeid(T));
    
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
            [targetType](const ListenerInfo& info) {
                return info.type == targetType;
            }),
        listeners.end()
    );
}

template<typename T>
void EventDispatcher::dispatch(const T& eventData) {
    TypedEvent<T> event(eventData);
    std::type_index eventType = std::type_index(typeid(T));
    
    for (const auto& listener : listeners) {
        if (listener.type == eventType) {
            listener.callback(event);
        }
    }
}

template<typename T>
void EventDispatcher::queueEvent(const T& eventData) {
    eventQueue.push_back(std::make_unique<TypedEvent<T>>(eventData));
}

class GlobalEventBus {
public:
    static GlobalEventBus& instance();
    
    template<typename T>
    int subscribe(std::function<void(const T&)> listener) {
        return dispatcher.addEventListener<T>(listener);
    }
    
    void unsubscribe(int listenerId) {
        dispatcher.removeEventListener(listenerId);
    }
    
    template<typename T>
    void publish(const T& eventData) {
        dispatcher.dispatch<T>(eventData);
    }
    
    template<typename T>
    void publishQueued(const T& eventData) {
        dispatcher.queueEvent<T>(eventData);
    }
    
    void processQueue() {
        dispatcher.dispatchQueued();
    }

private:
    GlobalEventBus() = default;
    EventDispatcher dispatcher;
};

} // namespace Events
} // namespace JJM
