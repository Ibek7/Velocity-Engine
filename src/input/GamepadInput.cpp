#include "input/GamepadInput.h"
#include <cmath>

namespace JJM {
namespace Input {

GamepadManager::GamepadManager() 
    : m_defaultDeadzoneProfile(DeadzoneProfile::Standard())
    , m_deadzone(0.15f) {
}

GamepadManager::~GamepadManager() {
}

void GamepadManager::setDeadzoneProfile(int gamepadId, const DeadzoneProfile& profile) {
    m_deadzoneProfiles[gamepadId] = profile;
}

const DeadzoneProfile& GamepadManager::getDeadzoneProfile(int gamepadId) const {
    auto it = m_deadzoneProfiles.find(gamepadId);
    if (it != m_deadzoneProfiles.end()) {
        return it->second;
    }
    return m_defaultDeadzoneProfile;
}

void GamepadManager::setDefaultDeadzoneProfile(const DeadzoneProfile& profile) {
    m_defaultDeadzoneProfile = profile;
}

float GamepadManager::applyDeadzoneProfile(int gamepadId, float value) const {
    const DeadzoneProfile& profile = getDeadzoneProfile(gamepadId);
    return profile.apply(value);
}

} // namespace Input
} // namespace JJM
