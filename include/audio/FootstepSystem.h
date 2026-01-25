#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

namespace JJM {

// Forward declarations
class Entity;
namespace Audio { class AudioManager; }

namespace Audio {

/**
 * Surface material types for footstep sounds
 */
enum class SurfaceMaterial {
    CONCRETE,
    WOOD,
    METAL,
    GRASS,
    GRAVEL,
    WATER,
    SNOW,
    MUD,
    SAND,
    CARPET,
    TILE,
    CUSTOM
};

/**
 * Footstep intensity based on movement speed
 */
enum class FootstepIntensity {
    WALK,      // Normal walking
    RUN,       // Running/sprinting
    CROUCH,    // Crouching/sneaking
    JUMP,      // Landing from jump
    SLIDE      // Sliding
};

/**
 * Configuration for a surface material's footstep sounds
 */
struct SurfaceAudioConfig {
    std::vector<std::string> walkSounds;     // Sound file paths for walking
    std::vector<std::string> runSounds;      // Sound file paths for running
    std::vector<std::string> crouchSounds;   // Sound file paths for crouching
    std::vector<std::string> landSounds;     // Sound file paths for landing
    std::vector<std::string> slideSounds;    // Sound file paths for sliding
    
    float volumeMultiplier = 1.0f;           // Base volume multiplier
    float pitchVariation = 0.1f;             // Random pitch variation
    float minInterval = 0.3f;                // Minimum time between footsteps
    float maxInterval = 0.6f;                // Maximum time between footsteps
};

/**
 * Footstep parameters for a specific entity
 */
struct FootstepParams {
    float velocity = 0.0f;                   // Current movement velocity
    bool isGrounded = true;                  // Is entity on ground
    bool isCrouching = false;                // Is entity crouching
    bool isInWater = false;                  // Is entity in water
    SurfaceMaterial currentSurface = SurfaceMaterial::CONCRETE;
    
    float baseVolume = 1.0f;                 // Base volume for this entity
    float basePitch = 1.0f;                  // Base pitch for this entity
    float speedThresholdRun = 5.0f;          // Speed threshold for run sounds
    float speedThresholdWalk = 1.0f;         // Speed threshold for walk sounds
};

/**
 * Individual footstep event data
 */
struct FootstepEvent {
    Entity* entity = nullptr;
    SurfaceMaterial material;
    FootstepIntensity intensity;
    float position[3];                       // World position
    float velocity;
    float volume;
    float pitch;
};

/**
 * System for managing dynamic footstep audio based on surfaces and movement
 */
class FootstepSystem {
public:
    FootstepSystem();
    ~FootstepSystem();
    
    /**
     * Initialize the footstep system
     * @param audioManager Reference to the audio manager
     */
    void initialize(AudioManager* audioManager);
    
    /**
     * Shut down the system and release resources
     */
    void shutdown();
    
    /**
     * Update footstep generation for all tracked entities
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime);
    
    // Entity management
    
    /**
     * Register an entity to generate footsteps
     * @param entity Entity to register
     * @param params Initial footstep parameters
     */
    void registerEntity(Entity* entity, const FootstepParams& params);
    
    /**
     * Unregister an entity from generating footsteps
     * @param entity Entity to unregister
     */
    void unregisterEntity(Entity* entity);
    
    /**
     * Update parameters for a registered entity
     * @param entity Entity to update
     * @param params New parameters
     */
    void updateEntityParams(Entity* entity, const FootstepParams& params);
    
    /**
     * Get current parameters for an entity
     * @param entity Entity to query
     * @return Current footstep parameters, nullptr if not registered
     */
    const FootstepParams* getEntityParams(Entity* entity) const;
    
    // Surface material configuration
    
    /**
     * Configure audio for a surface material
     * @param material Material type
     * @param config Audio configuration
     */
    void configureSurface(SurfaceMaterial material, const SurfaceAudioConfig& config);
    
    /**
     * Get configuration for a surface material
     * @param material Material type
     * @return Audio configuration for the material
     */
    const SurfaceAudioConfig& getSurfaceConfig(SurfaceMaterial material) const;
    
    /**
     * Load surface configurations from file
     * @param filepath Path to configuration file
     * @return True if loaded successfully
     */
    bool loadSurfaceConfigs(const std::string& filepath);
    
    // Manual footstep triggering
    
    /**
     * Manually trigger a footstep event
     * @param event Footstep event data
     */
    void triggerFootstep(const FootstepEvent& event);
    
    /**
     * Trigger landing sound for an entity
     * @param entity Entity that landed
     * @param impactVelocity Velocity at impact
     */
    void triggerLanding(Entity* entity, float impactVelocity);
    
    /**
     * Trigger slide sound for an entity
     * @param entity Entity that is sliding
     * @param slideVelocity Sliding velocity
     */
    void triggerSlide(Entity* entity, float slideVelocity);
    
    // Event callbacks
    
    /**
     * Set callback for footstep events (for custom processing)
     * @param callback Function to call when footstep occurs
     */
    void setFootstepCallback(std::function<void(const FootstepEvent&)> callback);
    
    // Global settings
    
    /**
     * Set master volume for all footsteps
     * @param volume Volume multiplier (0.0 - 1.0)
     */
    void setMasterVolume(float volume);
    
    /**
     * Get master volume
     * @return Current master volume
     */
    float getMasterVolume() const { return m_masterVolume; }
    
    /**
     * Enable or disable footstep generation
     * @param enabled True to enable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * Check if system is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * Enable or disable distance-based volume falloff
     * @param enabled True to enable
     */
    void setDistanceAttenuation(bool enabled) { m_useDistanceAttenuation = enabled; }
    
    /**
     * Set listener position for distance calculations
     * @param position Listener world position [x, y, z]
     */
    void setListenerPosition(const float position[3]);
    
    /**
     * Set maximum audible distance for footsteps
     * @param distance Maximum distance in world units
     */
    void setMaxAudibleDistance(float distance) { m_maxAudibleDistance = distance; }
    
    // Debug and visualization
    
    /**
     * Enable or disable debug visualization
     * @param enabled True to enable
     */
    void setDebugVisualization(bool enabled) { m_debugVisualization = enabled; }
    
    /**
     * Render debug information
     */
    void renderDebug();
    
    /**
     * Get statistics about footstep generation
     */
    struct Stats {
        int totalEntities = 0;           // Total registered entities
        int activeEntities = 0;          // Entities currently generating footsteps
        int footstepsThisFrame = 0;      // Footsteps generated this frame
        int totalFootsteps = 0;          // Total footsteps generated
        float averageInterval = 0.0f;    // Average time between footsteps
    };
    
    /**
     * Get current statistics
     * @return Current statistics
     */
    Stats getStatistics() const;

private:
    struct EntityData {
        FootstepParams params;
        float timeSinceLastStep = 0.0f;
        float nextStepInterval = 0.5f;
        int footstepCount = 0;
        bool leftFoot = true;           // Alternates between left/right
    };
    
    AudioManager* m_audioManager;
    
    // Entity tracking
    std::unordered_map<Entity*, EntityData> m_entities;
    
    // Surface configurations
    std::unordered_map<SurfaceMaterial, SurfaceAudioConfig> m_surfaceConfigs;
    
    // Settings
    float m_masterVolume;
    bool m_enabled;
    bool m_useDistanceAttenuation;
    float m_listenerPosition[3];
    float m_maxAudibleDistance;
    bool m_debugVisualization;
    
    // Callback
    std::function<void(const FootstepEvent&)> m_footstepCallback;
    
    // Statistics
    Stats m_stats;
    
    // Internal methods
    void updateEntity(Entity* entity, EntityData& data, float deltaTime);
    void playFootstepSound(Entity* entity, const EntityData& data);
    FootstepIntensity determineIntensity(const FootstepParams& params) const;
    float calculateVolume(const FootstepParams& params, const float position[3]) const;
    float calculatePitch(const SurfaceAudioConfig& config) const;
    float calculateInterval(const FootstepParams& params, const SurfaceAudioConfig& config) const;
    std::string selectSound(const std::vector<std::string>& sounds) const;
    void initializeDefaultSurfaces();
};

/**
 * Helper functions for surface material detection
 */
namespace SurfaceDetection {
    /**
     * Detect surface material at a world position
     * @param position World position [x, y, z]
     * @return Detected surface material
     */
    SurfaceMaterial detectSurfaceMaterial(const float position[3]);
    
    /**
     * Register custom surface material mapping
     * @param materialName Name of the physical material
     * @param surfaceType Corresponding surface audio type
     */
    void registerMaterialMapping(const std::string& materialName, SurfaceMaterial surfaceType);
}

} // namespace Audio
} // namespace JJM
