#pragma once

#include <string>
#include <vector>
#include <memory>

// Stealth mechanics system
namespace Engine {

enum class VisibilityState {
    Hidden,
    Suspicious,
    Detected,
    Alert
};

class StealthSystem {
public:
    static StealthSystem& getInstance();

    // Stealth state
    void setPlayerPosition(float x, float y, float z);
    void setPlayerCrouching(bool crouching);
    void setPlayerMovementSpeed(float speed);
    
    // Light/Shadow
    void setLightLevel(float level); // 0.0 = dark, 1.0 = bright
    void registerLightSource(int id, float x, float y, float z, float radius, float intensity);
    void unregisterLightSource(int id);
    
    // AI awareness
    void registerAI(int id, float x, float y, float z, float viewRadius, float viewAngle);
    void unregisterAI(int id);
    void setAIOrientation(int id, float yaw);
    
    // Update
    void update(float deltaTime);
    
    // Query
    VisibilityState getVisibilityState() const { return m_visibilityState; }
    float getDetectionLevel() const { return m_detectionLevel; }
    bool isPlayerVisible() const;
    float getNoiseMeter() const { return m_noiseMeter; }
    
    // Noise generation
    void makeNoise(float x, float y, float z, float intensity);
    
    // Configuration
    void setCrouchVisibilityReduction(float reduction) { m_crouchReduction = reduction; }
    void setShadowVisibilityReduction(float reduction) { m_shadowReduction = reduction; }
    void setDetectionSpeed(float speed) { m_detectionSpeed = speed; }

private:
    StealthSystem();
    StealthSystem(const StealthSystem&) = delete;
    StealthSystem& operator=(const StealthSystem&) = delete;

    struct LightSource {
        int id;
        float position[3];
        float radius;
        float intensity;
    };

    struct AIAgent {
        int id;
        float position[3];
        float viewRadius;
        float viewAngle;
        float orientation;
        float awarenessLevel;
    };

    void updateDetection(float deltaTime);
    void updateAIAwareness(float deltaTime);
    float calculateVisibility() const;
    float calculateLightLevelAtPlayer() const;
    bool isInView(const AIAgent& ai) const;
    float calculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const;

    float m_playerPosition[3];
    bool m_playerCrouching;
    float m_playerMovementSpeed;
    float m_lightLevel;
    
    std::vector<LightSource> m_lightSources;
    std::vector<AIAgent> m_aiAgents;
    
    VisibilityState m_visibilityState;
    float m_detectionLevel;
    float m_noiseMeter;
    
    float m_crouchReduction;
    float m_shadowReduction;
    float m_detectionSpeed;
};

} // namespace Engine
