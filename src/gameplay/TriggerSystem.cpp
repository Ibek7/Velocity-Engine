#include "gameplay/TriggerSystem.h"
#include <cmath>

namespace JJM {
namespace Gameplay {

TriggerVolume::TriggerVolume(const std::string& name)
    : m_name(name), m_enabled(true) {
    m_bounds.center[0] = m_bounds.center[1] = m_bounds.center[2] = 0;
    m_bounds.extents[0] = m_bounds.extents[1] = m_bounds.extents[2] = 1;
    m_bounds.shape = TriggerShape::BOX;
}

TriggerVolume::~TriggerVolume() {
}

void TriggerVolume::setCenter(float x, float y, float z) {
    m_bounds.center[0] = x;
    m_bounds.center[1] = y;
    m_bounds.center[2] = z;
}

void TriggerVolume::setSize(float x, float y, float z) {
    m_bounds.extents[0] = x * 0.5f;
    m_bounds.extents[1] = y * 0.5f;
    m_bounds.extents[2] = z * 0.5f;
}

void TriggerVolume::setRadius(float radius) {
    m_bounds.extents[0] = radius;
}

void TriggerVolume::update(float deltaTime) {
    if (!m_enabled) return;
    
    // Check entities and trigger callbacks
    for (Entity* entity : m_entitiesInside) {
        if (m_onStay) {
            m_onStay(entity);
        }
    }
}

bool TriggerVolume::contains(const float point[3]) const {
    switch (m_bounds.shape) {
        case TriggerShape::BOX: {
            float dx = std::abs(point[0] - m_bounds.center[0]);
            float dy = std::abs(point[1] - m_bounds.center[1]);
            float dz = std::abs(point[2] - m_bounds.center[2]);
            return dx <= m_bounds.extents[0] &&
                   dy <= m_bounds.extents[1] &&
                   dz <= m_bounds.extents[2];
        }
        case TriggerShape::SPHERE: {
            float dx = point[0] - m_bounds.center[0];
            float dy = point[1] - m_bounds.center[1];
            float dz = point[2] - m_bounds.center[2];
            float distSq = dx*dx + dy*dy + dz*dz;
            return distSq <= m_bounds.extents[0] * m_bounds.extents[0];
        }
        default:
            return false;
    }
}

void TriggerSystem::update(float deltaTime) {
    for (auto* trigger : m_triggers) {
        trigger->update(deltaTime);
    }
}

void TriggerSystem::registerTrigger(TriggerVolume* trigger) {
    m_triggers.push_back(trigger);
}

void TriggerSystem::unregisterTrigger(TriggerVolume* trigger) {
    m_triggers.erase(
        std::remove(m_triggers.begin(), m_triggers.end(), trigger),
        m_triggers.end()
    );
}

} // namespace Gameplay
} // namespace JJM
