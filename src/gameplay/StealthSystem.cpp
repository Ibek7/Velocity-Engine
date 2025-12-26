#include "gameplay/StealthSystem.h"
#include <cmath>
#include <algorithm>

namespace Engine {

StealthSystem::StealthSystem()
    : m_playerCrouching(false)
    , m_playerMovementSpeed(0.0f)
    , m_lightLevel(0.5f)
    , m_visibilityState(VisibilityState::Hidden)
    , m_detectionLevel(0.0f)
    , m_noiseMeter(0.0f)
    , m_crouchReduction(0.5f)
    , m_shadowReduction(0.7f)
    , m_detectionSpeed(1.0f)
{
    m_playerPosition[0] = 0.0f;
    m_playerPosition[1] = 0.0f;
    m_playerPosition[2] = 0.0f;
}

StealthSystem& StealthSystem::getInstance() {
    static StealthSystem instance;
    return instance;
}

void StealthSystem::setPlayerPosition(float x, float y, float z) {
    m_playerPosition[0] = x;
    m_playerPosition[1] = y;
    m_playerPosition[2] = z;
}

void StealthSystem::setPlayerCrouching(bool crouching) {
    m_playerCrouching = crouching;
}

void StealthSystem::setPlayerMovementSpeed(float speed) {
    m_playerMovementSpeed = speed;
}

void StealthSystem::setLightLevel(float level) {
    m_lightLevel = std::max(0.0f, std::min(1.0f, level));
}

void StealthSystem::registerLightSource(int id, float x, float y, float z, float radius, float intensity) {
    LightSource light;
    light.id = id;
    light.position[0] = x;
    light.position[1] = y;
    light.position[2] = z;
    light.radius = radius;
    light.intensity = intensity;
    
    m_lightSources.push_back(light);
}

void StealthSystem::unregisterLightSource(int id) {
    m_lightSources.erase(
        std::remove_if(m_lightSources.begin(), m_lightSources.end(),
            [id](const LightSource& light) { return light.id == id; }),
        m_lightSources.end()
    );
}

void StealthSystem::registerAI(int id, float x, float y, float z, float viewRadius, float viewAngle) {
    AIAgent ai;
    ai.id = id;
    ai.position[0] = x;
    ai.position[1] = y;
    ai.position[2] = z;
    ai.viewRadius = viewRadius;
    ai.viewAngle = viewAngle;
    ai.orientation = 0.0f;
    ai.awarenessLevel = 0.0f;
    
    m_aiAgents.push_back(ai);
}

void StealthSystem::unregisterAI(int id) {
    m_aiAgents.erase(
        std::remove_if(m_aiAgents.begin(), m_aiAgents.end(),
            [id](const AIAgent& ai) { return ai.id == id; }),
        m_aiAgents.end()
    );
}

void StealthSystem::setAIOrientation(int id, float yaw) {
    for (auto& ai : m_aiAgents) {
        if (ai.id == id) {
            ai.orientation = yaw;
            break;
        }
    }
}

void StealthSystem::update(float deltaTime) {
    updateDetection(deltaTime);
    updateAIAwareness(deltaTime);
    
    // Decay noise meter
    m_noiseMeter = std::max(0.0f, m_noiseMeter - deltaTime * 0.5f);
}

void StealthSystem::updateDetection(float deltaTime) {
    float visibility = calculateVisibility();
    
    // Adjust detection level based on visibility
    if (visibility > 0.0f) {
        m_detectionLevel += visibility * m_detectionSpeed * deltaTime;
    } else {
        m_detectionLevel -= deltaTime * 0.3f; // Decay when hidden
    }
    
    m_detectionLevel = std::max(0.0f, std::min(1.0f, m_detectionLevel));
    
    // Update visibility state
    if (m_detectionLevel >= 0.9f) {
        m_visibilityState = VisibilityState::Alert;
    } else if (m_detectionLevel >= 0.6f) {
        m_visibilityState = VisibilityState::Detected;
    } else if (m_detectionLevel >= 0.3f) {
        m_visibilityState = VisibilityState::Suspicious;
    } else {
        m_visibilityState = VisibilityState::Hidden;
    }
}

void StealthSystem::updateAIAwareness(float deltaTime) {
    for (auto& ai : m_aiAgents) {
        if (isInView(ai)) {
            ai.awarenessLevel += deltaTime;
        } else {
            ai.awarenessLevel = std::max(0.0f, ai.awarenessLevel - deltaTime * 0.5f);
        }
        
        ai.awarenessLevel = std::min(1.0f, ai.awarenessLevel);
    }
}

float StealthSystem::calculateVisibility() const {
    float visibility = 1.0f;
    
    // Light level
    float lightAtPlayer = calculateLightLevelAtPlayer();
    visibility *= lightAtPlayer;
    
    // Crouching
    if (m_playerCrouching) {
        visibility *= m_crouchReduction;
    }
    
    // Movement speed (slower = less visible)
    visibility *= std::min(1.0f, m_playerMovementSpeed / 5.0f);
    
    // Shadow bonus
    if (lightAtPlayer < 0.3f) {
        visibility *= m_shadowReduction;
    }
    
    // Noise meter
    visibility += m_noiseMeter * 0.2f;
    
    // Check if any AI can see player
    bool inSight = false;
    for (const auto& ai : m_aiAgents) {
        if (isInView(ai)) {
            inSight = true;
            break;
        }
    }
    
    if (!inSight) {
        return 0.0f;
    }
    
    return std::max(0.0f, std::min(1.0f, visibility));
}

float StealthSystem::calculateLightLevelAtPlayer() const {
    float totalLight = m_lightLevel;
    
    for (const auto& light : m_lightSources) {
        float dist = calculateDistance(
            m_playerPosition[0], m_playerPosition[1], m_playerPosition[2],
            light.position[0], light.position[1], light.position[2]
        );
        
        if (dist < light.radius) {
            float falloff = 1.0f - (dist / light.radius);
            totalLight += light.intensity * falloff;
        }
    }
    
    return std::min(1.0f, totalLight);
}

bool StealthSystem::isInView(const AIAgent& ai) const {
    // Calculate distance
    float dist = calculateDistance(
        ai.position[0], ai.position[1], ai.position[2],
        m_playerPosition[0], m_playerPosition[1], m_playerPosition[2]
    );
    
    if (dist > ai.viewRadius) {
        return false;
    }
    
    // Calculate angle to player
    float dx = m_playerPosition[0] - ai.position[0];
    float dz = m_playerPosition[2] - ai.position[2];
    float angleToPlayer = std::atan2(dz, dx) * 180.0f / 3.14159f;
    
    // Normalize angles
    float angleDiff = std::abs(angleToPlayer - ai.orientation);
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;
    angleDiff = std::abs(angleDiff);
    
    return angleDiff <= ai.viewAngle * 0.5f;
}

float StealthSystem::calculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool StealthSystem::isPlayerVisible() const {
    return m_visibilityState != VisibilityState::Hidden;
}

void StealthSystem::makeNoise(float x, float y, float z, float intensity) {
    float dist = calculateDistance(
        m_playerPosition[0], m_playerPosition[1], m_playerPosition[2],
        x, y, z
    );
    
    // Closer noises are louder
    float noiseFactor = intensity / (1.0f + dist * 0.1f);
    m_noiseMeter = std::min(1.0f, m_noiseMeter + noiseFactor);
}

} // namespace Engine
