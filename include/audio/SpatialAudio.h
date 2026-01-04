#pragma once

#include "math/Vector2D.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <optional>

namespace JJM {
namespace Audio {

struct AudioListener {
    Math::Vector2D position;
    Math::Vector2D velocity;
    float orientation;
    
    AudioListener() : position(0, 0), velocity(0, 0), orientation(0.0f) {}
};

struct SpatialSound {
    std::string soundId;
    Math::Vector2D position;
    Math::Vector2D velocity;
    
    float volume;
    float pitch;
    float maxDistance;
    float referenceDistance;
    float rolloffFactor;
    
    bool isLooping;
    bool is3D;
    bool isPlaying;
    
    SpatialSound()
        : position(0, 0), velocity(0, 0),
          volume(1.0f), pitch(1.0f),
          maxDistance(100.0f), referenceDistance(1.0f),
          rolloffFactor(1.0f),
          isLooping(false), is3D(true), isPlaying(false) {}
};

// =============================================================================
// Audio Occlusion System
// =============================================================================

/**
 * @brief Material types for audio propagation
 */
enum class AudioMaterialType {
    Air,
    Glass,
    Wood,
    Concrete,
    Metal,
    Brick,
    Fabric,
    Water,
    Vegetation,
    Custom
};

/**
 * @brief Audio material properties
 */
struct AudioMaterial {
    AudioMaterialType type;
    std::string name;
    
    // Absorption coefficients per frequency band (125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz)
    float absorption[6];
    
    // Transmission loss (how much sound passes through)
    float transmissionLoss;
    
    // Scattering coefficient (diffuse reflection)
    float scattering;
    
    // Density affects low frequency transmission
    float density;
    
    AudioMaterial()
        : type(AudioMaterialType::Concrete)
        , transmissionLoss(0.8f)
        , scattering(0.1f)
        , density(1.0f)
    {
        for (int i = 0; i < 6; ++i) absorption[i] = 0.1f;
    }
    
    static AudioMaterial Glass();
    static AudioMaterial Wood();
    static AudioMaterial Concrete();
    static AudioMaterial Metal();
    static AudioMaterial Fabric();
};

/**
 * @brief Occlusion query result
 */
struct OcclusionResult {
    float directAttenuation;        // Attenuation of direct path
    float lowPassCutoff;            // Low-pass filter cutoff frequency
    float reverbContribution;       // How much reverb to add
    int occluderCount;              // Number of occluders hit
    float totalThickness;           // Combined material thickness
    bool isFullyOccluded;           // No direct path at all
    
    OcclusionResult()
        : directAttenuation(1.0f)
        , lowPassCutoff(20000.0f)
        , reverbContribution(0.0f)
        , occluderCount(0)
        , totalThickness(0.0f)
        , isFullyOccluded(false)
    {}
};

/**
 * @brief Audio ray for occlusion testing
 */
struct AudioRay {
    Math::Vector2D origin;
    Math::Vector2D direction;
    float maxDistance;
    int bounceCount;
    float energy;
    
    AudioRay()
        : maxDistance(100.0f)
        , bounceCount(0)
        , energy(1.0f)
    {}
};

/**
 * @brief Ray hit information
 */
struct AudioRayHit {
    Math::Vector2D point;
    Math::Vector2D normal;
    float distance;
    AudioMaterial material;
    void* userData;
    
    AudioRayHit() : distance(0.0f), userData(nullptr) {}
};

/**
 * @brief Audio occluder geometry (polygon-based)
 */
class AudioOccluderGeometry {
private:
    std::vector<Math::Vector2D> m_vertices;
    AudioMaterial m_material;
    float m_thickness;
    bool m_twoSided;
    bool m_enabled;
    std::string m_id;
    
public:
    AudioOccluderGeometry(const std::string& id);
    
    // Geometry
    void setVertices(const std::vector<Math::Vector2D>& vertices);
    void addVertex(const Math::Vector2D& vertex);
    void clearVertices();
    const std::vector<Math::Vector2D>& getVertices() const { return m_vertices; }
    
    // Properties
    void setMaterial(const AudioMaterial& material) { m_material = material; }
    const AudioMaterial& getMaterial() const { return m_material; }
    void setThickness(float thickness) { m_thickness = thickness; }
    float getThickness() const { return m_thickness; }
    void setTwoSided(bool twoSided) { m_twoSided = twoSided; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    const std::string& getId() const { return m_id; }
    
    // Ray testing
    bool raycast(const AudioRay& ray, AudioRayHit& hit) const;
    
    // AABB for broad phase
    void getBounds(Math::Vector2D& min, Math::Vector2D& max) const;
};

/**
 * @brief Audio portal for sound propagation between zones
 */
struct AudioPortal {
    std::string id;
    Math::Vector2D start;
    Math::Vector2D end;
    float openness;                 // 0 = closed, 1 = fully open
    std::string zoneA;
    std::string zoneB;
    
    // Filter applied to sound passing through
    float lowPassFactor;
    float highPassFactor;
    
    AudioPortal()
        : openness(1.0f)
        , lowPassFactor(1.0f)
        , highPassFactor(1.0f)
    {}
    
    Math::Vector2D getCenter() const;
    float getWidth() const;
    bool isOpen() const { return openness > 0.0f; }
};

/**
 * @brief Audio zone for room-based acoustics
 */
class AudioZone {
private:
    std::string m_id;
    std::vector<Math::Vector2D> m_boundary;
    
    // Room acoustics
    float m_reverbTime;             // RT60
    float m_reverbWetness;
    float m_roomSize;
    float m_diffusion;
    float m_highFreqDecay;
    float m_lowFreqDecay;
    
    // Early reflections
    float m_earlyReflections;
    float m_earlyDelay;
    
    // Zone properties
    float m_priority;
    bool m_enabled;
    
    // Connected portals
    std::vector<std::string> m_portalIds;
    
public:
    AudioZone(const std::string& id);
    
    // Boundary
    void setBoundary(const std::vector<Math::Vector2D>& boundary);
    bool containsPoint(const Math::Vector2D& point) const;
    float getDistanceToEdge(const Math::Vector2D& point) const;
    
    // Reverb settings
    void setReverbTime(float rt60) { m_reverbTime = rt60; }
    float getReverbTime() const { return m_reverbTime; }
    void setReverbWetness(float wetness) { m_reverbWetness = wetness; }
    void setRoomSize(float size) { m_roomSize = size; }
    void setDiffusion(float diffusion) { m_diffusion = diffusion; }
    
    // Early reflections
    void setEarlyReflections(float level) { m_earlyReflections = level; }
    void setEarlyDelay(float delay) { m_earlyDelay = delay; }
    
    // Properties
    const std::string& getId() const { return m_id; }
    void setPriority(float priority) { m_priority = priority; }
    float getPriority() const { return m_priority; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Portals
    void addPortal(const std::string& portalId);
    void removePortal(const std::string& portalId);
    const std::vector<std::string>& getPortals() const { return m_portalIds; }
};

/**
 * @brief Advanced audio occlusion system
 */
class AudioOcclusionSystem {
private:
    static AudioOcclusionSystem* instance;
    
    // Occluders
    std::unordered_map<std::string, std::unique_ptr<AudioOccluderGeometry>> m_occluders;
    
    // Zones and portals
    std::unordered_map<std::string, std::unique_ptr<AudioZone>> m_zones;
    std::unordered_map<std::string, AudioPortal> m_portals;
    
    // Ray tracing settings
    int m_maxRayBounces;
    int m_raysPerSource;
    float m_raySpreadAngle;
    bool m_enableDiffraction;
    bool m_enableTransmission;
    
    // Caching
    struct OcclusionCache {
        int soundHandle;
        OcclusionResult result;
        float timestamp;
    };
    std::unordered_map<int, OcclusionCache> m_cache;
    float m_cacheLifetime;
    
    // Statistics
    struct Stats {
        int totalRaysCast;
        int occlusionQueriesPerFrame;
        int cacheHits;
        int cacheMisses;
        float averageOcclusion;
    };
    mutable Stats m_stats;
    
    AudioOcclusionSystem();
    
public:
    static AudioOcclusionSystem* getInstance();
    static void cleanup();
    
    // Occluder management
    AudioOccluderGeometry* createOccluder(const std::string& id);
    void destroyOccluder(const std::string& id);
    AudioOccluderGeometry* getOccluder(const std::string& id);
    void clearOccluders();
    
    // Zone management
    AudioZone* createZone(const std::string& id);
    void destroyZone(const std::string& id);
    AudioZone* getZone(const std::string& id);
    AudioZone* getZoneAtPoint(const Math::Vector2D& point);
    void clearZones();
    
    // Portal management
    void addPortal(const AudioPortal& portal);
    void removePortal(const std::string& id);
    AudioPortal* getPortal(const std::string& id);
    void setPortalOpenness(const std::string& id, float openness);
    
    // Occlusion queries
    OcclusionResult queryOcclusion(const Math::Vector2D& source, const Math::Vector2D& listener);
    OcclusionResult queryOcclusionCached(int soundHandle, const Math::Vector2D& source, 
                                          const Math::Vector2D& listener);
    
    // Ray tracing
    bool raycast(const AudioRay& ray, AudioRayHit& hit) const;
    std::vector<AudioRayHit> raycastAll(const AudioRay& ray) const;
    std::vector<Math::Vector2D> findDiffractionPaths(const Math::Vector2D& source, 
                                                       const Math::Vector2D& listener) const;
    
    // Path finding (for propagation through portals)
    std::vector<std::string> findSoundPath(const std::string& sourceZone, 
                                            const std::string& listenerZone) const;
    float calculatePathAttenuation(const std::vector<std::string>& path) const;
    
    // Configuration
    void setMaxRayBounces(int bounces) { m_maxRayBounces = bounces; }
    void setRaysPerSource(int rays) { m_raysPerSource = rays; }
    void setEnableDiffraction(bool enable) { m_enableDiffraction = enable; }
    void setEnableTransmission(bool enable) { m_enableTransmission = enable; }
    void setCacheLifetime(float seconds) { m_cacheLifetime = seconds; }
    
    // Cache management
    void invalidateCache();
    void invalidateCacheForSound(int soundHandle);
    void update(float deltaTime);
    
    // Statistics
    const Stats& getStats() const { return m_stats; }
    void resetStats();
};

class SpatialAudioSystem {
public:
    SpatialAudioSystem();
    ~SpatialAudioSystem();
    
    void setListener(const AudioListener& listener);
    const AudioListener& getListener() const { return listener; }
    
    int playSound(const std::string& soundId, const Math::Vector2D& position, bool looping = false);
    void stopSound(int soundHandle);
    void pauseSound(int soundHandle);
    void resumeSound(int soundHandle);
    
    void setSoundPosition(int soundHandle, const Math::Vector2D& position);
    void setSoundVelocity(int soundHandle, const Math::Vector2D& velocity);
    void setSoundVolume(int soundHandle, float volume);
    void setSoundPitch(int soundHandle, float pitch);
    
    void setMaxDistance(int soundHandle, float distance);
    void setReferenceDistance(int soundHandle, float distance);
    void setRolloffFactor(int soundHandle, float factor);
    
    void update(float deltaTime);
    
    void setDopplerFactor(float factor) { dopplerFactor = factor; }
    float getDopplerFactor() const { return dopplerFactor; }
    
    void setSpeedOfSound(float speed) { speedOfSound = speed; }
    float getSpeedOfSound() const { return speedOfSound; }
    
    void setMasterVolume(float volume) { masterVolume = volume; }
    float getMasterVolume() const { return masterVolume; }

private:
    AudioListener listener;
    std::vector<std::unique_ptr<SpatialSound>> spatialSounds;
    
    float dopplerFactor;
    float speedOfSound;
    float masterVolume;
    
    int nextSoundHandle;
    
    SpatialSound* getSoundByHandle(int handle);
    
    float calculateDistance(const Math::Vector2D& soundPos) const;
    float calculateAttenuation(float distance, float maxDist, float refDist, float rolloff) const;
    float calculateDopplerPitch(const SpatialSound* sound) const;
    float calculatePanning(const Math::Vector2D& soundPos) const;
    
    void updateSound(SpatialSound* sound);
};

class ReverbZone {
public:
    ReverbZone(const Math::Vector2D& center, float radius);
    
    void setCenter(const Math::Vector2D& center) { this->center = center; }
    const Math::Vector2D& getCenter() const { return center; }
    
    void setRadius(float radius) { this->radius = radius; }
    float getRadius() const { return radius; }
    
    void setReverbLevel(float level) { reverbLevel = level; }
    float getReverbLevel() const { return reverbLevel; }
    
    void setDecayTime(float time) { decayTime = time; }
    float getDecayTime() const { return decayTime; }
    
    bool contains(const Math::Vector2D& point) const;
    float getInfluence(const Math::Vector2D& point) const;

private:
    Math::Vector2D center;
    float radius;
    float reverbLevel;
    float decayTime;
};

class AudioOccluder {
public:
    AudioOccluder(const Math::Vector2D& start, const Math::Vector2D& end, float occlusionFactor);
    
    bool intersectsRay(const Math::Vector2D& rayStart, const Math::Vector2D& rayEnd) const;
    float getOcclusionFactor() const { return occlusionFactor; }

private:
    Math::Vector2D start;
    Math::Vector2D end;
    float occlusionFactor;
};

} // namespace Audio
} // namespace JJM
