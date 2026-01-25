#ifndef TRIGGER_SYSTEM_H
#define TRIGGER_SYSTEM_H

#include <vector>
#include <functional>
#include <string>
#include <memory>

namespace JJM {
namespace Gameplay {

class Entity;

enum class TriggerShape {
    BOX,
    SPHERE,
    CAPSULE,
    CUSTOM
};

enum class TriggerEvent {
    ON_ENTER,
    ON_EXIT,
    ON_STAY
};

struct TriggerBounds {
    float center[3];
    float extents[3];  // For box/sphere radius
    TriggerShape shape;
};

class TriggerVolume {
public:
    TriggerVolume(const std::string& name);
    ~TriggerVolume();
    
    void setShape(TriggerShape shape) { m_bounds.shape = shape; }
    void setCenter(float x, float y, float z);
    void setSize(float x, float y, float z);
    void setRadius(float radius);
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Callbacks
    using TriggerCallback = std::function<void(Entity*)>;
    void onEnter(TriggerCallback callback) { m_onEnter = callback; }
    void onExit(TriggerCallback callback) { m_onExit = callback; }
    void onStay(TriggerCallback callback) { m_onStay = callback; }
    
    void update(float deltaTime);
    bool contains(const float point[3]) const;
    
    const std::vector<Entity*>& getEntitiesInside() const { return m_entitiesInside; }
    
private:
    std::string m_name;
    TriggerBounds m_bounds;
    bool m_enabled;
    
    std::vector<Entity*> m_entitiesInside;
    TriggerCallback m_onEnter;
    TriggerCallback m_onExit;
    TriggerCallback m_onStay;
};

class TriggerSystem {
public:
    void update(float deltaTime);
    void registerTrigger(TriggerVolume* trigger);
    void unregisterTrigger(TriggerVolume* trigger);
    
private:
    std::vector<TriggerVolume*> m_triggers;
};

} // namespace Gameplay
} // namespace JJM

#endif
