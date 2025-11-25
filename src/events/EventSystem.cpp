#include "events/EventSystem.h"
#include <algorithm>

namespace JJM {
namespace Events {

// Event implementation
Event::Event(const std::string& eventType)
    : type(eventType), handled(false) {}

// EventDispatcher implementation
EventDispatcher* EventDispatcher::instance = nullptr;

EventDispatcher::EventDispatcher() : nextListenerId(1) {}

EventDispatcher::~EventDispatcher() {
    removeAllListeners();
}

EventDispatcher* EventDispatcher::getInstance() {
    if (!instance) {
        instance = new EventDispatcher();
    }
    return instance;
}

void EventDispatcher::destroy() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

int EventDispatcher::addEventListener(const std::string& eventType, const EventHandler& handler) {
    int id = nextListenerId++;
    listeners[eventType].push_back({id, handler});
    return id;
}

void EventDispatcher::removeEventListener(const std::string& eventType, int listenerId) {
    auto it = listeners.find(eventType);
    if (it != listeners.end()) {
        auto& listenerList = it->second;
        listenerList.erase(
            std::remove_if(listenerList.begin(), listenerList.end(),
                [listenerId](const ListenerInfo& info) { return info.id == listenerId; }),
            listenerList.end()
        );
        
        if (listenerList.empty()) {
            listeners.erase(it);
        }
    }
}

void EventDispatcher::removeAllListeners(const std::string& eventType) {
    listeners.erase(eventType);
}

void EventDispatcher::removeAllListeners() {
    listeners.clear();
}

void EventDispatcher::dispatchEvent(const Event& event) {
    auto it = listeners.find(event.getType());
    if (it != listeners.end()) {
        // Make a copy of the listener list in case it's modified during event handling
        std::vector<ListenerInfo> listenersCopy = it->second;
        
        for (const auto& listenerInfo : listenersCopy) {
            listenerInfo.handler(event);
            
            if (event.isHandled()) {
                break;
            }
        }
    }
}

void EventDispatcher::dispatchEvent(const std::string& eventType) {
    Event event(eventType);
    dispatchEvent(event);
}

} // namespace Events
} // namespace JJM
