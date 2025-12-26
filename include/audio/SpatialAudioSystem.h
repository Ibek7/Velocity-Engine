#pragma once

#include <string>
#include <vector>
#include <map>

// 3D spatial audio with HRTF and reverb
namespace Engine {

enum class AttenuationModel {
    Linear,
    Inverse,
    Exponential,
    Logarithmic
};

enum class ReverbPreset {
    None,
    Room,
    Cave,
    Cathedral,
    Underwater,
    Forest,
    Hallway,
    Custom
};

struct AudioEmitter {
    std::string name;
    int emitterId;
    
    float x, y, z;              // Position
    float velocityX, velocityY, velocityZ;
    
    float volume;
    float pitch;
    float minDistance;
    float maxDistance;
    float rolloffFactor;
    
    bool isLooping;
    bool is3D;
    bool isDoppler;
    
    AttenuationModel attenuationModel;
};

struct AudioListener {
    float x, y, z;              // Position
    float forwardX, forwardY, forwardZ;
    float upX, upY, upZ;
    float velocityX, velocityY, velocityZ;
    
    float volume;
};

struct ReverbZone {
    std::string name;
    float x, y, z;
    float radius;
    
    ReverbPreset preset;
    float decayTime;
    float density;
    float diffusion;
    float gain;
    float gainHF;
    float gainLF;
};

class SpatialAudioSystem {
public:
    static SpatialAudioSystem& getInstance();

    void initialize();
    void shutdown();
    void update(float deltaTime);

    // Listener
    void setListenerPosition(float x, float y, float z);
    void setListenerOrientation(float forwardX, float forwardY, float forwardZ,
                                float upX, float upY, float upZ);
    void setListenerVelocity(float x, float y, float z);
    void setListenerVolume(float volume);
    
    const AudioListener& getListener() const { return m_listener; }

    // Emitters
    int createEmitter(const std::string& name);
    void destroyEmitter(int emitterId);
    void setEmitterPosition(int emitterId, float x, float y, float z);
    void setEmitterVelocity(int emitterId, float x, float y, float z);
    void setEmitterVolume(int emitterId, float volume);
    void setEmitterPitch(int emitterId, float pitch);
    void setEmitterDistanceRange(int emitterId, float minDistance, float maxDistance);
    void setEmitterAttenuation(int emitterId, AttenuationModel model, float rolloff);
    void setEmitterLooping(int emitterId, bool looping);
    void enableDoppler(int emitterId, bool enable);
    
    void playEmitter(int emitterId, const std::string& audioFile);
    void stopEmitter(int emitterId);
    void pauseEmitter(int emitterId);
    
    AudioEmitter* getEmitter(int emitterId);

    // Reverb zones
    int createReverbZone(const std::string& name);
    void setReverbZonePosition(int zoneId, float x, float y, float z);
    void setReverbZoneRadius(int zoneId, float radius);
    void setReverbZonePreset(int zoneId, ReverbPreset preset);
    void setReverbZoneParameters(int zoneId, float decayTime, float density, float diffusion);
    void destroyReverbZone(int zoneId);

    // HRTF (Head-Related Transfer Function)
    void enableHRTF(bool enable) { m_hrtfEnabled = enable; }
    bool isHRTFEnabled() const { return m_hrtfEnabled; }
    void loadHRTFProfile(const std::string& profilePath);

    // Occlusion
    void setOcclusion(int emitterId, float occlusion);
    void enableOcclusionRaycast(bool enable) { m_occlusionEnabled = enable; }
    bool isOcclusionEnabled() const { return m_occlusionEnabled; }

    // Global settings
    void setSpeedOfSound(float speed) { m_speedOfSound = speed; }
    void setDopplerFactor(float factor) { m_dopplerFactor = factor; }
    void setMasterVolume(float volume) { m_masterVolume = volume; }
    
    float getSpeedOfSound() const { return m_speedOfSound; }
    float getDopplerFactor() const { return m_dopplerFactor; }

    // Distance calculations
    float calculateDistance(const AudioEmitter& emitter) const;
    float calculateAttenuation(const AudioEmitter& emitter) const;
    float calculateDopplerShift(const AudioEmitter& emitter) const;
    
    // Query
    int getEmitterCount() const { return static_cast<int>(m_emitters.size()); }
    int getReverbZoneCount() const { return static_cast<int>(m_reverbZones.size()); }

private:
    SpatialAudioSystem();
    SpatialAudioSystem(const SpatialAudioSystem&) = delete;
    SpatialAudioSystem& operator=(const SpatialAudioSystem&) = delete;

    void updateEmitterAudio(AudioEmitter& emitter, float deltaTime);
    void applyReverb(AudioEmitter& emitter);
    float calculateOcclusion(const AudioEmitter& emitter);
    
    ReverbZone* findActiveReverbZone(const AudioEmitter& emitter);

    AudioListener m_listener;
    std::map<int, AudioEmitter> m_emitters;
    std::map<int, ReverbZone> m_reverbZones;
    
    int m_nextEmitterId;
    int m_nextZoneId;
    
    bool m_hrtfEnabled;
    bool m_occlusionEnabled;
    
    float m_speedOfSound;       // Units per second (default: 343.0)
    float m_dopplerFactor;      // Doppler effect intensity
    float m_masterVolume;
    
    std::string m_hrtfProfile;
};

} // namespace Engine
