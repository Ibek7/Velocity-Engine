#include "audio/advanced/AdvancedAudioSystem.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <chrono>

namespace JJM {
namespace Audio {
namespace Advanced {

// -- Vector3D implementation
float Vector3D::magnitude() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3D Vector3D::normalized() const {
    float mag = magnitude();
    if (mag > 0.0f) return *this * (1.0f / mag);
    return Vector3D();
}

float Vector3D::dot(const Vector3D& other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vector3D Vector3D::cross(const Vector3D& other) const {
    return Vector3D(
        y * other.z - z * other.y,
        z * other.x - x * other.z,
        x * other.y - y * other.x
    );
}

float Vector3D::distance(const Vector3D& other) const {
    return (*this - other).magnitude();
}

// -- AudioBuffer implementation
AudioBuffer::AudioBuffer(size_t frames, size_t channels, size_t rate)
    : samples(nullptr), frameCount(0), channelCount(0), sampleRate(0) {
    allocate(frames, channels, rate);
}

AudioBuffer::~AudioBuffer() {
    deallocate();
}

void AudioBuffer::allocate(size_t frames, size_t channels, size_t rate) {
    if (samples) deallocate();
    frameCount = frames;
    channelCount = channels;
    sampleRate = rate;
    samples = new float[frames * channels];
    clear();
}

void AudioBuffer::deallocate() {
    if (samples) {
        delete[] samples;
        samples = nullptr;
    }
    frameCount = channelCount = sampleRate = 0;
}

void AudioBuffer::clear() {
    if (samples) std::memset(samples, 0, getSizeInBytes());
}

void AudioBuffer::copyFrom(const AudioBuffer& other) {
    if (frameCount != other.frameCount || channelCount != other.channelCount) {
        allocate(other.frameCount, other.channelCount, other.sampleRate);
    }
    std::memcpy(samples, other.samples, getSizeInBytes());
}

void AudioBuffer::mixWith(const AudioBuffer& other, float gain) {
    size_t framesToMix = std::min(frameCount, other.frameCount);
    size_t channelsToMix = std::min(channelCount, other.channelCount);
    
    for (size_t frame = 0; frame < framesToMix; ++frame) {
        for (size_t channel = 0; channel < channelsToMix; ++channel) {
            size_t index = frame * channelCount + channel;
            samples[index] += other.samples[frame * other.channelCount + channel] * gain;
        }
    }
}

float* AudioBuffer::getChannelData(size_t channel) {
    if (channel >= channelCount) return nullptr;
    return samples + channel;
}

const float* AudioBuffer::getChannelData(size_t channel) const {
    if (channel >= channelCount) return nullptr;
    return samples + channel;
}

// -- AudioListener implementation
AudioListener::AudioListener()
    : position(0, 0, 0), forward(0, 0, -1), up(0, 1, 0), right(1, 0, 0),
      velocity(0, 0, 0), gainMultiplier(1.0f), dopplerFactor(1.0f) {}

void AudioListener::setOrientation(const Vector3D& forward, const Vector3D& up) {
    this->forward = forward.normalized();
    this->up = up.normalized();
    this->right = this->forward.cross(this->up).normalized();
}

Vector3D AudioListener::worldToListener(const Vector3D& worldPos) const {
    Vector3D relative = worldPos - position;
    return Vector3D(
        relative.dot(right),
        relative.dot(up),
        relative.dot(forward)
    );
}

// -- AudioSource implementation
AudioSource::AudioSource()
    : position(0, 0, 0), velocity(0, 0, 0), direction(0, 0, -1),
      gain(1.0f), pitch(1.0f), referenceDistance(1.0f), maxDistance(100.0f), rolloffFactor(1.0f),
      attenuationModel(AttenuationModel::Inverse), directivityModel(DirectivityModel::Omnidirectional),
      looping(false), playing(false), spatialized(true), playbackPosition(0.0) {}

void AudioSource::play(const std::string& clipId) {
    audioClipId = clipId;
    playbackPosition = 0.0;
    playing = true;
}

void AudioSource::pause() {
    playing = false;
}

void AudioSource::stop() {
    playing = false;
    playbackPosition = 0.0;
}

float AudioSource::calculateAttenuation(float distance) const {
    if (distance <= referenceDistance) return 1.0f;
    if (distance >= maxDistance) return 0.0f;
    
    float normalizedDistance = (distance - referenceDistance) / (maxDistance - referenceDistance);
    
    switch (attenuationModel) {
        case AttenuationModel::None:
            return 1.0f;
        case AttenuationModel::Linear:
            return 1.0f - normalizedDistance;
        case AttenuationModel::Inverse:
            return referenceDistance / (referenceDistance + rolloffFactor * (distance - referenceDistance));
        case AttenuationModel::Exponential:
            return std::pow(referenceDistance / distance, rolloffFactor);
        case AttenuationModel::Custom:
            return customAttenuationFunc ? customAttenuationFunc(distance) : 1.0f;
    }
    return 1.0f;
}

float AudioSource::calculateDirectivity(const Vector3D& listenerPos) const {
    if (directivityModel == DirectivityModel::Omnidirectional) return 1.0f;
    
    Vector3D toListener = (listenerPos - position).normalized();
    float dotProduct = direction.dot(toListener);
    
    switch (directivityModel) {
        case DirectivityModel::Cardioid:
            return 0.5f * (1.0f + dotProduct);
        case DirectivityModel::Bidirectional:
            return std::abs(dotProduct);
        case DirectivityModel::Custom:
            return customDirectivityFunc ? customDirectivityFunc(dotProduct) : 1.0f;
        default:
            return 1.0f;
    }
}

// -- ReverbEffect implementation
ReverbEffect::ReverbEffect(size_t sampleRate) : sampleRate(sampleRate) {
    // Initialize default reverb parameters
    params.roomSize = 0.5f;
    params.damping = 0.5f;
    params.earlyReflections = 0.3f;
    params.lateDiffusion = 0.7f;
    params.preDelay = 20.0f;
    params.decayTime = 2.0f;
    params.highFreqDecay = 0.8f;
    params.lowFreqDecay = 1.2f;
    
    // Initialize delay lines and allpass filters
    delayLines.resize(8 * sampleRate / 1000);  // 8ms max delay
    delayIndices.resize(8, 0);
    allpassDelays.resize(4 * sampleRate / 1000);  // 4ms max allpass delay
    allpassIndices.resize(4, 0);
    combGains.resize(8, 0.7f);
}

void ReverbEffect::process(AudioBuffer& buffer) {
    if (!enabled) return;
    
    for (size_t frame = 0; frame < buffer.frameCount; ++frame) {
        for (size_t channel = 0; channel < buffer.channelCount; ++channel) {
            float input = buffer.samples[frame * buffer.channelCount + channel];
            float output = input * dryLevel;
            
            // Simple reverb processing (placeholder)
            // Real implementation would use proper comb and allpass filters
            float reverbSample = input * params.roomSize * 0.3f;
            output += reverbSample * wetLevel;
            
            buffer.samples[frame * buffer.channelCount + channel] = output;
        }
    }
}

void ReverbEffect::reset() {
    std::fill(delayLines.begin(), delayLines.end(), 0.0f);
    std::fill(delayIndices.begin(), delayIndices.end(), 0);
    std::fill(allpassDelays.begin(), allpassDelays.end(), 0.0f);
    std::fill(allpassIndices.begin(), allpassIndices.end(), 0);
}

void ReverbEffect::setParameters(const ReverbParameters& p) {
    params = p;
    // Recalculate internal parameters
}

ReverbEffect::ReverbParameters ReverbEffect::getPresetParameters(const std::string& presetName) {
    ReverbParameters preset;
    
    if (presetName == "hall") {
        preset.roomSize = 0.9f;
        preset.decayTime = 4.0f;
        preset.damping = 0.3f;
    } else if (presetName == "room") {
        preset.roomSize = 0.5f;
        preset.decayTime = 1.5f;
        preset.damping = 0.6f;
    } else if (presetName == "plate") {
        preset.roomSize = 0.3f;
        preset.decayTime = 2.5f;
        preset.damping = 0.2f;
    }
    
    return preset;
}

// -- EchoEffect implementation
EchoEffect::EchoEffect(size_t sampleRate) 
    : sampleRate(sampleRate), delayTime(0.5f), feedback(0.3f), writeIndex(0) {
    setDelayTime(delayTime);
}

void EchoEffect::process(AudioBuffer& buffer) {
    if (!enabled) return;
    
    for (size_t frame = 0; frame < buffer.frameCount; ++frame) {
        for (size_t channel = 0; channel < buffer.channelCount; ++channel) {
            float input = buffer.samples[frame * buffer.channelCount + channel];
            
            // Read from delay buffer
            size_t readIndex = writeIndex;
            float delayed = delayBuffer[readIndex];
            
            // Write to delay buffer with feedback
            delayBuffer[writeIndex] = input + delayed * feedback;
            
            // Output mix
            float output = input * dryLevel + delayed * wetLevel;
            buffer.samples[frame * buffer.channelCount + channel] = output;
            
            // Advance write index
            writeIndex = (writeIndex + 1) % delayBufferSize;
        }
    }
}

void EchoEffect::reset() {
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    writeIndex = 0;
}

void EchoEffect::setDelayTime(float timeSeconds) {
    delayTime = std::clamp(timeSeconds, 0.001f, 5.0f);
    delayBufferSize = static_cast<size_t>(delayTime * sampleRate);
    delayBuffer.resize(delayBufferSize, 0.0f);
    writeIndex = 0;
}

// -- EqualizerEffect implementation
EqualizerEffect::EqualizerEffect(size_t sampleRate) : sampleRate(sampleRate) {
    // Add default EQ bands (standard 10-band EQ)
    std::vector<float> frequencies = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    for (float freq : frequencies) {
        bands.push_back(EQBand(freq, 0.0f, 1.0f));
        filterStates.push_back({0.0f, 0.0f, 0.0f});
    }
}

void EqualizerEffect::process(AudioBuffer& buffer) {
    if (!enabled) return;
    
    for (size_t frame = 0; frame < buffer.frameCount; ++frame) {
        for (size_t channel = 0; channel < buffer.channelCount; ++channel) {
            float sample = buffer.samples[frame * buffer.channelCount + channel];
            
            // Process through each EQ band
            for (size_t band = 0; band < bands.size(); ++band) {
                if (bands[band].enabled && bands[band].gain != 0.0f) {
                    sample = processBiquad(sample, band);
                }
            }
            
            buffer.samples[frame * buffer.channelCount + channel] = sample;
        }
    }
}

void EqualizerEffect::reset() {
    for (auto& state : filterStates) {
        state.fill(0.0f);
    }
}

float EqualizerEffect::processBiquad(float input, size_t bandIndex) {
    // Simple gain application - real implementation would use proper biquad filters
    if (bandIndex >= bands.size()) return input;
    
    float gainLinear = AudioDSP::dbToLinear(bands[bandIndex].gain);
    return input * gainLinear;
}

void EqualizerEffect::addBand(const EQBand& band) {
    bands.push_back(band);
    filterStates.push_back({0.0f, 0.0f, 0.0f});
}

void EqualizerEffect::setBandGain(size_t index, float gainDB) {
    if (index < bands.size()) {
        bands[index].gain = std::clamp(gainDB, -20.0f, 20.0f);
    }
}

// -- AudioBus implementation
AudioBus::AudioBus(const std::string& name, size_t bufferSize, size_t channels, size_t sampleRate)
    : name(name), gain(1.0f), muted(false), soloed(false), parentBus(nullptr) {
    mixBuffer.allocate(bufferSize, channels, sampleRate);
}

AudioBus::~AudioBus() {
    clearEffects();
}

void AudioBus::addSource(AudioSource* source) {
    if (source && std::find(sources.begin(), sources.end(), source) == sources.end()) {
        sources.push_back(source);
    }
}

void AudioBus::removeSource(AudioSource* source) {
    auto it = std::find(sources.begin(), sources.end(), source);
    if (it != sources.end()) {
        sources.erase(it);
    }
}

void AudioBus::addEffect(std::unique_ptr<AudioEffect> effect) {
    if (effect) {
        effects.push_back(std::move(effect));
    }
}

void AudioBus::removeEffect(size_t index) {
    if (index < effects.size()) {
        effects.erase(effects.begin() + index);
    }
}

void AudioBus::clearEffects() {
    effects.clear();
}

AudioEffect* AudioBus::getEffect(size_t index) {
    return (index < effects.size()) ? effects[index].get() : nullptr;
}

void AudioBus::addChildBus(std::unique_ptr<AudioBus> childBus) {
    if (childBus) {
        childBus->parentBus = this;
        childBuses.push_back(std::move(childBus));
    }
}

AudioBus* AudioBus::getChildBus(const std::string& name) {
    for (auto& child : childBuses) {
        if (child->name == name) return child.get();
    }
    return nullptr;
}

void AudioBus::process(AudioBuffer& outputBuffer, const AudioListener& listener) {
    if (muted) return;
    
    mixBuffer.clear();
    
    // Process child buses first
    for (auto& child : childBuses) {
        child->process(mixBuffer, listener);
    }
    
    // Mix sources into buffer (simplified)
    for (AudioSource* source : sources) {
        if (source && source->isPlaying()) {
            // Here we would actually read from the audio clip and spatialize
            // For now, just add some simple processing
        }
    }
    
    // Apply effects chain
    for (auto& effect : effects) {
        if (effect && effect->isEnabled()) {
            effect->process(mixBuffer);
        }
    }
    
    // Mix into output buffer with bus gain
    outputBuffer.mixWith(mixBuffer, gain);
}

void AudioBus::reset() {
    for (auto& effect : effects) {
        if (effect) effect->reset();
    }
    for (auto& child : childBuses) {
        child->reset();
    }
}

// -- AudioSpatializer implementation
AudioSpatializer::AudioSpatializer(SpatializerType type, size_t sampleRate)
    : type(type), sampleRate(sampleRate), speedOfSound(343.0f) {}

AudioSpatializer::~AudioSpatializer() = default;

AudioSpatializer::SpatializationResult AudioSpatializer::spatialize(const AudioSource& source, const AudioListener& listener) {
    switch (type) {
        case SpatializerType::Simple:
            return spatializeSimple(source, listener);
        case SpatializerType::HRTF:
            return spatializeHRTF(source, listener);
        case SpatializerType::Ambisonics:
            return spatializeAmbisonics(source, listener);
        case SpatializerType::Binaural:
            return spatializeBinaural(source, listener);
    }
    return {};
}

AudioSpatializer::SpatializationResult AudioSpatializer::spatializeSimple(const AudioSource& source, const AudioListener& listener) {
    SpatializationResult result;
    
    Vector3D relativePos = listener.worldToListener(source.getPosition());
    float distance = relativePos.magnitude();
    
    // Simple panning based on left/right position
    float pan = std::clamp(relativePos.x / 10.0f, -1.0f, 1.0f);  // Arbitrary scale
    
    result.leftGain = (1.0f - pan) * 0.5f + 0.5f;
    result.rightGain = (1.0f + pan) * 0.5f + 0.5f;
    result.delay = distance / speedOfSound * sampleRate;  // Convert to samples
    result.dopplerShift = calculateDopplerShift(source, listener);
    
    // Apply attenuation
    float attenuation = source.calculateAttenuation(distance);
    result.leftGain *= attenuation;
    result.rightGain *= attenuation;
    
    return result;
}

AudioSpatializer::SpatializationResult AudioSpatializer::spatializeHRTF(const AudioSource& source, const AudioListener& listener) {
    // HRTF implementation would be more complex
    // For now, fall back to simple spatialization
    return spatializeSimple(source, listener);
}

AudioSpatializer::SpatializationResult AudioSpatializer::spatializeAmbisonics(const AudioSource& source, const AudioListener& listener) {
    // Ambisonic encoding would go here
    return spatializeSimple(source, listener);
}

AudioSpatializer::SpatializationResult AudioSpatializer::spatializeBinaural(const AudioSource& source, const AudioListener& listener) {
    // Binaural rendering would go here
    return spatializeSimple(source, listener);
}

float AudioSpatializer::calculateDopplerShift(const AudioSource& source, const AudioListener& listener) {
    Vector3D relativeVelocity = source.getVelocity() - listener.getVelocity();
    Vector3D direction = (source.getPosition() - listener.getPosition()).normalized();
    
    float velocityComponent = relativeVelocity.dot(direction);
    return (speedOfSound - velocityComponent) / speedOfSound;
}

// -- AdvancedAudioEngine implementation
AdvancedAudioEngine* AdvancedAudioEngine::instance = nullptr;

AdvancedAudioEngine::AdvancedAudioEngine()
    : initialized(false), sampleRate(44100), bufferSize(1024),
      outputConfig(AudioChannelConfig::Stereo), shouldStop(false),
      cpuUsage(0.0f), voiceCount(0) {}

AdvancedAudioEngine* AdvancedAudioEngine::getInstance() {
    if (!instance) {
        instance = new AdvancedAudioEngine();
    }
    return instance;
}

AdvancedAudioEngine::~AdvancedAudioEngine() {
    shutdown();
}

bool AdvancedAudioEngine::initialize(size_t sampleRate, size_t bufferSize, AudioChannelConfig config) {
    if (initialized) return true;
    
    this->sampleRate = sampleRate;
    this->bufferSize = bufferSize;
    this->outputConfig = config;
    
    listener = std::make_unique<AudioListener>();
    spatializer = std::make_unique<AudioSpatializer>(SpatializerType::Simple, sampleRate);
    masterBus = std::make_unique<AudioBus>("Master", bufferSize, static_cast<size_t>(config), sampleRate);
    
    shouldStop = false;
    processingThread = std::thread(&AdvancedAudioEngine::audioProcessingLoop, this);
    
    initialized = true;
    return true;
}

void AdvancedAudioEngine::shutdown() {
    if (!initialized) return;
    
    shouldStop = true;
    processingCV.notify_all();
    
    if (processingThread.joinable()) {
        processingThread.join();
    }
    
    sources.clear();
    audioClips.clear();
    
    initialized = false;
}

AudioSource* AdvancedAudioEngine::createSource() {
    std::lock_guard<std::mutex> lock(audioMutex);
    sources.push_back(std::make_unique<AudioSource>());
    return sources.back().get();
}

void AdvancedAudioEngine::destroySource(AudioSource* source) {
    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = std::find_if(sources.begin(), sources.end(),
        [source](const std::unique_ptr<AudioSource>& ptr) { return ptr.get() == source; });
    if (it != sources.end()) {
        sources.erase(it);
    }
}

bool AdvancedAudioEngine::loadAudioClip(const std::string& id, const std::string& filePath) {
    // Audio file loading would be implemented here
    // For now, create a simple test buffer
    auto buffer = std::make_unique<AudioBuffer>(sampleRate, 1, sampleRate);  // 1 second mono
    
    // Fill with a simple sine wave for testing
    float frequency = 440.0f;  // A note
    for (size_t i = 0; i < buffer->frameCount; ++i) {
        buffer->samples[i] = std::sin(2.0f * M_PI * frequency * i / sampleRate) * 0.5f;
    }
    
    return addAudioClip(id, std::move(buffer));
}

bool AdvancedAudioEngine::addAudioClip(const std::string& id, std::unique_ptr<AudioBuffer> buffer) {
    std::lock_guard<std::mutex> lock(audioMutex);
    audioClips[id] = std::move(buffer);
    return true;
}

void AdvancedAudioEngine::audioProcessingLoop() {
    AudioBuffer tempBuffer(bufferSize, static_cast<size_t>(outputConfig), sampleRate);
    
    while (!shouldStop) {
        auto start = std::chrono::high_resolution_clock::now();
        
        {
            std::lock_guard<std::mutex> lock(audioMutex);
            tempBuffer.clear();
            
            if (masterBus && listener) {
                masterBus->process(tempBuffer, *listener);
            }
            
            voiceCount = 0;
            for (const auto& source : sources) {
                if (source->isPlaying()) voiceCount++;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        float frameTimeMs = static_cast<float>(bufferSize) / sampleRate * 1000.0f;
        cpuUsage = static_cast<float>(duration.count()) / 1000.0f / frameTimeMs;
        
        // Sleep to simulate real-time processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// -- AudioDSP utility functions
namespace AudioDSP {

float lowpass(float input, float cutoff, float sampleRate, float& state) {
    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = dt / (rc + dt);
    state = state + alpha * (input - state);
    return state;
}

float highpass(float input, float cutoff, float sampleRate, float& state) {
    float rc = 1.0f / (2.0f * M_PI * cutoff);
    float dt = 1.0f / sampleRate;
    float alpha = rc / (rc + dt);
    state = alpha * (state + input - state);
    return input - state;
}

float linearInterpolate(float a, float b, float t) {
    return a + t * (b - a);
}

float cubicInterpolate(float a, float b, float c, float d, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    return a * (-0.5f * t3 + t2 - 0.5f * t) +
           b * (1.5f * t3 - 2.5f * t2 + 1.0f) +
           c * (-1.5f * t3 + 2.0f * t2 + 0.5f * t) +
           d * (0.5f * t3 - 0.5f * t2);
}

float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

float linearToDb(float linear) {
    return 20.0f * std::log10(std::max(linear, 1e-6f));
}

float semitonesToRatio(float semitones) {
    return std::pow(2.0f, semitones / 12.0f);
}

float ratioToSemitones(float ratio) {
    return 12.0f * std::log2(ratio);
}

float hannWindow(size_t index, size_t length) {
    if (length == 0) return 0.0f;
    return 0.5f * (1.0f - std::cos(2.0f * M_PI * index / (length - 1)));
}

float blackmanWindow(size_t index, size_t length) {
    if (length == 0) return 0.0f;
    float n = static_cast<float>(index) / (length - 1);
    return 0.42f - 0.5f * std::cos(2.0f * M_PI * n) + 0.08f * std::cos(4.0f * M_PI * n);
}

} // namespace AudioDSP

} // namespace Advanced
} // namespace Audio
} // namespace JJM