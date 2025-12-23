#include "gameplay/WindSystem.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>

namespace Engine {

WindSystem::WindSystem()
    : m_globalDirection(0.0f)
    , m_globalStrength(5.0f)
    , m_targetDirection(0.0f)
    , m_targetStrength(5.0f)
    , m_dynamicEnabled(true)
    , m_windVariation(0.3f)
    , m_windChangeSpeed(0.1f)
    , m_windChangeTimer(0.0f)
    , m_affectParticles(true)
    , m_affectTrees(true)
    , m_affectCloth(true)
{
}

WindSystem& WindSystem::getInstance() {
    static WindSystem instance;
    return instance;
}

void WindSystem::setGlobalWind(float directionDegrees, float strength) {
    m_globalDirection = directionDegrees;
    m_globalStrength = strength;
    m_targetDirection = directionDegrees;
    m_targetStrength = strength;
    
    // Notify callbacks
    for (auto& callback : m_callbacks) {
        callback(directionDegrees, strength);
    }
}

void WindSystem::setGlobalWindDirection(float degrees) {
    m_globalDirection = degrees;
    m_targetDirection = degrees;
}

void WindSystem::setGlobalWindStrength(float strength) {
    m_globalStrength = strength;
    m_targetStrength = strength;
}

void WindSystem::getGlobalWind(float& directionDegrees, float& strength) const {
    directionDegrees = m_globalDirection;
    strength = m_globalStrength;
}

void WindSystem::addWindZone(const WindZone& zone) {
    m_zones.push_back(zone);
}

void WindSystem::removeWindZone(float x, float y, float z) {
    m_zones.erase(
        std::remove_if(m_zones.begin(), m_zones.end(),
            [x, y, z](const WindZone& zone) {
                return std::abs(zone.position[0] - x) < 0.1f &&
                       std::abs(zone.position[1] - y) < 0.1f &&
                       std::abs(zone.position[2] - z) < 0.1f;
            }),
        m_zones.end()
    );
}

void WindSystem::clearWindZones() {
    m_zones.clear();
}

void WindSystem::getWindAt(float x, float y, float z, float& directionX, float& directionY, float& directionZ, float& strength) const {
    // Start with global wind
    float angleRad = m_globalDirection * 3.14159f / 180.0f;
    directionX = std::cos(angleRad);
    directionY = 0.0f;
    directionZ = std::sin(angleRad);
    strength = m_globalStrength;
    
    // Check wind zones
    for (const auto& zone : m_zones) {
        if (isPointInZone(zone, x, y, z)) {
            if (zone.isGlobal) {
                // Add to existing wind
                directionX += zone.direction[0] * zone.strength;
                directionY += zone.direction[1] * zone.strength;
                directionZ += zone.direction[2] * zone.strength;
                strength += zone.strength;
            } else {
                // Override with zone wind
                directionX = zone.direction[0];
                directionY = zone.direction[1];
                directionZ = zone.direction[2];
                strength = zone.strength;
            }
            
            // Add turbulence
            if (zone.turbulence > 0.0f) {
                float noise = (rand() % 100) / 100.0f - 0.5f;
                directionX += noise * zone.turbulence;
                directionZ += noise * zone.turbulence * 0.5f;
            }
        }
    }
    
    // Add gusts
    for (const auto& gust : m_gusts) {
        float dx = x - gust.position[0];
        float dy = y - gust.position[1];
        float dz = z - gust.position[2];
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (dist < gust.radius) {
            float falloff = 1.0f - (dist / gust.radius);
            float gustStrength = gust.strength * falloff;
            
            // Gust direction points away from center
            if (dist > 0.01f) {
                directionX += (dx / dist) * gustStrength;
                directionY += (dy / dist) * gustStrength;
                directionZ += (dz / dist) * gustStrength;
                strength += gustStrength;
            }
        }
    }
    
    // Normalize direction
    float len = std::sqrt(directionX*directionX + directionY*directionY + directionZ*directionZ);
    if (len > 0.01f) {
        directionX /= len;
        directionY /= len;
        directionZ /= len;
    }
}

float WindSystem::getWindStrengthAt(float x, float y, float z) const {
    float dx, dy, dz, strength;
    getWindAt(x, y, z, dx, dy, dz, strength);
    return strength;
}

void WindSystem::update(float deltaTime) {
    if (m_dynamicEnabled) {
        updateDynamicWind(deltaTime);
    }
    
    updateGusts(deltaTime);
}

void WindSystem::updateDynamicWind(float deltaTime) {
    m_windChangeTimer += deltaTime;
    
    // Change wind every few seconds
    if (m_windChangeTimer >= 5.0f) {
        // Random direction change
        float directionChange = ((rand() % 100) / 100.0f - 0.5f) * 45.0f * m_windVariation;
        m_targetDirection = m_globalDirection + directionChange;
        
        // Keep in 0-360 range
        while (m_targetDirection < 0.0f) m_targetDirection += 360.0f;
        while (m_targetDirection >= 360.0f) m_targetDirection -= 360.0f;
        
        // Random strength change
        float strengthChange = ((rand() % 100) / 100.0f - 0.5f) * m_windVariation;
        m_targetStrength = m_globalStrength + strengthChange * 5.0f;
        m_targetStrength = std::max(0.0f, m_targetStrength);
        
        m_windChangeTimer = 0.0f;
    }
    
    // Smoothly interpolate to target
    float t = deltaTime * m_windChangeSpeed;
    
    // Interpolate direction
    float dirDiff = m_targetDirection - m_globalDirection;
    if (dirDiff > 180.0f) dirDiff -= 360.0f;
    if (dirDiff < -180.0f) dirDiff += 360.0f;
    m_globalDirection += dirDiff * t;
    
    while (m_globalDirection < 0.0f) m_globalDirection += 360.0f;
    while (m_globalDirection >= 360.0f) m_globalDirection -= 360.0f;
    
    // Interpolate strength
    m_globalStrength += (m_targetStrength - m_globalStrength) * t;
}

void WindSystem::updateGusts(float deltaTime) {
    // Update existing gusts
    for (auto& gust : m_gusts) {
        gust.elapsed += deltaTime;
        
        // Fade out strength
        float t = gust.elapsed / gust.duration;
        if (t < 1.0f) {
            gust.strength *= (1.0f - t * 0.1f);
        }
    }
    
    // Remove expired gusts
    m_gusts.erase(
        std::remove_if(m_gusts.begin(), m_gusts.end(),
            [](const Gust& g) { return g.elapsed >= g.duration; }),
        m_gusts.end()
    );
}

void WindSystem::createGust(float x, float y, float z, float radius, float strength, float duration) {
    Gust gust;
    gust.position[0] = x;
    gust.position[1] = y;
    gust.position[2] = z;
    gust.radius = radius;
    gust.strength = strength;
    gust.duration = duration;
    gust.elapsed = 0.0f;
    
    m_gusts.push_back(gust);
}

void WindSystem::onWindChange(WindChangeCallback callback) {
    m_callbacks.push_back(callback);
}

bool WindSystem::isPointInZone(const WindZone& zone, float x, float y, float z) const {
    if (zone.isGlobal) {
        return true;
    }
    
    // Check if point is within axis-aligned bounding box
    float halfX = zone.size[0] * 0.5f;
    float halfY = zone.size[1] * 0.5f;
    float halfZ = zone.size[2] * 0.5f;
    
    return x >= zone.position[0] - halfX && x <= zone.position[0] + halfX &&
           y >= zone.position[1] - halfY && y <= zone.position[1] + halfY &&
           z >= zone.position[2] - halfZ && z <= zone.position[2] + halfZ;
}

} // namespace Engine
