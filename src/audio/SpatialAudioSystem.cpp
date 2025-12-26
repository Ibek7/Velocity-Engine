#include "audio/SpatialAudioSystem.h"
#include <cmath>
#include <algorithm>

namespace Engine {

SpatialAudioSystem::SpatialAudioSystem()
    : m_nextEmitterId(1)
    , m_nextZoneId(1)
    , m_hrtfEnabled(false)
    , m_occlusionEnabled(false)
    , m_speedOfSound(343.0f)
    , m_dopplerFactor(1.0f)
    , m_masterVolume(1.0f)
{
    // Initialize listener
    m_listener.x = m_listener.y = m_listener.z = 0.0f;
    m_listener.forwardX = 0.0f;
    m_listener.forwardY = 0.0f;
    m_listener.forwardZ = 1.0f;
    m_listener.upX = 0.0f;
    m_listener.upY = 1.0f;
    m_listener.upZ = 0.0f;
    m_listener.velocityX = m_listener.velocityY = m_listener.velocityZ = 0.0f;
    m_listener.volume = 1.0f;
}

SpatialAudioSystem& SpatialAudioSystem::getInstance() {
    static SpatialAudioSystem instance;
    return instance;
}

void SpatialAudioSystem::initialize() {
    // Initialize audio backend (OpenAL, FMOD, etc.)
}

void SpatialAudioSystem::shutdown() {
    m_emitters.clear();
    m_reverbZones.clear();
}

void SpatialAudioSystem::update(float deltaTime) {
    // Update all emitters
    for (auto& pair : m_emitters) {
        updateEmitterAudio(pair.second, deltaTime);
    }
}

void SpatialAudioSystem::setListenerPosition(float x, float y, float z) {
    m_listener.x = x;
    m_listener.y = y;
    m_listener.z = z;
}

void SpatialAudioSystem::setListenerOrientation(float forwardX, float forwardY, float forwardZ,
                                                 float upX, float upY, float upZ) {
    m_listener.forwardX = forwardX;
    m_listener.forwardY = forwardY;
    m_listener.forwardZ = forwardZ;
    m_listener.upX = upX;
    m_listener.upY = upY;
    m_listener.upZ = upZ;
}

void SpatialAudioSystem::setListenerVelocity(float x, float y, float z) {
    m_listener.velocityX = x;
    m_listener.velocityY = y;
    m_listener.velocityZ = z;
}

void SpatialAudioSystem::setListenerVolume(float volume) {
    m_listener.volume = std::clamp(volume, 0.0f, 1.0f);
}

int SpatialAudioSystem::createEmitter(const std::string& name) {
    int emitterId = m_nextEmitterId++;
    
    AudioEmitter emitter;
    emitter.name = name;
    emitter.emitterId = emitterId;
    emitter.x = emitter.y = emitter.z = 0.0f;
    emitter.velocityX = emitter.velocityY = emitter.velocityZ = 0.0f;
    emitter.volume = 1.0f;
    emitter.pitch = 1.0f;
    emitter.minDistance = 1.0f;
    emitter.maxDistance = 100.0f;
    emitter.rolloffFactor = 1.0f;
    emitter.isLooping = false;
    emitter.is3D = true;
    emitter.isDoppler = true;
    emitter.attenuationModel = AttenuationModel::Inverse;
    
    m_emitters[emitterId] = emitter;
    return emitterId;
}

void SpatialAudioSystem::destroyEmitter(int emitterId) {
    m_emitters.erase(emitterId);
}

void SpatialAudioSystem::setEmitterPosition(int emitterId, float x, float y, float z) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.x = x;
        it->second.y = y;
        it->second.z = z;
    }
}

void SpatialAudioSystem::setEmitterVelocity(int emitterId, float x, float y, float z) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.velocityX = x;
        it->second.velocityY = y;
        it->second.velocityZ = z;
    }
}

void SpatialAudioSystem::setEmitterVolume(int emitterId, float volume) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.volume = std::clamp(volume, 0.0f, 1.0f);
    }
}

void SpatialAudioSystem::setEmitterPitch(int emitterId, float pitch) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.pitch = std::max(0.01f, pitch);
    }
}

void SpatialAudioSystem::setEmitterDistanceRange(int emitterId, float minDistance, float maxDistance) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.minDistance = std::max(0.0f, minDistance);
        it->second.maxDistance = std::max(minDistance, maxDistance);
    }
}

void SpatialAudioSystem::setEmitterAttenuation(int emitterId, AttenuationModel model, float rolloff) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.attenuationModel = model;
        it->second.rolloffFactor = std::max(0.0f, rolloff);
    }
}

void SpatialAudioSystem::setEmitterLooping(int emitterId, bool looping) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.isLooping = looping;
    }
}

void SpatialAudioSystem::enableDoppler(int emitterId, bool enable) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        it->second.isDoppler = enable;
    }
}

void SpatialAudioSystem::playEmitter(int emitterId, const std::string& audioFile) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        // Load and play audio file
    }
}

void SpatialAudioSystem::stopEmitter(int emitterId) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        // Stop audio playback
    }
}

void SpatialAudioSystem::pauseEmitter(int emitterId) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        // Pause audio playback
    }
}

AudioEmitter* SpatialAudioSystem::getEmitter(int emitterId) {
    auto it = m_emitters.find(emitterId);
    return it != m_emitters.end() ? &it->second : nullptr;
}

int SpatialAudioSystem::createReverbZone(const std::string& name) {
    int zoneId = m_nextZoneId++;
    
    ReverbZone zone;
    zone.name = name;
    zone.x = zone.y = zone.z = 0.0f;
    zone.radius = 10.0f;
    zone.preset = ReverbPreset::Room;
    zone.decayTime = 1.0f;
    zone.density = 1.0f;
    zone.diffusion = 1.0f;
    zone.gain = 0.32f;
    zone.gainHF = 0.89f;
    zone.gainLF = 1.0f;
    
    m_reverbZones[zoneId] = zone;
    return zoneId;
}

void SpatialAudioSystem::setReverbZonePosition(int zoneId, float x, float y, float z) {
    auto it = m_reverbZones.find(zoneId);
    if (it != m_reverbZones.end()) {
        it->second.x = x;
        it->second.y = y;
        it->second.z = z;
    }
}

void SpatialAudioSystem::setReverbZoneRadius(int zoneId, float radius) {
    auto it = m_reverbZones.find(zoneId);
    if (it != m_reverbZones.end()) {
        it->second.radius = std::max(0.0f, radius);
    }
}

void SpatialAudioSystem::setReverbZonePreset(int zoneId, ReverbPreset preset) {
    auto it = m_reverbZones.find(zoneId);
    if (it != m_reverbZones.end()) {
        it->second.preset = preset;
        
        // Apply preset parameters
        switch (preset) {
            case ReverbPreset::Room:
                it->second.decayTime = 0.4f;
                it->second.density = 1.0f;
                it->second.diffusion = 1.0f;
                break;
            case ReverbPreset::Cave:
                it->second.decayTime = 2.9f;
                it->second.density = 1.0f;
                it->second.diffusion = 0.5f;
                break;
            case ReverbPreset::Cathedral:
                it->second.decayTime = 4.5f;
                it->second.density = 1.0f;
                it->second.diffusion = 1.0f;
                break;
            case ReverbPreset::Underwater:
                it->second.decayTime = 1.5f;
                it->second.density = 0.36f;
                it->second.diffusion = 1.0f;
                break;
            case ReverbPreset::Forest:
                it->second.decayTime = 0.15f;
                it->second.density = 0.5f;
                it->second.diffusion = 1.0f;
                break;
            case ReverbPreset::Hallway:
                it->second.decayTime = 1.5f;
                it->second.density = 1.0f;
                it->second.diffusion = 0.7f;
                break;
            default:
                break;
        }
    }
}

void SpatialAudioSystem::setReverbZoneParameters(int zoneId, float decayTime, float density, float diffusion) {
    auto it = m_reverbZones.find(zoneId);
    if (it != m_reverbZones.end()) {
        it->second.decayTime = std::max(0.1f, decayTime);
        it->second.density = std::clamp(density, 0.0f, 1.0f);
        it->second.diffusion = std::clamp(diffusion, 0.0f, 1.0f);
    }
}

void SpatialAudioSystem::destroyReverbZone(int zoneId) {
    m_reverbZones.erase(zoneId);
}

void SpatialAudioSystem::loadHRTFProfile(const std::string& profilePath) {
    m_hrtfProfile = profilePath;
    // Load HRTF profile from file
}

void SpatialAudioSystem::setOcclusion(int emitterId, float occlusion) {
    auto it = m_emitters.find(emitterId);
    if (it != m_emitters.end()) {
        // Apply occlusion filter
    }
}

float SpatialAudioSystem::calculateDistance(const AudioEmitter& emitter) const {
    float dx = emitter.x - m_listener.x;
    float dy = emitter.y - m_listener.y;
    float dz = emitter.z - m_listener.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float SpatialAudioSystem::calculateAttenuation(const AudioEmitter& emitter) const {
    if (!emitter.is3D) {
        return 1.0f;
    }
    
    float distance = calculateDistance(emitter);
    
    if (distance <= emitter.minDistance) {
        return 1.0f;
    }
    
    if (distance >= emitter.maxDistance) {
        return 0.0f;
    }
    
    float attenuation = 1.0f;
    
    switch (emitter.attenuationModel) {
        case AttenuationModel::Linear:
            attenuation = 1.0f - (distance - emitter.minDistance) / (emitter.maxDistance - emitter.minDistance);
            break;
            
        case AttenuationModel::Inverse:
            attenuation = emitter.minDistance / (emitter.minDistance + emitter.rolloffFactor * (distance - emitter.minDistance));
            break;
            
        case AttenuationModel::Exponential:
            attenuation = std::pow(distance / emitter.minDistance, -emitter.rolloffFactor);
            break;
            
        case AttenuationModel::Logarithmic:
            attenuation = 1.0f - emitter.rolloffFactor * std::log10(distance / emitter.minDistance) / std::log10(emitter.maxDistance / emitter.minDistance);
            break;
    }
    
    return std::clamp(attenuation, 0.0f, 1.0f);
}

float SpatialAudioSystem::calculateDopplerShift(const AudioEmitter& emitter) const {
    if (!emitter.isDoppler || m_dopplerFactor == 0.0f) {
        return 1.0f;
    }
    
    // Calculate relative velocity
    float dx = emitter.x - m_listener.x;
    float dy = emitter.y - m_listener.y;
    float dz = emitter.z - m_listener.z;
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    
    if (distance < 0.001f) {
        return 1.0f;
    }
    
    // Normalize direction
    dx /= distance;
    dy /= distance;
    dz /= distance;
    
    // Relative velocity along direction
    float vr = (emitter.velocityX - m_listener.velocityX) * dx +
               (emitter.velocityY - m_listener.velocityY) * dy +
               (emitter.velocityZ - m_listener.velocityZ) * dz;
    
    // Doppler shift formula
    float dopplerShift = (m_speedOfSound - m_dopplerFactor * vr) / m_speedOfSound;
    
    return std::clamp(dopplerShift, 0.5f, 2.0f);
}

void SpatialAudioSystem::updateEmitterAudio(AudioEmitter& emitter, float deltaTime) {
    if (!emitter.is3D) {
        return;
    }
    
    // Calculate attenuation
    float attenuation = calculateAttenuation(emitter);
    
    // Calculate doppler shift
    float dopplerShift = calculateDopplerShift(emitter);
    
    // Apply occlusion
    float occlusion = m_occlusionEnabled ? calculateOcclusion(emitter) : 1.0f;
    
    // Apply reverb
    applyReverb(emitter);
    
    // Update audio backend with calculated parameters
    float finalVolume = emitter.volume * attenuation * occlusion * m_listener.volume * m_masterVolume;
    float finalPitch = emitter.pitch * dopplerShift;
    
    // Apply to audio backend
}

void SpatialAudioSystem::applyReverb(AudioEmitter& emitter) {
    ReverbZone* zone = findActiveReverbZone(emitter);
    if (zone) {
        // Apply reverb effect based on zone parameters
    }
}

float SpatialAudioSystem::calculateOcclusion(const AudioEmitter& emitter) {
    // Raycast from listener to emitter
    // Return 0.0 (fully occluded) to 1.0 (not occluded)
    return 1.0f;
}

ReverbZone* SpatialAudioSystem::findActiveReverbZone(const AudioEmitter& emitter) {
    for (auto& pair : m_reverbZones) {
        ReverbZone& zone = pair.second;
        
        float dx = emitter.x - zone.x;
        float dy = emitter.y - zone.y;
        float dz = emitter.z - zone.z;
        float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        if (distance <= zone.radius) {
            return &zone;
        }
    }
    
    return nullptr;
}

} // namespace Engine
