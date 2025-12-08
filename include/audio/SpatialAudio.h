#pragma once

#include "math/Vector2D.h"
#include <memory>
#include <vector>
#include <string>

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
