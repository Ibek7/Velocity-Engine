#include "audio/SpatialAudio.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Audio {

SpatialAudioSystem::SpatialAudioSystem()
    : dopplerFactor(1.0f), speedOfSound(343.3f), masterVolume(1.0f), nextSoundHandle(1) {}

SpatialAudioSystem::~SpatialAudioSystem() {}

void SpatialAudioSystem::setListener(const AudioListener& listener) {
    this->listener = listener;
}

int SpatialAudioSystem::playSound(const std::string& soundId, const Math::Vector2D& position, bool looping) {
    auto sound = std::make_unique<SpatialSound>();
    sound->soundId = soundId;
    sound->position = position;
    sound->isLooping = looping;
    sound->isPlaying = true;
    
    int handle = nextSoundHandle++;
    spatialSounds.push_back(std::move(sound));
    
    return handle;
}

void SpatialAudioSystem::stopSound(int soundHandle) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->isPlaying = false;
    }
}

void SpatialAudioSystem::pauseSound(int soundHandle) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->isPlaying = false;
    }
}

void SpatialAudioSystem::resumeSound(int soundHandle) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->isPlaying = true;
    }
}

void SpatialAudioSystem::setSoundPosition(int soundHandle, const Math::Vector2D& position) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->position = position;
    }
}

void SpatialAudioSystem::setSoundVelocity(int soundHandle, const Math::Vector2D& velocity) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->velocity = velocity;
    }
}

void SpatialAudioSystem::setSoundVolume(int soundHandle, float volume) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->volume = volume;
    }
}

void SpatialAudioSystem::setSoundPitch(int soundHandle, float pitch) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->pitch = pitch;
    }
}

void SpatialAudioSystem::setMaxDistance(int soundHandle, float distance) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->maxDistance = distance;
    }
}

void SpatialAudioSystem::setReferenceDistance(int soundHandle, float distance) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->referenceDistance = distance;
    }
}

void SpatialAudioSystem::setRolloffFactor(int soundHandle, float factor) {
    SpatialSound* sound = getSoundByHandle(soundHandle);
    if (sound) {
        sound->rolloffFactor = factor;
    }
}

void SpatialAudioSystem::update(float deltaTime) {
    (void)deltaTime;
    
    for (auto& sound : spatialSounds) {
        if (sound && sound->isPlaying) {
            updateSound(sound.get());
        }
    }
    
    spatialSounds.erase(
        std::remove_if(spatialSounds.begin(), spatialSounds.end(),
            [](const std::unique_ptr<SpatialSound>& s) {
                return !s->isPlaying && !s->isLooping;
            }),
        spatialSounds.end()
    );
}

SpatialSound* SpatialAudioSystem::getSoundByHandle(int handle) {
    int index = handle - 1;
    if (index >= 0 && index < static_cast<int>(spatialSounds.size())) {
        return spatialSounds[index].get();
    }
    return nullptr;
}

float SpatialAudioSystem::calculateDistance(const Math::Vector2D& soundPos) const {
    float dx = soundPos.x - listener.position.x;
    float dy = soundPos.y - listener.position.y;
    return std::sqrt(dx * dx + dy * dy);
}

float SpatialAudioSystem::calculateAttenuation(float distance, float maxDist, float refDist, float rolloff) const {
    if (distance <= refDist) {
        return 1.0f;
    }
    
    if (distance >= maxDist) {
        return 0.0f;
    }
    
    float attenuation = refDist / (refDist + rolloff * (distance - refDist));
    return std::max(0.0f, std::min(1.0f, attenuation));
}

float SpatialAudioSystem::calculateDopplerPitch(const SpatialSound* sound) const {
    if (dopplerFactor <= 0.0f) {
        return 1.0f;
    }
    
    Math::Vector2D toListener = listener.position - sound->position;
    float distance = toListener.magnitude();
    
    if (distance < 0.01f) {
        return 1.0f;
    }
    
    toListener = toListener / distance;
    
    float sourceVel = sound->velocity.dot(toListener);
    float listenerVel = listener.velocity.dot(toListener);
    
    float velocityDiff = listenerVel - sourceVel;
    float pitchShift = (speedOfSound - dopplerFactor * velocityDiff) / speedOfSound;
    
    return std::max(0.5f, std::min(2.0f, pitchShift));
}

float SpatialAudioSystem::calculatePanning(const Math::Vector2D& soundPos) const {
    Math::Vector2D toSound = soundPos - listener.position;
    
    float angle = std::atan2(toSound.y, toSound.x);
    float listenerAngle = listener.orientation;
    
    float relativeAngle = angle - listenerAngle;
    
    while (relativeAngle > M_PI) relativeAngle -= 2.0f * M_PI;
    while (relativeAngle < -M_PI) relativeAngle += 2.0f * M_PI;
    
    float pan = std::sin(relativeAngle);
    return std::max(-1.0f, std::min(1.0f, pan));
}

void SpatialAudioSystem::updateSound(SpatialSound* sound) {
    if (!sound || !sound->is3D) {
        return;
    }
    
    float distance = calculateDistance(sound->position);
    float attenuation = calculateAttenuation(distance, sound->maxDistance, 
                                             sound->referenceDistance, sound->rolloffFactor);
    
    float dopplerPitch = calculateDopplerPitch(sound);
    float panning = calculatePanning(sound->position);
    
    float finalVolume = sound->volume * attenuation * masterVolume;
    float finalPitch = sound->pitch * dopplerPitch;
    
    (void)finalVolume;
    (void)finalPitch;
    (void)panning;
}

ReverbZone::ReverbZone(const Math::Vector2D& center, float radius)
    : center(center), radius(radius), reverbLevel(0.5f), decayTime(1.0f) {}

bool ReverbZone::contains(const Math::Vector2D& point) const {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    float distSq = dx * dx + dy * dy;
    return distSq <= radius * radius;
}

float ReverbZone::getInfluence(const Math::Vector2D& point) const {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    
    if (dist >= radius) {
        return 0.0f;
    }
    
    float influence = 1.0f - (dist / radius);
    return influence * reverbLevel;
}

AudioOccluder::AudioOccluder(const Math::Vector2D& start, const Math::Vector2D& end, float occlusionFactor)
    : start(start), end(end), occlusionFactor(occlusionFactor) {}

bool AudioOccluder::intersectsRay(const Math::Vector2D& rayStart, const Math::Vector2D& rayEnd) const {
    Math::Vector2D r = rayEnd - rayStart;
    Math::Vector2D s = end - start;
    
    float rxs = r.x * s.y - r.y * s.x;
    
    if (std::abs(rxs) < 0.0001f) {
        return false;
    }
    
    Math::Vector2D qp = start - rayStart;
    float t = (qp.x * s.y - qp.y * s.x) / rxs;
    float u = (qp.x * r.y - qp.y * r.x) / rxs;
    
    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}

} // namespace Audio
} // namespace JJM
