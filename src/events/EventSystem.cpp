#include "events/EventSystem.h"

#include <algorithm>

namespace JJM {
namespace Events {

// Static member initialization
std::atomic<uint64_t> Event::nextEventId{1};
EventDispatcher* EventDispatcher::instance = nullptr;

// Event implementation
Event::Event(const std::string& eventType, EventPriority priority)
    : type(eventType),
      handled(false),
      priority(priority),
      propagation(EventPropagation::Continue),
      timestamp(std::chrono::steady_clock::now()),
      eventId(nextEventId++) {}

// EventDispatcher implementation
EventDispatcher::EventDispatcher()
    : nextListenerId(1),
      processingQueue(false),
      maxQueueSize(100),
      recordHistory(false),
      maxHistorySize(100) {}

EventDispatcher::~EventDispatcher() { removeAllListeners(); }

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
    ListenerInfo info;
    info.id = id;
    info.handler = handler;
    info.priority = EventPriority::Normal;
    info.once = false;
    // filter is empty/null which is fine
    listeners[eventType].push_back(info);
    return id;
}

void EventDispatcher::removeEventListener(const std::string& eventType, int listenerId) {
    auto it = listeners.find(eventType);
    if (it != listeners.end()) {
        auto& listenerList = it->second;
        listenerList.erase(std::remove_if(listenerList.begin(), listenerList.end(),
                                          [listenerId](const ListenerInfo& info) {
                                              return info.id == listenerId;
                                          }),
                           listenerList.end());

        if (listenerList.empty()) {
            listeners.erase(it);
        }
    }
}

void EventDispatcher::removeAllListeners(const std::string& eventType) {
    listeners.erase(eventType);
}

void EventDispatcher::removeAllListeners() { listeners.clear(); }

void EventDispatcher::dispatchEvent(const Event& event) {
    auto it = listeners.find(event.getType());
    if (it != listeners.end()) {
        // Make a copy of the listener list in case it's modified during event handling
        std::vector<ListenerInfo> listenersCopy = it->second;

        // Sort by priority if needed, but for legacy addEventListener we used default priority.
        // If we want to support priority, we should sort.
        // Ideally listeners should be kept sorted.

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

// Implement subscribe/unsubscribe methods if needed or leave them for later if they are not used
// yet. For now, only fixing the compilation errors which were in Event constructor and
// addEventListener.

}  // namespace Events
}  // namespace JJM
