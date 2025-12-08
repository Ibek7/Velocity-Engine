#include "events/EventDispatcher.h"

namespace JJM {
namespace Events {

EventDispatcher::EventDispatcher() : nextListenerId(1) {}

EventDispatcher::~EventDispatcher() {
    clear();
}

void EventDispatcher::removeEventListener(int listenerId) {
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
            [listenerId](const ListenerInfo& info) {
                return info.id == listenerId;
            }),
        listeners.end()
    );
}

void EventDispatcher::dispatchQueued() {
    auto events = std::move(eventQueue);
    eventQueue.clear();
    
    for (const auto& event : events) {
        std::type_index eventType = event->getType();
        
        for (const auto& listener : listeners) {
            if (listener.type == eventType) {
                listener.callback(*event);
            }
        }
    }
}

void EventDispatcher::clear() {
    listeners.clear();
    eventQueue.clear();
}

GlobalEventBus& GlobalEventBus::instance() {
    static GlobalEventBus instance;
    return instance;
}

} // namespace Events
} // namespace JJM
